#ifndef DEVILCORE_ISR_H
#define DEVILCORE_ISR_H

#include <stdint.h>

struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

void irq_dispatch(uint64_t vector, uint64_t error_code, struct registers *regs);

void timer_tick(void);
void keyboard_read(void);

void interrupt_init(void);

#endif