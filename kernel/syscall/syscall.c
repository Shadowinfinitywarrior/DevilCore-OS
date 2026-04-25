/*
 * DevilCore OS — System Call C Handler
 */
#include "syscall.h"
#include "../proc/scheduler.h"
#include "../proc/elf_loader.h"
#include "../fs/vfs.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../libc/string.h"

/* MSR definitions */
#define IA32_STAR  0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084
#define EFER       0xC0000080

extern void syscall_entry(void);

typedef struct {
    uint64_t rax, rdi, rsi, rdx, r10, r8, r9;
    uint64_t r15, r14, r13, r12, rbp, rbx;
    uint64_t r11, rcx;
    uint64_t padding;
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
            
        case SYS_EXECVE: {
            const char *path = (const char *)regs->rdi;
            vfs_node_t *file = vfs_lookup(path);
            if (!file) {
                ret = (uint64_t)-1;
                break;
            }

            /* Create isolated address space for the new program */
            uint64_t new_pml4 = vmm_create_address_space();
            if (!new_pml4) {
                ret = (uint64_t)-1;
                break;
            }

            /* Load into new space */
            /* NOTE: For now, we reuse current space for the 'copy' phase
             * or implement a proper kernel mapping. Simple hack: switch to new space,
             * load, and if it fails, we are in trouble. 
             * Better: Map new_pml4 pages into kernel space temporarily. */
            uint64_t entry = elf_load(file, new_pml4);
            if (!entry) {
                ret = (uint64_t)-1;
                break;
            }

            /* Transition thread to user mode (future: switch stacks) */
            /* For now, just switch the PML4 and set the RAX as entry for the caller to jump */
            vmm_switch_pml4(new_pml4);
            
            /* The syscall stub in ASM will return to user space with sysret.
             * We need to set the RIP (RCX) to the new entry point. */
            regs->rcx = entry;
            /* Reset stack pointer for User Space if necessary */
            /* regs->rsp = new_user_stack; */
            
            ret = 0; /* Success on execve? Usually doesn't 'return' on success */
            break;
        }

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
