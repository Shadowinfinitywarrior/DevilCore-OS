/*
 * DevilCore OS — PS/2 Mouse Driver Header
 */
#ifndef DC_MOUSE_H
#define DC_MOUSE_H

#include "../include/types.h"

void mouse_init(void);
void mouse_handler(uint8_t byte);

extern volatile int g_mouse_x, g_mouse_y;
extern volatile uint8_t g_mouse_btn;
extern volatile int g_mouse_moved;

/* Screen bounds (set by desktop after FB init) */
void mouse_set_bounds(int w, int h);

#endif
