# libc/include/stdio.h
#ifndef _STDIO_H
#define _STDIO_H

#include <devil/types.h>

#define EOF (-1)
#define BUFSIZ  1024

typedef struct FILE {
    int fd;
    int flags;
    char buffer[1024];
    int pos;
    int len;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);

int fflush(FILE *f);
int fclose(FILE *f);
FILE *fopen(const char *path, const char *mode);
char *fgets(char *s, int n, FILE *f);
int fputs(const char *s, FILE *f);
int fread(void *ptr, size_t size, size_t nmemb, FILE *f);
int fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f);
int fseek(FILE *f, long offset, int whence);
long ftell(FILE *f);

int puts(const char *s);
int putchar(int c);
int putc(int c, FILE *f);

#endif // _STDIO_H