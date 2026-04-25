/*
 * DevilCore OS — PS/2 Keyboard Driver
 * Full scancode set 1 with modifiers, special keys, and ring buffer.
 */
#include "keyboard.h"
#include "../include/ports.h"

#define KBD_BUF_SIZE 128

static char g_kbd_buf[KBD_BUF_SIZE];
static volatile int g_kbd_head = 0;
static volatile int g_kbd_tail = 0;
volatile int g_shift = 0;
volatile int g_ctrl  = 0;
volatile int g_caps  = 0;

static const char sc_lower[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' ',0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,
    0,0
};

static const char sc_upper[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' ',0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,
    0,0
};

static void kbd_push(char c) {
    int next = (g_kbd_head + 1) % KBD_BUF_SIZE;
    if (next != g_kbd_tail) {
        g_kbd_buf[g_kbd_head] = c;
        g_kbd_head = next;
    }
}

int kbd_pop(char *c) {
    if (g_kbd_tail == g_kbd_head) return 0;
    *c = g_kbd_buf[g_kbd_tail];
    g_kbd_tail = (g_kbd_tail + 1) % KBD_BUF_SIZE;
    return 1;
}

void kbd_handler(uint8_t scancode) {
    int released = (scancode & 0x80);
    uint8_t key  = scancode & 0x7F;

    if (key == 0x2A || key == 0x36) { g_shift = !released; return; }
    if (key == 0x1D)                { g_ctrl  = !released; return; }
    if (key == 0x3A && !released)   { g_caps  = !g_caps;   return; }
    if (released) return;

    if (key < 128) {
        int upper = (g_shift ^ g_caps) && sc_lower[key] >= 'a' && sc_lower[key] <= 'z';
        char c = (g_shift || upper) ? sc_upper[key] : sc_lower[key];
        if (c) kbd_push(c);
    }
}

void kbd_init(void) {
    while (inb(0x64) & 1) inb(0x60);
}
