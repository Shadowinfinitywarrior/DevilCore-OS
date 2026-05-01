#include "devilui.h"
#include <string.h>

void painter_init(painter_t *p, surface_t *surf) {
    if (!p || !surf) return;
    
    p->pixels = surf->back_buffer;
    p->width = surf->width;
    p->height = surf->height;
    p->stride = surf->width;
    
    p->clip.x = 0;
    p->clip.y = 0;
    p->clip.width = surf->width;
    p->clip.height = surf->height;
    
    p->transform_depth = 0;
    p->transform_stack[0].tx = 0;
    p->transform_stack[0].ty = 0;
    p->transform_stack[0].sx = 1.0f;
    p->transform_stack[0].sy = 1.0f;
    
    p->fill_color = color_rgb(255, 255, 255);
    p->stroke_color = color_rgb(0, 0, 0);
    p->stroke_width = 1;
    p->opacity = 1.0f;
    p->current_font = NULL;
}

void painter_save(painter_t *p) {
    if (!p || p->transform_depth >= 31) return;
    
    p->transform_stack[p->transform_depth + 1] = p->transform_stack[p->transform_depth];
    p->transform_depth++;
}

void painter_restore(painter_t *p) {
    if (!p || p->transform_depth == 0) return;
    
    p->transform_depth--;
}

void painter_translate(painter_t *p, int32_t x, int32_t y) {
    if (!p) return;
    p->transform_stack[p->transform_depth].tx += x;
    p->transform_stack[p->transform_depth].ty += y;
}

void painter_scale(painter_t *p, float sx, float sy) {
    if (!p) return;
    p->transform_stack[p->transform_depth].sx *= sx;
    p->transform_stack[p->transform_depth].sy *= sy;
}

void painter_clip_rect(painter_t *p, rect_t *rect) {
    if (!p || !rect) return;
    
    // Intersection of current clip and new rect
    int32_t x1 = p->clip.x > rect->x ? p->clip.x : rect->x;
    int32_t y1 = p->clip.y > rect->y ? p->clip.y : rect->y;
    int32_t x2 = (p->clip.x + (int32_t)p->clip.width) < (rect->x + (int32_t)rect->width) ? 
                 (p->clip.x + (int32_t)p->clip.width) : (rect->x + (int32_t)rect->width);
    int32_t y2 = (p->clip.y + (int32_t)p->clip.height) < (rect->y + (int32_t)rect->height) ? 
                 (p->clip.y + (int32_t)p->clip.height) : (rect->y + (int32_t)rect->height);
                 
    if (x1 < x2 && y1 < y2) {
        p->clip.x = x1;
        p->clip.y = y1;
        p->clip.width = (uint32_t)(x2 - x1);
        p->clip.height = (uint32_t)(y2 - y1);
    } else {
        p->clip.width = 0;
        p->clip.height = 0;
    }
}

void painter_set_fill(painter_t *p, color_t color) {
    if (p) p->fill_color = color;
}

void painter_set_stroke(painter_t *p, color_t color, uint32_t width) {
    if (!p) return;
    p->stroke_color = color;
    p->stroke_width = width;
}

void painter_set_opacity(painter_t *p, float opacity) {
    if (p) p->opacity = opacity;
}

void painter_set_font(painter_t *p, font_t *font) {
    if (p) p->current_font = font;
}

static inline void painter_draw_pixel(painter_t *p, int32_t x, int32_t y, color_t color) {
    int32_t tx = p->transform_stack[p->transform_depth].tx + x;
    int32_t ty = p->transform_stack[p->transform_depth].ty + y;
    
    if (tx >= p->clip.x && tx < (int32_t)(p->clip.x + p->clip.width) &&
        ty >= p->clip.y && ty < (int32_t)(p->clip.y + p->clip.height)) {
        
        uint32_t dst_idx = ty * p->stride + tx;
        uint32_t src_pixel = color_to_uint32(color);
        
        if (p->opacity < 1.0f) {
            uint8_t a = (uint8_t)((color.a * p->opacity));
            src_pixel = (a << 24) | (src_pixel & 0x00FFFFFF);
        }
        
        p->pixels[dst_idx] = blend_pixels(src_pixel, p->pixels[dst_idx]);
    }
}

void painter_fill_rect(painter_t *p, rect_t *r) {
    if (!p || !r) return;
    
    for (uint32_t y = 0; y < r->height; y++) {
        for (uint32_t x = 0; x < r->width; x++) {
            painter_draw_pixel(p, r->x + x, r->y + y, p->fill_color);
        }
    }
}

void painter_stroke_rect(painter_t *p, rect_t *r) {
    if (!p || !r) return;
    
    for (uint32_t w = 0; w < p->stroke_width; w++) {
        // Top and bottom
        for (uint32_t x = 0; x < r->width; x++) {
            painter_draw_pixel(p, r->x + x, r->y + w, p->stroke_color);
            painter_draw_pixel(p, r->x + x, r->y + r->height - 1 - w, p->stroke_color);
        }
        // Left and right
        for (uint32_t y = 0; y < r->height; y++) {
            painter_draw_pixel(p, r->x + w, r->y + y, p->stroke_color);
            painter_draw_pixel(p, r->x + r->width - 1 - w, r->y + y, p->stroke_color);
        }
    }
}

void painter_fill_round_rect(painter_t *p, rect_t *r, uint32_t radius) {
    if (!p || !r) return;
    
    if (radius == 0) {
        painter_fill_rect(p, r);
        return;
    }
    
    for (uint32_t y = 0; y < r->height; y++) {
        for (uint32_t x = 0; x < r->width; x++) {
            bool draw = true;
            
            // Corner checks
            if (x < radius && y < radius) { // Top-left
                uint32_t dx = radius - x;
                uint32_t dy = radius - y;
                if (dx * dx + dy * dy > radius * radius) draw = false;
            } else if (x >= r->width - radius && y < radius) { // Top-right
                uint32_t dx = x - (r->width - radius - 1);
                uint32_t dy = radius - y;
                if (dx * dx + dy * dy > radius * radius) draw = false;
            } else if (x < radius && y >= r->height - radius) { // Bottom-left
                uint32_t dx = radius - x;
                uint32_t dy = y - (r->height - radius - 1);
                if (dx * dx + dy * dy > radius * radius) draw = false;
            } else if (x >= r->width - radius && y >= r->height - radius) { // Bottom-right
                uint32_t dx = x - (r->width - radius - 1);
                uint32_t dy = y - (r->height - radius - 1);
                if (dx * dx + dy * dy > radius * radius) draw = false;
            }
            
            if (draw) {
                painter_draw_pixel(p, r->x + x, r->y + y, p->fill_color);
            }
        }
    }
}

void painter_draw_line(painter_t *p, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if (!p) return;
    
    int32_t dx = x2 - x1;
    if (dx < 0) dx = -dx;
    int32_t dy = y2 - y1;
    if (dy < 0) dy = -dy;
    int32_t sx = x1 < x2 ? 1 : -1;
    int32_t sy = y1 < y2 ? 1 : -1;
    int32_t err = dx - dy;
    
    while (1) {
        painter_draw_pixel(p, x1, y1, p->stroke_color);
        if (x1 == x2 && y1 == y2) break;
        int32_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void painter_draw_text(painter_t *p, int32_t x, int32_t y, const char *text) {
    if (!p || !text || !p->current_font) return;
    
    int32_t tx = p->transform_stack[p->transform_depth].tx + x;
    int32_t ty = p->transform_stack[p->transform_depth].ty + y;
    
    // We bypass painter_draw_pixel here and use font_draw_string_bg with clipping 
    // This is a bit inconsistent but font rendering is specialized.
    // TODO: Make font_draw_string respect painter's clip and transform properly.
    font_draw_string(p->current_font, text, p->pixels, p->stride, tx, ty, p->fill_color);
}

void painter_draw_text_rect(painter_t *p, rect_t *r, const char *text, text_align_t align) {
    if (!p || !text || !p->current_font || !r) return;
    
    uint32_t tw, th;
    font_measure_string(p->current_font, text, &tw, &th);
    
    int32_t tx = r->x;
    int32_t ty = r->y + (r->height - th) / 2;
    
    if (align == TEXT_ALIGN_CENTER) {
        tx += (r->width - tw) / 2;
    } else if (align == TEXT_ALIGN_RIGHT) {
        tx += (r->width - tw);
    }
    
    painter_draw_text(p, tx, ty, text);
}
