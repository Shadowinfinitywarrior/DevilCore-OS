#ifndef DEVILCORE_TIMER_H
#define DEVILCORE_TIMER_H

#include <stdint.h>

void timer_init(uint32_t frequency);
uint64_t timer_ticks(void);
void timer_set_callback(void (*callback)(void));

#endif