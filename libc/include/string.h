# libc/include/string.h
#ifndef _STRING_H
#define _STRING_H

#include <devil/types.h>

void *memcpy(void *dest, const void *src, size_t count);
void *memmove(void *dest, const void *src, size_t count);
void *memset(void *dest, int val, size_t count);
int memcmp(const void *buf1, const void *buf2, size_t count);
void *memchr(const void *buf, int ch, size_t count);

size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t count);
char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);

#endif // _STRING_H