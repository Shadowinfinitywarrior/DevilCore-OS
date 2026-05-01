#include "texteditor.h"
#include "wm.h"
#include "framebuffer.h"
#include "kprintf.h"
#include <string.h>
#include <stdio.h>

#include "vfs.h"

extern struct wm_desktop *desktop;

#define TE_MAX_LINES  500
#define TE_LINE_WIDTH 256

struct te_line {
    char text[TE_LINE_WIDTH];
    uint32_t len;
};

struct te_data {
    struct te_line lines[TE_MAX_LINES];
    uint32_t line_count;
    uint32_t cursor_line;
    uint32_t cursor_col;
    uint32_t scroll_line;
    char filename[256];
    uint8_t modified;
};

static struct wm_window *te_window = NULL;

static void te_draw_content(struct wm_window *win) {
    struct te_data *data = (struct te_data *)win->widget.data;
    if (!data) return;
    
    uint32_t x = win->widget.x;
    uint32_t y = win->widget.y;
    uint32_t w = win->widget.width;
    uint32_t h = win->widget.height;
    
    // Toolbar
    fb_fill_rect(x, y, w, 30, 0x002a2a2a);
    fb_draw_rect_outline(x, y, w, 30, 0x00505050);
    fb_draw_string(x + 10, y + 10, "[New] [Open] [Save]", 0x00aaaaaa, 0);
    
    // Text area
    y += 30;
    uint32_t text_h = h - 54;
    fb_fill_rect(x, y, w, text_h, 0x00ffffff);
    fb_draw_rect_outline(x, y, w, text_h, 0x00505050);
    
    // Draw text with syntax highlighting (basic C-style)
    uint32_t text_x = x + 40; // Gutter for line numbers
    uint32_t text_y = y + 10;
    
    // Draw line numbers
    fb_fill_rect(x, y, 32, text_h, 0x00f0f0f0);
    fb_draw_line(x + 32, y, x + 32, y + text_h, 0x00cccccc);
    
    for (uint32_t i = data->scroll_line; i < data->line_count; i++) {
        if (text_y >= y + text_h - 20) break;
        
        // Draw line number
        char ln_buf[16];
        sprintf(ln_buf, "%d", i + 1);
        fb_draw_string(x + 4, text_y, ln_buf, 0x00888888, 0);
        
        // Basic Syntax Highlighting
        struct te_line *line = &data->lines[i];
        uint32_t current_x = text_x;
        
        for (uint32_t j = 0; j < line->len; j++) {
            uint32_t color = 0x00000000; // default black
            
            // Very simple token detection
            if (line->text[j] == '"' || line->text[j] == '\'') color = 0x0000aa00; // string green
            else if (j > 0 && line->text[j-1] == '/' && line->text[j] == '/') color = 0x00888888; // comment
            else if (strncmp(line->text + j, "int ", 4) == 0 || strncmp(line->text + j, "void ", 5) == 0) color = 0x000000ff; // keyword blue
            
            fb_draw_char_colored(current_x, text_y, line->text[j], color);
            current_x += 8;
        }
        
        // Draw cursor
        if (win->widget.focused && wm_is_cursor_visible() && i == data->cursor_line) {
            fb_fill_rect(text_x + data->cursor_col * 8, text_y, 2, 16, 0x00000000);
        }
        
        text_y += 16;
    }
    
    // Status bar
    y = win->widget.y + h - 24;
    fb_fill_rect_gradient_h(x, y, w, 24, 0x002a2a2a, 0x00303030);
    char status[128];
    sprintf(status, "File: %s %s | Line: %d, Col: %d", 
            data->filename[0] ? data->filename : "Untitled",
            data->modified ? "*" : "",
            data->cursor_line + 1, data->cursor_col + 1);
    fb_draw_string(x + 6, y + 8, status, 0x00aaaaaa, 0);
}

static void te_event(struct wm_widget *widget, struct wm_event *event) {
    struct wm_window *win = (struct wm_window *)widget;
    struct te_data *data = (struct te_data *)win->widget.data;
    if (win == NULL || data == NULL) return;
    
    if (wm_window_on_event(widget, event)) return;
    
    if (event->type == WM_EVENT_KEY) {
        uint8_t key = event->key;
        struct te_line *cline = &data->lines[data->cursor_line];
        
        if (key == 'q' && event->modifiers) { // Ctrl+Q
            wm_destroy_window(win);
            te_window = NULL;
        } else if (key == 17) { // Up
            if (data->cursor_line > 0) {
                data->cursor_line--;
                if (data->cursor_col > data->lines[data->cursor_line].len) 
                    data->cursor_col = data->lines[data->cursor_line].len;
            }
        } else if (key == 18) { // Down
            if (data->cursor_line < data->line_count - 1) {
                data->cursor_line++;
                if (data->cursor_col > data->lines[data->cursor_line].len) 
                    data->cursor_col = data->lines[data->cursor_line].len;
            }
        } else if (key == 19) { // Left
            if (data->cursor_col > 0) data->cursor_col--;
            else if (data->cursor_line > 0) {
                data->cursor_line--;
                data->cursor_col = data->lines[data->cursor_line].len;
            }
        } else if (key == 20) { // Right
            if (data->cursor_col < cline->len) data->cursor_col++;
            else if (data->cursor_line < data->line_count - 1) {
                data->cursor_line++;
                data->cursor_col = 0;
            }
        } else if (key == '\b') { // Backspace
            if (data->cursor_col > 0) {
                memmove(&cline->text[data->cursor_col - 1], &cline->text[data->cursor_col], cline->len - data->cursor_col);
                cline->len--;
                data->cursor_col--;
                data->modified = 1;
            } else if (data->cursor_line > 0) {
                // Merge lines
                struct te_line *prev = &data->lines[data->cursor_line - 1];
                uint32_t old_len = prev->len;
                if (prev->len + cline->len < TE_LINE_WIDTH) {
                    memcpy(&prev->text[prev->len], cline->text, cline->len);
                    prev->len += cline->len;
                    for (uint32_t i = data->cursor_line; i < data->line_count - 1; i++) {
                        data->lines[i] = data->lines[i+1];
                    }
                    data->line_count--;
                    data->cursor_line--;
                    data->cursor_col = old_len;
                    data->modified = 1;
                }
            }
        } else if (key >= 32 && key <= 126) { // Printable
            if (cline->len < TE_LINE_WIDTH - 1) {
                memmove(&cline->text[data->cursor_col + 1], &cline->text[data->cursor_col], cline->len - data->cursor_col);
                cline->text[data->cursor_col] = (char)key;
                cline->len++;
                data->cursor_col++;
                data->modified = 1;
            }
        } else if (key == '\n') {
            if (data->line_count < TE_MAX_LINES) {
                // Split line
                for (uint32_t i = data->line_count; i > data->cursor_line; i--) {
                    data->lines[i] = data->lines[i-1];
                }
                data->line_count++;
                struct te_line *nline = &data->lines[data->cursor_line + 1];
                nline->len = cline->len - data->cursor_col;
                memcpy(nline->text, &cline->text[data->cursor_col], nline->len);
                cline->len = data->cursor_col;
                data->cursor_line++;
                data->cursor_col = 0;
                data->modified = 1;
            }
        }
        
        // Auto scroll
        if (data->cursor_line < data->scroll_line) data->scroll_line = data->cursor_line;
        uint32_t visible_lines = (win->widget.height - 54) / 16;
        if (data->cursor_line >= data->scroll_line + visible_lines) {
            data->scroll_line = data->cursor_line - visible_lines + 1;
        }
    } else if (event->type == WM_EVENT_MOUSE_PRESS) {
        if (event->buttons & 1) { // Left click
            if (event->y >= 0 && event->y < 30) {
                if (event->x >= 10 && event->x < 50) { // [New]
                    data->line_count = 1;
                    data->lines[0].len = 0;
                    data->cursor_line = 0;
                    data->cursor_col = 0;
                    data->modified = 0;
                    data->filename[0] = '\0';
                } else if (event->x >= 60 && event->x < 110) { // [Open]
                    // Basic demo
                    strcpy(data->filename, "test.c");
                    strcpy(data->lines[0].text, "#include <stdio.h>");
                    data->lines[0].len = 18;
                    strcpy(data->lines[1].text, "int main() {");
                    data->lines[1].len = 12;
                    strcpy(data->lines[2].text, "    printf(\"Hello DevilCore!\\n\");");
                    data->lines[2].len = 33;
                    strcpy(data->lines[3].text, "    return 0;");
                    data->lines[3].len = 13;
                    strcpy(data->lines[4].text, "}");
                    data->lines[4].len = 1;
                    data->line_count = 5;
                    data->modified = 0;
                } else if (event->x >= 120 && event->x < 170) { // [Save]
                    if (data->filename[0]) {
                        // VFS write
                        struct vfs_file f;
                        if (vfs_open(&f, data->filename, 0) == 0) { // 0 for create/write
                            for (uint32_t i = 0; i < data->line_count; i++) {
                                size_t w;
                                vfs_write(&f, data->lines[i].text, data->lines[i].len, &w);
                                vfs_write(&f, "\n", 1, &w);
                            }
                            vfs_close(&f);
                            data->modified = 0;
                        }
                    }
                }
            } else if (event->y >= 30 && event->y < win->widget.height - 24) {
                // Click in text area to position cursor
                uint32_t click_line = data->scroll_line + (event->y - 30 - 10) / 16;
                if (click_line < data->line_count) {
                    data->cursor_line = click_line;
                    uint32_t click_col = (event->x - 40) / 8;
                    if (click_col > data->lines[click_line].len) click_col = data->lines[click_line].len;
                    data->cursor_col = click_col;
                }
            }
        }
    }
}

void te_open(void) {
    if (te_window != NULL) {
        wm_focus_window(te_window);
        return;
    }
    
    struct te_data *data = kmalloc(sizeof(struct te_data));
    memset(data, 0, sizeof(struct te_data));
    data->line_count = 1; // start with one empty line
    strcpy(data->filename, "");
    
    te_window = wm_create_window("Text Editor", 120, 100, 650, 450);
    if (te_window != NULL) {
        te_window->widget.data = data;
        te_window->draw_content = te_draw_content;
        te_window->widget.on_event = te_event;
        te_window->bg_color = 0x00ffffff;
    } else {
        kfree(data);
    }
}
