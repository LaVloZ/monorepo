#ifndef HTTP_TRACKER_H
#define HTTP_TRACKER_H

#include "tracker.h"

/* Builds the final announce URL (malloc'd). Appends with '&' if base has a query. */
char *http_build_url(const char *base, const tr_params *p);
/* Decodes a bencoded announce response into r. -1 if not a bencoded dict (r->error set). */
int http_parse_response(const uint8_t *body, size_t len, tr_result *r);
void http_announce(const char *url, const tr_params *p, tr_state *st, const tr_ctx *ctx, tr_result *res);

#endif
