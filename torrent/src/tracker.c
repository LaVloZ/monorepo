#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "tracker.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bencode.h"
#include "http_tracker.h"
#include "udp_tracker.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

void tr_global_init(void)
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

void tr_global_cleanup(void)
{
    WSACleanup();
}

void tr_cancel_set(volatile long *flag, long value)
{
    InterlockedExchange((volatile LONG *)flag, value);
}

void tr_ctx_defaults(tr_ctx *ctx)
{
    memset(ctx, 0, sizeof *ctx);
    ctx->udp_base_timeout_ms = 15000;
    ctx->udp_max_retries = 8;
    ctx->http_timeout_ms = 30000;
}

void tr_log(const tr_ctx *ctx, int level, const char *fmt, ...)
{
    if (!ctx || !ctx->log)
        return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    ctx->log(ctx->log_ud, level, buf);
}

int tr_cancelled(const tr_ctx *ctx)
{
    return ctx && ctx->cancel && InterlockedCompareExchange((volatile LONG *)ctx->cancel, 0, 0) != 0;
}

const char *tr_event_name(tr_event e)
{
    switch (e) {
    case TR_EVENT_NONE: return "none";
    case TR_EVENT_COMPLETED: return "completed";
    case TR_EVENT_STARTED: return "started";
    case TR_EVENT_STOPPED: return "stopped";
    }
    return "?";
}

const char *tr_status_name(tr_status s)
{
    switch (s) {
    case TR_OK: return "OK";
    case TR_ERR_URL: return "URL invalide";
    case TR_ERR_DNS: return "Erreur DNS";
    case TR_ERR_NET: return "Erreur réseau";
    case TR_ERR_TIMEOUT: return "Timeout";
    case TR_ERR_CANCELLED: return "Annulé";
    case TR_ERR_PROTOCOL: return "Réponse invalide";
    case TR_ERR_TRACKER: return "Erreur tracker";
    }
    return "?";
}

const char *tr_transport_name(tr_transport t)
{
    return t == TR_UDP ? "UDP" : "HTTP";
}

/* ---- URL parsing ---- */

static void url_err(char *err, size_t errlen, const char *msg)
{
    if (err && errlen)
        snprintf(err, errlen, "%s", msg);
}

int tr_parse_url(const char *url, tr_url *out, char *err, size_t errlen)
{
    memset(out, 0, sizeof *out);
    if (!url || !*url) {
        url_err(err, errlen, "URL vide");
        return -1;
    }
    const char *sep = strstr(url, "://");
    if (!sep) {
        url_err(err, errlen, "schéma manquant (http://, https://, udp://)");
        return -1;
    }
    size_t sl = (size_t)(sep - url);
    int default_port = 0;
    if (sl == 4 && _strnicmp(url, "http", 4) == 0) {
        out->transport = TR_HTTP;
        default_port = 80;
    } else if (sl == 5 && _strnicmp(url, "https", 5) == 0) {
        out->transport = TR_HTTP;
        out->tls = 1;
        default_port = 443;
    } else if (sl == 3 && _strnicmp(url, "udp", 3) == 0) {
        out->transport = TR_UDP;
    } else {
        url_err(err, errlen, "schéma non supporté (attendu http://, https:// ou udp://)");
        return -1;
    }

    const char *a = sep + 3;
    size_t alen = strcspn(a, "/?#");
    char auth[300];
    if (alen >= sizeof auth) {
        url_err(err, errlen, "hôte trop long");
        return -1;
    }
    memcpy(auth, a, alen);
    auth[alen] = 0;
    char *host = strrchr(auth, '@');
    host = host ? host + 1 : auth;

    char *port = NULL;
    if (host[0] == '[') {
        char *end = strchr(host, ']');
        if (!end) {
            url_err(err, errlen, "adresse IPv6 sans ']'");
            return -1;
        }
        *end = 0;
        host++;
        if (end[1] == ':')
            port = end + 2;
        else if (end[1]) {
            url_err(err, errlen, "caractères inattendus après l'adresse IPv6");
            return -1;
        }
    } else {
        char *colon = strchr(host, ':');
        if (colon) {
            if (strchr(colon + 1, ':')) {
                url_err(err, errlen, "adresse IPv6 à mettre entre crochets");
                return -1;
            }
            *colon = 0;
            port = colon + 1;
        }
    }
    if (!host[0]) {
        url_err(err, errlen, "hôte manquant");
        return -1;
    }
    if (strlen(host) >= sizeof out->host) {
        url_err(err, errlen, "hôte trop long");
        return -1;
    }
    strcpy(out->host, host);

    if (port && *port) {
        char *endp;
        unsigned long v = strtoul(port, &endp, 10);
        if (*endp || v == 0 || v > 65535 || port[0] == '-' || port[0] == '+') {
            url_err(err, errlen, "port invalide");
            return -1;
        }
        out->port = (uint16_t)v;
    } else if (out->transport == TR_UDP) {
        url_err(err, errlen, "port obligatoire pour udp:// (ex. udp://tracker:6969/announce)");
        return -1;
    } else {
        out->port = (uint16_t)default_port;
    }
    return 0;
}

/* ---- results ---- */

void tr_result_init(tr_result *r)
{
    memset(r, 0, sizeof *r);
    r->udp_action = -1;
}

void tr_result_free(tr_result *r)
{
    free(r->url);
    free(r->request_url);
    free(r->http_headers);
    free(r->connect_resp);
    free(r->raw);
    free(r->tracker_id);
    free(r->warning);
    free(r->failure);
    free(r->retry_in);
    free(r->other_keys);
    free(r->peers);
    tr_result_init(r);
}

void tr_state_free(tr_state *st)
{
    free(st->trackerid);
    memset(st, 0, sizeof *st);
}

int tr_result_add_peer(tr_result *r, const tr_peer *p)
{
    tr_peer *n = realloc(r->peers, (r->npeers + 1) * sizeof *n);
    if (!n)
        return -1;
    r->peers = n;
    r->peers[r->npeers++] = *p;
    return 0;
}

size_t tr_parse_compact_peers(const uint8_t *data, size_t len, int ipv6, tr_result *r)
{
    size_t sz = ipv6 ? 18 : 6;
    size_t alen = ipv6 ? 16 : 4;
    for (size_t off = 0; off + sz <= len; off += sz) {
        tr_peer p;
        memset(&p, 0, sizeof p);
        inet_ntop(ipv6 ? AF_INET6 : AF_INET, data + off, p.ip, sizeof p.ip);
        p.port = be_get16(data + off + alen);
        if (tr_result_add_peer(r, &p) < 0)
            break;
    }
    return len % sz;
}

void tr_generate_peer_id(uint8_t out[20])
{
    static const char alnum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t plen = strlen(TR_PEER_ID_PREFIX);
    memcpy(out, TR_PEER_ID_PREFIX, plen);
    uint8_t rnd[20];
    random_bytes(rnd, sizeof rnd);
    for (size_t i = plen; i < 20; i++)
        out[i] = (uint8_t)alnum[rnd[i] % 62];
}

/* Printable bytes as-is, others as %XX (like in an HTTP query). */
void tr_peer_id_str(const uint8_t *id, size_t len, char *out, size_t outlen)
{
    size_t o = 0;
    for (size_t i = 0; i < len && o + 4 < outlen; i++) {
        if (id[i] >= 0x21 && id[i] < 0x7f && id[i] != '%')
            out[o++] = (char)id[i];
        else
            o += (size_t)snprintf(out + o, outlen - o, "%%%02X", id[i]);
    }
    if (outlen)
        out[o < outlen ? o : outlen - 1] = 0;
}

/* ---- dispatch ---- */

void tr_announce(const char *url, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res)
{
    tr_result_init(res);
    res->url = xstrdup(url);
    res->params = *p;
    res->params.trackerid = NULL;

    tr_url u;
    char err[256];
    if (tr_parse_url(url, &u, err, sizeof err) < 0) {
        res->status = TR_ERR_URL;
        snprintf(res->error, sizeof res->error, "URL invalide : %s", err);
        tr_log(ctx, TR_LOG_ERROR, "%s", res->error);
        return;
    }
    res->transport = u.transport;
    int64_t t0 = mono_us();
    if (u.transport == TR_HTTP)
        http_announce(url, p, st, ctx, res);
    else
        udp_announce(&u, p, st, ctx, res);
    res->elapsed_ms = (double)(mono_us() - t0) / 1000.0;
}

/* ---- formatting ---- */

static void dump_block(bbuf *b, const char *title, const uint8_t *data, size_t len)
{
    char *hd = hexdump(data, len);
    bbuf_printf(b, "%s\n%s\n", title, hd);
    free(hd);
}

static void format_params(bbuf *b, const tr_params *p, tr_transport t)
{
    char hex[41], pid[100];
    hex_encode(p->info_hash, 20, hex);
    tr_peer_id_str(p->peer_id, 20, pid, sizeof pid);
    bbuf_printf(b, "Paramètres :\n");
    bbuf_printf(b, "  info_hash   %s\n", hex);
    bbuf_printf(b, "  peer_id     %s\n", pid);
    bbuf_printf(b, "  port        %u\n", p->port);
    bbuf_printf(b, "  uploaded    %" PRIu64 "\n", p->uploaded);
    bbuf_printf(b, "  downloaded  %" PRIu64 "\n", p->downloaded);
    bbuf_printf(b, "  left        %" PRIu64 "\n", p->left);
    if (t == TR_HTTP)
        bbuf_printf(b, "  compact     %d\n", p->compact ? 1 : 0);
    bbuf_printf(b, "  numwant     %" PRId32 "%s\n", p->numwant, p->numwant < 0 ? " (défaut du tracker)" : "");
    bbuf_printf(b, "  key         %08X\n", p->key);
    bbuf_printf(b, "  event       %s\n", tr_event_name(p->event));
}

char *tr_format_request(const tr_result *r)
{
    bbuf b;
    bbuf_init(&b);
    if (r->status == TR_ERR_URL) {
        bbuf_printf(&b, "Aucune requête envoyée : %s\n", r->error);
        return bbuf_steal(&b);
    }
    if (r->transport == TR_HTTP) {
        if (r->request_url)
            bbuf_printf(&b, "GET %s\n\n", r->request_url);
        format_params(&b, &r->params, TR_HTTP);
        return bbuf_steal(&b);
    }

    bbuf_printf(&b, "Tracker        %s\n", r->url ? r->url : "");
    bbuf_printf(&b, "Résolution     %s\n", r->resolved[0] ? r->resolved : "(échec)");
    if (r->conn_reused)
        bbuf_printf(&b, "connection_id  0x%016" PRIx64 " réutilisé (pas de connect)\n", r->connection_id);
    else if (r->has_connect_pkt)
        bbuf_printf(&b, "connection_id  obtenu par un nouveau connect\n");
    if (r->retransmissions)
        bbuf_printf(&b, "Retransmissions %d\n", r->retransmissions);
    bbuf_printf(&b, "\n");
    format_params(&b, &r->params, TR_UDP);
    if (r->has_connect_pkt) {
        bbuf_printf(&b, "\n== Paquet connect envoyé (%d octets) ==\n", UDP_CONNECT_LEN);
        udp_annotate_connect_req(r->connect_pkt, &b);
        dump_block(&b, "", r->connect_pkt, UDP_CONNECT_LEN);
    }
    if (r->has_announce_pkt) {
        bbuf_printf(&b, "\n== Paquet announce envoyé (%d octets) ==\n", UDP_ANNOUNCE_LEN);
        udp_annotate_announce_req(r->announce_pkt, &b);
        dump_block(&b, "", r->announce_pkt, UDP_ANNOUNCE_LEN);
    }
    return bbuf_steal(&b);
}

char *tr_format_raw(const tr_result *r)
{
    bbuf b;
    bbuf_init(&b);
    if (r->transport == TR_HTTP) {
        if (r->http_code)
            bbuf_printf(&b, "HTTP %ld\n\n", r->http_code);
        if (r->http_headers && r->http_headers[0])
            bbuf_printf(&b, "== En-têtes ==\n%s\n", r->http_headers);
        if (!r->raw) {
            bbuf_printf(&b, "(aucun corps de réponse reçu)\n");
            return bbuf_steal(&b);
        }
        char title[64];
        snprintf(title, sizeof title, "== Corps (%zu octets) ==", r->raw_len);
        dump_block(&b, title, r->raw, r->raw_len);
        char err[200];
        ben_node *n = ben_decode(r->raw, r->raw_len, err, sizeof err);
        if (n) {
            bbuf_printf(&b, "== Bencode décodé ==\n");
            ben_pretty(n, &b, 0);
            bbuf_printf(&b, "\n");
            ben_free(n);
        } else {
            bbuf_printf(&b, "== Bencode ==\nnon décodable : %s\n", err);
        }
        return bbuf_steal(&b);
    }

    if (r->connect_resp) {
        bbuf_printf(&b, "== Réponse connect (%zu octets) ==\n", r->connect_resp_len);
        udp_annotate_response(r->connect_resp, r->connect_resp_len, r->ipv6, &b);
        dump_block(&b, "", r->connect_resp, r->connect_resp_len);
    }
    if (r->raw) {
        bbuf_printf(&b, "%s== Réponse %s (%zu octets) ==\n", r->connect_resp ? "\n" : "",
                    r->udp_action == UDP_ACTION_ERROR ? "erreur" : "announce", r->raw_len);
        udp_annotate_response(r->raw, r->raw_len, r->ipv6, &b);
        dump_block(&b, "", r->raw, r->raw_len);
    }
    if (!r->connect_resp && !r->raw)
        bbuf_printf(&b, "(aucune réponse reçue)\n");
    return bbuf_steal(&b);
}
