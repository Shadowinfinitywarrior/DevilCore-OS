#include "filemanager.h"
#include "wm.h"
#include "framebuffer.h"
#include <string.h>
#include <stdio.h>
#include "vfs.h"
#include "memory.h"
#include "icons.h"

extern struct wm_desktop *desktop;

#define FM_MAX_FILES 64

struct fm_data {
    char current_path[256];
    struct vfs_node *entries[FM_MAX_FILES];
    int file_count;
    uint32_t scroll_offset;
};

static struct wm_window *fm_window = NULL;

static void fm_load_directory(struct fm_data *data, const char *path) {
    if (!data || !path) return;
    
    // Clear old entries
    for (int i = 0; i < data->file_count; i++) {
        if (data->entries[i]) {
            kfree(data->entries[i]);
            data->entries[i] = NULL;
        }
    }
    data->file_count = 0;
    
    struct vfs_node *dir_node = vfs_lookup(path);
    if (dir_node && (dir_node->type & VFS_TYPE_DIR)) {
        struct vfs_node *entries[FM_MAX_FILES];
        int count = vfs_read_directory(dir_node, entries, FM_MAX_FILES);
        for (int i = 0; i < count; i++) {
            struct vfs_node *node = kmalloc(sizeof(struct vfs_node));
            memcpy(node, entries[i], sizeof(struct vfs_node));
            data->entries[data->file_count++] = node;
        }
        strcpy(data->current_path, path);
    }
}

static void fm_draw_content(struct wm_window *win) {
    struct fm_data *data = (struct fm_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Path bar
    fb_fill_rect(x, y, w, 32, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, 32, 0x00505050);
    char path_label[270];
    sprintf(path_label, "Path: %s", data->current_path);
    fb_draw_string(x + 10, y + 12, path_label, 0x00cccccc, 0);
    
    // Toolbar / Breadcrumb area (Placeholder)
    fb_fill_rect(x, y + 32, w, 24, 0x001f1f1f);
    fb_draw_string(x + 10, y + 36, "[UP] [NEW FOLDER] [DELETE] [PROPERTIES]", 0x008888aa, 0);
    
    // File list area
    y += 56;
    uint32_t list_h = h - 56 - 24; // Adjust for path bar, toolbar and status bar
    fb_fill_rect(x, y, w, list_h, 0x00151515);
    fb_draw_rect_outline(x, y, w, list_h, 0x00505050);
    
    // Draw file entries with icons
    for (int i = 0; i < data->file_count; ++i) {
        uint32_t entry_y = y + 4 + ((i - data->scroll_offset) * 28);
        if (entry_y < y) continue; // Above view
        if (entry_y + 24 > y + list_h) break; // Below view
        
        struct vfs_node *node = data->entries[i];
        
        // Select icon based on file type
        const struct icon_data *file_icon = NULL;
        if (node->type & VFS_TYPE_DIR) {
            file_icon = icon_get("folder");
        } else {
            // Check for extension
            int len = strlen(node->name);
            if (len > 4 && node->name[len-4] == '.' && node->name[len-3] == 't') {
                file_icon = icon_get("document");
            } else if (len > 5 && strncmp(node->name, "kernel", 6) == 0) {
                file_icon = icon_get("kernel");
            } else {
                file_icon = icon_get("document");
            }
        }
        
        // Draw icon
        if (file_icon) {
            draw_icon(x + 10, entry_y + 2, file_icon);
        }
        
        // Draw filename
        fb_draw_string(x + 50, entry_y + 8, node->name, 0x00ffffff, 0);
        
        // Draw file info right-aligned
        char info[64];
        if (node->type & VFS_TYPE_DIR) {
            sprintf(info, "DIR");
        } else {
            sprintf(info, "%u bytes", (uint32_t)node->size);
        }
        fb_draw_string(x + w - 100, entry_y + 8, info, 0x00aaaaaa, 0);
    }
    
    // Status bar
    y += list_h;
    fb_fill_rect_gradient_h(x, y, w, 24, 0x002a2a2a, 0x00303030);
    char status[64];
    sprintf(status, "%d items", data->file_count);
    fb_draw_string(x + 10, y + 8, status, 0x00aaaaaa, 0);
}

static void fm_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct fm_data *data = (struct fm_data *)win->widget.data;
    if (win == NULL || data == NULL) return;
    
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
            // Check if click is in UP button
            if (event->y >= 32 && event->y < 56 && event->x >= 10 && event->x < 42) {
                // Go UP directory
                int len = strlen(data->current_path);
                if (len > 1) {
                    while (len > 1 && data->current_path[len-1] == '/') len--;
                    while (len > 0 && data->current_path[len-1] != '/') len--;
                    data->current_path[len] = '\0';
                    if (len == 0) strcpy(data->current_path, "/");
                    fm_load_directory(data, data->current_path);
                }
                return;
            }
            
            // Check if click is in file list area
            if (event->y >= 56) {
                int idx = (event->y - 56 - 4) / 28 + data->scroll_offset;
                if (idx >= 0 && idx < data->file_count) {
                    struct vfs_node *node = data->entries[idx];
                    
                    if (node->type & VFS_TYPE_DIR) {
                        // Navigate into directory
                        char new_path[256];
                        if (strcmp(data->current_path, "/") == 0) {
                            sprintf(new_path, "/%s", node->name);
                        } else {
                            sprintf(new_path, "%s/%s", data->current_path, node->name);
                        }
                        fm_load_directory(data, new_path);
                    } else {
                        // It's a file, we could open text editor here
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
    
    struct fm_data *data = kmalloc(sizeof(struct fm_data));
    memset(data, 0, sizeof(struct fm_data));
    strcpy(data->current_path, "/");
    
    fm_window = wm_create_window("File Manager", 100, 80, 600, 450);
    if (fm_window != NULL) {
        fm_window->widget.data = data;
        fm_window->draw_content = fm_draw_content;
        fm_window->widget.on_event = fm_event;
        fm_window->bg_color = 0x00151515;
        
        fm_load_directory(data, "/");
    } else {
        kfree(data);
    }
}
