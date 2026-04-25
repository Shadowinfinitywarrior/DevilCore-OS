#ifndef _PRINTF_H
#define _PRINTF_H

#include <devil/types.h>
#include <stdarg.h>

int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
int vsprintf(char *buf, const char *fmt, va_list args);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif // _PRINTF_H