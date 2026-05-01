#include "calculator.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include <string.h>
#include <stdio.h>

static struct wm_window *calc_window = NULL;
static char display[64] = "0";
static int64_t accumulator = 0;
static int64_t current_value = 0;
static char operation = 0;
static uint8_t new_input = 1;
static uint8_t has_decimal = 0;
static int32_t decimal_places = 0;

static void int_to_str(int64_t val, char *buf, uint8_t show_decimal) {
    (void)show_decimal;
    if (val == 0) {
        strcpy(buf, "0");
        return;
    }
    
    int64_t temp = val < 0 ? -val : val;
    char rev[64];
    int idx = 0;
    
    while (temp > 0) {
        rev[idx++] = '0' + (temp % 10);
        temp /= 10;
    }
    
    int start = 0;
    if (val < 0) {
        buf[0] = '-';
        start = 1;
    }
    
    for (int i = 0; i < idx; i++) {
        buf[start + i] = rev[idx - 1 - i];
    }
    buf[start + idx] = '\0';
}

static void draw_button(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const char *label, uint32_t color) {
    fb_fill_rect_rounded(x, y, w, h, 4, color);
    fb_draw_string(x + (w - strlen(label) * 8) / 2, y + (h - 8) / 2 + 2, label, 0x00ffffff, 0);
}

static void calc_draw_content(struct wm_window *win) {
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Display area
    fb_fill_rect(x, y, w, 50, 0x001a1a1a);
    fb_draw_rect_outline(x, y, w, 50, 0x00505050);
    fb_draw_string(x + 10, y + 20, display, 0x0000ff00, 0);
    
    // Button area
    y += 50;
    uint32_t btn_w = (w - 20) / 4;
    uint32_t btn_h = (h - 60) / 5;
    uint32_t margin = 4;
    
    // Row 1: C, CE, %, ÷
    uint32_t row_y = y + margin;
    draw_button(x + margin, row_y, btn_w - margin, btn_h - margin, "C", 0x00888888);
    draw_button(x + btn_w + margin, row_y, btn_w - margin*2, btn_h - margin, "CE", 0x00888888);
    draw_button(x + btn_w*2 + margin, row_y, btn_w - margin*2, btn_h - margin, "%", 0x00888888);
    draw_button(x + btn_w*3, row_y, btn_w - margin, btn_h - margin, "/", 0x00ff9500);
    
    // Row 2: 7, 8, 9, ×
    row_y += btn_h;
    draw_button(x + margin, row_y, btn_w - margin, btn_h - margin, "7", 0x00444444);
    draw_button(x + btn_w + margin, row_y, btn_w - margin*2, btn_h - margin, "8", 0x00444444);
    draw_button(x + btn_w*2 + margin, row_y, btn_w - margin*2, btn_h - margin, "9", 0x00444444);
    draw_button(x + btn_w*3, row_y, btn_w - margin, btn_h - margin, "X", 0x00ff9500);
    
    // Row 3: 4, 5, 6, -
    row_y += btn_h;
    draw_button(x + margin, row_y, btn_w - margin, btn_h - margin, "4", 0x00444444);
    draw_button(x + btn_w + margin, row_y, btn_w - margin*2, btn_h - margin, "5", 0x00444444);
    draw_button(x + btn_w*2 + margin, row_y, btn_w - margin*2, btn_h - margin, "6", 0x00444444);
    draw_button(x + btn_w*3, row_y, btn_w - margin, btn_h - margin, "-", 0x00ff9500);
    
    // Row 4: 1, 2, 3, +
    row_y += btn_h;
    draw_button(x + margin, row_y, btn_w - margin, btn_h - margin, "1", 0x00444444);
    draw_button(x + btn_w + margin, row_y, btn_w - margin*2, btn_h - margin, "2", 0x00444444);
    draw_button(x + btn_w*2 + margin, row_y, btn_w - margin*2, btn_h - margin, "3", 0x00444444);
    draw_button(x + btn_w*3, row_y, btn_w - margin, btn_h - margin, "+", 0x00ff9500);
    
    // Row 5: 0, ., =
    row_y += btn_h;
    draw_button(x + margin, row_y, btn_w*2 - margin, btn_h - margin, "0", 0x00444444);
    draw_button(x + btn_w*2 + margin, row_y, btn_w - margin*2, btn_h - margin, ".", 0x00444444);
    draw_button(x + btn_w*3, row_y, btn_w - margin, btn_h - margin, "=", 0x00ff9500);
}

static void handle_button(const char *btn) {
    if (strcmp(btn, "C") == 0) {
        strcpy(display, "0");
        accumulator = 0;
        current_value = 0;
        operation = 0;
        new_input = 1;
        has_decimal = 0;
        decimal_places = 0;
    } else if (strcmp(btn, "CE") == 0) {
        strcpy(display, "0");
        current_value = 0;
        new_input = 1;
        has_decimal = 0;
        decimal_places = 0;
    } else if (btn[0] >= '0' && btn[0] <= '9') {
        if (new_input || strcmp(display, "0") == 0) {
            strcpy(display, btn);
            current_value = btn[0] - '0';
            new_input = 0;
        } else {
            strcat(display, btn);
            current_value = current_value * 10 + (btn[0] - '0');
        }
    } else if (strcmp(btn, ".") == 0) {
        if (!has_decimal) {
            strcat(display, ".");
            has_decimal = 1;
            decimal_places = 0;
        }
    } else if (strcmp(btn, "+") == 0 || strcmp(btn, "-") == 0 || 
               strcmp(btn, "X") == 0 || strcmp(btn, "/") == 0) {
        accumulator = current_value;
        operation = btn[0];
        new_input = 1;
        has_decimal = 0;
        decimal_places = 0;
    } else if (strcmp(btn, "=") == 0) {
        if (operation) {
            int64_t result = 0;
            
            switch (operation) {
                case '+': result = accumulator + current_value; break;
                case '-': result = accumulator - current_value; break;
                case 'X': result = accumulator * current_value; break;
                case '/': result = current_value != 0 ? accumulator / current_value : 0; break;
            }
            
            current_value = result;
            int_to_str(result, display, 0);
            operation = 0;
            new_input = 1;
        }
    }
}

static void calc_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    if (win == NULL) return;
    
    // Let the default window handler check for title bar buttons first
    if (wm_window_on_event(widget, event)) {
        return; // Event was handled by title bar button
    }
    
    if (event->type == WM_EVENT_KEY) {
        uint8_t key = event->key;
        if (key == 'q' || key == 27) {
            wm_destroy_window(win);
            calc_window = NULL;
        } else if (key >= '0' && key <= '9') {
            char buf[2] = {key, '\0'};
            handle_button(buf);
        } else if (key == '+' || key == '-' || key == '*' || key == '/') {
            char buf[2] = {key == '*' ? 'X' : key, '\0'};
            handle_button(buf);
        } else if (key == '=' || key == '\n') {
            handle_button("=");
        } else if (key == 'c' || key == 'C') {
            handle_button("C");
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            // Check if click is in button area
            if (event->y >= 50) {
                uint32_t w = widget->width;
                uint32_t h = widget->height;
                uint32_t btn_w = (w - 20) / 4;
                uint32_t btn_h = (h - 60) / 5;
                
                int col = event->x / btn_w;
                int row = (event->y - 50) / btn_h;
                
                if (col >= 0 && col < 4 && row >= 0 && row < 5) {
                    const char *buttons[5][4] = {
                        {"C", "CE", "%", "/"},
                        {"7", "8", "9", "X"},
                        {"4", "5", "6", "-"},
                        {"1", "2", "3", "+"},
                        {"0", "0", ".", "="}
                    };
                    handle_button(buttons[row][col]);
                }
            }
        }
    }
}

void calc_open(void) {
    if (calc_window != NULL) {
        wm_focus_window(calc_window);
        return;
    }
    
    strcpy(display, "0");
    current_value = 0;
    calc_window = wm_create_window("Calculator", 300, 150, 320, 420);
    if (calc_window != NULL) {
        calc_window->draw_content = calc_draw_content;
        calc_window->widget.on_event = calc_event;
        calc_window->bg_color = 0x002a2a2a;
    }
}
