#ifndef DEVILCORE_SCHEDULER_H
#define DEVILCORE_SCHEDULER_H

#include <stdint.h>

#include "memory.h"

typedef int32_t pid_t;

#define MAX_TASKS    256
#define KERNEL_STACK_SIZE 16384
#define USER_STACK_SIZE  16384

#define TASK_RUNNING   0
#define TASK_READY     1
#define TASK_BLOCKED  2
#define TASK_SLEEPING 3
#define TASK_STOPPED  4
#define TASK_ZOMBIE   5

#define TASK_FLAG_KERNEL  0x01
#define TASK_FLAG_USER    0x02
#define TASK_FLAG_WAIT   0x04
#define TASK_FLAG_EXITED 0x08

struct task_context {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cr3;
};

struct task {
    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    pid_t sid;
    
    char name[64];
    uint32_t flags;
    uint32_t state;
    int32_t exit_code;
    
    struct task *parent;
    struct task *sibling;
    struct task *child;
    struct task *next;
    
    uint64_t kernel_stack;
    uint64_t user_stack;
    uint64_t rip;
    uint64_t rsp;
    uint64_t cr3;
    
    struct task_context context;
    
    uint64_t time_slice;
    uint64_t cpu_time;
    uint64_t start_time;
    uint64_t sleep_time;
    
    void *fd_table[32];
    int32_t exit_status;
    
    // For CFS scheduler data
    void *sched_data;
};

struct schedule_queue {
    struct task *head;
    struct task *tail;
    uint32_t count;
};

void scheduler_init(void);
void scheduler_start(void);
void scheduler_schedule(void);

pid_t task_create(const char *name, void (*entry)(void), uint32_t flags);
void task_exit(int32_t status);
pid_t task_wait(pid_t pid, int32_t *status);
int task_kill(pid_t pid, int signal);
pid_t task_getpid(void);
struct task *task_get_current(void);

int scheduler_add_task(struct task *task);
int scheduler_remove_task(struct task *task);
void scheduler_tick(void);
void scheduler_yield(void);
void scheduler_switch(struct task *task);

void task_sleep(uint64_t milliseconds);
void task_wake(struct task *task);
int task_clone(struct task *task);

extern struct task *current_task;
extern struct task *idle_task;
extern struct schedule_queue run_queue;
extern struct schedule_queue sleep_queue;

#endif