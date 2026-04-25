/*
 * DevilCore OS — Font Rendering Engine
 * Uses the 8x8 bitmap font from font8x8.h, rendered with correct
 * bit ordering (MSB-first = left-to-right) and 2x scaling for crisp
 * 16x16 character cells on 1024x768.
 */
#include "font.h"
#include "framebuffer.h"
#include "../include/font8x8.h"

void font_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    int idx = (uint8_t)c;
    if (idx >= 128) idx = '?';
    if (idx < 32)   idx = ' ';
    const unsigned char *glyph = font8x8[idx];

    for (int row = 0; row < FONT_NATIVE_H; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < FONT_NATIVE_W; col++) {
            /* MSB-first: bit 7 is leftmost pixel */
            uint32_t clr = (bits & (0x80 >> col)) ? fg : bg;
            /* 2x scale: each font pixel becomes a 2x2 block */
            int px = x + col * 2;
            int py = y + row * 2;
            if (clr != 0) {
                fb_pixel(px,     py,     clr);
                fb_pixel(px + 1, py,     clr);
                fb_pixel(px,     py + 1, clr);
                fb_pixel(px + 1, py + 1, clr);
            }
        }
    }
}

void font_str(int x, int y, const char *s, uint32_t fg, uint32_t bg) {
    int ox = x;
    while (*s) {
        if (*s == '\n') { x = ox; y += CHAR_H; s++; continue; }
        font_char(x, y, *s, fg, bg);
        x += CHAR_W;
        s++;
    }
}

void font_str_tr(int x, int y, const char *s, uint32_t fg) {
    font_str(x, y, s, fg, 0);
}

int font_str_width(const char *s) {
    int w = 0, max_w = 0;
    while (*s) {
        if (*s == '\n') { if (w > max_w) max_w = w; w = 0; }
        else w += CHAR_W;
        s++;
    }
    return MAX(w, max_w);
}
