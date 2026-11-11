/* Unit tests: bencode, info_hash, .torrent parsing, peers, UDP packets, HTTP URL, history. */

#include <windows.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bencode.h"
#include "history.h"
#include "http_tracker.h"
#include "peer_probe.h"
#include "torrent.h"
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

static ben_node *dec(const char *s)
{
    char err[200];
    return ben_decode(s, strlen(s), err, sizeof err);
}

static int decode_fails(const char *s)
{
    ben_node *n = dec(s);
    ben_free(n);
    return n == NULL;
}

static void test_bencode_decode(void)
{
    ben_node *n = dec("i42e");
    CHECK(n && n->type == BEN_INT && n->i == 42);
    ben_free(n);
    n = dec("i-17e");
    CHECK(n && n->i == -17);
    ben_free(n);
    n = dec("i0e");
    CHECK(n && n->i == 0);
    ben_free(n);
    n = dec("i9223372036854775807e");
    CHECK(n && n->i == INT64_MAX);
    ben_free(n);
    n = dec("i-9223372036854775808e");
    CHECK(n && n->i == INT64_MIN);
    ben_free(n);

    n = dec("4:spam");
    CHECK(n && n->type == BEN_STR && n->slen == 4 && memcmp(n->s, "spam", 4) == 0);
    ben_free(n);
    n = dec("0:");
    CHECK(n && n->type == BEN_STR && n->slen == 0);
    ben_free(n);

    n = dec("l4:spami7ee");
    CHECK(n && n->type == BEN_LIST && n->count == 2 && n->items[1]->i == 7);
    ben_free(n);

    n = dec("d3:cow3:moo4:spaml1:a1:bee");
    CHECK(n && n->type == BEN_DICT && n->nkv == 2);
    size_t len;
    const unsigned char *s = ben_get_str(n, "cow", &len);
    CHECK(s && len == 3 && memcmp(s, "moo", 3) == 0);
    ben_node *spam = ben_dict_get(n, "spam");
    CHECK(spam && spam->type == BEN_LIST && spam->count == 2);
    CHECK(ben_dict_get(n, "absent") == NULL);
    ben_free(n);

    /* Binary string with a NUL byte. */
    const char bin[] = "3:a\0b";
    n = ben_decode(bin, 5, NULL, 0);
    CHECK(n && n->slen == 3 && n->s[1] == 0 && n->s[2] == 'b');
    ben_free(n);

    /* Raw spans. */
    n = dec("d4:infod1:xi1eee");
    ben_node *info = ben_dict_get(n, "info");
    CHECK(info && info->raw_start == 7 && info->raw_end == 15);
    ben_free(n);

    /* Invalid inputs. */
    CHECK(decode_fails(""));
    CHECK(decode_fails("i-0e"));
    CHECK(decode_fails("i03e"));
    CHECK(decode_fails("ie"));
    CHECK(decode_fails("i12"));
    CHECK(decode_fails("i9223372036854775808e"));
    CHECK(decode_fails("5:abc"));
    CHECK(decode_fails("01:a"));
    CHECK(decode_fails("l4:spam"));
    CHECK(decode_fails("di1ei2ee"));
    CHECK(decode_fails("d3:keye"));
    CHECK(decode_fails("i1ei2e"));
    CHECK(decode_fails("x"));
    CHECK(decode_fails("<html>"));

    char err[200];
    n = ben_decode("i1ex", 4, err, sizeof err);
    CHECK(n == NULL && strstr(err, "en trop") != NULL);

    /* Depth limit. */
    char deep[300];
    memset(deep, 'l', 150);
    memset(deep + 150, 'e', 150);
    CHECK(ben_decode(deep, 300, NULL, 0) == NULL);
}

static void test_bencode_encode(void)
{
    ben_node *d = ben_new_dict();
    ben_node *l = ben_new_list();
    ben_list_add(l, ben_new_strz("foo"));
    ben_list_add(l, ben_new_int(-3));
    ben_dict_put(d, "b", l);
    ben_dict_put(d, "a", ben_new_int(1));
    ben_dict_put(d, "ab", ben_new_str("x\0y", 3));
    bbuf b;
    bbuf_init(&b);
    CHECK(ben_encode(d, &b) == 0);
    static const char expected[] = "d1:ai1e2:ab3:x\0y1:bl3:fooi-3eee";
    CHECK(b.len == sizeof expected - 1 && memcmp(b.data, expected, b.len) == 0);

    /* Round trip. */
    ben_node *r = ben_decode(b.data, b.len, NULL, 0);
    CHECK(r != NULL);
    bbuf b2;
    bbuf_init(&b2);
    CHECK(r && ben_encode(r, &b2) == 0 && b2.len == b.len && memcmp(b2.data, b.data, b.len) == 0);
    bbuf_free(&b2);
    ben_free(r);
    bbuf_free(&b);

    /* put replaces an existing key. */
    ben_dict_put(d, "a", ben_new_int(2));
    int64_t v = 0;
    CHECK(ben_get_int(d, "a", &v) && v == 2);
    ben_free(d);
}

static const char INFO_CANON[] = "d6:lengthi12345e4:name8:test.bin12:piece lengthi16384e6:pieces20:AAAAAAAAAAAAAAAAAAAAe";
static const char INFO_NONCANON[] =
    "d4:name8:test.bin6:lengthi12345e12:piece lengthi16384e6:pieces20:AAAAAAAAAAAAAAAAAAAAe";

static void test_info_hash(void)
{
    char buf[512];
    torrent_info t;
    char err[200];

    int n = snprintf(buf, sizeof buf, "d8:announce32:udp://tracker.test:6969/announce4:info%se", INFO_CANON);
    CHECK(torrent_parse((unsigned char *)buf, (size_t)n, &t, err, sizeof err) == 0);
    CHECK_STR(t.info_hash_hex, "e67b0825a0c476a33f41233e7cda2c92efefe93d");
    CHECK_STR(t.name, "test.bin");
    CHECK(t.total_size == 12345 && t.piece_length == 16384 && t.num_pieces == 1 && t.nfiles == 1);
    CHECK(t.ntrackers == 1 && t.trackers[0].tier == 0);
    torrent_free(&t);

    /* Non-canonical info dict: the hash must cover the raw bytes, not a re-encoding. */
    n = snprintf(buf, sizeof buf, "d8:announce32:udp://tracker.test:6969/announce4:info%se", INFO_NONCANON);
    CHECK(torrent_parse((unsigned char *)buf, (size_t)n, &t, err, sizeof err) == 0);
    CHECK_STR(t.info_hash_hex, "8d31c85847ad3d71281790d1ed9393a19c9888dc");
    torrent_free(&t);

    /* v2-only torrent is rejected with a clear message. */
    const char *v2 = "d4:infod12:meta versioni2e4:name1:x12:piece lengthi16384eee";
    CHECK(torrent_parse((const unsigned char *)v2, strlen(v2), &t, err, sizeof err) < 0);
    CHECK(strstr(err, "v2") != NULL);

    const char *noinfo = "d8:announce3:abce";
    CHECK(torrent_parse((const unsigned char *)noinfo, strlen(noinfo), &t, err, sizeof err) < 0);
    CHECK(strstr(err, "info") != NULL);
}

static void test_torrent_parse(void)
{
    /* Multi-file torrent with announce-list, duplicate trackers, url-list. */
    ben_node *root = ben_new_dict();
    ben_dict_put(root, "announce", ben_new_strz("http://main.test/announce"));
    ben_node *al = ben_new_list();
    ben_node *t1 = ben_new_list();
    ben_list_add(t1, ben_new_strz("udp://a.test:6969/announce"));
    ben_list_add(t1, ben_new_strz("udp://b.test:1337"));
    ben_node *t2 = ben_new_list();
    ben_list_add(t2, ben_new_strz(" udp://a.test:6969/announce "));
    ben_list_add(t2, ben_new_strz("https://c.test/announce"));
    ben_list_add(al, t1);
    ben_list_add(al, t2);
    ben_dict_put(root, "announce-list", al);
    ben_dict_put(root, "url-list", ben_new_strz("http://seed.test/files/"));
    ben_dict_put(root, "creation date", ben_new_int(1700000000));
    ben_dict_put(root, "created by", ben_new_strz("unit test"));
    ben_dict_put(root, "comment", ben_new_strz("hello"));

    ben_node *info = ben_new_dict();
    ben_dict_put(info, "name", ben_new_strz("dir"));
    ben_dict_put(info, "piece length", ben_new_int(32768));
    ben_dict_put(info, "pieces", ben_new_str("0123456789012345678901234567890123456789", 40));
    ben_dict_put(info, "private", ben_new_int(1));
    ben_node *files = ben_new_list();
    for (int i = 0; i < 2; i++) {
        ben_node *f = ben_new_dict();
        ben_dict_put(f, "length", ben_new_int(i ? 200 : 100));
        ben_node *path = ben_new_list();
        ben_list_add(path, ben_new_strz("sub"));
        ben_list_add(path, ben_new_strz(i ? "b.txt" : "a.txt"));
        ben_dict_put(f, "path", path);
        ben_list_add(files, f);
    }
    ben_dict_put(info, "files", files);
    ben_dict_put(root, "info", info);

    bbuf b;
    bbuf_init(&b);
    ben_encode(root, &b);
    ben_free(root);

    torrent_info t;
    char err[200];
    CHECK(torrent_parse(b.data, b.len, &t, err, sizeof err) == 0);
    CHECK(t.multi_file && t.nfiles == 2 && t.total_size == 300);
    CHECK_STR(t.files[1].path, "sub/b.txt");
    CHECK(t.num_pieces == 2 && t.is_private);
    CHECK(t.has_creation_date && t.creation_date == 1700000000);
    CHECK_STR(t.created_by, "unit test");
    CHECK_STR(t.comment, "hello");
    CHECK(t.ntrackers == 4);
    if (t.ntrackers == 4) {
        CHECK_STR(t.trackers[0].url, "udp://a.test:6969/announce");
        CHECK(t.trackers[0].tier == 1);
        CHECK_STR(t.trackers[1].url, "udp://b.test:1337");
        CHECK_STR(t.trackers[2].url, "https://c.test/announce");
        CHECK(t.trackers[2].tier == 2);
        CHECK_STR(t.trackers[3].url, "http://main.test/announce");
        CHECK(t.trackers[3].tier == 0);
    }
    CHECK(t.nweb_seeds == 1);
    torrent_free(&t);
    bbuf_free(&b);
}

static void test_compact_peers(void)
{
    tr_result r;
    tr_result_init(&r);
    const uint8_t v4[] = { 192, 168, 1, 10, 0x1A, 0xE1, 10, 0, 0, 1, 0xC8, 0xD5 };
    CHECK(tr_parse_compact_peers(v4, sizeof v4, 0, &r) == 0);
    CHECK(r.npeers == 2);
    CHECK_STR(r.peers[0].ip, "192.168.1.10");
    CHECK(r.peers[0].port == 6881);
    CHECK_STR(r.peers[1].ip, "10.0.0.1");
    CHECK(r.peers[1].port == 51413);
    tr_result_free(&r);

    uint8_t v6[18 + 3] = { 0x20, 0x01, 0x0d, 0xb8 };
    v6[15] = 1;
    v6[16] = 0x1A;
    v6[17] = 0xE1;
    CHECK(tr_parse_compact_peers(v6, sizeof v6, 1, &r) == 3);
    CHECK(r.npeers == 1);
    CHECK_STR(r.peers[0].ip, "2001:db8::1");
    CHECK(r.peers[0].port == 6881);
    tr_result_free(&r);

    /* 7 bytes = 1 peer + 1 leftover byte. */
    CHECK(tr_parse_compact_peers(v4, 7, 0, &r) == 1 && r.npeers == 1);
    tr_result_free(&r);
}

static void test_http_response(void)
{
    /* Dictionary peers + compact peers6. */
    ben_node *d = ben_new_dict();
    ben_dict_put(d, "interval", ben_new_int(1800));
    ben_dict_put(d, "min interval", ben_new_int(900));
    ben_dict_put(d, "complete", ben_new_int(5));
    ben_dict_put(d, "incomplete", ben_new_int(3));
    ben_dict_put(d, "tracker id", ben_new_strz("tid42"));
    ben_dict_put(d, "warning message", ben_new_strz("slow down"));
    ben_node *peers = ben_new_list();
    ben_node *p = ben_new_dict();
    ben_dict_put(p, "ip", ben_new_strz("10.0.0.1"));
    ben_dict_put(p, "port", ben_new_int(6881));
    ben_dict_put(p, "peer id", ben_new_strz("-qB4650-abcdefghijkl"));
    ben_list_add(peers, p);
    ben_dict_put(d, "peers", peers);
    uint8_t v6[18] = { 0 };
    v6[15] = 1;
    v6[17] = 80;
    ben_dict_put(d, "peers6", ben_new_str(v6, 18));
    bbuf b;
    bbuf_init(&b);
    ben_encode(d, &b);
    ben_free(d);

    tr_result r;
    tr_result_init(&r);
    CHECK(http_parse_response(b.data, b.len, &r) == 0);
    CHECK(r.has_interval && r.interval == 1800 && r.has_min_interval && r.min_interval == 900);
    CHECK(r.complete == 5 && r.incomplete == 3);
    CHECK_STR(r.tracker_id, "tid42");
    CHECK_STR(r.warning, "slow down");
    CHECK(r.failure == NULL);
    CHECK(r.npeers == 2);
    if (r.npeers == 2) {
        CHECK_STR(r.peers[0].ip, "10.0.0.1");
        CHECK(r.peers[0].peer_id_len == 20 && memcmp(r.peers[0].peer_id, "-qB4650-", 8) == 0);
        CHECK_STR(r.peers[1].ip, "::1");
        CHECK(r.peers[1].port == 80);
    }
    tr_result_free(&r);
    bbuf_free(&b);

    /* Common extensions and non-standard keys. */
    d = ben_new_dict();
    ben_dict_put(d, "interval", ben_new_int(60));
    ben_dict_put(d, "downloaded", ben_new_int(1234));
    const uint8_t ext_ip[4] = { 203, 0, 113, 7 };
    ben_dict_put(d, "external ip", ben_new_str(ext_ip, 4));
    ben_dict_put(d, "retry in", ben_new_int(30));
    const uint8_t crypto[3] = { 1, 0, 1 };
    ben_dict_put(d, "crypto_flags", ben_new_str(crypto, 3));
    ben_dict_put(d, "x-msg", ben_new_strz("hi"));
    ben_dict_put(d, "flags", ben_new_list());
    bbuf_init(&b);
    ben_encode(d, &b);
    ben_free(d);
    tr_result_init(&r);
    CHECK(http_parse_response(b.data, b.len, &r) == 0);
    CHECK(r.has_downloaded && r.downloaded == 1234);
    CHECK(strcmp(r.external_ip, "203.0.113.7") == 0);
    CHECK_STR(r.retry_in, "30 minute(s)");
    CHECK(r.has_crypto && r.crypto_total == 3 && r.crypto_on == 2);
    CHECK(r.other_keys && strstr(r.other_keys, "x-msg = \"hi\"") && strstr(r.other_keys, "flags = [liste"));
    CHECK(r.other_keys && !strstr(r.other_keys, "interval"));
    tr_result_free(&r);
    bbuf_free(&b);

    const char *never = "d8:retry in5:nevere";
    tr_result_init(&r);
    CHECK(http_parse_response((const uint8_t *)never, strlen(never), &r) == 0);
    CHECK_STR(r.retry_in, "\"never\"");
    CHECK(r.other_keys == NULL);
    tr_result_free(&r);

    const char *fail = "d14:failure reason12:unregisterede";
    tr_result_init(&r);
    CHECK(http_parse_response((const uint8_t *)fail, strlen(fail), &r) == 0);
    CHECK_STR(r.failure, "unregistered");
    tr_result_free(&r);

    const char *html = "<html>Not Found</html>";
    tr_result_init(&r);
    CHECK(http_parse_response((const uint8_t *)html, strlen(html), &r) < 0);
    CHECK(strstr(r.error, "non bencodée") != NULL);
    tr_result_free(&r);
}

static void sample_params(tr_params *p)
{
    memset(p, 0, sizeof *p);
    for (int i = 0; i < 20; i++)
        p->info_hash[i] = (uint8_t)i;
    memcpy(p->peer_id, "-DB0001-ABCDEFGHIJKL", 20);
    p->downloaded = 1000;
    p->left = 2000;
    p->uploaded = 3000;
    p->event = TR_EVENT_STARTED;
    p->key = 0xDEADBEEF;
    p->numwant = -1;
    p->port = 6881;
    p->compact = 1;
}

static void test_udp_packets(void)
{
    uint8_t c[UDP_CONNECT_LEN];
    udp_build_connect(c, 0x12345678);
    static const uint8_t expected_connect[16] = { 0x00, 0x00, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80,
                                                  0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78 };
    CHECK(memcmp(c, expected_connect, 16) == 0);

    tr_params p;
    sample_params(&p);
    uint8_t a[UDP_ANNOUNCE_LEN];
    udp_build_announce(a, 0x0102030405060708ULL, 0xAABBCCDD, &p);
    static const uint8_t expected_announce[98] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,                         /* connection_id */
        0x00, 0x00, 0x00, 0x01,                                                 /* action */
        0xAA, 0xBB, 0xCC, 0xDD,                                                 /* transaction_id */
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,             /* info_hash */
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
        '-', 'D', 'B', '0', '0', '0', '1', '-', 'A', 'B',                      /* peer_id */
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8,                         /* downloaded 1000 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xD0,                         /* left 2000 */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB8,                         /* uploaded 3000 */
        0x00, 0x00, 0x00, 0x02,                                                 /* event started */
        0x00, 0x00, 0x00, 0x00,                                                 /* ip */
        0xDE, 0xAD, 0xBE, 0xEF,                                                 /* key */
        0xFF, 0xFF, 0xFF, 0xFF,                                                 /* num_want -1 */
        0x1A, 0xE1,                                                             /* port 6881 */
    };
    CHECK(memcmp(a, expected_announce, 98) == 0);
}

static void test_udp_parse(void)
{
    char msg[256];

    uint8_t cr[16];
    be_put32(cr, 0);
    be_put32(cr + 4, 0x11111111);
    be_put64(cr + 8, 0xCAFEBABEDEADBEEFULL);
    uint64_t cid = 0;
    CHECK(udp_parse_connect(cr, 16, 0x11111111, &cid, msg, sizeof msg) == UDP_PARSE_OK);
    CHECK(cid == 0xCAFEBABEDEADBEEFULL);
    CHECK(udp_parse_connect(cr, 16, 0x22222222, &cid, msg, sizeof msg) == UDP_PARSE_BAD_TID);
    CHECK(udp_parse_connect(cr, 12, 0x11111111, &cid, msg, sizeof msg) == UDP_PARSE_SHORT);
    be_put32(cr, 1);
    CHECK(udp_parse_connect(cr, 16, 0x11111111, &cid, msg, sizeof msg) == UDP_PARSE_BAD_ACTION);

    /* Announce response with 2 IPv4 peers. */
    uint8_t ar[20 + 12];
    be_put32(ar, 1);
    be_put32(ar + 4, 0xAABBCCDD);
    be_put32(ar + 8, 1800);
    be_put32(ar + 12, 4);
    be_put32(ar + 16, 9);
    const uint8_t peers[12] = { 1, 2, 3, 4, 0x1A, 0xE1, 5, 6, 7, 8, 0x00, 0x50 };
    memcpy(ar + 20, peers, 12);
    tr_result r;
    tr_result_init(&r);
    CHECK(udp_parse_announce(ar, sizeof ar, 0xAABBCCDD, 0, &r, msg, sizeof msg) == UDP_PARSE_OK);
    CHECK(r.interval == 1800 && r.incomplete == 4 && r.complete == 9);
    CHECK(r.npeers == 2);
    if (r.npeers == 2) {
        CHECK_STR(r.peers[0].ip, "1.2.3.4");
        CHECK(r.peers[0].port == 6881);
        CHECK_STR(r.peers[1].ip, "5.6.7.8");
        CHECK(r.peers[1].port == 80);
    }
    tr_result_free(&r);

    /* Invalid transaction_id is detected. */
    tr_result_init(&r);
    CHECK(udp_parse_announce(ar, sizeof ar, 0xAABBCCDE, 0, &r, msg, sizeof msg) == UDP_PARSE_BAD_TID);
    CHECK(strstr(msg, "transaction_id") != NULL);
    CHECK(r.npeers == 0);
    tr_result_free(&r);

    /* Error response (action 3). */
    uint8_t er[8 + 15];
    be_put32(er, 3);
    be_put32(er + 4, 0xAABBCCDD);
    memcpy(er + 8, "torrent inconnu", 15);
    tr_result_init(&r);
    CHECK(udp_parse_announce(er, sizeof er, 0xAABBCCDD, 0, &r, msg, sizeof msg) == UDP_PARSE_TRACKER_ERROR);
    CHECK_STR(msg, "torrent inconnu");
    tr_result_free(&r);

    /* Annotation of the response mentions every field. */
    bbuf b;
    bbuf_init(&b);
    udp_annotate_response(ar, sizeof ar, 0, &b);
    CHECK(strstr((char *)b.data, "interval") && strstr((char *)b.data, "peer[1]") &&
          strstr((char *)b.data, "5.6.7.8:80"));
    bbuf_free(&b);
}

static void test_http_url(void)
{
    tr_params p;
    sample_params(&p);
    char *u = http_build_url("http://t.test/announce", &p);
    CHECK_STR(u, "http://t.test/announce?info_hash=%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13"
                 "&peer_id=-DB0001-ABCDEFGHIJKL&port=6881&uploaded=3000&downloaded=1000&left=2000&compact=1"
                 "&key=DEADBEEF&event=started");
    free(u);

    p.event = TR_EVENT_NONE;
    p.numwant = 50;
    p.trackerid = "a b";
    u = http_build_url("https://t.test/ann?passkey=xyz#frag", &p);
    CHECK(strncmp(u, "https://t.test/ann?passkey=xyz&info_hash=%00", 44) == 0);
    CHECK(strstr(u, "event=") == NULL);
    CHECK(strstr(u, "&numwant=50") != NULL);
    CHECK(strstr(u, "&trackerid=a%20b") != NULL);
    CHECK(strstr(u, "#frag") == NULL);
    free(u);

    u = http_build_url("http://t.test/a?", &p);
    CHECK(strncmp(u, "http://t.test/a?info_hash=", 26) == 0);
    free(u);
}

static void test_parse_url(void)
{
    tr_url u;
    char err[200];
    CHECK(tr_parse_url("udp://opentor.net:6969/announce", &u, err, sizeof err) == 0);
    CHECK(u.transport == TR_UDP && strcmp(u.host, "opentor.net") == 0 && u.port == 6969);
    CHECK(tr_parse_url("UDP://1.2.3.4:80", &u, err, sizeof err) == 0 && u.port == 80);
    CHECK(tr_parse_url("udp://opentor.net/announce", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("http://t.test/announce", &u, err, sizeof err) == 0);
    CHECK(u.transport == TR_HTTP && u.port == 80 && !u.tls);
    CHECK(tr_parse_url("https://user:pw@t.test/announce", &u, err, sizeof err) == 0);
    CHECK(u.tls && u.port == 443 && strcmp(u.host, "t.test") == 0);
    CHECK(tr_parse_url("http://[::1]:8080/a", &u, err, sizeof err) == 0);
    CHECK(strcmp(u.host, "::1") == 0 && u.port == 8080);
    CHECK(tr_parse_url("ftp://t.test/", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("t.test/announce", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("udp://:6969", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("udp://host:99999", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("udp://host:12ab", &u, err, sizeof err) < 0);
    CHECK(tr_parse_url("", &u, err, sizeof err) < 0);
}

static void test_history(void)
{
    history h;
    history_init(&h);
    for (int i = 0; i < HISTORY_MAX + 5; i++) {
        tr_result r;
        tr_result_init(&r);
        r.npeers = 0;
        r.interval = i;
        r.url = xstrdup("udp://x:1");
        char t[16];
        snprintf(t, sizeof t, "%d", i);
        CHECK(history_add(&h, t, &r) == 0);
        CHECK(r.url == NULL); /* moved */
    }
    CHECK(h.count == HISTORY_MAX);
    CHECK(history_get(&h, 0)->result.interval == HISTORY_MAX + 4);
    CHECK_STR(history_get(&h, 0)->time, "104");
    CHECK(history_get(&h, HISTORY_MAX - 1)->result.interval == 5);
    CHECK(history_get(&h, HISTORY_MAX) == NULL);
    history_clear(&h);
    CHECK(h.count == 0 && history_get(&h, 0) == NULL);
}

static void test_probe_messages(void)
{
    uint8_t ih[20], pid[20], hs[PROBE_HANDSHAKE_LEN];
    for (int i = 0; i < 20; i++)
        ih[i] = (uint8_t)i;
    memcpy(pid, "-qB4650-abcdefghijkl", 20);
    probe_build_handshake(hs, ih, pid);
    CHECK(hs[0] == 19 && memcmp(hs + 1, "BitTorrent protocol", 19) == 0);
    CHECK(hs[25] == 0x10 && hs[27] == 0x04); /* BEP 10 + BEP 6 bits */
    CHECK(memcmp(hs + 28, ih, 20) == 0 && memcmp(hs + 48, pid, 20) == 0);

    probe_result r;
    memset(&r, 0, sizeof r);
    CHECK(probe_parse_handshake(hs, ih, &r) == 0);
    CHECK(r.ext_proto && r.fast_ext && !r.dht && r.has_peer_id);
    uint8_t other[20] = { 9 };
    CHECK(probe_parse_handshake(hs, other, &r) == -2);
    hs[0] = 18;
    CHECK(probe_parse_handshake(hs, ih, &r) == -1);

    /* 10 pieces: bitfield with 9 bits set, then a have for the last one. */
    memset(&r, 0, sizeof r);
    r.status = PROBE_OK;
    r.upload_only = -1;
    probe_ctx c;
    probe_ctx_init(&c, &r, 10);
    const uint8_t bitfield[] = { 5, 0xFF, 0x80 };
    probe_on_message(&c, bitfield, sizeof bitfield);
    CHECK(r.got_pieces && r.pieces_have == 9 && r.bitfield_bits == 16);
    CHECK(!probe_is_seeder(&r, 10));
    char text[64];
    probe_completion_text(&r, 10, text, sizeof text);
    CHECK_STR(text, "90.0 % (9/10)");
    const uint8_t have[] = { 4, 0, 0, 0, 9 };
    probe_on_message(&c, have, sizeof have);
    CHECK(r.pieces_have == 10 && probe_is_seeder(&r, 10));
    probe_completion_text(&r, 10, text, sizeof text);
    CHECK_STR(text, "100 % (seeder)");
    const uint8_t have_out[] = { 4, 0, 0, 0, 200 }; /* index beyond the torrent: ignored */
    probe_on_message(&c, have_out, sizeof have_out);
    CHECK(r.pieces_have == 10);

    /* Extension handshake. */
    const char ext[] = "d1:md11:ut_metadatai1ee11:upload_onlyi1e1:v15:qBittorrent 5.06:yourip4:\x01\x02\x03\x04"
                       "e";
    uint8_t msg[128];
    msg[0] = 20;
    msg[1] = 0;
    memcpy(msg + 2, ext, sizeof ext - 1);
    probe_on_message(&c, msg, sizeof ext - 1 + 2);
    CHECK(c.got_ext_handshake && r.upload_only == 1);
    CHECK_STR(r.client, "qBittorrent 5.0");
    CHECK_STR(r.yourip, "1.2.3.4");
    probe_ctx_free(&c);

    /* have_all / have_none, and unknown piece count. */
    memset(&r, 0, sizeof r);
    r.status = PROBE_OK;
    probe_ctx_init(&c, &r, 0);
    const uint8_t have_all[] = { 0x0E };
    probe_on_message(&c, have_all, 1);
    CHECK(r.have_all && probe_is_seeder(&r, 0));
    probe_ctx_free(&c);
    memset(&r, 0, sizeof r);
    r.status = PROBE_OK;
    probe_ctx_init(&c, &r, 0);
    const uint8_t bf2[] = { 5, 0xF0 };
    probe_on_message(&c, bf2, sizeof bf2);
    probe_completion_text(&r, 0, text, sizeof text);
    CHECK_STR(text, "4 pièce(s), total inconnu");
    probe_ctx_free(&c);

    /* Client guessed from an Azureus-style peer_id when there is no "v". */
    memset(&r, 0, sizeof r);
    r.has_peer_id = 1;
    memcpy(r.peer_id, "-TR4050-zzzzzzzzzzzz", 20);
    probe_client_text(&r, text, sizeof text);
    CHECK_STR(text, "Transmission (4050)");
}

static void test_misc(void)
{
    uint8_t id[20];
    tr_generate_peer_id(id);
    CHECK(memcmp(id, TR_PEER_ID_PREFIX, 8) == 0);
    char s[100];
    tr_peer_id_str((const uint8_t *)"-qB\x01", 4, s, sizeof s);
    CHECK_STR(s, "-qB%01");
    uint8_t out[2];
    CHECK(hex_decode("aBcd", out, 2) == 0 && out[0] == 0xab && out[1] == 0xcd);
    CHECK(hex_decode("abc", out, 2) < 0);
    CHECK(hex_decode("zzzz", out, 2) < 0);
    char *hd = hexdump((const uint8_t *)"ABC", 3);
    CHECK(strstr(hd, "41 42 43") && strstr(hd, "|ABC|"));
    free(hd);
    CHECK(is_printable_text((const uint8_t *)"h\xc3\xa9llo", 6));
    CHECK(!is_printable_text((const uint8_t *)"a\x01", 2));
    CHECK(!is_printable_text((const uint8_t *)"\xc3", 1));
}

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);
    test_bencode_decode();
    test_bencode_encode();
    test_info_hash();
    test_torrent_parse();
    test_compact_peers();
    test_http_response();
    test_udp_packets();
    test_udp_parse();
    test_http_url();
    test_parse_url();
    test_history();
    test_probe_messages();
    test_misc();
#ifdef _DEBUG
    if (_CrtDumpMemoryLeaks()) {
        fprintf(stderr, "fuites mémoire détectées (détail ci-dessus dans le débogueur)\n");
        failures++;
    }
#endif
    printf("%d vérifications, %d échec(s)\n", checks, failures);
    return failures ? 1 : 0;
}
