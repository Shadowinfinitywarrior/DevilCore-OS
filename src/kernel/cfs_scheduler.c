#include "cfs_scheduler.h"
#include "memory.h"
#include "slab.h"
#include "timer.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);

// Scheduler weights (from Linux)
// Maps nice values (-20 to 19) to weights
static const uint32_t sched_prio_to_weight[40] = {
    // -20 to -15
    88761, 71755, 56483, 46273, 36291,
    // -15 to -10
    29154, 23254, 18705, 14949, 11916,
    // -10 to -5
    9548, 7620, 6100, 4904, 3906,
    // -5 to 0
    3121, 2501, 1991, 1586, 1277,
    // 0 to 5
    1024, 820, 655, 526, 423,
    // 5 to 10
    335, 272, 215, 172, 137,
    // 10 to 15
    110, 87, 70, 56, 45,
    // 15 to 19
    36, 29, 23, 18, 15,
};

// Inverse weights for faster division (not used in simple implementation)
static const uint32_t sched_prio_to_wmult[40] __attribute__((unused)) = {
    // Simplified - would need actual inverse weights
    // For now we just do division
};

cfs_rq_t global_cfs_rq;
static uint8_t cfs_initialized = 0;

void cfs_init(void) {
    if (cfs_initialized) return;
    
    kprintf("Initializing CFS scheduler...\n");
    
    memset(&global_cfs_rq, 0, sizeof(cfs_rq_t));
    global_cfs_rq.min_vruntime = 0;
    global_cfs_rq.exec_clock = 0;
    global_cfs_rq.nr_running = 0;
    global_cfs_rq.load_weight = 0;
    
    cfs_initialized = 1;
    
    kprintf("CFS scheduler initialized\n");
}

uint32_t cfs_prio_to_weight(int32_t nice) {
    // Clamp to valid range
    if (nice < CFS_MIN_NICE) nice = CFS_MIN_NICE;
    if (nice > CFS_MAX_NICE) nice = CFS_MAX_NICE;
    
    return sched_prio_to_weight[nice - CFS_MIN_NICE];
}

uint32_t cfs_prio_to_wmult(int32_t nice) {
    (void)nice;
    // Return inverse weight for multiplication instead of division
    // Simplified - returns 1/weight approximations
    return 1;  // Placeholder
}

// Red-Black tree implementation for CFS
static void rb_rotate_left(cfs_rq_t *rq, cfs_rq_node_t *x) {
    cfs_rq_node_t *y = x->right;
    x->right = y->left;
    if (y->left) y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent) rq->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void rb_rotate_right(cfs_rq_t *rq, cfs_rq_node_t *y) {
    cfs_rq_node_t *x = y->left;
    y->left = x->right;
    if (x->right) x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent) rq->root = x;
    else if (y == y->parent->right) y->parent->right = x;
    else y->parent->left = x;
    x->right = y;
    y->parent = x;
}

static void rb_insert_fixup(cfs_rq_t *rq, cfs_rq_node_t *z) {
    while (z->parent && z->parent->color == 0) { // Parent is Red
        if (z->parent == z->parent->parent->left) {
            cfs_rq_node_t *y = z->parent->parent->right;
            if (y && y->color == 0) { // Uncle is Red
                z->parent->color = 1;
                y->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rb_rotate_left(rq, z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                rb_rotate_right(rq, z->parent->parent);
            }
        } else {
            cfs_rq_node_t *y = z->parent->parent->left;
            if (y && y->color == 0) { // Uncle is Red
                z->parent->color = 1;
                y->color = 1;
                z->parent->parent->color = 0;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rb_rotate_right(rq, z);
                }
                z->parent->color = 1;
                z->parent->parent->color = 0;
                rb_rotate_left(rq, z->parent->parent);
            }
        }
    }
    rq->root->color = 1;
}

static void cfs_rb_insert(cfs_rq_t *rq, struct task *task, uint64_t vruntime) {
    cfs_rq_node_t *z = slab_alloc(cache_64);
    if (!z) return;
    
    memset(z, 0, sizeof(cfs_rq_node_t));
    z->task = task;
    z->vruntime = vruntime;
    z->color = 0; // Red
    
    cfs_rq_node_t *y = NULL;
    cfs_rq_node_t *x = rq->root;
    
    while (x) {
        y = x;
        if (z->vruntime < x->vruntime) x = x->left;
        else x = x->right;
    }
    
    z->parent = y;
    if (!y) {
        rq->root = z;
        rq->leftmost = z;
    } else if (z->vruntime < y->vruntime) {
        y->left = z;
        if (y == rq->leftmost) rq->leftmost = z;
    } else {
        y->right = z;
    }
    
    rb_insert_fixup(rq, z);
}

// Simplified pick next using leftmost node
struct task *cfs_pick_next_task(cfs_rq_t *rq) {
    if (!rq || !rq->leftmost) return NULL;
    
    struct task *task = rq->leftmost->task;
    
    cfs_task_info_t *info = (cfs_task_info_t *)task->sched_data;
    if (info) {
        info->exec_start = timer_ticks() * 1000;
    }
    
    return task;
}

void cfs_enqueue_task(struct task *task) {
    if (!task || !cfs_initialized) return;
    
    cfs_task_info_t *info = (cfs_task_info_t *)task->sched_data;
    if (!info) {
        cfs_post_init_entity(task);
        info = (cfs_task_info_t *)task->sched_data;
    }
    
    cfs_place_entity(&global_cfs_rq, task, 0);
    
    global_cfs_rq.nr_running++;
    global_cfs_rq.load_weight += info->weight;
}

void cfs_place_entity(cfs_rq_t *rq, struct task *task, uint64_t initial) {
    if (!rq || !task) return;
    
    cfs_task_info_t *info = (cfs_task_info_t *)task->sched_data;
    if (!info) return;
    
    uint64_t vruntime = info->vruntime;
    
    if (initial) {
        vruntime = rq->min_vruntime;
    } else {
        if (vruntime < rq->min_vruntime) {
            vruntime = rq->min_vruntime;
        }
    }
    
    info->vruntime = vruntime;
    cfs_rb_insert(rq, task, vruntime);
}

void cfs_post_init_entity(struct task *task) {
    if (!task) return;
    
    cfs_task_info_t *info = slab_alloc(cache_64);
    if (!info) return;
    
    memset(info, 0, sizeof(cfs_task_info_t));
    info->static_prio = 0;  // Default nice value
    info->weight = cfs_prio_to_weight(0);
    info->vruntime = global_cfs_rq.min_vruntime;
    
    task->sched_data = info;
}

// Set task priority (nice value)
void cfs_set_priority(struct task *task, int32_t nice) {
    if (!task || nice < CFS_MIN_NICE || nice > CFS_MAX_NICE) return;
    
    cfs_task_info_t *info = (cfs_task_info_t *)task->sched_data;
    if (!info) {
        cfs_post_init_entity(task);
        info = (cfs_task_info_t *)task->sched_data;
    }
    
    if (!info) return;
    
    // Update load weight
    uint32_t old_weight = info->weight;
    info->static_prio = nice;
    info->weight = cfs_prio_to_weight(nice);
    
    global_cfs_rq.load_weight = global_cfs_rq.load_weight - old_weight + info->weight;
}

void cfs_print_stats(void) {
    kprintf("=== CFS Statistics ===\n");
    kprintf("Tasks running: %u\n", global_cfs_rq.nr_running);
    kprintf("Load weight: %u\n", global_cfs_rq.load_weight);
    kprintf("Min vruntime: %lu\n", global_cfs_rq.min_vruntime);
    kprintf("Exec clock: %lu\n", global_cfs_rq.exec_clock);
}

static void cfs_print_node(cfs_rq_node_t *node, uint32_t level) {
    if (!node || level > 5) return; // Limit depth for safety
    
    cfs_print_node(node->left, level + 1);
    
    cfs_task_info_t *info = (cfs_task_info_t *)node->task->sched_data;
    kprintf("  %s: vruntime=%lu, weight=%u, color=%s\n",
            node->task->name, node->vruntime, 
            info ? info->weight : 0,
            node->color == 0 ? "RED" : "BLACK");
            
    cfs_print_node(node->right, level + 1);
}

void cfs_print_rq(cfs_rq_t *rq) {
    if (!rq) return;
    
    kprintf("CFS Run Queue:\n");
    kprintf("  Tasks: %u, Load: %u\n", rq->nr_running, rq->load_weight);
    
    if (rq->root) {
        cfs_print_node(rq->root, 0);
    }
}
