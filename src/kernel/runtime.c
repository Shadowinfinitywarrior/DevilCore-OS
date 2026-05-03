#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint64_t *d64 = (uint64_t *)dest;
    const uint64_t *s64 = (const uint64_t *)src;
    
    size_t n64 = n / 64; // 8 * 8-byte blocks
    for (size_t i = 0; i < n64; i++) {
        d64[0] = s64[0];
        d64[1] = s64[1];
        d64[2] = s64[2];
        d64[3] = s64[3];
        d64[4] = s64[4];
        d64[5] = s64[5];
        d64[6] = s64[6];
        d64[7] = s64[7];
        d64 += 8;
        s64 += 8;
    }
    
    // Remaining 8-byte blocks
    n64 = (n % 64) / 8;
    for (size_t i = 0; i < n64; i++) {
        *d64++ = *s64++;
    }
    
    uint8_t *d8 = (uint8_t *)d64;
    const uint8_t *s8 = (const uint8_t *)s64;
    for (size_t i = 0; i < n % 8; i++) {
        d8[i] = s8[i];
    }

    return dest;
}

void *memset(void *dest, int value, size_t n) {
    uint8_t v8 = (uint8_t)value;
    uint64_t v64 = (uint64_t)v8 | ((uint64_t)v8 << 8) | ((uint64_t)v8 << 16) | ((uint64_t)v8 << 24) |
                   ((uint64_t)v8 << 32) | ((uint64_t)v8 << 40) | ((uint64_t)v8 << 48) | ((uint64_t)v8 << 56);
                   
    uint64_t *d64 = (uint64_t *)dest;
    size_t n64 = n / 64;
    for (size_t i = 0; i < n64; i++) {
        d64[0] = v64;
        d64[1] = v64;
        d64[2] = v64;
        d64[3] = v64;
        d64[4] = v64;
        d64[5] = v64;
        d64[6] = v64;
        d64[7] = v64;
        d64 += 8;
    }
    
    n64 = (n % 64) / 8;
    for (size_t i = 0; i < n64; i++) {
        *d64++ = v64;
    }
    
    uint8_t *d8 = (uint8_t *)d64;
    for (size_t i = 0; i < n % 8; i++) {
        d8[i] = v8;
    }

    return dest;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *dst = (uint8_t *)dest;
    const uint8_t *source = (const uint8_t *)src;

    if (dst == source || n == 0) {
        return dest;
    }

    if (dst < source) {
        for (size_t i = 0; i < n; ++i) {
            dst[i] = source[i];
        }
    } else {
        for (size_t i = n; i > 0; --i) {
            dst[i - 1] = source[i - 1];
        }
    }

    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t n) {
    const uint8_t *left = (const uint8_t *)lhs;
    const uint8_t *right = (const uint8_t *)rhs;

    for (size_t i = 0; i < n; ++i) {
        if (left[i] != right[i]) {
            return (int)left[i] - (int)right[i];
        }
    }

    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    while (i < n) {
        dest[i] = '\0';
        ++i;
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return (int)s1[i] - (int)s2[i];
        }
        ++i;
    }
    return (int)s1[i] - (int)s2[i];
}

int strcasecmp(const char *s1, const char *s2) {
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        char c1 = s1[i];
        char c2 = s2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) {
            return (int)c1 - (int)c2;
        }
        ++i;
    }
    char c1 = s1[i];
    char c2 = s2[i];
    if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
    if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
    return (int)c1 - (int)c2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (s1[i] != s2[i]) {
            return (int)s1[i] - (int)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

char *strcat(char *dest, const char *src) {
    size_t i = 0;
    while (dest[i] != '\0') {
        ++i;
    }
    size_t j = 0;
    while (src[j] != '\0') {
        dest[i + j] = src[j];
        ++j;
    }
    dest[i + j] = '\0';
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (dest[i] != '\0') {
        ++i;
    }
    size_t j = 0;
    while (j < n && src[j] != '\0') {
        dest[i + j] = src[j];
        ++j;
    }
    dest[i + j] = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return NULL;
        }
    }
    return (char *)s;
}
