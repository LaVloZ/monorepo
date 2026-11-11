#include <windows.h>
#include <bcrypt.h>

#include "torrent.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bencode.h"
#include "util.h"

#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif

#define TORRENT_MAX_FILE_SIZE (64 * 1024 * 1024)

static int sha1(const void *data, size_t len, uint8_t out[20])
{
    BCRYPT_ALG_HANDLE alg;
    if (len > 0xFFFFFFFFu || !BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, NULL, 0)))
        return -1;
    NTSTATUS st = BCryptHash(alg, NULL, 0, (PUCHAR)data, (ULONG)len, out, 20);
    BCryptCloseAlgorithmProvider(alg, 0);
    return BCRYPT_SUCCESS(st) ? 0 : -1;
}

static void set_err(char *err, size_t errlen, const char *fmt, ...) PRINTF_FMT(3, 4);
static void set_err(char *err, size_t errlen, const char *fmt, ...)
{
    if (!err || !errlen)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
}

void torrent_free(torrent_info *t)
{
    free(t->name);
    free(t->created_by);
    free(t->comment);
    for (size_t i = 0; i < t->nfiles; i++)
        free(t->files[i].path);
    free(t->files);
    for (size_t i = 0; i < t->ntrackers; i++)
        free(t->trackers[i].url);
    free(t->trackers);
    for (size_t i = 0; i < t->nweb_seeds; i++)
        free(t->web_seeds[i]);
    free(t->web_seeds);
    memset(t, 0, sizeof *t);
}

/* Copies a string value, trimming surrounding whitespace. */
static char *trimmed(const unsigned char *s, size_t len)
{
    while (len && (s[0] == ' ' || s[0] == '\t' || s[0] == '\r' || s[0] == '\n')) {
        s++;
        len--;
    }
    while (len && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n'))
        len--;
    return xstrndup(s, len);
}

static int add_tracker(torrent_info *t, const ben_node *s, int tier)
{
    if (!s || s->type != BEN_STR)
        return 0;
    char *url = trimmed(s->s, s->slen);
    if (!url)
        return -1;
    if (!url[0]) {
        free(url);
        return 0;
    }
    for (size_t i = 0; i < t->ntrackers; i++) {
        if (strcmp(t->trackers[i].url, url) == 0) {
            free(url);
            return 0;
        }
    }
    tor_tracker *p = realloc(t->trackers, (t->ntrackers + 1) * sizeof *p);
    if (!p) {
        free(url);
        return -1;
    }
    t->trackers = p;
    t->trackers[t->ntrackers].url = url;
    t->trackers[t->ntrackers].tier = tier;
    t->ntrackers++;
    return 0;
}

static int add_web_seed(torrent_info *t, const ben_node *s)
{
    if (!s || s->type != BEN_STR || !s->slen)
        return 0;
    char **p = realloc(t->web_seeds, (t->nweb_seeds + 1) * sizeof *p);
    if (!p)
        return -1;
    t->web_seeds = p;
    t->web_seeds[t->nweb_seeds] = trimmed(s->s, s->slen);
    if (!t->web_seeds[t->nweb_seeds])
        return -1;
    t->nweb_seeds++;
    return 0;
}

/* Prefers the ".utf-8" variant of a key when present. */
static const ben_node *get_utf8(const ben_node *d, const char *key)
{
    char k[64];
    snprintf(k, sizeof k, "%s.utf-8", key);
    const ben_node *n = ben_dict_get(d, k);
    if (n)
        return n;
    return ben_dict_get(d, key);
}

static char *dup_str_node(const ben_node *n)
{
    if (!n || n->type != BEN_STR)
        return NULL;
    return xstrndup(n->s, n->slen);
}

static int parse_files(torrent_info *t, const ben_node *files, char *err, size_t errlen)
{
    t->multi_file = 1;
    t->files = calloc(files->count ? files->count : 1, sizeof *t->files);
    if (!t->files)
        return -1;
    for (size_t i = 0; i < files->count; i++) {
        const ben_node *f = files->items[i];
        int64_t length;
        if (f->type != BEN_DICT || !ben_get_int(f, "length", &length) || length < 0) {
            set_err(err, errlen, "fichier #%zu : 'length' invalide", i);
            return -1;
        }
        const ben_node *path = get_utf8(f, "path");
        if (!path || path->type != BEN_LIST || path->count == 0) {
            set_err(err, errlen, "fichier #%zu : 'path' invalide", i);
            return -1;
        }
        bbuf b;
        bbuf_init(&b);
        for (size_t k = 0; k < path->count; k++) {
            const ben_node *part = path->items[k];
            if (part->type != BEN_STR) {
                bbuf_free(&b);
                set_err(err, errlen, "fichier #%zu : élément de chemin non chaîne", i);
                return -1;
            }
            if (k)
                bbuf_putc(&b, '/');
            bbuf_append(&b, part->s, part->slen);
        }
        t->files[t->nfiles].path = bbuf_steal(&b);
        t->files[t->nfiles].length = length;
        t->nfiles++;
        t->total_size += length;
    }
    return 0;
}

int torrent_parse(const unsigned char *buf, size_t len, torrent_info *t, char *err, size_t errlen)
{
    memset(t, 0, sizeof *t);
    char berr[200];
    ben_node *root = ben_decode(buf, len, berr, sizeof berr);
    if (!root) {
        set_err(err, errlen, "fichier .torrent invalide (bencode) : %s", berr);
        return -1;
    }
    int rc = -1;
    if (root->type != BEN_DICT) {
        set_err(err, errlen, "la racine du .torrent doit être un dictionnaire");
        goto out;
    }
    const ben_node *info = ben_dict_get(root, "info");
    if (!info || info->type != BEN_DICT) {
        set_err(err, errlen, "dictionnaire 'info' absent");
        goto out;
    }

    if (sha1(buf + info->raw_start, info->raw_end - info->raw_start, t->info_hash) < 0) {
        set_err(err, errlen, "calcul SHA-1 impossible");
        goto out;
    }
    hex_encode(t->info_hash, 20, t->info_hash_hex);

    size_t plen;
    const unsigned char *pieces = ben_get_str(info, "pieces", &plen);
    if (!pieces) {
        int64_t mv;
        if (ben_get_int(info, "meta version", &mv) && mv == 2)
            set_err(err, errlen, "torrent BitTorrent v2 uniquement (BEP 52) : non supporté");
        else
            set_err(err, errlen, "champ 'pieces' absent du dictionnaire info");
        goto out;
    }
    if (plen % 20) {
        set_err(err, errlen, "'pieces' a une longueur (%zu) non multiple de 20", plen);
        goto out;
    }
    t->num_pieces = plen / 20;
    if (!ben_get_int(info, "piece length", &t->piece_length) || t->piece_length <= 0) {
        set_err(err, errlen, "'piece length' absent ou invalide");
        goto out;
    }

    t->name = dup_str_node(get_utf8(info, "name"));
    if (!t->name)
        t->name = xstrdup("(sans nom)");

    const ben_node *files = ben_dict_get(info, "files");
    if (files && files->type == BEN_LIST) {
        if (parse_files(t, files, err, errlen) < 0)
            goto out;
    } else {
        int64_t length;
        if (!ben_get_int(info, "length", &length) || length < 0) {
            set_err(err, errlen, "ni 'files' ni 'length' dans le dictionnaire info");
            goto out;
        }
        t->files = calloc(1, sizeof *t->files);
        if (!t->files)
            goto out;
        t->files[0].path = xstrdup(t->name);
        t->files[0].length = length;
        t->nfiles = 1;
        t->total_size = length;
    }

    int64_t v;
    if (ben_get_int(info, "private", &v))
        t->is_private = v == 1;
    if (ben_get_int(root, "creation date", &v)) {
        t->has_creation_date = 1;
        t->creation_date = v;
    }
    t->created_by = dup_str_node(ben_dict_get(root, "created by"));
    t->comment = dup_str_node(get_utf8(root, "comment"));

    const ben_node *al = ben_dict_get(root, "announce-list");
    if (al && al->type == BEN_LIST) {
        for (size_t i = 0; i < al->count; i++) {
            const ben_node *tier = al->items[i];
            if (tier->type == BEN_LIST) {
                for (size_t k = 0; k < tier->count; k++)
                    if (add_tracker(t, tier->items[k], (int)i + 1) < 0)
                        goto out;
            } else if (add_tracker(t, tier, (int)i + 1) < 0) {
                goto out;
            }
        }
    }
    if (add_tracker(t, ben_dict_get(root, "announce"), 0) < 0)
        goto out;

    const ben_node *ul = ben_dict_get(root, "url-list");
    if (ul && ul->type == BEN_LIST) {
        for (size_t i = 0; i < ul->count; i++)
            if (add_web_seed(t, ul->items[i]) < 0)
                goto out;
    } else if (add_web_seed(t, ul) < 0) {
        goto out;
    }
    rc = 0;
out:
    ben_free(root);
    if (rc < 0)
        torrent_free(t);
    return rc;
}

int torrent_load_file(const char *path, torrent_info *t, char *err, size_t errlen)
{
    memset(t, 0, sizeof *t);
    /* path is UTF-8: open it with the wide API so non-ASCII names work. */
    wchar_t *wpath = utf8_to_wide(path);
    FILE *f = wpath ? _wfopen(wpath, L"rb") : NULL;
    free(wpath);
    if (!f) {
        set_err(err, errlen, "impossible d'ouvrir %s : %s", path, strerror(errno));
        return -1;
    }
    bbuf b;
    bbuf_init(&b);
    unsigned char chunk[65536];
    size_t n;
    while ((n = fread(chunk, 1, sizeof chunk, f)) > 0) {
        if (b.len + n > TORRENT_MAX_FILE_SIZE || bbuf_append(&b, chunk, n) < 0) {
            set_err(err, errlen, "fichier trop volumineux pour un .torrent");
            fclose(f);
            bbuf_free(&b);
            return -1;
        }
    }
    int read_err = ferror(f);
    fclose(f);
    if (read_err) {
        set_err(err, errlen, "erreur de lecture de %s", path);
        bbuf_free(&b);
        return -1;
    }
    int rc = torrent_parse(b.data, b.len, t, err, errlen);
    bbuf_free(&b);
    return rc;
}
