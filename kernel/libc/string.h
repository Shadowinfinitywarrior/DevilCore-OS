/*
 * DevilCore OS — String & Memory Utilities Header
 */
#ifndef DC_STRING_H
#define DC_STRING_H

#include "../include/types.h"

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int   strcmp(const char *a, const char *b);
int   strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *d, const char *s);
char *strncpy(char *d, const char *s, size_t n);
char *strcat(char *d, const char *s);
const char *strchr(const char *s, int c);
void  itoa(int32_t n, char *buf, int base);
void  utoa(uint32_t n, char *buf, int base);
int   snprintf(char *buf, size_t size, const char *fmt, ...);

#endif /* DC_STRING_H */
