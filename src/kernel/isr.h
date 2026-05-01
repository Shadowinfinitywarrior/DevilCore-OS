#ifndef DEVILCORE_ISR_H
#define DEVILCORE_ISR_H

#include <stdint.h>

struct registers {
    uint64_t rax, rcx, rdx, rbx, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, cs, rflags, rsp, ss;
    uint64_t vector;
    uint64_t error_code;
};

void irq_dispatch(uint64_t vector, uint64_t error_code);

void timer_tick(void);
void keyboard_read(void);

void interrupt_init(void);

#endif