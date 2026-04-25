# libc/include/stdlib.h
#ifndef _STDLIB_H
#define _STDLIB_H

#include <devil/types.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void *memalign(size_t alignment, size_t size);

void abort(void);
int atexit(void (*func)(void));
void exit(int status);
void _exit(int status);

int system(const char *command);
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

int rand(void);
void srand(unsigned int seed);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
double atof(const char *nptr);

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));

#define RAND_MAX 2147483647

#endif // _STDLIB_H