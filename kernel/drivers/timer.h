/*
 * DevilCore OS — PIT Timer + RTC Driver Header
 */
#ifndef DC_TIMER_H
#define DC_TIMER_H

#include "../include/types.h"

#define HZ 100

extern volatile uint32_t g_ticks;
extern uint8_t g_hour, g_min, g_sec;

void pit_init(void);
void rtc_update(void);

#endif
