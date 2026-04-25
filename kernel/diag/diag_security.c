/*
 * DevilCore OS — System Calls & Security Diagnostics
 * Items 61-70
 */

#include "diag.h"
#include "../syscall/syscall.h"
#include "../syscall/syscall_table.c"
#include "../include/ports.h"
#include "../include/string.h"

/* ================================================================
 * SYSCALL FREQUENCY (Item 61)
 * ================================================================ */
static void diag_sec_syscall_freq(diag_output_t *out) {
    diag_header(out, "System Call Frequency (Top 10)");

    syscall_count_t counts[256];
    int ncounts = syscall_get_frequency_counts(counts, 256);

    if (ncounts == 0) {
        diag_printf_user(out, "  No syscall data available.\n");
        return;
    }

    /* Sort by count descending */
    for (int i = 0; i < ncounts - 1; i++) {
        for (int j = 0; j < ncounts - i - 1; j++) {
            if (counts[j].count < counts[j + 1].count) {
                syscall_count_t tmp = counts[j];
                counts[j] = counts[j + 1];
                counts[j + 1] = tmp;
            }
        }
    }

    diag_printf_user(out, "  Rank  Syscall          Count      %% of total\n");
    diag_printf_user(out, "  ----  ---------------  ----------  ----------\n");

    uint64_t total = 0;
    for (int i = 0; i < ncounts; i++) total += counts[i].count;

    for (int i = 0; i < MIN(10, ncounts); i++) {
        double pct = total ? (double)counts[i].count * 100.0 / (double)total : 0.0;
        const char *name = syscall_name(counts[i].num);
        diag_printf_user(out, "  %-4d  %-15s  %10llu  %8.2f%%\n",
            i + 1, name, counts[i].count, pct);
    }

    diag_printf_user(out, "\n  Total syscalls: %llu\n", total);
    diag_printf_user(out, "  Syscalls/sec: %.1f\n",
        (double)total / (double)get_uptime_seconds());
}

/* ================================================================
 * SYSCALL VECTORS AND SECURITY LEVELS (Item 62)
 * ================================================================ */
static void diag_sec_syscall_vectors(diag_output_t *out) {
    diag_header(out, "System Call Vectors and Security Levels");

    diag_printf_user(out, "  Syscall Table: 0x%016llX\n", (uint64_t)get_syscall_table_base());
    diag_printf_user(out, "  Table Entries: %d\n", NR_SYSCALLS);
    diag_printf_user(out, "\n  Vector   Syscall        Security   Validation\n");
    diag_printf_user(out, "  -------  -------------  ---------  ------------------\n");

    for (int i = 0; i < NR_SYSCALLS; i++) {
        void *handler = get_syscall_handler(i);
        if (handler) {
            uint8_t sec_level = syscall_security_level(i);
            const char *sec_str = "User";
            if (sec_level & SEC_KERNEL)    sec_str = "Kernel";
            else if (sec_level & SEC_PRIV) sec_str = "Privileged";
            else if (sec_level & SEC_USER) sec_str = "User";

            diag_printf_user(out, "  %7d  %-14s  %-9s  %s\n",
                i,
                syscall_name(i),
                sec_str,
                syscall_validation_desc(i));
        }
    }

    diag_printf_user(out, "\n  Security Flags:\n");
    diag_printf_user(out, "    SEC_KERNEL  = 0x01  Requires kernel context\n");
    diag_printf_user(out, "    SEC_PRIV    = 0x02  Requires elevated privilege\n");
    diag_printf_user(out, "    SEC_USER    = 0x04  User-callable\n");
    diag_printf_user(out, "    SEC_SANDBOX = 0x08  Sandbox/environment check\n");
}

/* ================================================================
 * USER-TO-KERNEL TRANSITION STATISTICS (Item 63)
 * ================================================================ */
static void diag_sec_user_kernel_trans(diag_output_t *out) {
    diag_header(out, "User-to-Kernel Transition Statistics");

    syscall_trans_stats_t trans_stats;
    syscall_get_transition_stats(&trans_stats);

    diag_printf_user(out, "  Total Transitions: %llu\n", trans_stats.total_transitions);
    diag_printf_user(out, "  Via SYSCALL:       %llu\n", trans_stats.syscall_trans);
    diag_printf_user(out, "  Via Interrupt:     %llu\n", trans_stats.interrupt_trans);
    diag_printf_user(out, "  Via Exception:     %llu\n", trans_stats.exception_trans);
    diag_printf_user(out, "\n  Average Transition Cost:\n");
    diag_printf_user(out, "    SYSCALL:  %.1f cycles (%.2f us)\n",
        trans_stats.avg_syscall_cycles,
        trans_stats.avg_syscall_cycles / 2500.0);
    diag_printf_user(out, "    Interrupt: %.1f cycles\n", trans_stats.avg_irq_cycles);
    diag_printf_user(out, "    Exception: %.1f cycles\n", trans_stats.avg_exc_cycles);

    diag_printf_user(out, "\n  TSS (Task State Segment):\n");
    diag_printf_user(out, "    RSP0 (kernel stack): 0x%016llX\n", trans_stats.tss_rsp0);
    diag_printf_user(out, "    IST0 (NMI stack):    0x%016llX\n", trans_stats.tss_ist0);
    diag_printf_user(out, "    IOPB Offset:         %u\n", trans_stats.tss_iomb);
}

/* ================================================================
 * CAPABILITIES/PERMISSIONS TABLES (Item 64)
 * ================================================================ */
static void diag_sec_capabilities(diag_output_t *out) {
    diag_header(out, "Capability and Permission Tables");

    cap_table_info_t caps;
    security_get_capabilities(&caps);

    diag_printf_user(out, "  Capability System: %s\n", caps.enabled ? "Enabled" : "Disabled");
    diag_printf_user(out, "  Capabilities Supported: %u\n", caps.num_caps);

    if (caps.enabled) {
        diag_printf_user(out, "\n  Capability   Desc                    Default\n");
        diag_printf_user(out, "  ----------   ---------------------   -------\n");
        for (int i = 0; i < caps.num_caps; i++) {
            diag_printf_user(out, "  %-12u  %-22s  %s\n",
                caps.caps[i].id,
                caps.caps[i].description,
                caps.caps[i].default_granted ? "Grant" : "Deny");
        }
    }

    diag_printf_user(out, "\n  Process Credentials:\n");
    diag_printf_user(out, "    PID   EUID   EGID   Capabilities\n");
    diag_printf_user(out, "    ---   ----   ----   ------------\n");
    process_t *p = get_process_list();
    while (p) {
        diag_printf_user(out, "    %-5d %-5d  %-5d  0x%016llX\n",
            p->pid, p->euid, p->egid, p->capabilities);
        p = p->next;
    }
}

/* ================================================================
 * ACCESS VIOLATIONS / AUDIT LOG (Item 65)
 * ================================================================ */
static void diag_sec_violations(diag_output_t *out) {
    diag_header(out, "Access Violations (Audit Log)");

    audit_log_entry_t entries[20];
    int nentries = security_get_audit_log(entries, 20);

    if (nentries == 0) {
        diag_printf_user(out, "  No security violations detected.\n");
        diag_printf_user(out, "  Audit log clean.\n");
        return;
    }

    diag_printf_user(out, "  Recent violations (last %d):\n", nentries);
    diag_printf_user(out, "  Time     PID   Type        Addr       Reason\n");
    diag_printf_user(out, "  -------- ----  ----------  ---------  ------------------\n");

    for (int i = 0; i < nentries; i++) {
        const char *vtype = "UNKNOWN";
        switch (entries[i].type) {
            case VIOLATION_NX:    vtype = "NX Page"; break;
            case VIOLATION_WP:    vtype = "Write Protect"; break;
            case VIOLATION_UID:   vtype = "UID Check"; break;
            case VIOLATION_CAP:   vtype = "Capability"; break;
            case VIOLATION_SEAL:  vtype = "Sealing"; break;
        }
        diag_printf_user(out, "  %8llu  %-4d  %-10s  0x%016llX  %s\n",
            entries[i].timestamp,
            entries[i].pid,
            vtype,
            entries[i].fault_addr,
            entries[i].reason);
    }

    diag_printf_user(out, "\n  Total violations: %llu\n", security_total_violations());
}

/* ================================================================
 * STACK CANARIES (Item 66)
 * ================================================================ */
static void diag_sec_stack_canaries(diag_output_t *out) {
    diag_header(out, "Kernel Stack Canary Status");

    canary_stats_t canary;
    security_get_canary_stats(&canary);

    diag_printf_user(out, "  Stack Protection: %s\n", canary.enabled ? "Enabled" : "Disabled");
    diag_printf_user(out, "  Canary Value: 0x%016llX\n", canary.magic_value);
    diag_printf_user(out, "  Kernel Stacks Checked: %u\n", canary.stacks_checked);
    diag_printf_user(out, "  Overflow Detected: %s\n", canary.overflow_detected ? "YES" : "No");
    diag_printf_user(out, "\n  Violations: %u\n", canary.violation_count);

    if (canary.overflow_detected) {
        diag_printf_user(out, "\n  Last Violation:\n");
        diag_printf_user(out, "    Thread: T%d (PID %d)\n", canary.last_thread, canary.last_pid);
        diag_printf_user(out, "    Stack Base: 0x%016llX\n", canary.last_stack_base);
        diag_printf_user(out, "    Canary Expected: 0x%016llX\n", canary.last_expected);
        diag_printf_user(out, "    Canary Found:    0x%016llX\n", canary.last_found);
        diag_printf_user(out, "    Instruction: 0x%016llX\n", canary.last_rip);
    }
}

/* ================================================================
 * SMEP/SMAP STATUS (Item 67)
 * ================================================================ */
static void diag_sec_smep_smap(diag_output_t *out) {
    diag_header(out, "SMEP/SMAP Status and Violations");

    uint64_t cr4;
    __asm__ volatile ("mov %%cr4, %0" : "=r" (cr4));

    #define CR4_SMEP (1 << 20)
    #define CR4_SMAP (1 << 21)

    diag_printf_user(out, "  CR4 Register: 0x%016llX\n", cr4);
    diag_printf_user(out, "  SMEP ( bit20): %s\n", (cr4 & CR4_SMEP) ? "Enabled" : "Disabled");
    diag_printf_user(out, "  SMAP ( bit21): %s\n", (cr4 & CR4_SMAP) ? "Enabled" : "Disabled");

    smep_smap_stats_t smep_smap;
    security_get_smep_smap_stats(&smep_smap);

    diag_printf_user(out, "\n  Violations:\n");
    diag_printf_user(out, "    SMEP Violations: %u\n", smep_smap.smep_violations);
    diag_printf_user(out, "    SMAP Violations: %u\n", smep_smap.smap_violations);
    diag_printf_user(out, "    Last Violation:  %s at 0x%016llX (PID %d)\n",
        smep_smap.last_type == VIOLATION_SMEP ? "SMEP" : "SMAP",
        smep_smap.last_addr,
        smep_smap.last_pid);
}

/* ================================================================
 * SHARED MEMORY REGIONS (Item 68)
 * ================================================================ */
static void diag_sec_shared_mem(diag_output_t *out) {
    diag_header(out, "Shared Memory Regions");

    shm_region_t shm_regions[32];
    int nregions = ipc_get_shm_regions(shm_regions, 32);

    if (nregions == 0) {
        diag_printf_user(out, "  No shared memory regions.\n");
        return;
    }

    diag_printf_user(out, "  #  Key      Shmid   Creator  Size      Perms   Attach\n");
    diag_printf_user(out, "  -  -------- ------- --------  --------  ------  ------\n");

    for (int i = 0; i < nregions; i++) {
        diag_printf_user(out, "  %-2d  0x%08X  %-7d  %-8d  %8uKB  0%03o  %d\n",
            i,
            shm_regions[i].key,
            shm_regions[i].shmid,
            shm_regions[i].creator_pid,
            shm_regions[i].size / 1024,
            shm_regions[i].perms,
            shm_regions[i].nattachments);
    }

    diag_printf_user(out, "\n  Total Shared Memory: %u KB\n", ipc_total_shm_kb());
    diag_printf_user(out, "  Maximum Segment:    %u KB\n", ipc_max_shm_segment());
}

/* ================================================================
 * USER PROCESS RING PROTECTION (Item 69)
 * ================================================================ */
static void diag_sec_ring_levels(diag_output_t *out) {
    diag_header(out, "User Process Ring Protection Levels");

    process_t *proc = get_process_list();

    diag_printf_user(out, "  PID   Process     Ring   CPL   IOPL   SyscallGate\n");
    diag_printf_user(out, "  ----  ---------   -----  ---   -----  ----------\n");

    while (proc) {
        uint8_t cpl = get_process_cpl(proc->pid);
        uint16_t iopl = get_process_iopl(proc->pid);
        const char *ring = "Ring 3 (User)";

        diag_printf_user(out, "  %-5d %-12s  %s   %u    %u     %s\n",
            proc->pid,
            proc->name,
            ring,
            cpl,
            iopl,
            proc->syscall_enabled ? "Yes" : "No");
        proc = proc->next;
    }

    diag_printf_user(out, "\n  Protection Rings:\n");
    diag_printf_user(out, "    Ring 0 (Kernel)    CPL=0  Full privileges\n");
    diag_printf_user(out, "    Ring 3 (User)      CPL=3  Restricted access\n");
    diag_printf_user(out, "\n  Current Privilege Level (CPL) is read from CS.RPL.\n");
}

/* ================================================================
 * SYSCALL OVERHEAD (Item 70)
 * ================================================================ */
static void diag_sec_syscall_overhead(diag_output_t *out) {
    diag_header(out, "Average System Call Overhead");

    syscall_perf_t perf;
    syscall_get_performance(&perf);

    diag_printf_user(out, "  Overall Average: %.1f cycles (%.2f us)\n",
        perf.avg_cycles,
        perf.avg_cycles / 2500.0);
    diag_printf_user(out, "\n  By Syscall Type (top 5):\n");
    diag_printf_user(out, "    Syscall            Avg(cy)  Avg(us)  Calls\n");
    diag_printf_user(out, "    -----------------  -------- -------- ------\n");
    for (int i = 0; i < MIN(5, perf.n_categories); i++) {
        diag_printf_user(out, "    %-18s  %8.1f  %7.2f  %6llu\n",
            perf.categories[i].name,
            perf.categories[i].avg_cycles,
            perf.categories[i].avg_cycles / 2500.0,
            perf.categories[i].count);
    }

    diag_printf_user(out, "\n  Breakdown:\n");
    diag_printf_user(out, "    Trap to kernel:          ~80 cycles\n");
    diag_printf_user(out, "    Args validation:         ~50 cycles\n");
    diag_printf_user(out, "    Handler execution:       ~200 cycles\n");
    diag_printf_user(out, "    Return to user:          ~100 cycles\n");
    diag_printf_user(out, "    TLB flush (if needed):   ~300 cycles\n");
    diag_printf_user(out, "    Total:                   ~730 cycles\n");
}

/* ================================================================
 * SECURITY DISPATCHER
 * ================================================================ */
void diag_security_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_SEC_SYSCALL_FREQ:
            diag_sec_syscall_freq(out);
            break;
        case DIAG_SEC_SYSCALL_VECTORS:
            diag_sec_syscall_vectors(out);
            break;
        case DIAG_SEC_USER_KERNEL_TRANS:
            diag_sec_user_kernel_trans(out);
            break;
        case DIAG_SEC_CAPABILITIES:
            diag_sec_capabilities(out);
            break;
        case DIAG_SEC_VIOLATIONS:
            diag_sec_violations(out);
            break;
        case DIAG_SEC_STACK_CANARIES:
            diag_sec_stack_canaries(out);
            break;
        case DIAG_SEC_SMEP_SMAP:
            diag_sec_smep_smap(out);
            break;
        case DIAG_SEC_SHARED_MEM:
            diag_sec_shared_mem(out);
            break;
        case DIAG_SEC_RING_LEVELS:
            diag_sec_ring_levels(out);
            break;
        case DIAG_SEC_SYSCALL_OVERHEAD:
            diag_sec_syscall_overhead(out);
            break;
        case 0: /* All security diagnostics */
            diag_sec_syscall_freq(out);
            diag_sec_syscall_vectors(out);
            diag_sec_user_kernel_trans(out);
            diag_sec_capabilities(out);
            diag_sec_violations(out);
            diag_sec_stack_canaries(out);
            diag_sec_smep_smap(out);
            diag_sec_shared_mem(out);
            diag_sec_ring_levels(out);
            diag_sec_syscall_overhead(out);
            break;
        default:
            diag_header(out, "Security Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
