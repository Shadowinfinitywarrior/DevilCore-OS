/*
 * DevilCore OS — Debugging & Diagnostics
 * Items 101-110
 */

#include "diag.h"
#include "../include/ports.h"
#include "../arch/x86_64/gdt.h"
#include "../arch/x86_64/idt.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../include/string.h"

/* ================================================================
 * LAST EXCEPTION/PAGE FAULT REGISTERS (Item 101)
 * ================================================================ */
static void diag_debug_last_exception(diag_output_t *out) {
    diag_header(out, "Last Exception/Page Fault Register State");

    exception_frame_t *frame = get_last_exception_frame();

    if (!frame) {
        diag_printf_user(out, "  No recent exceptions.\n");
        return;
    }

    diag_printf_user(out, "  Exception Number: %u\n", frame->vector);
    diag_printf_user(out, "  Error Code:       0x%08X\n", frame->err_code);
    diag_printf_user(out, "  RIP:              0x%016llX\n", frame->rip);
    diag_printf_user(out, "  CS:               0x%04X\n", frame->cs);
    diag_printf_user(out, "  RFLAGS:           0x%016llX\n", frame->rflags);
    diag_printf_user(out, "  CR2 (fault addr): 0x%016llX\n", get_cr2());
    diag_printf_user(out, "\n  Registers:\n");
    diag_printf_user(out, "    RAX=0x%016llX  RBX=0x%016llX\n", frame->rax, frame->rbx);
    diag_printf_user(out, "    RCX=0x%016llX  RDX=0x%016llX\n", frame->rcx, frame->rdx);
    diag_printf_user(out, "    RSI=0x%016llX  RDI=0x%016llX\n", frame->rsi, frame->rdi);
    diag_printf_user(out, "    RBP=0x%016llX  RSP=0x%016llX\n", frame->rbp, frame->rsp);
    diag_printf_user(out, "    R8-15: (saved)\n");

    /* Decode exception type */
    const char *exc_name = "Unknown";
    switch (frame->vector) {
        case 0:  exc_name = "Divide Error"; break;
        case 1:  exc_name = "Debug Exception"; break;
        case 3:  exc_name = "Breakpoint"; break;
        case 6:  exc_name = "Invalid Opcode"; break;
        case 8:  exc_name = "Double Fault"; break;
        case 10: exc_name = "Invalid TSS"; break;
        case 11: exc_name = "Segment Not Present"; break;
        case 12: exc_name = "Stack Segment Fault"; break;
        case 13: exc_name = "General Protection Fault"; break;
        case 14: exc_name = "Page Fault"; break;
        case 17: exc_name = "Alignment Check"; break;
    }
    diag_printf_user(out, "\n  Exception: %s (%u)\n", exc_name, frame->vector);

    if (frame->vector == 14) {
        diag_printf_user(out, "  Page Fault Details:\n");
        diag_printf_user(out, "    Present:  %s\n", frame->err_code & 0x1 ? "No (P=0)" : "Yes");
        diag_printf_user(out, "    Write:    %s\n", frame->err_code & 0x2 ? "Write" : "Read");
        diag_printf_user(out, "    User:     %s\n", frame->err_code & 0x4 ? "User mode" : "Kernel mode");
        diag_printf_user(out, "    RSVD:     %s\n", frame->err_code & 0x8 ? "Reserved bit set" : "OK");
        diag_printf_user(out, "    Inst:     %s\n", frame->err_code & 0x10 ? "Instruction fetch" : "Data access");
    }
}

/* ================================================================
 * CALL STACK FOR CURRENT PROCESS (Item 102)
 * ================================================================ */
static void diag_debug_call_stack(diag_output_t *out) {
    diag_header(out, "Current Process Call Stack");

    /* Capture stack trace */
    uint64_t frames[32];
    int nframes = capture_stack_trace(frames, 32, 0);

    if (nframes == 0) {
        diag_printf_user(out, "  Could not capture stack trace.\n");
        return;
    }

    diag_printf_user(out, "  Depth: %d frames\n", nframes);
    diag_printf_user(out, "\n  #  Return Address    Function (approximate)\n");
    diag_printf_user(out, "  -  ----------------  ---------------------------------\n");

    for (int i = 0; i < nframes; i++) {
        /* Try to resolve symbol name from address */
        const char *sym = resolve_symbol(frames[i]);
        diag_printf_user(out, "  %-2d  0x%016llX  %s\n",
            i, frames[i], sym ? sym : "(unknown)");
    }

    diag_printf_user(out, "\n  Current Thread: T%d (PID %d)\n",
        get_current_tid(), get_current_pid());
    diag_printf_user(out, "  Current Stack: 0x%016llX\n", get_current_sp());
}

/* ================================================================
 * KMALLOC/KFREE TRACE LOG (Item 103)
 * ================================================================ */
static void diag_debug_kmalloc_trace(diag_output_t *out) {
    diag_header(out, "kmalloc/kfree Trace Log (Last 100)");

    kmalloc_log_entry_t log[100];
    int nlog = heap_get_trace_log(log, 100);

    if (nlog == 0) {
        diag_printf_user(out, "  Allocation tracing disabled.\n");
        diag_printf_user(out, "  Recompile kernel with CONFIG_KMALLOC_DEBUG=y\n");
        return;
    }

    diag_printf_user(out, "  Last %d allocation/free operations:\n", nlog);
    diag_printf_user(out, "  #  Timestamp  Action   Size     Address      Caller\n");
    diag_printf_user(out, "  -  ---------  -------  -------  -----------  ------------------\n");

    for (int i = 0; i < nlog; i++) {
        diag_printf_user(out, "  %-2d  %8llu  %-7s  %7u  0x%016llX  0x%016llX\n",
            i,
            log[i].timestamp,
            log[i].action == 0 ? "alloc" : "free",
            log[i].size,
            log[i].address,
            log[i].caller);
    }

    diag_printf_user(out, "\n  Active Allocations: %u\n", heap_get_active_allocs());
    diag_printf_user(out, "  Total Allocated:   %u bytes\n", heap_get_total_allocated());
}

/* ================================================================
 * KERNEL THREADS AND STACK TRACES (Item 104)
 * ================================================================ */
static void diag_debug_kernel_threads(diag_output_t *out) {
    diag_header(out, "Kernel Threads");

    thread_t *threads = get_all_threads();
    int nthreads = count_all_threads();

    diag_printf_user(out, "  Total threads: %d\n", nthreads);
    diag_printf_user(out, "\n  TID   PID   Name              State     StackPtr         # Frames\n");
    diag_printf_user(out, "  ----  ----  ----------------  --------  ---------------  --------\n");

    for (int i = 0; i < nthreads; i++) {
        thread_t *t = &threads[i];
        const char *state = "UNK";
        switch (t->state) {
            case THREAD_RUNNING: state = "RUN"; break;
            case THREAD_READY:   state = "RDY"; break;
            case THREAD_BLOCKED: state = "BLK"; break;
            case THREAD_DEAD:    state = "DEAD"; break;
        }

        /* Capture a small stack trace */
        uint64_t frames[8];
        int nframes = capture_stack_trace_for_thread(t, frames, 8);

        diag_printf_user(out, "  %-4d  %-4d  %-16s  %-8s  0x%016llX  %d\n",
            t->tid,
            t->pid,
            t->name,
            state,
            t->stack_ptr,
            nframes);
    }

    diag_printf_user(out, "\n  Kernel Thread List (with stack traces):\n");
    for (int i = 0; i < nthreads; i++) {
        thread_t *t = &threads[i];
        if (t->pid == 0) { /* Kernel threads usually have PID 0 or 1 */
            uint64_t frames[4];
            int nf = capture_stack_trace_for_thread(t, frames, 4);
            diag_printf_user(out, "    %s (T%d): ", t->name, t->tid);
            for (int f = 0; f < nf; f++) {
                diag_printf_user(out, "0x%016llX ", frames[f]);
            }
            diag_printf_user(out, "\n");
        }
    }
}

/* ================================================================
 * SPINLOCK CONTENTION STATISTICS (Item 105)
 * ================================================================ */
static void diag_debug_spinlock_contention(diag_output_t *out) {
    diag_header(out, "Spinlock Contention Statistics");

    spinlock_stats_all_t all_stats;
    spinlock_get_all_stats(&all_stats);

    diag_printf_user(out, "  Total Spinlocks: %u\n", all_stats.num_locks);
    diag_printf_user(out, "  Locked Now:      %u\n", all_stats.locked_now);
    diag_printf_user(out, "  Total Acquisitions: %llu\n", all_stats.total_acquisitions);
    diag_printf_user(out, "  Contentions:     %llu\n", all_stats.contention_count);
    diag_printf_user(out, "  Avg Wait:       %.1f us\n", all_stats.avg_wait_us);
    diag_printf_user(out, "  Max Wait:       %.1f ms\n", all_stats.max_wait_ms);

    if (all_stats.num_locks > 0) {
        diag_printf_user(out, "\n  Top 5 Most Contended Locks:\n");
        diag_printf_user(out, "  Name              Acq      Cont   Wait(us)  %%\n");
        diag_printf_user(out, "  ----------------  --------  -----  --------  ---\n");
        for (int i = 0; i < MIN(5, all_stats.num_locks); i++) {
            diag_printf_user(out, "  %-16s  %8llu  %-5lu  %8.1f  %3.1f%%\n",
                all_stats.locks[i].name,
                all_stats.locks[i].acquisitions,
                all_stats.locks[i].contention,
                all_stats.locks[i].avg_wait_us,
                all_stats.locks[i].contention_pct);
        }
    }

    diag_printf_user(out, "\n  Waiters per lock:\n");
    for (int i = 0; i < all_stats.num_locks; i++) {
        diag_printf_user(out, "    %-16s: %d waiters\n",
            all_stats.locks[i].name, all_stats.locks[i].current_waiters);
    }
}

/* ================================================================
 * ATOMIC OPERATION COUNTS (Item 106)
 * ================================================================ */
static void diag_debug_atomic_ops(diag_output_t *out) {
    diag_header(out, "Atomic Operation Counts");

    atomic_op_stats_t atomic_stats;
    get_atomic_stats(&atomic_stats);

    diag_printf_user(out, "  Atomic Operations by Type:\n");
    diag_printf_user(out, "    x86 LOCK prefix:    %llu\n", atomic_stats.lock_prefix);
    diag_printf_user(out, "    XCHG:               %llu\n", atomic_stats.xchg);
    diag_printf_user(out, "    CMPXCHG:            %llu\n", atomic_stats.cmpxchg);
    diag_printf_user(out, "    Fetch-and-add (FAA):%llu\n", atomic_stats.faa);
    diag_printf_user(out, "    XADD:               %llu\n", atomic_stats.xadd);
    diag_printf_user(out, "    Atomic test-and-set:%llu\n", atomic_stats.test_set);
    diag_printf_user(out, "    LL/SC (load-linked/store-conditional): %llu\n", atomic_stats.ll_sc);

    diag_printf_user(out, "\n  Total Atomics: %llu\n", atomic_stats.total);
    diag_printf_user(out, "  Failed CAS:   %llu (%.2f%%)\n",
        atomic_stats.cas_failures,
        atomic_stats.cmpxchg ? (double)atomic_stats.cas_failures * 100.0 / (double)atomic_stats.cmpxchg : 0.0);
}

/* ================================================================
 * REGISTERED PANIC HANDLERS (Item 107)
 * ================================================================ */
static void diag_debug_panic_handlers(diag_output_t *out) {
    diag_header(out, "Registered Panic Handlers");

    panic_handler_t handlers[16];
    int nhandlers = get_panic_handlers(handlers, 16);

    if (nhandlers == 0) {
        diag_printf_user(out, "  No panic handlers registered.\n");
        return;
    }

    diag_printf_user(out, "  #  Handler Addr       Priority   Name\n");
    diag_printf_user(out, "  -  -----------------  ---------  ------------------\n");

    for (int i = 0; i < nhandlers; i++) {
        diag_printf_user(out, "  %-2d  0x%016llX  %-9d  %s\n",
            i,
            handlers[i].handler_addr,
            handlers[i].priority,
            handlers[i].name);
    }

    diag_printf_user(out, "\n  Total handlers: %d\n", nhandlers);
    diag_printf_user(out, "  Last Panic: %llu seconds ago\n", get_seconds_since_last_panic());
    diag_printf_user(out, "  Panic Count: %u\n", get_panic_count());
    diag_printf_user(out, "  Kernel OOPS:  %u\n", get_oops_count());
}

/* ================================================================
 * MEMORY POISONING CANARY VIOLATIONS (Item 108)
 * ================================================================ */
static void diag_debug_poison_canaries(diag_output_t *out) {
    diag_header(out, "Memory Poisoning Canary Violations");

    canary_violation_t violations[32];
    int nviol = get_canary_violations(violations, 32);

    if (nviol == 0) {
        diag_printf_user(out, "  No canary violations detected.\n");
        diag_printf_user(out, "  Memory poisoning system is active.\n");
        return;
    }

    diag_printf_user(out, "  WARNING: %d canary violation(s) detected!\n", nviol);
    diag_printf_user(out, "\n  #  Timestamp   Region          Expected      Found         Size\n");
    diag_printf_user(out, "  -  ----------  --------------  ------------  ------------  -----\n");

    for (int i = 0; i < nviol; i++) {
        diag_printf_user(out, "  %-2d  %10llu  0x%016llX  0x%016llX  0x%016llX  %5u\n",
            i,
            violations[i].timestamp,
            violations[i].region_start,
            violations[i].expected_canary,
            violations[i].found_canary,
            violations[i].region_size);
    }

    diag_printf_user(out, "\n  Canary Magic: 0xDEADC0DE00000000 (DEADC0DE in high bytes)\n");
    diag_printf_user(out, "  Last violation at: 0x%016llX (overwrote canary)\n",
        violations[0].fault_address);
}

/* ================================================================
 * KERNEL LOG BUFFER (DMESG) (Item 109)
 * ================================================================ */
static void diag_debug_kernel_log(diag_output_t *out) {
    diag_header(out, "Kernel Log Buffer (dmesg)");

    /* The kernel maintains a circular log buffer */
    char logbuf[4096];
    int len = get_kernel_log_buffer(logbuf, sizeof(logbuf));

    if (len == 0) {
        diag_printf_user(out, "  Log buffer is empty.\n");
        return;
    }

    diag_printf_user(out, "  Last %d bytes of kernel log:\n", len);
    diag_printf_user(out, "  ---\n");
    diag_printf_user(out, "%s", logbuf);
    diag_printf_user(out, "  ---\n");

    diag_printf_user(out, "\n  Log capacity: %d bytes\n", KERNEL_LOG_SIZE);
    diag_printf_user(out, "  Log level: %s\n", get_kernel_log_level_str());
    diag_printf_user(out, "  Dropped messages: %u\n", get_kernel_log_dropped());
}

/* ================================================================
 * LAST ASSERTION FAILURE (Item 110)
 * ================================================================ */
static void diag_debug_last_assert(diag_output_t *out) {
    diag_header(out, "Last Assertion Failure");

    assert_info_t assert;
    int has_assert = get_last_assert(&assert);

    if (!has_assert) {
        diag_printf_user(out, "  No assertion failures detected.\n");
        diag_printf_user(out, "  All assertions passing.\n");
        return;
    }

    diag_printf_user(out, "  Assertion Failed: %s\n", assert.expr);
    diag_printf_user(out, "  File:            %s\n", assert.file);
    diag_printf_user(out, "  Line:            %u\n", assert.line);
    diag_printf_user(out, "  Function:        %s\n", assert.function);
    diag_printf_user(out, "  Timestamp:       %llu\n", assert.timestamp);
    diag_printf_user(out, "  PID:             %d\n", assert.pid);
    diag_printf_user(out, "  Thread:          T%d\n", assert.tid);
    diag_printf_user(out, "\n  Stack Trace:\n");
    for (int i = 0; i < assert.stack_depth; i++) {
        diag_printf_user(out, "    0x%016llX\n", assert.stack_trace[i]);
    }
}

/* ================================================================
 * DEBUGGING DISPATCHER
 * ================================================================ */
void diag_debug_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_DEBUG_LAST_EXCEPTION:
            diag_debug_last_exception(out);
            break;
        case DIAG_DEBUG_CALL_STACK:
            diag_debug_call_stack(out);
            break;
        case DIAG_DEBUG_KMALLOC_TRACE:
            diag_debug_kmalloc_trace(out);
            break;
        case DIAG_DEBUG_KERNEL_THREADS:
            diag_debug_kernel_threads(out);
            break;
        case DIAG_DEBUG_SPINLOCK_CONTENTION:
            diag_debug_spinlock_contention(out);
            break;
        case DIAG_DEBUG_ATOMIC_OPS:
            diag_debug_atomic_ops(out);
            break;
        case DIAG_DEBUG_PANIC_HANDLERS:
            diag_debug_panic_handlers(out);
            break;
        case DIAG_DEBUG_POISON_CANARIES:
            diag_debug_poison_canaries(out);
            break;
        case DIAG_DEBUG_KERNEL_LOG:
            diag_debug_kernel_log(out);
            break;
        case DIAG_DEBUG_LAST_ASSERT:
            diag_debug_last_assert(out);
            break;
        case 0: /* All debugging diagnostics */
            diag_debug_last_exception(out);
            diag_debug_call_stack(out);
            diag_debug_kmalloc_trace(out);
            diag_debug_kernel_threads(out);
            diag_debug_spinlock_contention(out);
            diag_debug_atomic_ops(out);
            diag_debug_panic_handlers(out);
            diag_debug_poison_canaries(out);
            diag_debug_kernel_log(out);
            diag_debug_last_assert(out);
            break;
        default:
            diag_header(out, "Debugging Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
