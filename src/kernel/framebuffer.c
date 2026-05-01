#include "framebuffer.h"
#include <string.h>
#include "memory.h"

struct framebuffer *framebuffer;

// Standard 8x8 Font Data (95 characters, from space to ~)
const uint8_t font_data[128][8] = {
    [0x20] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
    [0x21] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00}, // !
    [0x22] = {0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    [0x23] = {0x36, 0x36, 0x7f, 0x36, 0x7f, 0x36, 0x36, 0x00}, // #
    [0x24] = {0x0c, 0x3e, 0x03, 0x1e, 0x60, 0x3e, 0x0c, 0x00}, // $
    [0x25] = {0x00, 0x63, 0x66, 0x0c, 0x18, 0x33, 0x63, 0x00}, // %
    [0x26] = {0x1c, 0x36, 0x1c, 0x3b, 0x6e, 0x66, 0x3b, 0x00}, // &
    [0x27] = {0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    [0x28] = {0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00}, // (
    [0x29] = {0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00}, // )
    [0x2A] = {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00}, // *
    [0x2B] = {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}, // +
    [0x2C] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x0c}, // ,
    [0x2D] = {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}, // -
    [0x2E] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    [0x2F] = {0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00}, // /
    [0x30] = {0x3e, 0x61, 0x61, 0x61, 0x61, 0x61, 0x3e, 0x00}, // 0
    [0x31] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00}, // 1
    [0x32] = {0x3e, 0x61, 0x01, 0x3e, 0x40, 0x41, 0x7f, 0x00}, // 2
    [0x33] = {0x3e, 0x61, 0x01, 0x3e, 0x01, 0x61, 0x3e, 0x00}, // 3
    [0x34] = {0x06, 0x0e, 0x16, 0x26, 0x7f, 0x06, 0x06, 0x00}, // 4
    [0x35] = {0x7f, 0x40, 0x7e, 0x01, 0x01, 0x61, 0x3e, 0x00}, // 5
    [0x36] = {0x3e, 0x61, 0x40, 0x7e, 0x61, 0x61, 0x3e, 0x00}, // 6
    [0x37] = {0x7f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00}, // 7
    [0x38] = {0x3e, 0x61, 0x61, 0x3e, 0x61, 0x61, 0x3e, 0x00}, // 8
    [0x39] = {0x3e, 0x61, 0x61, 0x3f, 0x01, 0x61, 0x3e, 0x00}, // 9
    [0x3A] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // :
    [0x3B] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x00}, // ;
    [0x3C] = {0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06, 0x00}, // <
    [0x3D] = {0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00}, // =
    [0x3E] = {0x60, 0x30, 0x18, 0x0c, 0x18, 0x30, 0x60, 0x00}, // >
    [0x3F] = {0x3e, 0x61, 0x02, 0x0c, 0x18, 0x00, 0x18, 0x00}, // ?
    [0x40] = {0x3e, 0x61, 0x6d, 0x6d, 0x6d, 0x60, 0x3e, 0x00}, // @
    [0x41] = {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00}, // A
    [0x42] = {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00}, // B
    [0x43] = {0x3c, 0x66, 0x40, 0x40, 0x40, 0x66, 0x3c, 0x00}, // C
    [0x44] = {0x78, 0x64, 0x62, 0x62, 0x62, 0x64, 0x78, 0x00}, // D
    [0x45] = {0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x7e, 0x00}, // E
    [0x46] = {0x7e, 0x40, 0x40, 0x7c, 0x40, 0x40, 0x40, 0x00}, // F
    [0x47] = {0x3c, 0x66, 0x40, 0x4e, 0x46, 0x66, 0x3e, 0x00}, // G
    [0x48] = {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00}, // H
    [0x49] = {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // I
    [0x4A] = {0x0e, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // J
    [0x4B] = {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00}, // K
    [0x4C] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x00}, // L
    [0x4D] = {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00}, // M
    [0x4E] = {0x66, 0x6e, 0x76, 0x76, 0x76, 0x72, 0x66, 0x00}, // N
    [0x4F] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // O
    [0x50] = {0x7c, 0x66, 0x66, 0x7c, 0x40, 0x40, 0x40, 0x00}, // P
    [0x51] = {0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00}, // Q
    [0x52] = {0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00}, // R
    [0x53] = {0x3c, 0x66, 0x40, 0x3c, 0x06, 0x66, 0x3c, 0x00}, // S
    [0x54] = {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    [0x55] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // U
    [0x56] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // V
    [0x57] = {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // W
    [0x58] = {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00}, // X
    [0x59] = {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00}, // Y
    [0x5A] = {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00}, // Z
    [0x5B] = {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00}, // [
    [0x5C] = {0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x01, 0x00}, // Backslash
    [0x5D] = {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00}, // ]
    [0x5E] = {0x18, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
    [0x5F] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff}, // _
    [0x60] = {0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00}, // `
    [0x61] = {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00}, // a
    [0x62] = {0x40, 0x40, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00}, // b
    [0x63] = {0x00, 0x00, 0x3c, 0x40, 0x40, 0x46, 0x3c, 0x00}, // c
    [0x64] = {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00}, // d
    [0x65] = {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x40, 0x3c, 0x00}, // e
    [0x66] = {0x1c, 0x32, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x00}, // f
    [0x67] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // g
    [0x68] = {0x40, 0x40, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // h
    [0x69] = {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00}, // i
    [0x6A] = {0x06, 0x00, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // j
    [0x6B] = {0x40, 0x40, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00}, // k
    [0x6C] = {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // l
    [0x6D] = {0x00, 0x00, 0x66, 0x7f, 0x7f, 0x6b, 0x63, 0x00}, // m
    [0x6E] = {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // n
    [0x6F] = {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00}, // o
    [0x70] = {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x40, 0x40}, // p
    [0x71] = {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06}, // q
    [0x72] = {0x00, 0x00, 0x7c, 0x66, 0x40, 0x40, 0x40, 0x00}, // r
    [0x73] = {0x00, 0x00, 0x3e, 0x40, 0x3c, 0x02, 0x7c, 0x00}, // s
    [0x74] = {0x30, 0x30, 0x7c, 0x30, 0x30, 0x36, 0x1c, 0x00}, // t
    [0x75] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00}, // u
    [0x76] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // v
    [0x77] = {0x00, 0x00, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // w
    [0x78] = {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00}, // x
    [0x79] = {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // y
    [0x7A] = {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00}, // z
    [0x7B] = {0x0c, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0c, 0x00}, // {
    [0x7C] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // |
    [0x7D] = {0x30, 0x18, 0x18, 0x0c, 0x18, 0x18, 0x30, 0x00}, // }
    [0x7E] = {0x00, 0x00, 0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00}, // ~
};

void fb_init(struct limine_framebuffer_response *response) {
    if (response == NULL || response->framebuffer_count < 1) {
        return;
    }
    
    struct limine_framebuffer *limine_fb = response->framebuffers[0];
    
    framebuffer = kmalloc(sizeof(struct framebuffer));
    if (framebuffer == NULL) {
        return;
    }
    
    framebuffer->address = limine_fb->address;
    framebuffer->width = (uint32_t)limine_fb->width;
    framebuffer->height = (uint32_t)limine_fb->height;
    framebuffer->pitch = (uint32_t)limine_fb->pitch;
    framebuffer->bpp = limine_fb->bpp;
    framebuffer->red_mask_size = limine_fb->red_mask_size;
    framebuffer->red_mask_shift = limine_fb->red_mask_shift;
    framebuffer->green_mask_size = limine_fb->green_mask_size;
    framebuffer->green_mask_shift = limine_fb->green_mask_shift;
    framebuffer->blue_mask_size = limine_fb->blue_mask_size;
    framebuffer->blue_mask_shift = limine_fb->blue_mask_shift;
    
    serial_write_string("Framebuffer: ");
    serial_write_hex_u64((uint64_t)framebuffer->address);
    serial_write_string("\nWidth: ");
    serial_write_dec(framebuffer->width);
    serial_write_string("\nHeight: ");
    serial_write_dec(framebuffer->height);
    serial_write_string("\nPitch: ");
    serial_write_dec(framebuffer->pitch);
    serial_write_string("\n");

    size_t bb_size = framebuffer->height * framebuffer->pitch;
    framebuffer->backbuffer = kmalloc(bb_size);
    if (framebuffer->backbuffer != NULL) {
        memset(framebuffer->backbuffer, 0, bb_size);
    }
}

static inline uint32_t pixel_offset(uint32_t x, uint32_t y) {
    return y * framebuffer->pitch + x * (framebuffer->bpp / 8);
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (framebuffer == NULL || x >= framebuffer->width || y >= framebuffer->height) {
        return;
    }
    
    uint32_t *target = (uint32_t *)((uint8_t *)(framebuffer->backbuffer ? framebuffer->backbuffer : framebuffer->address) + (y * framebuffer->pitch + x * 4));
    *target = color;
}

void fb_flip(void) {
    if (framebuffer == NULL || framebuffer->address == NULL || framebuffer->backbuffer == NULL) {
        return;
    }
    
    memcpy(framebuffer->address, framebuffer->backbuffer, framebuffer->height * framebuffer->pitch);
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (framebuffer == NULL) return;
    
    // Clipping
    if (x >= framebuffer->width || y >= framebuffer->height) return;
    if (x + w > framebuffer->width) w = framebuffer->width - x;
    if (y + h > framebuffer->height) h = framebuffer->height - y;
    
    uint8_t *bb = (uint8_t *)(framebuffer->backbuffer ? framebuffer->backbuffer : framebuffer->address);
    uint32_t pitch = framebuffer->pitch;
    
    for (uint32_t i = 0; i < h; ++i) {
        uint32_t *row = (uint32_t *)(bb + (y + i) * pitch + x * 4);
        for (uint32_t j = 0; j < w; ++j) {
            row[j] = color;
        }
    }
}

void fb_fill_rect_alpha(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color, uint8_t alpha) {
    if (alpha == 255) {
        fb_fill_rect(x, y, w, h, color);
        return;
    }
    if (alpha == 0) return;
    
    for (uint32_t j = 0; j < h; ++j) {
        for (uint32_t i = 0; i < w; ++i) {
            if (x + i >= framebuffer->width || y + j >= framebuffer->height) continue;
            
            uint32_t bg = 0;
            uint32_t offset = pixel_offset(x + i, y + j);
            bg = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + offset));
            
            uint32_t blended = fb_blend_colors(bg, color, alpha);
            fb_put_pixel(x + i, y + j, blended);
        }
    }
}

void fb_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < w; ++i) {
        fb_put_pixel(x + i, y, color);
        fb_put_pixel(x + i, y + h - 1, color);
    }
    for (uint32_t i = 0; i < h; ++i) {
        fb_put_pixel(x, y + i, color);
        fb_put_pixel(x + w - 1, y + i, color);
    }
}

void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    int32_t dx = (int32_t)x2 - (int32_t)x1;
    dx = dx < 0 ? -dx : dx;
    int32_t sx = x1 < x2 ? 1 : -1;
    int32_t dy = (int32_t)y2 - (int32_t)y1;
    dy = dy < 0 ? dy : -dy;
    int32_t sy = y1 < y2 ? 1 : -1;
    int32_t err = dx + dy;
    
    while (1) {
        fb_put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg_color, uint32_t bg_color) {
    uint8_t ch = (uint8_t)c;
    if (ch >= 128) ch = 0;
    
    for (uint8_t row = 0; row < 8; ++row) {
        uint8_t data = font_data[ch][row];
        for (uint8_t col = 0; col < 8; ++col) {
            if (data & (0x80 >> col)) {
                fb_put_pixel(x + col, y + row, fg_color);
            } else if (bg_color != 0) {
                fb_put_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    uint32_t pos_x = x;
    while (*str != '\0') {
        fb_draw_char(pos_x, y, *str, fg_color, bg_color);
        ++str;
        pos_x += 8;
    }
}

uint32_t fb_make_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

uint32_t fb_blend_colors(uint32_t color1, uint32_t color2, uint8_t alpha) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t r = (r1 * (255 - alpha) + r2 * alpha) / 255;
    uint8_t g = (g1 * (255 - alpha) + g2 * alpha) / 255;
    uint8_t b = (b1 * (255 - alpha) + b2 * alpha) / 255;
    
    return fb_make_color(r, g, b);
}

void fb_fill_rect_gradient_h(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color1, uint32_t color2) {
    for (uint32_t i = 0; i < w; ++i) {
        uint8_t alpha = (i * 255) / (w > 1 ? w - 1 : 1);
        uint32_t color = fb_blend_colors(color1, color2, alpha);
        for (uint32_t j = 0; j < h; ++j) {
            fb_put_pixel(x + i, y + j, color);
        }
    }
}

void fb_fill_rect_gradient_v(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color1, uint32_t color2) {
    for (uint32_t j = 0; j < h; ++j) {
        uint8_t alpha = (j * 255) / (h > 1 ? h - 1 : 1);
        uint32_t color = fb_blend_colors(color1, color2, alpha);
        for (uint32_t i = 0; i < w; ++i) {
            fb_put_pixel(x + i, y + j, color);
        }
    }
}

void fb_draw_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint32_t color) {
    fb_draw_line(x1, y1, x2, y2, color);
    fb_draw_line(x2, y2, x3, y3, color);
    fb_draw_line(x3, y3, x1, y1, color);
}

void fb_draw_rect_rounded(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t radius, uint32_t color) {
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius == 0) {
        fb_draw_rect_outline(x, y, w, h, color);
        return;
    }
    
    // Draw straight edges
    fb_draw_line(x + radius, y, x + w - radius, y, color); // Top
    fb_draw_line(x + radius, y + h - 1, x + w - radius, y + h - 1, color); // Bottom
    fb_draw_line(x, y + radius, x, y + h - radius, color); // Left
    fb_draw_line(x + w - 1, y + radius, x + w - 1, y + h - radius, color); // Right
    
    // Draw corners using Bresenham's circle algorithm (simplified)
    for (int32_t dy = 0; dy <= (int32_t)radius; ++dy) {
        for (int32_t dx = 0; dx <= (int32_t)radius; ++dx) {
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                fb_put_pixel(x + radius - dx, y + radius - dy, color); // Top-left
                fb_put_pixel(x + w - radius + dx, y + radius - dy, color); // Top-right
                fb_put_pixel(x + radius - dx, y + h - radius + dy, color); // Bottom-left
                fb_put_pixel(x + w - radius + dx, y + h - radius + dy, color); // Bottom-right
            }
        }
    }
}

void fb_fill_rect_rounded(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t radius, uint32_t color) {
    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;
    if (radius == 0) {
        fb_fill_rect(x, y, w, h, color);
        return;
    }
    
    // Fill main rectangle
    fb_fill_rect(x + radius, y, w - 2 * radius, h, color);
    fb_fill_rect(x, y + radius, w, h - 2 * radius, color);
    
    // Fill corner circles using Bresenham's circle algorithm (simplified)
    for (int32_t dy = 0; dy <= (int32_t)radius; ++dy) {
        for (int32_t dx = 0; dx <= (int32_t)radius; ++dx) {
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                fb_put_pixel(x + radius - dx, y + radius - dy, color); // Top-left
                fb_put_pixel(x + w - radius + dx, y + radius - dy, color); // Top-right
                fb_put_pixel(x + radius - dx, y + h - radius + dy, color); // Bottom-left
                fb_put_pixel(x + w - radius + dx, y + h - radius + dy, color); // Bottom-right
            }
        }
    }
}

void fb_draw_bitmap(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *bitmap) {
    for (uint32_t j = 0; j < h; ++j) {
        for (uint32_t i = 0; i < w; ++i) {
            uint32_t color = bitmap[j * w + i];
            if (color != 0xFF000000) { // Skip fully transparent (black with alpha)
                fb_put_pixel(x + i, y + j, color);
            }
        }
    }
}

void fb_draw_bitmap_alpha(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint32_t *bitmap) {
    for (uint32_t j = 0; j < h; ++j) {
        for (uint32_t i = 0; i < w; ++i) {
            uint32_t pixel = bitmap[j * w + i];
            uint8_t alpha = (pixel >> 24) & 0xFF;
            
            if (alpha == 0) continue; // Fully transparent
            
            if (alpha == 255) {
                // Fully opaque, no blending needed
                fb_put_pixel(x + i, y + j, pixel & 0x00FFFFFF);
            } else {
                // Alpha blending with background
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;
                
                // Get background pixel
                uint32_t bg = 0; // Default black if out of bounds
                if (x + i < framebuffer->width && y + j < framebuffer->height) {
                    uint32_t offset = pixel_offset(x + i, y + j);
                    bg = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + offset));
                }
                
                uint8_t bg_r = (bg >> 16) & 0xFF;
                uint8_t bg_g = (bg >> 8) & 0xFF;
                uint8_t bg_b = bg & 0xFF;
                
                // Blend: result = (fg * alpha + bg * (255 - alpha)) / 255
                uint8_t out_r = (r * alpha + bg_r * (255 - alpha)) / 255;
                uint8_t out_g = (g * alpha + bg_g * (255 - alpha)) / 255;
                uint8_t out_b = (b * alpha + bg_b * (255 - alpha)) / 255;
                
                fb_put_pixel(x + i, y + j, (out_r << 16) | (out_g << 8) | out_b);
            }
        }
    }
}

void fb_draw_bitmap_scaled(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t src_w, uint32_t src_h, const uint32_t *bitmap) {
    for (uint32_t j = 0; j < h; ++j) {
        for (uint32_t i = 0; i < w; ++i) {
            // Map destination pixel to source pixel
            uint32_t src_x = (i * src_w) / w;
            uint32_t src_y = (j * src_h) / h;
            uint32_t pixel = bitmap[src_y * src_w + src_x];
            
            uint8_t alpha = (pixel >> 24) & 0xFF;
            if (alpha == 0) continue;
            
            if (alpha == 255) {
                fb_put_pixel(x + i, y + j, pixel & 0x00FFFFFF);
            } else {
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;
                
                uint32_t bg = 0;
                if (x + i < framebuffer->width && y + j < framebuffer->height) {
                    uint32_t offset = pixel_offset(x + i, y + j);
                    bg = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + offset));
                }
                
                uint8_t bg_r = (bg >> 16) & 0xFF;
                uint8_t bg_g = (bg >> 8) & 0xFF;
                uint8_t bg_b = bg & 0xFF;
                
                uint8_t out_r = (r * alpha + bg_r * (255 - alpha)) / 255;
                uint8_t out_g = (g * alpha + bg_g * (255 - alpha)) / 255;
                uint8_t out_b = (b * alpha + bg_b * (255 - alpha)) / 255;
                
                fb_put_pixel(x + i, y + j, (out_r << 16) | (out_g << 8) | out_b);
            }
        }
    }
}

void fb_draw_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    if (framebuffer == NULL || r == 0) return;
    int32_t x = (int32_t)r, y = 0;
    int32_t d = 1 - x;
    while (x >= y) {
        fb_put_pixel(cx + x, cy + y, color);
        fb_put_pixel(cx - x, cy + y, color);
        fb_put_pixel(cx + x, cy - y, color);
        fb_put_pixel(cx - x, cy - y, color);
        fb_put_pixel(cx + y, cy + x, color);
        fb_put_pixel(cx - y, cy + x, color);
        fb_put_pixel(cx + y, cy - x, color);
        fb_put_pixel(cx - y, cy - x, color);
        y++;
        if (d <= 0) { d += 2 * y + 1; }
        else { x--; d += 2 * (y - x) + 1; }
    }
}

void fb_fill_circle(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color) {
    if (framebuffer == NULL || r == 0) return;
    for (int32_t dy = -(int32_t)r; dy <= (int32_t)r; dy++) {
        for (int32_t dx = -(int32_t)r; dx <= (int32_t)r; dx++) {
            if (dx * dx + dy * dy <= (int32_t)(r * r)) {
                fb_put_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

void fb_fill_triangle(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint32_t color) {
    // Sort vertices by y
    if (y1 > y2) { uint32_t t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }
    if (y1 > y3) { uint32_t t; t=x1;x1=x3;x3=t; t=y1;y1=y3;y3=t; }
    if (y2 > y3) { uint32_t t; t=x2;x2=x3;x3=t; t=y2;y2=y3;y3=t; }
    if (y3 == y1) return;
    for (uint32_t y = y1; y <= y3; y++) {
        int32_t xa = (int32_t)x1 + (int32_t)(x3 - x1) * (int32_t)(y - y1) / (int32_t)(y3 - y1);
        int32_t xb;
        if (y < y2) {
            if (y2 == y1) xb = (int32_t)x1;
            else xb = (int32_t)x1 + (int32_t)(x2 - x1) * (int32_t)(y - y1) / (int32_t)(y2 - y1);
        } else {
            if (y3 == y2) xb = (int32_t)x2;
            else xb = (int32_t)x2 + (int32_t)(x3 - x2) * (int32_t)(y - y2) / (int32_t)(y3 - y2);
        }
        if (xa > xb) { int32_t t = xa; xa = xb; xb = t; }
        for (int32_t xx = xa; xx <= xb; xx++) {
            fb_put_pixel((uint32_t)xx, y, color);
        }
    }
}

void fb_draw_char_colored(uint32_t x, uint32_t y, char c, uint32_t fg_color) {
    fb_draw_char(x, y, c, fg_color, 0);
}

void fb_draw_string_scaled(uint32_t x, uint32_t y, const char *str, uint32_t fg_color, uint32_t scale) {
    if (framebuffer == NULL || str == NULL || scale == 0) return;
    uint32_t pos_x = x;
    while (*str) {
        uint8_t ch = (uint8_t)*str;
        if (ch >= 128) ch = 0;
        for (uint8_t row = 0; row < 8; row++) {
            extern const uint8_t font_data[128][8];
            uint8_t data = font_data[ch][row];
            for (uint8_t col = 0; col < 8; col++) {
                if (data & (0x80 >> col)) {
                    fb_fill_rect(pos_x + col * scale, y + row * scale, scale, scale, fg_color);
                }
            }
        }
        str++;
        pos_x += 8 * scale;
    }
}

void fb_draw_string_centered(uint32_t cx, uint32_t y, uint32_t w, const char *str, uint32_t fg_color) {
    if (str == NULL) return;
    uint32_t len = 0;
    const char *p = str;
    while (*p++) len++;
    uint32_t text_w = len * 8;
    uint32_t x = cx;
    if (text_w < w) x = cx + (w - text_w) / 2;
    fb_draw_string(x, y, str, fg_color, 0);
}

void fb_blur_region(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (framebuffer == NULL || framebuffer->backbuffer == NULL) return;
    if (x >= framebuffer->width || y >= framebuffer->height) return;
    if (x + w > framebuffer->width) w = framebuffer->width - x;
    if (y + h > framebuffer->height) h = framebuffer->height - y;
    // Optimized Separable Blur (1D Horizontal then 1D Vertical)
    uint32_t *temp_buf = kmalloc(w * sizeof(uint32_t));
    if (!temp_buf) return;
    
    // Vertical pass (in-place approximation for simplicity, writing back)
    for (uint32_t i = x; i < x + w; i++) {
        uint32_t prev_r = 0, prev_g = 0, prev_b = 0;
        for (uint32_t j = y; j < y + h; j++) {
            uint32_t off = j * framebuffer->pitch + i * 4;
            uint32_t px = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + off));
            uint32_t r = (px >> 16) & 0xFF;
            uint32_t g = (px >> 8) & 0xFF;
            uint32_t b = px & 0xFF;
            
            if (j > y) {
                r = (r + prev_r) / 2;
                g = (g + prev_g) / 2;
                b = (b + prev_b) / 2;
                *((uint32_t *)((uint8_t *)framebuffer->backbuffer + off)) = (r << 16) | (g << 8) | b;
            }
            prev_r = r; prev_g = g; prev_b = b;
        }
    }
    
    // Horizontal pass
    for (uint32_t j = y; j < y + h; j++) {
        uint32_t prev_r = 0, prev_g = 0, prev_b = 0;
        for (uint32_t i = x; i < x + w; i++) {
            uint32_t off = j * framebuffer->pitch + i * 4;
            uint32_t px = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + off));
            uint32_t r = (px >> 16) & 0xFF;
            uint32_t g = (px >> 8) & 0xFF;
            uint32_t b = px & 0xFF;
            
            if (i > x) {
                r = (r + prev_r) / 2;
                g = (g + prev_g) / 2;
                b = (b + prev_b) / 2;
                *((uint32_t *)((uint8_t *)framebuffer->backbuffer + off)) = (r << 16) | (g << 8) | b;
            }
            prev_r = r; prev_g = g; prev_b = b;
        }
    }
    
    kfree(temp_buf);
}

void fb_draw_string_aa(uint32_t x, uint32_t y, const char *str, uint32_t fg_color) {
    if (framebuffer == NULL || str == NULL) return;
    uint32_t pos_x = x;
    uint32_t r_fg = (fg_color >> 16) & 0xFF;
    uint32_t g_fg = (fg_color >> 8) & 0xFF;
    uint32_t b_fg = fg_color & 0xFF;

    while (*str) {
        uint8_t ch = (uint8_t)*str;
        if (ch >= 128) ch = 0;
        for (uint8_t row = 0; row < 8; row++) {
            extern const uint8_t font_data[128][8];
            uint8_t data = font_data[ch][row];
            for (uint8_t col = 0; col < 8; col++) {
                if (data & (0x80 >> col)) {
                    // Simple AA by bleeding into adjacent pixels with 50% opacity
                    fb_put_pixel(pos_x + col, y + row, fg_color);
                    
                    if (!(data & (0x80 >> (col + 1))) && col < 7) {
                        uint32_t off = (y + row) * framebuffer->pitch + (pos_x + col + 1) * 4;
                        uint32_t bg = *((uint32_t *)((uint8_t *)framebuffer->backbuffer + off));
                        uint32_t r = (((bg >> 16) & 0xFF) + r_fg) / 2;
                        uint32_t g = (((bg >> 8) & 0xFF) + g_fg) / 2;
                        uint32_t b = ((bg & 0xFF) + b_fg) / 2;
                        fb_put_pixel(pos_x + col + 1, y + row, (r << 16) | (g << 8) | b);
                    }
                }
            }
        }
        str++;
        pos_x += 8;
    }
}

void fb_fill_rect_gradient_radial(uint32_t cx, uint32_t cy, uint32_t r, uint32_t color1, uint32_t color2) {
    if (framebuffer == NULL || r == 0) return;
    
    uint32_t r1 = (color1 >> 16) & 0xFF;
    uint32_t g1 = (color1 >> 8) & 0xFF;
    uint32_t b1 = color1 & 0xFF;
    
    uint32_t r2 = (color2 >> 16) & 0xFF;
    uint32_t g2 = (color2 >> 8) & 0xFF;
    uint32_t b2 = color2 & 0xFF;
    
    for (int32_t dy = -(int32_t)r; dy <= (int32_t)r; dy++) {
        for (int32_t dx = -(int32_t)r; dx <= (int32_t)r; dx++) {
            uint32_t dist_sq = dx * dx + dy * dy;
            if (dist_sq <= r * r) {
                // Integer square root approx
                uint32_t dist = 0;
                while ((dist + 1) * (dist + 1) <= dist_sq) dist++;
                
                uint32_t ratio = (dist * 255) / r; // 0 to 255
                uint32_t inv_ratio = 255 - ratio;
                
                uint32_t nr = (r1 * inv_ratio + r2 * ratio) / 255;
                uint32_t ng = (g1 * inv_ratio + g2 * ratio) / 255;
                uint32_t nb = (b1 * inv_ratio + b2 * ratio) / 255;
                
                // Read background and alpha blend (assume gradient is additive or screen-like for wallpaper, but we'll just alpha blend it manually if needed, or straight overwrite)
                // We'll just overwrite for simplicity since it's a solid gradient background
                fb_put_pixel(cx + dx, cy + dy, (nr << 16) | (ng << 8) | nb);
            }
        }
    }
}