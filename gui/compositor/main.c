// gui/compositor/main.c
// DevilUI - GUI Compositor

#include <devil/types.h>
#include "gui.h"

static window_t *windows;
static uint32_t next_window_id = 1;
static uint32_t focused_window;
static uint32_t width = 1024;
static uint32_t height = 768;
static uint32_t *framebuffer;

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = y; i < y + h && i < height; i++) {
        for (uint32_t j = x; j < x + w && j < width; j++) {
            framebuffer[i * width + j] = color;
        }
    }
}

static void draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    // Simple 8x16 bitmap font
    // Would draw character bitmap at position
    framebuffer[y * width + x] = color;
}

void gui_init(void) {
    width = 1024;
    height = 768;
    
    // Allocate framebuffer
    framebuffer = (uint32_t*)malloc(width * height * 4);
    
    // Draw desktop background
    draw_rect(0, 0, width, height, 0xFF1a1a2e);
    
    windows = NULL;
    focused_window = 0;
    
    printf("[DevilUI] GUI Compositor initialized\n");
}

window_t *window_create(const char *title, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    window_t *win = (window_t*)malloc(sizeof(window_t));
    
    win->id = next_window_id++;
    strncpy(win->title, title, 255);
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->bg_color = 0xFF2d2d44;
    win->content = NULL;
    
    // Add to window list
    win->prev = NULL;
    win->next = windows;
    if (windows) {
        windows->prev = win;
    }
    windows = win;
    
    window_show(win);
    
    return win;
}

void window_show(window_t *win) {
    if (!win) return;
    
    // Draw window
    draw_rect(win->x, win->y, win->width, win->height, win->bg_color);
    draw_rect(win->x, win->y, win->width, 1, 0xFF6ecf6e);
    
    // Draw title bar text
    draw_char(win->x + 4, win->y + 4, 'X', 0xFF000000);
}

void window_hide(window_t *win) {
    // Mark as hidden
    win->prev = NULL;
    win->next = NULL;
}

void window_move(window_t *win, uint32_t x, uint32_t y) {
    win->x = x;
    win->y = y;
    window_show(win);
}

void window_resize(window_t *win, uint32_t w, uint32_t h) {
    win->width = w;
    win->height = h;
    window_show(win);
}