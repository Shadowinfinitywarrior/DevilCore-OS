/*
 * DevilCore OS — Simple Round-Robin Scheduler
 */

#include "scheduler.h"
#include "../include/types.h"

static thread_t *g_current_thread = NULL;
static thread_t *g_thread_queue = NULL;
static bool g_scheduler_enabled = false;

void sched_init(thread_t *first_thread) {
    g_current_thread = first_thread;
    g_thread_queue = first_thread;
    first_thread->next = first_thread; /* Circular linked list */
    g_scheduler_enabled = true;
}

void sched_add_thread(thread_t *t) {
    if (!g_thread_queue) {
        g_thread_queue = t;
        t->next = t;
        return;
    }
    
    thread_t *last = g_thread_queue;
    while (last->next != g_thread_queue) {
        last = last->next;
    }
    last->next = t;
    t->next = g_thread_queue;
}

thread_t *sched_get_current(void) {
    return g_current_thread;
}

/* Diagnostic accessor */
thread_t *sched_get_thread_queue(void) {
    return g_thread_queue;
}

void sched_yield(void) {
    if (!g_scheduler_enabled || !g_current_thread || g_current_thread->next == g_current_thread) {
        return;
    }

    thread_t *prev = g_current_thread;
    thread_t *next = prev->next;
    
    while (next->state != THREAD_READY && next->state != THREAD_RUNNING) {
        next = next->next;
        if (next == prev) return;
    }
    
    g_current_thread = next;
    
    if (prev->state == THREAD_RUNNING) {
        prev->state = THREAD_READY;
    }
    next->state = THREAD_RUNNING;
    
    switch_context(&prev->stack_ptr, next->stack_ptr);
}

void sched_tick(void) {
    sched_yield();
}
