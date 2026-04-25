// kernel/lib/printf.c

#include <devil/types.h>
#include <devil/printf.h>
#include <devil/string.h>
#include <asm/io.h>

static char digit[] = "0123456789abcdef";

static void print_string(const char *s) {
    while (*s) {
        outb(0xE9, *s++);
    }
}

static void print_char(char c) {
    if (c == '\n') {
        outb(0xE9, '\r');
    }
    outb(0xE9, c);
}

static void print_number(long long value, int base, int sign) {
    char buf[32];
    int i = 0;
    int neg = 0;
    unsigned long long uvalue;
    
    if (sign && value < 0) {
        neg = 1;
        uvalue = -value;
    } else {
        uvalue = value;
    }
    
    if (uvalue == 0) {
        buf[i++] = '0';
    } else {
        while (uvalue > 0) {
            buf[i++] = digit[uvalue % base];
            uvalue /= base;
        }
    }
    
    if (neg) {
        print_char('-');
    }
    
    while (i-- > 0) {
        print_char(buf[i]);
    }
}

int vprintf(const char *fmt, va_list args) {
    int count = 0;
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'c': {
                    char c = va_arg(args, int);
                    print_char(c);
                    count++;
                    break;
                }
                case 's': {
                    const char *s = va_arg(args, char*);
                    print_string(s);
                    count += strlen(s);
                    break;
                }
                case 'd':
                case 'i': {
                    long long val = va_arg(args, int);
                    print_number(val, 10, 1);
                    break;
                }
                case 'u': {
                    unsigned long long val = va_arg(args, unsigned int);
                    print_number(val, 10, 0);
                    break;
                }
                case 'x': {
                    unsigned long long val = va_arg(args, unsigned int);
                    print_number(val, 16, 0);
                    break;
                }
                case 'p': {
                    void *ptr = va_arg(args, void*);
                    print_string("0x");
                    print_number((unsigned long long)ptr, 16, 0);
                    break;
                }
                case '%': {
                    print_char('%');
                    break;
                }
                default: {
                    print_char(*fmt);
                    break;
                }
            }
        } else {
            print_char(*fmt);
        }
        fmt++;
    }
    
    return count;
}

int vsprintf(char *buf, const char *fmt, va_list args) {
    char *p = buf;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'c': {
                    *p++ = (char)va_arg(args, int);
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char*);
                    if (!s) s = "(null)";
                    while (*s) *p++ = *s++;
                    break;
                }
                case 'd':
                case 'i': {
                    long long val = va_arg(args, int);
                    char tmp[32];
                    int i = 0;
                    if (val < 0) { *p++ = '-'; val = -val; }
                    if (val == 0) { tmp[i++] = '0'; }
                    else {
                        while (val > 0) { tmp[i++] = digit[val % 10]; val /= 10; }
                    }
                    while (i-- > 0) *p++ = tmp[i];
                    break;
                }
                case 'x': {
                    unsigned long long val = va_arg(args, unsigned int);
                    char tmp[32];
                    int i = 0;
                    if (val == 0) { tmp[i++] = '0'; }
                    else {
                        while (val > 0) { tmp[i++] = digit[val % 16]; val /= 16; }
                    }
                    while (i-- > 0) *p++ = tmp[i];
                    break;
                }
                case '%': *p++ = '%'; break;
                default: *p++ = *fmt; break;
            }
        } else {
            *p++ = *fmt;
        }
        fmt++;
    }
    *p = '\0';
    return (int)(p - buf);
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}