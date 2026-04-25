/*
 * DevilCore Window Manager
 * Manages windows, taskbar, and desktop environment
 */

#include "compositor.h"
#include <stddef.h>

#define MAX_WINDOWS 10

typedef struct {
    int x, y, w, h;
    const char *title;
    uint32_t color;
    int active;
} Window;

static Window windows[MAX_WINDOWS];
static int num_windows = 0;

/* Desktop colors - Dark grey / Black theme */
#define COLOR_DESKTOP     0xFF0A0A0A
#define COLOR_TASKBAR     0xFF1A1A1A
#define COLOR_WINDOW_BG   0xFF121212
#define COLOR_WINDOW_HDR  0xFF202020
#define COLOR_GREEN       0xFF00FF00
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_GREY        0xFF808080

void window_init() {
    num_windows = 0;
}

void create_window(int x, int y, int w, int h, const char *title) {
    if (num_windows >= MAX_WINDOWS) return;
    
    windows[num_windows].x = x;
    windows[num_windows].y = y;
    windows[num_windows].w = w;
    windows[num_windows].h = h;
    windows[num_windows].title = title;
    windows[num_windows].active = 1;
    num_windows++;
}

void draw_window(Window *win) {
    /* Main Background */
    draw_rect(win->x, win->y, win->w, win->h, COLOR_WINDOW_BG);
    
    /* Header Bar */
    draw_rect(win->x, win->y, win->w, 24, COLOR_WINDOW_HDR);
    draw_rect_outline(win->x, win->y, win->w, win->h, COLOR_TASKBAR);
    
    /* Title text */
    draw_string(win->x + 8, win->y + 8, win->title, COLOR_WHITE, 0xFF000000);
    
    /* Close button placeholder (just a red dot) */
    draw_rect(win->x + win->w - 20, win->y + 4, 16, 16, 0xFFFF0000);
}

void draw_taskbar() {
    int sw = compositor_get_width();
    int sh = compositor_get_height();
    
    /* Taskbar background */
    draw_rect(0, sh - 40, sw, 40, COLOR_TASKBAR);
    
    /* Start Button - Devil Icon placeholder */
    draw_rect(5, sh - 35, 80, 30, COLOR_WINDOW_HDR);
    draw_string(15, sh - 25, "DEVIL", COLOR_GREEN, 0xFF000000);
    
    /* Time placeholder */
    draw_string(sw - 80, sh - 25, "13:37", COLOR_GREY, 0xFF000000);
}

static char terminal_buffer[1024] = "devilcore@root:~$ _";

void draw_desktop() {
    int sw = compositor_get_width();
    int sh = compositor_get_height();
    
    /* Paint background */
    draw_rect(0, 0, sw, sh, COLOR_DESKTOP);
    
    /* Background Logo */
    draw_string2x(sw/2 - 100, sh/2 - 20, "DEVILCORE", 0xFF151515);
    
    /* Draw windows */
    for (int i = 0; i < num_windows; i++) {
        if (windows[i].active) {
            draw_window(&windows[i]);
            
            /* Special handling for terminal content */
            if (i == 0) {
                draw_string(windows[i].x + 10, windows[i].y + 35, terminal_buffer, COLOR_GREEN, 0xFF000000);
            }
        }
    }
    
    draw_taskbar();
}

void window_update() {
    /* Handle minimal interaction here or in kernel main */
}
