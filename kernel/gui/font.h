/*
 * DevilCore OS — Font Rendering Engine Header
 */
#ifndef DC_FONT_H
#define DC_FONT_H

#include "../include/types.h"

/* Character cell dimensions (native 8x8 font scaled 2x = 16x16 display) */
#define CHAR_W  16
#define CHAR_H  16
#define FONT_NATIVE_W 8
#define FONT_NATIVE_H 8

/* Render a single character at (x,y) */
void font_char(int x, int y, char c, uint32_t fg, uint32_t bg);

/* Render a null-terminated string. bg=0 means transparent. */
void font_str(int x, int y, const char *s, uint32_t fg, uint32_t bg);

/* Render string with transparent background */
void font_str_tr(int x, int y, const char *s, uint32_t fg);

/* Measure string width in pixels */
int font_str_width(const char *s);

#endif /* DC_FONT_H */
