#ifndef DEVILCORE_IDT_H
#define DEVILCORE_IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

#define IDT_PRESENT     (1ULL << 47)
#define IDT_USER       (1ULL << 45)
#define IDT_INTERRUPT  (0xE << 8)
#define IDT_TRAP       (0xF << 8)

void idt_init(void);
void idt_set_gate(uint8_t vector, void *handler, uint8_t dpl);

void isr_handler(void);
void irq_handler(void);

void idt_load(void);

#endif