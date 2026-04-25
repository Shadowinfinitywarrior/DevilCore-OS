// kernel/proc/thread.c
// Thread Management

#include <devil/types.h>
#include <devil/config.h>
#include "thread.h"
#include "process.h"

#define MAX_THREADS 16384

static thread_t threads[MAX_THREADS];
static thread_t *current_thread;
static uint64_t next_tid = 1;

thread_t *thread_create(void (*start_routine)(void *), void *arg) {
    thread_t *thread = NULL;
    
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].state == TASK_STOPPED) {
            thread = &threads[i];
            break;
        }
    }
    
    if (!thread) return NULL;
    
    thread->tid = next_tid++;
    thread->start_routine = start_routine;
    thread->arg = arg;
    thread->stack_size = KERNEL_STACK_SIZE;
    thread->state = TASK_READY;
    thread->priority = 0;
    thread->task = get_current_task();
    
    // Allocate stack
    extern void *pmm_alloc(void);
    thread->stack = (uint64_t)pmm_alloc();
    
    return thread;
}

void thread_exit(void *retval) {
    if (current_thread) {
        current_thread->state = TASK_ZOMBIE;
    }
    extern void schedule(void);
    schedule();
}

int thread_join(thread_t *tid, void **retval) {
    while (tid->state != TASK_ZOMBIE) {
        // Wait
        extern void schedule(void);
        schedule();
    }
    
    if (retval && tid->arg) {
        *retval = tid->arg;
    }
    
    tid->state = TASK_STOPPED;
    return 0;
}

void thread_start(thread_t *thread) {
    current_thread = thread;
    thread->state = TASK_RUNNING;
    thread->start_routine(thread->arg);
}