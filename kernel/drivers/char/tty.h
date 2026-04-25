/* kernel/drivers/char/tty.c */
#ifndef _TTY_H
#define _TTY_H

#include <devil/types.h>

void tty_init(void);
void tty_write(const char *buf, size_t len);
void tty_putchar(char c);
char tty_getchar(void);
int tty_read(char *buf, size_t count);

extern void console_putchar(char c);

#endif // _TTY_H