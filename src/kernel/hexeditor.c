#include "hexeditor.h"
#include "wm.h"
#include "framebuffer.h"
#include "vfs.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

#define HEX_MAX_DATA 4096

struct hex_data {
    uint8_t buffer[HEX_MAX_DATA];
    uint32_t length;
    uint32_t scroll_offset;
    char filename[256];
};

static struct wm_window *hex_window = NULL;

static void hex_draw_content(struct wm_window *win) {
    struct hex_data *data = (struct hex_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar
    fb_fill_rect(x, y, w, 30, 0x00222233);
    fb_draw_rect_outline(x, y, w, 30, 0x00444466);
    fb_draw_string(x + 10, y + 10, "[Open] /boot/kernel.elf", 0x00aaaaff, 0);
    
    // Main area
    uint32_t text_y = y + 40;
    uint32_t content_h = h - 70;
    fb_fill_rect(x, y + 30, w, content_h, 0x000a0a1a);
    
    uint32_t visible_lines = content_h / 16;
    uint32_t start_idx = data->scroll_offset * 16;
    
    for (uint32_t line = 0; line < visible_lines; line++) {
        uint32_t offset = start_idx + (line * 16);
        if (offset >= data->length) break;
        
        // Alternating row highlight
        if (line % 2 == 0) {
            fb_fill_rect(x, text_y - 2, w, 16, 0x00151525);
        }
        
        char buf[128];
        
        // Draw offset
        sprintf(buf, "%08X", offset);
        fb_draw_string(x + 10, text_y, buf, 0x0044ffcc, 0);
        
        // Draw hex
        uint32_t hex_x = x + 100;
        for (uint32_t i = 0; i < 16; i++) {
            if (offset + i < data->length) {
                sprintf(buf, "%02X", data->buffer[offset + i]);
                uint32_t color = (data->buffer[offset + i] == 0) ? 0x00444444 : 0x00ffddaa;
                fb_draw_string(hex_x + i * 24, text_y, buf, color, 0);
            } else {
                fb_draw_string(hex_x + i * 24, text_y, "..", 0x00444444, 0);
            }
        }
        
        // Draw ascii
        uint32_t ascii_x = hex_x + 16 * 24 + 16;
        for (uint32_t i = 0; i < 16; i++) {
            if (offset + i < data->length) {
                char c = data->buffer[offset + i];
                if (c < 32 || c > 126) c = '.';
                fb_draw_char_colored(ascii_x + i * 8, text_y, c, 0x00aaaaaa);
            }
        }
        
        text_y += 16;
    }
    
    // Status
    uint32_t stat_y = y + h - 30;
    fb_fill_rect_gradient_h(x, stat_y, w, 30, 0x001a1a2a, 0x002a2a3a);
    char status[128];
    sprintf(status, "File: %s | Size: %u bytes", data->filename, data->length);
    fb_draw_string(x + 10, stat_y + 8, status, 0x008888aa, 0);
}

static void hex_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct hex_data *data = (struct hex_data *)win->widget.data;
    if (!data) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        if (event->key == 'q' || event->key == 27) {
            wm_destroy_window(win);
            hex_window = NULL;
        } else if (event->key == 17) { // Up
            if (data->scroll_offset > 0) {
                data->scroll_offset--;
                wm_widget_mark_dirty(widget);
            }
        } else if (event->key == 18) { // Down
            uint32_t max_lines = data->length / 16;
            if (data->scroll_offset < max_lines) {
                data->scroll_offset++;
                wm_widget_mark_dirty(widget);
            }
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        if (event->y < 30 && event->x >= 10 && event->x < 200) {
            // Mock open file
            struct vfs_file f;
            if (vfs_open(&f, "/boot/kernel.elf", 0) == 0) {
                size_t read_bytes = 0;
                vfs_read(&f, data->buffer, HEX_MAX_DATA, &read_bytes);
                data->length = read_bytes;
                strcpy(data->filename, "/boot/kernel.elf");
                vfs_close(&f);
                data->scroll_offset = 0;
                wm_widget_mark_dirty(widget);
            }
        }
    }
}

void hexeditor_open(void) {
    if (hex_window != NULL) {
        wm_focus_window(hex_window);
        return;
    }
    
    struct hex_data *data = kmalloc(sizeof(struct hex_data));
    memset(data, 0, sizeof(struct hex_data));
    strcpy(data->filename, "No file loaded");
    
    hex_window = wm_create_window("Hex Editor", 200, 150, 700, 500);
    if (hex_window != NULL) {
        hex_window->widget.data = data;
        hex_window->draw_content = hex_draw_content;
        hex_window->widget.on_event = hex_event;
        hex_window->bg_color = 0x000a0a1a;
        wm_notify("Hex Editor ready", 0x00aaaaff);
    } else {
        kfree(data);
    }
}
