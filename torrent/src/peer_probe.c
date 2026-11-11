#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "peer_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bencode.h"
#include "util.h"

#define PSTR "BitTorrent protocol"
#define MAX_MESSAGE (4 * 1024 * 1024)
#define MSG_HAVE 4
#define MSG_BITFIELD 5
#define MSG_HAVE_ALL 0x0E
#define MSG_HAVE_NONE 0x0F
#define MSG_EXTENDED 20

/* ---- handshake ---- */

void probe_build_handshake(uint8_t out[PROBE_HANDSHAKE_LEN], const uint8_t info_hash[20], const uint8_t peer_id[20])
{
    out[0] = 19;
    memcpy(out + 1, PSTR, 19);
    memset(out + 20, 0, 8);
    out[20 + 5] |= 0x10; /* BEP 10 extension protocol */
    out[20 + 7] |= 0x04; /* BEP 6 fast extension */
    memcpy(out + 28, info_hash, 20);
    memcpy(out + 48, peer_id, 20);
}

int probe_parse_handshake(const uint8_t in[PROBE_HANDSHAKE_LEN], const uint8_t info_hash[20], probe_result *r)
{
    if (in[0] != 19 || memcmp(in + 1, PSTR, 19) != 0)
        return -1;
    const uint8_t *res = in + 20;
    r->ext_proto = (res[5] & 0x10) != 0;
    r->fast_ext = (res[7] & 0x04) != 0;
    r->dht = (res[7] & 0x01) != 0;
    memcpy(r->peer_id, in + 48, 20);
    r->has_peer_id = 1;
    return memcmp(in + 28, info_hash, 20) == 0 ? 0 : -2;
}

/* ---- messages ---- */

void probe_ctx_init(probe_ctx *c, probe_result *r, size_t num_pieces)
{
    memset(c, 0, sizeof *c);
    c->r = r;
    c->num_pieces = num_pieces;
}

void probe_ctx_free(probe_ctx *c)
{
    free(c->bits);
    c->bits = NULL;
    c->bits_len = 0;
}

static void recount(probe_ctx *c)
{
    size_t limit = c->num_pieces ? c->num_pieces : c->bits_len * 8;
    size_t n = 0;
    for (size_t i = 0; i < limit && i / 8 < c->bits_len; i++)
        if (c->bits[i / 8] & (0x80 >> (i % 8)))
            n++;
    c->r->pieces_have = n;
    c->r->bitfield_bits = c->bits_len * 8;
}

static void on_extended_handshake(probe_ctx *c, const uint8_t *p, size_t n)
{
    ben_node *d = ben_decode(p, n, NULL, 0);
    if (!d)
        return;
    c->got_ext_handshake = 1;
    size_t len;
    const unsigned char *v = ben_get_str(d, "v", &len);
    if (v && is_printable_text(v, len))
        snprintf(c->r->client, sizeof c->r->client, "%.*s", (int)len, (const char *)v);
    int64_t uo;
    if (ben_get_int(d, "upload_only", &uo))
        c->r->upload_only = uo != 0;
    const unsigned char *ip = ben_get_str(d, "yourip", &len);
    if (ip && (len == 4 || len == 16))
        inet_ntop(len == 4 ? AF_INET : AF_INET6, ip, c->r->yourip, sizeof c->r->yourip);
    ben_free(d);
}

void probe_on_message(probe_ctx *c, const uint8_t *msg, size_t len)
{
    if (len == 0)
        return; /* keep-alive */
    const uint8_t *p = msg + 1;
    size_t n = len - 1;
    switch (msg[0]) {
    case MSG_BITFIELD: {
        uint8_t *bits = malloc(n ? n : 1);
        if (!bits)
            return;
        memcpy(bits, p, n);
        free(c->bits);
        c->bits = bits;
        c->bits_len = n;
        c->r->got_pieces = 1;
        recount(c);
        break;
    }
    case MSG_HAVE: {
        if (n < 4)
            return;
        uint32_t idx = be_get32(p);
        if (c->num_pieces && idx >= c->num_pieces)
            return;
        size_t need = (size_t)idx / 8 + 1;
        if (need > MAX_MESSAGE)
            return;
        if (need > c->bits_len) {
            uint8_t *bits = realloc(c->bits, need);
            if (!bits)
                return;
            memset(bits + c->bits_len, 0, need - c->bits_len);
            c->bits = bits;
            c->bits_len = need;
        }
        c->bits[idx / 8] |= (uint8_t)(0x80 >> (idx % 8));
        c->r->got_pieces = 1;
        recount(c);
        break;
    }
    case MSG_HAVE_ALL:
        c->r->have_all = 1;
        c->r->got_pieces = 1;
        break;
    case MSG_HAVE_NONE:
        c->r->have_none = 1;
        c->r->got_pieces = 1;
        break;
    case MSG_EXTENDED:
        if (n >= 1 && p[0] == 0)
            on_extended_handshake(c, p + 1, n - 1);
        break;
    default:
        break;
    }
}

/* ---- presentation ---- */

int probe_is_seeder(const probe_result *r, size_t num_pieces)
{
    if (r->status != PROBE_OK)
        return 0;
    return r->have_all || (num_pieces && r->pieces_have >= num_pieces);
}

void probe_completion_text(const probe_result *r, size_t num_pieces, char *out, size_t n)
{
    if (r->status != PROBE_OK)
        snprintf(out, n, "—");
    else if (r->have_all)
        snprintf(out, n, "100 %% (seeder, have_all)");
    else if (r->have_none)
        snprintf(out, n, "0 %% (have_none)");
    else if (!r->got_pieces)
        snprintf(out, n, "rien annoncé (0 %% ?)");
    else if (num_pieces && r->pieces_have >= num_pieces)
        snprintf(out, n, "100 %% (seeder)");
    else if (num_pieces)
        snprintf(out, n, "%.1f %% (%zu/%zu)", (double)r->pieces_have * 100.0 / (double)num_pieces, r->pieces_have,
                 num_pieces);
    else
        snprintf(out, n, "%zu pièce(s), total inconnu", r->pieces_have);
}

void probe_extensions_text(const probe_result *r, char *out, size_t n)
{
    if (r->status != PROBE_OK && r->status != PROBE_WRONG_HASH) {
        snprintf(out, n, "—");
        return;
    }
    snprintf(out, n, "%s%s%s%s", r->fast_ext ? "fast " : "", r->ext_proto ? "ext " : "", r->dht ? "dht " : "",
             r->upload_only == 1 ? "upload_only" : "");
    size_t len = strlen(out);
    while (len && out[len - 1] == ' ')
        out[--len] = 0;
    if (!len)
        snprintf(out, n, "aucune");
}

static const struct {
    const char *code;
    const char *name;
} azureus_clients[] = {
    { "qB", "qBittorrent" }, { "TR", "Transmission" }, { "UT", "µTorrent" },    { "UM", "µTorrent Mac" },
    { "UW", "µTorrent Web" }, { "LT", "libtorrent" },  { "lt", "libTorrent" },  { "DE", "Deluge" },
    { "AZ", "Vuze" },         { "BI", "BiglyBT" },     { "KT", "KTorrent" },    { "BT", "BitTorrent" },
    { "TX", "Tixati" },       { "WW", "WebTorrent" },  { "FD", "Free Download Manager" },
    { "PI", "PicoTorrent" },  { "AG", "aria2" },       { "A2", "aria2" },       { "DB", "Announce Debugger" },
};

void probe_client_text(const probe_result *r, char *out, size_t n)
{
    if (r->client[0]) {
        snprintf(out, n, "%s", r->client);
        return;
    }
    if (r->has_peer_id && r->peer_id[0] == '-' && r->peer_id[7] == '-') {
        for (size_t i = 0; i < sizeof azureus_clients / sizeof azureus_clients[0]; i++) {
            if (memcmp(r->peer_id + 1, azureus_clients[i].code, 2) == 0) {
                snprintf(out, n, "%s (%.4s)", azureus_clients[i].name, (const char *)r->peer_id + 3);
                return;
            }
        }
        snprintf(out, n, "inconnu (%.2s %.4s)", (const char *)r->peer_id + 1, (const char *)r->peer_id + 3);
        return;
    }
    snprintf(out, n, "—");
}

const char *probe_status_name(probe_status s)
{
    switch (s) {
    case PROBE_PENDING: return "en attente";
    case PROBE_OK: return "joignable";
    case PROBE_CONNECT_FAIL: return "injoignable";
    case PROBE_TIMEOUT: return "timeout";
    case PROBE_HANDSHAKE_FAIL: return "handshake invalide";
    case PROBE_WRONG_HASH: return "n'a pas ce torrent";
    case PROBE_CANCELLED: return "annulé";
    }
    return "?";
}

/* ---- network ---- */

static int cancelled(volatile long *cancel)
{
    return cancel && InterlockedCompareExchange((volatile LONG *)cancel, 0, 0) != 0;
}

enum { IO_OK = 1, IO_CLOSED = 0, IO_TIMEOUT = -1, IO_CANCELLED = -2, IO_ERROR = -3 };

/* Waits until the socket is readable (write = 0) or writable (write = 1). */
static int wait_socket(SOCKET s, int write, int64_t deadline, volatile long *cancel)
{
    for (;;) {
        if (cancelled(cancel))
            return IO_CANCELLED;
        int64_t rem = deadline - mono_ms();
        if (rem <= 0)
            return IO_TIMEOUT;
        fd_set fs, es;
        FD_ZERO(&fs);
        FD_ZERO(&es);
        FD_SET(s, &fs);
        FD_SET(s, &es);
        int64_t slice = rem > 100 ? 100 : rem;
        struct timeval tv = { 0, (long)(slice * 1000) };
        int r = select(0, write ? NULL : &fs, write ? &fs : NULL, &es, &tv);
        if (r == SOCKET_ERROR)
            return IO_ERROR;
        if (r > 0)
            return FD_ISSET(s, &es) ? IO_ERROR : IO_OK;
    }
}

static int send_all(SOCKET s, const uint8_t *buf, size_t n, int64_t deadline, volatile long *cancel)
{
    while (n > 0) {
        int w = wait_socket(s, 1, deadline, cancel);
        if (w != IO_OK)
            return w;
        int k = send(s, (const char *)buf, (int)n, 0);
        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            return IO_ERROR;
        }
        buf += k;
        n -= (size_t)k;
    }
    return IO_OK;
}

static int recv_exact(SOCKET s, uint8_t *buf, size_t n, int64_t deadline, volatile long *cancel)
{
    while (n > 0) {
        int w = wait_socket(s, 0, deadline, cancel);
        if (w != IO_OK)
            return w;
        int k = recv(s, (char *)buf, (int)(n > 65536 ? 65536 : n), 0);
        if (k == 0)
            return IO_CLOSED;
        if (k == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                continue;
            return IO_ERROR;
        }
        buf += k;
        n -= (size_t)k;
    }
    return IO_OK;
}

static void set_error(probe_result *r, probe_status st, const char *what, int code)
{
    r->status = st;
    if (code) {
        char msg[128];
        win_error_text((unsigned long)code, NULL, msg, sizeof msg);
        snprintf(r->error, sizeof r->error, "%s : %s", what, msg);
    } else {
        snprintf(r->error, sizeof r->error, "%s", what);
    }
}

static int io_status(int io, probe_result *r, const char *what)
{
    if (io == IO_CANCELLED)
        set_error(r, PROBE_CANCELLED, "annulé", 0);
    else if (io == IO_TIMEOUT)
        set_error(r, PROBE_TIMEOUT, what, 0);
    else if (io == IO_CLOSED)
        set_error(r, PROBE_HANDSHAKE_FAIL, "connexion fermée par le peer pendant le handshake", 0);
    else
        set_error(r, PROBE_HANDSHAKE_FAIL, what, WSAGetLastError());
    return -1;
}

static SOCKET connect_peer(const char *ip, uint16_t port, int64_t deadline, volatile long *cancel, probe_result *r)
{
    char portstr[8];
    snprintf(portstr, sizeof portstr, "%u", port);
    struct addrinfo hints, *ai = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int g = getaddrinfo(ip, portstr, &hints, &ai);
    if (g != 0) {
        set_error(r, PROBE_CONNECT_FAIL, "adresse invalide", g);
        return INVALID_SOCKET;
    }
    SOCKET s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (s == INVALID_SOCKET) {
        set_error(r, PROBE_CONNECT_FAIL, "socket", WSAGetLastError());
        freeaddrinfo(ai);
        return INVALID_SOCKET;
    }
    u_long nb = 1;
    ioctlsocket(s, FIONBIO, &nb);
    int64_t t0 = mono_ms();
    int rc = connect(s, ai->ai_addr, (int)ai->ai_addrlen);
    freeaddrinfo(ai);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        set_error(r, PROBE_CONNECT_FAIL, "connexion refusée", WSAGetLastError());
        closesocket(s);
        return INVALID_SOCKET;
    }
    int w = wait_socket(s, 1, deadline, cancel);
    if (w != IO_OK) {
        int err = 0, len = sizeof err;
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        if (w == IO_CANCELLED)
            set_error(r, PROBE_CANCELLED, "annulé", 0);
        else if (w == IO_TIMEOUT)
            set_error(r, PROBE_TIMEOUT, "pas de réponse à la connexion TCP", 0);
        else
            set_error(r, PROBE_CONNECT_FAIL, "connexion refusée", err);
        closesocket(s);
        return INVALID_SOCKET;
    }
    r->connect_ms = (double)(mono_ms() - t0);
    return s;
}

/* Our extension handshake: no extension offered, just the client name. */
static void send_ext_handshake(SOCKET s, int64_t deadline, volatile long *cancel)
{
    static const char payload[] = "d1:mde1:v21:Announce Debugger 0.1e";
    size_t plen = sizeof payload - 1;
    uint8_t msg[64];
    be_put32(msg, (uint32_t)(plen + 2));
    msg[4] = MSG_EXTENDED;
    msg[5] = 0;
    memcpy(msg + 6, payload, plen);
    send_all(s, msg, plen + 6, deadline, cancel);
}

void probe_peer(const char *ip, uint16_t port, const uint8_t info_hash[20], const uint8_t peer_id[20],
                size_t num_pieces, int timeout_ms, volatile long *cancel, probe_result *r)
{
    memset(r, 0, sizeof *r);
    r->upload_only = -1;
    int64_t deadline = mono_ms() + timeout_ms;

    SOCKET s = connect_peer(ip, port, deadline, cancel, r);
    if (s == INVALID_SOCKET)
        return;

    uint8_t hs[PROBE_HANDSHAKE_LEN];
    probe_build_handshake(hs, info_hash, peer_id);
    int io = send_all(s, hs, sizeof hs, deadline, cancel);
    if (io != IO_OK) {
        io_status(io, r, "envoi du handshake");
        closesocket(s);
        return;
    }
    io = recv_exact(s, hs, sizeof hs, deadline, cancel);
    if (io != IO_OK) {
        io_status(io, r, "pas de handshake reçu");
        closesocket(s);
        return;
    }
    int ph = probe_parse_handshake(hs, info_hash, r);
    if (ph == -1) {
        set_error(r, PROBE_HANDSHAKE_FAIL, "réponse qui n'est pas un handshake BitTorrent", 0);
        closesocket(s);
        return;
    }
    if (ph == -2) {
        set_error(r, PROBE_WRONG_HASH, "le peer a répondu avec un autre info_hash", 0);
        closesocket(s);
        return;
    }
    r->status = PROBE_OK;
    if (r->ext_proto)
        send_ext_handshake(s, deadline, cancel);

    /* Read messages until we know what the peer has (and its client name), or the deadline. */
    probe_ctx c;
    probe_ctx_init(&c, r, num_pieces);
    uint8_t *buf = NULL;
    int grace = 0;
    for (;;) {
        if (r->got_pieces && (!r->ext_proto || c.got_ext_handshake))
            break;
        /* Pieces known: give the extension handshake (client name) one more second at most. */
        if (r->got_pieces && !grace) {
            grace = 1;
            if (mono_ms() + 1000 < deadline)
                deadline = mono_ms() + 1000;
        }
        uint8_t lenbuf[4];
        io = recv_exact(s, lenbuf, 4, deadline, cancel);
        if (io != IO_OK)
            break;
        uint32_t len = be_get32(lenbuf);
        if (len == 0)
            continue;
        if (len > MAX_MESSAGE) {
            snprintf(r->error, sizeof r->error, "message de %u octets refusé", len);
            break;
        }
        uint8_t *nb = realloc(buf, len);
        if (!nb)
            break;
        buf = nb;
        io = recv_exact(s, buf, len, deadline, cancel);
        if (io != IO_OK)
            break;
        probe_on_message(&c, buf, len);
    }
    if (io == IO_CANCELLED)
        set_error(r, PROBE_CANCELLED, "annulé", 0);
    else if (!r->got_pieces && !r->error[0])
        snprintf(r->error, sizeof r->error, "%s",
                 io == IO_CLOSED ? "connexion fermée sans annoncer de pièces" : "aucune pièce annoncée avant le timeout");
    free(buf);
    probe_ctx_free(&c);
    closesocket(s);
}
