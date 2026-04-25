/*
 * DevilCore OS — Process & Scheduling Diagnostics
 * Items 21-30
 */

#include "diag.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../proc/thread.h"
#include "../include/ports.h"
#include "../include/string.h"

/* ================================================================
 * LIST ALL RUNNING PROCESSES (Item 21)
 * ================================================================ */
static void diag_proc_list(diag_output_t *out) {
    diag_header(out, "Running Processes");

    /* Get process list from scheduler */
    diag_printf_user(out, "  PID   PPID  PRI  STATE     CPU  STACK    NAME\n");
    diag_printf_user(out, "  ----  ----  ---  --------  ---  --------  ------------------\n");

    process_t *proc = get_process_list();
    while (proc) {
        const char *state_str = "UNKNOWN";
        switch (proc->state) {
            case TASK_RUNNING:   state_str = "R"; break;
            case TASK_READY:     state_str = "S"; break;
            case TASK_BLOCKED:   state_str = "D"; break;
            case TASK_ZOMBIE:    state_str = "Z"; break;
            case TASK_SLEEPING:  state_str = "S"; break;
            case TASK_WAITING:   state_str = "W"; break;
        }

        diag_printf_user(out, "  %-5d %-5d %-3d  %-9s %3d  0x%08X  %s\n",
            proc->pid,
            proc->parent ? proc->parent->pid : 0,
            proc->priority,
            state_str,
            proc->cpu_usage / 1000, /* In thousandths of % */
            proc->kernel_stack_base,
            proc->name);

        proc = proc->next;
    }

    int total = count_processes();
    diag_printf_user(out, "\n  Total processes: %d\n", total);
}

/* ================================================================
 * SCHEDULER QUEUE AND TIMESLICES (Item 22)
 * ================================================================ */
static void diag_proc_sched_queue(diag_output_t *out) {
    diag_header(out, "Scheduler Queue and Timeslices");

    sched_stats_t stats;
    scheduler_get_stats(&stats);

    diag_printf_user(out, "  Scheduler: %s\n", stats.scheduler_name);
    diag_printf_user(out, "  Algorithm: %s\n", stats.algorithm);
    diag_printf_user(out, "  Timeslice: %u ms\n", stats.timeslice_ms);

    diag_printf_user(out, "\n  Run Queue (priority-ordered):\n");
    for (int prio = 0; prio < MAX_PRIORITY; prio++) {
        thread_t *t = get_threads_at_priority(prio);
        if (t) {
            diag_printf_user(out, "    Priority %d: ", prio);
            while (t) {
                diag_printf_user(out, "T%d ", t->tid);
                t = t->next;
            }
            diag_printf_user(out, "\n");
        }
    }

    diag_printf_user(out, "\n  Current Thread: T%d (%s)\n",
        stats.current_tid, stats.current_name);
    diag_printf_user(out, "  Next Sched: %u ms\n", stats.time_to_next_sched);
    diag_printf_user(out, "  Load Average: %.2f\n", stats.load_average);
}

/* ================================================================
 * CONTEXT SWITCH COUNT AND FREQUENCY (Item 23)
 * ================================================================ */
static void diag_proc_context_switches(diag_output_t *out) {
    diag_header(out, "Context Switch Statistics");

    cs_stats_t cs_stats;
    scheduler_get_cs_stats(&cs_stats);

    uint64_t uptime = get_uptime_seconds();
    double cs_per_sec = uptime ? (double)cs_stats.total_switches / (double)uptime : 0.0;

    diag_printf_user(out, "  Total Context Switches: %llu\n", cs_stats.total_switches);
    diag_printf_user(out, "  System Uptime: %llu seconds\n", uptime);
    diag_printf_user(out, "  Average CS Rate: %.2f switches/sec\n", cs_per_sec);
    diag_printf_user(out, "\n  Voluntary Switches: %llu\n", cs_stats.voluntary);
    diag_printf_user(out, "  Involuntary Switches: %llu\n", cs_stats.involuntary);
    diag_printf_user(out, "  Interrupt-induced: %llu\n", cs_stats.interrupt_cs);
    diag_printf_user(out, "  Syscall-induced: %llu\n", cs_stats.syscall_cs);

    diag_printf_user(out, "\n  Last CS Reason: %s\n", cs_stats.last_reason);
    diag_printf_user(out, "  CS Overhead (est): %.1f cycles/switch\n", cs_stats.avg_cycles_per_switch);
}

/* ================================================================
 * CPU USAGE PER PROCESS (Item 24)
 * ================================================================ */
static void diag_proc_cpu_usage(diag_output_t *out) {
    diag_header(out, "CPU Usage per Process");

    /* Get CPU time for each process */
    diag_printf_user(out, "  PID   %%CPU    USER    SYSTEM   TICKS    NAME\n");
    diag_printf_user(out, "  ----  -----   ------  -------  -------  ------------------\n");

    process_t *proc = get_process_list();
    while (proc) {
        uint64_t total_ticks = proc->user_ticks + proc->sys_ticks;
        double pct_user = total_ticks ? (double)proc->user_ticks * 100.0 / (double)total_ticks : 0.0;
        double pct_sys = total_ticks ? (double)proc->sys_ticks * 100.0 / (double)total_ticks : 0.0;
        double pct_total = (proc->cpu_usage / 1000.0); /* In % */

        diag_printf_user(out, "  %-5d %6.2f%%  %6.2f%% %7.2f%%  %8llu  %s\n",
            proc->pid,
            pct_total,
            pct_user,
            pct_sys,
            total_ticks,
            proc->name);
        proc = proc->next;
    }

    diag_printf_user(out, "\n  Load Average (1/5/15 min): %.2f / %.2f / %.2f\n",
        get_load_avg(0), get_load_avg(1), get_load_avg(2));
    diag_printf_user(out, "  Context Switches/min: %llu\n", get_cs_per_minute());
}

/* ================================================================
 * THREAD STATES (Item 25)
 * ================================================================ */
static void diag_proc_thread_states(diag_output_t *out) {
    diag_header(out, "Thread States");

    thread_state_counts_t state_counts;
    scheduler_get_thread_state_counts(&state_counts);

    int total = state_counts.running + state_counts.ready +
                state_counts.blocked + state_counts.sleeping +
                state_counts.zombie;

    diag_printf_user(out, "  Total Threads: %d\n", total);
    diag_printf_user(out, "\n  State Distribution:\n");
    diag_printf_user(out, "    Running:    %4d (%.1f%%)\n", state_counts.running,
        total ? (double)state_counts.running * 100.0 / total : 0.0);
    diag_printf_user(out, "    Ready:      %4d (%.1f%%)\n", state_counts.ready,
        total ? (double)state_counts.ready * 100.0 / total : 0.0);
    diag_printf_user(out, "    Blocked:    %4d (%.1f%%)\n", state_counts.blocked,
        total ? (double)state_counts.blocked * 100.0 / total : 0.0);
    diag_printf_user(out, "    Sleeping:   %4d (%.1f%%)\n", state_counts.sleeping,
        total ? (double)state_counts.sleeping * 100.0 / total : 0.0);
    diag_printf_user(out, "    Zombie:     %4d (%.1f%%)\n", state_counts.zombie,
        total ? (double)state_counts.zombie * 100.0 / total : 0.0);

    /* Detailed thread list */
    diag_printf_user(out, "\n  Thread Details:\n");
    diag_printf_user(out, "    TID   PID   STATE  PRI  STACK-PTR         NAME\n");
    diag_printf_user(out, "    ----  ---   -----  ---  ----------------  ------------------\n");

    thread_t *thread = get_thread_list();
    while (thread) {
        const char *state = "UNK";
        switch (thread->state) {
            case THREAD_RUNNING:  state = "RUN"; break;
            case THREAD_READY:    state = "RDY"; break;
            case THREAD_BLOCKED:  state = "BLK"; break;
            case THREAD_DEAD:     state = "DEAD"; break;
        }
        diag_printf_user(out, "    %-5d %-4d  %-5s  %-3d  0x%016llX  %s\n",
            thread->tid,
            thread->pid,
            state,
            thread->priority,
            thread->stack_ptr,
            thread->name);
        thread = thread->next;
    }
}

/* ================================================================
 * SCHEDULING LATENCY (Item 26)
 * ================================================================ */
static void diag_proc_sched_latency(diag_output_t *out) {
    diag_header(out, "Scheduling Latency");

    sched_latency_t latency;
    scheduler_get_latency_stats(&latency);

    diag_printf_user(out, "  Scheduler latency statistics:\n");
    diag_printf_user(out, "    Average latency: %u cycles (%.2f us)\n",
        latency.avg_cycles,
        latency.avg_cycles / 2500.0); /* Assuming 2.5 GHz, rough */
    diag_printf_user(out, "    Min latency: %u cycles\n", latency.min_cycles);
    diag_printf_user(out, "    Max latency: %u cycles (%.2f ms)\n",
        latency.max_cycles,
        latency.max_cycles / 2500000.0);
    diag_printf_user(out, "    Samples: %llu\n", latency.samples);

    diag_printf_user(out, "\n  Latency distribution:\n");
    diag_printf_user(out, "    < 1000 cycles:   %llu\n", latency.dist[0]);
    diag_printf_user(out, "    1K-10K cycles:   %llu\n", latency.dist[1]);
    diag_printf_user(out, "    10K-100K cycles: %llu\n", latency.dist[2]);
    diag_printf_user(out, "    > 100K cycles:   %llu\n", latency.dist[3]);
}

/* ================================================================
 * BLOCKED PROCESSES AND WAIT REASONS (Item 27)
 * ================================================================ */
static void diag_proc_blocked(diag_output_t *out) {
    diag_header(out, "Blocked Processes");

    blocked_proc_info_t blocked[32];
    int nblocked = scheduler_get_blocked_processes(blocked, 32);

    if (nblocked == 0) {
        diag_printf_user(out, "  No blocked processes.\n");
        return;
    }

    diag_printf_user(out, "  PID   TID   WAIT_REASON            RESOURCE           TIMEOUT\n");
    diag_printf_user(out, "  ----  ----  ---------------------  -----------------  --------\n");

    for (int i = 0; i < nblocked; i++) {
        const char *reason = "UNKNOWN";
        switch (blocked[i].wait_reason) {
            case WAIT_SEMAPHORE:  reason = "Semaphore"; break;
            case WAIT_MUTEX:      reason = "Mutex"; break;
            case WAIT_COND:       reason = "Condition"; break;
            case WAIT_IO:         reason = "I/O"; break;
            case WAIT_SLEEP:      reason = "Sleep"; break;
            case WAIT_PIPE:       reason = "Pipe"; break;
            case WAIT_MSG:        reason = "Message"; break;
            case WAIT_FUTEX:      reason = "Futex"; break;
        }
        diag_printf_user(out, "  %-5d %-4d  %-22s  %-18s  %u ms\n",
            blocked[i].pid,
            blocked[i].tid,
            reason,
            blocked[i].resource,
            blocked[i].timeout_ms);
    }

    diag_printf_user(out, "\n  Total blocked: %d\n", nblocked);
}

/* ================================================================
 * PROCESS STACK USAGE (Item 28)
 * ================================================================ */
static void diag_proc_stack_usage(diag_output_t *out) {
    diag_header(out, "Process Stack Usage");

    process_t *proc = get_process_list();
    while (proc) {
        thread_t *t = proc->main_thread;
        if (!t) {
            proc = proc->next;
            continue;
        }

        /* Compute stack usage from stack canary or sentinel */
        uint64_t stack_base = t->stack_base;
        uint64_t stack_size = t->stack_size;
        uint64_t stack_ptr = (uint64_t)t->stack_ptr;
        uint64_t used = stack_base + stack_size - stack_ptr;
        double pct = (double)used * 100.0 / (double)stack_size;

        diag_printf_user(out, "  PID %d (%s):\n", proc->pid, proc->name);
        diag_printf_user(out, "    Kernel Stack: 0x%016llX - 0x%016llX (%u bytes)\n",
            stack_base, stack_base + stack_size, stack_size);
        diag_printf_user(out, "    Current SP:   0x%016llX\n", stack_ptr);
        diag_printf_user(out, "    Used:         %u bytes (%.1f%%)\n", (uint32_t)used, pct);
        diag_printf_user(out, "    Free:         %u bytes (%.1f%%)\n",
            (uint32_t)(stack_size - used),
            100.0 - pct);

        /* Check for stack overflow canaries */
        uint64_t canary_val = *(uint64_t*)(stack_base + stack_size - 8);
        if (canary_val != STACK_CANARY_MAGIC) {
            diag_printf_user(out, "    WARNING: Stack overflow detected! Canary corrupted: 0x%016llX\n", canary_val);
        } else {
            diag_printf_user(out, "    Canary: OK (0x%016llX)\n", canary_val);
        }

        proc = proc->next;
    }
}

/* ================================================================
 * PROCESS TREE (Item 29)
 * ================================================================ */
static void diag_proc_tree(diag_output_t *out) {
    diag_header(out, "Process Tree");

    /* Print process hierarchy */
    diag_printf_user(out, "  PID   PPID  NAME\n");
    diag_printf_user(out, "  ----  ----  ------------------\n");

    process_t *proc = get_process_list();
    while (proc) {
        /* Print with indentation based on depth */
        int depth = 0;
        process_t *p = proc;
        while (p->parent && p->parent != p) {
            depth++;
            p = p->parent;
        }

        for (int i = 0; i < depth; i++) diag_printf_user(out, "  ");
        diag_printf_user(out, "%-5d %-5d  %s\n",
            proc->pid,
            proc->parent ? proc->parent->pid : 0,
            proc->name);

        proc = proc->next;
    }
}

/* ================================================================
 * CONTEXT SWITCH OVERHEAD (Item 30)
 * ================================================================ */
static void diag_proc_cs_overhead(diag_output_t *out) {
    diag_header(out, "Context Switch Overhead");

    cs_stats_t cs;
    scheduler_get_cs_stats(&cs);

    diag_printf_user(out, "  Average CS overhead: %.1f cycles\n", cs.avg_cycles_per_switch);
    diag_printf_user(out, "    Register save/restore: ~%d cycles\n", 200);
    diag_printf_user(out, "    CR3 switch (if needed): ~%d cycles\n", 30);
    diag_printf_user(out, "    Cache effects: ~%d cycles\n", 300);
    diag_printf_user(out, "    TLB shootdown (if needed): ~%d cycles\n", 500);
    diag_printf_user(out, "\n  Overhead breakdown:\n");
    diag_printf_user(out, "    Kernel overhead:  %.1f%%\n", cs.kernel_overhead_pct);
    diag_printf_user(out, "    Context switches: %.1f%%\n", cs.cs_overhead_pct);
    diag_printf_user(out, "    Scheduler:        %.1f%%\n", cs.scheduler_overhead_pct);
    diag_printf_user(out, "\n  Total CS overhead per second: %llu cycles\n", cs.total_cs_cycles_per_sec);
    diag_printf_user(out, "  CPU spend on CS: %.2f%%\n",
        cs.total_cs_cycles_per_sec ? (double)cs.total_cs_cycles_per_sec * 100.0 / 2500000000.0 : 0.0);
}

/* ================================================================
 * PROCESS DISPATCHER
 * ================================================================ */
void diag_process_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_PROC_LIST:
            diag_proc_list(out);
            break;
        case DIAG_PROC_SCHED_QUEUE:
            diag_proc_sched_queue(out);
            break;
        case DIAG_PROC_CONTEXT_SWITCHES:
            diag_proc_context_switches(out);
            break;
        case DIAG_PROC_CPU_USAGE:
            diag_proc_cpu_usage(out);
            break;
        case DIAG_PROC_THREAD_STATES:
            diag_proc_thread_states(out);
            break;
        case DIAG_PROC_SCHED_LATENCY:
            diag_proc_sched_latency(out);
            break;
        case DIAG_PROC_BLOCKED:
            diag_proc_blocked(out);
            break;
        case DIAG_PROC_STACK_USAGE:
            diag_proc_stack_usage(out);
            break;
        case DIAG_PROC_TREE:
            diag_proc_tree(out);
            break;
        case DIAG_PROC_CS_OVERHEAD:
            diag_proc_cs_overhead(out);
            break;
        case 0: /* All process diagnostics */
            diag_proc_list(out);
            diag_proc_sched_queue(out);
            diag_proc_context_switches(out);
            diag_proc_cpu_usage(out);
            diag_proc_thread_states(out);
            diag_proc_sched_latency(out);
            diag_proc_blocked(out);
            diag_proc_stack_usage(out);
            diag_proc_tree(out);
            diag_proc_cs_overhead(out);
            break;
        default:
            diag_header(out, "Process Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
