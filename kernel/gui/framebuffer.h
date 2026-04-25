/*
 * DevilCore OS — Framebuffer & Drawing Engine Header
 */
#ifndef DC_FRAMEBUFFER_H
#define DC_FRAMEBUFFER_H

#include "../include/types.h"

/* Framebuffer initialization (called after MB2 parse) */
void fb_init(volatile uint8_t *addr, uint32_t pitch, uint32_t w, uint32_t h,
             uint8_t bpp, uint8_t rpos, uint8_t gpos, uint8_t bpos);

/* Core accessors */
uint32_t fb_width(void);
uint32_t fb_height(void);

/* Colour helpers */
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

/* Pixel + primitive operations (draw to back buffer) */
void fb_pixel(int x, int y, uint32_t colour);
void fb_fill(int x, int y, int w, int h, uint32_t colour);
void fb_rect(int x, int y, int w, int h, uint32_t colour);
void fb_hline(int x, int y, int len, uint32_t colour);
void fb_vline(int x, int y, int len, uint32_t colour);
void fb_gradient(int x, int y, int w, int h,
                 uint8_t r1, uint8_t g1, uint8_t b1,
                 uint8_t r2, uint8_t g2, uint8_t b2);
void fb_rounded_rect(int x, int y, int w, int h, int r, uint32_t colour);
void fb_alpha_fill(int x, int y, int w, int h, uint32_t colour, uint8_t alpha);

/* Blit embedded logo at (x,y) with given scale factor */
void fb_blit_logo(int x, int y, int scale);

/* Double buffer: flush back buffer to VRAM */
void fb_flip(void);

/* Theme colour constants (pre-packed hex, use COL() to convert) */
#define C_BG         0x080810
#define C_WIN_BG     0x10101E
#define C_WIN_BORDER 0x4040CC
#define C_TITLEBAR   0x141430
#define C_TASKBAR    0x06060F
#define C_TEXT       0xD0D0FF
#define C_TEXT_DIM   0x606080
#define C_ACCENT     0x00FFCC
#define C_ACCENT2    0xFF4488
#define C_ICON_BG    0x1C1C40
#define C_ICON_HL    0x303080
#define C_BTN        0x1E1E48
#define C_BTN_HL     0x3030AA
#define C_CLOSE      0xCC2244
#define C_SCROLLBAR  0x202040
#define C_CURSOR     0x00FFCC
#define C_GRID       0x0F0F22
#define C_GREEN      0x00FF88
#define C_RED        0xFF3344
#define C_YELLOW     0xFFCC00
#define C_ORANGE     0xFF8800

#define COL(hex) rgb(((hex)>>16)&0xFF, ((hex)>>8)&0xFF, (hex)&0xFF)

#endif /* DC_FRAMEBUFFER_H */
