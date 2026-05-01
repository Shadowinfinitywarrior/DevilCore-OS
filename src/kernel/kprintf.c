#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

static void uint_to_str(uint64_t value, char *buf, int base) {
    const char digits[] = "0123456789ABCDEF";
    char temp[64];
    int i = 0;
    
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    while (value > 0) {
        temp[i++] = digits[value % base];
        value /= base;
    }
    
    for (int j = 0; j < i; ++j) {
        buf[j] = temp[i - j - 1];
    }
    buf[i] = '\0';
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char *p = buf;
    const char *f = fmt;
    
    while (*f) {
        if (*f == '%') {
            ++f;
            if (*f == 'u') {
                uint64_t val = va_arg(args, uint64_t);
                char num[32];
                uint_to_str(val, num, 10);
                char *n = num;
                while (*n) { *p++ = *n; n++; }
            } else if (*f == 'd' || *f == 'i') {
                int64_t sval = va_arg(args, int64_t);
                if (sval < 0) {
                    *p++ = '-';
                    sval = -sval;
                }
                char num[32];
                uint_to_str((uint64_t)sval, num, 10);
                char *n = num;
                while (*n) { *p++ = *n; n++; }
            } else if (*f == 'x') {
                uint64_t val = va_arg(args, uint64_t);
                *p++ = '0';
                *p++ = 'x';
                char num[32];
                uint_to_str(val, num, 16);
                char *n = num;
                while (*n) { *p++ = *n; n++; }
            } else if (*f == 's') {
                char *str = va_arg(args, char *);
                if (str == NULL) str = "(null)";
                while (*str) { *p++ = *str; str++; }
            } else if (*f == 'c') {
                int c = va_arg(args, int);
                *p++ = (char)c;
            } else {
                *p++ = '%';
                if (*f) { *p++ = *f; }
            }
            ++f;
        } else {
            *p++ = *f++;
        }
    }
    
    *p = '\0';
    va_end(args);
    return (int)(p - buf);
}
