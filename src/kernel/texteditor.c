#include "texteditor.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include <string.h>
#include <stdio.h>

extern struct wm_desktop *desktop;

static struct wm_window *te_window = NULL;
static char text_buffer[4096] = "Welcome to DevilCore Text Editor\n\nType your text here...\n";
static uint32_t cursor_pos = 0;
static uint32_t scroll_offset = 0;

static void te_draw_content(struct wm_window *win) {
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar
    fb_fill_rect(x, y, w, 30, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, 30, 0x00505050);
    fb_draw_string(x + 10, y + 10, "[New] [Open] [Save]", 0x00aaaaaa, 0);
    
    // Text area
    y += 30;
    uint32_t text_h = h - 54;
    fb_fill_rect(x, y, w, text_h, 0x00ffffff);
    fb_draw_rect_outline(x, y, w, text_h, 0x00505050);
    
    // Draw text with line wrapping
    uint32_t text_x = x + 10;
    uint32_t text_y = y + 10;
    uint32_t chars_per_line = (w - 20) / 8;
    uint32_t line = 0;
    
    char *ptr = text_buffer;
    while (*ptr && text_y < y + text_h - 20) {
        char line_buf[256];
        uint32_t i = 0;
        
        // Get one line of text
        while (*ptr && *ptr != '\n' && i < chars_per_line && i < 255) {
            line_buf[i++] = *ptr++;
        }
        if (*ptr == '\n') ptr++;
        line_buf[i] = '\0';
        
        // Draw the line
        if (line >= scroll_offset) {
            fb_draw_string(text_x, text_y, line_buf, 0x00000000, 0x00ffffff);
            text_y += 16;
        }
        line++;
        
        if (text_y >= y + text_h - 20) break;
    }
    
    // Draw cursor if window is focused
    if (win->widget.focused && wm_is_cursor_visible()) {
        // Simplified cursor position
        fb_fill_rect(text_x + (cursor_pos % chars_per_line) * 8, 
                     y + 10 + (cursor_pos / chars_per_line - scroll_offset) * 16,
                     2, 16, 0x00000000);
    }
    
    // Status bar
    y = win->widget.y + h - 24;
    fb_fill_rect_gradient_h(x, y, w, 24, 0x002a2a2a, 0x00303030);
    char status[64];
    sprintf(status, "Lines: %d | Cursor: %d", line, cursor_pos);
    fb_draw_string(x + 6, y + 8, status, 0x00aaaaaa, 0);
}

static void te_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL) return;
    
    // Let the default window handler check for title bar buttons first
    if (wm_window_on_event(widget, event)) {
        return; // Event was handled by title bar button
    }
    
    if (event->type == WM_EVENT_KEY) {
        uint8_t key = event->key;
        
        if (key == 'q' || key == 27) { // Q or ESC
            wm_destroy_window(win);
            te_window = NULL;
        } else if (key == '\b') { // Backspace
            if (cursor_pos > 0) {
                text_buffer[cursor_pos - 1] = '\0';
                cursor_pos--;
            }
        } else if (key >= 32 && key <= 126) { // Printable characters
            if (cursor_pos < 4095) {
                text_buffer[cursor_pos++] = (char)key;
                text_buffer[cursor_pos] = '\0';
            }
        } else if (key == '\n') {
            if (cursor_pos < 4095) {
                text_buffer[cursor_pos++] = '\n';
                text_buffer[cursor_pos] = '\0';
            }
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            // Check if click is in toolbar area
            if (event->y >= 0 && event->y < 30) {
                if (event->x >= 10 && event->x < 50) { // [New]
                    memset(text_buffer, 0, sizeof(text_buffer));
                    cursor_pos = 0;
                } else if (event->x >= 60 && event->x < 110) { // [Open]
                    strcpy(text_buffer, "Opening file...\nFeature coming soon!");
                    cursor_pos = strlen(text_buffer);
                } else if (event->x >= 120 && event->x < 170) { // [Save]
                    strcpy(text_buffer, "Saving file...\nFeature coming soon!");
                    cursor_pos = strlen(text_buffer);
                }
            }
        }
    }
}

void te_open(void) {
    if (te_window != NULL) {
        wm_focus_window(te_window);
        return;
    }
    
    te_window = wm_create_window("Text Editor", 120, 100, 650, 450);
    if (te_window != NULL) {
        te_window->draw_content = te_draw_content;
        te_window->widget.on_event = te_event;
        te_window->bg_color = 0x00ffffff;
    }
}
