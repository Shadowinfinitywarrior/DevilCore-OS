#ifndef DEVILCORE_WM_H
#define DEVILCORE_WM_H

#include <stdint.h>

#include "framebuffer.h"

#define WM_MAX_WINDOWS 32
#define WM_MAX_ICONS 16
#define WM_MAX_CHILDREN 64
#define WM_TITLE_HEIGHT 24
#define WM_BORDER_SIZE 2

enum wm_screen {
    WM_SCREEN_LOGIN,
    WM_SCREEN_DESKTOP
};

enum wm_widget_type {
    WM_WIDGET_GENERIC,
    WM_WIDGET_WINDOW,
    WM_WIDGET_TASKBAR,
    WM_WIDGET_ICON,
    WM_WIDGET_BUTTON,
    WM_WIDGET_LABEL,
    WM_WIDGET_MENU,
    WM_WIDGET_WIDGET // Dashboard widgets
};

struct wm_theme {
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t accent_color;
    uint32_t border_color;
    uint32_t title_bar_color;
    uint32_t selection_color;
};

extern struct wm_theme current_theme;

void wm_set_theme(struct wm_theme new_theme);

#define WM_EVENT_MOUSE 1
#define WM_EVENT_KEY 2
#define WM_EVENT_MOUSE_PRESS 3
#define WM_EVENT_MOUSE_RELEASE 4

struct wm_event {
    uint32_t type;
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    uint32_t buttons;
    uint8_t key;
    uint8_t modifiers;
};

struct wm_widget {
    uint32_t id;
    enum wm_widget_type type;
    char name[64];
    
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    
    uint8_t visible;
    uint8_t focused;
    uint8_t enabled;
    
    uint8_t dirty;
    int32_t last_x;
    int32_t last_y;
    uint32_t last_width;
    uint32_t last_height;
    
    struct wm_widget *parent;
    struct wm_widget *children[WM_MAX_CHILDREN];
    uint32_t child_count;
    
    void (*draw)(struct wm_widget *);
    void (*on_event)(struct wm_widget *, struct wm_event *);
    void *data;
};

struct wm_window {
    struct wm_widget widget;
    
    uint32_t id;
    char title[64];
    
    uint8_t decorated;
    uint8_t modal;
    uint8_t minimized;
    uint8_t maximized;
    
    int32_t restore_x;
    int32_t restore_y;
    uint32_t restore_width;
    uint32_t restore_height;
    
    uint32_t bg_color;
    uint32_t fg_color;
    
    void (*draw_content)(struct wm_window *);
};

struct wm_icon {
    struct wm_widget widget;
    char label[32];
    char icon_name[32];
    void (*on_click)(void);
};

struct wm_taskbar {
    struct wm_widget widget;
    uint32_t color;
    uint32_t clock_x;
};

struct wm_start_menu {
    struct wm_widget widget;
    uint32_t color;
    uint8_t visible;
};

struct wm_context_menu {
    struct wm_widget widget;
    char items[8][32];
    void (*callbacks[8])(void);
    uint32_t item_count;
};

struct wm_dashboard_widget {
    struct wm_widget widget;
    uint32_t bg_color;
    uint32_t fg_color;
    void (*update)(struct wm_dashboard_widget *);
};

struct wm_desktop {
    struct wm_widget widget;
    
    struct wm_taskbar taskbar;
    struct wm_start_menu start_menu;
    struct wm_context_menu context_menu;
    
    struct wm_dashboard_widget cpu_widget;
    struct wm_dashboard_widget mem_widget;
    
    uint32_t background;
    uint32_t desktop_color;
    uint32_t taskbar_height;
    uint32_t taskbar_color;
    
    struct wm_window *windows[WM_MAX_WINDOWS];
    uint32_t window_count;
    struct wm_window *focused_window;
    
    struct wm_icon icons[WM_MAX_ICONS];
    uint32_t icon_count;
    
    int32_t mouse_x;
    int32_t mouse_y;
    uint8_t mouse_buttons;
    
    enum wm_screen screen;
    char login_user[32];
    char login_pass[32];
    int login_focus; // 0: user, 1: pass
    
    void (*on_start_click)(void);
    uint8_t needs_redraw;
};

void wm_init(void);
void wm_draw_desktop(void);
void wm_draw_taskbar(void);
void wm_draw_cursor(void);
void wm_draw_login(void);
int wm_verify_login(const char *user, const char *pass);

struct wm_window *wm_create_window(const char *title, int32_t x, int32_t y, uint32_t w, uint32_t h);
void wm_destroy_window(struct wm_window *win);
void wm_draw_window(struct wm_window *win);
void wm_focus_window(struct wm_window *win);
void wm_move_window(struct wm_window *win, int32_t x, int32_t y);
void wm_resize_window(struct wm_window *win, uint32_t w, uint32_t h);

void wm_handle_mouse(int32_t x, int32_t y, int32_t dx, int32_t dy, uint8_t buttons);
void wm_handle_key(uint8_t key, uint8_t modifiers);
void wm_handle_special_key(uint8_t special_key); // Handle special keys like GUI/Windows key

// Application launchers
void browser_open(void);
void browser_close(void);
void wm_open_filemanager(void);
void wm_open_texteditor(void);
void wm_open_calculator(void);
void wm_open_calendar(void);
void wm_open_sysmonitor(void);
void notes_open(void);
void fm_open(void);
void te_open(void);
void show_sysinfo(void);
void wm_handle_shortcuts(uint8_t key, uint8_t modifiers);

void wm_process_events(void);
void wm_refresh(void);

struct wm_desktop *wm_get_desktop(void);

void wm_add_icon(const char *label, const char *icon_name, int32_t x, int32_t y, void (*on_click)(void));

// Widget management
void wm_widget_init(struct wm_widget *widget, enum wm_widget_type type, const char *name, int32_t x, int32_t y, uint32_t w, uint32_t h);
void wm_widget_add_child(struct wm_widget *parent, struct wm_widget *child);
void wm_widget_remove_child(struct wm_widget *parent, struct wm_widget *child);
void wm_widget_mark_dirty(struct wm_widget *widget);
void wm_widget_draw(struct wm_widget *widget);
void wm_widget_dispatch_event(struct wm_widget *widget, struct wm_event *event);

// Window event handler (to be called by app-specific handlers for title bar buttons)
// Returns 1 if event was handled (button click), 0 otherwise
int wm_window_on_event(struct wm_widget *widget, struct wm_event *event);

// Start menu
void wm_show_start_menu(void);
void wm_hide_start_menu(void);
uint8_t wm_is_start_menu_visible(void);
uint8_t wm_is_cursor_visible(void);

// Modern UI enhancements
void wm_set_theme(struct wm_theme new_theme);
void wm_draw_shadow(struct wm_window *win);

// Notifications
void wm_notify(const char *text, uint32_t color);
void wm_draw_notifications(void);

#endif