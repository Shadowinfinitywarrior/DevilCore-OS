/*
 * DevilCore OS — Terminal Emulator & PTY Handler
 */
#ifndef DC_TERMINAL_H
#define DC_TERMINAL_H

#include "../include/types.h"
#include "wm.h"

/* Append an ANSI text payload to a Window's text buffer */
void terminal_print(Window *win, const char *str);

/* Clear the terminal buffer */
void terminal_clear(Window *win);

#endif /* DC_TERMINAL_H */
