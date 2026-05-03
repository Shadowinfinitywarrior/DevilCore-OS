// DevilCore OS Logo rendering implementation
#include "logo.h"
#include "framebuffer.h"

#include "logo_raw.h"

// Draw the large 612x408 raw logo
void logo_raw_draw(int32_t x, int32_t y) {
    for (uint32_t row = 0; row < LOGO_HEIGHT; row++) {
        for (uint32_t col = 0; col < LOGO_WIDTH; col++) {
            uint32_t color = logo_pixels[row * LOGO_WIDTH + col];
            // Simple transparency (0x00000000 is transparent)
            if ((color >> 24) != 0xFF) {
                // If alpha is not 0xFF, we treat it as potentially transparent 
                // but here we'll just check for literal 0x00000000
                if (color == 0) continue;
            }
            fb_put_pixel(x + col, y + row, color);
        }
    }
}

// Draw 64x64 logo
void logo64_draw(int32_t x, int32_t y, uint32_t scale) {
    for (uint32_t row = 0; row < LOGO64_HEIGHT; row++) {
        for (uint32_t col = 0; col < LOGO64_WIDTH; col++) {
            uint32_t color = logo64_pixels[row][col];
            if (color != 0x00101010) { // Don't draw background color
                if (scale == 1) {
                    fb_put_pixel(x + col, y + row, color);
                } else {
                    fb_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

// Draw 32x32 logo
void logo32_draw(int32_t x, int32_t y) {
    for (uint32_t row = 0; row < LOGO32_HEIGHT; row++) {
        for (uint32_t col = 0; col < LOGO32_WIDTH; col++) {
            uint32_t color = logo32_pixels[row][col];
            if (color != 0) {
                fb_put_pixel(x + col, y + row, color);
            }
        }
    }
}
