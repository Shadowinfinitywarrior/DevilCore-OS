#include "settings.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include "timer.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>

extern struct wm_desktop *desktop;

static struct wm_window *settings_window = NULL;
static uint8_t selected_tab = 0; // 0=General, 1=Display, 2=Network, 3=Security, 4=System, 5=About
static uint8_t theme_idx = 0;
static uint8_t resolution_idx = 0;
static uint8_t wallpaper_idx = 0;
static uint8_t font_size = 1; // 0=Small, 1=Normal, 2=Large
static uint8_t animations_enabled = 1;
static uint8_t sound_enabled = 1;
static uint8_t notifications_enabled = 1;
static uint8_t auto_save = 1;
static uint8_t firewall_enabled = 1;
static uint8_t encryption_enabled = 0;
static uint8_t secure_boot = 0;
static uint8_t dhcp_enabled = 1;
static uint8_t show_seconds = 0;
static uint8_t dark_mode = 1;
static uint8_t high_contrast = 0;

// Predefined themes
static const char *theme_names[] = {"Dark", "Blue", "Green", "Purple", "Orange", "Red"};
static uint32_t theme_colors[][3] = {
    {0x001a1a1a, 0x002d2d2d, 0x000055aa}, // Dark
    {0x001a1a2a, 0x002d2d3d, 0x000000aa}, // Blue
    {0x001a2a1a, 0x002d3d2d, 0x0000aa00}, // Green
    {0x002a1a2a, 0x003d2d3d, 0x00aa00aa}, // Purple
    {0x002a1a1a, 0x003d2d2d, 0x00aa5500}, // Orange
    {0x002a0a0a, 0x003d1515, 0x00aa0000}  // Red
};
#define NUM_THEMES (sizeof(theme_names) / sizeof(theme_names[0]))

// Resolutions
static const char *resolution_names[] = {"800x600", "1024x768", "1280x720", "1366x768", "1920x1080"};
#define NUM_RESOLUTIONS 5

// Wallpaper names
static const char *wallpaper_names[] = {"Default", "Dark", "Blue Gradient", "Matrix", "Abstract"};
#define NUM_WALLPAPERS 5

// Font sizes
static const char *font_size_names[] = {"Small", "Normal", "Large"};

// Toggle/checkbox drawing helper
static void draw_checkbox(uint32_t x, uint32_t y, uint32_t checked, const char *label) {
    uint32_t box_color = checked ? 0x0000aa00 : 0x00333333;
    fb_fill_rect_rounded(x, y, 20, 20, 3, box_color);
    fb_draw_rect_rounded(x, y, 20, 20, 3, 0x00555555);
    if (checked) {
        // Draw checkmark
        fb_draw_line(x + 5, y + 10, x + 9, y + 14, 0x00ffffff);
        fb_draw_line(x + 9, y + 14, x + 15, y + 6, 0x00ffffff);
    }
    fb_draw_string(x + 28, y + 4, label, 0x00cccccc, 0);
}

// Button drawing helper
static void draw_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *label, uint32_t color) {
    fb_fill_rect_rounded(x, y, w, h, 5, color);
    fb_draw_rect_rounded(x, y, w, h, 5, 0x00555555);
    uint32_t label_x = x + (w - strlen(label) * 8) / 2;
    fb_draw_string(label_x, y + (h - 16) / 2 + 4, label, 0x00ffffff, 0);
}

// Slider drawing helper
static void draw_slider(uint32_t x, uint32_t y, uint32_t w, uint32_t value, uint32_t max) {
    // Track
    fb_fill_rect_rounded(x, y + 6, w, 8, 4, 0x00222222);
    // Fill
    uint32_t fill_w = (w * value) / max;
    if (fill_w > 0) {
        fb_fill_rect_rounded(x, y + 6, fill_w, 8, 4, 0x000055aa);
    }
    // Knob
    uint32_t knob_x = x + fill_w - 6;
    if (knob_x > x + w - 12) knob_x = x + w - 12;
    fb_fill_rect_rounded(knob_x, y, 12, 20, 6, 0x00ffffff);
}

void settings_draw(struct wm_window *win) {
    if (win == NULL || desktop == NULL) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Tab bar - 6 tabs
    uint32_t tab_w = w / 6;
    const char *tabs[] = {"General", "Display", "Network", "Security", "System", "About"};
    
    for (int i = 0; i < 6; ++i) {
        uint32_t tab_color = (i == selected_tab) ? 0x00444444 : 0x002a2a2a;
        uint32_t text_color = (i == selected_tab) ? 0x00ffffff : 0x00aaaaaa;
        fb_fill_rect_rounded(x + i * tab_w, y, tab_w - 2, 30, 4, tab_color);
        // Center text in tab
        uint32_t text_x = x + i * tab_w + (tab_w - strlen(tabs[i]) * 8) / 2;
        fb_draw_string(text_x, y + 10, tabs[i], text_color, 0);
    }
    
    // Content area
    y += 30;
    uint32_t content_h = h - 30;
    fb_fill_rect(x, y, w, content_h, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, content_h, 0x00505050);
    
    if (selected_tab == 0) {
        // General settings - Left column: Appearance, Right column: Behavior
        fb_draw_string(x + 10, y + 15, "Appearance", 0x00ffffff, 0);
        fb_draw_line(x + 10, y + 32, x + w/2 - 20, y + 32, 0x00555555);
        
        // Theme selection
        fb_draw_string(x + 10, y + 45, "Theme:", 0x00cccccc, 0);
        for (uint32_t i = 0; i < NUM_THEMES && i < 4; ++i) {
            uint32_t btn_y = y + 60 + i * 28;
            uint32_t btn_color = (i == theme_idx) ? 0x000055aa : 0x00333333;
            draw_button(x + 10, btn_y, 140, 24, theme_names[i], btn_color);
            fb_fill_rect(x + 155, btn_y + 4, 16, 16, theme_colors[i][2]);
        }
        
        // Wallpaper selection
        fb_draw_string(x + 10, y + 180, "Wallpaper:", 0x00cccccc, 0);
        for (uint32_t i = 0; i < NUM_WALLPAPERS && i < 3; ++i) {
            uint32_t btn_y = y + 195 + i * 28;
            uint32_t btn_color = (i == wallpaper_idx) ? 0x000055aa : 0x00333333;
            draw_button(x + 10, btn_y, 140, 24, wallpaper_names[i], btn_color);
        }
        
        // Right column - Behavior
        fb_draw_string(x + w/2 + 10, y + 15, "Behavior", 0x00ffffff, 0);
        fb_draw_line(x + w/2 + 10, y + 32, x + w - 20, y + 32, 0x00555555);
        
        draw_checkbox(x + w/2 + 10, y + 45, animations_enabled, "Animations");
        draw_checkbox(x + w/2 + 10, y + 75, sound_enabled, "Sound Effects");
        draw_checkbox(x + w/2 + 10, y + 105, notifications_enabled, "Notifications");
        draw_checkbox(x + w/2 + 10, y + 135, auto_save, "Auto-save Settings");
        draw_checkbox(x + w/2 + 10, y + 165, dark_mode, "Dark Mode");
        draw_checkbox(x + w/2 + 10, y + 195, high_contrast, "High Contrast");
        
        // Apply button at bottom
        draw_button(x + 10, y + content_h - 40, 120, 32, "Apply Changes", 0x0000aa00);
        draw_button(x + 140, y + content_h - 40, 80, 32, "Reset", 0x00aa0000);
        
    } else if (selected_tab == 1) {
        // Display settings
        fb_draw_string(x + 10, y + 15, "Screen Resolution", 0x00ffffff, 0);
        fb_draw_line(x + 10, y + 32, x + 200, y + 32, 0x00555555);
        
        for (uint32_t i = 0; i < NUM_RESOLUTIONS; ++i) {
            uint32_t btn_y = y + 40 + i * 32;
            uint32_t btn_color = (i == resolution_idx) ? 0x000055aa : 0x00333333;
            draw_button(x + 10, btn_y, 180, 28, resolution_names[i], btn_color);
        }
        
        // Display info
        fb_draw_string(x + 220, y + 15, "Display Information", 0x00ffffff, 0);
        fb_draw_line(x + 220, y + 32, x + w - 20, y + 32, 0x00555555);
        fb_draw_string(x + 220, y + 45, "Current: 1024x768 @ 60Hz", 0x00aaaaaa, 0);
        fb_draw_string(x + 220, y + 65, "Color Depth: 32-bit RGBA", 0x00aaaaaa, 0);
        fb_draw_string(x + 220, y + 85, "Framebuffer: Linear", 0x00aaaaaa, 0);
        
        // Font size
        fb_draw_string(x + 220, y + 115, "Font Size:", 0x00cccccc, 0);
        for (uint32_t i = 0; i < 3; ++i) {
            uint32_t btn_x = x + 220 + i * 70;
            uint32_t btn_color = (i == font_size) ? 0x000055aa : 0x00333333;
            draw_button(btn_x, y + 135, 65, 28, font_size_names[i], btn_color);
        }
        
        // Brightness slider (visual only for now)
        fb_draw_string(x + 220, y + 175, "Brightness:", 0x00cccccc, 0);
        draw_slider(x + 220, y + 195, 180, 75, 100);
        
    } else if (selected_tab == 2) {
        // Network settings
        fb_draw_string(x + 10, y + 15, "Network Configuration", 0x00ffffff, 0);
        fb_draw_line(x + 10, y + 32, x + 250, y + 32, 0x00555555);
        
        draw_checkbox(x + 10, y + 45, dhcp_enabled, "DHCP (Auto IP)");
        draw_checkbox(x + 10, y + 75, 1, "IPv6 Support");
        
        fb_draw_string(x + 10, y + 115, "IP Address:", 0x00cccccc, 0);
        fb_draw_string(x + 120, y + 115, "192.168.1.100", 0x00ffffff, 0);
        
        fb_draw_string(x + 10, y + 140, "Subnet Mask:", 0x00cccccc, 0);
        fb_draw_string(x + 120, y + 140, "255.255.255.0", 0x00ffffff, 0);
        
        fb_draw_string(x + 10, y + 165, "Gateway:", 0x00cccccc, 0);
        fb_draw_string(x + 120, y + 165, "192.168.1.1", 0x00ffffff, 0);
        
        fb_draw_string(x + 10, y + 190, "MAC Address:", 0x00cccccc, 0);
        fb_draw_string(x + 120, y + 190, "52:54:00:12:34:56", 0x00ffffff, 0);
        
        // Connection status
        fb_draw_string(x + w/2 + 10, y + 15, "Connection Status", 0x00ffffff, 0);
        fb_draw_line(x + w/2 + 10, y + 32, x + w - 20, y + 32, 0x00555555);
        
        fb_fill_rect_rounded(x + w/2 + 10, y + 45, 12, 12, 3, 0x0000aa00);
        fb_draw_string(x + w/2 + 30, y + 45, "Connected", 0x0000ff00, 0);
        
        fb_draw_string(x + w/2 + 10, y + 70, "Interface: eth0", 0x00aaaaaa, 0);
        fb_draw_string(x + w/2 + 10, y + 90, "Speed: 1000 Mbps", 0x00aaaaaa, 0);
        fb_draw_string(x + w/2 + 10, y + 110, "Packets: 1,234,567", 0x00aaaaaa, 0);
        
        draw_button(x + w/2 + 10, y + 145, 100, 28, "Renew IP", 0x000055aa);
        draw_button(x + w/2 + 10, y + 180, 100, 28, "Release", 0x00aa5500);
        
    } else if (selected_tab == 3) {
        // Security settings
        fb_draw_string(x + 10, y + 15, "Security Features", 0x00ffffff, 0);
        fb_draw_line(x + 10, y + 32, x + 200, y + 32, 0x00555555);
        
        draw_checkbox(x + 10, y + 45, firewall_enabled, "Firewall Enabled");
        draw_checkbox(x + 10, y + 75, encryption_enabled, "Disk Encryption");
        draw_checkbox(x + 10, y + 105, secure_boot, "Secure Boot");
        draw_checkbox(x + 10, y + 135, 1, "Automatic Updates");
        draw_checkbox(x + 10, y + 165, 0, "Guest Account");
        
        // Security status
        fb_draw_string(x + w/2 + 10, y + 15, "Security Status", 0x00ffffff, 0);
        fb_draw_line(x + w/2 + 10, y + 32, x + w - 20, y + 32, 0x00555555);
        
        fb_draw_string(x + w/2 + 10, y + 45, "Overall: Secure", 0x0000ff00, 0);
        fb_draw_string(x + w/2 + 10, y + 70, "Last Scan: Today", 0x00aaaaaa, 0);
        fb_draw_string(x + w/2 + 10, y + 90, "Threats: 0", 0x0000aa00, 0);
        
        draw_button(x + w/2 + 10, y + 125, 120, 32, "Run Security Scan", 0x000055aa);
        draw_button(x + w/2 + 10, y + 165, 120, 32, "View Logs", 0x00555555);
        
    } else if (selected_tab == 4) {
        // System settings
        fb_draw_string(x + 10, y + 15, "System Information", 0x00ffffff, 0);
        fb_draw_line(x + 10, y + 32, x + 200, y + 32, 0x00555555);
        
        fb_draw_string(x + 10, y + 45, "OS: DevilCore v0.1", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 70, "Kernel: x86_64", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 95, "Architecture: 64-bit", 0x00aaaaaa, 0);
        fb_draw_string(x + 10, y + 120, "Bootloader: Limine", 0x00aaaaaa, 0);
        
        // Memory info
        uint32_t total_pages = pmm_total_pages();
        uint32_t free_pages = pmm_free_pages();
        uint32_t used_pages = total_pages - free_pages;
        
        char mem_info[64];
        sprintf(mem_info, "Memory: %u MB / %u MB", (used_pages * 4) / 1024, (total_pages * 4) / 1024);
        fb_draw_string(x + 10, y + 155, mem_info, 0x00aaaaaa, 0);
        
        // Uptime
        uint64_t uptime_sec = timer_ticks() / 100;
        uint32_t sec = (uint32_t)(uptime_sec % 60);
        uint32_t min = (uint32_t)((uptime_sec / 60) % 60);
        uint32_t hr = (uint32_t)(uptime_sec / 3600);
        char uptime[32];
        sprintf(uptime, "Uptime: %u:%02u:%02u", hr, min, sec);
        fb_draw_string(x + 10, y + 180, uptime, 0x00aaaaaa, 0);
        
        // Power options
        fb_draw_string(x + w/2 + 10, y + 15, "Power Options", 0x00ffffff, 0);
        fb_draw_line(x + w/2 + 10, y + 32, x + w - 20, y + 32, 0x00555555);
        
        draw_checkbox(x + w/2 + 10, y + 45, 1, "Sleep after 30 min");
        draw_checkbox(x + w/2 + 10, y + 75, 0, "Hibernate");
        
        draw_button(x + w/2 + 10, y + 115, 80, 32, "Sleep", 0x000055aa);
        draw_button(x + w/2 + 10, y + 155, 80, 32, "Restart", 0x00aa5500);
        draw_button(x + w/2 + 10, y + 195, 80, 32, "Shutdown", 0x00aa0000);
        
    } else if (selected_tab == 5) {
        // About
        uint32_t center_x = x + w/2;
        
        // Logo area (centered)
        fb_fill_rect_rounded(center_x - 40, y + 20, 80, 60, 8, 0x001a1a1a);
        fb_draw_rect_rounded(center_x - 40, y + 20, 80, 60, 8, 0x00555555);
        fb_draw_string(center_x - 20, y + 45, "DC", 0x00ffffff, 0);
        
        fb_draw_string(center_x - 70, y + 90, "DevilCore OS v0.1", 0x00ffffff, 0);
        fb_draw_string(center_x - 85, y + 110, "Ethical Hacking Environment", 0x00aaaaaa, 0);
        
        fb_draw_line(x + 20, y + 135, x + w - 20, y + 135, 0x00555555);
        
        fb_draw_string(x + 30, y + 155, "Kernel:", 0x00888888, 0);
        fb_draw_string(x + 120, y + 155, "Monolithic x86_64", 0x00cccccc, 0);
        
        fb_draw_string(x + 30, y + 175, "Compiler:", 0x00888888, 0);
        fb_draw_string(x + 120, y + 175, "GCC x86_64-elf", 0x00cccccc, 0);
        
        fb_draw_string(x + 30, y + 195, "Bootloader:", 0x00888888, 0);
        fb_draw_string(x + 120, y + 195, "Limine v7.x", 0x00cccccc, 0);
        
        fb_draw_string(x + 30, y + 215, "Build Date:", 0x00888888, 0);
        fb_draw_string(x + 120, y + 215, "May 2026", 0x00cccccc, 0);
        
        fb_draw_string(x + w/2 + 30, y + 155, "GUI:", 0x00888888, 0);
        fb_draw_string(x + w/2 + 100, y + 155, "Custom FB-Based", 0x00cccccc, 0);
        
        fb_draw_string(x + w/2 + 30, y + 175, "Network:", 0x00888888, 0);
        fb_draw_string(x + w/2 + 100, y + 175, "RTL8139 Driver", 0x00cccccc, 0);
        
        fb_draw_string(x + w/2 + 30, y + 195, "Filesystem:", 0x00888888, 0);
        fb_draw_string(x + w/2 + 100, y + 195, "Custom FAT32", 0x00cccccc, 0);
        
        fb_draw_string(x + 30, y + 245, "Licensed under MIT License", 0x00888888, 0);
        fb_draw_string(x + 30, y + 265, "(C) 2026 DevilCore Team", 0x00888888, 0);
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
        } else if (key >= '1' && key <= '6') {
            selected_tab = key - '1';
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            uint32_t w = widget->width;
            uint32_t h = widget->height;
            uint32_t tab_w = w / 6;
            
            // Check tab clicks (top 30 pixels)
            if (event->y >= 0 && event->y < 30) {
                for (int i = 0; i < 6; ++i) {
                    if (event->x >= (int32_t)(i * tab_w) && 
                        event->x < (int32_t)((i + 1) * tab_w)) {
                        selected_tab = (uint8_t)i;
                        return;
                    }
                }
            }
            
            uint32_t content_y = 30;
            
            // Tab-specific interactions
            if (selected_tab == 0) { // General
                // Theme buttons (left column)
                for (uint32_t i = 0; i < NUM_THEMES && i < 4; ++i) {
                    uint32_t btn_y = content_y + 60 + i * 28;
                    if (event->x >= 10 && event->x < 150 &&
                        event->y >= (int32_t)btn_y && event->y < (int32_t)(btn_y + 24)) {
                        theme_idx = (uint8_t)i;
                        return;
                    }
                }
                
                // Wallpaper buttons
                for (uint32_t i = 0; i < NUM_WALLPAPERS && i < 3; ++i) {
                    uint32_t btn_y = content_y + 195 + i * 28;
                    if (event->x >= 10 && event->x < 150 &&
                        event->y >= (int32_t)btn_y && event->y < (int32_t)(btn_y + 24)) {
                        wallpaper_idx = (uint8_t)i;
                        return;
                    }
                }
                
                // Checkboxes (right column starting at w/2 + 10)
                uint32_t col_x = w / 2 + 10;
                
                // Animations checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 45) && event->y < (int32_t)(content_y + 65)) {
                    animations_enabled = !animations_enabled;
                    return;
                }
                // Sound checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 75) && event->y < (int32_t)(content_y + 95)) {
                    sound_enabled = !sound_enabled;
                    return;
                }
                // Notifications checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 105) && event->y < (int32_t)(content_y + 125)) {
                    notifications_enabled = !notifications_enabled;
                    return;
                }
                // Auto-save checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 135) && event->y < (int32_t)(content_y + 155)) {
                    auto_save = !auto_save;
                    return;
                }
                // Dark mode checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 165) && event->y < (int32_t)(content_y + 185)) {
                    dark_mode = !dark_mode;
                    return;
                }
                // High contrast checkbox
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 150) &&
                    event->y >= (int32_t)(content_y + 195) && event->y < (int32_t)(content_y + 215)) {
                    high_contrast = !high_contrast;
                    return;
                }
                
                // Apply button
                if (event->x >= 10 && event->x < 130 &&
                    event->y >= (int32_t)(content_y + h - 30 - 40) && 
                    event->y < (int32_t)(content_y + h - 30 - 8)) {
                    struct wm_theme new_theme = current_theme;
                    new_theme.bg_color = theme_colors[theme_idx][0];
                    new_theme.fg_color = theme_colors[theme_idx][1];
                    new_theme.accent_color = theme_colors[theme_idx][2];
                    wm_set_theme(new_theme);
                    return;
                }
                
                // Reset button
                if (event->x >= 140 && event->x < 220 &&
                    event->y >= (int32_t)(content_y + h - 30 - 40) && 
                    event->y < (int32_t)(content_y + h - 30 - 8)) {
                    theme_idx = 0;
                    wallpaper_idx = 0;
                    animations_enabled = 1;
                    sound_enabled = 1;
                    notifications_enabled = 1;
                    dark_mode = 1;
                    return;
                }
                
            } else if (selected_tab == 1) { // Display
                // Resolution buttons
                for (uint32_t i = 0; i < NUM_RESOLUTIONS; ++i) {
                    uint32_t btn_y = content_y + 40 + i * 32;
                    if (event->x >= 10 && event->x < 190 &&
                        event->y >= (int32_t)btn_y && event->y < (int32_t)(btn_y + 28)) {
                        resolution_idx = (uint8_t)i;
                        return;
                    }
                }
                
                // Font size buttons
                for (uint32_t i = 0; i < 3; ++i) {
                    uint32_t btn_x = 220 + i * 70;
                    if (event->x >= (int32_t)btn_x && event->x < (int32_t)(btn_x + 65) &&
                        event->y >= (int32_t)(content_y + 135) && event->y < (int32_t)(content_y + 163)) {
                        font_size = (uint8_t)i;
                        return;
                    }
                }
                
            } else if (selected_tab == 2) { // Network
                // DHCP checkbox
                if (event->x >= 10 && event->x < 180 &&
                    event->y >= (int32_t)(content_y + 45) && event->y < (int32_t)(content_y + 65)) {
                    dhcp_enabled = !dhcp_enabled;
                    return;
                }
                
                // IPv6 checkbox
                if (event->x >= 10 && event->x < 180 &&
                    event->y >= (int32_t)(content_y + 75) && event->y < (int32_t)(content_y + 95)) {
                    // Toggle IPv6
                    return;
                }
                
                // Buttons on right side
                uint32_t col_x = w / 2 + 10;
                
                // Renew IP button
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 100) &&
                    event->y >= (int32_t)(content_y + 145) && event->y < (int32_t)(content_y + 173)) {
                    // Renew IP action
                    return;
                }
                
                // Release button
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 100) &&
                    event->y >= (int32_t)(content_y + 180) && event->y < (int32_t)(content_y + 208)) {
                    // Release IP action
                    return;
                }
                
            } else if (selected_tab == 3) { // Security
                // Checkboxes
                if (event->x >= 10 && event->x < 180) {
                    if (event->y >= (int32_t)(content_y + 45) && event->y < (int32_t)(content_y + 65)) {
                        firewall_enabled = !firewall_enabled;
                        return;
                    }
                    if (event->y >= (int32_t)(content_y + 75) && event->y < (int32_t)(content_y + 95)) {
                        encryption_enabled = !encryption_enabled;
                        return;
                    }
                    if (event->y >= (int32_t)(content_y + 105) && event->y < (int32_t)(content_y + 125)) {
                        secure_boot = !secure_boot;
                        return;
                    }
                }
                
                // Scan button
                uint32_t col_x = w / 2 + 10;
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 120) &&
                    event->y >= (int32_t)(content_y + 125) && event->y < (int32_t)(content_y + 157)) {
                    // Run security scan
                    return;
                }
                
            } else if (selected_tab == 4) { // System
                uint32_t col_x = w / 2 + 10;
                
                // Sleep button
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 80) &&
                    event->y >= (int32_t)(content_y + 115) && event->y < (int32_t)(content_y + 147)) {
                    // Sleep action
                    return;
                }
                
                // Restart button
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 80) &&
                    event->y >= (int32_t)(content_y + 155) && event->y < (int32_t)(content_y + 187)) {
                    // Restart action
                    return;
                }
                
                // Shutdown button
                if (event->x >= (int32_t)col_x && event->x < (int32_t)(col_x + 80) &&
                    event->y >= (int32_t)(content_y + 195) && event->y < (int32_t)(content_y + 227)) {
                    // Shutdown action
                    return;
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
    
    settings_window = wm_create_window("Settings", 150, 80, 600, 450);
    if (settings_window != NULL) {
        settings_window->draw_content = (void (*)(struct wm_window *))settings_draw;
        settings_window->widget.on_event = settings_on_event;
        settings_window->bg_color = 0x002a2a2a;
    }
}
