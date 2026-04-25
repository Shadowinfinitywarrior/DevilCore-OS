// kernel/lib/string.c

#include <devil/types.h>
#include <devil/string.h>

void *memcpy(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    return dest;
}

void *memset(void *dest, int val, size_t count) {
    uint8_t *d = dest;
    while (count--) {
        *d++ = (uint8_t)val;
    }
    return dest;
}

int memcmp(const void *buf1, const void *buf2, size_t count) {
    const uint8_t *b1 = buf1;
    const uint8_t *b2 = buf2;
    while (count--) {
        if (*b1 != *b2) {
            return *b1 - *b2;
        }
        b1++;
        b2++;
    }
    return 0;
}

void *memchr(const void *buf, int ch, size_t count) {
    const uint8_t *b = buf;
    while (count--) {
        if (*b == (uint8_t)ch) {
            return (void*)b;
        }
        b++;
    }
    return NULL;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++)) {}
    return dest;
}

char *strncpy(char *dest, const char *src, size_t count) {
    char *d = dest;
    size_t i = 0;
    while ((*d++ = *src++) && i++ < count) {}
    while (i++ < count) {
        *d++ = 0;
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t count) {
    size_t i = 0;
    while (*s1 && *s2 && i < count) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
        i++;
    }
    if (i == count) {
        return 0;
    }
    return *s1 - *s2;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) {
        d++;
    }
    while ((*d++ = *src++)) {}
    return dest;
}

char *strncat(char *dest, const char *src, size_t count) {
    char *d = dest;
    while (*d) {
        d++;
    }
    size_t i = 0;
    while ((*d++ = *src++) && i++ < count) {}
    return dest;
}

char *strchr(const char *str, int ch) {
    while (*str) {
        if (*str == (char)ch) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

char *strrchr(const char *str, int ch) {
    const char *end = str;
    while (*end) {
        end++;
    }
    while (end != str) {
        if (*end == (char)ch) {
            return (char*)end;
        }
        end--;
    }
    return NULL;
}