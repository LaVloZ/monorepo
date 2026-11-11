#include <windows.h>
#include <winhttp.h>

#include "http_tracker.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "bencode.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "winhttp.lib")
#endif

#define HTTP_MAX_BODY (8 * 1024 * 1024)

static int is_unreserved(uint8_t c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' ||
           c == '_' || c == '~';
}

/* all = 1 encodes every byte as %XX (used for info_hash). */
static void pct_encode(bbuf *b, const uint8_t *d, size_t n, int all)
{
    for (size_t i = 0; i < n; i++) {
        if (!all && is_unreserved(d[i]))
            bbuf_putc(b, d[i]);
        else
            bbuf_printf(b, "%%%02X", d[i]);
    }
}

char *http_build_url(const char *base, const tr_params *p)
{
    bbuf b;
    bbuf_init(&b);
    size_t blen = strcspn(base, "#");
    bbuf_append(&b, base, blen);
    const char *q = memchr(base, '?', blen);
    if (!q)
        bbuf_putc(&b, '?');
    else if (blen && base[blen - 1] != '?' && base[blen - 1] != '&')
        bbuf_putc(&b, '&');

    bbuf_printf(&b, "info_hash=");
    pct_encode(&b, p->info_hash, 20, 1);
    bbuf_printf(&b, "&peer_id=");
    pct_encode(&b, p->peer_id, 20, 0);
    bbuf_printf(&b, "&port=%u&uploaded=%" PRIu64 "&downloaded=%" PRIu64 "&left=%" PRIu64 "&compact=%d", p->port,
                p->uploaded, p->downloaded, p->left, p->compact ? 1 : 0);
    if (p->numwant >= 0)
        bbuf_printf(&b, "&numwant=%" PRId32, p->numwant);
    bbuf_printf(&b, "&key=%08X", p->key);
    if (p->event != TR_EVENT_NONE)
        bbuf_printf(&b, "&event=%s", tr_event_name(p->event));
    if (p->trackerid && p->trackerid[0]) {
        bbuf_printf(&b, "&trackerid=");
        pct_encode(&b, (const uint8_t *)p->trackerid, strlen(p->trackerid), 0);
    }
    return bbuf_steal(&b);
}

static void parse_dict_peers(const ben_node *list, tr_result *r)
{
    for (size_t i = 0; i < list->count; i++) {
        const ben_node *d = list->items[i];
        size_t iplen, idlen;
        const unsigned char *ip = ben_get_str(d, "ip", &iplen);
        int64_t port;
        if (!ip || !ben_get_int(d, "port", &port) || port < 0 || port > 65535)
            continue;
        tr_peer p;
        memset(&p, 0, sizeof p);
        snprintf(p.ip, sizeof p.ip, "%.*s", (int)(iplen < sizeof p.ip ? iplen : sizeof p.ip - 1), (const char *)ip);
        p.port = (uint16_t)port;
        const unsigned char *id = ben_get_str(d, "peer id", &idlen);
        if (id) {
            p.peer_id_len = idlen > 20 ? 20 : idlen;
            memcpy(p.peer_id, id, p.peer_id_len);
        }
        tr_result_add_peer(r, &p);
    }
}

static char *dup_ben_str(const ben_node *dict, const char *key)
{
    size_t n;
    const unsigned char *s = ben_get_str(dict, key, &n);
    return s ? xstrndup(s, n) : NULL;
}

int http_parse_response(const uint8_t *body, size_t len, tr_result *r)
{
    char err[200];
    ben_node *root = ben_decode(body, len, err, sizeof err);
    if (!root) {
        snprintf(r->error, sizeof r->error, "réponse non bencodée : %s", err);
        return -1;
    }
    if (root->type != BEN_DICT) {
        snprintf(r->error, sizeof r->error, "réponse bencodée mais pas un dictionnaire");
        ben_free(root);
        return -1;
    }
    r->failure = dup_ben_str(root, "failure reason");
    r->warning = dup_ben_str(root, "warning message");
    r->tracker_id = dup_ben_str(root, "tracker id");
    r->has_interval = ben_get_int(root, "interval", &r->interval);
    r->has_min_interval = ben_get_int(root, "min interval", &r->min_interval);
    r->has_complete = ben_get_int(root, "complete", &r->complete);
    r->has_incomplete = ben_get_int(root, "incomplete", &r->incomplete);

    const ben_node *peers = ben_dict_get(root, "peers");
    if (peers && peers->type == BEN_STR)
        r->peers_trailing += tr_parse_compact_peers(peers->s, peers->slen, 0, r);
    else if (peers && peers->type == BEN_LIST)
        parse_dict_peers(peers, r);
    const ben_node *peers6 = ben_dict_get(root, "peers6");
    if (peers6 && peers6->type == BEN_STR)
        r->peers_trailing += tr_parse_compact_peers(peers6->s, peers6->slen, 1, r);
    ben_free(root);
    return 0;
}

/* ---- WinHTTP (asynchronous mode, so that a request can be cancelled) ---- */

typedef struct {
    HANDLE ev;     /* auto-reset: one completion arrived */
    HANDLE closed; /* manual-reset: request handle fully closed */
    DWORD status;  /* last completion */
    DWORD error;   /* for WINHTTP_CALLBACK_STATUS_REQUEST_ERROR */
    DWORD length;  /* DATA_AVAILABLE / READ_COMPLETE */
} async_state;

static void CALLBACK http_cb(HINTERNET h, DWORD_PTR ctx, DWORD status, LPVOID info, DWORD info_len)
{
    (void)h;
    async_state *s = (async_state *)ctx;
    if (!s)
        return;
    switch (status) {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        s->status = status;
        SetEvent(s->ev);
        break;
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        s->length = *(DWORD *)info;
        s->status = status;
        SetEvent(s->ev);
        break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        s->length = info_len;
        s->status = status;
        SetEvent(s->ev);
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        s->error = ((WINHTTP_ASYNC_RESULT *)info)->dwError;
        s->status = status;
        SetEvent(s->ev);
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        SetEvent(s->closed);
        break;
    default:
        break;
    }
}

enum { WAIT_OK = 0, WAIT_ERROR = -1, WAIT_CANCELLED = -2 };

/* Waits for the given completion, checking the cancel flag every 100 ms. */
static int wait_for(async_state *s, DWORD expected, const tr_ctx *ctx)
{
    for (;;) {
        if (WaitForSingleObject(s->ev, 100) == WAIT_OBJECT_0) {
            if (s->status == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)
                return WAIT_ERROR;
            if (s->status == expected)
                return WAIT_OK;
            continue;
        }
        if (tr_cancelled(ctx))
            return WAIT_CANCELLED;
    }
}

static void set_winhttp_error(tr_result *res, DWORD code)
{
    switch (code) {
    case ERROR_WINHTTP_NAME_NOT_RESOLVED:
        res->status = TR_ERR_DNS;
        break;
    case ERROR_WINHTTP_TIMEOUT:
        res->status = TR_ERR_TIMEOUT;
        break;
    case ERROR_WINHTTP_OPERATION_CANCELLED:
        res->status = TR_ERR_CANCELLED;
        break;
    default:
        res->status = TR_ERR_NET;
    }
    char msg[300];
    win_error_text(code, "winhttp.dll", msg, sizeof msg);
    if (code == ERROR_WINHTTP_SECURE_FAILURE)
        snprintf(res->error, sizeof res->error, "%s : certificat TLS refusé (option « ne pas vérifier le certificat »)",
                 msg);
    else
        snprintf(res->error, sizeof res->error, "%s", msg);
}

static wchar_t *wcsndup_(const wchar_t *s, size_t n)
{
    wchar_t *d = malloc((n + 1) * sizeof *d);
    if (d) {
        memcpy(d, s, n * sizeof *d);
        d[n] = 0;
    }
    return d;
}

/* Raw response headers as UTF-8 (malloc'd). */
static char *query_headers(HINTERNET req)
{
    DWORD size = 0;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                        &size, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !size)
        return xstrdup("");
    wchar_t *w = malloc(size + sizeof(wchar_t));
    if (!w)
        return xstrdup("");
    char *u = NULL;
    if (WinHttpQueryHeaders(req, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, w, &size,
                            WINHTTP_NO_HEADER_INDEX)) {
        w[size / sizeof(wchar_t)] = 0;
        u = wide_to_utf8(w);
    }
    free(w);
    return u ? u : xstrdup("");
}

static void log_effective_url(HINTERNET req, const wchar_t *requested, const tr_ctx *ctx)
{
    DWORD size = 0;
    WinHttpQueryOption(req, WINHTTP_OPTION_URL, NULL, &size);
    if (!size)
        return;
    wchar_t *w = malloc(size + sizeof(wchar_t));
    if (w && WinHttpQueryOption(req, WINHTTP_OPTION_URL, w, &size)) {
        w[size / sizeof(wchar_t)] = 0;
        if (wcscmp(w, requested) != 0) {
            char *u = wide_to_utf8(w);
            tr_log(ctx, TR_LOG_WARN, "HTTP : redirigé vers %s", u ? u : "?");
            free(u);
        }
    }
    free(w);
}

/* Performs the GET. Returns 0 if a response (any status code) was received. */
static int do_request(const char *url, int tls_no_verify, const tr_ctx *ctx, tr_result *res, bbuf *body)
{
    int rc = -1;
    HINTERNET ses = NULL, con = NULL, req = NULL;
    async_state as;
    memset(&as, 0, sizeof as);
    wchar_t *wurl = utf8_to_wide(url), *host = NULL, *object = NULL;

    URL_COMPONENTS uc;
    memset(&uc, 0, sizeof uc);
    uc.dwStructSize = sizeof uc;
    uc.dwSchemeLength = (DWORD)-1;
    uc.dwHostNameLength = (DWORD)-1;
    uc.dwUrlPathLength = (DWORD)-1;
    uc.dwExtraInfoLength = (DWORD)-1;
    if (!wurl || !WinHttpCrackUrl(wurl, 0, 0, &uc)) {
        res->status = TR_ERR_URL;
        snprintf(res->error, sizeof res->error, "URL refusée par WinHTTP");
        goto out;
    }
    host = wcsndup_(uc.lpszHostName, uc.dwHostNameLength);
    if (uc.dwUrlPathLength == 0) {
        size_t n = wcslen(uc.lpszExtraInfo);
        object = malloc((n + 2) * sizeof *object);
        if (object) {
            object[0] = L'/';
            memcpy(object + 1, uc.lpszExtraInfo, (n + 1) * sizeof *object);
        }
    } else {
        object = wcsndup_(uc.lpszUrlPath, wcslen(uc.lpszUrlPath));
    }
    int https = uc.nScheme == INTERNET_SCHEME_HTTPS;

    as.ev = CreateEventW(NULL, FALSE, FALSE, NULL);
    as.closed = CreateEventW(NULL, TRUE, FALSE, NULL);
    ses = WinHttpOpen(L"AnnounceDebugger/0.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                      WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (!host || !object || !as.ev || !as.closed || !ses) {
        set_winhttp_error(res, GetLastError());
        goto out;
    }
    int t = ctx->http_timeout_ms;
    WinHttpSetTimeouts(ses, t, t, t, t);
    DWORD dec = WINHTTP_DECOMPRESSION_FLAG_ALL;
    WinHttpSetOption(ses, WINHTTP_OPTION_DECOMPRESSION, &dec, sizeof dec);

    con = WinHttpConnect(ses, host, uc.nPort, 0);
    if (con)
        req = WinHttpOpenRequest(con, L"GET", object, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 (https ? WINHTTP_FLAG_SECURE : 0) | WINHTTP_FLAG_ESCAPE_DISABLE |
                                     WINHTTP_FLAG_ESCAPE_DISABLE_QUERY);
    if (!req) {
        set_winhttp_error(res, GetLastError());
        goto out;
    }
    if (https && tls_no_verify) {
        DWORD f = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &f, sizeof f);
    }
    WinHttpSetStatusCallback(req, http_cb, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_HANDLES, 0);
    /* Set the context now so HANDLE_CLOSING reaches us even if SendRequest fails. */
    DWORD_PTR ctxval = (DWORD_PTR)&as;
    WinHttpSetOption(req, WINHTTP_OPTION_CONTEXT_VALUE, &ctxval, sizeof ctxval);

    int w = WAIT_OK;
    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR)&as)) {
        set_winhttp_error(res, GetLastError());
        goto out;
    }
    if ((w = wait_for(&as, WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, ctx)) != WAIT_OK)
        goto wait_failed;
    if (!WinHttpReceiveResponse(req, NULL)) {
        set_winhttp_error(res, GetLastError());
        goto out;
    }
    if ((w = wait_for(&as, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, ctx)) != WAIT_OK)
        goto wait_failed;

    DWORD code = 0, size = sizeof code;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &code, &size, WINHTTP_NO_HEADER_INDEX);
    res->http_code = (long)code;
    res->http_headers = query_headers(req);
    log_effective_url(req, wurl, ctx);

    for (;;) {
        if (!WinHttpQueryDataAvailable(req, NULL)) {
            set_winhttp_error(res, GetLastError());
            goto out;
        }
        if ((w = wait_for(&as, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, ctx)) != WAIT_OK)
            goto wait_failed;
        DWORD avail = as.length;
        if (avail == 0)
            break;
        if (body->len + avail > HTTP_MAX_BODY) {
            res->status = TR_ERR_PROTOCOL;
            snprintf(res->error, sizeof res->error, "réponse trop volumineuse (> %d octets)", HTTP_MAX_BODY);
            goto out;
        }
        uint8_t *chunk = malloc(avail);
        if (!chunk || !WinHttpReadData(req, chunk, avail, NULL)) {
            set_winhttp_error(res, chunk ? GetLastError() : ERROR_NOT_ENOUGH_MEMORY);
            free(chunk);
            goto out;
        }
        w = wait_for(&as, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, ctx);
        if (w == WAIT_OK)
            bbuf_append(body, chunk, as.length);
        free(chunk);
        if (w != WAIT_OK)
            goto wait_failed;
    }
    rc = 0;
    goto out;

wait_failed:
    if (w == WAIT_CANCELLED) {
        res->status = TR_ERR_CANCELLED;
        snprintf(res->error, sizeof res->error, "requête annulée");
    } else {
        set_winhttp_error(res, as.error);
    }
out:
    if (req) {
        /* Closing cancels pending operations; wait until WinHTTP is done with &as. */
        WinHttpCloseHandle(req);
        WaitForSingleObject(as.closed, 10000);
    }
    if (con)
        WinHttpCloseHandle(con);
    if (ses)
        WinHttpCloseHandle(ses);
    if (as.ev)
        CloseHandle(as.ev);
    if (as.closed)
        CloseHandle(as.closed);
    free(wurl);
    free(host);
    free(object);
    return rc;
}

void http_announce(const char *url, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res)
{
    res->request_url = http_build_url(url, p);
    tr_log(ctx, TR_LOG_INFO, "HTTP : GET %s", res->request_url);

    bbuf body;
    bbuf_init(&body);
    if (do_request(res->request_url, p->tls_no_verify, ctx, res, &body) < 0) {
        tr_log(ctx, res->status == TR_ERR_CANCELLED ? TR_LOG_WARN : TR_LOG_ERROR, "HTTP : %s", res->error);
        bbuf_free(&body);
        return;
    }
    res->raw_len = body.len;
    res->raw = (uint8_t *)bbuf_steal(&body);
    tr_log(ctx, TR_LOG_INFO, "HTTP : réponse %ld, %zu octets", res->http_code, res->raw_len);

    if (http_parse_response(res->raw, res->raw_len, res) < 0) {
        res->status = TR_ERR_PROTOCOL;
        if (res->http_code >= 400) {
            char tmp[sizeof res->error];
            snprintf(tmp, sizeof tmp, "%s", res->error);
            snprintf(res->error, sizeof res->error, "HTTP %ld, %.450s", res->http_code, tmp);
        }
    } else if (res->failure) {
        res->status = TR_ERR_TRACKER;
        snprintf(res->error, sizeof res->error, "failure reason : %.450s", res->failure);
    } else if (res->http_code >= 400) {
        res->status = TR_ERR_PROTOCOL;
        snprintf(res->error, sizeof res->error, "HTTP %ld sans failure reason", res->http_code);
    } else {
        res->status = TR_OK;
    }

    if (res->warning)
        tr_log(ctx, TR_LOG_WARN, "HTTP : warning message : %s", res->warning);
    if (res->peers_trailing)
        tr_log(ctx, TR_LOG_WARN, "HTTP : %zu octet(s) en trop dans les peers compact", res->peers_trailing);
    if (res->tracker_id) {
        free(st->trackerid);
        st->trackerid = xstrdup(res->tracker_id);
    }
    if (res->status == TR_OK)
        tr_log(ctx, TR_LOG_INFO, "HTTP : announce OK, %zu peer(s)", res->npeers);
    else
        tr_log(ctx, TR_LOG_ERROR, "HTTP : %s", res->error);
}
