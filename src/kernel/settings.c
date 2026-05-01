#include "settings.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>

extern struct wm_desktop *desktop;

static struct wm_window *settings_window = NULL;
static uint8_t selected_tab = 0; // 0=General, 1=Display, 2=About
static uint8_t theme_idx = 0;

// Predefined themes
static const char *theme_names[] = {"Dark", "Blue", "Green", "Purple"};
static uint32_t theme_colors[][3] = {
    {0x001a1a1a, 0x002d2d2d, 0x000055aa}, // Dark
    {0x001a1a2a, 0x002d2d3d, 0x000000aa}, // Blue
    {0x001a2a1a, 0x002d3d2d, 0x0000aa00}, // Green
    {0x002a1a2a, 0x003d2d3d, 0x00aa00aa}  // Purple
};
#define NUM_THEMES (sizeof(theme_names) / sizeof(theme_names[0]))

void settings_draw(struct wm_window *win) {
    if (win == NULL || desktop == NULL) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Tab bar
    uint32_t tab_w = w / 3;
    const char *tabs[] = {"General", "Display", "About"};
    
    for (int i = 0; i < 3; ++i) {
        uint32_t tab_color = (i == selected_tab) ? 0x00444444 : 0x002a2a2a;
        fb_fill_rect_rounded(x + i * tab_w, y, tab_w - 2, 30, 4, tab_color);
        fb_draw_string(x + i * tab_w + 10, y + 10, tabs[i], 0x00ffffff, 0);
    }
    
    // Content area
    y += 30;
    uint32_t content_h = h - 30;
    fb_fill_rect(x, y, w, content_h, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, content_h, 0x00505050);
    
    if (selected_tab == 0) {
        // General settings
        fb_draw_string(x + 10, y + 15, "Theme Selection:", 0x00ffffff, 0);
        
        // Theme buttons
        for (uint32_t i = 0; i < NUM_THEMES; ++i) {
            uint32_t btn_y = y + 40 + i * 35;
            uint32_t btn_color = (i == theme_idx) ? 0x00444444 : 0x00333333;
            
            fb_fill_rect_rounded(x + 10, btn_y, 200, 30, 4, btn_color);
            fb_draw_string(x + 20, btn_y + 10, theme_names[i], 0x00ffffff, 0);
            
            // Color preview
            fb_fill_rect(x + 220, btn_y + 5, 20, 20, theme_colors[i][2]);
        }
        
        // Apply button
        fb_fill_rect_rounded(x + 10, y + content_h - 40, 100, 30, 4, 0x000055aa);
        fb_draw_string(x + 35, y + content_h - 30, "Apply", 0x00ffffff, 0);
        
    } else if (selected_tab == 1) {
        // Display settings
        fb_draw_string(x + 10, y + 15, "Display Settings", 0x00ffffff, 0);
        fb_draw_string(x + 10, y + 45, "Resolution: 1024x768", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 75, "Color Depth: 32-bit", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 105, "Refresh Rate: 60Hz", 0x00aaaaaa, 0);
        
    } else if (selected_tab == 2) {
        // About
        fb_draw_string(x + 10, y + 20, "DevilCore OS v0.1", 0x0000ff00, 0);
        fb_draw_string(x + 10, y + 40, "Ethical Hacking Environment", 0x00ffffff, 0);
        fb_draw_string(x + 10, y + 70, "Kernel: Monolithic x86_64", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 90, "Bootloader: Limine", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 110, "GCC: x86_64-linux-gnu-gcc", 0x00aaaaaa, 0);
    }
}

void settings_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL || desktop == NULL) return;
    
    // Let the default window handler check for title bar buttons first
    if (wm_window_on_event(widget, event)) {
        return; // Event was handled by title bar button
    }
    
    if (event->type == WM_EVENT_KEY) {
        uint8_t key = event->key;
        if (key == 'q' || key == 27) { // Q or ESC
            wm_destroy_window(win);
            settings_window = NULL;
        } else if (key >= '1' && key <= '3') {
            selected_tab = key - '1';
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            // Check tab clicks
            uint32_t x = 0; // Relative to widget
            uint32_t y = 0; // Relative to widget
            uint32_t tab_w = widget->width / 3;
            
            if (event->y >= (int32_t)y && event->y < (int32_t)(y + 30)) {
                for (int i = 0; i < 3; ++i) {
                    if (event->x >= (int32_t)(x + i * tab_w) && 
                        event->x < (int32_t)(x + (i + 1) * tab_w)) {
                        selected_tab = (uint8_t)i;
                        break;
                    }
                }
            }
            
            // Check theme selection (General tab)
            if (selected_tab == 0) {
                y = 30; // Start of content area
                for (uint32_t i = 0; i < NUM_THEMES; ++i) {
                    uint32_t btn_y = y + 40 + i * 35;
                    if (event->x >= 10 && event->x < 210 &&
                        event->y >= (int32_t)btn_y && event->y < (int32_t)(btn_y + 30)) {
                        theme_idx = (uint8_t)i;
                        break;
                    }
                }
                
                // Apply button
                y = 30 + widget->height - 30 - 40;
                if (event->x >= 10 && event->x < 110 &&
                    event->y >= (int32_t)y && event->y < (int32_t)(y + 30)) {
                    // Apply theme
                    struct wm_theme new_theme = current_theme;
                    new_theme.bg_color = theme_colors[theme_idx][0];
                    new_theme.fg_color = theme_colors[theme_idx][1];
                    new_theme.accent_color = theme_colors[theme_idx][2];
                    wm_set_theme(new_theme);
                }
            }
        }
    }
}

void settings_open(void) {
    if (settings_window != NULL) {
        wm_focus_window(settings_window);
        return;
    }
    
    settings_window = wm_create_window("Settings", 200, 100, 500, 400);
    if (settings_window != NULL) {
        settings_window->draw_content = (void (*)(struct wm_window *))settings_draw;
        settings_window->widget.on_event = settings_on_event;
        settings_window->bg_color = 0x002a2a2a;
    }
}
