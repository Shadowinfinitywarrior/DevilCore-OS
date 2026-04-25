/* kernel/gui/compositor.h */
#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdint.h>

void compositor_init(uint32_t *framebuffer, int width, int height, int pitch_bytes);
void compositor_flush(void);

void draw_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_outline(int x, int y, int w, int h, uint32_t color);
void draw_gradient_rect(int x, int y, int w, int h, uint32_t top, uint32_t bot);
void draw_char(int x, int y, char ch, uint32_t color, uint32_t bg);
void draw_string(int x, int y, const char *s, uint32_t color, uint32_t bg);
void draw_string2x(int x, int y, const char *s, uint32_t color);
void compositor_draw_cursor(void);

int  compositor_get_width(void);
int  compositor_get_height(void);
int  compositor_mouse_x(void);
int  compositor_mouse_y(void);
int  compositor_mouse_left(void);
int  compositor_mouse_right(void);
int  compositor_mouse_clicked(void);
char compositor_get_char(void);

void compositor_move_mouse(int dx, int dy);
void compositor_click_mouse(int left, int right);
void compositor_keyboard_event(uint8_t scancode);

#endif
