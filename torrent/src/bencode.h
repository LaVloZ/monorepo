#ifndef BENCODE_H
#define BENCODE_H

#include <stddef.h>
#include <stdint.h>

#include "util.h"

typedef enum { BEN_INT, BEN_STR, BEN_LIST, BEN_DICT } ben_type;

typedef struct ben_node ben_node;

typedef struct {
    char *key; /* NUL-terminated copy (may contain NUL bytes: use key_len) */
    size_t key_len;
    ben_node *val;
} ben_kv;

struct ben_node {
    ben_type type;
    /* Span of this value in the decoded buffer: [raw_start, raw_end). */
    size_t raw_start, raw_end;
    int64_t i;            /* BEN_INT */
    unsigned char *s;     /* BEN_STR, NUL-terminated copy */
    size_t slen;
    ben_node **items;     /* BEN_LIST */
    size_t count;
    ben_kv *kv;           /* BEN_DICT, in file order */
    size_t nkv;
};

/* Decodes exactly one value covering the whole buffer. NULL on error (err set). */
ben_node *ben_decode(const void *buf, size_t len, char *err, size_t errlen);
void ben_free(ben_node *n);

/* Encodes n; dictionary keys are emitted sorted (canonical form). 0 = ok. */
int ben_encode(const ben_node *n, bbuf *out);

/* Builders (return NULL on allocation failure). add/put take ownership of v. */
ben_node *ben_new_int(int64_t v);
ben_node *ben_new_str(const void *s, size_t len);
ben_node *ben_new_strz(const char *s);
ben_node *ben_new_list(void);
ben_node *ben_new_dict(void);
int ben_list_add(ben_node *list, ben_node *v);
int ben_dict_put(ben_node *dict, const char *key, ben_node *v);

/* Lookups (NULL / 0 when missing or of the wrong type). */
ben_node *ben_dict_get(const ben_node *dict, const char *key);
int ben_get_int(const ben_node *dict, const char *key, int64_t *out);
const unsigned char *ben_get_str(const ben_node *dict, const char *key, size_t *len);

/* Human-readable dump (binary strings shown as hex). */
void ben_pretty(const ben_node *n, bbuf *out, int indent);

#endif
