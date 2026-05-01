#include "idt.h"

#include <stddef.h>
#include <stdint.h>

#define IDT_FLAGS(seg, dpl, type) (((uint64_t)(seg) << 16) | ((uint64_t)(dpl) << 13) | (uint64_t)(type) | (1ULL << 47)))

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags_type;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_p;

static inline void lidt(uint64_t ptr) {
    __asm__ volatile ("lidt (%0)" : : "r"(ptr));
}

void idt_init(void) {
    uint16_t current_cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(current_cs));

    for (size_t i = 0; i < IDT_ENTRIES; ++i) {
        idt[i].offset_low = 0;
        idt[i].selector = current_cs;
        idt[i].ist = 0;
        idt[i].flags_type = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].reserved = 0;
    }

    idt_p.limit = (uint16_t)(sizeof(idt) - 1);
    idt_p.base = (uint64_t)(uintptr_t)idt;

    lidt((uint64_t)(uintptr_t)&idt_p);
}

void idt_set_gate(uint8_t vector, void *handler, uint8_t dpl) {
    uint64_t addr = (uint64_t)(uintptr_t)handler;
    uint16_t current_cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(current_cs));

    idt[vector].offset_low = (uint16_t)(addr & 0xFFFF);
    idt[vector].selector = current_cs;
    idt[vector].ist = 0;
    idt[vector].flags_type = 0x8E | (dpl << 5);
    idt[vector].offset_mid = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vector].reserved = 0;
}