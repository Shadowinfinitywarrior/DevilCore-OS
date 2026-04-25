/*
 * DevilCore OS — Scheduler Header
 */
#ifndef DC_SCHEDULER_H
#define DC_SCHEDULER_H

#include "process.h"

void sched_init(thread_t *first_thread);
void sched_add_thread(thread_t *t);
thread_t *sched_get_current(void);
thread_t *sched_get_thread_queue(void);  /* Diagnostic helper */

/* Invoked by the timer interrupt to preempt the current thread */
void sched_tick(void);

/* Force yield the current thread */
void sched_yield(void);

/* Actual context switcher invoked internally */
extern void switch_context(uint8_t **old_sp, uint8_t *new_sp);

#endif /* DC_SCHEDULER_H */