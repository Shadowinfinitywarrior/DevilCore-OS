#include "timer.h"

#include <stddef.h>
#include <stdint.h>

#include "pic.h"
#include "scheduler.h"

static uint64_t g_ticks;
static void (*g_callback)(void);

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    g_ticks = 0;
    g_callback = NULL;

    pic_enable_irq(0);
}

uint64_t timer_ticks(void) {
    return g_ticks;
}

void timer_set_callback(void (*callback)(void)) {
    g_callback = callback;
}

void timer_tick(void) {
    ++g_ticks;
    if (g_ticks % 100 == 0) {
        // serial_write_string("."); // Heartbeat every second
    }
    if (g_callback != NULL) {
        g_callback();
    }
    
    scheduler_tick();
}