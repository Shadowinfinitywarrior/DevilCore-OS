#include "scheduler.h"
#include "memory.h"
#include "vma.h"
#include "slab.h"
#include "spinlock.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);

struct task *current_task;
struct task *idle_task;
struct schedule_queue run_queue;
struct schedule_queue sleep_queue;

static spinlock_t sched_lock;

static struct task tasks[MAX_TASKS];
static uint32_t task_count;
static pid_t next_pid = 1;

static void task_setup_kernel_stack(struct task *task, void (*entry)(void)) {
    uint64_t *stack = (uint64_t *)((uint64_t)task->kernel_stack + KERNEL_STACK_SIZE);
    
    // Initial stack setup - must match context_switch_asm pop order:
    // popfq, pop rax, rbx, rcx, rdx, rsi, rdi, rbp, r8-r15, then ret
    
    // Return address for 'ret'
    *--stack = (uint64_t)entry;
    
    // Push registers in reverse order of pops (r15 first, rflags last)
    *--stack = 0;     // r15
    *--stack = 0;     // r14
    *--stack = 0;     // r13
    *--stack = 0;     // r12
    *--stack = 0;     // r11
    *--stack = 0;     // r10
    *--stack = 0;     // r9
    *--stack = 0;     // r8
    *--stack = 0;     // rbp
    *--stack = 0;     // rdi
    *--stack = 0;     // rsi
    *--stack = 0;     // rdx
    *--stack = 0;     // rcx
    *--stack = 0;     // rbx
    *--stack = 0;     // rax
    *--stack = 0x202; // rflags (IF bit set)
    
    task->rsp = (uint64_t)stack;
    task->rip = (uint64_t)entry;
}

static void task_setup_user_stack(struct task *task, void (*entry)(void)) {
    uint64_t *stack = (uint64_t *)((uint64_t)task->user_stack + USER_STACK_SIZE);
    
    // IRET stack frame for Ring 3
    *--stack = 0x23;            // SS (User Data Selector | 3)
    *--stack = (uint64_t)task->user_stack + USER_STACK_SIZE; // RSP
    *--stack = 0x202;           // RFLAGS (IF bit set)
    *--stack = 0x1B;            // CS (User Code Selector | 3)
    *--stack = (uint64_t)entry; // RIP
    
    // Context switch expects registers as well
    *--stack = 0;     // r15
    *--stack = 0;     // r14
    *--stack = 0;     // r13
    *--stack = 0;     // r12
    *--stack = 0;     // r11
    *--stack = 0;     // r10
    *--stack = 0;     // r9
    *--stack = 0;     // r8
    *--stack = 0;     // rbp
    *--stack = 0;     // rdi
    *--stack = 0;     // rsi
    *--stack = 0;     // rdx
    *--stack = 0;     // rcx
    *--stack = 0;     // rbx
    *--stack = 0;     // rax
    *--stack = 0x202; // rflags dummy (popfq)

    task->rsp = (uint64_t)stack;
    task->rip = (uint64_t)entry;
}

extern void tss_set_rsp0(uint64_t rsp0);

extern void context_switch_asm(struct task *old_task, struct task *new_task);
static void context_switch(struct task *old_task, struct task *new_task) {
    context_switch_asm(old_task, new_task);
}

static uint64_t kernel_cr3;

static void idle_loop(void) {
    serial_write_string("Idle task running!\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void scheduler_init(void) {
    memset(&tasks, 0, sizeof(tasks));
    memset(&run_queue, 0, sizeof(run_queue));
    memset(&sleep_queue, 0, sizeof(sleep_queue));
    
    spin_init(&sched_lock);
    
    task_count = 0;
    next_pid = 1;
    current_task = NULL;
    
    // Save kernel CR3
    __asm__ volatile ("mov %%cr3, %0" : "=r"(kernel_cr3));
    serial_write_string("[DBG] kernel_cr3: 0x");
    serial_write_hex_u64(kernel_cr3);
    serial_write_string("\n");

    // Map virtual address 0x0 to a physical page
    void *page_for_zero = pmm_alloc_page();
    if (page_for_zero == NULL) {
        serial_write_string("ERROR: Failed to allocate page for virtual address 0x0\n");
        // Halt the system if allocation fails
        for (;;) {
            __asm__ volatile ("hlt");
        }
    }
    // Map as present, read-only for kernel.
    paging_map_page(0x0, (uint64_t)(uintptr_t)page_for_zero, PAGE_PRESENT);
    serial_write_string("[DBG] Mapped virtual 0x0 to physical 0x");
    serial_write_hex_u64((uint64_t)(uintptr_t)page_for_zero);
    serial_write_string("\n");
    
    idle_task = &tasks[0];
    idle_task->pid = 0;
    strncpy(idle_task->name, "idle", 63);
    idle_task->name[63] = '\0';
    idle_task->flags = TASK_FLAG_KERNEL;
    idle_task->state = TASK_READY;
    
    // Initialize memory management structure
    idle_task->mm = kmalloc_slab(sizeof(struct mm_struct));
    if (idle_task->mm) {
        mm_init(idle_task->mm);
    }
    
    idle_task->kernel_stack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
    if (idle_task->mm) {
        mm_add_vma(idle_task->mm, idle_task->kernel_stack, KERNEL_STACK_SIZE, VMA_READ | VMA_WRITE);
    }
    idle_task->user_stack = 0;
    serial_write_string("[DBG] idle kernel_stack: 0x");
    serial_write_hex_u64(idle_task->kernel_stack);
    serial_write_string("\n");
    // TEST: Use kernel_cr3 to see if idle task crashes due to missing kernel mapping
    idle_task->cr3 = kernel_cr3; 
    serial_write_string("[DBG] idle_task->cr3: 0x");
    serial_write_hex_u64(idle_task->cr3);
    serial_write_string("\n");
    task_setup_kernel_stack(idle_task, idle_loop);
    serial_write_string("[DBG] idle_task->rsp: 0x");
    serial_write_hex_u64(idle_task->rsp);
    serial_write_string(" rip: 0x");
    serial_write_hex_u64(idle_task->rip);
    serial_write_string("\n");
    idle_task->time_slice = 10;
    
    task_count = 1;
    scheduler_add_task(idle_task);
}

void scheduler_start(void) {
    serial_write_string("scheduler_start: Checking run_queue...\n");
    if (run_queue.head == NULL) {
        serial_write_string("scheduler_start: No tasks, switching to idle.\n");
        // Debug: print current state
        uint64_t cr3, rsp, rip;
        __asm__ volatile ("mov %%cr3, %0; mov %%rsp, %1; lea 1f, %2; 1:" : "=r"(cr3), "=r"(rsp), "=r"(rip));
        serial_write_string("[DBG] Pre-switch: CR3=0x");
        serial_write_hex_u64(cr3);
        serial_write_string(" RSP=0x");
        serial_write_hex_u64(rsp);
        serial_write_string(" RIP=0x");
        serial_write_hex_u64(rip);
        serial_write_string("\n");
        // Debug: print idle task state
        serial_write_string("[DBG] Idle task: CR3=0x");
        serial_write_hex_u64(idle_task->cr3);
        serial_write_string(" RSP=0x");
        serial_write_hex_u64(idle_task->rsp);
        serial_write_string(" RIP=0x");
        serial_write_hex_u64(idle_task->rip);
        serial_write_string("\n");
        
        current_task = idle_task;
        idle_task->state = TASK_RUNNING;
        context_switch(NULL, idle_task);
    } else {
        serial_write_string("scheduler_start: Found task, switching.\n");
        struct task *next_task = run_queue.head;
        run_queue.head = next_task->next;
        if (run_queue.head == NULL) {
            run_queue.tail = NULL;
        }
        run_queue.count--;
        
        context_switch(NULL, next_task);
    }
    
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

pid_t task_create(const char *name, void (*entry)(void), uint32_t flags) {
    if (task_count >= MAX_TASKS) {
        return -1;
    }
    
    struct task *task = &tasks[task_count++];
    
    task->pid = next_pid++;
    strncpy(task->name, name, 63);
    task->name[63] = '\0';
    task->flags = flags;
    task->state = TASK_READY;
    task->exit_code = 0;
    
    // Initialize memory management structure
    task->mm = kmalloc_slab(sizeof(struct mm_struct));
    if (task->mm) {
        mm_init(task->mm);
    }
    
    if (flags & TASK_FLAG_KERNEL) {
        task->kernel_stack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
        if (task->kernel_stack == 0) {
            return -1;
        }
        if (task->mm) {
            mm_add_vma(task->mm, task->kernel_stack, KERNEL_STACK_SIZE, VMA_READ | VMA_WRITE);
        }
        task_setup_kernel_stack(task, entry);
        task->cr3 = kernel_cr3;
    } else {
        task->kernel_stack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
        if (task->kernel_stack == 0) {
            return -1;
        }
        task->user_stack = (uint64_t)kmalloc(USER_STACK_SIZE);
        if (task->user_stack == 0) {
            kfree((void *)task->kernel_stack);
            return -1;
        }
        if (task->mm) {
            mm_add_vma(task->mm, task->user_stack, USER_STACK_SIZE, VMA_READ | VMA_WRITE | VMA_USER | VMA_STACK);
        }
        task_setup_user_stack(task, entry);
        
        task->cr3 = (uint64_t)pmm_alloc_page();
        if (task->cr3 == 0) {
            kfree((void *)task->kernel_stack);
            kfree((void *)task->user_stack);
            return -1;
        }
        zero_page(phys_to_virt(task->cr3));
        
        // Map kernel into user page table
        uint64_t *pml4_user = (uint64_t *)phys_to_virt(task->cr3);
        uint64_t *pml4_kernel = (uint64_t *)phys_to_virt(kernel_cr3);
        for (int i = 256; i < 512; ++i) {
            pml4_user[i] = pml4_kernel[i];
        }
    }
    
    task->time_slice = 100;
    task->cpu_time = 0;
    task->start_time = 0;
    
    task->parent = current_task;
    task->sibling = NULL;
    task->child = NULL;
    task->next = NULL;
    
    if (current_task != NULL) {
        task->sibling = current_task->child;
        current_task->child = task;
    }
    
    scheduler_add_task(task);
    
    return task->pid;
}

void task_exit(int32_t status) {
    if (current_task == NULL) {
        return;
    }
    
    current_task->exit_code = status;
    current_task->state = TASK_ZOMBIE;
    current_task->flags |= TASK_FLAG_EXITED;
    
    if (current_task->child != NULL) {
        struct task *child = current_task->child;
        while (child != NULL) {
            if (child->state != TASK_ZOMBIE) {
                child->parent = idle_task;
            }
            child = child->sibling;
        }
    }
    
    scheduler_remove_task(current_task);
    scheduler_yield();
}

pid_t task_wait(pid_t pid, int32_t *status) {
    (void)pid;
    (void)status;
    
    current_task->flags |= TASK_FLAG_WAIT;
    current_task->state = TASK_BLOCKED;
    scheduler_yield();
    
    if (status != NULL && current_task != NULL) {
        *status = current_task->exit_code;
    }
    
    return 0;
}

int task_kill(pid_t pid, int signal) {
    (void)signal;
    
    if (pid <= 0 || pid >= (pid_t)task_count) {
        return -1;
    }
    
    struct task *task = &tasks[pid];
    task->state = TASK_STOPPED;
    
    return 0;
}

pid_t task_getpid(void) {
    return current_task != NULL ? current_task->pid : 0;
}

struct task *task_get_current(void) {
    return current_task;
}

int scheduler_add_task(struct task *task) {
    if (task == NULL || task->state != TASK_READY) {
        return -1;
    }
    
    spin_lock(&sched_lock);
    if (run_queue.tail == NULL) {
        run_queue.head = task;
        run_queue.tail = task;
    } else {
        run_queue.tail->next = task;
        run_queue.tail = task;
    }
    
    task->next = NULL;
    run_queue.count++;
    spin_unlock(&sched_lock);
    
    return 0;
}

int scheduler_remove_task(struct task *task) {
    if (task == NULL) {
        return -1;
    }
    
    spin_lock(&sched_lock);
    struct task *prev = NULL;
    struct task *current = run_queue.head;
    
    while (current != NULL) {
        if (current == task) {
            if (prev == NULL) {
                run_queue.head = task->next;
            } else {
                prev->next = task->next;
            }
            
            if (run_queue.tail == task) {
                run_queue.tail = prev;
            }
            
            run_queue.count--;
            task->next = NULL;
            spin_unlock(&sched_lock);
            return 0;
        }
        
        prev = current;
        current = current->next;
    }
    
    spin_unlock(&sched_lock);
    return -1;
}

void scheduler_schedule(void) {
    spin_lock(&sched_lock);
    if (run_queue.head == NULL) {
        spin_unlock(&sched_lock);
        return;
    }
    
    struct task *prev_task = current_task;
    struct task *next_task = run_queue.head;
    
    run_queue.head = next_task->next;
    if (run_queue.head == NULL) {
        run_queue.tail = NULL;
    }
    run_queue.count--;
    
    next_task->next = NULL;
    spin_unlock(&sched_lock);
    
    context_switch(prev_task, next_task);
}

void scheduler_tick(void) {
    if (current_task == NULL || current_task == idle_task) {
        scheduler_yield();
        return;
    }
    
    if (current_task->time_slice > 0) {
        current_task->time_slice--;
    }
    
    if (current_task->time_slice == 0) {
        current_task->time_slice = 10; // Reset slice
        scheduler_yield();
    }
}

void scheduler_yield(void) {
    __asm__ volatile ("int $0x81");
}

void scheduler_switch(struct task *task) {
    if (task == NULL || current_task == NULL) {
        return;
    }
    
    context_switch(current_task, task);
}

void task_sleep(uint64_t milliseconds) {
    if (current_task == NULL) {
        return;
    }
    
    current_task->sleep_time = milliseconds;
    current_task->state = TASK_SLEEPING;
    scheduler_remove_task(current_task);
    scheduler_yield();
}

void task_wake(struct task *task) {
    if (task == NULL || task->state != TASK_SLEEPING) {
        return;
    }
    
    task->state = TASK_READY;
    scheduler_add_task(task);
}

int task_clone(struct task *task) {
    (void)task;
    return -1;
}