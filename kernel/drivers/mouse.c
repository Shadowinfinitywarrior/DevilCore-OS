/*
 * DevilCore OS — PS/2 Mouse Driver
 * 3-byte packet protocol with smooth acceleration.
 */
#include "mouse.h"
#include "../include/ports.h"

volatile int g_mouse_x = 512;
volatile int g_mouse_y = 384;
volatile uint8_t g_mouse_btn = 0;
volatile int g_mouse_moved = 0;

static int g_bound_w = 1024, g_bound_h = 768;
static uint8_t g_mouse_pkt[3];
static int g_mouse_phase = 0;

void mouse_set_bounds(int w, int h) {
    g_bound_w = w;
    g_bound_h = h;
    g_mouse_x = w / 2;
    g_mouse_y = h / 2;
}

static void mouse_write(uint8_t b) {
    while (inb(0x64) & 2);
    outb(0x64, 0xD4);
    while (inb(0x64) & 2);
    outb(0x60, b);
}

static uint8_t mouse_read(void) {
    while (!(inb(0x64) & 1));
    return inb(0x60);
}

void mouse_init(void) {
    while (inb(0x64) & 2) {}
    outb(0x64, 0xA8);
    while (inb(0x64) & 2) {}
    outb(0x64, 0x20);
    io_wait();
    uint8_t status = inb(0x60) | 2;
    while (inb(0x64) & 2) {}
    outb(0x64, 0x60);
    while (inb(0x64) & 2) {}
    outb(0x60, status);
    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();
}

void mouse_handler(uint8_t byte) {
    g_mouse_pkt[g_mouse_phase++] = byte;
    if (g_mouse_phase < 3) return;
    g_mouse_phase = 0;

    uint8_t flags = g_mouse_pkt[0];
    /* Discard if overflow bits set */
    if (flags & 0xC0) return;
    /* Discard if bit 3 not set (sync check) */
    if (!(flags & 0x08)) { g_mouse_phase = 0; return; }

    int8_t dx = (int8_t)g_mouse_pkt[1];
    int8_t dy = (int8_t)g_mouse_pkt[2];

    g_mouse_btn = flags & 0x07;

    /* Apply mouse acceleration curve */
    int adx = dx, ady = -dy;
    int speed = (adx < 0 ? -adx : adx) + (ady < 0 ? -ady : ady);
    if (speed > 8) { adx = adx * 2; ady = ady * 2; }

    g_mouse_x += adx;
    g_mouse_y += ady;

    /* Clamp */
    if (g_mouse_x < 0) g_mouse_x = 0;
    if (g_mouse_y < 0) g_mouse_y = 0;
    if (g_mouse_x >= g_bound_w) g_mouse_x = g_bound_w - 1;
    if (g_mouse_y >= g_bound_h) g_mouse_y = g_bound_h - 1;

    g_mouse_moved = 1;
}
