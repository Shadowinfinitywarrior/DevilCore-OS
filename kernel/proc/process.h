/*
 * DevilCore OS — Process and Thread Definitions
 */
#ifndef DC_PROCESS_H
#define DC_PROCESS_H

#include "../include/types.h"

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

/* Architecture-specific saved execution context */
typedef struct {
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
    uint64_t rip;
    /* Other volatile registers are saved by the ABI / interrupt caller */
} context_t;

/* Thread Control Block (TCB) */
typedef struct thread {
    int tid;
    int pid;
    thread_state_t state;
    
    uint64_t stack_base;
    uint64_t stack_size;
    uint8_t *stack_ptr; /* Points to the top of the stack during context switch */
    
    char name[32];
    
    struct thread *next;
} thread_t;

/* Process Control Block (PCB) */
typedef struct process {
    int pid;
    uint64_t cr3; /* Page Directory Root (PML4) */
    char name[32];
    thread_t *main_thread;
    
    struct process *next;
} process_t;

/* Initialize process subsystem */
void proc_init(void);

/* Create a new kernel thread */
thread_t *thread_create(void (*entry_point)(void*), void *arg, const char *name);

/* Get current executing thread */
thread_t *thread_current(void);

/* Yield execution to another thread */
void thread_yield(void);

#endif /* DC_PROCESS_H */