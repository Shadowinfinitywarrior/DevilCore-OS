#include "sysmonitor.h"
#include "wm.h"
#include "framebuffer.h"
#include "scheduler.h"
#include "memory.h"
#include "timer.h"
#include "icons.h"
#include <string.h>
#include <stdio.h>

extern struct wm_desktop *desktop;

#define CPU_HISTORY_LEN 60

struct sysmon_data {
    uint8_t cpu_history[CPU_HISTORY_LEN];
    uint32_t cpu_idx;
    uint64_t last_update;
    uint64_t last_idle_time;
    uint64_t last_total_time;
    int32_t selected_pid;
    uint32_t scroll_offset;
};

static struct wm_window *sysmon_window = NULL;

static void sysmon_draw_content(struct wm_window *win) {
    struct sysmon_data *data = (struct sysmon_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Top Panel: CPU Graph and Memory Bar
    uint32_t top_h = 140;
    fb_fill_rect(x, y, w, top_h, 0x00151515);
    fb_draw_rect_outline(x, y, w, top_h, 0x00404040);
    
    // Memory Stacked Bar
    uint32_t total_pages = pmm_total_pages();
    uint32_t free_pages = pmm_free_pages();
    uint32_t used_pages = total_pages - free_pages;
    
    uint32_t mem_bar_x = x + 10;
    uint32_t mem_bar_y = y + 20;
    uint32_t mem_bar_w = w - 20;
    uint32_t mem_bar_h = 20;
    
    fb_fill_rect(mem_bar_x, mem_bar_y, mem_bar_w, mem_bar_h, 0x00222222);
    uint32_t used_w = total_pages > 0 ? (used_pages * mem_bar_w) / total_pages : 0;
    
    // Dynamic color based on usage
    uint32_t mem_color = 0x0000ff00; // Green
    if (used_pages * 10 > total_pages * 8) mem_color = 0x00ff0000; // Red > 80%
    else if (used_pages * 10 > total_pages * 5) mem_color = 0x00ffff00; // Yellow > 50%

    fb_fill_rect_gradient_v(mem_bar_x, mem_bar_y, used_w, mem_bar_h, mem_color, 0x00000000);
    fb_draw_rect_outline(mem_bar_x, mem_bar_y, mem_bar_w, mem_bar_h, 0x00444444);
    
    char buf[64];
    uint32_t used_mb = (used_pages * 4) / 1024;
    uint32_t total_mb = (total_pages * 4) / 1024;
    sprintf(buf, "Memory: %u MB / %u MB", used_mb, total_mb);
    fb_draw_string(mem_bar_x, mem_bar_y - 15, buf, 0x00dddddd, 0);
    
    // CPU Graph
    uint32_t graph_x = x + 10;
    uint32_t graph_y = y + 60;
    uint32_t graph_w = w - 20;
    uint32_t graph_h = 60;
    
    fb_fill_rect(graph_x, graph_y, graph_w, graph_h, 0x000a1a1a);
    fb_draw_rect_outline(graph_x, graph_y, graph_w, graph_h, 0x00004400);
    
    // Draw grid
    for (uint32_t i = 1; i < 4; i++) {
        fb_draw_line(graph_x, graph_y + i * 15, graph_x + graph_w, graph_y + i * 15, 0x00003300);
    }
    for (uint32_t i = 1; i < 6; i++) {
        fb_draw_line(graph_x + i * (graph_w / 6), graph_y, graph_x + i * (graph_w / 6), graph_y + graph_h, 0x00003300);
    }
    
    // Draw line
    uint32_t step = graph_w / (CPU_HISTORY_LEN - 1);
    for (uint32_t i = 0; i < CPU_HISTORY_LEN - 1; i++) {
        uint32_t idx1 = (data->cpu_idx + i) % CPU_HISTORY_LEN;
        uint32_t idx2 = (data->cpu_idx + i + 1) % CPU_HISTORY_LEN;
        
        uint32_t val1 = data->cpu_history[idx1];
        uint32_t val2 = data->cpu_history[idx2];
        
        uint32_t py1 = graph_y + graph_h - (val1 * graph_h) / 100;
        uint32_t py2 = graph_y + graph_h - (val2 * graph_h) / 100;
        
        fb_draw_line(graph_x + i * step, py1, graph_x + (i + 1) * step, py2, 0x0000ff00);
    }
    
    uint8_t curr_cpu = data->cpu_history[(data->cpu_idx + CPU_HISTORY_LEN - 1) % CPU_HISTORY_LEN];
    sprintf(buf, "CPU Usage: %u%%", curr_cpu);
    fb_draw_string(graph_x, graph_y - 15, buf, 0x0000ff00, 0);
    
    // Bottom Panel: Process List
    uint32_t bot_y = y + top_h;
    uint32_t bot_h = h - top_h;
    fb_fill_rect(x, bot_y, w, bot_h, 0x001a1a1a);
    
    // Headers
    fb_fill_rect(x, bot_y, w, 20, 0x002a2a2a);
    fb_draw_string(x + 10, bot_y + 4, "PID", 0x00aaaaaa, 0);
    fb_draw_string(x + 60, bot_y + 4, "NAME", 0x00aaaaaa, 0);
    fb_draw_string(x + 160, bot_y + 4, "STATE", 0x00aaaaaa, 0);
    fb_draw_string(x + 240, bot_y + 4, "CPU(ms)", 0x00aaaaaa, 0);
    
    // List
    uint32_t list_y = bot_y + 20;
    struct task *t = run_queue.head;
    
    // Skip to scroll offset
    for (uint32_t i = 0; i < data->scroll_offset && t; i++) {
        t = t->next;
    }
    
    uint32_t row_i = 0;
    while (t && list_y + 20 <= y + h - 30) { // Keep room for status bar/Kill button
        if (data->selected_pid == t->pid) {
            fb_fill_rect(x, list_y, w, 20, 0x003333aa);
        }
        
        sprintf(buf, "%d", t->pid);
        fb_draw_string(x + 10, list_y + 4, buf, 0x00ffffff, 0);
        fb_draw_string(x + 60, list_y + 4, t->name, 0x00ffffff, 0);
        
        const char *state_str = "???";
        uint32_t state_color = 0;
        switch (t->state) {
            case TASK_RUNNING: state_str = "RUN"; state_color = 0x0000ff00; break;
            case TASK_READY: state_str = "READY"; state_color = 0x0000aaaa; break;
            case TASK_BLOCKED: state_str = "BLOCK"; state_color = 0x00aa5500; break;
            case TASK_SLEEPING: state_str = "SLEEP"; state_color = 0x00aaaaaa; break;
            case TASK_STOPPED: state_str = "STOP"; state_color = 0x00ff0000; break;
        }
        fb_draw_string(x + 160, list_y + 4, state_str, state_color, 0);
        
        sprintf(buf, "%u", (uint32_t)t->cpu_time);
        fb_draw_string(x + 240, list_y + 4, buf, 0x00ffffff, 0);
        
        list_y += 20;
        row_i++;
        t = t->next;
    }
    
    // Footer / Kill Button
    uint32_t foot_y = win->widget.y + h - 30;
    fb_fill_rect_gradient_h(x, foot_y, w, 30, 0x002a2a2a, 0x00303030);
    
    if (data->selected_pid > 0) {
        // Draw Kill Button
        fb_fill_rect_rounded(x + w - 100, foot_y + 4, 90, 22, 4, 0x00cc2222);
        fb_draw_string(x + w - 75, foot_y + 8, "Kill Task", 0x00ffffff, 0);
    }
    
    // Update data loop (simulated timer)
    uint64_t ticks = timer_ticks();
    if (ticks - data->last_update > 100) { // ~1 second
        data->last_update = ticks;
        // Simulate CPU usage based on idle task or randomness for demo if idle task isn't tracked properly
        uint8_t simulated_cpu = 5 + (ticks % 20); 
        data->cpu_history[data->cpu_idx] = simulated_cpu;
        data->cpu_idx = (data->cpu_idx + 1) % CPU_HISTORY_LEN;
        wm_widget_mark_dirty(&win->widget);
    }
}

static void sysmon_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct sysmon_data *data = (struct sysmon_data *)win->widget.data;
    if (!data) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        uint32_t top_h = 140;
        uint32_t foot_y = win->widget.height - 30;
        
        if (event->y > top_h + 20 && event->y < foot_y) {
            // Click in process list
            int row = (event->y - top_h - 20) / 20;
            struct task *t = run_queue.head;
            for (uint32_t i = 0; i < data->scroll_offset + row && t; i++) {
                t = t->next;
            }
            if (t) {
                data->selected_pid = t->pid;
                wm_widget_mark_dirty(widget);
            }
        } else if (event->y >= foot_y && event->x >= win->widget.width - 100) {
            // Click Kill Button
            if (data->selected_pid > 0) {
                task_kill(data->selected_pid, 9);
                data->selected_pid = -1;
                wm_widget_mark_dirty(widget);
            }
        }
    }
}

void sysmonitor_open(void) {
    if (sysmon_window != NULL) {
        wm_focus_window(sysmon_window);
        return;
    }
    
    struct sysmon_data *data = kmalloc(sizeof(struct sysmon_data));
    memset(data, 0, sizeof(struct sysmon_data));
    data->selected_pid = -1;
    data->last_update = timer_ticks();
    
    sysmon_window = wm_create_window("System Monitor", 50, 50, 400, 500);
    if (sysmon_window != NULL) {
        sysmon_window->widget.data = data;
        sysmon_window->draw_content = sysmon_draw_content;
        sysmon_window->widget.on_event = sysmon_event;
        sysmon_window->bg_color = 0x00151515;
        wm_notify("System Monitor active", 0x0044ffcc);
    } else {
        kfree(data);
    }
}
