/*
 * DevilCore OS — Terminal Emulator
 */
#include "terminal.h"
#include "../libc/string.h"

void terminal_print(Window *win, const char *str) {
    if (!win) return;
    int slen = strlen(str);
    if (win->text_len + slen >= (int)sizeof(win->text_buf) - 2) {
        /* Simple scroll / buffer shift */
        int shift = win->text_len / 2;
        memmove(win->text_buf, win->text_buf + shift, win->text_len - shift);
        win->text_len -= shift;
        win->text_buf[win->text_len] = '\0';
    }
    strcat(win->text_buf, str);
    win->text_len += slen;
    win->dirty = 1;
}

void terminal_clear(Window *win) {
    if (!win) return;
    win->text_buf[0] = '\0';
    win->text_len = 0;
    win->dirty = 1;
}
