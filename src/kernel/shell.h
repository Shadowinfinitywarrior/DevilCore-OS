#ifndef DEVILCORE_SHELL_H
#define DEVILCORE_SHELL_H

#include "wm.h"

void shell_init(struct wm_window *win);
void shell_on_event(struct wm_widget *widget, struct wm_event *event);
void shell_update(struct wm_window *win);

#endif
