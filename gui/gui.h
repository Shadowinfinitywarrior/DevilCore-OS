#ifndef _GUI_H
#define _GUI_H

#include <devil/types.h>

void gui_init(void);

typedef struct window {
    uint32_t  id;
    char      title[256];
    uint32_t  x, y;
    uint32_t  width, height;
    uint32_t  bg_color;
    void     *content;
    struct window *next;
    struct window *prev;
} window_t;

typedef struct event {
    int       type;
    uint32_t  window_id;
    uint32_t  x, y;
    uint32_t  key;
    uint32_t  button;
} event_t;

#define EVENT_KEY_PRESS   1
#define EVENT_KEY_RELEASE 2
#define EVENT_BUTTON_PRESS 3
#define EVENT_BUTTON_RELEASE 4
#define EVENT_MOUSE_MOVE 5
#define EVENT_EXPOSE     6
#define EVENT_CLOSE     7
#define EVENT_RESIZE   8

window_t *window_create(const char *title, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void window_destroy(window_t *win);
void window_show(window_t *win);
void window_hide(window_t *win);
void window_move(window_t *win, uint32_t x, uint32_t y);
void window_resize(window_t *win, uint32_t w, uint32_t h);
void window_draw(window_t *win, uint32_t x, uint32_t y, uint32_t color);
void window_draw_text(window_t *win, uint32_t x, uint32_t y, const char *text);

#endif // _GUI_H