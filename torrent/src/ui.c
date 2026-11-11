/* Win32 user interface. All network work runs in a worker thread (tr_announce). */

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shellapi.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "history.h"
#include "torrent.h"
#include "tracker.h"
#include "ui.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' " \
                        "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define WM_APP_LOG (WM_APP + 1)
#define WM_APP_DONE (WM_APP + 2)

enum {
    ID_OPEN = 100,
    ID_COPY_HASH,
    ID_INFO_LV,
    ID_TRACKERS_LV,
    ID_MANUAL_URL,
    ID_ADD,
    ID_INFO_HASH,
    ID_PEER_ID,
    ID_REGEN_PEER,
    ID_PORT,
    ID_UPLOADED,
    ID_DOWNLOADED,
    ID_LEFT,
    ID_NUMWANT,
    ID_KEY,
    ID_REGEN_KEY,
    ID_EVENT,
    ID_COMPACT,
    ID_NOVERIFY,
    ID_ANNOUNCE,
    ID_CANCEL,
    ID_STATUS,
    ID_HISTORY_LV,
    ID_CLEAR_HIST,
    ID_TAB,
    ID_RESP_LV,
    ID_COPY_REQ,
    ID_REQ_EDIT,
    ID_PEERS_LABEL,
    ID_PEERS_SEARCH,
    ID_PEERS_LV,
    ID_RAW_EDIT,
    ID_LOG,
    ID_LABEL, /* all static labels */
};

/* Row colors (stored in the ListView item lParam). */
enum { C_NORMAL, C_ERROR, C_WARN, C_NA, C_OK };

static COLORREF color_of(int c)
{
    switch (c) {
    case C_ERROR: return RGB(200, 30, 40);
    case C_WARN: return RGB(200, 95, 0);
    case C_NA: return RGB(160, 160, 160);
    case C_OK: return RGB(20, 135, 60);
    }
    return GetSysColor(COLOR_WINDOWTEXT);
}

typedef struct {
    char *url;
    char tier[24];
    char transport[16];
    tr_state state;
    history hist;
    char status[48];
    char sl[48];
    int status_err;
} TrackerEntry;

typedef struct {
    TrackerEntry *entry;
    char *url;
    char *trackerid;
    tr_params params;
    char time[32];
    tr_result res;
} Job;

typedef struct {
    int level;
    char ts[32];
    char *text;
} LogMsg;

static struct {
    HINSTANCE inst;
    HWND hwnd;
    UINT dpi;
    HFONT font, font_bold, font_mono;
    HMODULE richedit;

    HWND open, lbl_torrent, copy_hash, info_lv;
    HWND lbl_trackers, trackers_lv, manual_url, add;
    HWND lbl_params, lbl_ih, e_info_hash, lbl_pid, e_peer_id, regen_peer, lbl_key, e_key, regen_key, lbl_event,
        cb_event, lbl_port, e_port, lbl_numwant, e_numwant, lbl_up, e_uploaded, lbl_down, e_downloaded, lbl_left,
        e_left, chk_compact, chk_noverify;
    HWND announce, cancel, status;
    HWND lbl_history, clear_hist, history_lv, tab, resp_lv, copy_req, req_edit, peers_label, peers_search, peers_lv,
        raw_edit;
    HWND lbl_log, log;
    int status_err;

    /* Copy of the peers of the announce on display (the search box filters them). */
    tr_peer *shown_peers;
    size_t nshown;
    size_t shown_trailing;

    torrent_info tor;
    int has_torrent;
    TrackerEntry **trackers;
    size_t ntrackers;
    int updating;
    int closing;
    char *copy_text;

    tr_ctx ctx;
    volatile long cancel_flag;
    HANDLE thread;
    Job *job;
} g;

static const tr_event event_values[] = { TR_EVENT_STARTED, TR_EVENT_COMPLETED, TR_EVENT_STOPPED, TR_EVENT_NONE };

/* ======================================================================
 * Small helpers
 * ====================================================================== */

static int S(int v)
{
    return MulDiv(v, (int)g.dpi, 96);
}

static void now_str(char out[32])
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    snprintf(out, 32, "%02u:%02u:%02u.%03u", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
}

static void set_text(HWND h, const char *utf8)
{
    wchar_t *w = utf8_to_wide(utf8);
    SetWindowTextW(h, w ? w : L"");
    free(w);
}

/* Multi-line edit controls need \r\n line breaks. */
static void set_edit_text(HWND h, const char *utf8)
{
    bbuf b;
    bbuf_init(&b);
    for (const char *p = utf8 ? utf8 : ""; *p; p++) {
        if (*p == '\n' && (p == utf8 || p[-1] != '\r'))
            bbuf_putc(&b, '\r');
        bbuf_putc(&b, (unsigned char)*p);
    }
    char *s = bbuf_steal(&b);
    set_text(h, s);
    free(s);
}

static char *get_text(HWND h)
{
    int n = GetWindowTextLengthW(h);
    wchar_t *w = malloc(((size_t)n + 1) * sizeof *w);
    if (!w)
        return xstrdup("");
    GetWindowTextW(h, w, n + 1);
    char *u = wide_to_utf8(w);
    free(w);
    return u ? u : xstrdup("");
}

static char *trim(char *s)
{
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    size_t n = strlen(p);
    while (n && (p[n - 1] == ' ' || p[n - 1] == '\t' || p[n - 1] == '\r' || p[n - 1] == '\n'))
        n--;
    memmove(s, p, n);
    s[n] = 0;
    return s;
}

static char *get_trimmed(HWND h)
{
    return trim(get_text(h));
}

static void copy_to_clipboard(const char *utf8)
{
    if (!utf8 || !utf8[0])
        return;
    wchar_t *w = utf8_to_wide(utf8);
    if (!w)
        return;
    size_t bytes = (wcslen(w) + 1) * sizeof(wchar_t);
    if (OpenClipboard(g.hwnd)) {
        EmptyClipboard();
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (h) {
            void *dst = GlobalLock(h);
            if (dst) {
                memcpy(dst, w, bytes);
                GlobalUnlock(h);
                if (!SetClipboardData(CF_UNICODETEXT, h))
                    GlobalFree(h);
            } else {
                GlobalFree(h);
            }
        }
        CloseClipboard();
    }
    free(w);
}

static void fmt_size(char *out, size_t n, int64_t b)
{
    static const char *const units[] = { "o", "Kio", "Mio", "Gio", "Tio" };
    double v = (double)b;
    int i = 0;
    while (v >= 1024 && i < 4) {
        v /= 1024;
        i++;
    }
    if (i == 0)
        snprintf(out, n, "%" PRId64 " o", b);
    else
        snprintf(out, n, "%.2f %s (%" PRId64 " octets)", v, units[i], b);
}

/* ---- ListView ---- */

static HWND make_listview(int id, const wchar_t *const *cols, const int *widths, int n)
{
    HWND lv = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0,
                              0, 0, 0, g.hwnd, (HMENU)(INT_PTR)id, g.inst, NULL);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
    for (int i = 0; i < n; i++) {
        LVCOLUMNW c;
        memset(&c, 0, sizeof c);
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)cols[i];
        c.cx = S(widths[i]);
        ListView_InsertColumn(lv, i, &c);
    }
    return lv;
}

static void lv_set(HWND lv, int row, int insert, int color, int n, const char *const *cols)
{
    wchar_t *w = utf8_to_wide(cols[0]);
    LVITEMW it;
    memset(&it, 0, sizeof it);
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = row;
    it.pszText = w;
    it.lParam = color;
    if (insert)
        row = ListView_InsertItem(lv, &it);
    else
        ListView_SetItem(lv, &it);
    free(w);
    for (int i = 1; i < n; i++) {
        w = utf8_to_wide(cols[i]);
        ListView_SetItemText(lv, row, i, w);
        free(w);
    }
}

static void lv_add(HWND lv, int color, int n, const char *const *cols)
{
    lv_set(lv, ListView_GetItemCount(lv), 1, color, n, cols);
}

static int lv_sel(HWND lv)
{
    return ListView_GetNextItem(lv, -1, LVNI_SELECTED);
}

static void lv_select(HWND lv, int i)
{
    ListView_SetItemState(lv, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (i >= 0) {
        ListView_SetItemState(lv, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(lv, i, FALSE);
    }
}

static char *lv_text(HWND lv, int row, int col)
{
    wchar_t buf[4096];
    buf[0] = 0;
    ListView_GetItemText(lv, row, col, buf, (int)(sizeof buf / sizeof buf[0]));
    return wide_to_utf8(buf);
}

/* Gives column `col` all the width the other columns leave. */
static void lv_stretch(HWND lv, int col)
{
    HWND hdr = ListView_GetHeader(lv);
    int n = Header_GetItemCount(hdr);
    RECT rc;
    GetClientRect(lv, &rc);
    int used = 0;
    for (int i = 0; i < n; i++)
        if (i != col)
            used += ListView_GetColumnWidth(lv, i);
    /* Keep room for a vertical scrollbar so no horizontal one appears. */
    int w = rc.right - used;
    if (!(GetWindowLongPtrW(lv, GWL_STYLE) & WS_VSCROLL))
        w -= GetSystemMetricsForDpi(SM_CXVSCROLL, g.dpi);
    ListView_SetColumnWidth(lv, col, w > S(80) ? w : S(80));
}

/* ======================================================================
 * Log and status
 * ====================================================================== */

static void re_append(const char *utf8, COLORREF color)
{
    wchar_t *w = utf8_to_wide(utf8);
    if (!w)
        return;
    CHARRANGE cr = { -1, -1 };
    SendMessageW(g.log, EM_EXSETSEL, 0, (LPARAM)&cr);
    CHARFORMAT2W cf;
    memset(&cf, 0, sizeof cf);
    cf.cbSize = sizeof cf;
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessageW(g.log, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(g.log, EM_REPLACESEL, FALSE, (LPARAM)w);
    free(w);
}

static void log_line(int level, const char *ts, const char *msg)
{
    if (g.closing || !g.log)
        return;
    SendMessageW(g.log, EM_SETREADONLY, FALSE, 0);
    char head[48];
    snprintf(head, sizeof head, "%s  ", ts);
    re_append(head, RGB(128, 128, 128));
    re_append(msg, level == TR_LOG_ERROR ? color_of(C_ERROR) : level == TR_LOG_WARN ? color_of(C_WARN)
                                                                                     : GetSysColor(COLOR_WINDOWTEXT));
    re_append("\r\n", GetSysColor(COLOR_WINDOWTEXT));
    SendMessageW(g.log, EM_SETREADONLY, TRUE, 0);
    SendMessageW(g.log, WM_VSCROLL, SB_BOTTOM, 0);
}

static void log_now(int level, const char *fmt, ...) PRINTF_FMT(2, 3);
static void log_now(int level, const char *fmt, ...)
{
    char ts[32], msg[2048];
    now_str(ts);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    log_line(level, ts, msg);
}

/* Called from the worker thread: hand the message to the UI thread. */
static void worker_log(void *ud, int level, const char *msg)
{
    (void)ud;
    LogMsg *m = calloc(1, sizeof *m);
    if (!m)
        return;
    m->level = level;
    now_str(m->ts);
    m->text = xstrdup(msg);
    if (!PostMessageW(g.hwnd, WM_APP_LOG, 0, (LPARAM)m)) {
        free(m->text);
        free(m);
    }
}

static void set_status(int error, const char *fmt, ...) PRINTF_FMT(2, 3);
static void set_status(int error, const char *fmt, ...)
{
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    g.status_err = error;
    set_text(g.status, msg);
    InvalidateRect(g.status, NULL, TRUE);
}

static void set_busy(int busy)
{
    EnableWindow(g.announce, !busy);
    EnableWindow(g.cancel, busy);
    EnableWindow(g.open, !busy);
    EnableWindow(g.add, !busy);
    EnableWindow(g.clear_hist, !busy);
}

/* ======================================================================
 * Trackers and history
 * ====================================================================== */

static TrackerEntry *selected_tracker(void)
{
    int i = lv_sel(g.trackers_lv);
    return (i >= 0 && (size_t)i < g.ntrackers) ? g.trackers[i] : NULL;
}

static void tracker_row(TrackerEntry *e, int pos)
{
    const char *cols[] = { e->tier, e->url, e->transport, e->status, e->sl };
    int color = e->status_err ? C_ERROR : strcmp(e->status, "OK") == 0 ? C_OK : C_NORMAL;
    if (pos < 0)
        lv_add(g.trackers_lv, color, 5, cols);
    else
        lv_set(g.trackers_lv, pos, 0, color, 5, cols);
}

static int add_tracker_entry(const char *url, const char *tier)
{
    for (size_t i = 0; i < g.ntrackers; i++)
        if (strcmp(g.trackers[i]->url, url) == 0)
            return (int)i;
    TrackerEntry **arr = realloc(g.trackers, (g.ntrackers + 1) * sizeof *arr);
    TrackerEntry *e = calloc(1, sizeof *e);
    if (!arr || !e) {
        free(e);
        if (arr)
            g.trackers = arr;
        return -1;
    }
    g.trackers = arr;
    e->url = xstrdup(url);
    snprintf(e->tier, sizeof e->tier, "%s", tier);
    tr_url u;
    char err[128];
    if (tr_parse_url(url, &u, err, sizeof err) == 0)
        snprintf(e->transport, sizeof e->transport, "%s", u.transport == TR_UDP ? "UDP" : u.tls ? "HTTPS" : "HTTP");
    else
        snprintf(e->transport, sizeof e->transport, "invalide");
    snprintf(e->status, sizeof e->status, "jamais testé");
    snprintf(e->sl, sizeof e->sl, "—");
    history_init(&e->hist);
    g.trackers[g.ntrackers++] = e;
    tracker_row(e, -1);
    return (int)g.ntrackers - 1;
}

static void free_trackers(void)
{
    for (size_t i = 0; i < g.ntrackers; i++) {
        TrackerEntry *e = g.trackers[i];
        free(e->url);
        tr_state_free(&e->state);
        history_clear(&e->hist);
        free(e);
    }
    free(g.trackers);
    g.trackers = NULL;
    g.ntrackers = 0;
}

static void clear_trackers(void)
{
    g.updating++;
    ListView_DeleteAllItems(g.trackers_lv);
    free_trackers();
    g.updating--;
}

/* ---- details panes ---- */

enum {
    F_TRANSPORT, F_STATUS, F_HTTP_CODE, F_UDP_ACTION, F_TIME, F_INTERVAL, F_MIN_INTERVAL, F_COMPLETE, F_INCOMPLETE,
    F_TRACKER_ID, F_WARNING, F_ERROR, F_DOWNLOADED, F_EXTERNAL_IP, F_RETRY_IN, F_CRYPTO, F_OTHER, F_PEERS, F_CONN_ID,
    F_RETRANS, F_COUNT
};

static const char *const field_names[F_COUNT] = {
    "Transport",
    "Statut",
    "Code HTTP",
    "Action UDP",
    "Temps de réponse",
    "interval",
    "min interval",
    "complete (seeders)",
    "incomplete (leechers)",
    "tracker id",
    "warning message",
    "failure reason / erreur",
    "downloaded (terminés)",
    "external ip (ton IP vue)",
    "retry in",
    "crypto_flags",
    "Autres champs",
    "Peers reçus",
    "connection_id",
    "Retransmissions",
};

static int field_applies(int f, tr_transport t)
{
    if (t == TR_HTTP)
        return f != F_UDP_ACTION && f != F_CONN_ID && f != F_RETRANS;
    return f != F_HTTP_CODE && f != F_MIN_INTERVAL && f != F_TRACKER_ID && f != F_WARNING && f != F_DOWNLOADED &&
           f != F_EXTERNAL_IP && f != F_RETRY_IN && f != F_CRYPTO && f != F_OTHER;
}

/* Case-insensitive (ASCII) substring test. */
static int ci_contains(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    if (!n)
        return 1;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < n && hay[i] && tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == n)
            return 1;
    }
    return 0;
}

/* Fills the Peers table from shown_peers, keeping the rows that match the search box. */
static void fill_peers(void)
{
    char *filter = get_trimmed(g.peers_search);
    size_t shown = 0;
    SendMessageW(g.peers_lv, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.peers_lv);
    for (size_t i = 0; i < g.nshown; i++) {
        const tr_peer *p = &g.shown_peers[i];
        char idx[16], port[8], pid[100], addr[96];
        snprintf(idx, sizeof idx, "%zu", i + 1);
        snprintf(port, sizeof port, "%u", p->port);
        snprintf(addr, sizeof addr, strchr(p->ip, ':') ? "[%s]:%u" : "%s:%u", p->ip, p->port);
        if (p->peer_id_len)
            tr_peer_id_str(p->peer_id, p->peer_id_len, pid, sizeof pid);
        else
            snprintf(pid, sizeof pid, "—");
        if (filter[0] && !ci_contains(addr, filter) && !(p->peer_id_len && ci_contains(pid, filter)))
            continue;
        const char *cols[] = { idx, p->ip, port, pid };
        lv_add(g.peers_lv, C_NORMAL, 4, cols);
        shown++;
    }
    SendMessageW(g.peers_lv, WM_SETREDRAW, TRUE, 0);

    char buf[256];
    if (!g.nshown && !g.shown_trailing)
        snprintf(buf, sizeof buf, "Aucun peer.");
    else if (filter[0])
        snprintf(buf, sizeof buf, "%zu peer(s) affiché(s) sur %zu", shown, g.nshown);
    else
        snprintf(buf, sizeof buf, "%zu peer(s). Double-clic pour copier ip:port.", g.nshown);
    if (g.shown_trailing) {
        size_t len = strlen(buf);
        snprintf(buf + len, sizeof buf - len, " Attention : %zu octet(s) en trop dans la liste compacte.",
                 g.shown_trailing);
    }
    set_text(g.peers_label, buf);
    free(filter);
}

static void set_shown_peers(const tr_peer *peers, size_t n, size_t trailing)
{
    free(g.shown_peers);
    g.shown_peers = NULL;
    g.nshown = 0;
    if (n) {
        g.shown_peers = malloc(n * sizeof *g.shown_peers);
        if (g.shown_peers) {
            memcpy(g.shown_peers, peers, n * sizeof *g.shown_peers);
            g.nshown = n;
        }
    }
    g.shown_trailing = trailing;
}

static void clear_details(void)
{
    ListView_DeleteAllItems(g.resp_lv);
    for (int f = 0; f < F_COUNT; f++) {
        const char *cols[] = { field_names[f], "—" };
        lv_add(g.resp_lv, C_NORMAL, 2, cols);
    }
    set_edit_text(g.req_edit, "");
    set_edit_text(g.raw_edit, "");
    set_shown_peers(NULL, 0, 0);
    ListView_DeleteAllItems(g.peers_lv);
    set_text(g.peers_label, "Aucun announce sélectionné");
    free(g.copy_text);
    g.copy_text = NULL;
}

static const char *udp_action_name(int action)
{
    switch (action) {
    case 0: return "connect";
    case 1: return "announce";
    case 2: return "scrape";
    case 3: return "error";
    }
    return "inconnue";
}

static void show_result(const tr_result *r)
{
    char val[F_COUNT][600];
    int color[F_COUNT];
    for (int f = 0; f < F_COUNT; f++) {
        snprintf(val[f], sizeof val[f], "—");
        color[f] = C_NORMAL;
    }
    int is_http = r->transport == TR_HTTP;
    if (r->status != TR_ERR_URL)
        snprintf(val[F_TRANSPORT], sizeof val[0], "%s",
                 is_http ? (r->url && _strnicmp(r->url, "https", 5) == 0 ? "HTTPS" : "HTTP") : "UDP");
    snprintf(val[F_STATUS], sizeof val[0], "%s", tr_status_name(r->status));
    color[F_STATUS] = r->status == TR_OK ? C_OK : C_ERROR;
    if (r->http_code) {
        snprintf(val[F_HTTP_CODE], sizeof val[0], "%ld", r->http_code);
        color[F_HTTP_CODE] = r->http_code >= 400 ? C_ERROR : C_NORMAL;
    }
    if (r->udp_action >= 0) {
        snprintf(val[F_UDP_ACTION], sizeof val[0], "%d (%s)", r->udp_action, udp_action_name(r->udp_action));
        color[F_UDP_ACTION] = r->udp_action == 3 ? C_ERROR : C_NORMAL;
    }
    snprintf(val[F_TIME], sizeof val[0], "%.1f ms", r->elapsed_ms);
    if (r->has_interval)
        snprintf(val[F_INTERVAL], sizeof val[0], "%" PRId64 " s", r->interval);
    if (r->has_min_interval)
        snprintf(val[F_MIN_INTERVAL], sizeof val[0], "%" PRId64 " s", r->min_interval);
    if (r->has_complete)
        snprintf(val[F_COMPLETE], sizeof val[0], "%" PRId64, r->complete);
    if (r->has_incomplete)
        snprintf(val[F_INCOMPLETE], sizeof val[0], "%" PRId64, r->incomplete);
    if (r->tracker_id)
        snprintf(val[F_TRACKER_ID], sizeof val[0], "%.590s", r->tracker_id);
    if (r->warning) {
        snprintf(val[F_WARNING], sizeof val[0], "%.590s", r->warning);
        color[F_WARNING] = C_WARN;
    }
    if (r->failure || r->status != TR_OK) {
        snprintf(val[F_ERROR], sizeof val[0], "%.590s", r->failure ? r->failure : r->error);
        color[F_ERROR] = C_ERROR;
    }
    if (r->has_downloaded)
        snprintf(val[F_DOWNLOADED], sizeof val[0], "%" PRId64, r->downloaded);
    if (r->external_ip[0])
        snprintf(val[F_EXTERNAL_IP], sizeof val[0], "%s", r->external_ip);
    if (r->retry_in) {
        snprintf(val[F_RETRY_IN], sizeof val[0], "%.590s", r->retry_in);
        color[F_RETRY_IN] = C_WARN;
    }
    if (r->has_crypto)
        snprintf(val[F_CRYPTO], sizeof val[0], "%zu peer(s) sur %zu acceptent le chiffrement", r->crypto_on,
                 r->crypto_total);
    if (r->other_keys)
        snprintf(val[F_OTHER], sizeof val[0], "voir les lignes « autre : … » ci-dessous");
    snprintf(val[F_PEERS], sizeof val[0], "%zu", r->npeers);
    if (r->connection_id)
        snprintf(val[F_CONN_ID], sizeof val[0], "0x%016" PRIx64 " (%s)", r->connection_id,
                 r->conn_reused ? "réutilisé" : "nouveau");
    snprintf(val[F_RETRANS], sizeof val[0], "%d", r->retransmissions);
    if (r->retransmissions)
        color[F_RETRANS] = C_WARN;
    if (r->status != TR_ERR_URL) {
        for (int f = 0; f < F_COUNT; f++) {
            if (!field_applies(f, r->transport)) {
                snprintf(val[f], sizeof val[0], "n/a en %s", tr_transport_name(r->transport));
                color[f] = C_NA;
            }
        }
    }
    SendMessageW(g.resp_lv, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.resp_lv);
    for (int f = 0; f < F_COUNT; f++) {
        const char *cols[] = { field_names[f], val[f] };
        lv_add(g.resp_lv, color[f], 2, cols);
    }
    /* One row per non-standard key ("key = preview" lines). */
    for (const char *line = r->other_keys; line && *line;) {
        const char *end = strchr(line, '\n');
        size_t n = end ? (size_t)(end - line) : strlen(line);
        const char *eq = strstr(line, " = ");
        char key[160], value[600];
        if (eq && (size_t)(eq - line) < n) {
            snprintf(key, sizeof key, "autre : %.*s", (int)(eq - line), line);
            snprintf(value, sizeof value, "%.*s", (int)(n - (size_t)(eq - line) - 3), eq + 3);
        } else {
            snprintf(key, sizeof key, "autre");
            snprintf(value, sizeof value, "%.*s", (int)n, line);
        }
        const char *cols[] = { key, value };
        lv_add(g.resp_lv, C_NORMAL, 2, cols);
        line = end ? end + 1 : line + n;
    }
    SendMessageW(g.resp_lv, WM_SETREDRAW, TRUE, 0);

    char *req = tr_format_request(r);
    set_edit_text(g.req_edit, req);
    free(g.copy_text);
    g.copy_text = xstrdup(is_http && r->request_url ? r->request_url : req);
    free(req);
    char *raw = tr_format_raw(r);
    set_edit_text(g.raw_edit, raw);
    free(raw);

    set_shown_peers(r->peers, r->npeers, r->peers_trailing);
    fill_peers();
}

static void show_selected_history(void)
{
    TrackerEntry *e = selected_tracker();
    int i = lv_sel(g.history_lv);
    const hist_entry *h = (e && i >= 0) ? history_get(&e->hist, (size_t)i) : NULL;
    if (h)
        show_result(&h->result);
    else
        clear_details();
}

static void refresh_history(void)
{
    TrackerEntry *e = selected_tracker();
    g.updating++;
    SendMessageW(g.history_lv, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.history_lv);
    size_t n = e ? e->hist.count : 0;
    for (size_t i = 0; i < n; i++) {
        const tr_result *r = &history_get(&e->hist, i)->result;
        char code[48], ms[32], sl[64], peers[16];
        if (r->transport == TR_HTTP && r->http_code)
            snprintf(code, sizeof code, "HTTP %ld", r->http_code);
        else if (r->transport == TR_UDP && r->udp_action >= 0)
            snprintf(code, sizeof code, "action %d", r->udp_action);
        else
            snprintf(code, sizeof code, "—");
        snprintf(ms, sizeof ms, "%.0f ms", r->elapsed_ms);
        if (r->has_complete || r->has_incomplete)
            snprintf(sl, sizeof sl, "%" PRId64 " / %" PRId64, r->complete, r->incomplete);
        else
            snprintf(sl, sizeof sl, "—");
        snprintf(peers, sizeof peers, "%zu", r->npeers);
        const char *cols[] = { history_get(&e->hist, i)->time, tr_event_name(r->params.event),
                               tr_status_name(r->status), code, ms, sl, peers };
        lv_add(g.history_lv, r->status == TR_OK ? C_NORMAL : C_ERROR, 7, cols);
    }
    if (n)
        lv_select(g.history_lv, 0);
    SendMessageW(g.history_lv, WM_SETREDRAW, TRUE, 0);
    g.updating--;
    show_selected_history();
}

/* ======================================================================
 * Torrent loading
 * ====================================================================== */

static void info_row(const char *k, const char *v)
{
    const char *cols[] = { k, v && v[0] ? v : "—" };
    lv_add(g.info_lv, C_NORMAL, 2, cols);
}

static void fill_torrent_info(void)
{
    torrent_info *t = &g.tor;
    char buf[512];
    SendMessageW(g.info_lv, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g.info_lv);
    info_row("Nom", t->name);
    info_row("info_hash", t->info_hash_hex);
    fmt_size(buf, sizeof buf, t->total_size);
    info_row("Taille", buf);
    snprintf(buf, sizeof buf, "%zu%s", t->nfiles, t->multi_file ? " (multi-fichiers)" : " (fichier unique)");
    info_row("Fichiers", buf);
    char pl[64];
    fmt_size(pl, sizeof pl, t->piece_length);
    snprintf(buf, sizeof buf, "%zu × %s", t->num_pieces, pl);
    info_row("Pièces", buf);
    info_row("Privé", t->is_private ? "oui (DHT/PEX désactivés)" : "non");
    buf[0] = 0;
    if (t->has_creation_date) {
        time_t tt = (time_t)t->creation_date;
        struct tm tmv;
        if (localtime_s(&tmv, &tt) == 0)
            strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv);
        else
            snprintf(buf, sizeof buf, "date invalide (%" PRId64 ")", t->creation_date);
    }
    info_row("Créé le", buf);
    info_row("Créé par", t->created_by);
    info_row("Commentaire", t->comment);
    for (size_t i = 0; i < t->nweb_seeds; i++)
        info_row("Web seed", t->web_seeds[i]);
    for (size_t i = 0; i < t->nfiles; i++) {
        char sz[96], label[32];
        fmt_size(sz, sizeof sz, t->files[i].length);
        snprintf(label, sizeof label, "Fichier %zu", i + 1);
        bbuf b;
        bbuf_init(&b);
        bbuf_printf(&b, "%s   (%s)", t->files[i].path, sz);
        char *s = bbuf_steal(&b);
        info_row(label, s);
        free(s);
    }
    SendMessageW(g.info_lv, WM_SETREDRAW, TRUE, 0);
}

static void load_torrent(const char *path)
{
    torrent_info t;
    char err[400];
    if (torrent_load_file(path, &t, err, sizeof err) < 0) {
        log_now(TR_LOG_ERROR, "Chargement de %s : %s", path, err);
        set_status(1, "%s", err);
        return;
    }
    if (g.has_torrent)
        torrent_free(&g.tor);
    g.tor = t;
    g.has_torrent = 1;
    fill_torrent_info();

    clear_trackers();
    g.updating++;
    for (size_t i = 0; i < t.ntrackers; i++) {
        char tier[24];
        if (t.trackers[i].tier == 0)
            snprintf(tier, sizeof tier, "announce");
        else
            snprintf(tier, sizeof tier, "tier %d", t.trackers[i].tier);
        add_tracker_entry(t.trackers[i].url, tier);
    }
    if (t.ntrackers)
        lv_select(g.trackers_lv, 0);
    g.updating--;

    set_text(g.e_info_hash, t.info_hash_hex);
    char left[32];
    snprintf(left, sizeof left, "%" PRId64, t.total_size);
    set_text(g.e_left, left);

    bbuf b;
    bbuf_init(&b);
    bbuf_printf(&b, "Announce Debugger — %s", t.name);
    char *title = bbuf_steal(&b);
    set_text(g.hwnd, title);
    free(title);

    log_now(TR_LOG_INFO, "Torrent chargé : %s (info_hash %s, %zu tracker(s))", t.name, t.info_hash_hex, t.ntrackers);
    if (t.ntrackers)
        set_status(0, "%zu tracker(s). Sélectionne-en un puis clique sur Announce.", t.ntrackers);
    else
        set_status(1, "Ce torrent ne contient aucun tracker (DHT uniquement). Ajoute une URL à la main.");
    refresh_history();
}

static void load_torrent_w(const wchar_t *path)
{
    char *u = wide_to_utf8(path);
    if (u)
        load_torrent(u);
    free(u);
}

static void open_dialog(void)
{
    wchar_t file[MAX_PATH * 4] = L"";
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = g.hwnd;
    ofn.lpstrFilter = L"Fichiers torrent (*.torrent)\0*.torrent\0Tous les fichiers (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = (DWORD)(sizeof file / sizeof file[0]);
    ofn.lpstrTitle = L"Ouvrir un fichier .torrent";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn))
        load_torrent_w(file);
}

/* ======================================================================
 * Parameters
 * ====================================================================== */

static int parse_u64(const char *s, uint64_t max, uint64_t *out)
{
    if (!*s)
        return 0;
    uint64_t v = 0;
    for (const char *p = s; *p; p++) {
        if (*p < '0' || *p > '9')
            return 0;
        unsigned d = (unsigned)(*p - '0');
        if (v > (UINT64_MAX - d) / 10)
            return 0;
        v = v * 10 + d;
    }
    if (v > max)
        return 0;
    *out = v;
    return 1;
}

static int read_u64(HWND e, uint64_t max, uint64_t *out, const char *name, char *err, size_t errlen)
{
    char *s = get_trimmed(e);
    int ok = parse_u64(s, max, out);
    if (!ok)
        snprintf(err, errlen, "%s : valeur invalide « %.40s »", name, s);
    free(s);
    return ok;
}

static int collect_params(tr_params *p, char *err, size_t errlen)
{
    memset(p, 0, sizeof *p);
    int ok = 0;
    char *s = get_trimmed(g.e_info_hash);
    if (hex_decode(s, p->info_hash, 20) < 0) {
        snprintf(err, errlen, "info_hash : 40 caractères hexadécimaux attendus");
        goto out;
    }
    free(s);

    s = get_text(g.e_peer_id);
    size_t len = strlen(s);
    if (len == 40 && hex_decode(s, p->peer_id, 20) == 0) {
        /* hex form */
    } else if (len == 20) {
        memcpy(p->peer_id, s, 20);
    } else {
        snprintf(err, errlen, "peer_id : 20 octets attendus (20 caractères ou 40 hexadécimaux), %zu reçus", len);
        goto out;
    }
    free(s);

    s = get_trimmed(g.e_key);
    uint8_t key[4];
    if (hex_decode(s, key, 4) < 0) {
        snprintf(err, errlen, "key : 8 caractères hexadécimaux attendus");
        goto out;
    }
    p->key = be_get32(key);
    free(s);
    s = NULL;

    uint64_t v;
    if (!read_u64(g.e_port, 65535, &v, "port", err, errlen))
        goto out;
    p->port = (uint16_t)v;
    if (!read_u64(g.e_uploaded, UINT64_MAX, &p->uploaded, "uploaded", err, errlen) ||
        !read_u64(g.e_downloaded, UINT64_MAX, &p->downloaded, "downloaded", err, errlen) ||
        !read_u64(g.e_left, UINT64_MAX, &p->left, "left", err, errlen))
        goto out;

    s = get_trimmed(g.e_numwant);
    if (strcmp(s, "-1") == 0) {
        p->numwant = -1;
    } else if (parse_u64(s, INT32_MAX, &v)) {
        p->numwant = (int32_t)v;
    } else {
        snprintf(err, errlen, "numwant : entier ≥ -1 attendu (-1 = défaut du tracker)");
        goto out;
    }

    LRESULT ev = SendMessageW(g.cb_event, CB_GETCURSEL, 0, 0);
    p->event = (ev >= 0 && ev < 4) ? event_values[ev] : TR_EVENT_NONE;
    p->compact = SendMessageW(g.chk_compact, BM_GETCHECK, 0, 0) == BST_CHECKED;
    p->tls_no_verify = SendMessageW(g.chk_noverify, BM_GETCHECK, 0, 0) == BST_CHECKED;
    ok = 1;
out:
    free(s);
    return ok;
}

static void set_new_peer_id(void)
{
    uint8_t id[20];
    tr_generate_peer_id(id);
    char s[21];
    memcpy(s, id, 20);
    s[20] = 0;
    set_text(g.e_peer_id, s);
}

static void set_new_key(void)
{
    char s[16];
    snprintf(s, sizeof s, "%08X", random_u32());
    set_text(g.e_key, s);
}

/* ======================================================================
 * Announce (worker thread)
 * ====================================================================== */

static void job_free(Job *j)
{
    tr_result_free(&j->res);
    free(j->url);
    free(j->trackerid);
    free(j);
}

static void job_done(Job *j)
{
    if (g.thread) {
        WaitForSingleObject(g.thread, INFINITE);
        CloseHandle(g.thread);
        g.thread = NULL;
    }
    g.job = NULL;
    if (g.closing) {
        job_free(j);
        return;
    }

    TrackerEntry *e = j->entry;
    tr_result *r = &j->res;
    e->status_err = r->status != TR_OK;
    snprintf(e->status, sizeof e->status, "%s", r->status == TR_OK ? "OK" : tr_status_name(r->status));
    if (r->has_complete || r->has_incomplete)
        snprintf(e->sl, sizeof e->sl, "%" PRId64 " / %" PRId64, r->complete, r->incomplete);
    else
        snprintf(e->sl, sizeof e->sl, "—");

    if (r->status == TR_OK)
        log_now(TR_LOG_INFO,
                "Réponse en %.0f ms : interval %" PRId64 " s, seeders %" PRId64 ", leechers %" PRId64 ", %zu peer(s)",
                r->elapsed_ms, r->interval, r->complete, r->incomplete, r->npeers);
    else
        log_now(r->status == TR_ERR_CANCELLED ? TR_LOG_WARN : TR_LOG_ERROR, "Échec (%s) : %s",
                tr_status_name(r->status), r->error);
    if (r->status == TR_OK)
        set_status(0, "Announce terminé.");
    else
        set_status(1, "%s", r->error);

    history_add(&e->hist, j->time, r);
    job_free(j);

    for (size_t i = 0; i < g.ntrackers; i++)
        if (g.trackers[i] == e)
            tracker_row(e, (int)i);
    set_busy(0);
    if (selected_tracker() == e)
        refresh_history();
}

static DWORD WINAPI worker(LPVOID p)
{
    Job *j = p;
    tr_announce(j->url, &j->params, &j->entry->state, &g.ctx, &j->res);
    PostMessageW(g.hwnd, WM_APP_DONE, 0, (LPARAM)j);
    return 0;
}

static void start_announce(void)
{
    if (g.thread)
        return;
    TrackerEntry *e = selected_tracker();
    if (!e) {
        set_status(1, "Sélectionne un tracker dans la liste (ou ajoute une URL).");
        return;
    }
    tr_params p;
    char err[256];
    if (!collect_params(&p, err, sizeof err)) {
        set_status(1, "%s", err);
        log_now(TR_LOG_ERROR, "Paramètres invalides : %s", err);
        return;
    }
    Job *j = calloc(1, sizeof *j);
    if (!j)
        return;
    j->entry = e;
    j->url = xstrdup(e->url);
    j->params = p;
    j->trackerid = xstrdup(e->state.trackerid);
    j->params.trackerid = j->trackerid;
    now_str(j->time);

    tr_cancel_set(&g.cancel_flag, 0);
    log_now(TR_LOG_INFO, "── Announce (event=%s) → %s", tr_event_name(p.event), e->url);
    set_status(0, "Announce en cours…");
    g.job = j;
    set_busy(1);
    g.thread = CreateThread(NULL, 0, worker, j, 0, NULL);
    if (!g.thread) {
        g.job = NULL;
        job_free(j);
        set_busy(0);
        set_status(1, "Impossible de créer le thread réseau.");
    }
}

/* ======================================================================
 * Commands
 * ====================================================================== */

static void add_manual_tracker(void)
{
    if (g.thread)
        return;
    char *url = get_trimmed(g.manual_url);
    tr_url u;
    char err[200];
    if (tr_parse_url(url, &u, err, sizeof err) < 0) {
        set_status(1, "URL invalide : %s", err);
        free(url);
        return;
    }
    int idx = add_tracker_entry(url, "manuel");
    if (idx >= 0)
        lv_select(g.trackers_lv, idx);
    set_text(g.manual_url, "");
    log_now(TR_LOG_INFO, "Tracker ajouté à la main : %s", url);
    char *ih = get_trimmed(g.e_info_hash);
    if (!ih[0])
        set_status(1, "Tracker ajouté. Renseigne l'info_hash (40 hex) avant d'annoncer.");
    else
        set_status(0, "Tracker ajouté.");
    free(ih);
    free(url);
}

static void clear_history(void)
{
    TrackerEntry *e = selected_tracker();
    if (!e || g.thread)
        return;
    history_clear(&e->hist);
    log_now(TR_LOG_INFO, "Historique vidé pour %s", e->url);
    refresh_history();
}

static void show_tab(int i)
{
    int sw[4] = { SW_HIDE, SW_HIDE, SW_HIDE, SW_HIDE };
    if (i >= 0 && i < 4)
        sw[i] = SW_SHOW;
    ShowWindow(g.resp_lv, sw[0]);
    ShowWindow(g.copy_req, sw[1]);
    ShowWindow(g.req_edit, sw[1]);
    ShowWindow(g.peers_label, sw[2]);
    ShowWindow(g.peers_search, sw[2]);
    ShowWindow(g.peers_lv, sw[2]);
    ShowWindow(g.raw_edit, sw[3]);
}

static void on_double_click(int id, int row)
{
    if (row < 0)
        return;
    char *text = NULL;
    if (id == ID_INFO_LV || id == ID_RESP_LV || id == ID_TRACKERS_LV) {
        text = lv_text(id == ID_INFO_LV ? g.info_lv : id == ID_RESP_LV ? g.resp_lv : g.trackers_lv, row, 1);
    } else if (id == ID_PEERS_LV) {
        char *ip = lv_text(g.peers_lv, row, 1), *port = lv_text(g.peers_lv, row, 2);
        bbuf b;
        bbuf_init(&b);
        bbuf_printf(&b, strchr(ip ? ip : "", ':') ? "[%s]:%s" : "%s:%s", ip ? ip : "", port ? port : "");
        text = bbuf_steal(&b);
        free(ip);
        free(port);
    }
    if (text && text[0] && strcmp(text, "—") != 0) {
        copy_to_clipboard(text);
        set_status(0, "Copié : %.200s", text);
    }
    free(text);
}

/* ======================================================================
 * Window creation and layout
 * ====================================================================== */

static HWND ctl(DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int id)
{
    return CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, g.hwnd, (HMENU)(INT_PTR)id,
                           g.inst, NULL);
}

static HWND label(const wchar_t *text)
{
    return ctl(0, WC_STATICW, text, SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE, ID_LABEL);
}

static HWND button(const wchar_t *text, int id)
{
    return ctl(0, WC_BUTTONW, text, BS_PUSHBUTTON | WS_TABSTOP, id);
}

static HWND edit(int id)
{
    return ctl(WS_EX_CLIENTEDGE, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, id);
}

static HWND checkbox(const wchar_t *text, int id)
{
    return ctl(0, WC_BUTTONW, text, BS_AUTOCHECKBOX | WS_TABSTOP, id);
}

static HWND text_view(int id, int wrap)
{
    DWORD style = ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP;
    if (!wrap)
        style |= ES_AUTOHSCROLL | WS_HSCROLL;
    HWND h = ctl(WS_EX_CLIENTEDGE, WC_EDITW, L"", style, id);
    SendMessageW(h, EM_SETLIMITTEXT, 0, 0);
    return h;
}

static void create_controls(void)
{
    g.open = button(L"Ouvrir un .torrent…", ID_OPEN);
    g.lbl_torrent = label(L"Torrent  (glisser-déposer un .torrent ici ; double-clic pour copier)");
    g.copy_hash = button(L"Copier l'info_hash", ID_COPY_HASH);
    static const wchar_t *const info_cols[] = { L"Champ", L"Valeur" };
    static const int info_w[] = { 110, 300 };
    g.info_lv = make_listview(ID_INFO_LV, info_cols, info_w, 2);

    g.lbl_trackers = label(L"Trackers");
    static const wchar_t *const tr_cols[] = { L"Tier", L"URL", L"Transport", L"Dernier statut", L"S / L" };
    static const int tr_w[] = { 70, 230, 70, 110, 70 };
    g.trackers_lv = make_listview(ID_TRACKERS_LV, tr_cols, tr_w, 5);
    g.manual_url = edit(ID_MANUAL_URL);
    SendMessageW(g.manual_url, EM_SETCUEBANNER, TRUE, (LPARAM)L"udp://tracker:6969/announce ou http(s)://… puis Entrée");
    g.add = button(L"Ajouter", ID_ADD);

    g.lbl_params = label(L"Paramètres");
    g.lbl_ih = label(L"info_hash");
    g.e_info_hash = edit(ID_INFO_HASH);
    g.lbl_pid = label(L"peer_id");
    g.e_peer_id = edit(ID_PEER_ID);
    g.regen_peer = button(L"↻", ID_REGEN_PEER);
    g.lbl_key = label(L"key");
    g.e_key = edit(ID_KEY);
    g.regen_key = button(L"↻", ID_REGEN_KEY);
    g.lbl_event = label(L"event");
    g.cb_event = ctl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_EVENT);
    static const wchar_t *const events[] = { L"started", L"completed", L"stopped", L"none" };
    for (int i = 0; i < 4; i++)
        SendMessageW(g.cb_event, CB_ADDSTRING, 0, (LPARAM)events[i]);
    SendMessageW(g.cb_event, CB_SETCURSEL, 0, 0);
    g.lbl_port = label(L"port");
    g.e_port = edit(ID_PORT);
    g.lbl_numwant = label(L"numwant");
    g.e_numwant = edit(ID_NUMWANT);
    g.lbl_up = label(L"uploaded");
    g.e_uploaded = edit(ID_UPLOADED);
    g.lbl_down = label(L"downloaded");
    g.e_downloaded = edit(ID_DOWNLOADED);
    g.lbl_left = label(L"left");
    g.e_left = edit(ID_LEFT);
    g.chk_compact = checkbox(L"compact=1 (HTTP seulement)", ID_COMPACT);
    SendMessageW(g.chk_compact, BM_SETCHECK, BST_CHECKED, 0);
    g.chk_noverify = checkbox(L"Ne pas vérifier le certificat TLS", ID_NOVERIFY);
    set_text(g.e_port, "6881");
    set_text(g.e_numwant, "50");
    set_text(g.e_uploaded, "0");
    set_text(g.e_downloaded, "0");
    set_text(g.e_left, "0");

    g.announce = button(L"Announce", ID_ANNOUNCE);
    g.cancel = button(L"Annuler", ID_CANCEL);
    g.status = ctl(0, WC_STATICW, L"Ouvre un .torrent ou ajoute une URL de tracker.", SS_LEFT | SS_NOPREFIX, ID_STATUS);

    g.lbl_history = label(L"Historique des announces (tracker sélectionné)");
    g.clear_hist = button(L"Vider l'historique de ce tracker", ID_CLEAR_HIST);
    static const wchar_t *const h_cols[] = { L"Heure", L"Event", L"Statut", L"Code", L"Temps", L"Seeders / Leechers",
                                             L"Peers" };
    static const int h_w[] = { 95, 80, 120, 90, 70, 120, 60 };
    g.history_lv = make_listview(ID_HISTORY_LV, h_cols, h_w, 7);

    g.tab = ctl(0, WC_TABCONTROLW, L"", WS_CLIPSIBLINGS | WS_TABSTOP, ID_TAB);
    static const wchar_t *const tabs[] = { L"Réponse", L"Requête", L"Peers", L"Raw" };
    for (int i = 0; i < 4; i++) {
        TCITEMW ti;
        memset(&ti, 0, sizeof ti);
        ti.mask = TCIF_TEXT;
        ti.pszText = (LPWSTR)tabs[i];
        TabCtrl_InsertItem(g.tab, i, &ti);
    }
    static const wchar_t *const r_cols[] = { L"Champ", L"Valeur" };
    static const int r_w[] = { 170, 400 };
    g.resp_lv = make_listview(ID_RESP_LV, r_cols, r_w, 2);
    g.copy_req = button(L"Copier l'URL / la requête", ID_COPY_REQ);
    g.req_edit = text_view(ID_REQ_EDIT, 1);
    g.peers_label = label(L"");
    g.peers_search = edit(ID_PEERS_SEARCH);
    SendMessageW(g.peers_search, EM_SETCUEBANNER, TRUE, (LPARAM)L"Rechercher (IP, port, peer_id)…");
    static const wchar_t *const p_cols[] = { L"#", L"IP", L"Port", L"peer_id" };
    static const int p_w[] = { 50, 220, 70, 200 };
    g.peers_lv = make_listview(ID_PEERS_LV, p_cols, p_w, 4);
    g.raw_edit = text_view(ID_RAW_EDIT, 0);

    g.lbl_log = label(L"Journal");
    g.log = ctl(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
                ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | ES_NOHIDESEL, ID_LOG);
    SendMessageW(g.log, EM_EXLIMITTEXT, 0, 0x7FFFFFFF);
}

static void create_fonts(void)
{
    if (g.font)
        DeleteObject(g.font);
    if (g.font_bold)
        DeleteObject(g.font_bold);
    if (g.font_mono)
        DeleteObject(g.font_mono);
    NONCLIENTMETRICSW ncm;
    memset(&ncm, 0, sizeof ncm);
    ncm.cbSize = sizeof ncm;
    SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0, g.dpi);
    g.font = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfWeight = FW_BOLD;
    g.font_bold = CreateFontIndirectW(&lf);
    LOGFONTW m;
    memset(&m, 0, sizeof m);
    m.lfHeight = -MulDiv(9, (int)g.dpi, 72);
    m.lfCharSet = DEFAULT_CHARSET;
    m.lfQuality = CLEARTYPE_QUALITY;
    wcscpy(m.lfFaceName, L"Consolas");
    g.font_mono = CreateFontIndirectW(&m);
}

static BOOL CALLBACK set_font_cb(HWND h, LPARAM lp)
{
    (void)lp;
    HFONT f = g.font;
    if (h == g.e_info_hash || h == g.e_peer_id || h == g.e_key || h == g.req_edit || h == g.raw_edit || h == g.log)
        f = g.font_mono;
    else if (h == g.lbl_torrent || h == g.lbl_trackers || h == g.lbl_params || h == g.lbl_history || h == g.lbl_log)
        f = g.font_bold;
    SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE);
    return TRUE;
}

static void mv(HWND h, int x, int y, int w, int hgt)
{
    SetWindowPos(h, HWND_TOP, x, y, w > 0 ? w : 0, hgt > 0 ? hgt : 0, SWP_NOACTIVATE);
}

static void layout(void)
{
    RECT rc;
    GetClientRect(g.hwnd, &rc);
    int W = rc.right, H = rc.bottom;
    if (W <= 0 || H <= 0)
        return;
    int M = S(8), gap = S(4), BH = S(26), RH = S(24), LBW = S(78);
    int LW = S(600);
    if (LW > W * 55 / 100)
        LW = W * 55 / 100;
    int logH = S(170);
    int logTop = H - logH;

    /* Left column */
    int x = M, w = LW - 2 * M, y = M;
    mv(g.open, x, y, S(190), BH);
    y += BH + M;
    mv(g.lbl_torrent, x, y, w - S(150) - gap, BH);
    mv(g.copy_hash, x + w - S(150), y, S(150), BH);
    y += BH + gap;
    mv(g.info_lv, x, y, w, S(165));
    y += S(165) + M;
    mv(g.lbl_trackers, x, y, w, RH);
    y += RH;

    int bottom_h = BH + M + RH + 6 * (RH + gap) + RH + gap + BH + gap + S(40);
    int tr_h = logTop - M - y - gap - bottom_h;
    if (tr_h < S(80))
        tr_h = S(80);
    mv(g.trackers_lv, x, y, w, tr_h);
    y += tr_h + gap;
    mv(g.manual_url, x, y, w - S(90) - gap, BH);
    mv(g.add, x + w - S(90), y, S(90), BH);
    y += BH + M;
    mv(g.lbl_params, x, y, w, RH);
    y += RH;

    int half = (w - S(10)) / 2, x2 = x + half + S(10);
    mv(g.lbl_ih, x, y, LBW, RH);
    mv(g.e_info_hash, x + LBW, y, w - LBW, RH);
    y += RH + gap;
    mv(g.lbl_pid, x, y, LBW, RH);
    mv(g.e_peer_id, x + LBW, y, w - LBW - S(34), RH);
    mv(g.regen_peer, x + w - S(30), y, S(30), RH);
    y += RH + gap;
    mv(g.lbl_key, x, y, LBW, RH);
    mv(g.e_key, x + LBW, y, half - LBW - S(34), RH);
    mv(g.regen_key, x + half - S(30), y, S(30), RH);
    mv(g.lbl_event, x2, y, LBW, RH);
    mv(g.cb_event, x2 + LBW, y, half - LBW, S(200));
    y += RH + gap;
    mv(g.lbl_port, x, y, LBW, RH);
    mv(g.e_port, x + LBW, y, half - LBW, RH);
    mv(g.lbl_numwant, x2, y, LBW, RH);
    mv(g.e_numwant, x2 + LBW, y, half - LBW, RH);
    y += RH + gap;
    mv(g.lbl_up, x, y, LBW, RH);
    mv(g.e_uploaded, x + LBW, y, half - LBW, RH);
    mv(g.lbl_down, x2, y, LBW, RH);
    mv(g.e_downloaded, x2 + LBW, y, half - LBW, RH);
    y += RH + gap;
    mv(g.lbl_left, x, y, LBW, RH);
    mv(g.e_left, x + LBW, y, half - LBW, RH);
    y += RH + gap;
    mv(g.chk_compact, x, y, half, RH);
    mv(g.chk_noverify, x2, y, half, RH);
    y += RH + gap;
    mv(g.announce, x, y, w - S(130) - gap, BH);
    mv(g.cancel, x + w - S(130), y, S(130), BH);
    y += BH + gap;
    mv(g.status, x, y, w, S(40));

    /* Right column */
    int rx = LW, rw = W - LW - M;
    y = M;
    mv(g.lbl_history, rx, y, rw - S(250), BH);
    mv(g.clear_hist, rx + rw - S(240), y, S(240), BH);
    y += BH + gap;
    mv(g.history_lv, rx, y, rw, S(180));
    y += S(180) + M;
    int tab_h = logTop - M - y;
    mv(g.tab, rx, y, rw, tab_h);
    RECT tr = { rx, y, rx + rw, y + tab_h };
    TabCtrl_AdjustRect(g.tab, FALSE, &tr);
    int pw = tr.right - tr.left, ph = tr.bottom - tr.top;
    mv(g.resp_lv, tr.left, tr.top, pw, ph);
    mv(g.copy_req, tr.left + gap, tr.top + gap, S(220), BH);
    mv(g.req_edit, tr.left, tr.top + BH + 2 * gap, pw, ph - BH - 2 * gap);
    int sw = S(280);
    mv(g.peers_search, tr.left + gap, tr.top + gap, sw, RH);
    mv(g.peers_label, tr.left + sw + 3 * gap, tr.top + gap, pw - sw - 4 * gap, RH);
    mv(g.peers_lv, tr.left, tr.top + RH + 2 * gap, pw, ph - RH - 2 * gap);
    mv(g.raw_edit, tr.left, tr.top, pw, ph);

    /* Log */
    mv(g.lbl_log, M, logTop, W - 2 * M, RH);
    mv(g.log, M, logTop + RH, W - 2 * M, H - logTop - RH - M);

    lv_stretch(g.info_lv, 1);
    lv_stretch(g.trackers_lv, 1);
    lv_stretch(g.history_lv, 6);
    lv_stretch(g.resp_lv, 1);
    lv_stretch(g.peers_lv, 3);
}

/* ======================================================================
 * Window procedure
 * ====================================================================== */

static LRESULT on_notify(NMHDR *nh)
{
    int id = (int)nh->idFrom;
    if (nh->code == NM_CUSTOMDRAW &&
        (id == ID_TRACKERS_LV || id == ID_HISTORY_LV || id == ID_RESP_LV)) {
        NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW *)nh;
        if (cd->nmcd.dwDrawStage == CDDS_PREPAINT)
            return CDRF_NOTIFYITEMDRAW;
        if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
            int c = (int)cd->nmcd.lItemlParam;
            if (c != C_NORMAL)
                cd->clrText = color_of(c);
        }
        return CDRF_DODEFAULT;
    }
    if (nh->code == LVN_ITEMCHANGED && !g.updating) {
        NMLISTVIEW *nl = (NMLISTVIEW *)nh;
        if ((nl->uChanged & LVIF_STATE) && (nl->uNewState & LVIS_SELECTED) && !(nl->uOldState & LVIS_SELECTED)) {
            if (id == ID_TRACKERS_LV)
                refresh_history();
            else if (id == ID_HISTORY_LV)
                show_selected_history();
        }
        return 0;
    }
    if (nh->code == NM_DBLCLK) {
        on_double_click(id, ((NMITEMACTIVATE *)nh)->iItem);
        return 0;
    }
    if (nh->code == TCN_SELCHANGE && id == ID_TAB) {
        show_tab(TabCtrl_GetCurSel(g.tab));
        return 0;
    }
    return 0;
}

static void on_command(int id, int code)
{
    if (id == ID_PEERS_SEARCH) {
        if (code == EN_CHANGE)
            fill_peers();
        return;
    }
    switch (id) {
    case ID_OPEN:
        open_dialog();
        break;
    case ID_COPY_HASH:
        if (g.has_torrent) {
            copy_to_clipboard(g.tor.info_hash_hex);
            set_status(0, "info_hash copié.");
        }
        break;
    case ID_ADD:
        add_manual_tracker();
        break;
    case ID_REGEN_PEER:
        set_new_peer_id();
        break;
    case ID_REGEN_KEY:
        set_new_key();
        break;
    case ID_ANNOUNCE:
        start_announce();
        break;
    case ID_CANCEL:
        if (g.thread) {
            tr_cancel_set(&g.cancel_flag, 1);
            log_now(TR_LOG_WARN, "Annulation demandée");
        }
        break;
    case ID_CLEAR_HIST:
        clear_history();
        break;
    case ID_COPY_REQ:
        if (g.copy_text) {
            copy_to_clipboard(g.copy_text);
            set_status(0, "Requête copiée.");
        }
        break;
    case IDOK: /* Enter key (IsDialogMessage) */
        if (GetFocus() == g.manual_url)
            add_manual_tracker();
        break;
    default:
        break;
    }
}

static void shutdown_worker(void)
{
    g.closing = 1;
    tr_cancel_set(&g.cancel_flag, 1);
    if (g.thread) {
        WaitForSingleObject(g.thread, INFINITE);
        CloseHandle(g.thread);
        g.thread = NULL;
    }
    /* Free the data carried by messages the worker already posted. */
    MSG msg;
    while (PeekMessageW(&msg, g.hwnd, WM_APP_LOG, WM_APP_DONE, PM_REMOVE)) {
        if (msg.message == WM_APP_LOG) {
            LogMsg *m = (LogMsg *)msg.lParam;
            free(m->text);
            free(m);
        } else {
            job_done((Job *)msg.lParam);
        }
    }
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        g.hwnd = hwnd;
        g.dpi = GetDpiForWindow(hwnd);
        create_fonts();
        create_controls();
        EnumChildWindows(hwnd, set_font_cb, 0);
        DragAcceptFiles(hwnd, TRUE);
        return 0;
    case WM_SIZE:
        layout();
        return 0;
    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lp;
        UINT dpi = g.dpi ? g.dpi : 96;
        mm->ptMinTrackSize.x = MulDiv(1000, (int)dpi, 96);
        mm->ptMinTrackSize.y = MulDiv(720, (int)dpi, 96);
        return 0;
    }
    case WM_DPICHANGED: {
        g.dpi = HIWORD(wp);
        RECT *r = (RECT *)lp;
        create_fonts();
        EnumChildWindows(hwnd, set_font_cb, 0);
        SetWindowPos(hwnd, NULL, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        layout();
        return 0;
    }
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == g.status) {
            HDC dc = (HDC)wp;
            SetTextColor(dc, g.status_err ? color_of(C_ERROR) : GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
        break;
    case WM_NOTIFY:
        return on_notify((NMHDR *)lp);
    case WM_COMMAND:
        on_command(LOWORD(wp), HIWORD(wp));
        return 0;
    case WM_DROPFILES: {
        HDROP drop = (HDROP)wp;
        wchar_t path[MAX_PATH * 4];
        if (!g.thread && DragQueryFileW(drop, 0, path, (UINT)(sizeof path / sizeof path[0])))
            load_torrent_w(path);
        DragFinish(drop);
        return 0;
    }
    case WM_APP_LOG: {
        LogMsg *m = (LogMsg *)lp;
        log_line(m->level, m->ts, m->text);
        free(m->text);
        free(m);
        return 0;
    }
    case WM_APP_DONE:
        job_done((Job *)lp);
        return 0;
    case WM_CLOSE:
        shutdown_worker();
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int ui_run(HINSTANCE inst, int show)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);
    g.richedit = LoadLibraryW(L"Msftedit.dll");
    g.inst = inst;

    tr_ctx_defaults(&g.ctx);
    g.ctx.log = worker_log;
    g.ctx.cancel = &g.cancel_flag;

    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"AnnounceDebuggerWindow";
    RegisterClassExW(&wc);

    UINT sdpi = GetDpiForSystem();
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Announce Debugger", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                CW_USEDEFAULT, CW_USEDEFAULT, MulDiv(1450, (int)sdpi, 96),
                                MulDiv(960, (int)sdpi, 96), NULL, NULL, inst, NULL);
    if (!hwnd)
        return 1;

    set_new_peer_id();
    set_new_key();
    show_tab(0);
    clear_details();
    set_busy(0);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    log_now(TR_LOG_INFO, "Prêt. Aucun announce n'est envoyé automatiquement.");

    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        if (argc > 1)
            load_torrent_w(argv[1]);
        LocalFree(argv);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    free_trackers();
    if (g.has_torrent)
        torrent_free(&g.tor);
    free(g.copy_text);
    free(g.shown_peers);
    DeleteObject(g.font);
    DeleteObject(g.font_bold);
    DeleteObject(g.font_mono);
    if (g.richedit)
        FreeLibrary(g.richedit);
    return (int)msg.wParam;
}
