#ifndef _THREAD_H
#define _THREAD_H

#include <devil/types.h>
#include <devil/config.h>
#include "process.h"

typedef struct thread {
    uint64_t   tid;
    task_t    *task;
    void      (*start_routine)(void *);
    void      *arg;
    uint64_t   stack;
    uint64_t   stack_size;
    uint8_t   state;
    uint8_t   priority;
} thread_t;

thread_t *thread_create(void (*start_routine)(void *), void *arg);
void thread_exit(void *retval);
int thread_join(thread_t *tid, void **retval);
void thread_start(thread_t *thread);

#endif // _THREAD_H