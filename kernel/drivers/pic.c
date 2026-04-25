/*
 * DevilCore OS — PIC (8259A) Driver
 */
#include "pic.h"
#include "../include/ports.h"

#define PIC1_CMD  0x20
#define PIC1_DAT  0x21
#define PIC2_CMD  0xA0
#define PIC2_DAT  0xA1

void pic_init(void) {
    outb(PIC1_CMD, 0x11); io_wait();
    outb(PIC2_CMD, 0x11); io_wait();
    outb(PIC1_DAT, 32);   io_wait();
    outb(PIC2_DAT, 40);   io_wait();
    outb(PIC1_DAT, 4);    io_wait();
    outb(PIC2_DAT, 2);    io_wait();
    outb(PIC1_DAT, 0x01); io_wait();
    outb(PIC2_DAT, 0x01); io_wait();
    outb(PIC1_DAT, 0b11111000); /* unmask IRQ 0,1,2 */
    outb(PIC2_DAT, 0b11101111); /* unmask IRQ 12 (mouse) */
}

void pic_eoi(uint32_t irq) {
    if (irq >= 40) outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}
