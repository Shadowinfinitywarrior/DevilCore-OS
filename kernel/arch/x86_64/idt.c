// kernel/arch/x86_64/idt.c

#include <devil/types.h>
#include "idt.h"
#include "gdt.h"

extern void isr_stub_table(void);

static IDTGate idt[256];
static IDTPointer idt_ptr;

void idt_set_gate(uint8_t num, void *handler, uint16_t sel, uint8_t attrs) {
    uint64_t addr = (uint64_t)handler;
    
    idt[num].offset_low = addr & 0xFFFF;
    idt[num].offset_mid = (addr >> 16) & 0xFFFF;
    idt[num].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].type_attr = attrs;
    idt[num].reserved = 0;
}

void idt_init(void) {
    // Set all gates to zero first
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (void*)0, 0, 0);
    }
    
    // CPU Exceptions (0-31)
    extern void isr_stub_0(void);  extern void isr_stub_1(void);
    extern void isr_stub_2(void);  extern void isr_stub_3(void);
    extern void isr_stub_4(void);  extern void isr_stub_5(void);
    extern void isr_stub_6(void);  extern void isr_stub_7(void);
    extern void isr_stub_8(void);  extern void isr_stub_9(void);
    extern void isr_stub_10(void); extern void isr_stub_11(void);
    extern void isr_stub_12(void); extern void isr_stub_13(void);
    extern void isr_stub_14(void); extern void isr_stub_15(void);
    extern void isr_stub_16(void); extern void isr_stub_17(void);
    extern void isr_stub_18(void); extern void isr_stub_19(void);
    extern void isr_stub_20(void); extern void isr_stub_21(void);
    extern void isr_stub_22(void); extern void isr_stub_23(void);
    extern void isr_stub_24(void); extern void isr_stub_25(void);
    extern void isr_stub_26(void); extern void isr_stub_27(void);
    extern void isr_stub_28(void); extern void isr_stub_29(void);
    extern void isr_stub_30(void); extern void isr_stub_31(void);
    
    // IRQ Interrupts (32-47)
    extern void irq_stub_0(void);  extern void irq_stub_1(void);
    extern void irq_stub_2(void);  extern void irq_stub_3(void);
    extern void irq_stub_4(void);  extern void irq_stub_5(void);
    extern void irq_stub_6(void);  extern void irq_stub_7(void);
    extern void irq_stub_8(void);  extern void irq_stub_9(void);
    extern void irq_stub_10(void); extern void irq_stub_11(void);
    extern void irq_stub_12(void); extern void irq_stub_13(void);
    extern void irq_stub_14(void); extern void irq_stub_15(void);
    
    // Kernel code segment selector
    uint16_t kernel_cs = 0x08;
    
    // Set exception handlers
    idt_set_gate(0,  isr_stub_0,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(1,  isr_stub_1,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(2,  isr_stub_2,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(3,  isr_stub_3,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(4,  isr_stub_4,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(5,  isr_stub_5,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(6,  isr_stub_6,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(7,  isr_stub_7,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(8,  isr_stub_8,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(9,  isr_stub_9,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(10, isr_stub_10, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(11, isr_stub_11, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(12, isr_stub_12, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(13, isr_stub_13, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(14, isr_stub_14, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(15, isr_stub_15, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(16, isr_stub_16, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(17, isr_stub_17, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(18, isr_stub_18, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(19, isr_stub_19, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(20, isr_stub_20, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(21, isr_stub_21, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(22, isr_stub_22, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(23, isr_stub_23, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(24, isr_stub_24, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(25, isr_stub_25, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(26, isr_stub_26, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(27, isr_stub_27, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(28, isr_stub_28, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(29, isr_stub_29, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(30, isr_stub_30, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(31, isr_stub_31, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    
    // IRQ handlers
    idt_set_gate(32, irq_stub_0,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(33, irq_stub_1,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(34, irq_stub_2,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(35, irq_stub_3,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(36, irq_stub_4,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(37, irq_stub_5,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(38, irq_stub_6,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(39, irq_stub_7,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(40, irq_stub_8,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(41, irq_stub_9,  kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(42, irq_stub_10, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(43, irq_stub_11, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(44, irq_stub_12, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(45, irq_stub_13, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(46, irq_stub_14, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    idt_set_gate(47, irq_stub_15, kernel_cs, IDT_DESC(IDT_INTERRUPT, IDT_DPL0) | IDT_PRESENT);
    
    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    
    extern void idt_flush(uint64_t ptr);
    idt_flush((uint64_t)&idt_ptr);
}