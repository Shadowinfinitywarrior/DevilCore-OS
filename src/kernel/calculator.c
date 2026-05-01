#include "calculator.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include <string.h>
#include <stdio.h>
#include "memory.h"

struct calc_data {
    char display[64];
    char expr_preview[128];
    char history[20][64];
    uint32_t history_count;
    int64_t accumulator;
    int64_t current_value;
    char operation;
    uint8_t new_input;
    uint8_t has_decimal;
    int32_t decimal_places;
    uint8_t scientific_mode;
};

static struct wm_window *calc_window = NULL;

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
    struct calc_data *data = (struct calc_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Check mode
    uint32_t right_panel_w = data->scientific_mode ? 120 : 0;
    uint32_t calc_w = w - right_panel_w;
    
    // Display area
    fb_fill_rect(x, y, calc_w, 60, 0x001a1a1a);
    fb_draw_rect_outline(x, y, calc_w, 60, 0x00505050);
    fb_draw_string(x + 10, y + 10, data->expr_preview, 0x00888888, 0); // Expression
    fb_draw_string_scaled(x + 10, y + 30, data->display, 0x0000ff00, 2); // Result
    
    // Button area
    y += 60;
    uint32_t cols = data->scientific_mode ? 6 : 4;
    uint32_t btn_w = (calc_w - 20) / cols;
    uint32_t btn_h = (h - 70) / 5;
    uint32_t margin = 4;
    
    // Button definitions (Scientific vs Standard)
    const char *btns_std[5][4] = {
        {"C", "CE", "%", "/"},
        {"7", "8", "9", "X"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"M", "0", ".", "="} // M toggles mode
    };
    
    const char *btns_sci[5][6] = {
        {"sin", "cos", "tan", "C", "CE", "/"},
        {"log", "ln",  "sqrt","7", "8", "9"},
        {"pi",  "e",   "^",   "4", "5", "6"},
        {"(",   ")",   "X",   "1", "2", "3"},
        {"M",   "0",   ".",   "-", "+", "="}
    };
    
    for (uint32_t r = 0; r < 5; r++) {
        for (uint32_t c = 0; c < cols; c++) {
            const char *label = data->scientific_mode ? btns_sci[r][c] : btns_std[r][c];
            uint32_t color = 0x00444444; // Default gray
            if (label[0] >= '0' && label[0] <= '9') color = 0x00333333; // Darker gray for numbers
            else if (strcmp(label, "=") == 0 || strcmp(label, "+") == 0 || strcmp(label, "-") == 0 || strcmp(label, "X") == 0 || strcmp(label, "/") == 0) color = 0x00ff9500; // Orange for ops
            else if (strcmp(label, "M") == 0) color = 0x00884488; // Mode toggle
            
            draw_button(x + margin + c * btn_w, y + margin + r * btn_h, btn_w - margin, btn_h - margin, label, color);
        }
    }
    
    // History Tape
    if (data->scientific_mode) {
        uint32_t hx = x + calc_w;
        uint32_t hy = win->widget.y;
        fb_fill_rect(hx, hy, right_panel_w, h, 0x00222222);
        fb_draw_rect_outline(hx, hy, right_panel_w, h, 0x00505050);
        fb_draw_string(hx + 10, hy + 10, "History", 0x00aaaaaa, 0);
        
        uint32_t text_y = hy + 30;
        for (int i = 0; i < data->history_count && i < 10; i++) {
            fb_draw_string(hx + 10, text_y, data->history[i], 0x00ffffff, 0);
            text_y += 16;
        }
    }
}

static void add_history(struct calc_data *data, const char *expr) {
    if (data->history_count < 20) {
        for (int i = data->history_count; i > 0; i--) {
            strcpy(data->history[i], data->history[i-1]);
        }
        strcpy(data->history[0], expr);
        data->history_count++;
    } else {
        for (int i = 19; i > 0; i--) {
            strcpy(data->history[i], data->history[i-1]);
        }
        strcpy(data->history[0], expr);
    }
}

static void handle_button(struct calc_data *data, const char *btn) {
    if (strcmp(btn, "M") == 0) {
        data->scientific_mode = !data->scientific_mode;
    } else if (strcmp(btn, "C") == 0) {
        strcpy(data->display, "0");
        data->expr_preview[0] = '\0';
        data->accumulator = 0;
        data->current_value = 0;
        data->operation = 0;
        data->new_input = 1;
        data->has_decimal = 0;
        data->decimal_places = 0;
    } else if (strcmp(btn, "CE") == 0) {
        strcpy(data->display, "0");
        data->current_value = 0;
        data->new_input = 1;
        data->has_decimal = 0;
        data->decimal_places = 0;
    } else if (btn[0] >= '0' && btn[0] <= '9') {
        if (data->new_input || strcmp(data->display, "0") == 0) {
            strcpy(data->display, btn);
            data->current_value = btn[0] - '0';
            data->new_input = 0;
        } else {
            strcat(data->display, btn);
            data->current_value = data->current_value * 10 + (btn[0] - '0');
        }
    } else if (strcmp(btn, ".") == 0) {
        if (!data->has_decimal) {
            strcat(data->display, ".");
            data->has_decimal = 1;
            data->decimal_places = 0;
            data->new_input = 0;
        }
    } else if (strcmp(btn, "+") == 0 || strcmp(btn, "-") == 0 || 
               strcmp(btn, "X") == 0 || strcmp(btn, "/") == 0) {
        data->accumulator = data->current_value;
        data->operation = btn[0];
        data->new_input = 1;
        data->has_decimal = 0;
        data->decimal_places = 0;
        sprintf(data->expr_preview, "%d %c ", (int)data->accumulator, data->operation);
    } else if (strcmp(btn, "=") == 0) {
        if (data->operation) {
            int64_t result = 0;
            char hist[64];
            sprintf(hist, "%d %c %d = ", (int)data->accumulator, data->operation, (int)data->current_value);
            
            switch (data->operation) {
                case '+': result = data->accumulator + data->current_value; break;
                case '-': result = data->accumulator - data->current_value; break;
                case 'X': result = data->accumulator * data->current_value; break;
                case '/': result = data->current_value != 0 ? data->accumulator / data->current_value : 0; break;
            }
            
            data->current_value = result;
            int_to_str(result, data->display, 0);
            data->operation = 0;
            data->new_input = 1;
            data->expr_preview[0] = '\0';
            
            char res_str[32];
            int_to_str(result, res_str, 0);
            strcat(hist, res_str);
            add_history(data, hist);
        }
    }
}

static void calc_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct calc_data *data = (struct calc_data *)win->widget.data;
    if (win == NULL || data == NULL) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        uint8_t key = event->key;
        if (key == 'q' || key == 27) {
            wm_destroy_window(win);
            calc_window = NULL;
        } else if (key >= '0' && key <= '9') {
            char buf[2] = {key, '\0'};
            handle_button(data, buf);
        } else if (key == '+' || key == '-' || key == '*' || key == '/') {
            char buf[2] = {key == '*' ? 'X' : key, '\0'};
            handle_button(data, buf);
        } else if (key == '=' || key == '\n') {
            handle_button(data, "=");
        } else if (key == 'c' || key == 'C') {
            handle_button(data, "C");
        } else if (key == 'm' || key == 'M') {
            handle_button(data, "M");
            if (data->scientific_mode) wm_resize_window(win, 480, 420);
            else wm_resize_window(win, 320, 420);
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            if (event->y >= 60) {
                uint32_t right_panel_w = data->scientific_mode ? 120 : 0;
                uint32_t calc_w = widget->width - right_panel_w;
                
                if (event->x < calc_w) {
                    uint32_t cols = data->scientific_mode ? 6 : 4;
                    uint32_t btn_w = (calc_w - 20) / cols;
                    uint32_t btn_h = (widget->height - 70) / 5;
                    
                    int col = (event->x - 4) / btn_w;
                    int row = (event->y - 60 - 4) / btn_h;
                    
                    if (col >= 0 && col < (int)cols && row >= 0 && row < 5) {
                        const char *btns_std[5][4] = {
                            {"C", "CE", "%", "/"},
                            {"7", "8", "9", "X"},
                            {"4", "5", "6", "-"},
                            {"1", "2", "3", "+"},
                            {"M", "0", ".", "="}
                        };
                        const char *btns_sci[5][6] = {
                            {"sin", "cos", "tan", "C", "CE", "/"},
                            {"log", "ln",  "sqrt","7", "8", "9"},
                            {"pi",  "e",   "^",   "4", "5", "6"},
                            {"(",   ")",   "X",   "1", "2", "3"},
                            {"M",   "0",   ".",   "-", "+", "="}
                        };
                        const char *btn = data->scientific_mode ? btns_sci[row][col] : btns_std[row][col];
                        handle_button(data, btn);
                        
                        if (strcmp(btn, "M") == 0) {
                            if (data->scientific_mode) wm_resize_window(win, 480, 420);
                            else wm_resize_window(win, 320, 420);
                        }
                    }
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
    
    struct calc_data *data = kmalloc(sizeof(struct calc_data));
    memset(data, 0, sizeof(struct calc_data));
    strcpy(data->display, "0");
    data->new_input = 1;
    
    calc_window = wm_create_window("Calculator", 300, 150, 320, 420);
    if (calc_window != NULL) {
        calc_window->widget.data = data;
        calc_window->draw_content = calc_draw_content;
        calc_window->widget.on_event = calc_event;
        calc_window->bg_color = 0x002a2a2a;
    } else {
        kfree(data);
    }
}
