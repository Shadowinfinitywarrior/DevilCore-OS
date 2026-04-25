/*
 * DevilCore OS — PIT Timer + RTC Driver
 */
#include "timer.h"
#include "../include/ports.h"

volatile uint32_t g_ticks = 0;
uint8_t g_hour = 0, g_min = 0, g_sec = 0;

void pit_init(void) {
    uint32_t divisor = 1193180 / HZ;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

static uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static uint8_t bcd2bin(uint8_t b) {
    return ((b >> 4) * 10) + (b & 0x0F);
}

void rtc_update(void) {
    g_sec  = bcd2bin(rtc_read(0x00));
    g_min  = bcd2bin(rtc_read(0x02));
    g_hour = bcd2bin(rtc_read(0x04));
}
