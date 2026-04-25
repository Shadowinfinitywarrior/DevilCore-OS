// kernel/drivers/input/ps2_keyboard.c
// PS/2 Keyboard driver

#include <devil/types.h>
#include <asm/io.h>

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_CMD_PORT 0x64

#define KBD_STAT_OBF 0x01
#define KBD_STAT_AUX 0x20

static uint8_t kbd_buffer[256];
static int kbd_head;
static int kbd_tail;
static uint8_t keymap[128][4] = {
    [0] = {0, 0, 0, 0},
    ['\r'] = {'\r', '\r', '\r', '\r'},
    ['\n'] = {'\n', '\n', '\n', '\n'},
    [' '] = {' ', ' ', ' ', ' '},
};

void kbd_init(void) {
    kbd_head = 0;
    kbd_tail = 0;
    
    // Enable keyboard
    outb(KBD_CMD_PORT, 0xAE);
    outb(KBD_CMD_PORT, 0x60);
    outb(KBD_DATA_PORT, 0xF0);
    outb(KBD_DATA_PORT, 0x01);
    
    printf("[Keyboard] PS/2 keyboard initialized\n");
}

void kbd_interrupt(void) {
    uint8_t scancode = inb(KBD_DATA_PORT);
    
    kbd_buffer[kbd_head] = scancode;
    kbd_head = (kbd_head + 1) % 256;
}

uint8_t kbd_read(void) {
    if (kbd_head == kbd_tail) return 0;
    
    uint8_t key = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % 256;
    return key;
}