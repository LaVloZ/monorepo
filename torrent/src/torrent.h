#ifndef TORRENT_H
#define TORRENT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *path;
    int64_t length;
} tor_file;

typedef struct {
    char *url;
    int tier; /* 0 = "announce" key, 1.. = announce-list tier */
} tor_tracker;

typedef struct {
    char *name;
    uint8_t info_hash[20];
    char info_hash_hex[41];
    int64_t total_size;
    int64_t piece_length;
    size_t num_pieces;
    int is_private;
    int has_creation_date;
    int64_t creation_date;
    char *created_by;
    char *comment;
    int multi_file;
    tor_file *files;
    size_t nfiles;
    tor_tracker *trackers;
    size_t ntrackers;
    char **web_seeds;
    size_t nweb_seeds;
} torrent_info;

/* info_hash = SHA-1 of the raw bytes of the "info" value, as found in buf. */
int torrent_parse(const unsigned char *buf, size_t len, torrent_info *t, char *err, size_t errlen);
int torrent_load_file(const char *path, torrent_info *t, char *err, size_t errlen);
void torrent_free(torrent_info *t);

#endif
