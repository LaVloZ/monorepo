#include <windows.h>
#include <bcrypt.h>

#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif

void bbuf_init(bbuf *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int bbuf_reserve(bbuf *b, size_t extra)
{
    if (extra > SIZE_MAX - b->len - 1)
        return -1;
    size_t need = b->len + extra + 1;
    if (need <= b->cap)
        return 0;
    size_t cap = b->cap ? b->cap : 64;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            cap = need;
            break;
        }
        cap *= 2;
    }
    unsigned char *p = realloc(b->data, cap);
    if (!p)
        return -1;
    b->data = p;
    b->cap = cap;
    return 0;
}

int bbuf_append(bbuf *b, const void *p, size_t n)
{
    if (bbuf_reserve(b, n) < 0)
        return -1;
    if (n)
        memcpy(b->data + b->len, p, n);
    b->len += n;
    b->data[b->len] = 0;
    return 0;
}

int bbuf_putc(bbuf *b, unsigned char c)
{
    return bbuf_append(b, &c, 1);
}

int bbuf_printf(bbuf *b, const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || bbuf_reserve(b, (size_t)n) < 0) {
        va_end(ap2);
        return -1;
    }
    vsnprintf((char *)b->data + b->len, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    b->len += (size_t)n;
    return 0;
}

char *bbuf_steal(bbuf *b)
{
    char *s = (char *)b->data;
    if (!s)
        s = xstrdup("");
    bbuf_init(b);
    return s;
}

void bbuf_free(bbuf *b)
{
    free(b->data);
    bbuf_init(b);
}

void hex_encode(const uint8_t *in, size_t n, char *out)
{
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = digits[in[i] >> 4];
        out[2 * i + 1] = digits[in[i] & 15];
    }
    out[2 * n] = 0;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int hex_decode(const char *in, uint8_t *out, size_t outlen)
{
    if (strlen(in) != outlen * 2)
        return -1;
    for (size_t i = 0; i < outlen; i++) {
        int hi = hexval(in[2 * i]), lo = hexval(in[2 * i + 1]);
        if (hi < 0 || lo < 0)
            return -1;
        out[i] = (uint8_t)(hi << 4 | lo);
    }
    return 0;
}

char *hexdump(const uint8_t *d, size_t len)
{
    bbuf b;
    bbuf_init(&b);
    if (len == 0)
        bbuf_printf(&b, "(vide)\n");
    for (size_t off = 0; off < len; off += 16) {
        bbuf_printf(&b, "%06zx  ", off);
        for (size_t i = 0; i < 16; i++) {
            if (off + i < len)
                bbuf_printf(&b, "%02x ", d[off + i]);
            else
                bbuf_append(&b, "   ", 3);
            if (i == 7)
                bbuf_putc(&b, ' ');
        }
        bbuf_append(&b, " |", 2);
        for (size_t i = 0; i < 16 && off + i < len; i++) {
            unsigned char c = d[off + i];
            bbuf_putc(&b, (c >= 0x20 && c < 0x7f) ? c : '.');
        }
        bbuf_append(&b, "|\n", 2);
    }
    return bbuf_steal(&b);
}

void random_bytes(void *buf, size_t n)
{
    unsigned char *p = buf;
    while (n > 0) {
        ULONG chunk = n > 0x10000 ? 0x10000 : (ULONG)n;
        if (!BCRYPT_SUCCESS(BCryptGenRandom(NULL, p, chunk, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
            /* Degraded fallback, should never happen. */
            uint64_t x = (uint64_t)mono_us() ^ (uintptr_t)p ^ GetCurrentProcessId();
            for (ULONG i = 0; i < chunk; i++) {
                x ^= x << 13;
                x ^= x >> 7;
                x ^= x << 17;
                p[i] = (unsigned char)x;
            }
        }
        p += chunk;
        n -= chunk;
    }
}

uint32_t random_u32(void)
{
    uint32_t v;
    random_bytes(&v, sizeof v);
    return v;
}

int64_t mono_us(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (int64_t)(now.QuadPart / freq.QuadPart * 1000000 + now.QuadPart % freq.QuadPart * 1000000 / freq.QuadPart);
}

wchar_t *utf8_to_wide(const char *s)
{
    if (!s)
        s = "";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w = malloc((size_t)(n > 0 ? n : 1) * sizeof *w);
    if (!w)
        return NULL;
    if (n <= 0 || !MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n))
        w[0] = 0;
    return w;
}

char *wide_to_utf8(const wchar_t *s)
{
    if (!s)
        s = L"";
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    char *u = malloc((size_t)(n > 0 ? n : 1));
    if (!u)
        return NULL;
    if (n <= 0 || !WideCharToMultiByte(CP_UTF8, 0, s, -1, u, n, NULL, NULL))
        u[0] = 0;
    return u;
}

void win_error_text(unsigned long code, const char *module, char *buf, size_t n)
{
    wchar_t *msg = NULL;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    HMODULE mod = NULL;
    if (module) {
        mod = GetModuleHandleA(module);
        if (mod)
            flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    DWORD len = FormatMessageW(flags, mod, code, 0, (LPWSTR)&msg, 0, NULL);
    if (!len || !msg) {
        snprintf(buf, n, "erreur %lu", code);
        return;
    }
    while (len && (msg[len - 1] == L'\r' || msg[len - 1] == L'\n' || msg[len - 1] == L' ' || msg[len - 1] == L'.'))
        msg[--len] = 0;
    char *u = wide_to_utf8(msg);
    snprintf(buf, n, "%s (%lu)", u ? u : "?", code);
    free(u);
    LocalFree(msg);
}

int64_t mono_ms(void)
{
    return mono_us() / 1000;
}

char *xstrdup(const char *s)
{
    if (!s)
        return NULL;
    return xstrndup(s, strlen(s));
}

char *xstrndup(const void *s, size_t n)
{
    if (!s)
        return NULL;
    char *p = malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

int is_printable_text(const uint8_t *s, size_t n)
{
    size_t i = 0;
    while (i < n) {
        uint8_t c = s[i];
        if (c < 0x80) {
            if ((c < 0x20 && c != '\t' && c != '\n' && c != '\r') || c == 0x7f)
                return 0;
            i++;
            continue;
        }
        size_t extra;
        if ((c & 0xe0) == 0xc0 && c >= 0xc2)
            extra = 1;
        else if ((c & 0xf0) == 0xe0)
            extra = 2;
        else if ((c & 0xf8) == 0xf0 && c <= 0xf4)
            extra = 3;
        else
            return 0;
        if (i + extra >= n)
            return 0;
        for (size_t k = 1; k <= extra; k++)
            if ((s[i + k] & 0xc0) != 0x80)
                return 0;
        i += extra + 1;
    }
    return 1;
}

void be_put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

void be_put32(uint8_t *p, uint32_t v)
{
    for (int i = 3; i >= 0; i--) {
        p[i] = (uint8_t)v;
        v >>= 8;
    }
}

void be_put64(uint8_t *p, uint64_t v)
{
    for (int i = 7; i >= 0; i--) {
        p[i] = (uint8_t)v;
        v >>= 8;
    }
}

uint16_t be_get16(const uint8_t *p)
{
    return (uint16_t)(p[0] << 8 | p[1]);
}

uint32_t be_get32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

uint64_t be_get64(const uint8_t *p)
{
    return (uint64_t)be_get32(p) << 32 | be_get32(p + 4);
}
