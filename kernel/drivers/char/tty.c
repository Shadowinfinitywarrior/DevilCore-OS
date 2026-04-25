// kernel/drivers/char/tty.c

#include <devil/types.h>
#include "tty.h"
#include <asm/io.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

static uint16_t base_port = COM1;
static char input_buffer[4096];
static size_t input_head;
static size_t input_tail;

static int line_height = 25;
static int line_width = 80;
static int cursor_x;
static int cursor_y;

static void serial_putchar(char c) {
    while (!(inb(base_port + 5) & 0x20)) {}
    outb(base_port, c);
}

static void serial_write(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        serial_putchar(buf[i]);
    }
}

void console_putchar(char c) {
    // VGA text mode output
    static volatile uint16_t *vga = (volatile uint16_t*)0xB8000;
    
    if (c == '\n') {
        cursor_y++;
        cursor_x = 0;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
    } else {
        vga[cursor_y * 80 + cursor_x] = (c & 0xFF) | 0x0F00;
        cursor_x++;
        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    
    if (cursor_y >= 25) {
        // Scroll
        for (int i = 0; i < 25 * 80; i++) {
            vga[i] = vga[i + 80];
        }
        for (int i = 24 * 80; i < 25 * 80; i++) {
            vga[i] = 0x0F00;
        }
        cursor_y = 24;
    }
    
    // Update cursor
    uint16_t pos = cursor_y * 80 + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, pos >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, pos & 0xFF);
}

void tty_init(void) {
    // Initialize serial port
    outb(base_port + 1, 0);      // Disable interrupts
    outb(base_port + 3, 0x80);   // Enable DLAB
    outb(base_port + 0, 12);    // Set divisor: 115200/12 = 9600
    outb(base_port + 1, 0);
    outb(base_port + 3, 0x03);   // 8N1
    outb(base_port + 2, 0xC7);   // Enable FIFO
    outb(base_port + 4, 0x0B);   // IRQs enabled, RTS/DSR
    
    // Initialize VGA
    cursor_x = 0;
    cursor_y = 0;
    
    input_head = 0;
    input_tail = 0;
}

void tty_putchar(char c) {
    console_putchar(c);
    serial_putchar(c);
}

void tty_write(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        tty_putchar(buf[i]);

char tty_getchar(void) {
    while (input_head == input_tail) {
        // Wait for input
    }
    char c = input_buffer[input_tail];
    input_tail = (input_tail + 1) % 4096;
    return c;
}

int tty_read(char *buf, size_t count) {
    size_t i = 0;
    while (i < count) {
        if (input_head == input_tail) {
            break;
        }
        buf[i++] = input_buffer[input_tail];
        input_tail = (input_tail + 1) % 4096;
    }
    return i;
}