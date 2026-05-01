#ifndef DEVILCORE_COMPOSITOR_H
#define DEVILCORE_COMPOSITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Pixel format
typedef enum {
    PIXEL_FORMAT_ARGB8888,
    PIXEL_FORMAT_XRGB8888,
    PIXEL_FORMAT_RGBA8888,
    PIXEL_FORMAT_RGB888,
    PIXEL_FORMAT_RGB565
} pixel_format_t;

// Surface types
typedef enum {
    SURFACE_TYPE_WINDOW,
    SURFACE_TYPE_POPUP,
    SURFACE_TYPE_TOOLTIP,
    SURFACE_TYPE_CURSOR,
    SURFACE_TYPE_WALLPAPER,
    SURFACE_TYPE_OVERLAY
} surface_type_t;

// Rectangle for damage tracking
typedef struct rect {
    int32_t x, y;
    uint32_t width, height;
} rect_t;

// Forward declarations
struct surface;
struct compositor;

// A surface is a buffer that can be composited
typedef struct surface {
    uint32_t id;
    surface_type_t type;
    pixel_format_t format;
    
    // Position and size
    int32_t x, y;
    uint32_t width, height;
    
    // Buffers (double buffering per surface)
    uint32_t *front_buffer;
    uint32_t *back_buffer;
    bool dirty;                    // Needs redraw
    
    // Transform
    float scale_x, scale_y;
    float opacity;                 // 0.0 - 1.0
    bool visible;
    
    // Damage tracking (only redraw changed areas)
    rect_t damage_rects[32];
    uint32_t damage_count;
    
    // Z-order
    int32_t z_order;
    
    // Parent/child relationship
    struct surface *parent;
    struct surface *children[64];
    uint32_t child_count;
    
    // Callbacks
    void (*on_expose)(struct surface *surf, rect_t *region);
    void (*on_resize)(struct surface *surf, uint32_t w, uint32_t h);
    
    struct surface *next, *prev;
} surface_t;

// Compositor state
typedef struct compositor {
    // Output (physical display)
    uint32_t *framebuffer;
    uint32_t screen_width;
    uint32_t screen_height;
    uint32_t stride;
    
    // Composition buffer (full screen)
    uint32_t *composite_buffer;
    
    // Surface list (back to front order)
    surface_t *surfaces_head;
    surface_t *surfaces_tail;
    uint32_t surface_count;
    
    // Cursor
    surface_t *cursor_surface;
    int32_t cursor_x, cursor_y;
    int32_t cursor_hot_x, cursor_hot_y;
    
    // Wallpaper
    surface_t *wallpaper;
    
    // Damage regions (dirty screen areas)
    rect_t damage[64];
    uint32_t damage_count;
    bool full_damage;
    
    // VSync
    bool vsync_enabled;
    uint64_t last_frame_time;
    uint32_t target_fps;
    
    // Statistics
    uint64_t frames_rendered;
    uint64_t pixels_composited;
} compositor_t;

// Compositor API
void compositor_init(compositor_t *comp, uint32_t *framebuffer, 
                     uint32_t width, uint32_t height, uint32_t stride);
void compositor_destroy(compositor_t *comp);

// Surface management
surface_t *surface_create(uint32_t width, uint32_t height, 
                          pixel_format_t format, surface_type_t type);
void surface_destroy(surface_t *surf);
void surface_move(surface_t *surf, int32_t x, int32_t y);
void surface_resize(surface_t *surf, uint32_t w, uint32_t h);
void surface_show(surface_t *surf);
void surface_hide(surface_t *surf);
void surface_raise(surface_t *surf);
void surface_lower(surface_t *surf);
void surface_set_z_order(surface_t *surf, int32_t z);
void surface_swap_buffers(surface_t *surf);

// Damage tracking
void compositor_add_damage(compositor_t *comp, rect_t *damage);
void compositor_add_surface_damage(surface_t *surf, rect_t *damage);
void compositor_clear_damage(compositor_t *comp);
bool compositor_needs_redraw(compositor_t *comp);

// Rendering
void compositor_render(compositor_t *comp);
void compositor_present(compositor_t *comp);
void compositor_force_redraw(compositor_t *comp);

// Cursor management
void compositor_set_cursor(compositor_t *comp, surface_t *cursor);
void compositor_move_cursor(compositor_t *comp, int32_t x, int32_t y);

// Wallpaper
void compositor_set_wallpaper(compositor_t *comp, surface_t *wallpaper);

// Helper functions
static inline bool rects_intersect(rect_t *a, rect_t *b) {
    return !(a->x + (int32_t)a->width <= b->x ||
             b->x + (int32_t)b->width <= a->x ||
             a->y + (int32_t)a->height <= b->y ||
             b->y + (int32_t)b->height <= a->y);
}

static inline bool rects_adjacent(rect_t *a, rect_t *b) {
    int32_t gap_x = (a->x < b->x) ? (b->x - (a->x + (int32_t)a->width))
                                  : (a->x - (b->x + (int32_t)b->width));
    int32_t gap_y = (a->y < b->y) ? (b->y - (a->y + (int32_t)a->height))
                                  : (a->y - (b->y + (int32_t)b->height));
    return gap_x <= 1 && gap_y <= 1;
}

static inline void rect_merge(rect_t *a, rect_t *b) {
    int32_t x1 = (a->x < b->x) ? a->x : b->x;
    int32_t y1 = (a->y < b->y) ? a->y : b->y;
    int32_t x2 = (a->x + (int32_t)a->width > b->x + (int32_t)b->width) 
                 ? (a->x + (int32_t)a->width) : (b->x + (int32_t)b->width);
    int32_t y2 = (a->y + (int32_t)a->height > b->y + (int32_t)b->height)
                 ? (a->y + (int32_t)a->height) : (b->y + (int32_t)b->height);
    a->x = x1;
    a->y = y1;
    a->width = x2 - x1;
    a->height = y2 - y1;
}

// Global compositor instance
extern compositor_t *g_compositor;

// Convenience macros
#define COMPOSITOR() g_compositor

#endif // DEVILCORE_COMPOSITOR_H
