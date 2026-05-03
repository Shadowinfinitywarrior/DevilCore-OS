#include "isr.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"
#include "vma.h"
#include "io.h"

#include <stddef.h>
#include <stdint.h>

extern void *isr_stub_table[];
extern void *irq_stub_table[];

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);
extern void serial_write_dec(uint64_t value);

#include "scheduler.h"
#include "syscall.h"

void irq_dispatch(uint64_t vector, uint64_t error_code, struct registers *regs) {
    if (vector < 32) {
        if (vector == 14) {
            uint64_t cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            if (handle_page_fault(cr2, error_code) == 0) {
                return;
            }
        }
        serial_write_string("EXCEPTION: ");
        serial_write_dec(vector);
        serial_write_string(" (err: ");
        serial_write_hex_u64(error_code);
        serial_write_string(")\n");
        if (vector == 14) {
            uint64_t cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            serial_write_string("CR2: ");
            serial_write_hex_u64(cr2);
            serial_write_string("\n");
        }
        __asm__ volatile ("cli");
        for (;;) {
            __asm__ volatile ("hlt");
        }
    }

    if (vector >= 32 && vector <= 47) {
        pic_end_of_interrupt((uint8_t)(vector - 32));
    }

    if (vector == 32) {
        timer_tick();
    } else if (vector == 33) {
        keyboard_interrupt();
    } else if (vector == 44) {
        mouse_interrupt();
    } else if (vector == 129) { // 0x81
        scheduler_schedule();
    } else if (vector == 128) { // 0x80 - Syscall
        regs->rax = (uint64_t)syscall_handle(regs->rax, regs->rdi, regs->rsi, regs->rdx, regs->r10, regs);
    }
}

void interrupt_init(void) {
    idt_init();

    for (size_t i = 0; i < 32; ++i) {
        idt_set_gate((uint8_t)i, isr_stub_table[i], 0);
    }

    for (size_t i = 0; i < 16; ++i) {
        idt_set_gate((uint8_t)(i + 32), irq_stub_table[i], 0);
    }

    pic_init();

    timer_init(100);
    keyboard_init();
    mouse_init();

    // Flush PS/2 buffer to avoid stuck edge-triggered interrupts
    while (inb(0x64) & 1) {
        inb(0x60);
    }

    // Register yield interrupt (DPL 3 so user mode can call it)
    idt_set_gate(129, irq_stub_table[129 - 32], 3);
    
    // Register syscall interrupt (0x80)
    idt_set_gate(128, irq_stub_table[128 - 32], 3);
}