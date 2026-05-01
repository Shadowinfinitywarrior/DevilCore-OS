#include "keyboard.h"

#include <stdint.h>

#include "pic.h"
#include "io.h"
#include "wm.h"

static uint8_t g_extended;
static uint8_t g_shift_pressed;
static uint8_t g_caps_lock;
static uint8_t g_gui_pressed;
static uint8_t g_ctrl_pressed;
static uint8_t g_alt_pressed;

// Normal scancode to ASCII (unshifted)
static const char normal_map[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// Shifted scancode to ASCII
static const char shift_map[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_init(void) {
    g_extended = 0;
    g_shift_pressed = 0;
    g_caps_lock = 0;
    g_gui_pressed = 0;
    g_ctrl_pressed = 0;
    g_alt_pressed = 0;

    // Enable the keyboard in PS/2 controller
    // Wait for the controller to be ready
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) break;
    }
    outb(0x64, 0x20); // Read Command Byte
    timeout = 100000;
    uint8_t status = 0;
    while (timeout--) {
        if ((inb(0x64) & 1) == 1) {
            status = inb(0x60);
            break;
        }
    }
    
    status |= 0x01; // Enable Keyboard Interrupt (bit 0)
    status &= ~0x10; // Enable Keyboard clock (clear bit 4)
    
    timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) break;
    }
    outb(0x64, 0x60); // Write Command Byte
    timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) break;
    }
    outb(0x60, status);

    pic_enable_irq(1);
}

static char scancode_to_char(uint8_t scancode) {
    if (scancode >= sizeof(normal_map)) return 0;
    
    // Handle Caps Lock for letters
    char c;
    if (g_shift_pressed) {
        c = shift_map[scancode];
    } else {
        c = normal_map[scancode];
    }
    
    // Caps Lock overrides shift for letters
    if (g_caps_lock && c >= 'a' && c <= 'z') {
        c = c - 32; // to uppercase
    } else if (g_caps_lock && c >= 'A' && c <= 'Z') {
        c = c + 32; // to lowercase (if shift+caps)
    }
    
    return c;
}

void keyboard_interrupt(void) {
    if ((inb(0x64) & 0x01) == 0) {
        return;
    }

    uint8_t scancode = inb(0x60);

    // Handle extended scancodes (0xE0 prefix)
    if (scancode == 0xE0) {
        g_extended = 1;
        return;
    }

    // Check if key released
    uint8_t released = (scancode & 0x80) != 0;
    scancode &= 0x7F; // Remove release bit

    // Handle modifier keys
    if (scancode == 0x2A || scancode == 0x36) { // Left Shift / Right Shift
        if (released) {
            g_shift_pressed = 0;
        } else {
            g_shift_pressed = 1;
        }
        g_extended = 0;
        return;
    }
    
    if (scancode == 0x3A) { // Caps Lock
        if (!released) {
            g_caps_lock = !g_caps_lock;
        }
        g_extended = 0;
        return;
    }
    
    // Handle GUI (Windows/Super) key - scancode 0x5B (left) and 0x5C (right)
    if (scancode == 0x5B || scancode == 0x5C) {
        if (released) {
            g_gui_pressed = 0;
        } else {
            g_gui_pressed = 1;
            // Send special GUI key event to window manager
            wm_handle_special_key(0xE0); // GUI key pressed
        }
        g_extended = 0;
        return;
    }
    
    // Handle Ctrl keys
    if (scancode == 0x1D || (g_extended && scancode == 0x1D)) { // Ctrl
        if (released) {
            g_ctrl_pressed = 0;
        } else {
            g_ctrl_pressed = 1;
        }
        g_extended = 0;
        return;
    }
    
    // Handle Alt keys
    if (scancode == 0x38 || (g_extended && scancode == 0x38)) { // Alt
        if (released) {
            g_alt_pressed = 0;
        } else {
            g_alt_pressed = 1;
        }
        g_extended = 0;
        return;
    }

    if (released) {
        g_extended = 0;
        return;
    }

    // Regular key press
    char c = scancode_to_char(scancode);
    if (c != 0) {
        uint8_t modifiers = 0;
        if (g_shift_pressed) modifiers |= 1;
        if (g_caps_lock) modifiers |= 2;
        if (g_gui_pressed) modifiers |= 4; // GUI key modifier
        if (g_ctrl_pressed) modifiers |= 8;
        if (g_alt_pressed) modifiers |= 16;
        wm_handle_key(c, modifiers);
    }

    g_extended = 0;
}

// Get current modifier state
uint8_t keyboard_get_modifiers(void) {
    uint8_t modifiers = 0;
    if (g_shift_pressed) modifiers |= 1;
    if (g_caps_lock) modifiers |= 2;
    if (g_gui_pressed) modifiers |= 4;
    if (g_ctrl_pressed) modifiers |= 8;
    if (g_alt_pressed) modifiers |= 16;
    return modifiers;
}
