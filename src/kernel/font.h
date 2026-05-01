#ifndef DEVILCORE_FONT_H
#define DEVILCORE_FONT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// PSF2 font format (very compact, good for OS)
#define PSF2_MAGIC 0x864AB572
#define PSF2_HAS_UNICODE_TABLE 0x01

typedef struct psf2_header {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t num_glyphs;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
} psf2_header_t;

typedef struct font {
    psf2_header_t *header;
    uint8_t *glyphs;
    uint32_t *unicode_table;  // Unicode to glyph index
    
    // Metrics
    uint32_t glyph_width;
    uint32_t glyph_height;
    uint32_t advance;
    
    // Scale factor (for different sizes)
    uint32_t scale;
    
    // Font properties
    char name[64];
    bool bold;
    bool italic;
} font_t;

// Font registry
typedef struct font_entry {
    char name[64];
    uint32_t size;
    bool bold;
    bool italic;
    font_t *font;
} font_entry_t;

#define FONT_REGISTRY_MAX 32

typedef struct font_registry {
    font_entry_t entries[FONT_REGISTRY_MAX];
    uint32_t count;
} font_registry_t;

// Color with alpha
typedef struct color {
    uint8_t r, g, b, a;
} color_t;

// Font API
void font_system_init(void);
font_t *font_load_psf2(const void *data, size_t size, const char *name);
font_t *font_get(const char *name, uint32_t size, bool bold, bool italic);
void font_register(const char *name, uint32_t size, bool bold, bool italic, font_t *font);

// Render glyph to buffer
void font_render_glyph(font_t *font, uint32_t codepoint,
                        uint32_t *pixels, uint32_t stride,
                        int32_t x, int32_t y,
                        color_t fg, color_t bg);

// Anti-aliased rendering
void font_render_glyph_aa(font_t *font, uint32_t codepoint,
                            uint32_t *pixels, uint32_t stride,
                            int32_t x, int32_t y,
                            color_t fg);

// Text measurement
void font_measure_string(font_t *font, const char *text,
                          uint32_t *width, uint32_t *height);
uint32_t font_string_width(font_t *font, const char *text);
uint32_t font_string_height(font_t *font, const char *text);

// String rendering
void font_draw_string(font_t *font, const char *text,
                      uint32_t *pixels, uint32_t stride,
                      int32_t x, int32_t y, color_t fg);
void font_draw_string_bg(font_t *font, const char *text,
                         uint32_t *pixels, uint32_t stride,
                         int32_t x, int32_t y, color_t fg, color_t bg);

// Color helpers
static inline color_t color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (color_t){r, g, b, 255};
}

static inline color_t color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (color_t){r, g, b, a};
}

static inline uint32_t color_to_uint32(color_t c) {
    return ((uint32_t)c.a << 24) | ((uint32_t)c.r << 16) | 
           ((uint32_t)c.g << 8) | (uint32_t)c.b;
}

static inline color_t uint32_to_color(uint32_t c) {
    return (color_t){
        .a = (c >> 24) & 0xFF,
        .r = (c >> 16) & 0xFF,
        .g = (c >> 8) & 0xFF,
        .b = c & 0xFF
    };
}

// Alpha blending
static inline uint32_t blend_pixels(uint32_t src, uint32_t dst) {
    uint8_t a = (src >> 24) & 0xFF;
    if (a == 0) return dst;
    if (a == 255) return src;
    
    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = src & 0xFF;
    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = dst & 0xFF;
    
    uint8_t inv_a = 255 - a;
    uint8_t r = (sr * a + dr * inv_a) / 255;
    uint8_t g = (sg * a + dg * inv_a) / 255;
    uint8_t b = (sb * a + db * inv_a) / 255;
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Embedded font declarations (will be defined in font_data.c)
extern const uint8_t font_terminus_8x16[];
extern const uint32_t font_terminus_8x16_size;
extern const uint8_t font_terminus_12x24[];
extern const uint32_t font_terminus_12x24_size;

#endif // DEVILCORE_FONT_H
