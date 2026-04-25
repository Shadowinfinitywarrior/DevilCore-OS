/*
 * DevilCore OS — Process and Thread Management
 */
#include "process.h"
#include "../mm/heap.h"
#include "../mm/pmm.h"
#include "../libc/string.h"
#include "scheduler.h"

#define DEFAULT_STACK_SIZE 32768 /* 32KB stack */

static int g_next_pid = 1;
static int g_next_tid = 1;

void proc_init(void) {
    /* Create the idle thread or main kernel thread acting as root */
    thread_t *root_thread = (thread_t*)kmalloc(sizeof(thread_t));
    memset(root_thread, 0, sizeof(thread_t));
    
    root_thread->tid = 0; /* TID 0 is the initial boot CPU thread */
    root_thread->pid = 0;
    root_thread->state = THREAD_RUNNING;
    /* stack_ptr is already loaded in RSP from assembly */
    
    memcpy(root_thread->name, "kernel_main", 12);
    
    sched_init(root_thread);
}

/* External assembly stub that jumps into the C function and calls thread_exit when done */
extern void thread_start_stub(void);

/* The actual function the stub calls */
void thread_entry(void (*entry)(void*), void *arg) {
    entry(arg);
    /* In the future: call thread_exit() to clean up */
    while(1) { __asm__ volatile("hlt"); }
}

thread_t *thread_create(void (*entry_point)(void*), void *arg, const char *name) {
    thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
    if (!t) return NULL;
    
    memset(t, 0, sizeof(thread_t));
    t->tid = g_next_tid++;
    t->pid = 0; /* Kernel process */
    t->state = THREAD_READY;
    t->stack_size = DEFAULT_STACK_SIZE;
    t->stack_base = (uint64_t)kmalloc(DEFAULT_STACK_SIZE);
    
    if (name) {
        int i = 0;
        while (name[i] && i < 31) { t->name[i] = name[i]; i++; }
        t->name[i] = 0;
    }
    
    /* Set up initial stack frame for context switch */
    /* Stack grows downwards */
    uint64_t *stack = (uint64_t *)(t->stack_base + t->stack_size);
    
    /* We mimic the stack layout expected by 'switch_context' and 'thread_start_stub' */
    
    *(--stack) = (uint64_t)arg;           /* Arg mapped to R13 */
    *(--stack) = (uint64_t)entry_point;   /* Entry mapped to R12 */
    
    *(--stack) = 0; /* R15 */
    *(--stack) = 0; /* R14 */
    *(--stack) = (uint64_t)arg;         /* R13 = argument for thread_entry */
    *(--stack) = (uint64_t)entry_point; /* R12 = function pointer */
    *(--stack) = 0; /* RBX */
    *(--stack) = 0; /* RBP */
    
    /* RIP pointing to the assembly stub */
    *(--stack) = (uint64_t)thread_start_stub;
    
    t->stack_ptr = (uint8_t *)stack;
    
    sched_add_thread(t);
    return t;
}

thread_t *thread_current(void) {
    return sched_get_current();
}

void thread_yield(void) {
    /* Fire a software interrupt or call schedule directly. 
       For round-robin, we can just call sched_switch() directly. */
    sched_yield();
}