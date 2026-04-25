/*
 * DevilCore OS — Double-Buffered Framebuffer Engine
 * Renders everything to a RAM back-buffer first, then memcpy to VRAM.
 * Eliminates all screen tearing and flicker.
 */
#include "framebuffer.h"
#include "../libc/string.h"
#include "../include/logo_data.h"
#include "../mm/heap.h"

/* Framebuffer hardware state */
static volatile uint8_t *g_vram = (volatile uint8_t*)0;
static uint32_t g_pitch = 0, g_w = 0, g_h = 0;
static uint8_t  g_bpp = 0;
static uint8_t  g_rpos = 16, g_gpos = 8, g_bpos = 0;

/* Dynamically allocated RAM back-buffer */
static uint8_t *g_backbuf = NULL;

void fb_init(volatile uint8_t *addr, uint32_t pitch, uint32_t w, uint32_t h,
             uint8_t bpp, uint8_t rpos, uint8_t gpos, uint8_t bpos) {
    g_vram = addr; g_pitch = pitch; g_w = w; g_h = h;
    g_bpp = bpp; g_rpos = rpos; g_gpos = gpos; g_bpos = bpos;

    /* Allocate back-buffer dynamically */
    uint32_t buf_size = g_w * g_h * 4;
    g_backbuf = (uint8_t *)kmalloc_aligned(buf_size, 64);
    
    if (g_backbuf) {
        memset(g_backbuf, 0, buf_size);
    }
}

uint32_t fb_width(void)  { return g_w; }
uint32_t fb_height(void) { return g_h; }

uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << g_rpos) |
           ((uint32_t)g << g_gpos) |
           ((uint32_t)b << g_bpos);
}

void fb_pixel(int x, int y, uint32_t colour) {
    if ((uint32_t)x >= g_w || (uint32_t)y >= g_h) return;
    /* Write to back buffer (always 32bpp internally) */
    uint32_t *px = (uint32_t*)(g_backbuf + (uint32_t)y * (g_w * 4) + (uint32_t)x * 4);
    *px = colour;
}

void fb_fill(int x, int y, int w, int h, uint32_t colour) {
    /* Clip to screen bounds */
    int x0 = MAX(x, 0), y0 = MAX(y, 0);
    int x1 = MIN(x + w, (int)g_w), y1 = MIN(y + h, (int)g_h);
    if (x0 >= x1 || y0 >= y1) return;
    int rw = x1 - x0;

    for (int row = y0; row < y1; row++) {
        uint32_t *dst = (uint32_t*)(g_backbuf + (uint32_t)row * (g_w * 4) + (uint32_t)x0 * 4);
        for (int i = 0; i < rw; i++) dst[i] = colour;
    }
}

void fb_rect(int x, int y, int w, int h, uint32_t colour) {
    fb_hline(x, y, w, colour);
    fb_hline(x, y + h - 1, w, colour);
    fb_vline(x, y, h, colour);
    fb_vline(x + w - 1, y, h, colour);
}

void fb_hline(int x, int y, int len, uint32_t colour) {
    for (int i = 0; i < len; i++) fb_pixel(x + i, y, colour);
}

void fb_vline(int x, int y, int len, uint32_t colour) {
    for (int i = 0; i < len; i++) fb_pixel(x, y + i, colour);
}

void fb_gradient(int x, int y, int w, int h,
                 uint8_t r1, uint8_t g1, uint8_t b1,
                 uint8_t r2, uint8_t g2, uint8_t b2) {
    for (int row = 0; row < h; row++) {
        uint8_t r = (uint8_t)((int)r1 + ((int)r2 - (int)r1) * row / h);
        uint8_t g = (uint8_t)((int)g1 + ((int)g2 - (int)g1) * row / h);
        uint8_t b = (uint8_t)((int)b1 + ((int)b2 - (int)b1) * row / h);
        uint32_t c = rgb(r, g, b);
        int x0 = MAX(x, 0), x1 = MIN(x + w, (int)g_w);
        int ry = y + row;
        if ((uint32_t)ry >= g_h) break;
        uint32_t *dst = (uint32_t*)(g_backbuf + (uint32_t)ry * (g_w * 4) + (uint32_t)x0 * 4);
        for (int col = x0; col < x1; col++) *dst++ = c;
    }
}

void fb_rounded_rect(int x, int y, int w, int h, int r, uint32_t colour) {
    /* Draw rectangle with rounded corners using circle quadrant exclusion */
    fb_hline(x + r, y, w - 2*r, colour);       /* top */
    fb_hline(x + r, y + h - 1, w - 2*r, colour); /* bottom */
    fb_vline(x, y + r, h - 2*r, colour);       /* left */
    fb_vline(x + w - 1, y + r, h - 2*r, colour); /* right */

    /* Quarter-circle corners using midpoint circle algorithm */
    int cx, cy, d;
    d = 3 - 2 * r;
    cx = 0; cy = r;
    while (cx <= cy) {
        /* Top-left */
        fb_pixel(x + r - cx, y + r - cy, colour);
        fb_pixel(x + r - cy, y + r - cx, colour);
        /* Top-right */
        fb_pixel(x + w - 1 - r + cx, y + r - cy, colour);
        fb_pixel(x + w - 1 - r + cy, y + r - cx, colour);
        /* Bottom-left */
        fb_pixel(x + r - cx, y + h - 1 - r + cy, colour);
        fb_pixel(x + r - cy, y + h - 1 - r + cx, colour);
        /* Bottom-right */
        fb_pixel(x + w - 1 - r + cx, y + h - 1 - r + cy, colour);
        fb_pixel(x + w - 1 - r + cy, y + h - 1 - r + cx, colour);

        if (d < 0) { d += 4 * cx + 6; }
        else       { d += 4 * (cx - cy) + 10; cy--; }
        cx++;
    }
}

void fb_alpha_fill(int x, int y, int w, int h, uint32_t colour, uint8_t alpha) {
    uint8_t sr = (colour >> g_rpos) & 0xFF;
    uint8_t sg = (colour >> g_gpos) & 0xFF;
    uint8_t sb = (colour >> g_bpos) & 0xFF;

    int x0 = MAX(x, 0), y0 = MAX(y, 0);
    int x1 = MIN(x + w, (int)g_w), y1 = MIN(y + h, (int)g_h);

    for (int row = y0; row < y1; row++) {
        uint32_t *dst = (uint32_t*)(g_backbuf + (uint32_t)row * (g_w * 4) + (uint32_t)x0 * 4);
        for (int col = x0; col < x1; col++) {
            uint32_t bg = *dst;
            uint8_t dr = (bg >> g_rpos) & 0xFF;
            uint8_t dg = (bg >> g_gpos) & 0xFF;
            uint8_t db = (bg >> g_bpos) & 0xFF;
            uint8_t nr = (uint8_t)(((int)sr * alpha + (int)dr * (255 - alpha)) / 255);
            uint8_t ng = (uint8_t)(((int)sg * alpha + (int)dg * (255 - alpha)) / 255);
            uint8_t nb = (uint8_t)(((int)sb * alpha + (int)db * (255 - alpha)) / 255);
            *dst++ = rgb(nr, ng, nb);
        }
    }
}

void fb_blit_logo(int x, int y, int scale) {
    /* Logo pixel data is stored as 0x00RRGGBB in logo_data.h */
    extern const unsigned int logo_pixels[];
    if (scale < 1) scale = 1;
    for (int ly = 0; ly < LOGO_H; ly++) {
        for (int lx = 0; lx < LOGO_W; lx++) {
            uint32_t raw = logo_pixels[ly * LOGO_W + lx];
            uint8_t r = (raw >> 16) & 0xFF;
            uint8_t g_c = (raw >> 8) & 0xFF;
            uint8_t b = raw & 0xFF;
            /* Skip near-background pixels for transparency effect */
            if (r < 0x10 && g_c < 0x10 && b < 0x20) continue;
            uint32_t col = rgb(r, g_c, b);
            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++)
                    fb_pixel(x + lx * scale + sx, y + ly * scale + sy, col);
        }
    }
}

void fb_flip(void) {
    if (!g_vram || !g_backbuf || g_w == 0 || g_h == 0) return;

    for (uint32_t row = 0; row < g_h; row++) {
        uint8_t *dst = (uint8_t *)(g_vram + row * g_pitch);
        uint8_t *src = (uint8_t *)(g_backbuf + row * g_w * 4);
        
        if (g_bpp == 32) {
            uint32_t *d32 = (uint32_t *)dst;
            uint32_t *s32 = (uint32_t *)src;
            for (uint32_t i = 0; i < g_w; i++) d32[i] = s32[i];
        } else if (g_bpp == 24) {
            for (uint32_t col = 0; col < g_w; col++) {
                uint32_t c = ((uint32_t *)src)[col];
                dst[col*3 + 0] = c & 0xFF;
                dst[col*3 + 1] = (c >> 8) & 0xFF;
                dst[col*3 + 2] = (c >> 16) & 0xFF;
            }
        } else if (g_bpp == 16) {
            uint16_t *d16 = (uint16_t *)dst;
            uint32_t *s32 = (uint32_t *)src;
            for (uint32_t i = 0; i < g_w; i++) d16[i] = (uint16_t)(s32[i] & 0xFFFF);
        }
    }
}

