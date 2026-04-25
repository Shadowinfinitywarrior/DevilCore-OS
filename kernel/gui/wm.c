/*
 * DevilCore OS — Window Manager Implementation
 */
#include "wm.h"
#include "../libc/string.h"

Window g_wins[MAX_WINDOWS];
int    g_win_count = 0;
int    g_focused_win = -1;
uint8_t g_prev_mouse_btn = 0;
int    g_drag_win = -1;

int g_os_state = STATE_LOGIN;
char g_login_user[32] = {0};
char g_login_pass[32] = {0};
int  g_login_focus = 0;
int  g_login_error = 0;

Window *wm_create(int type, int x, int y, int w, int h, const char *title) {
    if (g_win_count >= MAX_WINDOWS) return NULL;
    for (int i = 0; i < g_win_count; i++) {
        if (g_wins[i].type == type && g_wins[i].visible) {
            g_focused_win = i; 
            return &g_wins[i];
        }
    }
    Window *win = &g_wins[g_win_count++];
    memset(win, 0, sizeof(Window));
    win->id = g_win_count;
    win->type = type;
    win->x = x; 
    win->y = y; 
    win->w = w; 
    win->h = h;
    win->visible = true;
    win->dirty = 1;
    strncpy(win->title, title, sizeof(win->title) - 1);
    
    if (type == WM_TERMINAL) {
        strcpy(win->text_buf, "DevilCore OS v5.0 — Privacy Terminal\n"
               "Type 'help' for available commands\n\n");
        win->text_len = strlen(win->text_buf);
    }
    if (type == WM_EDITOR) {
        strcpy(win->text_buf, "# DevilCore Text Editor\n# Start typing...\n\n");
        win->text_len = strlen(win->text_buf);
    }
    
    g_focused_win = g_win_count - 1;
    return win;
}

void wm_close_type(int type) {
    for (int i = 0; i < g_win_count; i++) {
        if (g_wins[i].type == type) {
            g_wins[i].visible = false;
            if (g_focused_win == i) g_focused_win = -1;
        }
    }
}
