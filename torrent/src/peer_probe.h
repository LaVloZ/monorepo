#ifndef PEER_PROBE_H
#define PEER_PROBE_H

/* Peer wire protocol probe (BEP 3, with BEP 6 fast extension and BEP 10 extension
 * handshake): connect to a peer, exchange handshakes and read what it announces
 * owning (bitfield / have / have_all / have_none). Nothing is downloaded. */

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PROBE_PENDING = 0,
    PROBE_OK,
    PROBE_CONNECT_FAIL,
    PROBE_TIMEOUT,
    PROBE_HANDSHAKE_FAIL,
    PROBE_WRONG_HASH,
    PROBE_CANCELLED,
} probe_status;

typedef struct {
    probe_status status;
    char error[160];
    double connect_ms;
    uint8_t peer_id[20];
    int has_peer_id;
    int fast_ext, ext_proto, dht; /* reserved bits of its handshake */
    int got_pieces;               /* bitfield / have / have_all / have_none received */
    int have_all, have_none;
    size_t pieces_have;           /* bits set (within num_pieces when known) */
    size_t bitfield_bits;         /* 8 x bitfield length, 0 if none */
    char client[64];              /* "v" of the extension handshake */
    int upload_only;              /* -1 unknown, 0 or 1 (BEP 21) */
    char yourip[64];              /* "yourip" of the extension handshake */
} probe_result;

/* State while reading messages (exposed for the tests). */
typedef struct {
    size_t num_pieces; /* 0 = unknown */
    uint8_t *bits;
    size_t bits_len;
    int got_ext_handshake;
    probe_result *r;
} probe_ctx;

#define PROBE_HANDSHAKE_LEN 68

void probe_build_handshake(uint8_t out[PROBE_HANDSHAKE_LEN], const uint8_t info_hash[20], const uint8_t peer_id[20]);
/* 0 = ok, -1 = not a BitTorrent handshake, -2 = other info_hash. Fills peer_id and flags. */
int probe_parse_handshake(const uint8_t in[PROBE_HANDSHAKE_LEN], const uint8_t info_hash[20], probe_result *r);

void probe_ctx_init(probe_ctx *c, probe_result *r, size_t num_pieces);
void probe_ctx_free(probe_ctx *c);
/* msg = message id followed by its payload (length prefix already removed). */
void probe_on_message(probe_ctx *c, const uint8_t *msg, size_t len);

int probe_is_seeder(const probe_result *r, size_t num_pieces);
void probe_completion_text(const probe_result *r, size_t num_pieces, char *out, size_t n);
void probe_extensions_text(const probe_result *r, char *out, size_t n);
/* Client name: extension handshake "v", else guessed from an Azureus-style peer_id. */
void probe_client_text(const probe_result *r, char *out, size_t n);
const char *probe_status_name(probe_status s);

/* Blocking probe of one peer over TCP. cancel may be NULL. */
void probe_peer(const char *ip, uint16_t port, const uint8_t info_hash[20], const uint8_t peer_id[20],
                size_t num_pieces, int timeout_ms, volatile long *cancel, probe_result *r);

#endif
