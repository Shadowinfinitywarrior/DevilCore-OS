/*
 * DevilCore OS — Window Manager Abstraction Header
 */
#ifndef DC_WM_H
#define DC_WM_H

#include "../include/types.h"

#define MAX_WINDOWS  16
#define TITLEBAR_H   24
#define WM_NONE      0
#define WM_TERMINAL  1
#define WM_EDITOR    2
#define WM_FILEMAN   3
#define WM_SYSMON    4
#define WM_NETMON    5
#define WM_SETTINGS  6
#define WM_ABOUT     7

typedef struct {
    int     id, type;
    int     x, y, w, h;
    bool    visible, focused, dragging;
    int     drag_ox, drag_oy;
    char    title[48];
    int     scroll;
    char    text_buf[4096];
    int     text_len;
    char    input_buf[128];
    int     input_len;
    int     selected;
    int     dirty;
} Window;

extern Window g_wins[MAX_WINDOWS];
extern int    g_win_count;
extern int    g_focused_win;
extern uint8_t g_prev_mouse_btn;
extern int    g_drag_win;

Window *wm_create(int type, int x, int y, int w, int h, const char *title);
void    wm_close_type(int type);

/* OS State Machine */
#define STATE_LOGIN   0
#define STATE_DESKTOP 1
extern int g_os_state;

extern char g_login_user[32];
extern char g_login_pass[32];
extern int  g_login_focus;
extern int  g_login_error;

#endif /* DC_WM_H */
