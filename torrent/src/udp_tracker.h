#ifndef UDP_TRACKER_H
#define UDP_TRACKER_H

#include "tracker.h"
#include "util.h"

#define UDP_PROTOCOL_ID 0x41727101980ULL
#define UDP_CONNECT_LEN 16
#define UDP_ANNOUNCE_LEN 98
#define UDP_RECV_BUF 2048
/* BEP 15: a client may use a connection ID until one minute after receiving it. */
#define UDP_CONN_ID_LIFETIME_MS 60000

enum {
    UDP_ACTION_CONNECT = 0,
    UDP_ACTION_ANNOUNCE = 1,
    UDP_ACTION_SCRAPE = 2,
    UDP_ACTION_ERROR = 3,
};

typedef enum {
    UDP_PARSE_OK,
    UDP_PARSE_BAD_TID,       /* not our transaction: ignore the packet */
    UDP_PARSE_SHORT,
    UDP_PARSE_BAD_ACTION,
    UDP_PARSE_TRACKER_ERROR, /* action 3, msg holds the text */
} udp_parse_rc;

void udp_build_connect(uint8_t out[UDP_CONNECT_LEN], uint32_t tid);
void udp_build_announce(uint8_t out[UDP_ANNOUNCE_LEN], uint64_t connection_id, uint32_t tid, const tr_params *p);

udp_parse_rc udp_parse_connect(const uint8_t *b, size_t len, uint32_t tid, uint64_t *connection_id, char *msg,
                               size_t msglen);
/* Fills interval / leechers / seeders / peers in r. */
udp_parse_rc udp_parse_announce(const uint8_t *b, size_t len, uint32_t tid, int ipv6, tr_result *r, char *msg,
                                size_t msglen);

/* Annotated field tables (offset, length, name, decoded value). */
void udp_annotate_connect_req(const uint8_t *p, bbuf *out);
void udp_annotate_announce_req(const uint8_t *p, bbuf *out);
void udp_annotate_response(const uint8_t *p, size_t len, int ipv6, bbuf *out);

void udp_announce(const tr_url *u, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res);

#endif
