#include "isr.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"

#include <stddef.h>
#include <stdint.h>

extern void *isr_stub_table[];
extern void *irq_stub_table[];

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);
extern void serial_write_dec(uint64_t value);

#include "scheduler.h"

void irq_dispatch(uint64_t vector, uint64_t error_code) {
    if (vector < 32) {
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
    
    // Register yield interrupt (DPL 3 so user mode can call it)
    idt_set_gate(129, irq_stub_table[129 - 32], 3);
}