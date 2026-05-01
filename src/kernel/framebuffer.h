#ifndef DEVILCORE_FRAMEBUFFER_H
#define DEVILCORE_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "limine.h"

extern void serial_write_string(const char *str);
extern void serial_write_hex_u64(uint64_t value);
extern void serial_write_dec(uint64_t value);

struct framebuffer {
    void *address;
    void *backbuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint16_t bpp;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

void fb_init(struct limine_framebuffer_response *response);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_fill_rect_alpha(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint8_t alpha);
void fb_flip(void);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color);
void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg_color, uint32_t bg_color);
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void fb_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_copy_region(uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y, uint32_t w, uint32_t h);

// Advanced graphics functions
void fb_fill_rect_gradient_h(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color1, uint32_t color2);
void fb_fill_rect_gradient_v(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color1, uint32_t color2);
void fb_draw_rect_rounded(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t radius, uint32_t color);
void fb_fill_rect_rounded(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t radius, uint32_t color);
void fb_draw_bitmap(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *bitmap);
void fb_draw_bitmap_alpha(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *bitmap);
void fb_draw_bitmap_scaled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t src_w, uint32_t src_h, const uint32_t *bitmap);

uint32_t fb_make_color(uint8_t r, uint8_t g, uint8_t b);
uint32_t fb_blend_colors(uint32_t color1, uint32_t color2, uint8_t alpha);

extern struct framebuffer *framebuffer;
extern uint32_t fb_width;
extern uint32_t fb_height;

#endif