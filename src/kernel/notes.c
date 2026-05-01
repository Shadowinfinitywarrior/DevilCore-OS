#include "notes.h"
#include "wm.h"
#include "framebuffer.h"
#include "vfs.h"
#include "memory.h"
#include "timer.h"
#include <string.h>

#define NOTES_MAX_LINES 100
#define NOTES_LINE_LEN 128

struct notes_data {
    char lines[NOTES_MAX_LINES][NOTES_LINE_LEN];
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t line_count;
};

static struct wm_window *notes_window = NULL;

static void notes_draw_content(struct wm_window *win) {
    struct notes_data *data = (struct notes_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar (Dark theme for notes)
    fb_fill_rect(x, y, w, 30, 0x00222222);
    fb_draw_rect_outline(x, y, w, 30, 0x00444444);
    fb_draw_string(x + 10, y + 10, "Quick Notes", 0x0044ffcc, 0);
    
    // Main area (Dark gray paper)
    uint32_t content_y = y + 30;
    uint32_t content_h = h - 30;
    fb_fill_rect(x, content_y, w, content_h, 0x001a1a1a);
    
    // Draw lines
    for (uint32_t i = 0; i < data->line_count; i++) {
        uint32_t line_y = content_y + 5 + i * 16;
        if (line_y + 16 > y + h) break;
        fb_draw_string(x + 5, line_y, data->lines[i], 0x00aaaaaa, 0);
    }
    
    // Draw cursor
    if ((win->widget.focused) && (timer_ticks() % 60 < 30)) {
        uint32_t cx = x + 5 + data->cursor_x * 8;
        uint32_t cy = content_y + 5 + data->cursor_y * 16;
        fb_draw_line(cx, cy, cx, cy + 14, 0x0044ffcc);
    }
}

static void notes_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct notes_data *data = (struct notes_data *)win->widget.data;
    if (!data) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        if (event->key == 27) { // Esc to close
            wm_destroy_window(win);
            notes_window = NULL;
            return;
        }
        
        if (event->key == 8) { // Backspace
            if (data->cursor_x > 0) {
                data->cursor_x--;
                for (int i = data->cursor_x; i < strlen(data->lines[data->cursor_y]); i++) {
                    data->lines[data->cursor_y][i] = data->lines[data->cursor_y][i+1];
                }
            } else if (data->cursor_y > 0) {
                // Merge lines
                uint32_t len_prev = strlen(data->lines[data->cursor_y - 1]);
                uint32_t len_curr = strlen(data->lines[data->cursor_y]);
                if (len_prev + len_curr < NOTES_LINE_LEN - 1) {
                    strcat(data->lines[data->cursor_y - 1], data->lines[data->cursor_y]);
                    for (uint32_t i = data->cursor_y; i < data->line_count - 1; i++) {
                        strcpy(data->lines[i], data->lines[i+1]);
                    }
                    data->line_count--;
                    data->cursor_y--;
                    data->cursor_x = len_prev;
                }
            }
        } else if (event->key == '\n') { // Enter
            if (data->line_count < NOTES_MAX_LINES) {
                for (int i = data->line_count; i > data->cursor_y; i--) {
                    strcpy(data->lines[i], data->lines[i-1]);
                }
                strcpy(data->lines[data->cursor_y + 1], &data->lines[data->cursor_y][data->cursor_x]);
                data->lines[data->cursor_y][data->cursor_x] = '\0';
                data->cursor_y++;
                data->cursor_x = 0;
                data->line_count++;
            }
        } else if (event->key >= 32 && event->key <= 126) {
            char *line = data->lines[data->cursor_y];
            uint32_t len = strlen(line);
            if (len < NOTES_LINE_LEN - 1) {
                for (int i = len; i >= (int)data->cursor_x; i--) {
                    line[i+1] = line[i];
                }
                line[data->cursor_x] = event->key;
                data->cursor_x++;
            }
        } else if (event->key == 17) { // Up
            if (data->cursor_y > 0) data->cursor_y--;
            if (data->cursor_x > strlen(data->lines[data->cursor_y]))
                data->cursor_x = strlen(data->lines[data->cursor_y]);
        } else if (event->key == 18) { // Down
            if (data->cursor_y < data->line_count - 1) data->cursor_y++;
            if (data->cursor_x > strlen(data->lines[data->cursor_y]))
                data->cursor_x = strlen(data->lines[data->cursor_y]);
        } else if (event->key == 19) { // Right
            if (data->cursor_x < strlen(data->lines[data->cursor_y])) data->cursor_x++;
            else if (data->cursor_y < data->line_count - 1) { data->cursor_y++; data->cursor_x = 0; }
        } else if (event->key == 20) { // Left
            if (data->cursor_x > 0) data->cursor_x--;
            else if (data->cursor_y > 0) { data->cursor_y--; data->cursor_x = strlen(data->lines[data->cursor_y]); }
        }
        wm_widget_mark_dirty(widget);
    }
}

void notes_open(void) {
    if (notes_window != NULL) {
        wm_focus_window(notes_window);
        return;
    }
    
    struct notes_data *data = kmalloc(sizeof(struct notes_data));
    memset(data, 0, sizeof(struct notes_data));
    strcpy(data->lines[0], "System breach notes:");
    data->line_count = 1;
    data->cursor_x = strlen(data->lines[0]);
    data->cursor_y = 0;
    
    notes_window = wm_create_window("Notes", 100, 100, 300, 300);
    if (notes_window != NULL) {
        notes_window->widget.data = data;
        notes_window->draw_content = notes_draw_content;
        notes_window->widget.on_event = notes_event;
        notes_window->bg_color = 0x001a1a1a;
        wm_notify("Notes app launched", 0x0044ffcc);
    } else {
        kfree(data);
    }
}
