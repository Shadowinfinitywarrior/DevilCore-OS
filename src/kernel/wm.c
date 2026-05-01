#include "wm.h"

#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "vfs.h"
#include "shell.h"
#include "timer.h"
#include "kprintf.h"
#include "filemanager.h"
#include "texteditor.h"
#include "calculator.h"
#include "settings.h"
#include "logo_raw.h"
#include "logo.h"
#include "framebuffer.h"
#include "icons.h"
#include <stdio.h>

struct wm_theme current_theme = {
    .bg_color = 0xFF2D2D2D,
    .fg_color = 0xFFFFFFFF,
    .accent_color = 0xFF007ACC,
    .border_color = 0xFF444444,
    .title_bar_color = 0xFF3D3D3D,
    .selection_color = 0xFF555555
};

struct wm_desktop *desktop = NULL;
static uint32_t next_window_id = 1;
static struct wm_window *dragged_window = NULL;

void wm_set_theme(struct wm_theme new_theme) {
    current_theme = new_theme;
    
    // Mark all windows as dirty to trigger a global redraw
    if (desktop != NULL) {
        for (uint32_t i = 0; i < desktop->window_count; ++i) {
            wm_widget_mark_dirty(&desktop->windows[i]->widget);
        }
    }
}

void wm_show_context_menu(int32_t x, int32_t y);

void wm_widget_init(struct wm_widget *widget, enum wm_widget_type type, const char *name, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    if (widget == NULL) return;
    
    static uint32_t next_widget_id = 1;
    widget->id = next_widget_id++;
    widget->type = type;
    if (name) strncpy(widget->name, name, 63);
    else widget->name[0] = '\0';
    
    widget->x = x;
    widget->y = y;
    widget->width = w;
    widget->height = h;
    
    widget->visible = 1;
    widget->focused = 0;
    widget->enabled = 1;
    widget->dirty = 1;
    widget->last_x = x;
    widget->last_y = y;
    widget->last_width = w;
    widget->last_height = h;
    
    widget->parent = NULL;
    widget->child_count = 0;
    for (uint32_t i = 0; i < WM_MAX_CHILDREN; ++i) {
        widget->children[i] = NULL;
    }
    
    widget->draw = NULL;
    widget->on_event = NULL;
    widget->data = NULL;
}

void wm_widget_add_child(struct wm_widget *parent, struct wm_widget *child) {
    if (parent == NULL || child == NULL || parent->child_count >= WM_MAX_CHILDREN) return;
    
    // Remove from old parent if exists
    if (child->parent) {
        wm_widget_remove_child(child->parent, child);
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void wm_widget_remove_child(struct wm_widget *parent, struct wm_widget *child) {
    if (parent == NULL || child == NULL) return;
    
    for (uint32_t i = 0; i < parent->child_count; ++i) {
        if (parent->children[i] == child) {
            // Shift remaining children
            for (uint32_t j = i; j < parent->child_count - 1; ++j) {
                parent->children[j] = parent->children[j+1];
            }
            parent->children[--parent->child_count] = NULL;
            child->parent = NULL;
            return;
        }
    }
}

void wm_widget_mark_dirty(struct wm_widget *widget) {
    if (widget == NULL) return;
    widget->dirty = 1;
    if (widget->parent) {
        wm_widget_mark_dirty(widget->parent);
    }
}

// Mark widget and all its children as dirty (used for full redraw)
static void wm_widget_mark_dirty_recursive(struct wm_widget *widget) {
    if (widget == NULL) return;
    widget->dirty = 1;
    for (uint32_t i = 0; i < widget->child_count; ++i) {
        wm_widget_mark_dirty_recursive(widget->children[i]);
    }
}

void wm_widget_draw(struct wm_widget *widget) {
    if (widget == NULL || !widget->visible) return;
    
    // Draw self if dirty
    if (widget->dirty && widget->draw) {
        widget->draw(widget);
        widget->dirty = 0; // Reset dirty flag
    }
    
    // Draw children
    for (uint32_t i = 0; i < widget->child_count; ++i) {
        wm_widget_draw(widget->children[i]);
    }
}


static uint8_t is_point_in_widget(struct wm_widget *widget, int32_t x, int32_t y) {
    // Relative coordinates
    int32_t top_offset = 0;
    if (widget->type == WM_WIDGET_WINDOW) {
        struct wm_window *win = (struct wm_window *)widget;
        if (win->decorated) top_offset = WM_TITLE_HEIGHT;
    }
    return (x >= 0 && x < (int32_t)widget->width && y >= -top_offset && y < (int32_t)widget->height);
}

void wm_widget_dispatch_event(struct wm_widget *widget, struct wm_event *event) {
    if (widget == NULL || !widget->visible || !widget->enabled) return;
    
    // Mouse event logic: check children first (top-down in terms of visual stack)
    if (event->type == WM_EVENT_MOUSE || event->type == WM_EVENT_MOUSE_PRESS || event->type == WM_EVENT_MOUSE_RELEASE) {
        for (int32_t i = (int32_t)widget->child_count - 1; i >= 0; --i) {
            struct wm_widget *child = widget->children[i];
            if (child == NULL || !child->visible) continue;
            
            // Adjust event to child local coordinates
            struct wm_event local_event = *event;
            local_event.x -= child->x;
            local_event.y -= child->y;
            
            if (is_point_in_widget(child, local_event.x, local_event.y)) {
                wm_widget_dispatch_event(child, &local_event);
                return; // Event consumed
            }
        }
    }
    
    // Key events: dispatch to focused widget
    if (event->type == WM_EVENT_KEY) {
        if (widget->focused) {
            if (widget->on_event) widget->on_event(widget, event);
            return;
        }
        for (uint32_t i = 0; i < widget->child_count; ++i) {
            wm_widget_dispatch_event(widget->children[i], event);
        }
        return;
    }

    // Handle event by self if not consumed by children
    if (widget->on_event) {
        widget->on_event(widget, event);
    }
}
static uint64_t last_tick = 0;
static uint8_t cursor_visible = 1;
static uint8_t start_menu_visible = 0;

#define WM_EVENT_QUEUE_SIZE 256
static struct wm_event event_queue[WM_EVENT_QUEUE_SIZE];
static uint32_t event_queue_head = 0;
static uint32_t event_queue_tail = 0;

static void wm_push_event(struct wm_event *event) {
    uint32_t next = (event_queue_head + 1) % WM_EVENT_QUEUE_SIZE;
    if (next != event_queue_tail) {
        event_queue[event_queue_head] = *event;
        event_queue_head = next;
    }
}

static int wm_pop_event(struct wm_event *event) {
    if (event_queue_head == event_queue_tail) return 0;
    *event = event_queue[event_queue_tail];
    event_queue_tail = (event_queue_tail + 1) % WM_EVENT_QUEUE_SIZE;
    return 1;
}

void wm_process_events(void) {
    struct wm_event event;
    while (wm_pop_event(&event)) {
        if (event.type == WM_EVENT_KEY) {
            if (desktop->screen == WM_SCREEN_LOGIN) {
                if (event.key == '\t') desktop->login_focus = !desktop->login_focus;
                else if (event.key == '\n') {
                    if (wm_verify_login(desktop->login_user, desktop->login_pass)) {
                        desktop->screen = WM_SCREEN_DESKTOP;
                    } else {
                        memset(desktop->login_pass, 0, sizeof(desktop->login_pass));
                    }
                } else if (event.key == '\b') {
                    char *buf = desktop->login_focus ? desktop->login_pass : desktop->login_user;
                    size_t len = strlen(buf);
                    if (len > 0) buf[len - 1] = '\0';
                } else {
                    char *buf = desktop->login_focus ? desktop->login_pass : desktop->login_user;
                    size_t len = strlen(buf);
                    if (len < 31) {
                        buf[len] = (char)event.key;
                        buf[len + 1] = '\0';
                    }
                }
            } else if (desktop->focused_window) {
                wm_widget_dispatch_event(&desktop->focused_window->widget, &event);
            }
        } else {
            // Mouse events
            if (event.type == WM_EVENT_MOUSE) {
                if (dragged_window != NULL) {
                    dragged_window->widget.x += event.dx;
                    dragged_window->widget.y += event.dy;
                    desktop->needs_redraw = 1;
                } else {
                    wm_widget_dispatch_event(&desktop->widget, &event);
                }
            } else if (event.type == WM_EVENT_MOUSE_PRESS) {
                wm_widget_dispatch_event(&desktop->widget, &event);
                if (event.buttons & 2) {
                    wm_show_context_menu(event.x, event.y);
                }
            } else if (event.type == WM_EVENT_MOUSE_RELEASE) {
                if (dragged_window != NULL) {
                    dragged_window = NULL;
                }
                wm_widget_dispatch_event(&desktop->widget, &event);
            }
        }
        desktop->needs_redraw = 1;
    }
}

// Theme colors
// Forward declarations for callbacks
static void wm_open_terminal(void);
static void wm_open_settings(void);
static void wm_show_about(void);
// Track hovered icon for visual feedback
static struct wm_widget *hovered_icon = NULL;

void wm_icon_draw(struct wm_widget *widget) {
    struct wm_icon *icon = (struct wm_icon *)widget;
    if (icon == NULL || !icon->widget.visible) return;

    int32_t x = icon->widget.x;
    int32_t y = icon->widget.y;
    uint8_t is_hovered = (hovered_icon == widget);

    // Modern icon background with rounded corners and shadow
    if (is_hovered) {
        // Hover state: semi-transparent white highlight with shadow
        fb_fill_rect_alpha(x + 2, y + 2, 48, 64, 0x00000000, 40);  // Shadow
        fb_fill_rect_rounded(x, y, 48, 64, 8, 0x00ffffff);         // White highlight
        fb_fill_rect_alpha(x, y, 48, 64, current_theme.accent_color, 30);  // Accent tint
    } else {
        // Normal state: subtle dark background on hover area only
        fb_fill_rect_rounded(x + 4, y + 4, 40, 56, 6, 0x00000000);
    }

    // Icon background plate (subtle gradient effect)
    uint32_t plate_color = is_hovered ? 0x00ffffff : 0x00f0f0f0;
    uint8_t plate_alpha = is_hovered ? 60 : 20;
    fb_fill_rect_rounded(x + 8, y + 8, 32, 32, 6, plate_color);

    // Get the proper icon data using icon_name (not label)
    const struct icon_data *icon_data = icon_get(icon->icon_name);

    if (icon_data != NULL) {
        // Draw the proper icon (32x32 pixels) centered
        draw_icon(x + 12, y + 12, icon_data);
    } else {
        // Fallback: draw a modern placeholder with gradient
        fb_fill_rect_rounded(x + 12, y + 12, 24, 24, 4, 0x00555555);
        fb_draw_rect_rounded(x + 12, y + 12, 24, 24, 4, 0x00777777);
    }

    // Draw icon label with shadow for better readability
    size_t len = strlen(icon->label);
    int32_t text_x = x + 24 - (len * 4);  // Center text (8px font / 2)
    if (text_x < x) text_x = x;

    // Text shadow
    fb_draw_string(text_x + 1, icon->widget.y + 49, icon->label, 0x00000000, 0);
    // Main text - white with subtle glow when hovered
    uint32_t text_color = is_hovered ? 0x00ffffff : 0x00dddddd;
    fb_draw_string(text_x, icon->widget.y + 48, icon->label, text_color, 0);
}

static void wm_icon_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_icon *icon = (struct wm_icon *)widget;
    if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            if (icon->on_click) {
                icon->on_click();
            }
        }
    } else if (event->type == WM_EVENT_MOUSE) {
        // Track hover state for visual feedback
        struct wm_widget *old_hover = hovered_icon;
        hovered_icon = widget;
        if (old_hover != hovered_icon) {
            // Mark both old and new as dirty to trigger redraw
            if (old_hover) wm_widget_mark_dirty(old_hover);
            wm_widget_mark_dirty(widget);
            desktop->needs_redraw = 1;  // Trigger actual redraw for hover effect
        }
    }
}


void wm_taskbar_draw(struct wm_widget *widget) {
    struct wm_taskbar *tb = (struct wm_taskbar *)widget;
    
    // Draw taskbar with semi-transparent gradient
    fb_fill_rect_alpha(tb->widget.x, tb->widget.y, tb->widget.width, tb->widget.height, current_theme.title_bar_color, 240);
    fb_draw_line(tb->widget.x, tb->widget.y, tb->widget.x + tb->widget.width, tb->widget.y, 0x00505050);
    
    // Draw Start Button with icon
    uint32_t start_color = start_menu_visible ? current_theme.accent_color : 0x003a3a3a;
    fb_fill_rect_rounded(tb->widget.x + 4, tb->widget.y + 4, 32, 32, 4, start_color);
    // Draw small DevilCore logo icon in start button
    const struct icon_data *logo_icon = icon_get("system");
    if (logo_icon) {
        draw_icon(tb->widget.x + 8, tb->widget.y + 8, logo_icon);
    } else {
        fb_draw_string(tb->widget.x + 12, tb->widget.y + 12, "D", 0x00ffffff, 0);
    }
    
    // Draw clock (system tray)
    uint64_t uptime_sec = timer_ticks() / 100;
    uint32_t hour = (uint32_t)(uptime_sec / 3600) % 24;
    uint32_t min = (uint32_t)(uptime_sec / 60) % 60;
    char time_str[16];
    sprintf(time_str, "%02u:%02u", hour, min);
    fb_draw_string(tb->widget.x + tb->widget.width - 60, tb->widget.y + 12, time_str, 0x00ffffff, 0);
    
    // Draw open window indicators with icons
    int32_t tab_x = 44;
    for (uint32_t i = 0; i < desktop->window_count; ++i) {
        struct wm_window *win = desktop->windows[i];
        uint32_t tab_color = win->widget.focused ? current_theme.accent_color : 0x00303030;
        fb_fill_rect_rounded(tab_x, tb->widget.y + 4, 120, 32, 4, tab_color);
        
        // Draw window icon based on title (check first few chars)
        const struct icon_data *win_icon = NULL;
        if (win->title[0] == 'T' && win->title[1] == 'e' && win->title[2] == 'r') win_icon = icon_get("terminal");
        else if (win->title[0] == 'F' && win->title[1] == 'i' && win->title[2] == 'l') win_icon = icon_get("filemanager");
        else if (win->title[0] == 'T' && win->title[1] == 'e' && win->title[2] == 'x') win_icon = icon_get("texteditor");
        else if (win->title[0] == 'C' && win->title[1] == 'a' && win->title[2] == 'l') win_icon = icon_get("calculator");
        else if (win->title[0] == 'S' && win->title[1] == 'e' && win->title[2] == 't') win_icon = icon_get("settings");
        else if (win->title[0] == 'S' && win->title[1] == 'y' && win->title[2] == 's') win_icon = icon_get("system_monitor");
        
        if (win_icon) {
            draw_icon(tab_x + 4, tb->widget.y + 6, win_icon);
            fb_draw_string(tab_x + 28, tb->widget.y + 12, win->title, 0x00ffffff, 0);
        } else {
            fb_draw_string(tab_x + 8, tb->widget.y + 12, win->title, 0x00ffffff, 0);
        }
        tab_x += 124;
    }
}

static void wm_taskbar_on_event(struct wm_widget *widget, struct wm_event *event) {
    (void)widget;
    if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        if (event->x >= 4 && event->x <= 36) {
            wm_show_start_menu();
        } else {
            // Check window tabs
            int32_t tab_x = 44;
            for (uint32_t i = 0; i < desktop->window_count; ++i) {
                if (event->x >= tab_x && event->x < tab_x + 120) {
                    struct wm_window *win = desktop->windows[i];
                    if (win->minimized) {
                        // Restore minimized window
                        win->minimized = 0;
                        win->widget.visible = 1;
                        wm_focus_window(win);
                    } else {
                        // Minimize window
                        win->minimized = 1;
                        win->widget.visible = 0;
                    }
                    wm_draw_desktop();
                    break;
                }
                tab_x += 124;
            }
        }
    }
}

void wm_start_menu_draw(struct wm_widget *widget) {
    if (!start_menu_visible) return;
    
    struct wm_start_menu *sm = (struct wm_start_menu *)widget;
    
    // Expanded menu dimensions
    uint32_t menu_w = 450;
    uint32_t menu_h = 500;
    
    // Update widget size
    sm->widget.width = menu_w;
    sm->widget.height = menu_h;
    
    // Draw menu box with shadow
    fb_fill_rect(sm->widget.x + 4, sm->widget.y + 4, menu_w, menu_h, 0x00000000); // Shadow
    fb_fill_rect_alpha(sm->widget.x, sm->widget.y, menu_w, menu_h, 0x00202020, 245);
    fb_draw_rect_outline(sm->widget.x, sm->widget.y, menu_w, menu_h, 0x00666666);
    
    // Header with OS branding
    fb_fill_rect_gradient_v(sm->widget.x + 1, sm->widget.y + 1, menu_w - 2, 80, 
                            current_theme.accent_color, 0x00880000);
    fb_draw_string(sm->widget.x + 20, sm->widget.y + 25, "DevilCore OS v0.3", 0x00ffffff, 0);
    fb_draw_string(sm->widget.x + 20, sm->widget.y + 50, "The Ethical Hacking Platform", 0x00ff6666, 0);
    fb_draw_string(sm->widget.x + 20, sm->widget.y + 65, "root@devilcore", 0x00cccccc, 0);
    
    // Draw small logo in header (32x32 at position)
    logo32_draw(sm->widget.x + menu_w - 45, sm->widget.y + 24);
    
    // Categories and apps
    int32_t y_offset = sm->widget.y + 95;
    
    // === PRODUCTIVITY ===
    fb_draw_string(sm->widget.x + 15, y_offset, "PRODUCTIVITY", 0x00ff6666, 0);
    fb_draw_line(sm->widget.x + 15, y_offset + 12, sm->widget.x + menu_w - 15, y_offset + 12, 0x00444444);
    y_offset += 20;
    
    const char *prod_apps[] = {"Terminal (Win+T)", "File Manager (Win+E)", "Text Editor (Win+N)", "Calculator (Win+C)", "Notes", "Music", "Pictures", "Videos"};
    const char *prod_icons[] = {"terminal", "filemanager", "texteditor", "calculator", "notes", "music", "pictures", "videos"};
    for (int i = 0; i < 8; ++i) {
        const struct icon_data *app_icon = icon_get(prod_icons[i]);
        if (app_icon) draw_icon(sm->widget.x + 20, y_offset, app_icon);
        fb_draw_string(sm->widget.x + 55, y_offset + 8, prod_apps[i], 0x00ffffff, 0);
        y_offset += 32;
    }
    
    // === SYSTEM ===
    y_offset += 5;
    fb_draw_string(sm->widget.x + 15, y_offset, "SYSTEM", 0x00ff6666, 0);
    fb_draw_line(sm->widget.x + 15, y_offset + 12, sm->widget.x + menu_w - 15, y_offset + 12, 0x00444444);
    y_offset += 20;
    
    const char *sys_apps[] = {"Privacy Browser (Win+B)", "Settings (Win+I)", "System Monitor", "About DevilCore", "Disk Manager", "Network", "Logs", "Tasks"};
    const char *sys_icons[] = {"browser", "settings", "system_monitor", "info", "disk", "network", "logs", "cpu"};
    for (int i = 0; i < 8; ++i) {
        const struct icon_data *app_icon = icon_get(sys_icons[i]);
        if (app_icon) draw_icon(sm->widget.x + 20, y_offset, app_icon);
        fb_draw_string(sm->widget.x + 55, y_offset + 8, sys_apps[i], 0x00ffffff, 0);
        y_offset += 32;
    }
    
    // === POWER ===
    y_offset += 5;
    fb_draw_string(sm->widget.x + 15, y_offset, "POWER", 0x00ff6666, 0);
    fb_draw_line(sm->widget.x + 15, y_offset + 12, sm->widget.x + menu_w - 15, y_offset + 12, 0x00444444);
    y_offset += 20;
    
    const char *power_apps[] = {"Lock Screen (Win+L)", "Show Desktop (Win+D)", "Sleep", "Shutdown", "Restart"};
    const char *power_icons[] = {"lock", "desktop", "sleep", "power", "restart"};
    for (int i = 0; i < 5; ++i) {
        const struct icon_data *app_icon = icon_get(power_icons[i]);
        if (app_icon) draw_icon(sm->widget.x + 20, y_offset, app_icon);
        else fb_fill_rect_rounded(sm->widget.x + 20, y_offset, 24, 24, 3, 0x00555555);
        fb_draw_string(sm->widget.x + 55, y_offset + 8, power_apps[i], 0x00ffffff, 0);
        y_offset += 32;
    }
    
    // Keyboard shortcuts hint at bottom
    y_offset = sm->widget.y + menu_h - 25;
    fb_draw_line(sm->widget.x + 15, y_offset, sm->widget.x + menu_w - 15, y_offset, 0x00444444);
    fb_draw_string(sm->widget.x + 15, y_offset + 8, "Press Win key for shortcuts", 0x00888888, 0);
}

static void wm_start_menu_on_event(struct wm_widget *widget, struct wm_event *event) {
    (void)widget;
    if (!start_menu_visible) return;
    
    if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        // Get relative Y position in menu
        int32_t rel_y = event->y - (desktop->widget.height - 40 - 500 + 95);
        
        // Productivity section: y = 0 to 128 (4 items * 32px)
        if (rel_y >= 0 && rel_y < 128) {
            int idx = rel_y / 32;
            switch (idx) {
                case 0: wm_open_terminal(); break;
                case 1: wm_open_filemanager(); break;
                case 2: wm_open_texteditor(); break;
                case 3: wm_open_calculator(); break;
            }
            wm_hide_start_menu();
        }
        // System section: y = 133 to 261
        else if (rel_y >= 133 && rel_y < 261) {
            int idx = (rel_y - 133) / 32;
            switch (idx) {
                case 0: browser_open(); break;
                case 1: wm_open_settings(); break;
                case 2: show_sysinfo(); break; // System Monitor
                case 3: wm_show_about(); break;
            }
            wm_hide_start_menu();
        }
        // Power section: y = 266 to 426
        else if (rel_y >= 266 && rel_y < 426) {
            int idx = (rel_y - 266) / 32;
            switch (idx) {
                case 0: // Lock Screen
                    desktop->screen = WM_SCREEN_LOGIN;
                    memset(desktop->login_pass, 0, sizeof(desktop->login_pass));
                    break;
                case 1: // Show Desktop
                    for (uint32_t i = 0; i < desktop->window_count; i++) {
                        desktop->windows[i]->minimized = 1;
                    }
                    break;
                case 2: wm_open_terminal(); break; // Run
                case 3: // Shutdown
                    // Show shutdown dialog
                    break;
                case 4: // Restart
                    break;
            }
            wm_hide_start_menu();
        }
    }
}

void wm_context_menu_draw(struct wm_widget *widget) {
    struct wm_context_menu *cm = (struct wm_context_menu *)widget;
    if (!cm->widget.visible) return;
    
    fb_fill_rect_alpha(cm->widget.x, cm->widget.y, cm->widget.width, cm->widget.height, 0x00ffffff, 245);
    fb_draw_rect_outline(cm->widget.x, cm->widget.y, cm->widget.width, cm->widget.height, 0x00808080);
    
    for (uint32_t i = 0; i < cm->item_count; ++i) {
        fb_draw_string(cm->widget.x + 10, cm->widget.y + 5 + i * 25, cm->items[i], 0x00000000, 0);
    }
}

static void wm_context_menu_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_context_menu *cm = (struct wm_context_menu *)widget;
    if (!cm->widget.visible) return;
    
    if (event->type == WM_EVENT_MOUSE_PRESS && (event->buttons & 1)) {
        int idx = (event->y - 5) / 25;
        if (idx >= 0 && (uint32_t)idx < cm->item_count) {
            if (cm->callbacks[idx]) cm->callbacks[idx]();
            cm->widget.visible = 0;
        }
    }
}

void wm_show_context_menu(int32_t x, int32_t y) {
    struct wm_context_menu *cm = &desktop->context_menu;
    cm->widget.x = x;
    cm->widget.y = y;
    cm->widget.visible = 1;
    
    // Bring to front
    wm_widget_remove_child(&desktop->widget, &cm->widget);
    wm_widget_add_child(&desktop->widget, &cm->widget);
}

void wm_init(void) {
    serial_write_string("Initializing Window Manager...\n");
    desktop = (struct wm_desktop *)kmalloc(sizeof(struct wm_desktop));
    if (desktop == NULL) {
        serial_write_string("ERROR: Desktop allocation failed\n");
        return;
    }
    
    memset(desktop, 0, sizeof(struct wm_desktop));
    
    // Initialize desktop as root widget
    wm_widget_init(&desktop->widget, WM_WIDGET_GENERIC, "Desktop", 0, 0, framebuffer->width, framebuffer->height);
    
    // Initialize Taskbar
    wm_widget_init(&desktop->taskbar.widget, WM_WIDGET_TASKBAR, "Taskbar", 0, framebuffer->height - 40, framebuffer->width, 40);
    desktop->taskbar.widget.draw = wm_taskbar_draw;
    desktop->taskbar.widget.on_event = wm_taskbar_on_event;
    wm_widget_add_child(&desktop->widget, &desktop->taskbar.widget);
    
    // Initialize Start Menu
    wm_widget_init(&desktop->start_menu.widget, WM_WIDGET_MENU, "StartMenu", 4, framebuffer->height - 40 - 400, 300, 400);
    desktop->start_menu.widget.draw = wm_start_menu_draw;
    desktop->start_menu.widget.on_event = wm_start_menu_on_event;
    wm_widget_add_child(&desktop->widget, &desktop->start_menu.widget);

    // Initialize Context Menu
    wm_widget_init(&desktop->context_menu.widget, WM_WIDGET_MENU, "ContextMenu", 0, 0, 150, 100);
    desktop->context_menu.widget.draw = wm_context_menu_draw;
    desktop->context_menu.widget.on_event = wm_context_menu_on_event;
    desktop->context_menu.widget.visible = 0;
    desktop->context_menu.item_count = 4;
    strncpy(desktop->context_menu.items[0], "Refresh", 31);
    strncpy(desktop->context_menu.items[1], "Terminal", 31);
    strncpy(desktop->context_menu.items[2], "Settings", 31);
    strncpy(desktop->context_menu.items[3], "About", 31);
    desktop->context_menu.callbacks[0] = wm_draw_desktop;
    desktop->context_menu.callbacks[1] = wm_open_terminal;
    desktop->context_menu.callbacks[2] = wm_open_settings;
    desktop->context_menu.callbacks[3] = wm_show_about;
    wm_widget_add_child(&desktop->widget, &desktop->context_menu.widget);

    desktop->desktop_color = current_theme.bg_color;
    desktop->taskbar_height = 40;
    desktop->taskbar_color = current_theme.title_bar_color;
    
    desktop->mouse_x = framebuffer->width / 2;
    desktop->mouse_y = framebuffer->height / 2;
    desktop->screen = WM_SCREEN_DESKTOP;
    desktop->login_focus = 0;
    memset(desktop->login_user, 0, sizeof(desktop->login_user));
    memset(desktop->login_pass, 0, sizeof(desktop->login_pass));
    
    // Set start button callback
    desktop->on_start_click = wm_show_start_menu;
    
    // Icons will be added by main() to avoid duplicates and ensure correct callbacks
    
    serial_write_string("Drawing initial desktop...\n");
    wm_draw_desktop();
    serial_write_string("Initial desktop drawn.\n");
}

void wm_add_icon(const char *label, const char *icon_name, int32_t x, int32_t y, void (*on_click)(void)) {
    if (desktop->icon_count >= WM_MAX_ICONS) return;
    
    struct wm_icon *icon = &desktop->icons[desktop->icon_count++];
    wm_widget_init(&icon->widget, WM_WIDGET_ICON, label, x, y, 48, 64);
    icon->widget.draw = wm_icon_draw;
    icon->widget.on_event = wm_icon_on_event;
    strncpy(icon->label, label, 31);
    strncpy(icon->icon_name, icon_name, 31);
    icon->on_click = on_click;
    
    // Add icon as child of desktop
    wm_widget_add_child(&desktop->widget, &icon->widget);
}

void wm_show_start_menu(void) {
    start_menu_visible = !start_menu_visible;
}

void wm_hide_start_menu(void) {
    start_menu_visible = 0;
}

uint8_t wm_is_start_menu_visible(void) {
    return start_menu_visible;
}

uint8_t wm_is_cursor_visible(void) {
    return cursor_visible;
}

void wm_refresh(void) {
    if (desktop == NULL) return;
    
    uint64_t ticks = timer_ticks();
    // Blink cursor every 20 ticks (roughly 200ms at 100Hz)
    if (ticks - last_tick >= 20) {
        cursor_visible = !cursor_visible;
        last_tick = ticks;
        
        // Only mark focused window as dirty - don't redraw immediately
        // The main loop will handle the actual redraw
        if (desktop->focused_window != NULL) {
            wm_widget_mark_dirty(&desktop->focused_window->widget);
            desktop->needs_redraw = 1;
        }
    }
}

static void wm_draw_system_monitor(void) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    uint32_t widget_w = 220;
    uint32_t widget_h = 140;
    uint32_t widget_x = framebuffer->width - widget_w - 20;
    uint32_t widget_y = framebuffer->height - desktop->taskbar_height - widget_h - 20;
    
    // Modern glass-morphism shadow
    for (int i = 4; i >= 0; i--) {
        fb_fill_rect_rounded(widget_x + i, widget_y + i, widget_w, widget_h, 12, 0x00000000);
    }
    
    // Widget background with rounded corners (glass effect)
    fb_fill_rect_rounded(widget_x, widget_y, widget_w, widget_h, 12, 0x001a1a2e);
    // Glass overlay
    fb_fill_rect_alpha(widget_x, widget_y, widget_w, widget_h/2, 0x00ffffff, 8);
    // Border
    fb_draw_rect_rounded(widget_x, widget_y, widget_w, widget_h, 12, 0x00303050);
    
    // Title with icon
    fb_draw_string(widget_x + 12, widget_y + 12, "System Monitor", 0x00ffffff, 0);
    
    // Animated CPU pulse dot
    static uint8_t cpu_blink = 0;
    cpu_blink = (cpu_blink + 1) % 3;
    uint32_t cpu_colors[] = {0x0000ff00, 0x0055ff55, 0x0000aa00};
    fb_fill_rect_rounded(widget_x + widget_w - 20, widget_y + 10, 8, 8, 4, cpu_colors[cpu_blink]);
    
    // Memory section with modern progress bar
    uint32_t total = pmm_total_pages();
    uint32_t free = pmm_free_pages();
    uint32_t used = total - free;
    uint32_t usage_pct = (used * 100) / (total > 0 ? total : 1);
    uint32_t bar_w = widget_w - 24;
    
    fb_draw_string(widget_x + 12, widget_y + 38, "Memory", 0x00aaaaaa, 0);
    char mem_pct[16];
    sprintf(mem_pct, "%u%%", usage_pct);
    fb_draw_string(widget_x + widget_w - 40, widget_y + 38, mem_pct, 0x00ffffff, 0);
    
    // Modern rounded progress bar background
    fb_fill_rect_rounded(widget_x + 12, widget_y + 52, bar_w, 10, 5, 0x000d0d1a);
    fb_draw_rect_rounded(widget_x + 12, widget_y + 52, bar_w, 10, 5, 0x00202030);
    
    // Progress bar fill with gradient effect
    uint32_t fill_w = (bar_w * usage_pct) / 100;
    if (fill_w > 0) {
        uint32_t bar_color = 0x0000cc66;  // Green
        if (usage_pct > 50) bar_color = 0x00ffaa00;  // Yellow
        if (usage_pct > 75) bar_color = 0x00ff4444;  // Red
        fb_fill_rect_rounded(widget_x + 12, widget_y + 52, fill_w, 10, 5, bar_color);
        // Highlight on top half
        fb_fill_rect_alpha(widget_x + 12, widget_y + 52, fill_w, 5, 0x00ffffff, 40);
    }
    
    // Stats row - smart memory formatting (GB if > 1024 MB, else MB)
    char stats[64];
    uint32_t used_mb = (used * 4) / 1024;
    uint32_t total_mb = (total * 4) / 1024;
    
    if (total_mb >= 1024) {
        // Show in GB with one decimal
        uint32_t used_gb_tenths = (used_mb * 10) / 1024;
        uint32_t total_gb_tenths = (total_mb * 10) / 1024;
        sprintf(stats, "%u.%u GB / %u.%u GB", 
                used_gb_tenths / 10, used_gb_tenths % 10,
                total_gb_tenths / 10, total_gb_tenths % 10);
    } else {
        sprintf(stats, "%u MB / %u MB", used_mb, total_mb);
    }
    fb_draw_string(widget_x + 12, widget_y + 72, stats, 0x00888888, 0);
    
    // Divider line
    fb_draw_line(widget_x + 12, widget_y + 88, widget_x + widget_w - 12, widget_y + 88, 0x00202030);
    
    // Uptime with icon-style display
    uint64_t uptime_sec = timer_ticks() / 100;
    uint32_t sec = (uint32_t)(uptime_sec % 60);
    uint32_t min = (uint32_t)((uptime_sec / 60) % 60);
    uint32_t hr = (uint32_t)(uptime_sec / 3600);
    char uptime[32];
    sprintf(uptime, "%.2u:%.2u:%.2u", hr, min, sec);
    fb_draw_string(widget_x + 12, widget_y + 102, "Uptime", 0x00aaaaaa, 0);
    fb_draw_string(widget_x + widget_w - 70, widget_y + 102, uptime, 0x00ffffff, 0);
    
    // Window count pill
    char task_info[32];
    sprintf(task_info, "%u windows", desktop->window_count);
    uint32_t pill_w = strlen(task_info) * 8 + 16;
    fb_fill_rect_rounded(widget_x + widget_w - pill_w - 12, widget_y + 118, pill_w, 16, 8, 0x00252535);
    fb_draw_string(widget_x + widget_w - pill_w - 4, widget_y + 122, task_info, 0x00aaaaaa, 0);
}

// Modern desktop background with subtle pattern
static void draw_modern_desktop_bg(void) {
    uint32_t w = framebuffer->width;
    uint32_t h = framebuffer->height - desktop->taskbar_height;
    
    // Rich dark gradient background (modern dark theme)
    fb_fill_rect_gradient_v(0, 0, w, h, 0x001e1e2e, 0x00161620);
    
    // Subtle grid pattern overlay (very faint)
    for (uint32_t y = 0; y < h; y += 60) {
        fb_draw_line(0, y, w, y, 0x00252535);
    }
    for (uint32_t x = 0; x < w; x += 60) {
        fb_draw_line(x, 0, x, h, 0x00252535);
    }
    
    // Subtle vignette effect (darker corners)
    for (int i = 0; i < 40; i++) {
        uint8_t alpha = (40 - i) * 2;
        // Top edge
        fb_fill_rect_alpha(0, i, w, 1, 0x00000000, alpha);
        // Bottom edge (above taskbar)
        fb_fill_rect_alpha(0, h - 1 - i, w, 1, 0x00000000, alpha);
        // Left edge
        fb_fill_rect_alpha(i, 0, 1, h, 0x00000000, alpha);
        // Right edge
        fb_fill_rect_alpha(w - 1 - i, 0, 1, h, 0x00000000, alpha);
    }
}

void wm_draw_desktop(void) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    if (desktop->screen == WM_SCREEN_LOGIN) {
        wm_draw_login();
        return;
    }
    
    // Draw modern desktop background
    draw_modern_desktop_bg();
    
    // Header area with OS branding (top left)
    fb_draw_string(20, 20, "DevilCore OS", 0x00ffffff, 0);
    fb_draw_string(20, 35, "v0.1", 0x00888888, 0);
    
    // Mark all desktop children as dirty so they redraw after background clear
    wm_widget_mark_dirty_recursive(&desktop->widget);
    
    // Draw system monitor widget in bottom-right corner (above taskbar)
    wm_draw_system_monitor();
    
    // Draw the entire widget tree (icons, windows, etc.)
    wm_widget_draw(&desktop->widget);

    wm_draw_taskbar();
    wm_draw_cursor();
    
    fb_flip();
}

// Simple cursor sprite (16x16 arrow) - 1 = draw, 0 = transparent
static const uint8_t cursor_sprite[16][16] = {
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,1,1,1,1,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

void wm_draw_cursor(void) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    int32_t x = desktop->mouse_x;
    int32_t y = desktop->mouse_y;
    
    // Cursor colors
    uint32_t fill_color = 0x00ffffff;  // White fill
    uint32_t outline_color = 0x00000000; // Black outline
    
    // Click feedback - change color when clicking
    if (desktop->mouse_buttons & 1) {
        fill_color = current_theme.accent_color;
    }
    if (desktop->mouse_buttons & 2) {
        fill_color = 0x00ffaa00;  // Orange for right click
    }

    // Draw cursor using sprite for consistency and speed
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint8_t pixel = cursor_sprite[row][col];
            if (pixel == 1) {
                fb_put_pixel(x + col, y + row, fill_color);
            } else if (pixel == 2) {
                fb_put_pixel(x + col, y + row, outline_color);
            }
        }
    }
}

void wm_draw_start_menu(void) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    uint32_t menu_x = 4;
    uint32_t menu_y = framebuffer->height - desktop->taskbar_height - 480;
    uint32_t menu_w = 350;
    uint32_t menu_h = 480;
    
    // Menu background with shadow
    fb_fill_rect(menu_x + 3, menu_y + 3, menu_w, menu_h, 0x0040000000);
    fb_fill_rect_gradient_v(menu_x, menu_y, menu_w, menu_h, 0x00f0f0f0, 0x00e0e0e0);
    fb_draw_rect_outline(menu_x, menu_y, menu_w, menu_h, 0x00cccccc);
    
    // Logo section at top
    uint32_t logo_h = 100;
    fb_fill_rect_gradient_h(menu_x, menu_y, menu_w, logo_h, current_theme.accent_color, current_theme.accent_color);
    
    // Draw scaled logo (centerd)
    uint32_t logo_w_scaled = 80;
    uint32_t logo_h_scaled = 53; // Maintain aspect ratio
    uint32_t logo_x = menu_x + 15;
    uint32_t logo_y = menu_y + 20;
    fb_draw_bitmap_scaled(logo_x, logo_y, logo_w_scaled, logo_h_scaled, LOGO_WIDTH, LOGO_HEIGHT, logo_pixels);
    
    // OS Name next to logo
    fb_draw_string(logo_x + logo_w_scaled + 10, menu_y + 40, "DevilCore OS", 0x00ffffff, 0);
    fb_draw_string(logo_x + logo_w_scaled + 10, menu_y + 55, "v0.1", 0x00e0e0e0, 0);
    
    // Separator
    fb_draw_line(menu_x + 10, menu_y + logo_h, menu_x + menu_w - 10, menu_y + logo_h, 0x00cccccc);
    
    // Menu items with icons
    const char *menu_items[] = {"Terminal", "File Manager", "Text Editor", "Calculator", "Settings", "About"};
    uint32_t icon_colors[] = {0x00006600, 0x00996633, 0x00660066, 0x00666600, 0x00663399, 0x00666666};
    
    for (int i = 0; i < 6; ++i) {
        uint32_t item_y = menu_y + logo_h + 10 + (i * 45);
        
        // Highlight on hover
        if (desktop->mouse_y >= (int32_t)item_y && desktop->mouse_y < (int32_t)(item_y + 40) &&
            desktop->mouse_x >= (int32_t)menu_x && desktop->mouse_x < (int32_t)(menu_x + menu_w)) {
            fb_fill_rect(menu_x + 5, item_y, menu_w - 10, 40, 0x00e0e0e0);
        }
        
        // Icon placeholder with color
        fb_fill_rect_rounded(menu_x + 10, item_y + 4, 32, 32, 4, icon_colors[i]);
        fb_draw_string(menu_x + 15, item_y + 14, ">", 0x00ffffff, 0);
        
        // Item text
        fb_draw_string(menu_x + 55, item_y + 14, menu_items[i], 0x00000000, 0);
    }
}

void wm_draw_taskbar(void) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    // Ensure we use actual framebuffer dimensions
    uint32_t screen_w = framebuffer->width;
    uint32_t screen_h = framebuffer->height;
    uint32_t taskbar_h = desktop->taskbar_height;
    uint32_t y = screen_h - taskbar_h;
    
    // Solid background first to ensure no transparency issues
    fb_fill_rect(0, y, screen_w, taskbar_h, current_theme.title_bar_color);
    
    // Top highlight line for 3D effect
    fb_draw_line(0, y, screen_w, y, 0x00404040);
    fb_draw_line(0, y + 1, screen_w, y + 1, 0x00505050);
    
    // Subtle gradient overlay
    fb_fill_rect_gradient_v(0, y + 2, screen_w, taskbar_h - 2, 
                           current_theme.title_bar_color, 0x00282828);
    
    // Start button with logo (left side) - clearly visible
    uint32_t start_color = start_menu_visible ? 0x00aa4444 : current_theme.accent_color;
    fb_fill_rect_rounded(8, y + 6, 44, taskbar_h - 12, 6, start_color);
    fb_fill_rect_rounded(10, y + 8, 40, taskbar_h - 16, 4, 0x00ffffff); // White inner
    fb_draw_string(20, y + 14, "D", 0x00000000, 0); // D for DevilCore
    
    // Window tabs area (center-left) - start after start button
    int32_t tab_x = 65;
    int32_t max_tab_x = screen_w - 250; // Leave room for system tray
    
    for (uint32_t i = 0; i < desktop->window_count && tab_x < max_tab_x; ++i) {
        struct wm_window *win = desktop->windows[i];
        uint32_t tab_color = 0x00383838;
        uint32_t text_color = 0x00cccccc;
        
        if (win->minimized) {
            tab_color = 0x002a2a2a;
            text_color = 0x00888888;
        } else if (win->widget.focused) {
            tab_color = current_theme.accent_color;
            text_color = 0x00ffffff;
        }
        
        // Active window indicator (top line)
        if (win->widget.focused) {
            fb_fill_rect(tab_x, y + 2, 120, 3, 0x00ffffff);
        }
        
        fb_fill_rect_rounded(tab_x, y + 6, 120, taskbar_h - 12, 4, tab_color);
        fb_draw_string(tab_x + 8, y + 14, win->title, text_color, 0);
        
        // Minimize indicator (bottom line)
        if (win->minimized) {
            fb_fill_rect(tab_x + 10, y + taskbar_h - 8, 100, 2, 0x00666666);
        }
        
        tab_x += 124;
    }
    
    // System tray background (right side)
    uint32_t tray_w = 180;
    uint32_t tray_x = screen_w - tray_w - 10;
    fb_fill_rect_rounded(tray_x, y + 4, tray_w, taskbar_h - 8, 4, 0x00202020);
    
    // System tray items
    uint32_t item_x = tray_x + 10;
    
    // Network icon (N)
    fb_fill_rect_rounded(item_x, y + 8, 26, taskbar_h - 16, 3, 0x00303030);
    fb_draw_string(item_x + 7, y + 14, "N", 0x00aaaaaa, 0);
    item_x += 32;
    
    // Volume icon (V)  
    fb_fill_rect_rounded(item_x, y + 8, 26, taskbar_h - 16, 3, 0x00303030);
    fb_draw_string(item_x + 7, y + 14, "V", 0x00aaaaaa, 0);
    item_x += 32;
    
    // Battery/Power icon (B)
    fb_fill_rect_rounded(item_x, y + 8, 26, taskbar_h - 16, 3, 0x00303030);
    fb_draw_string(item_x + 7, y + 14, "B", 0x00aaaaaa, 0);
    item_x += 32;
    
    // Clock (rightmost in tray)
    uint64_t uptime_sec = timer_ticks() / 100;
    uint32_t min = (uint32_t)((uptime_sec / 60) % 60);
    uint32_t hr = (uint32_t)(uptime_sec / 3600) % 24;
    char clock[32];
    sprintf(clock, "%02u:%02u", hr, min);
    fb_draw_string(screen_w - 75, y + 14, clock, 0x00ffffff, 0);
    
    // Draw separator line on right edge of taskbar area
    fb_draw_line(screen_w - 1, y, screen_w - 1, screen_h, 0x00101010);
    
    // Draw start menu if visible
    if (start_menu_visible) {
        wm_draw_start_menu();
    }
}

// Track hovered window button for visual feedback
static int hovered_button = -1; // -1=none, 0=min, 1=max, 2=close

void wm_window_draw(struct wm_widget *widget) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL || !win->widget.visible) return;
    
    int32_t wx = win->widget.x;
    int32_t wy = win->widget.y;
    uint32_t ww = win->widget.width;
    uint32_t wh = win->widget.height;
    int32_t title_top = win->decorated ? wy - WM_TITLE_HEIGHT : wy;
    uint32_t total_h = win->decorated ? wh + WM_TITLE_HEIGHT : wh;

    // Modern multi-layer shadow
    for (int i = 8; i >= 0; i--) {
        uint8_t alpha = 15 - i;
        fb_fill_rect_rounded(wx + i, title_top + i, ww, total_h, 8, 0x00000000);
    }
    
    // Window border with rounded corners (outer)
    uint32_t border_color = win->widget.focused ? current_theme.accent_color : 0x00404040;
    fb_fill_rect_rounded(wx - 1, title_top - 1, ww + 2, total_h + 2, 10, border_color);
    
    // Title bar with glass effect (gradient)
    if (win->decorated) {
        uint32_t title_color1 = win->widget.focused ? current_theme.accent_color : 0x00303030;
        uint32_t title_color2 = win->widget.focused ? 
            fb_blend_colors(current_theme.accent_color, 0x000000, 50) : 0x00252525;
        
        // Title bar background with rounded top
        fb_fill_rect_rounded(wx, title_top, ww, WM_TITLE_HEIGHT, 8, title_color1);
        // Gradient overlay
        fb_fill_rect_alpha(wx + 2, title_top + 2, ww - 4, WM_TITLE_HEIGHT/2, 0x00ffffff, 20);
        
        // Window title with shadow
        fb_draw_string(wx + 12, title_top + 5, win->title, 0x00000000, 0); // Shadow
        fb_draw_string(wx + 11, title_top + 4, win->title, current_theme.fg_color, 0); // Main
        
        // Modern window control buttons (right side) - macOS style with always-visible colors
        int32_t btn_y = title_top + 5;
        int32_t btn_close_x = wx + ww - 28;
        int32_t btn_max_x = wx + ww - 50;
        int32_t btn_min_x = wx + ww - 72;
        
        // Base colors (traffic light style - always visible)
        uint32_t min_base = 0x00ffbd2e;    // Yellow
        uint32_t max_base = 0x0028c840;    // Green
        uint32_t close_base = 0x00ff5f56;  // Red
        
        // Dim colors when not hovered
        uint32_t min_dim = 0x00aa7d1e;
        uint32_t max_dim = 0x001a8028;
        uint32_t close_dim = 0x00aa3a32;
        
        // Minimize button (yellow)
        uint32_t min_color = (hovered_button == 0) ? min_base : min_dim;
        fb_fill_rect_rounded(btn_min_x, btn_y, 18, 16, 8, min_color);
        // Show icon only on hover
        if (hovered_button == 0) {
            fb_fill_rect(btn_min_x + 5, btn_y + 7, 8, 2, 0x008b4513); // Darker bar
        }
        
        // Maximize button (green)
        uint32_t max_color = (hovered_button == 1) ? max_base : max_dim;
        fb_fill_rect_rounded(btn_max_x, btn_y, 18, 16, 8, max_color);
        // Show icon only on hover
        if (hovered_button == 1) {
            if (win->maximized) {
                fb_draw_rect_outline(btn_max_x + 5, btn_y + 5, 8, 6, 0x00004400);
            } else {
                fb_fill_rect(btn_max_x + 5, btn_y + 4, 8, 8, 0x00004400);
            }
        }
        
        // Close button (red)
        uint32_t close_color = (hovered_button == 2) ? close_base : close_dim;
        fb_fill_rect_rounded(btn_close_x, btn_y, 18, 16, 8, close_color);
        // Show X only on hover
        if (hovered_button == 2) {
            fb_draw_line(btn_close_x + 5, btn_y + 4, btn_close_x + 13, btn_y + 12, 0x00440000);
            fb_draw_line(btn_close_x + 13, btn_y + 4, btn_close_x + 5, btn_y + 12, 0x00440000);
        }
    }
    
    // Window content area with rounded bottom corners
    uint32_t bg_color = win->bg_color != 0 ? win->bg_color : current_theme.bg_color;
    if (!win->widget.focused) {
        // Slightly dim when not focused
        bg_color = fb_blend_colors(bg_color, 0x000000, 30);
    }
    
    if (win->decorated) {
        // Content area with rounded bottom
        fb_fill_rect(wx, wy, ww, wh - 8, bg_color);
        fb_fill_rect_rounded(wx, wy + wh - 16, ww, 16, 8, bg_color);
    } else {
        fb_fill_rect_rounded(wx, wy, ww, wh, 8, bg_color);
    }

    // Draw content if callback exists
    if (win->draw_content) {
        win->draw_content(win);
    }
}

void wm_draw_window(struct wm_window *win) {
    if (win == NULL) return;
    wm_window_draw(&win->widget);
}

int wm_window_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL) return 0;
    
    if (event->type == WM_EVENT_MOUSE_PRESS) {
        wm_focus_window(win);
        desktop->needs_redraw = 1;
        // Check for title bar interaction
        if (win->decorated && event->y < 0 && event->y >= -(int32_t)WM_TITLE_HEIGHT) {
            uint32_t ww = win->widget.width;
            
            // Minimize button (leftmost of the three: width - 72 to width - 54)
            if (event->x >= (int32_t)(ww - 72) && event->x < (int32_t)(ww - 54)) {
                if (win->minimized) {
                    // Restore window
                    win->minimized = 0;
                    win->widget.visible = 1;
                    wm_focus_window(win);
                } else {
                    // Minimize window
                    win->minimized = 1;
                    win->widget.visible = 0;
                }
                wm_draw_desktop();
                return 1;
            }
            // Maximize/Restore button (middle: width - 50 to width - 32)
            if (event->x >= (int32_t)(ww - 50) && event->x < (int32_t)(ww - 32)) {
                if (win->maximized) {
                    // Restore window size and position
                    win->widget.x = win->restore_x;
                    win->widget.y = win->restore_y;
                    win->widget.width = win->restore_width;
                    win->widget.height = win->restore_height;
                    win->maximized = 0;
                } else {
                    // Save current position and size, then maximize
                    win->restore_x = win->widget.x;
                    win->restore_y = win->widget.y;
                    win->restore_width = win->widget.width;
                    win->restore_height = win->widget.height;
                    // Maximize to (almost) full screen, leaving taskbar visible
                    win->widget.x = 0;
                    win->widget.y = WM_TITLE_HEIGHT;
                    win->widget.width = framebuffer->width;
                    win->widget.height = framebuffer->height - WM_TITLE_HEIGHT - desktop->taskbar_height;
                    win->maximized = 1;
                }
                wm_draw_desktop();
                return 1;
            }
            // Close button (rightmost: width - 28 to width - 10)
            if (event->x >= (int32_t)(ww - 28) && event->x < (int32_t)(ww - 10)) {
                wm_destroy_window(win);
                return 1;
            }
            // Dragging logic (click on title bar but not on buttons)
            dragged_window = win;
            return 1;
        }
    }
    return 0;
}

static void wm_window_on_event_wrapper(struct wm_widget *widget, struct wm_event *event) {
    wm_window_on_event(widget, event);
}

struct wm_window *wm_create_window(const char *title, int32_t x, int32_t y, uint32_t w, uint32_t h) {
    if (desktop == NULL || w == 0 || h == 0) {
        return NULL;
    }
    
    if (desktop->window_count >= WM_MAX_WINDOWS) {
        return NULL;
    }
    
    struct wm_window *win = kmalloc(sizeof(struct wm_window));
    if (win == NULL) {
        return NULL;
    }
    
    memset(win, 0, sizeof(struct wm_window));
    
    wm_widget_init(&win->widget, WM_WIDGET_WINDOW, title, x, y, w, h);
    win->widget.draw = wm_window_draw;
    win->widget.on_event = wm_window_on_event_wrapper;
    
    win->id = next_window_id++;
    if (title != NULL) {
        strncpy(win->title, title, 63);
        win->title[63] = '\0';
    }
    
    win->widget.visible = 1;
    win->decorated = 1;
    win->minimized = 0;
    win->maximized = 0;
    win->restore_x = x;
    win->restore_y = y;
    win->restore_width = w;
    win->restore_height = h;
    win->bg_color = 0x00404040;
    win->fg_color = 0x00ffffff;
    
    desktop->windows[desktop->window_count++] = win;
    
    // Add window as child of desktop widget
    wm_widget_add_child(&desktop->widget, &win->widget);
    
    wm_focus_window(win);
    
    return win;
}

void wm_destroy_window(struct wm_window *win) {
    if (desktop == NULL || win == NULL) {
        return;
    }
    
    // Remove from widget tree
    wm_widget_remove_child(&desktop->widget, &win->widget);
    
    for (uint32_t i = 0; i < desktop->window_count; ++i) {
        if (desktop->windows[i] == win) {
            for (uint32_t j = i; j < desktop->window_count - 1; ++j) {
                desktop->windows[j] = desktop->windows[j + 1];
            }
            --desktop->window_count;
            break;
        }
    }
    
    if (desktop->focused_window == win) {
        desktop->focused_window = (desktop->window_count > 0) ? desktop->windows[desktop->window_count - 1] : NULL;
    }
    
    kfree(win);
}

void wm_focus_window(struct wm_window *win) {
    if (desktop == NULL || win == NULL) {
        return;
    }
    
    // Un-minimize if minimized
    if (win->minimized) {
        win->minimized = 0;
        win->widget.visible = 1;
    }
    
    if (desktop->focused_window != NULL) {
        desktop->focused_window->widget.focused = 0;
    }
    
    win->widget.focused = 1;
    desktop->focused_window = win;
    
    // Update widget z-order: move to end of children list
    wm_widget_remove_child(&desktop->widget, &win->widget);
    wm_widget_add_child(&desktop->widget, &win->widget);
}

void wm_move_window(struct wm_window *win, int32_t x, int32_t y) {
    if (win == NULL) {
        return;
    }
    
    win->widget.x = x;
    win->widget.y = y;
    desktop->needs_redraw = 1;
}

void wm_resize_window(struct wm_window *win, uint32_t w, uint32_t h) {
    if (win == NULL || w == 0 || h == 0) {
        return;
    }
    
    win->widget.width = w;
    win->widget.height = h;
    desktop->needs_redraw = 1;
}

void wm_handle_mouse(int32_t x, int32_t y, int32_t dx, int32_t dy, uint8_t buttons) {
    if (desktop == NULL || framebuffer == NULL) {
        return;
    }
    
    (void)x; (void)y;
    
    // Store old position for dirty tracking
    int32_t old_mouse_x = desktop->mouse_x;
    int32_t old_mouse_y = desktop->mouse_y;
    
    // Update cursor position
    desktop->mouse_x += dx;
    desktop->mouse_y += dy;
    uint8_t old_buttons = desktop->mouse_buttons;
    desktop->mouse_buttons = buttons;
    
    if (desktop->mouse_x < 0) desktop->mouse_x = 0;
    if (desktop->mouse_y < 0) desktop->mouse_y = 0;
    if (desktop->mouse_x >= (int32_t)framebuffer->width) desktop->mouse_x = (int32_t)framebuffer->width - 1;
    if (desktop->mouse_y >= (int32_t)framebuffer->height) desktop->mouse_y = (int32_t)framebuffer->height - 1;
    
    // Track if redraw is actually needed
    uint8_t needs_redraw = 0;
    
    // Only redraw if mouse moved significantly (more than 2 pixels)
    int32_t dx_abs = (desktop->mouse_x - old_mouse_x) < 0 ? -(desktop->mouse_x - old_mouse_x) : (desktop->mouse_x - old_mouse_x);
    int32_t dy_abs = (desktop->mouse_y - old_mouse_y) < 0 ? -(desktop->mouse_y - old_mouse_y) : (desktop->mouse_y - old_mouse_y);
    if (dx_abs > 2 || dy_abs > 2) {
        needs_redraw = 1;
    }
    
    // Dispatch mouse events to queue
    struct wm_event event;
    event.x = desktop->mouse_x;
    event.y = desktop->mouse_y;
    event.dx = dx;
    event.dy = dy;
    event.buttons = buttons;
    
    event.type = WM_EVENT_MOUSE;
    wm_push_event(&event);

    if ((buttons & 1) && !(old_buttons & 1)) {
        event.type = WM_EVENT_MOUSE_PRESS;
        wm_push_event(&event);
        needs_redraw = 1;  // Click needs redraw
    } else if (!(buttons & 1) && (old_buttons & 1)) {
        event.type = WM_EVENT_MOUSE_RELEASE;
        wm_push_event(&event);
        needs_redraw = 1;  // Release needs redraw
    }

    if ((buttons & 2) && !(old_buttons & 2)) {
        event.type = WM_EVENT_MOUSE_PRESS;
        event.buttons = 2; // Right click
        wm_push_event(&event);
        needs_redraw = 1;
    }
    
    // Only redraw on mouse movement if buttons are pressed (dragging)
    // or if we have a hover state change (handled in event processing)
    if ((dx != 0 || dy != 0) && (buttons != 0)) {
        needs_redraw = 1;
    }
    
    // Check for window button hover effects
    if (desktop->screen == WM_SCREEN_DESKTOP && desktop->window_count > 0) {
        int old_hovered = hovered_button;
        hovered_button = -1; // Reset hover state
        
        // Check if mouse is over any window's title bar buttons
        for (uint32_t i = 0; i < desktop->window_count; i++) {
            struct wm_window *win = desktop->windows[i];
            if (win->minimized || !win->widget.visible || !win->decorated) continue;
            
            int32_t wx = win->widget.x;
            int32_t wy = win->widget.y;
            int32_t ww = win->widget.width;
            int32_t title_top = wy - WM_TITLE_HEIGHT;
            
            // Check if mouse is in title bar area (y range)
            if (desktop->mouse_y >= title_top && desktop->mouse_y < title_top + 24) {
                // Check if mouse is in button area (right side of title bar)
                if (desktop->mouse_x >= wx + ww - 72 && desktop->mouse_x < wx + ww - 10) {
                    // Mouse is over buttons - determine which one
                    int32_t rel_x = desktop->mouse_x - (wx + ww - 72);
                    
                    // Button order: minimize (0-18), maximize (18-36), close (36-54)
                    // But actual positions are: min at 72, max at 50, close at 28 from right edge
                    // So relative to (ww - 72): min=0-18, max=22-40, close=44-62
                    int32_t btn_min_x = wx + ww - 72;
                    int32_t btn_max_x = wx + ww - 50;
                    int32_t btn_close_x = wx + ww - 28;
                    
                    if (desktop->mouse_x >= btn_min_x && desktop->mouse_x < btn_min_x + 18) {
                        hovered_button = 0; // Minimize
                    } else if (desktop->mouse_x >= btn_max_x && desktop->mouse_x < btn_max_x + 18) {
                        hovered_button = 1; // Maximize
                    } else if (desktop->mouse_x >= btn_close_x && desktop->mouse_x < btn_close_x + 18) {
                        hovered_button = 2; // Close
                    }
                    
                    if (hovered_button != old_hovered) {
                        needs_redraw = 1;
                    }
                    break; // Only check topmost visible window
                }
            }
        }
        
        // If hover state changed to none, redraw
        if (old_hovered != -1 && hovered_button == -1) {
            needs_redraw = 1;
        }
    }
    
    if (needs_redraw) {
        desktop->needs_redraw = 1;
    }
}

void wm_draw_login(void) {
    // Beautiful gradient background (Ubuntu-style)
    fb_fill_rect_gradient_v(0, 0, framebuffer->width, framebuffer->height, 
                              0x00202020, 0x001a1a1a);
    
    // Draw centered login box with shadow
    uint32_t box_w = 450;
    uint32_t box_h = 350;
    uint32_t box_x = (framebuffer->width - box_w) / 2;
    uint32_t box_y = (framebuffer->height - box_h) / 2;
    
    // Shadow
    fb_fill_rect(box_x + 5, box_y + 5, box_w, box_h, 0x0040000000);
    
    // Main box with gradient
    fb_fill_rect_gradient_v(box_x, box_y, box_w, box_h, 0x002a2a2a, 0x00303030);
    fb_draw_rect_outline(box_x, box_y, box_w, box_h, 0x00505050);
    
    // Logo at top of login box
    uint32_t logo_w_scaled = 100;
    uint32_t logo_h_scaled = 67; // Maintain aspect ratio
    uint32_t logo_x = box_x + (box_w - logo_w_scaled) / 2;
    fb_draw_bitmap_scaled(logo_x, box_y + 20, logo_w_scaled, logo_h_scaled, 
                          LOGO_WIDTH, LOGO_HEIGHT, logo_pixels);
    
    // OS Name
    fb_draw_string(box_x + 150, box_y + 100, "DevilCore OS", 0x00ffffff, 0);
    fb_draw_string(box_x + 140, box_y + 120, "v0.1 - Ethical Hacking Environment", 0x00aaaaaa, 0);
    
    // Separator
    fb_draw_line(box_x + 40, box_y + 140, box_x + box_w - 40, box_y + 140, 0x00505050);
    
    // User field
    fb_draw_string(box_x + 40, box_y + 160, "Username:", 0x00cccccc, 0);
    fb_fill_rect_rounded(box_x + 40, box_y + 180, box_w - 80, 30, 4, 0x00222222);
    if (desktop->login_focus == 0) fb_draw_rect_outline(box_x + 40, box_y + 180, box_w - 80, 30, current_theme.accent_color);
    fb_draw_string(box_x + 50, box_y + 190, desktop->login_user, 0x00ffffff, 0);
    
    // Pass field
    fb_draw_string(box_x + 40, box_y + 220, "Password:", 0x00cccccc, 0);
    fb_fill_rect_rounded(box_x + 40, box_y + 240, box_w - 80, 30, 4, 0x00222222);
    if (desktop->login_focus == 1) fb_draw_rect_outline(box_x + 40, box_y + 240, box_w - 80, 30, current_theme.accent_color);
    
    char masked_pass[32];
    size_t pass_len = strlen(desktop->login_pass);
    for (size_t i = 0; i < pass_len; ++i) masked_pass[i] = '*';
    masked_pass[pass_len] = '\0';
    fb_draw_string(box_x + 50, box_y + 250, masked_pass, 0x00ffffff, 0);
    
    fb_draw_string(box_x + 40, box_y + 290, "TAB: switch field | ENTER: login", 0x00666666, 0);
    
    wm_draw_cursor();
    fb_flip();
}

int wm_verify_login(const char *user, const char *pass) {
    struct vfs_file file;
    if (vfs_open(&file, "/etc/passwd", 0) != 0) {
        return 0;
    }
    
    char buffer[1024];
    size_t read_bytes;
    if (vfs_read(&file, buffer, 1024, &read_bytes) != 0) {
        vfs_close(&file);
        return 0;
    }
    buffer[read_bytes] = '\0';
    vfs_close(&file);
    
    char *line = buffer;
    while (line && *line) {
        char *next_line = strchr(line, '\n');
        if (next_line) *next_line = '\0';
        
        char *u = line;
        char *p_field = strchr(u, ':');
        if (p_field) {
            *p_field = '\0';
            p_field++;
            char *p = p_field;
            char *extra = strchr(p, ':');
            if (extra) *extra = '\0';
            
            if (strcmp(u, user) == 0) {
                if (strcmp(p, "x") == 0) {
                    return (strcmp(pass, "devil") == 0);
                }
                return (strcmp(p, pass) == 0);
            }
        }
        
        if (next_line) line = next_line + 1;
        else break;
    }
    
    return 0;
}

extern void serial_write_string(const char *text);
extern void serial_write_char(char ch);
extern void serial_write_dec(uint64_t value);

void wm_handle_key(uint8_t key, uint8_t modifiers) {
    if (desktop == NULL) return;
    
    struct wm_event event;
    event.type = WM_EVENT_KEY;
    event.key = key;
    event.modifiers = modifiers;
    
    if (desktop->screen == WM_SCREEN_LOGIN) {
        if (key == '\t') {
            desktop->login_focus = !desktop->login_focus;
        } else if (key == '\n') {
            if (wm_verify_login(desktop->login_user, desktop->login_pass)) {
                desktop->screen = WM_SCREEN_DESKTOP;
            } else {
                memset(desktop->login_pass, 0, sizeof(desktop->login_pass));
            }
        } else if (key == '\b') {
            char *buf = (desktop->login_focus == 0) ? desktop->login_user : desktop->login_pass;
            size_t len = strlen(buf);
            if (len > 0) buf[len-1] = '\0';
        } else if (key >= 32 && key <= 126) {
            char *buf = (desktop->login_focus == 0) ? desktop->login_user : desktop->login_pass;
            size_t len = strlen(buf);
            if (len < 31) {
                buf[len] = (char)key;
                buf[len+1] = '\0';
            }
        }
        desktop->needs_redraw = 1;
        return;
    }
    
    // Handle global keyboard shortcuts (GUI/Win key, Ctrl+Alt, etc.)
    if (modifiers & (4 | 8 | 16)) { // GUI, Ctrl, or Alt pressed
        wm_handle_shortcuts(key, modifiers);
        // If GUI key was used for shortcut, don't pass to focused window
        if (modifiers & 4) return;
    }

    // Dispatch key event to widget tree (focused window)
    wm_widget_dispatch_event(&desktop->widget, &event);
    
    desktop->needs_redraw = 1;
}

struct wm_desktop *wm_get_desktop(void) {
    return desktop;
}

extern void open_terminal(void);
extern void show_sysinfo(void);
extern void browser_open(void);

// Forward declaration for shortcut handler (defined at end of file)
void wm_handle_shortcuts(uint8_t key, uint8_t modifiers);

// Application launcher callbacks
void wm_open_terminal(void) {
    open_terminal();
}

void wm_open_filemanager(void) {
    fm_open();
}

void wm_open_texteditor(void) {
    te_open();
}

void wm_open_calculator(void) {
    calc_open();
}

void wm_open_settings(void) {
    // For now, settings can just open the info dialog or a placeholder
    show_sysinfo();
}

void wm_show_about(void) {
    show_sysinfo();
}

// Handle special keys (GUI/Windows key, etc.)
void wm_handle_special_key(uint8_t special_key) {
    if (desktop == NULL) return;
    
    // 0xE0 = GUI/Windows/Super key pressed
    if (special_key == 0xE0) {
        // Toggle start menu
        wm_show_start_menu();
        desktop->needs_redraw = 1;
    }
}

// Keyboard shortcuts handler - called from wm_handle_key with modifiers
void wm_handle_shortcuts(uint8_t key, uint8_t modifiers) {
    // GUI/Windows key shortcuts
    if (modifiers & 4) { // GUI key pressed
        switch (key) {
            case 't': case 'T': // Win+T = Terminal
                wm_open_terminal();
                break;
            case 'e': case 'E': // Win+E = File Manager
                wm_open_filemanager();
                break;
            case 'n': case 'N': // Win+N = Notepad/Text Editor
                wm_open_texteditor();
                break;
            case 'c': case 'C': // Win+C = Calculator
                wm_open_calculator();
                break;
            case 'i': case 'I': // Win+I = Settings
                wm_open_settings();
                break;
            case 'b': case 'B': // Win+B = Browser
                browser_open();
                break;
            case 'l': case 'L': // Win+L = Lock screen
                desktop->screen = WM_SCREEN_LOGIN;
                memset(desktop->login_pass, 0, sizeof(desktop->login_pass));
                break;
            case 'd': case 'D': // Win+D = Show desktop (minimize all)
                for (uint32_t i = 0; i < desktop->window_count; i++) {
                    desktop->windows[i]->minimized = 1;
                }
                break;
            case 'r': case 'R': // Win+R = Run dialog (terminal for now)
                wm_open_terminal();
                break;
        }
        desktop->needs_redraw = 1;
    }
    
    // Ctrl+Alt shortcuts
    if ((modifiers & 8) && (modifiers & 16)) { // Ctrl+Alt
        if (key == 't' || key == 'T') {
            wm_open_terminal(); // Ctrl+Alt+T = Terminal
        }
    }
}
