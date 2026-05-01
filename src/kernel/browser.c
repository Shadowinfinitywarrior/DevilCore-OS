// DevilCore Privacy Browser - Secure web browsing with privacy features
// Modern GUI implementation with improved visual design

#include <stdint.h>
#include <string.h>
#include "framebuffer.h"
#include "wm.h"
#include "memory.h"
#include "kprintf.h"

extern struct wm_desktop *desktop;

#define BROWSER_MAX_URL 256
#define BROWSER_MAX_HISTORY 20
#define BROWSER_MAX_TABS 8

struct browser_tab {
    char url[BROWSER_MAX_URL];
    char title[64];
    uint8_t loading;
};

struct browser_data {
    struct browser_tab tabs[BROWSER_MAX_TABS];
    uint32_t active_tab;
    uint32_t tab_count;
    char content[50][100];
    uint32_t content_lines;
    uint8_t private_mode;
    uint8_t https_only;
    uint8_t ad_blocker;
    uint32_t history_count;
    char history[BROWSER_MAX_HISTORY][BROWSER_MAX_URL];
    uint32_t scroll_offset;
    uint8_t loading_anim;
    uint32_t loading_frame;
};

static struct wm_window *browser_window = NULL;

// Modern color scheme
#define BROWSER_BG 0x00f5f5f5
#define BROWSER_HEADER_BG 0x00e8e8e8
#define BROWSER_TAB_ACTIVE 0x00ffffff
#define BROWSER_TAB_INACTIVE 0x00d0d0d0
#define BROWSER_TAB_HOVER 0x00e0e0e0
#define BROWSER_ACCENT 0x006699ff
#define BROWSER_TEXT 0x00222222
#define BROWSER_TEXT_LIGHT 0x00666666
#define BROWSER_BORDER 0x00cccccc
#define BROWSER_SHADOW 0x00aaaaaa

// Simulated secure pages
static const char *secure_pages[][3] = {
    {"devilcore://welcome", "Welcome to DevilCore Privacy Browser", "Secure browsing environment"},
    {"devilcore://privacy", "Privacy Settings", "Your privacy is protected"},
    {"devilcore://secure", "Secure Connection", "HTTPS encryption active"},
    {"https://example.com", "Example Domain", "This is a secure example page"},
    {"https://duckduckgo.com", "DuckDuckGo", "Privacy-focused search engine"},
    {"https://startpage.com", "Startpage", "Google results without tracking"},
};

void browser_init_data(struct browser_data *data) {
    memset(data, 0, sizeof(struct browser_data));
    data->tab_count = 1;
    data->active_tab = 0;
    data->private_mode = 1;
    data->https_only = 1;
    data->ad_blocker = 1;
    data->scroll_offset = 0;
    data->loading_anim = 0;
    data->loading_frame = 0;
    
    strcpy(data->tabs[0].url, "devilcore://welcome");
    strcpy(data->tabs[0].title, "Welcome");
    data->tabs[0].loading = 0;
    
    strcpy(data->content[0], "=== DevilCore Privacy Browser ===");
    strcpy(data->content[1], "");
    strcpy(data->content[2], "Features:");
    strcpy(data->content[3], "  • Private browsing by default");
    strcpy(data->content[4], "  • HTTPS-only mode");
    strcpy(data->content[5], "  • Ad blocker enabled");
    strcpy(data->content[6], "  • No tracking or cookies");
    strcpy(data->content[7], "");
    strcpy(data->content[8], "Keyboard shortcuts:");
    strcpy(data->content[9], "  Ctrl+L - Focus address bar");
    strcpy(data->content[10], "  Ctrl+T - New tab");
    strcpy(data->content[11], "  Ctrl+W - Close tab");
    strcpy(data->content[12], "  Ctrl+R - Reload page");
    strcpy(data->content[13], "");
    strcpy(data->content[14], "Type a URL to navigate.");
    data->content_lines = 15;
}

void browser_draw_tabs(struct wm_window *win, struct browser_data *data) {
    int32_t x = win->widget.x + 5;
    int32_t y = win->widget.y + 30;
    uint32_t tab_width = 140;
    uint32_t tab_height = 32;
    
    for (uint32_t i = 0; i < data->tab_count; i++) {
        uint32_t tab_x = x + i * (tab_width + 5);
        
        // Tab background
        uint32_t bg_color = (i == data->active_tab) ? BROWSER_TAB_ACTIVE : BROWSER_TAB_INACTIVE;
        fb_fill_rect_rounded(tab_x, y, tab_width, tab_height, 6, bg_color);
        
        // Tab border
        fb_draw_rect_rounded(tab_x, y, tab_width, tab_height, 6, BROWSER_BORDER);
        
        // Tab title
        fb_draw_string(tab_x + 10, y + 10, data->tabs[i].title, BROWSER_TEXT, 0);
        
        // Close button (X)
        fb_fill_rect_rounded(tab_x + tab_width - 22, y + 8, 16, 16, 3, 0x00ffcccc);
        fb_draw_string(tab_x + tab_width - 18, y + 10, "×", 0x00cc0000, 0);
        
        // Loading indicator
        if (data->tabs[i].loading) {
            fb_fill_rect_rounded(tab_x + tab_width - 40, y + 10, 8, 8, 2, BROWSER_ACCENT);
        }
    }
    
    // New tab button
    uint32_t new_tab_x = x + data->tab_count * (tab_width + 5);
    fb_fill_rect_rounded(new_tab_x, y, 32, tab_height, 6, BROWSER_TAB_INACTIVE);
    fb_draw_rect_rounded(new_tab_x, y, 32, tab_height, 6, BROWSER_BORDER);
    fb_draw_string(new_tab_x + 10, y + 10, "+", BROWSER_TEXT, 0);
}

void browser_draw_address_bar(struct wm_window *win, struct browser_data *data) {
    int32_t x = win->widget.x + 10;
    int32_t y = win->widget.y + 68;
    uint32_t w = win->widget.width - 20;
    uint32_t h = 36;
    
    // Shadow
    fb_fill_rect_rounded(x + 2, y + 2, w, h, 8, 0x00dddddd);
    
    // Address bar background with gradient effect
    fb_fill_rect_rounded(x, y, w, h, 8, 0x00ffffff);
    fb_draw_rect_rounded(x, y, w, h, 8, BROWSER_BORDER);
    
    // Security lock icon
    uint32_t sec_color = data->https_only ? 0x0000cc00 : 0x00cc0000;
    fb_fill_rect_rounded(x + 8, y + 8, 20, 20, 4, sec_color);
    fb_draw_string(x + 13, y + 13, "🔒", 0x00ffffff, 0);
    
    // URL text
    fb_draw_string(x + 36, y + 12, data->tabs[data->active_tab].url, BROWSER_TEXT, 0);
    
    // Privacy badge
    if (data->private_mode) {
        fb_fill_rect_rounded(x + w - 90, y + 8, 80, 20, 4, 0x00660066);
        fb_draw_string(x + w - 80, y + 10, "🔒 Private", 0x00ffffff, 0);
    }
    
    // Reload button
    fb_fill_rect_rounded(x + w - 110, y + 8, 16, 20, 4, 0x00e0e0e0);
    fb_draw_string(x + w - 106, y + 12, "↻", BROWSER_TEXT, 0);
}

void browser_draw_toolbar(struct wm_window *win, struct browser_data *data) {
    (void)data;
    int32_t x = win->widget.x + 10;
    int32_t y = win->widget.y + 112;
    
    // Navigation buttons with modern styling
    uint32_t btn_w = 36;
    uint32_t btn_h = 32;
    uint32_t btn_spacing = 8;
    
    // Back button
    fb_fill_rect_rounded(x, y, btn_w, btn_h, 6, 0x00f0f0f0);
    fb_draw_rect_rounded(x, y, btn_w, btn_h, 6, BROWSER_BORDER);
    fb_draw_string(x + 14, y + 10, "◀", BROWSER_TEXT, 0);
    
    // Forward button
    fb_fill_rect_rounded(x + btn_w + btn_spacing, y, btn_w, btn_h, 6, 0x00f0f0f0);
    fb_draw_rect_rounded(x + btn_w + btn_spacing, y, btn_w, btn_h, 6, BROWSER_BORDER);
    fb_draw_string(x + btn_w + btn_spacing + 14, y + 10, "▶", BROWSER_TEXT, 0);
    
    // Home button
    fb_fill_rect_rounded(x + (btn_w + btn_spacing) * 2, y, btn_w, btn_h, 6, 0x00f0f0f0);
    fb_draw_rect_rounded(x + (btn_w + btn_spacing) * 2, y, btn_w, btn_h, 6, BROWSER_BORDER);
    fb_draw_string(x + (btn_w + btn_spacing) * 2 + 12, y + 10, "⌂", BROWSER_TEXT, 0);
    
    // Menu button (right side)
    fb_fill_rect_rounded(win->widget.x + win->widget.width - 50, y, 40, btn_h, 6, 0x00f0f0f0);
    fb_draw_rect_rounded(win->widget.x + win->widget.width - 50, y, 40, btn_h, 6, BROWSER_BORDER);
    fb_draw_string(win->widget.x + win->widget.width - 38, y + 10, "≡", BROWSER_TEXT, 0);
}

void browser_draw_content(struct wm_window *win, struct browser_data *data) {
    int32_t x = win->widget.x + 10;
    int32_t y = win->widget.y + 152;
    uint32_t w = win->widget.width - 20;
    uint32_t h = win->widget.height - 170;
    
    // Content area background with subtle gradient
    fb_fill_rect(x, y, w, h, BROWSER_BG);
    
    // Content border
    fb_draw_rect_outline(x, y, w, h, BROWSER_BORDER);
    
    // Draw page header
    fb_fill_rect(x, y, w, 50, 0x00f8f8f8);
    fb_draw_line(x, y + 50, x + w, y + 50, BROWSER_BORDER);
    
    // Page title with bold styling
    fb_draw_string(x + 15, y + 15, data->tabs[data->active_tab].title, BROWSER_ACCENT, 1);
    
    // Loading animation
    if (data->loading_anim) {
        char loading_text[32];
        const char *dots = "   ";
        for (uint32_t i = 0; i < (data->loading_frame % 4); i++) {
            ((char*)dots)[i] = '.';
        }
        sprintf(loading_text, "Loading%s", dots);
        fb_draw_string(x + w - 100, y + 15, loading_text, BROWSER_TEXT_LIGHT, 0);
    }
    
    // Draw content lines with better spacing
    int32_t line_y = y + 65;
    for (uint32_t i = data->scroll_offset; i < data->content_lines && line_y < (int32_t)(y + h - 20); i++) {
        // Highlight headers (lines starting with ===)
        if (strncmp(data->content[i], "===", 3) == 0) {
            fb_draw_string(x + 15, line_y, data->content[i], BROWSER_ACCENT, 1);
        } else if (data->content[i][0] == '-' || data->content[i][0] == '*') {
            // Bullet points
            fb_draw_string(x + 15, line_y, data->content[i], BROWSER_TEXT, 0);
        } else {
            fb_draw_string(x + 15, line_y, data->content[i], BROWSER_TEXT, 0);
        }
        line_y += 18;
    }
    
    // Modern scrollbar
    if (data->content_lines > 15) {
        uint32_t sb_width = 12;
        uint32_t sb_x = x + w - sb_width;
        uint32_t sb_height = h;
        uint32_t thumb_height = (sb_height * 15) / data->content_lines;
        if (thumb_height < 30) thumb_height = 30;
        uint32_t thumb_y = y + (data->scroll_offset * (sb_height - thumb_height)) / (data->content_lines - 15);
        
        // Scrollbar track
        fb_fill_rect(sb_x, y, sb_width, sb_height, 0x00e8e8e8);
        
        // Scrollbar thumb with rounded corners
        fb_fill_rect_rounded(sb_x + 2, thumb_y, sb_width - 4, thumb_height, 4, BROWSER_ACCENT);
    }
}

void browser_draw_status_bar(struct wm_window *win, struct browser_data *data) {
    int32_t x = win->widget.x + 10;
    int32_t y = win->widget.y + win->widget.height - 28;
    uint32_t w = win->widget.width - 20;
    uint32_t h = 24;
    
    // Status bar background
    fb_fill_rect(x, y, w, h, 0x00f0f0f0);
    fb_draw_line(x, y, x + w, y, BROWSER_BORDER);
    
    // Status text
    char status[64];
    if (data->loading_anim) {
        strcpy(status, "Loading page...");
    } else {
        sprintf(status, "Done | %u items loaded", data->content_lines);
    }
    fb_draw_string(x + 10, y + 6, status, BROWSER_TEXT_LIGHT, 0);
    
    // Privacy indicators
    if (data->ad_blocker) {
        fb_draw_string(x + w - 150, y + 6, "🛡️ Ads blocked", 0x0000aa00, 0);
    }
    if (data->https_only) {
        fb_draw_string(x + w - 70, y + 6, "🔒 Secure", 0x0000aa00, 0);
    }
}

void browser_draw(struct wm_widget *widget) {
    struct wm_window *win = (struct wm_window *)widget;
    struct browser_data *data = (struct browser_data *)win->widget.data;
    
    if (data == NULL) return;
    
    // Draw window chrome
    wm_draw_window(win);
    
    // Draw browser UI components
    browser_draw_tabs(win, data);
    browser_draw_address_bar(win, data);
    browser_draw_toolbar(win, data);
    browser_draw_content(win, data);
    browser_draw_status_bar(win, data);
    
    // Update loading animation
    if (data->loading_anim) {
        data->loading_frame++;
        if (data->loading_frame > 30) {
            data->loading_anim = 0;
            data->loading_frame = 0;
        }
    }
}

void browser_navigate(struct browser_data *data, const char *url) {
    if (strlen(url) >= BROWSER_MAX_URL) return;
    
    strcpy(data->tabs[data->active_tab].url, url);
    data->tabs[data->active_tab].loading = 1;
    data->loading_anim = 1;
    data->loading_frame = 0;
    
    // Add to history
    if (data->history_count < BROWSER_MAX_HISTORY) {
        strcpy(data->history[data->history_count], url);
        data->history_count++;
    }
    
    data->content_lines = 0;
    data->scroll_offset = 0;
    
    // Simulate page load
    uint8_t found = 0;
    for (uint32_t i = 0; i < sizeof(secure_pages) / sizeof(secure_pages[0]); i++) {
        if (strcmp(url, secure_pages[i][0]) == 0) {
            strcpy(data->tabs[data->active_tab].title, secure_pages[i][1]);
            strcpy(data->content[0], secure_pages[i][1]);
            strcpy(data->content[1], "");
            strcpy(data->content[2], secure_pages[i][2]);
            strcpy(data->content[3], "");
            strcpy(data->content[4], "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            strcpy(data->content[5], "Connection Status: ✓ Secure");
            strcpy(data->content[6], "Encryption: AES-256-GCM");
            strcpy(data->content[7], "Certificate: Valid (DevilCore CA)");
            strcpy(data->content[8], "");
            strcpy(data->content[9], "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            strcpy(data->content[10], "Privacy Protections Active:");
            strcpy(data->content[11], "  • Third-party cookies blocked");
            strcpy(data->content[12], "  • Tracking scripts blocked");
            strcpy(data->content[13], "  • Fingerprinting protection enabled");
            strcpy(data->content[14], "  • DNS over HTTPS active");
            strcpy(data->content[15], "");
            strcpy(data->content[16], "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            data->content_lines = 17;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        strcpy(data->tabs[data->active_tab].title, "Unknown Site");
        strcpy(data->content[0], "Unable to connect to:");
        strcpy(data->content[1], url);
        strcpy(data->content[2], "");
        strcpy(data->content[3], "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        strcpy(data->content[4], "This browser is in simulation mode.");
        strcpy(data->content[5], "Network stack not yet implemented.");
        strcpy(data->content[6], "");
        strcpy(data->content[7], "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        data->content_lines = 8;
    }
    
    data->tabs[data->active_tab].loading = 0;
}

void browser_on_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct browser_data *data = (struct browser_data *)win->widget.data;
    
    if (data == NULL) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_MOUSE_PRESS) {
        int32_t rel_y = event->y - win->widget.y;
        int32_t rel_x = event->x - win->widget.x;
        
        // Check toolbar buttons
         if (rel_y >= 112 && rel_y <= 144) {
             int32_t btn_x = 10;
             int32_t btn_w = 36;
             int32_t btn_spacing = 8;
            
            if ((int32_t)rel_x >= btn_x && (int32_t)rel_x <= btn_x + btn_w) {
                // Back
                if (data->history_count > 1) {
                    data->history_count--;
                    browser_navigate(data, data->history[data->history_count - 1]);
                }
            } else if ((int32_t)rel_x >= btn_x + btn_w + btn_spacing && (int32_t)rel_x <= btn_x + (btn_w + btn_spacing) * 2) {
                // Forward (placeholder)
            } else if ((int32_t)rel_x >= btn_x + (btn_w + btn_spacing) * 2 && (int32_t)rel_x <= btn_x + (btn_w + btn_spacing) * 3) {
                // Home
                browser_navigate(data, "devilcore://welcome");
            }
        }
        
         // Check tabs
         if (rel_y >= 30 && rel_y <= 62) {
             int32_t tab_width = 140;
             uint32_t tab_idx = (rel_x - 5) / (tab_width + 5);
            if (tab_idx < data->tab_count) {
                 // Check if close button clicked
                 int32_t close_x = 5 + tab_idx * (tab_width + 5) + tab_width - 22;
                 if ((int32_t)rel_x >= close_x && (int32_t)rel_x <= close_x + 16) {
                    // Close tab
                    if (data->tab_count > 1) {
                        for (uint32_t i = tab_idx; i < data->tab_count - 1; i++) {
                            data->tabs[i] = data->tabs[i + 1];
                        }
                        data->tab_count--;
                        if (data->active_tab >= data->tab_count) {
                            data->active_tab = data->tab_count - 1;
                        }
                    }
                } else {
                    // Switch tab
                    data->active_tab = tab_idx;
                }
             } else if (tab_idx == data->tab_count && (int32_t)rel_x <= 5 + (int32_t)(data->tab_count * (tab_width + 5)) + 32) {
                // New tab
                if (data->tab_count < BROWSER_MAX_TABS) {
                    strcpy(data->tabs[data->tab_count].url, "devilcore://welcome");
                    strcpy(data->tabs[data->tab_count].title, "New Tab");
                    data->tabs[data->tab_count].loading = 0;
                    data->tab_count++;
                    data->active_tab = data->tab_count - 1;
                    browser_navigate(data, "devilcore://welcome");
                }
            }
        }
        
        wm_refresh();
    } else if (event->type == WM_EVENT_KEY) {
        char c = event->key;
        uint8_t mods = event->modifiers;
        
        if ((mods & 8) && (c == 'l' || c == 'L')) {
            browser_navigate(data, "https://duckduckgo.com");
        } else if ((mods & 8) && (c == 'r' || c == 'R')) {
            browser_navigate(data, data->tabs[data->active_tab].url);
        } else if ((mods & 8) && (c == 't' || c == 'T')) {
            // New tab
            if (data->tab_count < BROWSER_MAX_TABS) {
                strcpy(data->tabs[data->tab_count].url, "devilcore://welcome");
                strcpy(data->tabs[data->tab_count].title, "New Tab");
                data->tabs[data->tab_count].loading = 0;
                data->tab_count++;
                data->active_tab = data->tab_count - 1;
            }
        } else if ((mods & 8) && (c == 'w' || c == 'W')) {
            // Close tab
            if (data->tab_count > 1) {
                for (uint32_t i = data->active_tab; i < data->tab_count - 1; i++) {
                    data->tabs[i] = data->tabs[i + 1];
                }
                data->tab_count--;
                if (data->active_tab >= data->tab_count) {
                    data->active_tab = data->tab_count - 1;
                }
            }
        } else if (c == 'w' || c == 'W') {
            if (data->scroll_offset > 0) data->scroll_offset--;
        } else if (c == 's' || c == 'S') {
            if (data->scroll_offset + 15 < data->content_lines) data->scroll_offset++;
        }
        
        desktop->needs_redraw = 1;
    }
}

void browser_open(void) {
    if (browser_window != NULL) {
        wm_focus_window(browser_window);
        return;
    }
    
    browser_window = wm_create_window("DevilCore Privacy Browser", 
                                      80, 40, 
                                      1000, 750);
    if (browser_window == NULL) return;
    
    struct browser_data *data = kmalloc(sizeof(struct browser_data));
    browser_init_data(data);
    browser_window->widget.data = data;
    browser_window->widget.draw = browser_draw;
    browser_window->widget.on_event = browser_on_event;
    
    wm_focus_window(browser_window);
}

void browser_close(void) {
    if (browser_window != NULL) {
        wm_destroy_window(browser_window);
        browser_window = NULL;
    }
}
