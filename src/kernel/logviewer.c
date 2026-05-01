#include "logviewer.h"
#include "wm.h"
#include "framebuffer.h"
#include "timer.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

#define MAX_LOG_LINES 100

struct log_entry {
    char message[128];
    uint8_t severity; // 0=info, 1=warn, 2=error, 3=debug
};

struct logviewer_data {
    struct log_entry logs[MAX_LOG_LINES];
    uint32_t log_count;
    uint32_t log_idx;
    uint32_t scroll_offset;
    uint64_t last_log_time;
};

static struct wm_window *lv_window = NULL;

static void lv_draw_content(struct wm_window *win) {
    struct logviewer_data *data = (struct logviewer_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar
    fb_fill_rect(x, y, w, 30, 0x00222222);
    fb_draw_rect_outline(x, y, w, 30, 0x00444444);
    fb_draw_string(x + 10, y + 10, "[Clear] [Export] Source: /var/log/syslog", 0x00aaaaaa, 0);
    
    // List background
    uint32_t content_y = y + 30;
    uint32_t content_h = h - 30;
    fb_fill_rect(x, content_y, w, content_h, 0x00050505);
    
    uint32_t visible_lines = content_h / 16;
    uint32_t text_y = content_y + 4;
    
    uint32_t draw_count = data->log_count > visible_lines ? visible_lines : data->log_count;
    
    for (uint32_t i = 0; i < draw_count; i++) {
        uint32_t idx = (data->log_idx + MAX_LOG_LINES - 1 - i - data->scroll_offset) % MAX_LOG_LINES;
        if (i + data->scroll_offset >= data->log_count) break;
        
        struct log_entry *entry = &data->logs[idx];
        
        uint32_t badge_color = 0x00444444;
        const char *tag = "INFO";
        if (entry->severity == 1) { badge_color = 0x00aa6600; tag = "WARN"; }
        else if (entry->severity == 2) { badge_color = 0x00aa0000; tag = "ERR "; }
        else if (entry->severity == 3) { badge_color = 0x000066aa; tag = "DBUG"; }
        
        // Draw badge
        fb_fill_rect_rounded(x + 5, text_y, 40, 14, 3, badge_color);
        fb_draw_string(x + 8, text_y + 3, tag, 0x00ffffff, 0);
        
        fb_draw_string(x + 55, text_y + 3, entry->message, 0x00aaaaaa, 0);
        
        text_y += 16;
    }
    
    // Simulate incoming logs
    uint64_t ticks = timer_ticks();
    if (ticks - data->last_log_time > 150) {
        data->last_log_time = ticks;
        
        struct log_entry *entry = &data->logs[data->log_idx];
        entry->severity = (ticks % 10 == 0) ? 2 : ((ticks % 4 == 0) ? 1 : 0);
        sprintf(entry->message, "System event %llu triggered. CPU scheduling active.", ticks);
        
        data->log_idx = (data->log_idx + 1) % MAX_LOG_LINES;
        if (data->log_count < MAX_LOG_LINES) data->log_count++;
        
        wm_widget_mark_dirty(&win->widget);
    }
}

static void lv_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct logviewer_data *data = (struct logviewer_data *)win->widget.data;
    if (!data) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        if (event->key == 'q' || event->key == 27) {
            wm_destroy_window(win);
            lv_window = NULL;
        } else if (event->key == 17) { // Up
            uint32_t visible_lines = (win->widget.height - 30) / 16;
            if (data->scroll_offset < data->log_count - visible_lines) {
                data->scroll_offset++;
                wm_widget_mark_dirty(widget);
            }
        } else if (event->key == 18) { // Down
            if (data->scroll_offset > 0) {
                data->scroll_offset--;
                wm_widget_mark_dirty(widget);
            }
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        if (event->y < 30 && event->x >= 10 && event->x <= 60) {
            data->log_count = 0;
            data->log_idx = 0;
            data->scroll_offset = 0;
            wm_widget_mark_dirty(widget);
        }
    }
}

void logviewer_open(void) {
    if (lv_window != NULL) {
        wm_focus_window(lv_window);
        return;
    }
    
    struct logviewer_data *data = kmalloc(sizeof(struct logviewer_data));
    memset(data, 0, sizeof(struct logviewer_data));
    data->last_log_time = timer_ticks();
    
    // Initial logs
    strcpy(data->logs[0].message, "DevilCore Kernel started.");
    data->logs[0].severity = 0;
    strcpy(data->logs[1].message, "Framebuffer initialized.");
    data->logs[1].severity = 0;
    strcpy(data->logs[2].message, "VFS mounted rootfs.");
    data->logs[2].severity = 0;
    strcpy(data->logs[3].message, "Networking device eth0 missing.");
    data->logs[3].severity = 1;
    data->log_count = 4;
    data->log_idx = 4;
    
    lv_window = wm_create_window("System Log Viewer", 300, 200, 600, 400);
    if (lv_window != NULL) {
        lv_window->widget.data = data;
        lv_window->draw_content = lv_draw_content;
        lv_window->widget.on_event = lv_event;
        lv_window->bg_color = 0x00050505;
        wm_notify("System logs attached", 0x00aaaaaa);
    } else {
        kfree(data);
    }
}
