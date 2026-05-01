#include "calendar.h"
#include "wm.h"
#include "framebuffer.h"
#include "timer.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

struct calendar_data {
    uint32_t year;
    uint32_t month;
};

static struct wm_window *calendar_window = NULL;

static const char *month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static int get_days_in_month(int month, int year) {
    if (month == 1) { // February
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
        return 28;
    }
    if (month == 3 || month == 5 || month == 8 || month == 10) return 30;
    return 31;
}

static void calendar_draw_content(struct wm_window *win) {
    struct calendar_data *data = (struct calendar_data *)win->widget.data;
    if (!data) return;

    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;

    // Header (Month and Year)
    fb_fill_rect(x, y, w, 40, 0x002a2a2a);
    char title[64];
    sprintf(title, "%s %u", month_names[data->month], data->year);
    fb_draw_string(x + (w - strlen(title) * 8) / 2, y + 12, title, 0x00ffffff, 0);

    // Day labels
    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    uint32_t cell_w = w / 7;
    uint32_t cell_h = (h - 40) / 7;
    
    for (int i = 0; i < 7; i++) {
        fb_draw_string(x + i * cell_w + (cell_w - 16) / 2, y + 50, days[i], 0x00aaaaaa, 0);
    }

    // Grid
    uint32_t start_day = 5; // May 2026 starts on Friday (5)
    uint32_t days_in_month = get_days_in_month(data->month, data->year);
    
    for (uint32_t i = 0; i < days_in_month; i++) {
        uint32_t day = i + 1;
        uint32_t pos = i + start_day;
        uint32_t row = pos / 7;
        uint32_t col = pos % 7;
        
        uint32_t dx = x + col * cell_w;
        uint32_t dy = y + 80 + row * cell_h;
        
        char day_str[4];
        sprintf(day_str, "%u", day);
        
        // Highlight today (May 2nd 2026)
        if (day == 2) {
            fb_fill_rect_rounded(dx + 4, dy - 4, cell_w - 8, cell_h - 4, 4, current_theme.accent_color);
            fb_draw_string(dx + (cell_w - strlen(day_str) * 8) / 2, dy, day_str, 0x00ffffff, 0);
        } else {
            fb_draw_string(dx + (cell_w - strlen(day_str) * 8) / 2, dy, day_str, 0x00dddddd, 0);
        }
    }
}

void calendar_open(void) {
    if (calendar_window != NULL) {
        wm_focus_window(calendar_window);
        return;
    }

    struct calendar_data *data = kmalloc(sizeof(struct calendar_data));
    data->year = 2026;
    data->month = 4; // May (0-indexed)

    calendar_window = wm_create_window("Calendar", 150, 150, 300, 300);
    if (calendar_window) {
        calendar_window->widget.data = data;
        calendar_window->draw_content = calendar_draw_content;
        calendar_window->bg_color = 0x001a1a1a;
        wm_notify("Calendar opened", 0x0044ffcc);
    }
}
