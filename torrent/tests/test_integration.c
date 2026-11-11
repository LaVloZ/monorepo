/* Integration tests: tr_announce() against local fake HTTP and UDP trackers (127.0.0.1, ephemeral ports). */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bencode.h"
#include "peer_probe.h"
#include "tracker.h"
#include "udp_tracker.h"
#include "util.h"

static int checks, failures;

#define CHECK(cond)                                                            \
    do {                                                                       \
        checks++;                                                              \
        if (!(cond)) {                                                         \
            failures++;                                                        \
            fprintf(stderr, "%s:%d: échec : %s\n", __FILE__, __LINE__, #cond); \
        }                                                                      \
    } while (0)

#define CHECK_STR(a, b) CHECK((a) && strcmp((a), (b)) == 0)

/* ---- common ---- */

static void cap_log(void *ud, int level, const char *msg)
{
    (void)level;
    bbuf_printf(ud, "%s\n", msg);
}

static const char *log_text(const bbuf *b)
{
    return b->data ? (const char *)b->data : "";
}

static void make_ctx(tr_ctx *ctx, bbuf *log, volatile long *cancel)
{
    tr_ctx_defaults(ctx);
    ctx->log = cap_log;
    ctx->log_ud = log;
    ctx->cancel = cancel;
    ctx->udp_base_timeout_ms = 200;
    ctx->udp_max_retries = 2;
    ctx->http_timeout_ms = 5000;
}

static void make_params(tr_params *p)
{
    memset(p, 0, sizeof *p);
    for (int i = 0; i < 20; i++)
        p->info_hash[i] = (uint8_t)(0xA0 + i);
    memcpy(p->peer_id, "-DB0001-TESTTESTTEST", 20);
    p->port = 6881;
    p->left = 12345;
    p->numwant = 50;
    p->compact = 1;
    p->key = 0x01020304;
    p->event = TR_EVENT_STARTED;
}

static SOCKET loopback_socket(int type, uint16_t *port)
{
    SOCKET fd = socket(AF_INET, type, 0);
    if (fd == INVALID_SOCKET)
        return INVALID_SOCKET;
    struct sockaddr_in a;
    memset(&a, 0, sizeof a);
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int al = sizeof a;
    if (bind(fd, (struct sockaddr *)&a, sizeof a) == SOCKET_ERROR ||
        getsockname(fd, (struct sockaddr *)&a, &al) == SOCKET_ERROR) {
        closesocket(fd);
        return INVALID_SOCKET;
    }
    *port = ntohs(a.sin_port);
    return fd;
}

static long counter(volatile long *c)
{
    return InterlockedCompareExchange((volatile LONG *)c, 0, 0);
}

/* ---- fake HTTP tracker (one connection, canned response) ---- */

typedef struct {
    SOCKET lfd;
    uint16_t port;
    char request[8192];
    const char *status_line;
    const void *body;
    size_t body_len;
    HANDLE th;
} http_srv;

static DWORD WINAPI http_srv_main(LPVOID arg)
{
    http_srv *s = arg;
    SOCKET c = accept(s->lfd, NULL, NULL);
    if (c == INVALID_SOCKET)
        return 0;
    size_t n = 0;
    while (n < sizeof s->request - 1) {
        int r = recv(c, s->request + n, (int)(sizeof s->request - 1 - n), 0);
        if (r <= 0)
            break;
        n += (size_t)r;
        s->request[n] = 0;
        if (strstr(s->request, "\r\n\r\n"))
            break;
    }
    char hdr[256];
    int hl = snprintf(hdr, sizeof hdr,
                      "HTTP/1.1 %s\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
                      s->status_line, s->body_len);
    send(c, hdr, hl, 0);
    send(c, (const char *)s->body, (int)s->body_len, 0);
    shutdown(c, SD_SEND);
    closesocket(c);
    return 0;
}

static int http_srv_start(http_srv *s, const char *status, const void *body, size_t len)
{
    memset(s, 0, sizeof *s);
    s->status_line = status;
    s->body = body;
    s->body_len = len;
    s->lfd = loopback_socket(SOCK_STREAM, &s->port);
    if (s->lfd == INVALID_SOCKET || listen(s->lfd, 4) == SOCKET_ERROR)
        return -1;
    s->th = CreateThread(NULL, 0, http_srv_main, s, 0, NULL);
    return s->th ? 0 : -1;
}

static void http_srv_stop(http_srv *s)
{
    closesocket(s->lfd); /* unblocks accept() if the client never connected */
    WaitForSingleObject(s->th, INFINITE);
    CloseHandle(s->th);
}

static void http_announce_to(http_srv *s, const char *path, tr_params *p, tr_state *st, tr_ctx *ctx, tr_result *r)
{
    char url[256];
    snprintf(url, sizeof url, "http://127.0.0.1:%u%s", s->port, path);
    tr_announce(url, p, st, ctx, r);
    http_srv_stop(s);
}

static void test_http(void)
{
    bbuf log;
    bbuf_init(&log);
    volatile long cancel = 0;
    tr_ctx ctx;
    make_ctx(&ctx, &log, &cancel);
    tr_params p;
    make_params(&p);
    tr_state st;
    memset(&st, 0, sizeof st);
    tr_result r;
    http_srv s;

    /* 1. Successful announce with compact peers, peers6 and tracker id. */
    ben_node *d = ben_new_dict();
    ben_dict_put(d, "interval", ben_new_int(1800));
    ben_dict_put(d, "min interval", ben_new_int(900));
    ben_dict_put(d, "complete", ben_new_int(3));
    ben_dict_put(d, "incomplete", ben_new_int(4));
    ben_dict_put(d, "tracker id", ben_new_strz("abc"));
    const uint8_t peers[12] = { 127, 0, 0, 1, 0x1A, 0xE1, 192, 168, 0, 2, 0x1A, 0xE2 };
    ben_dict_put(d, "peers", ben_new_str(peers, 12));
    uint8_t v6[18] = { 0 };
    v6[15] = 1;
    v6[16] = 0x1A;
    v6[17] = 0xE3;
    ben_dict_put(d, "peers6", ben_new_str(v6, 18));
    bbuf body;
    bbuf_init(&body);
    ben_encode(d, &body);
    ben_free(d);

    CHECK(http_srv_start(&s, "200 OK", body.data, body.len) == 0);
    http_announce_to(&s, "/announce?passkey=xyz", &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK);
    if (r.status != TR_OK)
        fprintf(stderr, "  erreur : %s\n", r.error);
    CHECK(r.transport == TR_HTTP && r.http_code == 200);
    CHECK(r.interval == 1800 && r.min_interval == 900 && r.complete == 3 && r.incomplete == 4);
    CHECK(r.npeers == 3);
    if (r.npeers == 3) {
        CHECK_STR(r.peers[0].ip, "127.0.0.1");
        CHECK(r.peers[1].port == 6882);
        CHECK_STR(r.peers[2].ip, "::1");
    }
    CHECK(strncmp(s.request, "GET /announce?passkey=xyz&info_hash=%A0%A1", 42) == 0);
    CHECK(strstr(s.request, "&event=started") != NULL);
    CHECK(strstr(s.request, "&compact=1") != NULL);
    CHECK(strstr(s.request, "&left=12345") != NULL);
    CHECK_STR(st.trackerid, "abc");
    CHECK(r.raw_len == body.len);
    char *raw = tr_format_raw(&r);
    CHECK(strstr(raw, "Bencode décodé") && strstr(raw, "\"tracker id\""));
    free(raw);
    tr_result_free(&r);

    /* 2. The tracker id is sent back on the next announce. */
    CHECK(http_srv_start(&s, "200 OK", body.data, body.len) == 0);
    p.trackerid = st.trackerid;
    p.event = TR_EVENT_NONE;
    http_announce_to(&s, "/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK);
    CHECK(strstr(s.request, "&trackerid=abc") != NULL);
    CHECK(strstr(s.request, "event=") == NULL);
    tr_result_free(&r);
    p.trackerid = NULL;
    p.event = TR_EVENT_STARTED;
    bbuf_free(&body);

    /* 3. failure reason. */
    const char *fail = "d14:failure reason12:unregisterede";
    CHECK(http_srv_start(&s, "200 OK", fail, strlen(fail)) == 0);
    http_announce_to(&s, "/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_TRACKER);
    CHECK_STR(r.failure, "unregistered");
    tr_result_free(&r);

    /* 4. Non-bencoded response. */
    const char *html = "<html>oops</html>";
    CHECK(http_srv_start(&s, "200 OK", html, strlen(html)) == 0);
    http_announce_to(&s, "/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_PROTOCOL);
    CHECK(strstr(r.error, "non bencodée") != NULL);
    tr_result_free(&r);

    /* 5. HTTP error with a bencoded failure reason. */
    CHECK(http_srv_start(&s, "404 Not Found", fail, strlen(fail)) == 0);
    http_announce_to(&s, "/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_TRACKER && r.http_code == 404);
    tr_result_free(&r);

    /* 6. Connection refused (nothing listening). */
    uint16_t closed;
    SOCKET tmp = loopback_socket(SOCK_STREAM, &closed);
    closesocket(tmp);
    char url[128];
    snprintf(url, sizeof url, "http://127.0.0.1:%u/announce", closed);
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_NET);
    tr_result_free(&r);

    /* 7. Invalid URL and DNS failure. */
    tr_announce("ftp://t.test/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_URL);
    tr_result_free(&r);
    tr_announce("http://nonexistent.invalid/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_DNS);
    if (r.status != TR_ERR_DNS)
        fprintf(stderr, "  DNS : statut %s, %s\n", tr_status_name(r.status), r.error);
    tr_result_free(&r);

    tr_state_free(&st);
    bbuf_free(&log);
}

/* ---- fake UDP tracker ---- */

#define SRV_CID 0xC0FFEE0012345678ULL

typedef struct {
    SOCKET fd;
    uint16_t port;
    HANDLE th;
    volatile long stop;
    int drop_first_connect, bad_tid_first, error_on_announce, silent;
    volatile long n_connect, n_announce, bad_cid;
    uint8_t last_announce[98];
} udp_srv;

static DWORD WINAPI udp_srv_main(LPVOID arg)
{
    udp_srv *s = arg;
    uint8_t buf[2048];
    while (!counter(&s->stop)) {
        fd_set rs;
        FD_ZERO(&rs);
        FD_SET(s->fd, &rs);
        struct timeval tv = { 0, 50000 };
        if (select(0, &rs, NULL, NULL, &tv) <= 0)
            continue;
        struct sockaddr_storage from;
        int fl = sizeof from;
        int r = recvfrom(s->fd, (char *)buf, (int)sizeof buf, 0, (struct sockaddr *)&from, &fl);
        if (r == SOCKET_ERROR)
            continue;
        uint8_t out[64];
        int ol;
        if (r == 16 && be_get64(buf) == UDP_PROTOCOL_ID && be_get32(buf + 8) == UDP_ACTION_CONNECT) {
            long n = InterlockedIncrement((volatile LONG *)&s->n_connect);
            if (s->silent || (s->drop_first_connect && n == 1))
                continue;
            be_put32(out, UDP_ACTION_CONNECT);
            memcpy(out + 4, buf + 12, 4);
            be_put64(out + 8, SRV_CID);
            ol = 16;
        } else if (r == 98 && be_get32(buf + 8) == UDP_ACTION_ANNOUNCE) {
            InterlockedIncrement((volatile LONG *)&s->n_announce);
            memcpy(s->last_announce, buf, 98);
            if (be_get64(buf) != SRV_CID)
                InterlockedIncrement((volatile LONG *)&s->bad_cid);
            if (s->silent)
                continue;
            if (s->bad_tid_first) {
                uint8_t bad[20] = { 0 };
                be_put32(bad, UDP_ACTION_ANNOUNCE);
                be_put32(bad + 4, be_get32(buf + 12) ^ 0xFFFFFFFFu);
                sendto(s->fd, (const char *)bad, (int)sizeof bad, 0, (struct sockaddr *)&from, fl);
            }
            if (s->error_on_announce) {
                be_put32(out, UDP_ACTION_ERROR);
                memcpy(out + 4, buf + 12, 4);
                memcpy(out + 8, "torrent inconnu", 15);
                ol = 23;
            } else {
                be_put32(out, UDP_ACTION_ANNOUNCE);
                memcpy(out + 4, buf + 12, 4);
                be_put32(out + 8, 1800);
                be_put32(out + 12, 2);
                be_put32(out + 16, 7);
                const uint8_t peers[12] = { 127, 0, 0, 1, 0x1A, 0xE1, 10, 1, 2, 3, 0x1A, 0xE2 };
                memcpy(out + 20, peers, 12);
                ol = 32;
            }
        } else {
            continue;
        }
        sendto(s->fd, (const char *)out, ol, 0, (struct sockaddr *)&from, fl);
    }
    return 0;
}

static int udp_srv_start(udp_srv *s)
{
    s->fd = loopback_socket(SOCK_DGRAM, &s->port);
    if (s->fd == INVALID_SOCKET)
        return -1;
    s->th = CreateThread(NULL, 0, udp_srv_main, s, 0, NULL);
    return s->th ? 0 : -1;
}

static void udp_srv_stop(udp_srv *s)
{
    InterlockedExchange((volatile LONG *)&s->stop, 1);
    WaitForSingleObject(s->th, INFINITE);
    CloseHandle(s->th);
    closesocket(s->fd);
}

static void udp_url(char *out, size_t n, uint16_t port)
{
    snprintf(out, n, "udp://127.0.0.1:%u/announce", port);
}

typedef struct {
    volatile long *flag;
    DWORD delay_ms;
} canceller;

static DWORD WINAPI cancel_later(LPVOID arg)
{
    canceller *c = arg;
    Sleep(c->delay_ms);
    tr_cancel_set(c->flag, 1);
    return 0;
}

static void test_udp(void)
{
    bbuf log;
    bbuf_init(&log);
    volatile long cancel = 0;
    tr_ctx ctx;
    make_ctx(&ctx, &log, &cancel);
    tr_params p;
    make_params(&p);
    tr_state st;
    memset(&st, 0, sizeof st);
    tr_result r;
    char url[128];

    /* 1. connect + announce, with a stray packet carrying a wrong transaction_id. */
    udp_srv s;
    memset(&s, 0, sizeof s);
    s.bad_tid_first = 1;
    CHECK(udp_srv_start(&s) == 0);
    udp_url(url, sizeof url, s.port);
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK);
    if (r.status != TR_OK)
        fprintf(stderr, "  erreur : %s\n", r.error);
    CHECK(r.transport == TR_UDP && r.udp_action == 1);
    CHECK(r.interval == 1800 && r.incomplete == 2 && r.complete == 7);
    CHECK(r.npeers == 2);
    if (r.npeers == 2) {
        CHECK_STR(r.peers[0].ip, "127.0.0.1");
        CHECK(r.peers[1].port == 6882);
    }
    CHECK(r.has_connect_pkt && r.has_announce_pkt && !r.conn_reused);
    CHECK(r.connect_resp_len == 16 && r.connection_id == SRV_CID);
    CHECK(counter(&s.n_connect) == 1 && counter(&s.bad_cid) == 0);
    CHECK(strstr(log_text(&log), "paquet ignoré") != NULL);
    CHECK(memcmp(s.last_announce + 16, p.info_hash, 20) == 0);
    CHECK(be_get32(s.last_announce + 80) == TR_EVENT_STARTED);
    CHECK(be_get16(s.last_announce + 96) == 6881);
    CHECK(strstr(r.resolved, "127.0.0.1") != NULL && !r.ipv6);
    char *req = tr_format_request(&r);
    CHECK(strstr(req, "Paquet connect envoyé") && strstr(req, "protocol_id") && strstr(req, "num_want"));
    free(req);
    char *raw = tr_format_raw(&r);
    CHECK(strstr(raw, "Réponse connect") && strstr(raw, "peer[1]"));
    free(raw);
    tr_result_free(&r);

    /* 2. The connection_id is reused while it is valid. */
    s.bad_tid_first = 0;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK && r.conn_reused && !r.has_connect_pkt);
    CHECK(counter(&s.n_connect) == 1 && counter(&s.n_announce) == 2);
    tr_result_free(&r);

    /* 3. An expired connection_id is renewed. */
    st.conn_obtained_ms -= UDP_CONN_ID_LIFETIME_MS + 1000;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK && !r.conn_reused && r.has_connect_pkt);
    CHECK(counter(&s.n_connect) == 2);
    CHECK(strstr(log_text(&log), "expiré") != NULL);
    tr_result_free(&r);

    /* 4. Error response (action 3). */
    s.error_on_announce = 1;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_TRACKER && r.udp_action == 3);
    CHECK_STR(r.failure, "torrent inconnu");
    tr_result_free(&r);
    udp_srv_stop(&s);

    /* 5. Retransmission when the first connect is lost. */
    tr_state_free(&st);
    memset(&s, 0, sizeof s);
    s.drop_first_connect = 1;
    CHECK(udp_srv_start(&s) == 0);
    udp_url(url, sizeof url, s.port);
    ctx.udp_base_timeout_ms = 100;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_OK);
    CHECK(r.retransmissions == 1);
    CHECK(counter(&s.n_connect) == 2);
    CHECK(strstr(log_text(&log), "retransmission n°1") != NULL);
    tr_result_free(&r);
    udp_srv_stop(&s);

    /* 6. Timeout after the configured retransmissions (20 + 40 + 80 ms). */
    tr_state_free(&st);
    memset(&s, 0, sizeof s);
    s.silent = 1;
    CHECK(udp_srv_start(&s) == 0);
    udp_url(url, sizeof url, s.port);
    ctx.udp_base_timeout_ms = 20;
    ctx.udp_max_retries = 2;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_TIMEOUT);
    CHECK(r.retransmissions == 2);
    CHECK(r.elapsed_ms >= 130 && r.elapsed_ms < 2000);
    CHECK(counter(&s.n_connect) == 3);
    tr_result_free(&r);

    /* 7. Cancel during a long wait. */
    ctx.udp_base_timeout_ms = 15000;
    ctx.udp_max_retries = 8;
    canceller c = { &cancel, 150 };
    HANDLE ct = CreateThread(NULL, 0, cancel_later, &c, 0, NULL);
    tr_announce(url, &p, &st, &ctx, &r);
    WaitForSingleObject(ct, INFINITE);
    CloseHandle(ct);
    CHECK(r.status == TR_ERR_CANCELLED);
    CHECK(r.elapsed_ms < 2000);
    tr_result_free(&r);
    tr_cancel_set(&cancel, 0);
    udp_srv_stop(&s);

    /* 8. Closed port: ICMP port unreachable. */
    uint16_t closed;
    SOCKET tmp = loopback_socket(SOCK_DGRAM, &closed);
    closesocket(tmp);
    udp_url(url, sizeof url, closed);
    ctx.udp_base_timeout_ms = 500;
    tr_announce(url, &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_NET);
    CHECK(strstr(r.error, "fermé") != NULL);
    tr_result_free(&r);

    /* 9. DNS failure and missing port. */
    tr_announce("udp://nonexistent.invalid:6969/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_DNS);
    tr_result_free(&r);
    tr_announce("udp://127.0.0.1/announce", &p, &st, &ctx, &r);
    CHECK(r.status == TR_ERR_URL);
    tr_result_free(&r);

    tr_state_free(&st);
    bbuf_free(&log);
}

/* ---- fake BitTorrent peer (one TCP connection) ---- */

enum { PEER_SEEDER, PEER_HAVE_ALL, PEER_WRONG_HASH, PEER_SILENT };

typedef struct {
    SOCKET lfd;
    uint16_t port;
    HANDLE th;
    int mode;
    uint8_t info_hash[20];
    int got_handshake;
} peer_srv;

static int recv_n(SOCKET s, uint8_t *b, int n)
{
    int got = 0;
    while (got < n) {
        int r = recv(s, (char *)b + got, n - got, 0);
        if (r <= 0)
            return -1;
        got += r;
    }
    return 0;
}

static DWORD WINAPI peer_srv_main(LPVOID arg)
{
    peer_srv *s = arg;
    SOCKET c = accept(s->lfd, NULL, NULL);
    if (c == INVALID_SOCKET)
        return 0;
    uint8_t hs[68];
    if (recv_n(c, hs, 68) == 0) {
        s->got_handshake = hs[0] == 19 && memcmp(hs + 1, "BitTorrent protocol", 19) == 0;
        if (s->mode != PEER_SILENT) {
            uint8_t ih[20];
            memcpy(ih, s->info_hash, 20);
            if (s->mode == PEER_WRONG_HASH)
                ih[0] ^= 0xFF;
            uint8_t reply[68];
            probe_build_handshake(reply, ih, (const uint8_t *)"-TR4050-fakepeer0001");
            send(c, (const char *)reply, 68, 0);
            if (s->mode == PEER_SEEDER) {
                static const char ext[] = "d1:v12:FakePeer 1.0e";
                uint8_t msg[64];
                be_put32(msg, (uint32_t)(sizeof ext - 1 + 2));
                msg[4] = 20;
                msg[5] = 0;
                memcpy(msg + 6, ext, sizeof ext - 1);
                send(c, (const char *)msg, (int)(sizeof ext - 1 + 6), 0);
                const uint8_t bitfield[] = { 0, 0, 0, 3, 5, 0xFF, 0xC0 }; /* 10 pieces, all set */
                send(c, (const char *)bitfield, sizeof bitfield, 0);
            } else if (s->mode == PEER_HAVE_ALL) {
                const uint8_t have_all[] = { 0, 0, 0, 1, 0x0E };
                send(c, (const char *)have_all, sizeof have_all, 0);
            }
        }
        /* Wait for the client to hang up. */
        uint8_t drain[256];
        while (recv(c, (char *)drain, sizeof drain, 0) > 0) {
        }
    }
    closesocket(c);
    return 0;
}

static int peer_srv_start(peer_srv *s, int mode, const uint8_t ih[20])
{
    memset(s, 0, sizeof *s);
    s->mode = mode;
    memcpy(s->info_hash, ih, 20);
    s->lfd = loopback_socket(SOCK_STREAM, &s->port);
    if (s->lfd == INVALID_SOCKET || listen(s->lfd, 4) == SOCKET_ERROR)
        return -1;
    s->th = CreateThread(NULL, 0, peer_srv_main, s, 0, NULL);
    return s->th ? 0 : -1;
}

static void peer_srv_stop(peer_srv *s)
{
    closesocket(s->lfd);
    WaitForSingleObject(s->th, INFINITE);
    CloseHandle(s->th);
}

static void test_probe(void)
{
    uint8_t ih[20], pid[20];
    for (int i = 0; i < 20; i++)
        ih[i] = (uint8_t)(0x40 + i);
    memcpy(pid, "-DB0001-PROBEPROBE01", 20);
    probe_result r;
    peer_srv s;

    /* Seeder with extension handshake and full bitfield. */
    CHECK(peer_srv_start(&s, PEER_SEEDER, ih) == 0);
    probe_peer("127.0.0.1", s.port, ih, pid, 10, 3000, NULL, &r);
    peer_srv_stop(&s);
    CHECK(s.got_handshake);
    CHECK(r.status == PROBE_OK);
    if (r.status != PROBE_OK)
        fprintf(stderr, "  sondage : %s\n", r.error);
    CHECK(r.ext_proto && r.fast_ext);
    CHECK(r.pieces_have == 10 && probe_is_seeder(&r, 10));
    CHECK_STR(r.client, "FakePeer 1.0");
    CHECK(r.has_peer_id && memcmp(r.peer_id, "-TR4050-", 8) == 0);

    /* have_all (fast extension). */
    CHECK(peer_srv_start(&s, PEER_HAVE_ALL, ih) == 0);
    probe_peer("127.0.0.1", s.port, ih, pid, 0, 3000, NULL, &r);
    peer_srv_stop(&s);
    CHECK(r.status == PROBE_OK && r.have_all && probe_is_seeder(&r, 0));

    /* Peer answering with another info_hash. */
    CHECK(peer_srv_start(&s, PEER_WRONG_HASH, ih) == 0);
    probe_peer("127.0.0.1", s.port, ih, pid, 10, 3000, NULL, &r);
    peer_srv_stop(&s);
    CHECK(r.status == PROBE_WRONG_HASH);

    /* Peer that accepts the connection but never answers. */
    CHECK(peer_srv_start(&s, PEER_SILENT, ih) == 0);
    int64_t t0 = mono_ms();
    probe_peer("127.0.0.1", s.port, ih, pid, 10, 300, NULL, &r);
    int64_t took = mono_ms() - t0;
    peer_srv_stop(&s);
    CHECK(r.status == PROBE_TIMEOUT);
    CHECK(took >= 250 && took < 2000);

    /* Nothing listening. */
    uint16_t closed;
    SOCKET tmp = loopback_socket(SOCK_STREAM, &closed);
    closesocket(tmp);
    probe_peer("127.0.0.1", closed, ih, pid, 10, 3000, NULL, &r);
    CHECK(r.status == PROBE_CONNECT_FAIL || r.status == PROBE_TIMEOUT);

    /* Cancel flag already set. */
    volatile long cancel = 1;
    CHECK(peer_srv_start(&s, PEER_SILENT, ih) == 0);
    probe_peer("127.0.0.1", s.port, ih, pid, 10, 3000, &cancel, &r);
    peer_srv_stop(&s);
    CHECK(r.status == PROBE_CANCELLED);
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    tr_global_init();
    test_http();
    test_udp();
    test_probe();
    tr_global_cleanup();
#ifdef _DEBUG
    if (_CrtDumpMemoryLeaks()) {
        fprintf(stderr, "fuites mémoire détectées\n");
        failures++;
    }
#endif
    printf("%d vérifications, %d échec(s)\n", checks, failures);
    return failures ? 1 : 0;
}
