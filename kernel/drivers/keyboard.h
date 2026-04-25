/*
 * DevilCore OS — PS/2 Keyboard Driver Header
 */
#ifndef DC_KEYBOARD_H
#define DC_KEYBOARD_H

#include "../include/types.h"

void kbd_init(void);
void kbd_handler(uint8_t scancode);
int  kbd_pop(char *c);

extern volatile int g_shift, g_ctrl, g_caps;

#endif
