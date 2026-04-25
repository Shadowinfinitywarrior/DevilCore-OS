/*
 * DevilCore OS — System Call C Handler
 */
#include "syscall.h"
#include "../proc/scheduler.h"

/* MSR definitions */
#define IA32_STAR  0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084
#define EFER       0xC0000080

extern void syscall_entry(void);

typedef struct {
    uint64_t rax, rdi, rsi, rdx, r10, r8, r9;
} syscall_regs_t;

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void syscall_init(void) {
    /* Enable SCE (System Call Extensions) in EFER */
    uint64_t efer = rdmsr(EFER);
    wrmsr(EFER, efer | 1);
    
    /* Set up STAR: [63:48] User CS/SS, [47:32] Kernel CS/SS */
    /* Assuming Kernel CS = 0x08, User CS = 0x1B (just standard values for later) */
    uint64_t star = ((0x1BULL << 48) | (0x08ULL << 32));
    wrmsr(IA32_STAR, star);
    
    /* Set up LSTAR with the entry point */
    wrmsr(IA32_LSTAR, (uint64_t)syscall_entry);
    
    /* Mask out interrupts during syscall execution */
    wrmsr(IA32_FMASK, 0x200); /* Disable interrupts (IF bit) */
}

/* Returning uint64_t allows error codes or pointers */
uint64_t syscall_handler(syscall_regs_t *regs) {
    uint64_t ret = 0;
    
    switch (regs->rax) {
        case SYS_WRITE:
            /* Basic string output implementation for testing */
            /* Later this connects to VFS and Terminal driver */
            break;
            
        case SYS_YIELD:
            sched_yield();
            break;
            
        case SYS_EXIT:
            /* thread_exit(); */
            while(1) { __asm__ volatile("hlt"); }
            break;
            
        default:
            /* Unhandled syscall */
            ret = (uint64_t)-1;
            break;
    }
    
    return ret;
}
