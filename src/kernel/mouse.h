#ifndef DEVILCORE_MOUSE_H
#define DEVILCORE_MOUSE_H

#include <stdint.h>

struct mouse_state {
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
    uint8_t buttons;
};

void mouse_init(void);
void mouse_interrupt(void);
struct mouse_state mouse_get_state(void);

#endif
