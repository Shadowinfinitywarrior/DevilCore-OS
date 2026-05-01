#include "mouse.h"
#include "io.h"
#include "pic.h"
#include "wm.h"

#define MOUSE_DATA_PORT 0x60
#define MOUSE_STATUS_PORT 0x64
#define MOUSE_COMMAND_PORT 0x64

// Simple, responsive mouse parameters
#define MOUSE_SENSITIVITY_NUM 3    // Sensitivity numerator (3/2 = 1.5x)
#define MOUSE_SENSITIVITY_DEN 2

static uint8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];
static struct mouse_state current_state;

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_STATUS_PORT) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_STATUS_PORT) & 2) == 0) return;
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0xD4);
    mouse_wait(1);
    outb(MOUSE_DATA_PORT, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(MOUSE_DATA_PORT);
}

void mouse_init(void) {
    uint8_t status;

    // Enable the auxiliary mouse device
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0xA8);

    // Enable the interrupts
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0x20);
    mouse_wait(0);
    status = (inb(MOUSE_DATA_PORT) | 2);
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait(1);
    outb(MOUSE_DATA_PORT, status);

    // Use default settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge

    // Enable the mouse
    mouse_write(0xF4);
    mouse_read(); // Acknowledge

    pic_enable_irq(12);
}

// Simple direct mouse movement without excessive smoothing
static int32_t apply_sensitivity(int32_t delta) {
    return (delta * MOUSE_SENSITIVITY_NUM) / MOUSE_SENSITIVITY_DEN;
}

void mouse_interrupt(void) {
    uint8_t status = inb(MOUSE_STATUS_PORT);
    if (!(status & 0x20)) return; // Not mouse data

    uint8_t data = inb(MOUSE_DATA_PORT);
    
    // Synchronize: the first byte of a packet always has bit 3 set
    if (mouse_cycle == 0 && !(data & 0x08)) {
        return; 
    }

    mouse_byte[mouse_cycle++] = data;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        int32_t raw_dx = (int8_t)mouse_byte[1];  // Sign-extend
        int32_t raw_dy = (int8_t)mouse_byte[2];  // Sign-extend

        if (mouse_byte[0] & 0x10) raw_dx -= 256;
        if (mouse_byte[0] & 0x20) raw_dy -= 256;
        
        // Invert Y (hardware Y is opposite to screen Y)
        raw_dy = -raw_dy;

        // Apply simple sensitivity multiplier (direct, responsive movement)
        int32_t final_dx = apply_sensitivity(raw_dx);
        int32_t final_dy = apply_sensitivity(raw_dy);
        
        current_state.dx = final_dx;
        current_state.dy = final_dy;
        current_state.buttons = mouse_byte[0] & 0x07;

        wm_handle_mouse(0, 0, current_state.dx, current_state.dy, current_state.buttons);
    }
}

struct mouse_state mouse_get_state(void) {
    return current_state;
}
