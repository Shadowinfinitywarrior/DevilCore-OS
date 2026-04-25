/*
 * DevilCore OS — 64-bit IDT Module
 */
#include "idt.h"
#include "../libc/string.h"

struct idt_entry {
    uint16_t off_low;
    uint16_t seg;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t off_mid;
    uint32_t off_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry g_idt[256];
static struct idt_ptr   g_idtp;

extern uint64_t isr_table[];

static void idt_set(int i, uint64_t handler) {
    g_idt[i].off_low  = handler & 0xFFFF;
    g_idt[i].seg      = 0x08;
    g_idt[i].ist      = 0;
    g_idt[i].flags    = 0x8E;
    g_idt[i].off_mid  = (handler >> 16) & 0xFFFF;
    g_idt[i].off_high = (handler >> 32) & 0xFFFFFFFF;
    g_idt[i].reserved = 0;
}

void idt_init(void) {
    memset(g_idt, 0, sizeof(g_idt));
    g_idtp.limit = sizeof(g_idt) - 1;
    g_idtp.base  = (uint64_t)&g_idt;
    for (int i = 0; i < 256; i++)
        idt_set(i, isr_table[i]);
    __asm__ volatile("lidt %0" :: "m"(g_idtp));
}
