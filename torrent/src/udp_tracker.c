#include <winsock2.h>
#include <ws2tcpip.h>

#include "udp_tracker.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- packets ---- */

void udp_build_connect(uint8_t out[UDP_CONNECT_LEN], uint32_t tid)
{
    be_put64(out, UDP_PROTOCOL_ID);
    be_put32(out + 8, UDP_ACTION_CONNECT);
    be_put32(out + 12, tid);
}

void udp_build_announce(uint8_t out[UDP_ANNOUNCE_LEN], uint64_t connection_id, uint32_t tid, const tr_params *p)
{
    be_put64(out, connection_id);
    be_put32(out + 8, UDP_ACTION_ANNOUNCE);
    be_put32(out + 12, tid);
    memcpy(out + 16, p->info_hash, 20);
    memcpy(out + 36, p->peer_id, 20);
    be_put64(out + 56, p->downloaded);
    be_put64(out + 64, p->left);
    be_put64(out + 72, p->uploaded);
    be_put32(out + 80, (uint32_t)p->event);
    be_put32(out + 84, 0); /* ip: 0 = use the packet source address */
    be_put32(out + 88, p->key);
    be_put32(out + 92, (uint32_t)p->numwant);
    be_put16(out + 96, p->port);
}

static void copy_text(char *dst, size_t dstlen, const uint8_t *s, size_t n)
{
    if (!dstlen)
        return;
    size_t o = 0;
    for (size_t i = 0; i < n && o + 1 < dstlen; i++)
        dst[o++] = (s[i] >= 0x20 && s[i] != 0x7f) ? (char)s[i] : '?';
    dst[o] = 0;
}

static udp_parse_rc check_header(const uint8_t *b, size_t len, uint32_t want_action, uint32_t tid, size_t min_len,
                                 char *msg, size_t msglen)
{
    if (len < 8) {
        snprintf(msg, msglen, "paquet trop court (%zu octets)", len);
        return UDP_PARSE_SHORT;
    }
    uint32_t action = be_get32(b), rtid = be_get32(b + 4);
    if (rtid != tid) {
        snprintf(msg, msglen, "transaction_id 0x%08" PRIx32 " ne correspond pas (attendu 0x%08" PRIx32 ")", rtid, tid);
        return UDP_PARSE_BAD_TID;
    }
    if (action == UDP_ACTION_ERROR) {
        copy_text(msg, msglen, b + 8, len - 8);
        return UDP_PARSE_TRACKER_ERROR;
    }
    if (action != want_action) {
        snprintf(msg, msglen, "action %" PRIu32 " inattendue (attendu %" PRIu32 ")", action, want_action);
        return UDP_PARSE_BAD_ACTION;
    }
    if (len < min_len) {
        snprintf(msg, msglen, "réponse trop courte (%zu octets, minimum %zu)", len, min_len);
        return UDP_PARSE_SHORT;
    }
    return UDP_PARSE_OK;
}

udp_parse_rc udp_parse_connect(const uint8_t *b, size_t len, uint32_t tid, uint64_t *connection_id, char *msg,
                               size_t msglen)
{
    udp_parse_rc rc = check_header(b, len, UDP_ACTION_CONNECT, tid, 16, msg, msglen);
    if (rc == UDP_PARSE_OK)
        *connection_id = be_get64(b + 8);
    return rc;
}

udp_parse_rc udp_parse_announce(const uint8_t *b, size_t len, uint32_t tid, int ipv6, tr_result *r, char *msg,
                                size_t msglen)
{
    udp_parse_rc rc = check_header(b, len, UDP_ACTION_ANNOUNCE, tid, 20, msg, msglen);
    if (rc != UDP_PARSE_OK)
        return rc;
    r->has_interval = r->has_incomplete = r->has_complete = 1;
    r->interval = be_get32(b + 8);
    r->incomplete = be_get32(b + 12);
    r->complete = be_get32(b + 16);
    r->peers_trailing = tr_parse_compact_peers(b + 20, len - 20, ipv6, r);
    return UDP_PARSE_OK;
}

/* ---- annotations ---- */

static void annot(bbuf *o, size_t off, size_t len, const char *name, const char *fmt, ...) PRINTF_FMT(5, 6);
static void annot(bbuf *o, size_t off, size_t len, const char *name, const char *fmt, ...)
{
    char val[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(val, sizeof val, fmt, ap);
    va_end(ap);
    bbuf_printf(o, "  %4zu  %3zu  %-16s %s\n", off, len, name, val);
}

static void annot_header(bbuf *o)
{
    bbuf_printf(o, "  %4s  %3s  %-16s %s\n", "off", "len", "champ", "valeur");
}

static const char *action_name(uint32_t a)
{
    switch (a) {
    case UDP_ACTION_CONNECT: return "connect";
    case UDP_ACTION_ANNOUNCE: return "announce";
    case UDP_ACTION_SCRAPE: return "scrape";
    case UDP_ACTION_ERROR: return "error";
    }
    return "inconnue";
}

void udp_annotate_connect_req(const uint8_t *p, bbuf *o)
{
    annot_header(o);
    annot(o, 0, 8, "protocol_id", "0x%016" PRIx64, be_get64(p));
    annot(o, 8, 4, "action", "%" PRIu32 " (%s)", be_get32(p + 8), action_name(be_get32(p + 8)));
    annot(o, 12, 4, "transaction_id", "0x%08" PRIx32, be_get32(p + 12));
}

void udp_annotate_announce_req(const uint8_t *p, bbuf *o)
{
    char hex[41], pid[100];
    hex_encode(p + 16, 20, hex);
    tr_peer_id_str(p + 36, 20, pid, sizeof pid);
    uint32_t ev = be_get32(p + 80);
    annot_header(o);
    annot(o, 0, 8, "connection_id", "0x%016" PRIx64, be_get64(p));
    annot(o, 8, 4, "action", "%" PRIu32 " (%s)", be_get32(p + 8), action_name(be_get32(p + 8)));
    annot(o, 12, 4, "transaction_id", "0x%08" PRIx32, be_get32(p + 12));
    annot(o, 16, 20, "info_hash", "%s", hex);
    annot(o, 36, 20, "peer_id", "%s", pid);
    annot(o, 56, 8, "downloaded", "%" PRIu64, be_get64(p + 56));
    annot(o, 64, 8, "left", "%" PRIu64, be_get64(p + 64));
    annot(o, 72, 8, "uploaded", "%" PRIu64, be_get64(p + 72));
    annot(o, 80, 4, "event", "%" PRIu32 " (%s)", ev, ev <= 3 ? tr_event_name((tr_event)ev) : "?");
    annot(o, 84, 4, "ip", "%u.%u.%u.%u (0 = adresse source)", p[84], p[85], p[86], p[87]);
    annot(o, 88, 4, "key", "0x%08" PRIx32, be_get32(p + 88));
    annot(o, 92, 4, "num_want", "%" PRId32 "%s", (int32_t)be_get32(p + 92),
          (int32_t)be_get32(p + 92) == -1 ? " (défaut)" : "");
    annot(o, 96, 2, "port", "%u", be_get16(p + 96));
}

void udp_annotate_response(const uint8_t *p, size_t len, int ipv6, bbuf *o)
{
    annot_header(o);
    if (len < 8) {
        annot(o, 0, len, "(tronqué)", "paquet de %zu octets, en-tête incomplet", len);
        return;
    }
    uint32_t action = be_get32(p);
    annot(o, 0, 4, "action", "%" PRIu32 " (%s)", action, action_name(action));
    annot(o, 4, 4, "transaction_id", "0x%08" PRIx32, be_get32(p + 4));
    size_t off = 8;
    if (action == UDP_ACTION_CONNECT && len >= 16) {
        annot(o, 8, 8, "connection_id", "0x%016" PRIx64, be_get64(p + 8));
        off = 16;
    } else if (action == UDP_ACTION_ANNOUNCE && len >= 20) {
        annot(o, 8, 4, "interval", "%" PRIu32 " s", be_get32(p + 8));
        annot(o, 12, 4, "leechers", "%" PRIu32, be_get32(p + 12));
        annot(o, 16, 4, "seeders", "%" PRIu32, be_get32(p + 16));
        size_t sz = ipv6 ? 18 : 6;
        size_t i = 0;
        for (off = 20; off + sz <= len; off += sz, i++) {
            char ip[INET6_ADDRSTRLEN], name[32];
            inet_ntop(ipv6 ? AF_INET6 : AF_INET, p + off, ip, sizeof ip);
            snprintf(name, sizeof name, "peer[%zu]", i);
            annot(o, off, sz, name, ipv6 ? "[%s]:%u" : "%s:%u", ip, be_get16(p + off + sz - 2));
        }
    } else if (action == UDP_ACTION_ERROR) {
        char msg[200];
        copy_text(msg, sizeof msg, p + 8, len - 8);
        annot(o, 8, len - 8, "message", "\"%s\"", msg);
        off = len;
    }
    if (off < len)
        annot(o, off, len - off, "(restant)", "%zu octet(s) non interprété(s)", len - off);
}

/* ---- network ---- */

static void fail(tr_result *res, const tr_ctx *ctx, tr_status st, const char *fmt, ...) PRINTF_FMT(4, 5);
static void fail(tr_result *res, const tr_ctx *ctx, tr_status st, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(res->error, sizeof res->error, fmt, ap);
    va_end(ap);
    res->status = st;
    tr_log(ctx, st == TR_ERR_CANCELLED ? TR_LOG_WARN : TR_LOG_ERROR, "UDP : %s", res->error);
}

static void keep_raw(uint8_t **dst, size_t *dstlen, const uint8_t *b, size_t n)
{
    free(*dst);
    *dst = malloc(n ? n : 1);
    if (*dst) {
        memcpy(*dst, b, n);
        *dstlen = n;
    } else {
        *dstlen = 0;
    }
}

static void sock_error(int code, char *buf, size_t n)
{
    win_error_text((unsigned long)code, NULL, buf, n);
}

static SOCKET open_socket(const tr_url *u, const tr_ctx *ctx, tr_result *res, char *endpoint, size_t eplen)
{
    char portstr[8];
    snprintf(portstr, sizeof portstr, "%u", u->port);
    struct addrinfo hints, *ai = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    tr_log(ctx, TR_LOG_INFO, "UDP : résolution de %s", u->host);
    int g = getaddrinfo(u->host, portstr, &hints, &ai);
    if (g != 0) {
        char msg[256];
        sock_error(g, msg, sizeof msg);
        fail(res, ctx, TR_ERR_DNS, "résolution DNS de %s échouée : %s", u->host, msg);
        return INVALID_SOCKET;
    }

    SOCKET fd = INVALID_SOCKET;
    int last_err = 0;
    /* Pass 0: IPv4 addresses, pass 1: IPv6 fallback. */
    for (int pass = 0; pass < 2 && fd == INVALID_SOCKET; pass++) {
        for (struct addrinfo *a = ai; a && fd == INVALID_SOCKET; a = a->ai_next) {
            int want = pass == 0 ? AF_INET : AF_INET6;
            if (a->ai_family != want)
                continue;
            SOCKET s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            if (s == INVALID_SOCKET) {
                last_err = WSAGetLastError();
                continue;
            }
            if (connect(s, a->ai_addr, (int)a->ai_addrlen) == SOCKET_ERROR) {
                last_err = WSAGetLastError();
                closesocket(s);
                continue;
            }
            fd = s;
            res->ipv6 = a->ai_family == AF_INET6;
            char ip[INET6_ADDRSTRLEN] = "?";
            const void *addr = res->ipv6 ? (const void *)&((struct sockaddr_in6 *)a->ai_addr)->sin6_addr
                                         : (const void *)&((struct sockaddr_in *)a->ai_addr)->sin_addr;
            inet_ntop(a->ai_family, addr, ip, sizeof ip);
            snprintf(endpoint, eplen, res->ipv6 ? "[%s]:%u" : "%s:%u", ip, u->port);
        }
    }
    freeaddrinfo(ai);
    if (fd == INVALID_SOCKET) {
        char msg[256] = "pas d'adresse IPv4/IPv6";
        if (last_err)
            sock_error(last_err, msg, sizeof msg);
        fail(res, ctx, TR_ERR_NET, "aucune adresse utilisable pour %s : %s", u->host, msg);
        return INVALID_SOCKET;
    }
    snprintf(res->resolved, sizeof res->resolved, "%s:%u → %s (%s)", u->host, u->port, endpoint,
             res->ipv6 ? "IPv6" : "IPv4");
    tr_log(ctx, TR_LOG_INFO, "UDP : %s", res->resolved);
    return fd;
}

static int conn_id_valid(const tr_state *st, const char *endpoint)
{
    return st->conn_valid && strcmp(st->conn_endpoint, endpoint) == 0 &&
           mono_ms() - st->conn_obtained_ms < UDP_CONN_ID_LIFETIME_MS;
}

enum { PH_CONNECT, PH_ANNOUNCE };

void udp_announce(const tr_url *u, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res)
{
    char endpoint[sizeof st->conn_endpoint];
    SOCKET fd = open_socket(u, ctx, res, endpoint, sizeof endpoint);
    if (fd == INVALID_SOCKET)
        return;

    int phase;
    if (conn_id_valid(st, endpoint)) {
        phase = PH_ANNOUNCE;
        res->conn_reused = 1;
        res->connection_id = st->connection_id;
        tr_log(ctx, TR_LOG_INFO, "UDP : connection_id 0x%016" PRIx64 " réutilisé (obtenu il y a %.1f s)",
               st->connection_id, (double)(mono_ms() - st->conn_obtained_ms) / 1000.0);
    } else {
        phase = PH_CONNECT;
        if (st->conn_valid && strcmp(st->conn_endpoint, endpoint) == 0)
            tr_log(ctx, TR_LOG_INFO, "UDP : connection_id expiré (obtenu il y a %.1f s), renouvellement",
                   (double)(mono_ms() - st->conn_obtained_ms) / 1000.0);
        else
            tr_log(ctx, TR_LOG_INFO, "UDP : pas de connection_id valide, envoi d'un connect");
        st->conn_valid = 0;
    }

    uint8_t buf[UDP_RECV_BUF];
    uint32_t tid = 0;
    int n = 0;           /* retransmission exponent */
    int new_phase = 1;   /* draw a fresh transaction_id */
    int need_send = 1;
    int64_t sent_at = 0, deadline = 0;

    for (;;) {
        if (need_send) {
            if (phase == PH_ANNOUNCE && !conn_id_valid(st, endpoint)) {
                tr_log(ctx, TR_LOG_WARN, "UDP : connection_id expiré pendant les retransmissions, nouveau connect");
                st->conn_valid = 0;
                res->conn_reused = 0;
                phase = PH_CONNECT;
                new_phase = 1;
                n = 0;
            }
            if (new_phase) {
                tid = random_u32();
                new_phase = 0;
            }
            const uint8_t *pkt;
            size_t pktlen;
            if (phase == PH_CONNECT) {
                udp_build_connect(res->connect_pkt, tid);
                res->has_connect_pkt = 1;
                pkt = res->connect_pkt;
                pktlen = UDP_CONNECT_LEN;
            } else {
                udp_build_announce(res->announce_pkt, st->connection_id, tid, p);
                res->has_announce_pkt = 1;
                pkt = res->announce_pkt;
                pktlen = UDP_ANNOUNCE_LEN;
            }
            int w = send(fd, (const char *)pkt, (int)pktlen, 0);
            if (w != (int)pktlen) {
                char emsg[256] = "envoi partiel";
                if (w == SOCKET_ERROR)
                    sock_error(WSAGetLastError(), emsg, sizeof emsg);
                fail(res, ctx, TR_ERR_NET, "envoi du paquet impossible : %s", emsg);
                break;
            }
            int64_t timeout = (int64_t)ctx->udp_base_timeout_ms << n;
            sent_at = mono_ms();
            deadline = sent_at + timeout;
            if (n == 0)
                tr_log(ctx, TR_LOG_INFO, "UDP : → %s envoyé (%zu octets, transaction_id 0x%08" PRIx32 ")",
                       phase == PH_CONNECT ? "connect" : "announce", pktlen, tid);
            need_send = 0;
        }

        if (tr_cancelled(ctx)) {
            fail(res, ctx, TR_ERR_CANCELLED, "annulé par l'utilisateur");
            break;
        }
        int64_t rem = deadline - mono_ms();
        if (rem <= 0) {
            const char *what = phase == PH_CONNECT ? "connect" : "announce";
            double waited = (double)(mono_ms() - sent_at) / 1000.0;
            if (n >= ctx->udp_max_retries) {
                fail(res, ctx, TR_ERR_TIMEOUT, "pas de réponse au %s après %d retransmission(s), abandon", what, n);
                break;
            }
            n++;
            res->retransmissions++;
            tr_log(ctx, TR_LOG_WARN, "UDP : pas de réponse au %s après %.1f s, retransmission n°%d (délai suivant %.1f s)",
                   what, waited, n, (double)((int64_t)ctx->udp_base_timeout_ms << n) / 1000.0);
            need_send = 1;
            continue;
        }

        /* Wait at most 100 ms at a time so Cancel stays responsive. */
        fd_set rs;
        FD_ZERO(&rs);
        FD_SET(fd, &rs);
        int64_t slice = rem > 100 ? 100 : rem;
        struct timeval tv = { 0, (long)(slice * 1000) };
        int sr = select(0, &rs, NULL, NULL, &tv);
        if (sr == SOCKET_ERROR) {
            char emsg[256];
            sock_error(WSAGetLastError(), emsg, sizeof emsg);
            fail(res, ctx, TR_ERR_NET, "select : %s", emsg);
            break;
        }
        if (sr == 0)
            continue;
        int r = recv(fd, (char *)buf, (int)sizeof buf, 0);
        if (r == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e == WSAEINTR || e == WSAEWOULDBLOCK)
                continue;
            char emsg[256];
            sock_error(e, emsg, sizeof emsg);
            if (e == WSAECONNRESET)
                fail(res, ctx, TR_ERR_NET, "port UDP fermé (ICMP port unreachable reçu de %s)", endpoint);
            else if (e == WSAEMSGSIZE)
                fail(res, ctx, TR_ERR_PROTOCOL, "datagramme plus grand que le buffer de %d octets", UDP_RECV_BUF);
            else
                fail(res, ctx, TR_ERR_NET, "réception : %s", emsg);
            break;
        }
        size_t rl = (size_t)r;
        char msg[256];

        if (phase == PH_CONNECT) {
            uint64_t cid = 0;
            udp_parse_rc rc = udp_parse_connect(buf, rl, tid, &cid, msg, sizeof msg);
            if (rc == UDP_PARSE_BAD_TID) {
                tr_log(ctx, TR_LOG_WARN, "UDP : paquet ignoré (%zu octets) : %s", rl, msg);
                continue;
            }
            if (rc != UDP_PARSE_OK) {
                keep_raw(&res->raw, &res->raw_len, buf, rl);
                res->udp_action = rl >= 4 ? (int)be_get32(buf) : -1;
                if (rc == UDP_PARSE_TRACKER_ERROR) {
                    res->failure = xstrndup(buf + 8, rl - 8);
                    fail(res, ctx, TR_ERR_TRACKER, "erreur du tracker au connect : %s", msg);
                } else {
                    fail(res, ctx, TR_ERR_PROTOCOL, "réponse connect invalide : %s", msg);
                }
                break;
            }
            keep_raw(&res->connect_resp, &res->connect_resp_len, buf, rl);
            st->connection_id = cid;
            st->conn_valid = 1;
            st->conn_obtained_ms = mono_ms();
            snprintf(st->conn_endpoint, sizeof st->conn_endpoint, "%s", endpoint);
            res->connection_id = cid;
            tr_log(ctx, TR_LOG_INFO, "UDP : ← connect OK en %.0f ms, connection_id 0x%016" PRIx64,
                   (double)(mono_ms() - sent_at), cid);
            phase = PH_ANNOUNCE;
            new_phase = 1;
            need_send = 1;
            n = 0;
            continue;
        }

        udp_parse_rc rc = udp_parse_announce(buf, rl, tid, res->ipv6, res, msg, sizeof msg);
        if (rc == UDP_PARSE_BAD_TID) {
            tr_log(ctx, TR_LOG_WARN, "UDP : paquet ignoré (%zu octets) : %s", rl, msg);
            continue;
        }
        keep_raw(&res->raw, &res->raw_len, buf, rl);
        res->udp_action = rl >= 4 ? (int)be_get32(buf) : -1;
        if (rc == UDP_PARSE_TRACKER_ERROR) {
            res->failure = xstrndup(buf + 8, rl - 8);
            fail(res, ctx, TR_ERR_TRACKER, "erreur du tracker : %s", msg);
        } else if (rc != UDP_PARSE_OK) {
            fail(res, ctx, TR_ERR_PROTOCOL, "réponse announce invalide : %s", msg);
        } else {
            res->status = TR_OK;
            if (res->peers_trailing)
                tr_log(ctx, TR_LOG_WARN, "UDP : %zu octet(s) en trop après les peers", res->peers_trailing);
            tr_log(ctx, TR_LOG_INFO,
                   "UDP : ← announce OK en %.0f ms : interval %" PRId64 " s, seeders %" PRId64 ", leechers %" PRId64
                   ", %zu peer(s)",
                   (double)(mono_ms() - sent_at), res->interval, res->complete, res->incomplete, res->npeers);
        }
        break;
    }
    closesocket(fd);
}
