/*
 * DevilCore OS — String & Memory Utilities Implementation
 */
#include "string.h"

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t*)a, *q = (const uint8_t*)b;
    while (n--) { if (*p != *q) return *p - *q; p++; q++; }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    if (n == 0) return 0;
    while (--n && *a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *d, const char *s) {
    char *r = d;
    while ((*d++ = *s++));
    return r;
}

char *strncpy(char *d, const char *s, size_t n) {
    char *r = d;
    while (n && (*d++ = *s++)) n--;
    while (n--) *d++ = 0;
    return r;
}

char *strcat(char *d, const char *s) {
    char *r = d;
    while (*d) d++;
    while ((*d++ = *s++));
    return r;
}

const char *strchr(const char *s, int c) {
    while (*s) { if (*s == (char)c) return s; s++; }
    return NULL;
}

void itoa(int32_t n, char *buf, int base) {
    char tmp[32];
    int i = 0, neg = 0;
    if (n < 0 && base == 10) { neg = 1; n = -n; }
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    while (n) { tmp[i++] = "0123456789ABCDEF"[n % base]; n /= base; }
    if (neg) tmp[i++] = '-';
    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

void utoa(uint32_t n, char *buf, int base) {
    char tmp[32];
    int i = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    while (n) { tmp[i++] = "0123456789ABCDEF"[n % base]; n /= base; }
    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

/* Minimal snprintf supporting: %s %d %u %x %% %c */
int snprintf(char *buf, size_t size, const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);

    size_t pos = 0;
    #define PUTC(ch) do { if (pos < size-1) buf[pos] = (ch); pos++; } while(0)

    while (*fmt && pos < size - 1) {
        if (*fmt != '%') { PUTC(*fmt++); continue; }
        fmt++; /* skip '%' */
        switch (*fmt) {
            case 's': {
                const char *s = __builtin_va_arg(ap, const char*);
                if (!s) s = "(null)";
                while (*s) PUTC(*s++);
                break;
            }
            case 'd': {
                int32_t v = __builtin_va_arg(ap, int32_t);
                char tmp[16]; itoa(v, tmp, 10);
                for (int i = 0; tmp[i]; i++) PUTC(tmp[i]);
                break;
            }
            case 'u': {
                uint32_t v = __builtin_va_arg(ap, uint32_t);
                char tmp[16]; utoa(v, tmp, 10);
                for (int i = 0; tmp[i]; i++) PUTC(tmp[i]);
                break;
            }
            case 'x': {
                uint32_t v = __builtin_va_arg(ap, uint32_t);
                char tmp[16]; utoa(v, tmp, 16);
                for (int i = 0; tmp[i]; i++) PUTC(tmp[i]);
                break;
            }
            case 'c': {
                char c = (char)__builtin_va_arg(ap, int);
                PUTC(c);
                break;
            }
            case '%': PUTC('%'); break;
            default: PUTC('%'); PUTC(*fmt); break;
        }
        fmt++;
    }
    #undef PUTC

    if (size > 0) buf[MIN(pos, size-1)] = 0;
    __builtin_va_end(ap);
    return (int)pos;
}
