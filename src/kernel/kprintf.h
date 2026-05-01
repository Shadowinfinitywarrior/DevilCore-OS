#ifndef DEVILCORE_KPRINTF_H
#define DEVILCORE_KPRINTF_H

#include <stdint.h>
#include <stdarg.h>

int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
void kprintf(const char *fmt, ...);

#endif
