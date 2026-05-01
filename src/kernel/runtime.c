#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict dst = (uint8_t *)dest;
    const uint8_t *restrict source = (const uint8_t *)src;

    for (size_t i = 0; i < n; ++i) {
        dst[i] = source[i];
    }

    return dest;
}

void *memset(void *dest, int value, size_t n) {
    uint8_t *dst = (uint8_t *)dest;

    for (size_t i = 0; i < n; ++i) {
        dst[i] = (uint8_t)value;
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
