#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_FMT(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define PRINTF_FMT(fmt, args)
#endif

/* Growable byte buffer. data is always NUL-terminated when non-NULL. */
typedef struct {
    unsigned char *data;
    size_t len;
    size_t cap;
} bbuf;

void bbuf_init(bbuf *b);
int bbuf_append(bbuf *b, const void *p, size_t n);
int bbuf_putc(bbuf *b, unsigned char c);
int bbuf_printf(bbuf *b, const char *fmt, ...) PRINTF_FMT(2, 3);
/* Returns the buffer as a malloc'd string (never NULL on success) and resets b. */
char *bbuf_steal(bbuf *b);
void bbuf_free(bbuf *b);

void hex_encode(const uint8_t *in, size_t n, char *out); /* out: 2n+1 bytes */
int hex_decode(const char *in, uint8_t *out, size_t outlen); /* exact length, 0 = ok */
/* Classic hex dump ("offset  xx xx ... |ascii|"), malloc'd. */
char *hexdump(const uint8_t *data, size_t len);

void random_bytes(void *buf, size_t n);
uint32_t random_u32(void);
int64_t mono_ms(void);
int64_t mono_us(void);

char *xstrdup(const char *s);
char *xstrndup(const void *s, size_t n);
/* 1 if the bytes are printable text (valid UTF-8, no control chars except tab/newline). */
int is_printable_text(const uint8_t *s, size_t n);

/* UTF-8 <-> UTF-16 (malloc'd, invalid sequences replaced). */
wchar_t *utf8_to_wide(const char *s);
char *wide_to_utf8(const wchar_t *s);
/* Windows error message (system or the given module, e.g. "winhttp.dll") as UTF-8. */
void win_error_text(unsigned long code, const char *module, char *buf, size_t n);

void be_put16(uint8_t *p, uint16_t v);
void be_put32(uint8_t *p, uint32_t v);
void be_put64(uint8_t *p, uint64_t v);
uint16_t be_get16(const uint8_t *p);
uint32_t be_get32(const uint8_t *p);
uint64_t be_get64(const uint8_t *p);

#endif
