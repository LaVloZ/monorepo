#ifndef TRACKER_H
#define TRACKER_H

/* Common interface for HTTP and UDP trackers: the UI only uses this header. */

#include <stddef.h>
#include <stdint.h>

#include "util.h"

#define TR_PEER_ID_PREFIX "-DB0001-"

typedef enum { TR_HTTP, TR_UDP } tr_transport;

/* Values match the UDP (BEP 15) encoding. */
typedef enum {
    TR_EVENT_NONE = 0,
    TR_EVENT_COMPLETED = 1,
    TR_EVENT_STARTED = 2,
    TR_EVENT_STOPPED = 3,
} tr_event;

typedef enum {
    TR_OK,
    TR_ERR_URL,
    TR_ERR_DNS,
    TR_ERR_NET,
    TR_ERR_TIMEOUT,
    TR_ERR_CANCELLED,
    TR_ERR_PROTOCOL, /* malformed / unexpected response */
    TR_ERR_TRACKER,  /* failure reason (HTTP) or action 3 (UDP) */
} tr_status;

typedef struct {
    tr_transport transport;
    int tls;
    char host[256];
    uint16_t port;
} tr_url;

typedef struct {
    uint8_t info_hash[20];
    uint8_t peer_id[20];
    uint16_t port;
    uint64_t uploaded, downloaded, left;
    int32_t numwant;      /* -1 = tracker default */
    int compact;          /* HTTP only */
    uint32_t key;
    tr_event event;
    int tls_no_verify;    /* HTTPS only */
    const char *trackerid; /* HTTP only, may be NULL (not owned) */
} tr_params;

typedef struct {
    char ip[64];
    uint16_t port;
    uint8_t peer_id[20];
    size_t peer_id_len; /* 0 = not provided */
} tr_peer;

/* Per-tracker state kept between announces. */
typedef struct {
    int conn_valid;
    uint64_t connection_id;
    int64_t conn_obtained_ms;
    char conn_endpoint[80];
    char *trackerid;
} tr_state;

enum { TR_LOG_INFO, TR_LOG_WARN, TR_LOG_ERROR };
typedef void (*tr_log_fn)(void *ud, int level, const char *msg);

typedef struct {
    tr_log_fn log;
    void *log_ud;
    volatile long *cancel;   /* non-zero = abort as soon as possible (set with tr_cancel_set) */
    int udp_base_timeout_ms; /* BEP 15: 15 s */
    int udp_max_retries;     /* BEP 15: 8 */
    int http_timeout_ms;
} tr_ctx;

typedef struct {
    tr_status status;
    tr_transport transport;
    char error[512];
    char *url;
    tr_params params; /* copy of what was sent (trackerid not kept) */
    double elapsed_ms;

    /* HTTP */
    char *request_url;
    char *http_headers;
    long http_code;

    /* UDP */
    char resolved[400];
    int ipv6;
    int conn_reused;
    uint64_t connection_id;
    int has_connect_pkt, has_announce_pkt;
    uint8_t connect_pkt[16];
    uint8_t announce_pkt[98];
    uint8_t *connect_resp;
    size_t connect_resp_len;
    int udp_action; /* -1 = none */
    int retransmissions;

    /* Response */
    uint8_t *raw;
    size_t raw_len;
    int has_interval, has_min_interval, has_complete, has_incomplete;
    int64_t interval, min_interval, complete, incomplete;
    char *tracker_id;
    char *warning;
    char *failure;
    /* Common HTTP extensions */
    int has_downloaded;
    int64_t downloaded;     /* "downloaded": completed downloads */
    char external_ip[64];   /* BEP 24 "external ip", decoded ("" if absent) */
    char *retry_in;         /* BEP 31 "retry in": "30 minute(s)" or "never" */
    int has_crypto;         /* "crypto_flags" present */
    size_t crypto_total, crypto_on;
    char *other_keys;       /* non-standard keys with a short preview, one per line */
    tr_peer *peers;
    size_t npeers;
    size_t peers_trailing; /* bytes left over in compact peer strings */
} tr_result;

void tr_global_init(void);
void tr_global_cleanup(void);

void tr_ctx_defaults(tr_ctx *ctx);
int tr_parse_url(const char *url, tr_url *out, char *err, size_t errlen);

/* Blocking announce. Run it in a worker thread. st is updated (connection_id, tracker id). */
void tr_announce(const char *url, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res);

void tr_result_init(tr_result *r);
void tr_result_free(tr_result *r);
void tr_state_free(tr_state *st);

/* Text for the "Requête" and "Raw" panes (malloc'd). */
char *tr_format_request(const tr_result *r);
char *tr_format_raw(const tr_result *r);

void tr_generate_peer_id(uint8_t out[20]);
void tr_peer_id_str(const uint8_t *id, size_t len, char *out, size_t outlen);
const char *tr_event_name(tr_event e);
const char *tr_status_name(tr_status s);
const char *tr_transport_name(tr_transport t);

/* Internal helpers shared by the transports (also used by tests). */
void tr_log(const tr_ctx *ctx, int level, const char *fmt, ...) PRINTF_FMT(3, 4);
int tr_cancelled(const tr_ctx *ctx);
/* Thread-safe write of a cancel flag. */
void tr_cancel_set(volatile long *flag, long value);
int tr_result_add_peer(tr_result *r, const tr_peer *p);
/* Parses compact peers (6 bytes IPv4 or 18 bytes IPv6). Returns leftover bytes. */
size_t tr_parse_compact_peers(const uint8_t *data, size_t len, int ipv6, tr_result *r);

#endif
