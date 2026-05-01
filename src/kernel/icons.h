#ifndef DEVILCORE_ICONS_H
#define DEVILCORE_ICONS_H

#include <stdint.h>

// Icon pixel data structure
struct icon_data {
    const char *name;
    uint32_t width;
    uint32_t height;
    const uint32_t *pixels; // Array of RGBA pixels (0xAARRGGBB)
};

// Get icon data by name
const struct icon_data *icon_get(const char *name);

// Draw an icon at position
void draw_icon(uint32_t x, uint32_t y, const struct icon_data *icon);

#endif
