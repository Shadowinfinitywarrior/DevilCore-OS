#ifndef DEVILCORE_CFS_SCHEDULER_H
#define DEVILCORE_CFS_SCHEDULER_H

#include <stdint.h>
#include "scheduler.h"

// CFS (Completely Fair Scheduler) enhancements
// Based on Linux CFS scheduling algorithm

// Scheduler weights (from Linux sched/prio.h)
// These map nice values (-20 to 19) to weights
#define CFS_MIN_NICE (-20)
#define CFS_MAX_NICE (19)
#define CFS_NICE_RANGE (40)

// Scheduler parameters
#define CFS_TARGET_LATENCY_MS (6)       // Target scheduling latency in ms
#define CFS_MIN_GRANULARITY_MS (0.75f)  // Minimum time slice in ms
#define CFS_WAKEUP_GRANULARITY_MS (1.0f)

// Virtual time scaling
#define CFS_NICE_0_LOAD (1024)
#define CFS_TICK_US (1000)  // 1ms tick

// CFS run queue node (red-black tree would be better, but we use sorted array)
typedef struct cfs_rq_node {
    struct task *task;
    uint64_t vruntime;
    struct cfs_rq_node *left;
    struct cfs_rq_node *right;
    struct cfs_rq_node *parent;
    uint8_t color;  // For RB tree: 0=red, 1=black
} cfs_rq_node_t;

// CFS run queue
typedef struct cfs_rq {
    cfs_rq_node_t *root;           // Root of RB tree
    cfs_rq_node_t *leftmost;       // Leftmost node (lowest vruntime)
    uint64_t min_vruntime;         // Minimum vruntime in queue
    uint64_t exec_clock;           // Execution clock
    uint32_t nr_running;           // Number of runnable tasks
    uint64_t load_weight;          // Total load weight
} cfs_rq_t;

// Task CFS statistics
typedef struct cfs_task_info {
    uint64_t vruntime;             // Virtual runtime
    uint64_t prev_sum_exec_runtime; // Previous execution time
    uint64_t exec_start;           // When task started executing
    uint64_t sum_exec_runtime;     // Total execution time
    uint64_t wait_start;           // When task started waiting
    uint64_t sleep_start;          // When task went to sleep
    uint32_t weight;               // Scheduling weight based on nice
    int32_t static_prio;           // Static priority (nice value)
    uint64_t deadline;             // For deadline scheduling
} cfs_task_info_t;

// CFS API
void cfs_init(void);
void cfs_enqueue_task(struct task *task);
void cfs_dequeue_task(struct task *task);
struct task *cfs_pick_next_task(cfs_rq_t *rq);
void cfs_update_task_stats(struct task *task, uint64_t delta_exec);
void cfs_update_min_vruntime(cfs_rq_t *rq);
uint64_t cfs_calc_delta_fair(uint64_t delta_exec, struct task *task);

// Priority to weight conversion
uint32_t cfs_prio_to_weight(int32_t nice);
uint32_t cfs_prio_to_wmult(int32_t nice);  // Inverse weight

// Time slice calculation
uint64_t cfs_sched_slice(struct task *task);
uint64_t cfs_sched_vslice(struct task *task);

// Virtual time calculation
uint64_t cfs_account_delta_exec(uint64_t delta_exec, uint32_t weight);
uint64_t cfs_calc_vruntime(uint64_t vruntime, uint64_t delta_exec, uint32_t weight);

// Task placement
void cfs_place_entity(cfs_rq_t *rq, struct task *task, uint64_t initial);
void cfs_post_init_entity(struct task *task);

// Statistics and debugging
void cfs_print_stats(void);
void cfs_print_rq(cfs_rq_t *rq);

// Global CFS run queue
extern cfs_rq_t global_cfs_rq;

#endif // DEVILCORE_CFS_SCHEDULER_H
