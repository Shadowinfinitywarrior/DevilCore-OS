#include "compositor.h"
#include "memory.h"
#include "kprintf.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);

static uint32_t next_surface_id = 1;
compositor_t *g_compositor = NULL;

void compositor_init(compositor_t *comp, uint32_t *framebuffer, 
                     uint32_t width, uint32_t height, uint32_t stride) {
    comp->framebuffer = framebuffer;
    comp->screen_width = width;
    comp->screen_height = height;
    comp->stride = stride;
    
    // Allocate composition buffer
    size_t buffer_size = height * stride * sizeof(uint32_t);
    comp->composite_buffer = kmalloc(buffer_size);
    if (!comp->composite_buffer) {
        kprintf("Failed to allocate composition buffer\n");
        return;
    }
    
    // Clear to black
    memset(comp->composite_buffer, 0, buffer_size);
    
    // Initialize surface list
    comp->surfaces_head = NULL;
    comp->surfaces_tail = NULL;
    comp->surface_count = 0;
    
    // Initialize cursor
    comp->cursor_surface = NULL;
    comp->cursor_x = (int32_t)width / 2;
    comp->cursor_y = (int32_t)height / 2;
    comp->cursor_hot_x = 0;
    comp->cursor_hot_y = 0;
    
    // Initialize wallpaper
    comp->wallpaper = NULL;
    
    // Initialize damage tracking
    comp->damage_count = 0;
    comp->full_damage = true;  // Force first full redraw
    
    // Initialize VSync settings
    comp->vsync_enabled = false;
    comp->last_frame_time = 0;
    comp->target_fps = 60;
    
    // Initialize statistics
    comp->frames_rendered = 0;
    comp->pixels_composited = 0;
    
    g_compositor = comp;
    
    kprintf("Compositor initialized: %dx%d @ %d stride\n", width, height, stride);
}

void compositor_destroy(compositor_t *comp) {
    if (!comp) return;
    
    // Destroy all surfaces
    surface_t *surf = comp->surfaces_head;
    while (surf) {
        surface_t *next = surf->next;
        surface_destroy(surf);
        surf = next;
    }
    
    if (comp->composite_buffer) {
        kfree(comp->composite_buffer);
    }
    
    if (g_compositor == comp) {
        g_compositor = NULL;
    }
}

surface_t *surface_create(uint32_t width, uint32_t height, 
                          pixel_format_t format, surface_type_t type) {
    surface_t *surf = kmalloc(sizeof(surface_t));
    if (!surf) return NULL;
    
    memset(surf, 0, sizeof(surface_t));
    
    surf->id = next_surface_id++;
    surf->type = type;
    surf->format = format;
    surf->width = width;
    surf->height = height;
    surf->scale_x = 1.0f;
    surf->scale_y = 1.0f;
    surf->opacity = 1.0f;
    surf->visible = true;
    surf->z_order = 0;
    
    // Allocate front and back buffers
    size_t buffer_size = width * height * sizeof(uint32_t);
    
    surf->front_buffer = kmalloc(buffer_size);
    if (!surf->front_buffer) {
        kfree(surf);
        return NULL;
    }
    
    surf->back_buffer = kmalloc(buffer_size);
    if (!surf->back_buffer) {
        kfree(surf->front_buffer);
        kfree(surf);
        return NULL;
    }
    
    // Clear buffers
    memset(surf->front_buffer, 0, buffer_size);
    memset(surf->back_buffer, 0, buffer_size);
    
    surf->damage_count = 0;
    surf->dirty = true;
    
    return surf;
}

void surface_destroy(surface_t *surf) {
    if (!surf) return;
    
    // Remove from compositor if attached
    if (g_compositor) {
        if (g_compositor->cursor_surface == surf) {
            g_compositor->cursor_surface = NULL;
        }
        if (g_compositor->wallpaper == surf) {
            g_compositor->wallpaper = NULL;
        }
        
        // Remove from surface list
        if (surf->prev) {
            surf->prev->next = surf->next;
        } else {
            g_compositor->surfaces_head = surf->next;
        }
        if (surf->next) {
            surf->next->prev = surf->prev;
        } else {
            g_compositor->surfaces_tail = surf->prev;
        }
        
        g_compositor->surface_count--;
    }
    
    // Free buffers
    if (surf->front_buffer) kfree(surf->front_buffer);
    if (surf->back_buffer) kfree(surf->back_buffer);
    
    kfree(surf);
}

void surface_move(surface_t *surf, int32_t x, int32_t y) {
    if (!surf) return;
    
    // Add old position to damage
    if (g_compositor && surf->visible) {
        rect_t old_pos = {surf->x, surf->y, surf->width, surf->height};
        compositor_add_damage(g_compositor, &old_pos);
    }
    
    surf->x = x;
    surf->y = y;
    
    // Add new position to damage
    if (g_compositor && surf->visible) {
        rect_t new_pos = {x, y, surf->width, surf->height};
        compositor_add_damage(g_compositor, &new_pos);
    }
}

void surface_resize(surface_t *surf, uint32_t w, uint32_t h) {
    if (!surf || (w == 0) || (h == 0)) return;
    
    // Call resize callback
    if (surf->on_resize) {
        surf->on_resize(surf, w, h);
    }
    
    // Reallocate buffers
    size_t new_size = w * h * sizeof(uint32_t);
    uint32_t *new_front = kmalloc(new_size);
    uint32_t *new_back = kmalloc(new_size);
    
    if (!new_front || !new_back) {
        if (new_front) kfree(new_front);
        if (new_back) kfree(new_back);
        return;
    }
    
    // Clear new buffers
    memset(new_front, 0, new_size);
    memset(new_back, 0, new_size);
    
    // Free old buffers
    if (surf->front_buffer) kfree(surf->front_buffer);
    if (surf->back_buffer) kfree(surf->back_buffer);
    
    surf->front_buffer = new_front;
    surf->back_buffer = new_back;
    surf->width = w;
    surf->height = h;
    surf->dirty = true;
    
    // Mark full damage
    if (g_compositor) {
        rect_t damage = {surf->x, surf->y, w, h};
        compositor_add_damage(g_compositor, &damage);
    }
}

void surface_show(surface_t *surf) {
    if (!surf || surf->visible) return;
    surf->visible = true;
    surf->dirty = true;
    
    if (g_compositor) {
        rect_t damage = {surf->x, surf->y, surf->width, surf->height};
        compositor_add_damage(g_compositor, &damage);
    }
}

void surface_hide(surface_t *surf) {
    if (!surf || !surf->visible) return;
    surf->visible = false;
    
    if (g_compositor) {
        rect_t damage = {surf->x, surf->y, surf->width, surf->height};
        compositor_add_damage(g_compositor, &damage);
    }
}

void surface_raise(surface_t *surf) {
    if (!surf || !g_compositor) return;
    
    // Remove from current position
    if (surf->prev) surf->prev->next = surf->next;
    else g_compositor->surfaces_head = surf->next;
    
    if (surf->next) surf->next->prev = surf->prev;
    else g_compositor->surfaces_tail = surf->prev;
    
    // Add to end (top)
    surf->prev = g_compositor->surfaces_tail;
    surf->next = NULL;
    if (g_compositor->surfaces_tail) {
        g_compositor->surfaces_tail->next = surf;
    }
    g_compositor->surfaces_tail = surf;
    if (!g_compositor->surfaces_head) {
        g_compositor->surfaces_head = surf;
    }
    
    // Mark damage
    rect_t damage = {surf->x, surf->y, surf->width, surf->height};
    compositor_add_damage(g_compositor, &damage);
}

void surface_lower(surface_t *surf) {
    if (!surf || !g_compositor) return;
    
    // Remove from current position
    if (surf->prev) surf->prev->next = surf->next;
    else g_compositor->surfaces_head = surf->next;
    
    if (surf->next) surf->next->prev = surf->prev;
    else g_compositor->surfaces_tail = surf->prev;
    
    // Add to beginning (bottom)
    surf->next = g_compositor->surfaces_head;
    surf->prev = NULL;
    if (g_compositor->surfaces_head) {
        g_compositor->surfaces_head->prev = surf;
    }
    g_compositor->surfaces_head = surf;
    if (!g_compositor->surfaces_tail) {
        g_compositor->surfaces_tail = surf;
    }
    
    // Mark full damage since z-order changed
    compositor_force_redraw(g_compositor);
}

void surface_set_z_order(surface_t *surf, int32_t z) {
    if (!surf || !g_compositor) return;
    
    if (surf->z_order == z) return;
    surf->z_order = z;
    
    // Re-sort surfaces by z-order
    // First, remove from current position
    if (surf->prev) surf->prev->next = surf->next;
    else g_compositor->surfaces_head = surf->next;
    
    if (surf->next) surf->next->prev = surf->prev;
    else g_compositor->surfaces_tail = surf->prev;
    
    // Find new position
    surface_t *current = g_compositor->surfaces_head;
    surface_t *prev = NULL;
    
    while (current && current->z_order <= z) {
        prev = current;
        current = current->next;
    }
    
    // Insert after prev, before current
    surf->next = current;
    surf->prev = prev;
    
    if (prev) prev->next = surf;
    else g_compositor->surfaces_head = surf;
    
    if (current) current->prev = surf;
    else g_compositor->surfaces_tail = surf;
    
    // Mark full damage since z-order changed
    compositor_force_redraw(g_compositor);
}

void surface_swap_buffers(surface_t *surf) {
    if (!surf) return;
    
    uint32_t *temp = surf->front_buffer;
    surf->front_buffer = surf->back_buffer;
    surf->back_buffer = temp;
    surf->dirty = true;
    
    // Add entire surface to damage
    if (g_compositor && surf->visible) {
        rect_t damage = {surf->x, surf->y, surf->width, surf->height};
        compositor_add_damage(g_compositor, &damage);
    }
}

void compositor_add_surface_damage(surface_t *surf, rect_t *damage) {
    if (!surf || !damage || surf->damage_count >= 32) return;
    
    // Clip to surface bounds
    int32_t x1 = damage->x;
    int32_t y1 = damage->y;
    int32_t x2 = x1 + (int32_t)damage->width;
    int32_t y2 = y1 + (int32_t)damage->height;
    
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > (int32_t)surf->width) x2 = (int32_t)surf->width;
    if (y2 > (int32_t)surf->height) y2 = (int32_t)surf->height;
    
    if (x1 >= x2 || y1 >= y2) return;
    
    rect_t clipped = {
        .x = x1,
        .y = y1,
        .width = (uint32_t)(x2 - x1),
        .height = (uint32_t)(y2 - y1)
    };
    
    // Try to merge with existing damage
    for (uint32_t i = 0; i < surf->damage_count; i++) {
        if (rects_intersect(&surf->damage_rects[i], &clipped) ||
            rects_adjacent(&surf->damage_rects[i], &clipped)) {
            rect_merge(&surf->damage_rects[i], &clipped);
            return;
        }
    }
    
    // Add new damage region
    surf->damage_rects[surf->damage_count++] = clipped;
    surf->dirty = true;
}

void compositor_add_damage(compositor_t *comp, rect_t *damage) {
    if (!comp || !damage || comp->full_damage) return;
    
    // Clip to screen bounds
    int32_t x1 = damage->x;
    int32_t y1 = damage->y;
    int32_t x2 = x1 + (int32_t)damage->width;
    int32_t y2 = y1 + (int32_t)damage->height;
    
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > (int32_t)comp->screen_width) x2 = (int32_t)comp->screen_width;
    if (y2 > (int32_t)comp->screen_height) y2 = (int32_t)comp->screen_height;
    
    if (x1 >= x2 || y1 >= y2) return;
    
    rect_t clipped = {
        .x = x1,
        .y = y1,
        .width = (uint32_t)(x2 - x1),
        .height = (uint32_t)(y2 - y1)
    };
    
    // Try to merge with existing damage
    for (uint32_t i = 0; i < comp->damage_count; i++) {
        if (rects_intersect(&comp->damage[i], &clipped) ||
            rects_adjacent(&comp->damage[i], &clipped)) {
            rect_merge(&comp->damage[i], &clipped);
            return;
        }
    }
    
    // Add new damage region
    if (comp->damage_count < 64) {
        comp->damage[comp->damage_count++] = clipped;
    } else {
        // Too many regions, mark full screen dirty
        comp->full_damage = true;
        comp->damage_count = 0;
    }
}

void compositor_clear_damage(compositor_t *comp) {
    if (!comp) return;
    comp->damage_count = 0;
    comp->full_damage = false;
}

bool compositor_needs_redraw(compositor_t *comp) {
    if (!comp) return false;
    return comp->full_damage || comp->damage_count > 0;
}

void compositor_force_redraw(compositor_t *comp) {
    if (!comp) return;
    comp->full_damage = true;
    comp->damage_count = 0;
}

// Blend a single pixel with alpha
static inline uint32_t blend_pixel(uint32_t src, uint32_t dst) {
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

// Composite a surface region to the composite buffer
static void compositor_blit_surface_region(compositor_t *comp, 
                                            surface_t *surf,
                                            int32_t rx, int32_t ry,
                                            uint32_t rw, uint32_t rh) {
    if (!surf->visible || !surf->front_buffer) return;
    
    // Calculate intersection
    int32_t surf_x2 = surf->x + (int32_t)surf->width;
    int32_t surf_y2 = surf->y + (int32_t)surf->height;
    int32_t r_x2 = rx + (int32_t)rw;
    int32_t r_y2 = ry + (int32_t)rh;
    
    int32_t x1 = (surf->x > rx) ? surf->x : rx;
    int32_t y1 = (surf->y > ry) ? surf->y : ry;
    int32_t x2 = (surf_x2 < r_x2) ? surf_x2 : r_x2;
    int32_t y2 = (surf_y2 < r_y2) ? surf_y2 : r_y2;
    
    if (x1 >= x2 || y1 >= y2) return;
    
    // Blit pixels
    int32_t src_x = x1 - surf->x;
    int32_t src_y = y1 - surf->y;
    int32_t dst_x = x1;
    int32_t dst_y = y1;
    uint32_t w = (uint32_t)(x2 - x1);
    uint32_t h = (uint32_t)(y2 - y1);
    
    // Apply opacity scaling
    uint8_t opacity = (uint8_t)(surf->opacity * 255.0f);
    
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t src_idx = (src_y + y) * surf->width + (src_x + x);
            uint32_t dst_idx = (dst_y + y) * comp->stride + (dst_x + x);
            
            uint32_t src_pixel = surf->front_buffer[src_idx];
            
            // Apply surface opacity
            if (opacity < 255) {
                uint8_t src_a = ((src_pixel >> 24) & 0xFF) * opacity / 255;
                src_pixel = (src_a << 24) | (src_pixel & 0xFFFFFF);
            }
            
            comp->composite_buffer[dst_idx] = blend_pixel(src_pixel, 
                comp->composite_buffer[dst_idx]);
        }
    }
    
    comp->pixels_composited += w * h;
}

// Composite wallpaper
static void compositor_render_wallpaper(compositor_t *comp,
                                         int32_t rx, int32_t ry,
                                         uint32_t rw, uint32_t rh) {
    if (!comp->wallpaper || !comp->wallpaper->front_buffer) {
        // Fill with black
        for (uint32_t y = 0; y < rh; y++) {
            uint32_t dst_row_idx = (ry + y) * comp->stride + rx;
            for (uint32_t x = 0; x < rw; x++) {
                comp->composite_buffer[dst_row_idx + x] = 0xFF000000;
            }
        }
        return;
    }
    
    // Tile wallpaper
    uint32_t wp_w = comp->wallpaper->width;
    uint32_t wp_h = comp->wallpaper->height;
    uint32_t *wp_buf = comp->wallpaper->front_buffer;
    
    for (uint32_t y = 0; y < rh; y++) {
        uint32_t dst_row_idx = (ry + y) * comp->stride + rx;
        uint32_t wp_y = (ry + y) % wp_h;
        uint32_t wp_row_idx = wp_y * wp_w;
        
        for (uint32_t x = 0; x < rw; x++) {
            uint32_t wp_x = (rx + x) % wp_w;
            comp->composite_buffer[dst_row_idx + x] = wp_buf[wp_row_idx + wp_x];
        }
    }
}

// Render a region
static void compositor_render_region(compositor_t *comp,
                                      int32_t rx, int32_t ry,
                                      uint32_t rw, uint32_t rh) {
    // First: render wallpaper/background
    compositor_render_wallpaper(comp, rx, ry, rw, rh);
    
    // Then: render all surfaces in z-order (back to front)
    surface_t *surf = comp->surfaces_head;
    while (surf) {
        if (surf->visible && surf != comp->cursor_surface) {
            compositor_blit_surface_region(comp, surf, rx, ry, rw, rh);
        }
        surf = surf->next;
    }
    
    // Finally: render cursor on top
    if (comp->cursor_surface && comp->cursor_surface->visible) {
        surface_t *cursor = comp->cursor_surface;
        int32_t cursor_x = comp->cursor_x - comp->cursor_hot_x;
        int32_t cursor_y = comp->cursor_y - comp->cursor_hot_y;
        
        // Temporarily adjust cursor position for rendering
        int32_t old_x = cursor->x;
        int32_t old_y = cursor->y;
        cursor->x = cursor_x;
        cursor->y = cursor_y;
        
        compositor_blit_surface_region(comp, cursor, rx, ry, rw, rh);
        
        cursor->x = old_x;
        cursor->y = old_y;
    }
}

void compositor_render(compositor_t *comp) {
    if (!comp || !compositor_needs_redraw(comp)) return;
    
    if (comp->full_damage) {
        // Redraw everything
        compositor_render_region(comp, 0, 0, 
                                  comp->screen_width, 
                                  comp->screen_height);
        comp->full_damage = false;
    } else {
        // Only redraw damaged regions
        for (uint32_t i = 0; i < comp->damage_count; i++) {
            rect_t *r = &comp->damage[i];
            compositor_render_region(comp, r->x, r->y, 
                                      r->width, r->height);
        }
    }
    
    comp->damage_count = 0;
    comp->frames_rendered++;
}

void compositor_present(compositor_t *comp) {
    if (!comp || !comp->framebuffer || !comp->composite_buffer) return;
    
    // Copy composite buffer to framebuffer
    for (uint32_t y = 0; y < comp->screen_height; y++) {
        memcpy(&comp->framebuffer[y * comp->stride],
               &comp->composite_buffer[y * comp->stride],
               comp->screen_width * sizeof(uint32_t));
    }
}

void compositor_set_cursor(compositor_t *comp, surface_t *cursor) {
    if (!comp) return;
    
    // Remove old cursor damage
    if (comp->cursor_surface) {
        rect_t damage = {
            comp->cursor_x - comp->cursor_hot_x,
            comp->cursor_y - comp->cursor_hot_y,
            comp->cursor_surface->width,
            comp->cursor_surface->height
        };
        compositor_add_damage(comp, &damage);
    }
    
    comp->cursor_surface = cursor;
    
    // Add new cursor damage
    if (cursor) {
        rect_t damage = {
            comp->cursor_x - comp->cursor_hot_x,
            comp->cursor_y - comp->cursor_hot_y,
            cursor->width,
            cursor->height
        };
        compositor_add_damage(comp, &damage);
    }
}

void compositor_move_cursor(compositor_t *comp, int32_t x, int32_t y) {
    if (!comp) return;
    
    // Add old position to damage
    if (comp->cursor_surface) {
        rect_t old_damage = {
            comp->cursor_x - comp->cursor_hot_x,
            comp->cursor_y - comp->cursor_hot_y,
            comp->cursor_surface->width,
            comp->cursor_surface->height
        };
        compositor_add_damage(comp, &old_damage);
    }
    
    comp->cursor_x = x;
    comp->cursor_y = y;
    
    // Add new position to damage
    if (comp->cursor_surface) {
        rect_t new_damage = {
            x - comp->cursor_hot_x,
            y - comp->cursor_hot_y,
            comp->cursor_surface->width,
            comp->cursor_surface->height
        };
        compositor_add_damage(comp, &new_damage);
    }
}

void compositor_set_wallpaper(compositor_t *comp, surface_t *wallpaper) {
    if (!comp) return;
    comp->wallpaper = wallpaper;
    compositor_force_redraw(comp);
}
