#ifndef _ISR_H
#define _ISR_H

#include <devil/types.h>

void idt_init(void);
void idt_set_gate(uint8_t num, void *handler, uint16_t sel, uint8_t attrs);

#define IDT_PRESENT    0x80
#define IDT_DPL0      0x00
#define IDT_DPL3      0x60
#define IDT_32BIT     0x0E
#define IDT_64BIT     0x0E
#define IDT_TRAP      0x0F
#define IDT_INTERRUPT 0x06

#define IDT_DESC(type, dpl) (((type) & 0x0F) | ((dpl) & 0x60))

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} PACKED IDTGate;

typedef struct {
    uint16_t limit;
    uint64_t base;
} PACKED IDTPointer;

#endif // _ISR_H