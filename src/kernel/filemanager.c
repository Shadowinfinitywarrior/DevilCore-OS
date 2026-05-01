#include "filemanager.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include "icons.h"
#include <string.h>
#include <stdio.h>

extern struct wm_desktop *desktop;

static struct wm_window *fm_window = NULL;
static char current_path[256] = "/";
static char file_list[10][256];
static int file_count = 0;

static void fm_draw_content(struct wm_window *win) {
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Path bar
    fb_fill_rect(x, y, w, 24, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, 24, 0x00505050);
    char path_label[270];
    sprintf(path_label, "Path: %s", current_path);
    fb_draw_string(x + 6, y + 8, path_label, 0x00cccccc, 0);
    
    // File list area
    y += 24;
    uint32_t list_h = h - 48; // Adjust for status bar
    fb_fill_rect(x, y, w, list_h, 0x001a1a1a);
    fb_draw_rect_outline(x, y, w, list_h, 0x00505050);
    
    // Draw file entries with icons
    for (int i = 0; i < file_count && i < 10; ++i) {
        uint32_t entry_y = y + 4 + (i * 24);
        if (entry_y + 20 > y + list_h) break;
        
        // Select icon based on file type
        const struct icon_data *file_icon = NULL;
        if (file_list[i][0] == '[' && file_list[i][1] == 'D') {
            file_icon = icon_get("folder");
        } else if (file_list[i][0] == '[' && file_list[i][1] == 'F') {
            // Check for .txt extension
            int len = strlen(file_list[i]);
            if (len > 4 && file_list[i][len-4] == '.' && file_list[i][len-3] == 't') {
                file_icon = icon_get("document");
            } else if (len > 5 && file_list[i][6] == 'k') { // "[FILE] kernel..."
                file_icon = icon_get("kernel");
            } else {
                file_icon = icon_get("document");
            }
        } else {
            file_icon = icon_get("document");
        }
        
        // Draw icon
        if (file_icon) {
            draw_icon(x + 8, entry_y + 2, file_icon);
        }
        
        fb_draw_string(x + 44, entry_y + 6, file_list[i], 0x00ffffff, 0);
    }
    
    // Status bar
    y += list_h;
    fb_fill_rect_gradient_h(x, y, w, 24, 0x002a2a2a, 0x00303030);
    char status[64];
    sprintf(status, "%d items", file_count);
    fb_draw_string(x + 6, y + 8, status, 0x00aaaaaa, 0);
}

static void fm_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL) return;
    
    // Let the default window handler check for title bar buttons first
    if (wm_window_on_event(widget, event)) {
        return; // Event was handled by title bar button
    }
    
    if (event->type == WM_EVENT_KEY) {
        // Handle keyboard input
        uint8_t key = event->key;
        if (key == 'q' || key == 27) { // Q or ESC
            wm_destroy_window(win);
            fm_window = NULL;
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            // Check if click is in file list area (after path bar)
            if (event->y >= 24) {
                int idx = (event->y - 24) / 24;
                if (idx >= 0 && idx < file_count) {
                    // Just a visual feedback for now: update path if it's a directory
                    if (strncmp(file_list[idx], "[DIR]", 5) == 0) {
                        const char *name = file_list[idx] + 6; // Skip "[DIR] "
                        if (strcmp(name, "/home") == 0) strcpy(current_path, "/home");
                        else if (strcmp(name, "/etc") == 0) strcpy(current_path, "/etc");
                        else if (strcmp(name, "/dev") == 0) strcpy(current_path, "/dev");
                    }
                }
            }
        }
    }
}

void fm_open(void) {
    if (fm_window != NULL) {
        wm_focus_window(fm_window);
        return;
    }
    
    // Initialize with some example files
    strcpy(file_list[0], "[DIR] /home");
    strcpy(file_list[1], "[DIR] /etc");
    strcpy(file_list[2], "[DIR] /dev");
    strcpy(file_list[3], "[FILE] README.txt");
    strcpy(file_list[4], "[FILE] kernel.elf");
    file_count = 5;
    
    fm_window = wm_create_window("File Manager", 100, 80, 600, 400);
    if (fm_window != NULL) {
        fm_window->draw_content = fm_draw_content;
        fm_window->widget.on_event = fm_event;
        fm_window->bg_color = 0x001a1a1a;
    }
}
