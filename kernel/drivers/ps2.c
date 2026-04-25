#include "io.h"
#include <stdint.h>
#include <stdbool.h>

extern void compositor_move_mouse(int dx, int dy);
extern void compositor_click_mouse(int left, int right);
extern void compositor_keyboard_event(uint8_t scancode);

#define PS2_DATA_PORT 0x60
#define PS2_CMD_PORT  0x64

static inline void ps2_wait_write() {
    while (inb(PS2_CMD_PORT) & 2);
}

static inline void ps2_wait_read() {
    while (!(inb(PS2_CMD_PORT) & 1));
}

static inline void ps2_write_cmd(uint8_t cmd) {
    ps2_wait_write();
    outb(PS2_CMD_PORT, cmd);
}

static inline void ps2_write_data(uint8_t data) {
    ps2_wait_write();
    outb(PS2_DATA_PORT, data);
}

static inline uint8_t ps2_read_data() {
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

void ps2_mouse_init() {
    // Enable auxiliary device
    ps2_write_cmd(0xA8);

    // Read compaq status
    ps2_write_cmd(0x20);
    uint8_t status = ps2_read_data();

    // Enable IRQ12 for mouse and clear disable clock
    status |= 2;
    status &= ~0x20;
    
    ps2_write_cmd(0x60);
    ps2_write_data(status);

    // Tell mouse to use default settings
    ps2_write_cmd(0xD4);
    ps2_write_data(0xF6);
    ps2_read_data();

    // Enable packet streaming
    ps2_write_cmd(0xD4);
    ps2_write_data(0xF4);
    ps2_read_data();
}

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];

void ps2_poll() {
    while (inb(PS2_CMD_PORT) & 1) {
        uint8_t status = inb(PS2_CMD_PORT);
        bool is_mouse = (status & 0x20) != 0;
        uint8_t data = inb(PS2_DATA_PORT);

        if (is_mouse) {
            switch (mouse_cycle) {
                case 0:
                    if (data & 0x08) { // Valid packet
                        mouse_byte[0] = data;
                        mouse_cycle++;
                    }
                    break;
                case 1:
                    mouse_byte[1] = data;
                    mouse_cycle++;
                    break;
                case 2:
                    mouse_byte[2] = data;
                    int dx = mouse_byte[1] - ((mouse_byte[0] << 4) & 0x100);
                    int dy = mouse_byte[2] - ((mouse_byte[0] << 3) & 0x100);
                    
                    int left_click = mouse_byte[0] & 1;
                    int right_click = (mouse_byte[0] & 2) >> 1;
                    
                    compositor_move_mouse(dx, -dy); // Y is reversed in PS/2
                    compositor_click_mouse(left_click, right_click);
                    
                    mouse_cycle = 0;
                    break;
            }
        } else {
            // Keyboard scancode
            compositor_keyboard_event(data);
        }
    }
}
