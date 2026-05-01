#include "font.h"
#include "memory.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);

static font_registry_t font_registry = {0};

void font_system_init(void) {
    kprintf("Initializing font system...\n");
    
    // Note: Actual font data will be embedded and loaded elsewhere
    // This function just initializes the registry
    
    font_registry.count = 0;
    
    kprintf("Font system initialized\n");
}

font_t *font_load_psf2(const void *data, size_t size, const char *name) {
    if (!data || size < sizeof(psf2_header_t)) {
        kprintf("Invalid PSF2 font data\n");
        return NULL;
    }
    
    psf2_header_t *header = (psf2_header_t*)data;
    
    // Verify magic
    if (header->magic != PSF2_MAGIC) {
        kprintf("Invalid PSF2 magic: 0x%x (expected 0x%x)\n", 
                header->magic, PSF2_MAGIC);
        return NULL;
    }
    
    // Verify header size
    if (header->header_size < sizeof(psf2_header_t)) {
        kprintf("Invalid PSF2 header size\n");
        return NULL;
    }
    
    font_t *font = kmalloc(sizeof(font_t));
    if (!font) {
        kprintf("Failed to allocate font structure\n");
        return NULL;
    }
    
    font->header = header;
    font->glyphs = (uint8_t*)data + header->header_size;
    font->glyph_width = header->width;
    font->glyph_height = header->height;
    font->advance = header->width;
    font->scale = 1;
    font->bold = false;
    font->italic = false;
    
    strncpy(font->name, name, 63);
    font->name[63] = '\0';
    
    // Check for unicode table
    if (header->flags & PSF2_HAS_UNICODE_TABLE) {
        // Unicode table starts after glyphs
        // For now, we don't parse the unicode table
        // Just use direct glyph index
        font->unicode_table = NULL;
    } else {
        font->unicode_table = NULL;
    }
    
    kprintf("Loaded font '%s': %dx%d, %u glyphs\n",
            name, font->glyph_width, font->glyph_height, header->num_glyphs);
    
    return font;
}

void font_register(const char *name, uint32_t size, bool bold, bool italic, font_t *font) {
    if (font_registry.count >= FONT_REGISTRY_MAX) {
        kprintf("Font registry full\n");
        return;
    }
    
    font_entry_t *entry = &font_registry.entries[font_registry.count++];
    strncpy(entry->name, name, 63);
    entry->name[63] = '\0';
    entry->size = size;
    entry->bold = bold;
    entry->italic = italic;
    entry->font = font;
    
    font->bold = bold;
    font->italic = italic;
}

font_t *font_get(const char *name, uint32_t size, bool bold, bool italic) {
    // Find best matching font
    font_t *best_match = NULL;
    
    for (uint32_t i = 0; i < font_registry.count; i++) {
        font_entry_t *entry = &font_registry.entries[i];
        
        if (strcmp(entry->name, name) == 0 &&
            entry->bold == bold &&
            entry->italic == italic) {
            // Exact match
            if (entry->size == size) {
                return entry->font;
            }
            // Size approximation
            if (!best_match || 
                (entry->size > best_match->glyph_height && entry->size <= size) ||
                (entry->size < best_match->glyph_height && entry->size > size)) {
                best_match = entry->font;
            }
        }
    }
    
    return best_match;
}

void font_render_glyph(font_t *font, uint32_t codepoint,
                        uint32_t *pixels, uint32_t stride,
                        int32_t x, int32_t y,
                        color_t fg, color_t bg) {
    if (!font || !pixels) return;
    
    // Map codepoint to glyph index (simple mapping for now)
    // For proper unicode, would need to parse unicode table
    uint32_t glyph_idx = codepoint;
    if (glyph_idx >= font->header->num_glyphs) {
        glyph_idx = 0;  // Use first glyph as fallback
    }
    
    // Get glyph data
    uint8_t *glyph_data = font->glyphs + glyph_idx * font->header->bytes_per_glyph;
    
    uint32_t fg_color = color_to_uint32(fg);
    uint32_t bg_color = color_to_uint32(bg);
    
    // Render glyph (1 bit per pixel)
    for (uint32_t row = 0; row < font->glyph_height; row++) {
        for (uint32_t col = 0; col < font->glyph_width; col++) {
            // Calculate bit position
            uint32_t byte_idx = (row * font->glyph_width + col) / 8;
            uint32_t bit_idx = 7 - ((row * font->glyph_width + col) % 8);
            
            uint8_t byte = glyph_data[byte_idx];
            uint8_t pixel_set = (byte >> bit_idx) & 1;
            
            int32_t dst_x = x + col;
            int32_t dst_y = y + row;
            
            if (dst_x >= 0 && dst_y >= 0) {
                pixels[dst_y * stride + dst_x] = pixel_set ? fg_color : bg_color;
            }
        }
    }
}

// Anti-aliased rendering using 4x supersampling
void font_render_glyph_aa(font_t *font, uint32_t codepoint,
                            uint32_t *pixels, uint32_t stride,
                            int32_t x, int32_t y,
                            color_t fg) {
    if (!font || !pixels) return;
    
    uint32_t glyph_idx = codepoint;
    if (glyph_idx >= font->header->num_glyphs) glyph_idx = 0;
    
    uint8_t *glyph_data = font->glyphs + glyph_idx * font->header->bytes_per_glyph;
    
    for (uint32_t row = 0; row < font->glyph_height; row++) {
        for (uint32_t col = 0; col < font->glyph_width; col++) {
            uint32_t byte_idx = (row * font->glyph_width + col) / 8;
            uint32_t bit_idx = 7 - ((row * font->glyph_width + col) % 8);
            
            uint8_t byte = glyph_data[byte_idx];
            uint8_t pixel_set = (byte >> bit_idx) & 1;
            
            if (pixel_set) {
                int32_t dst_x = x + col;
                int32_t dst_y = y + row;
                
                if (dst_x >= 0 && dst_y >= 0) {
                    uint32_t dst_idx = dst_y * stride + dst_x;
                    uint32_t fg_uint = color_to_uint32(fg);
                    pixels[dst_idx] = blend_pixels(fg_uint, pixels[dst_idx]);
                }
            }
        }
    }
    
    // TODO: Implement actual sub-pixel AA by sampling adjacent bits if the font
    // was high-resolution or using a grayscale font. For PSF2, we can only
    // do simple blending with existing pixels (which we do above).
}

void font_measure_string(font_t *font, const char *text,
                          uint32_t *width, uint32_t *height) {
    if (!font || !text) {
        if (width) *width = 0;
        if (height) *height = 0;
        return;
    }
    
    uint32_t max_w = 0;
    uint32_t cur_w = 0;
    uint32_t h = font->glyph_height;
    uint32_t lines = 1;
    
    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            if (cur_w > max_w) max_w = cur_w;
            cur_w = 0;
            lines++;
        } else if (*p == '\t') {
            cur_w += font->advance * 4;
        } else if ((unsigned char)*p >= 32) {
            cur_w += font->advance;
        }
    }
    
    if (cur_w > max_w) max_w = cur_w;
    
    if (width) *width = max_w;
    if (height) *height = h * lines;
}

uint32_t font_string_width(font_t *font, const char *text) {
    uint32_t width;
    font_measure_string(font, text, &width, NULL);
    return width;
}

uint32_t font_string_height(font_t *font, const char *text) {
    uint32_t height;
    font_measure_string(font, text, NULL, &height);
    return height;
}

void font_draw_string(font_t *font, const char *text,
                      uint32_t *pixels, uint32_t stride,
                      int32_t x, int32_t y, color_t fg) {
    if (!font || !text || !pixels) return;
    
    int32_t cursor_x = x;
    int32_t cursor_y = y;
    
    color_t transparent = {0, 0, 0, 0};
    
    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            cursor_x = x;
            cursor_y += font->glyph_height;
        } else if (*p == '\t') {
            cursor_x += font->advance * 4;  // 4 spaces per tab
        } else if (*p >= 32) {  // Printable characters
            font_render_glyph(font, (uint8_t)*p, pixels, stride,
                              cursor_x, cursor_y, fg, transparent);
            cursor_x += font->advance;
        }
    }
}

void font_draw_string_bg(font_t *font, const char *text,
                         uint32_t *pixels, uint32_t stride,
                         int32_t x, int32_t y, color_t fg, color_t bg) {
    if (!font || !text || !pixels) return;
    
    int32_t cursor_x = x;
    int32_t cursor_y = y;
    
    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            cursor_x = x;
            cursor_y += font->glyph_height;
        } else if (*p == '\t') {
            cursor_x += font->advance * 4;
        } else if (*p >= 32) {
            font_render_glyph(font, (uint8_t)*p, pixels, stride,
                              cursor_x, cursor_y, fg, bg);
            cursor_x += font->advance;
        }
    }
}
