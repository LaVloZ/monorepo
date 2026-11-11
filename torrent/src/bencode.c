#include "bencode.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BEN_MAX_DEPTH 64

typedef struct {
    const unsigned char *b;
    size_t len, pos;
    char *err;
    size_t errlen;
} dctx;

static void derr(dctx *c, const char *fmt, ...) PRINTF_FMT(2, 3);
static void derr(dctx *c, const char *fmt, ...)
{
    if (!c->err || !c->errlen || c->err[0])
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(c->err, c->errlen, fmt, ap);
    va_end(ap);
}

static ben_node *node_new(ben_type t)
{
    ben_node *n = calloc(1, sizeof *n);
    if (n)
        n->type = t;
    return n;
}

void ben_free(ben_node *n)
{
    if (!n)
        return;
    free(n->s);
    for (size_t i = 0; i < n->count; i++)
        ben_free(n->items[i]);
    free(n->items);
    for (size_t i = 0; i < n->nkv; i++) {
        free(n->kv[i].key);
        ben_free(n->kv[i].val);
    }
    free(n->kv);
    free(n);
}

static int is_digit(unsigned char c)
{
    return c >= '0' && c <= '9';
}

/* Reads "<len>:" and checks the string fits in the buffer. */
static int read_len(dctx *c, size_t *out)
{
    size_t start = c->pos;
    uint64_t v = 0;
    while (c->pos < c->len && is_digit(c->b[c->pos])) {
        if (v > (UINT64_MAX - 9) / 10) {
            derr(c, "longueur de chaîne trop grande à l'offset %zu", start);
            return -1;
        }
        v = v * 10 + (uint64_t)(c->b[c->pos] - '0');
        c->pos++;
    }
    if (c->pos == start) {
        derr(c, "longueur de chaîne attendue à l'offset %zu", start);
        return -1;
    }
    if (c->pos - start > 1 && c->b[start] == '0') {
        derr(c, "longueur avec zéro non significatif à l'offset %zu", start);
        return -1;
    }
    if (c->pos >= c->len || c->b[c->pos] != ':') {
        derr(c, "':' attendu à l'offset %zu", c->pos);
        return -1;
    }
    c->pos++;
    if (v > c->len - c->pos) {
        derr(c, "chaîne de %" PRIu64 " octets à l'offset %zu dépasse la fin des données", v, start);
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

static int list_push(ben_node *l, ben_node *v)
{
    ben_node **p = realloc(l->items, (l->count + 1) * sizeof *p);
    if (!p)
        return -1;
    l->items = p;
    l->items[l->count++] = v;
    return 0;
}

static int dict_push(ben_node *d, char *key, size_t klen, ben_node *v)
{
    ben_kv *p = realloc(d->kv, (d->nkv + 1) * sizeof *p);
    if (!p)
        return -1;
    d->kv = p;
    d->kv[d->nkv].key = key;
    d->kv[d->nkv].key_len = klen;
    d->kv[d->nkv].val = v;
    d->nkv++;
    return 0;
}

static ben_node *dec(dctx *c, int depth);

static ben_node *dec_int(dctx *c)
{
    size_t start = c->pos;
    c->pos++; /* 'i' */
    int neg = 0;
    if (c->pos < c->len && c->b[c->pos] == '-') {
        neg = 1;
        c->pos++;
    }
    size_t ds = c->pos;
    uint64_t v = 0;
    while (c->pos < c->len && is_digit(c->b[c->pos])) {
        if (v > (UINT64_MAX - 9) / 10) {
            derr(c, "entier trop grand à l'offset %zu", start);
            return NULL;
        }
        v = v * 10 + (uint64_t)(c->b[c->pos] - '0');
        c->pos++;
    }
    if (c->pos == ds) {
        derr(c, "entier vide à l'offset %zu", start);
        return NULL;
    }
    if (c->pos - ds > 1 && c->b[ds] == '0') {
        derr(c, "entier avec zéro non significatif à l'offset %zu", start);
        return NULL;
    }
    if (neg && v == 0) {
        derr(c, "entier -0 interdit à l'offset %zu", start);
        return NULL;
    }
    if (c->pos >= c->len || c->b[c->pos] != 'e') {
        derr(c, "'e' attendu à la fin de l'entier (offset %zu)", c->pos);
        return NULL;
    }
    c->pos++;
    if ((!neg && v > (uint64_t)INT64_MAX) || (neg && v > (uint64_t)INT64_MAX + 1)) {
        derr(c, "entier hors limites 64 bits à l'offset %zu", start);
        return NULL;
    }
    ben_node *n = node_new(BEN_INT);
    if (!n)
        return NULL;
    if (neg)
        n->i = (v == (uint64_t)INT64_MAX + 1) ? INT64_MIN : -(int64_t)v;
    else
        n->i = (int64_t)v;
    return n;
}

static ben_node *dec_str(dctx *c)
{
    size_t len;
    if (read_len(c, &len) < 0)
        return NULL;
    ben_node *n = node_new(BEN_STR);
    if (!n)
        return NULL;
    n->s = (unsigned char *)xstrndup(c->b + c->pos, len);
    if (!n->s) {
        free(n);
        return NULL;
    }
    n->slen = len;
    c->pos += len;
    return n;
}

static ben_node *dec_list(dctx *c, int depth)
{
    size_t start = c->pos;
    c->pos++; /* 'l' */
    ben_node *n = node_new(BEN_LIST);
    if (!n)
        return NULL;
    for (;;) {
        if (c->pos >= c->len) {
            derr(c, "liste non terminée (ouverte à l'offset %zu)", start);
            goto fail;
        }
        if (c->b[c->pos] == 'e') {
            c->pos++;
            return n;
        }
        ben_node *v = dec(c, depth + 1);
        if (!v)
            goto fail;
        if (list_push(n, v) < 0) {
            ben_free(v);
            goto fail;
        }
    }
fail:
    ben_free(n);
    return NULL;
}

static ben_node *dec_dict(dctx *c, int depth)
{
    size_t start = c->pos;
    c->pos++; /* 'd' */
    ben_node *n = node_new(BEN_DICT);
    if (!n)
        return NULL;
    for (;;) {
        if (c->pos >= c->len) {
            derr(c, "dictionnaire non terminé (ouvert à l'offset %zu)", start);
            goto fail;
        }
        if (c->b[c->pos] == 'e') {
            c->pos++;
            return n;
        }
        if (!is_digit(c->b[c->pos])) {
            derr(c, "clé de dictionnaire non chaîne à l'offset %zu", c->pos);
            goto fail;
        }
        size_t klen;
        if (read_len(c, &klen) < 0)
            goto fail;
        char *key = xstrndup(c->b + c->pos, klen);
        if (!key)
            goto fail;
        c->pos += klen;
        ben_node *v = dec(c, depth + 1);
        if (!v) {
            free(key);
            goto fail;
        }
        if (dict_push(n, key, klen, v) < 0) {
            free(key);
            ben_free(v);
            goto fail;
        }
    }
fail:
    ben_free(n);
    return NULL;
}

static ben_node *dec(dctx *c, int depth)
{
    if (depth > BEN_MAX_DEPTH) {
        derr(c, "imbrication trop profonde à l'offset %zu", c->pos);
        return NULL;
    }
    if (c->pos >= c->len) {
        derr(c, "fin de données inattendue à l'offset %zu", c->pos);
        return NULL;
    }
    size_t start = c->pos;
    unsigned char ch = c->b[c->pos];
    ben_node *n;
    if (ch == 'i')
        n = dec_int(c);
    else if (is_digit(ch))
        n = dec_str(c);
    else if (ch == 'l')
        n = dec_list(c, depth);
    else if (ch == 'd')
        n = dec_dict(c, depth);
    else {
        derr(c, "octet inattendu 0x%02x à l'offset %zu", ch, start);
        return NULL;
    }
    if (n) {
        n->raw_start = start;
        n->raw_end = c->pos;
    }
    return n;
}

ben_node *ben_decode(const void *buf, size_t len, char *err, size_t errlen)
{
    dctx c = { buf, len, 0, err, errlen };
    if (err && errlen)
        err[0] = 0;
    if (len == 0) {
        derr(&c, "données vides");
        return NULL;
    }
    ben_node *n = dec(&c, 0);
    if (!n) {
        derr(&c, "mémoire insuffisante");
        return NULL;
    }
    if (c.pos != len) {
        derr(&c, "%zu octet(s) en trop après la fin de la valeur (offset %zu)", len - c.pos, c.pos);
        ben_free(n);
        return NULL;
    }
    return n;
}

/* ---- encoding ---- */

static int kv_cmp(const void *a, const void *b)
{
    const ben_kv *x = *(const ben_kv *const *)a;
    const ben_kv *y = *(const ben_kv *const *)b;
    size_t m = x->key_len < y->key_len ? x->key_len : y->key_len;
    int r = memcmp(x->key, y->key, m);
    if (r)
        return r;
    return (x->key_len > y->key_len) - (x->key_len < y->key_len);
}

int ben_encode(const ben_node *n, bbuf *out)
{
    switch (n->type) {
    case BEN_INT:
        return bbuf_printf(out, "i%" PRId64 "e", n->i);
    case BEN_STR:
        if (bbuf_printf(out, "%zu:", n->slen) < 0)
            return -1;
        return bbuf_append(out, n->s, n->slen);
    case BEN_LIST:
        if (bbuf_putc(out, 'l') < 0)
            return -1;
        for (size_t i = 0; i < n->count; i++)
            if (ben_encode(n->items[i], out) < 0)
                return -1;
        return bbuf_putc(out, 'e');
    case BEN_DICT: {
        const ben_kv **sorted = malloc((n->nkv ? n->nkv : 1) * sizeof *sorted);
        if (!sorted)
            return -1;
        for (size_t i = 0; i < n->nkv; i++)
            sorted[i] = &n->kv[i];
        qsort(sorted, n->nkv, sizeof *sorted, kv_cmp);
        int rc = bbuf_putc(out, 'd');
        for (size_t i = 0; rc == 0 && i < n->nkv; i++) {
            rc = bbuf_printf(out, "%zu:", sorted[i]->key_len);
            if (rc == 0)
                rc = bbuf_append(out, sorted[i]->key, sorted[i]->key_len);
            if (rc == 0)
                rc = ben_encode(sorted[i]->val, out);
        }
        if (rc == 0)
            rc = bbuf_putc(out, 'e');
        free(sorted);
        return rc;
    }
    }
    return -1;
}

/* ---- builders ---- */

ben_node *ben_new_int(int64_t v)
{
    ben_node *n = node_new(BEN_INT);
    if (n)
        n->i = v;
    return n;
}

ben_node *ben_new_str(const void *s, size_t len)
{
    ben_node *n = node_new(BEN_STR);
    if (!n)
        return NULL;
    n->s = (unsigned char *)xstrndup(s, len);
    if (!n->s) {
        free(n);
        return NULL;
    }
    n->slen = len;
    return n;
}

ben_node *ben_new_strz(const char *s)
{
    return ben_new_str(s, strlen(s));
}

ben_node *ben_new_list(void)
{
    return node_new(BEN_LIST);
}

ben_node *ben_new_dict(void)
{
    return node_new(BEN_DICT);
}

int ben_list_add(ben_node *list, ben_node *v)
{
    if (!list || !v || list->type != BEN_LIST || list_push(list, v) < 0) {
        ben_free(v);
        return -1;
    }
    return 0;
}

int ben_dict_put(ben_node *dict, const char *key, ben_node *v)
{
    if (!dict || !v || dict->type != BEN_DICT) {
        ben_free(v);
        return -1;
    }
    size_t klen = strlen(key);
    for (size_t i = 0; i < dict->nkv; i++) {
        if (dict->kv[i].key_len == klen && memcmp(dict->kv[i].key, key, klen) == 0) {
            ben_free(dict->kv[i].val);
            dict->kv[i].val = v;
            return 0;
        }
    }
    char *k = xstrdup(key);
    if (!k || dict_push(dict, k, klen, v) < 0) {
        free(k);
        ben_free(v);
        return -1;
    }
    return 0;
}

/* ---- lookups ---- */

ben_node *ben_dict_get(const ben_node *dict, const char *key)
{
    if (!dict || dict->type != BEN_DICT)
        return NULL;
    size_t klen = strlen(key);
    for (size_t i = 0; i < dict->nkv; i++)
        if (dict->kv[i].key_len == klen && memcmp(dict->kv[i].key, key, klen) == 0)
            return dict->kv[i].val;
    return NULL;
}

int ben_get_int(const ben_node *dict, const char *key, int64_t *out)
{
    ben_node *n = ben_dict_get(dict, key);
    if (!n || n->type != BEN_INT)
        return 0;
    *out = n->i;
    return 1;
}

const unsigned char *ben_get_str(const ben_node *dict, const char *key, size_t *len)
{
    ben_node *n = ben_dict_get(dict, key);
    if (!n || n->type != BEN_STR)
        return NULL;
    if (len)
        *len = n->slen;
    return n->s;
}

/* ---- pretty printer ---- */

static void pretty_str(const unsigned char *s, size_t len, bbuf *out)
{
    if (is_printable_text(s, len)) {
        size_t shown = len > 300 ? 300 : len;
        bbuf_putc(out, '"');
        bbuf_append(out, s, shown);
        bbuf_printf(out, "%s\"", shown < len ? "…" : "");
        return;
    }
    size_t shown = len > 32 ? 32 : len;
    char hex[65];
    hex_encode(s, shown, hex);
    bbuf_printf(out, "<%zu octets binaires: %s%s>", len, hex, shown < len ? "…" : "");
}

static void indent_to(bbuf *out, int indent)
{
    for (int i = 0; i < indent; i++)
        bbuf_append(out, "  ", 2);
}

void ben_pretty(const ben_node *n, bbuf *out, int indent)
{
    switch (n->type) {
    case BEN_INT:
        bbuf_printf(out, "%" PRId64, n->i);
        break;
    case BEN_STR:
        pretty_str(n->s, n->slen, out);
        break;
    case BEN_LIST:
        if (!n->count) {
            bbuf_append(out, "[]", 2);
            break;
        }
        bbuf_append(out, "[\n", 2);
        for (size_t i = 0; i < n->count; i++) {
            indent_to(out, indent + 1);
            ben_pretty(n->items[i], out, indent + 1);
            bbuf_append(out, "\n", 1);
        }
        indent_to(out, indent);
        bbuf_putc(out, ']');
        break;
    case BEN_DICT:
        if (!n->nkv) {
            bbuf_append(out, "{}", 2);
            break;
        }
        bbuf_append(out, "{\n", 2);
        for (size_t i = 0; i < n->nkv; i++) {
            indent_to(out, indent + 1);
            pretty_str((const unsigned char *)n->kv[i].key, n->kv[i].key_len, out);
            bbuf_append(out, ": ", 2);
            ben_pretty(n->kv[i].val, out, indent + 1);
            bbuf_append(out, "\n", 1);
        }
        indent_to(out, indent);
        bbuf_putc(out, '}');
        break;
    }
}
