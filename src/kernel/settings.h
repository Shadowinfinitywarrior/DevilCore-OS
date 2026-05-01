#ifndef DEVILCORE_SETTINGS_H
#define DEVILCORE_SETTINGS_H

#include "wm.h"

void settings_open(void);
void settings_on_event(struct wm_widget *widget, struct wm_event *event);
void settings_draw(struct wm_window *win);

#endif
