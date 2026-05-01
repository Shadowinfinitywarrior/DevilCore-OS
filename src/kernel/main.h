#ifndef DEVILCORE_MAIN_H
#define DEVILCORE_MAIN_H

#include <stdint.h>

inline void io_wait(void) {
    outb(0x80, 0);
}

inline void outb(uint8_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

inline uint8_t inb(uint8_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

inline void sti(void) {
    __asm__ volatile ("sti");
}

inline void cli(void) {
    __asm__ volatile ("cli");
}

struct registers {
    uint64_t rax, rcx, rdx, rbx, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, cs, rflags, rsp, ss;
};

void panic(struct registers *regs);

#endif