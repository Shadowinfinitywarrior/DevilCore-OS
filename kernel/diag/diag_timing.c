/*
 * DevilCore OS — Timing & Performance Diagnostics
 * Items 81-90
 */

#include "diag.h"
#include "../drivers/timer.h"
#include "../arch/x86_64/cpu.h"
#include "../include/ports.h"
#include "../include/string.h"

/* ================================================================
 * SYSTEM UPTIME AND IDLE TIME (Item 81)
 * ================================================================ */
static void diag_time_uptime(diag_output_t *out) {
    diag_header(out, "System Uptime and Idle Time");

    uint64_t uptime_sec = get_uptime_seconds();
    uint64_t idle_sec = get_idle_seconds();

    diag_printf_user(out, "  Uptime: %llu seconds (%llu hours, %llu minutes, %llu seconds)\n",
        uptime_sec,
        uptime_sec / 3600,
        (uptime_sec % 3600) / 60,
        uptime_sec % 60);

    diag_printf_user(out, "  Idle Time: %llu seconds (%.1f%% of total)\n",
        idle_sec,
        uptime_sec ? (double)idle_sec * 100.0 / (double)uptime_sec : 0.0);

    diag_printf_user(out, "  Boot Time: %llu seconds since epoch\n", get_boot_time());
    diag_printf_user(out, "  Last Wake: %llu seconds ago\n", get_last_wake_seconds());
    diag_printf_user(out, "  Sleep Events: %u\n", get_sleep_count());
}

/* ================================================================
 * TIMER CALLBACK LIST (Item 82)
 * ================================================================ */
static void diag_time_timers(diag_output_t *out) {
    diag_header(out, "Timer Callback List");

    timer_info_t timers[32];
    int ntimers = timer_get_active(timers, 32);

    diag_printf_user(out, "  Active Timers: %d\n", ntimers);
    diag_printf_user(out, "\n  #  Name            Interval   Next Fire   Handler Addr\n");
    diag_printf_user(out, "  -  --------------  ---------  ----------  ------------------\n");

    for (int i = 0; i < ntimers; i++) {
        diag_printf_user(out, "  %-2d  %-14s  %8lums  %9lu 0x%016llX\n",
            i,
            timers[i].name,
            timers[i].interval_ms,
            timers[i].next_fire_tick,
            timers[i].handler_addr);
    }

    /* Show timer tick rate */
    diag_printf_user(out, "\n  Timer Tick Rate: %u Hz\n", get_timer_frequency());
    diag_printf_user(out, "  Tick Count: %llu\n", get_tick_count());
    diag_printf_user(out, "  Ticks per ms: %u\n", get_ticks_per_ms());
}

/* ================================================================
 * TIME SINCE LAST SCHEDULER TICK (Item 83)
 * ================================================================ */
static void diag_time_sched_tick(diag_output_t *out) {
    diag_header(out, "Time Since Last Scheduler Tick");

    uint64_t ticks_ago = scheduler_ticks_since_last();
    uint64_t ms_ago = ticks_ago / get_ticks_per_ms();

    diag_printf_user(out, "  Ticks since last tick: %llu\n", ticks_ago);
    diag_printf_user(out, "  Time since last tick: %llu ms\n", ms_ago);
    diag_printf_user(out, "  Scheduler interval:  %u ms\n", get_scheduler_interval_ms());
    diag_printf_user(out, "\n  Scheduler is ticking: %s\n",
        ticks_ago < get_ticks_per_ms() * get_scheduler_interval_ms() ? "Yes" : "NO - LATENCY ISSUE!");
}

/* ================================================================
 * HIGH-RESOLUTION TIMERS (Item 84)
 * ================================================================ */
static void diag_time_hpet_tsc(diag_output_t *out) {
    diag_header(out, "High-Resolution Timers");

    /* TSC (Time Stamp Counter) */
    uint64_t tsc = rdtsc();
    diag_printf_user(out, "  Time Stamp Counter (TSC):\n");
    diag_printf_user(out, "    Current TSC: 0x%016llX\n", tsc);
    diag_printf_user(out, "    Approx GHz:  %.3f GHz\n", estimate_tsc_freq() / 1e9);

    /* HPET (High Precision Event Timer) */
    diag_printf_user(out, "\n  HPET (if present):\n");
    hpet_info_t hpet;
    if (hpet_get_info(&hpet)) {
        diag_printf_user(out, "    HPET Address: 0x%016llX\n", hpet.address);
        diag_printf_user(out, "    Frequency:    %u Hz\n", hpet.frequency);
        diag_printf_user(out, "    Period:       %.1f ns\n", hpet.period_ns);
        diag_printf_user(out, "    Comparators:  %u\n", hpet.comparators);
    } else {
        diag_printf_user(out, "    HPET: Not detected or enabled\n");
    }

    /* PIT (Programmable Interval Timer) */
    diag_printf_user(out, "\n  PIT (8254):\n");
    diag_printf_user(out, "    Channel 0: %u Hz (sys timer)\n", PIT_FREQUENCY);
    diag_printf_user(out, "    Channel 1: DRAM refresh (legacy)\n");
    diag_printf_user(out, "    Channel 2: PC speaker\n");

    /* APIC Timer */
    diag_printf_user(out, "\n  Local APIC Timer:\n");
    apic_timer_info_t apic_t;
    apic_get_timer_info(&apic_t);
    diag_printf_user(out, "    Current Count: %u\n", apic_t.current_count);
    diag_printf_user(out, "    Divisor:       %u\n", apic_t.divisor);
    diag_printf_user(out, "    Mode:         %s\n",
        apic_t.one_shot ? "One-shot" : "Periodic");
}

/* ================================================================
 * PERFORMANCE COUNTERS (Item 85)
 * ================================================================ */
static void diag_time_perf_counters(diag_output_t *out) {
    diag_header(out, "Performance Monitoring Counters (PMU)");

    /* Check if PMU is available */
    uint32_t eax, ebx, ecx, edx;
    cpuid(0x0A, 0, &eax, &ebx, &ecx, &edx);

    if (eax == 0) {
        diag_printf_user(out, "  Performance Counters: Not supported\n");
        return;
    }

    diag_printf_user(out, "  PMU Version: %u\n", eax & 0xFF);
    diag_printf_user(out, "  GP Counters: %u\n", (eax >> 8) & 0xFF);
    diag_printf_user(out, "  Fixed Counters: %u\n", (eax >> 16) & 0xFF);
    diag_printf_user(out, "  Counter Width: %u bits\n", ((eax >> 24) & 0xFF) ? 64 : 32);

    /* Read some counters if available */
    uint64_t cycles, instructions, cache_misses, branch_misses;

    /* Use RDPMC if available */
    __asm__ volatile ("rdpmc" : "=a"(eax), "=d"(edx) : "c"(0));
    cycles = ((uint64_t)edx << 32) | eax;

    __asm__ volatile ("rdpmc" : "=a"(eax), "=d"(edx) : "c"(1));
    instructions = ((uint64_t)edx << 32) | eax;

    diag_printf_user(out, "\n  Current Counter Values:\n");
    diag_printf_user(out, "    CPU Cycles:               %llu\n", cycles);
    diag_printf_user(out, "    Instructions Retired:     %llu\n", instructions);
    diag_printf_user(out, "    CPI (Cycles/Instruction): %.2f\n",
        instructions ? (double)cycles / (double)instructions : 0.0);
}

/* ================================================================
 * CACHE MISS RATE (Item 86)
 * ================================================================ */
static void diag_time_cache_miss(diag_output_t *out) {
    diag_header(out, "Cache Miss Rate");

    /* Use performance counters if available to measure cache misses */
    cache_stats_t cache;
    perf_get_cache_stats(&cache);

    diag_printf_user(out, "  L1 Data Cache:\n");
    diag_printf_user(out, "    Size:        %u KB\n", cache.l1d_size_kb);
    diag_printf_user(out, "    Line Size:   %u bytes\n", cache.l1d_line_size);
    diag_printf_user(out, "    Ways:        %u\n", cache.l1d_ways);
    diag_printf_user(out, "    Misses:      %llu\n", cache.l1d_misses);
    diag_printf_user(out, "    Hits:        %llu\n", cache.l1d_hits);
    diag_printf_user(out, "    Miss Rate:   %.2f%%\n",
        cache.l1d_hits + cache.l1d_misses ?
            (double)cache.l1d_misses * 100.0 / (double)(cache.l1d_hits + cache.l1d_misses) : 0.0);

    diag_printf_user(out, "\n  L1 Instruction Cache:\n");
    diag_printf_user(out, "    Size:        %u KB\n", cache.l1i_size_kb);
    diag_printf_user(out, "    Misses:      %llu\n", cache.l1i_misses);
    diag_printf_user(out, "    Hits:        %llu\n", cache.l1i_hits);
    diag_printf_user(out, "    Miss Rate:   %.2f%%\n",
        cache.l1i_hits + cache.l1i_misses ?
            (double)cache.l1i_misses * 100.0 / (double)(cache.l1i_hits + cache.l1i_misses) : 0.0);

    diag_printf_user(out, "\n  L2 Cache (Unified):\n");
    diag_printf_user(out, "    Size:        %u KB\n", cache.l2_size_kb);
    diag_printf_user(out, "    Misses:      %llu\n", cache.l2_misses);
    diag_printf_user(out, "    Hits:        %llu\n", cache.l2_hits);
    diag_printf_user(out, "    Miss Rate:   %.2f%%\n",
        cache.l2_hits + cache.l2_misses ?
            (double)cache.l2_misses * 100.0 / (double)(cache.l2_hits + cache.l2_misses) : 0.0);

    diag_printf_user(out, "\n  L3 Cache (if present):\n");
    if (cache.l3_size_kb > 0) {
        diag_printf_user(out, "    Size:        %u KB\n", cache.l3_size_kb);
        diag_printf_user(out, "    Misses:      %llu\n", cache.l3_misses);
        diag_printf_user(out, "    Hits:        %llu\n", cache.l3_hits);
        diag_printf_user(out, "    Miss Rate:   %.2f%%\n",
            cache.l3_hits + cache.l3_misses ?
                (double)cache.l3_misses * 100.0 / (double)(cache.l3_hits + cache.l3_misses) : 0.0);
    } else {
        diag_printf_user(out, "    Not present\n");
    }
}

/* ================================================================
 * TLB MISS RATE (Item 87)
 * ================================================================ */
static void diag_time_tlb_miss(diag_output_t *out) {
    diag_header(out, "TLB Miss Rate");

    tlb_stats_t tlb;
    perf_get_tlb_stats(&tlb);

    diag_printf_user(out, "  TLB Configuration:\n");
    diag_printf_user(out, "    L1 Data TLB (4KB):   %d entries\n", tlb.l1dtlb_4k);
    diag_printf_user(out, "    L1 Instr TLB (4KB):  %d entries\n", tlb.l1itlb_4k);
    diag_printf_user(out, "    L1 TLB (Huge):       %d entries\n", tlb.l1tlb_huge);
    diag_printf_user(out, "    L2 TLB (Shared):     %d entries\n", tlb.l2tlb);

    diag_printf_user(out, "\n  TLB Miss Statistics (last 100,000 accesses):\n");
    diag_printf_user(out, "    DTLB Misses:  %llu (%.2f%%)\n",
        tlb.dtlb_misses,
        tlb.total_accesses ? (double)tlb.dtlb_misses * 100.0 / (double)tlb.total_accesses : 0.0);
    diag_printf_user(out, "    ITLB Misses:  %llu (%.2f%%)\n",
        tlb.itlb_misses,
        tlb.total_accesses ? (double)tlb.itlb_misses * 100.0 / (double)tlb.total_accesses : 0.0);

    diag_printf_user(out, "\n  Average TLB Miss Latency: %.1f cycles\n", tlb.avg_miss_penalty);
    diag_printf_user(out, "  TLB Shootdowns: %llu (IPI-induced)\n", tlb.shootdowns);
    diag_printf_user(out, "  TLB Flushes:    %llu (local)\n", tlb.local_flushes);
}

/* ================================================================
 * CPU FREQUENCY SCALING (Item 88)
 * ================================================================ */
static void diag_time_cpu_freq(diag_output_t *out) {
    diag_header(out, "CPU Frequency Scaling");

    cpufreq_info_t freq;
    get_cpufreq_info(&freq);

    diag_printf_user(out, "  CPU Frequency Information:\n");
    diag_printf_user(out, "    Current Frequency: %.3f GHz (%u MHz)\n",
        (double)freq.current_mhz / 1000.0,
        freq.current_mhz);
    diag_printf_user(out, "    Minimum Frequency: %.3f GHz\n", (double)freq.min_mhz / 1000.0);
    diag_printf_user(out, "    Maximum Frequency: %.3f GHz\n", (double)freq.max_mhz / 1000.0);
    diag_printf_user(out, "    Governors: %s\n", freq.governor);
    diag_printf_user(out, "    Scaling Driver: %s\n", freq.driver);

    diag_printf_user(out, "\n  Frequency History (last 10 samples):\n");
    for (int i = 0; i < 10; i++) {
        diag_printf_user(out, "    Sample %d: %.3f GHz\n", i,
            (double)freq.history[i] / 1000.0);
    }

    diag_printf_user(out, "\n  Power States (C-states): ");
    for (int i = 0; i < freq.ncstates; i++) {
        diag_printf_user(out, "C%d ", i);
    }
    diag_printf_user(out, "\n");
}

/* ================================================================
 * KERNEL PROFILING DATA (Item 89)
 * ================================================================ */
static void diag_time_profiling(diag_output_t *out) {
    diag_header(out, "Kernel Function Profiling");

    profiler_entry_t profile[50];
    int nentries = profiler_get_top(profile, 50);

    if (nentries == 0) {
        diag_printf_user(out, "  Profiling not enabled.\n");
        diag_printf_user(out, "  Recompile kernel with CONFIG_PROFILING=y\n");
        return;
    }

    diag_printf_user(out, "  Top %d kernel functions by time:\n", nentries);
    diag_printf_user(out, "  Function                          Time(ms)  Calls  Avg(us)\n");
    diag_printf_user(out, "  --------------------------------  --------  -----  --------\n");

    for (int i = 0; i < nentries; i++) {
        uint64_t avg_us = profile[i].calls ? profile[i].cycles / 2500 / profile[i].calls : 0;
        diag_printf_user(out, "  %-32s  %8llu  %-5u  %8llu\n",
            profile[i].name,
            profile[i].cycles / 2500000,
            profile[i].calls,
            avg_us);
    }

    diag_printf_user(out, "\n  Total profiling samples: %llu\n", profiler_total_samples());
    diag_printf_user(out, "  Profiling interval: %u ms\n", profiler_interval_ms());
    diag_printf_user(out, "  Profiling mode: %s\n", profiler_is_running() ? "Running" : "Stopped");
}

/* ================================================================
 * SPINLOCKS WITH LONG WAITERS (Item 90)
 * ================================================================ */
static void diag_time_spinlocks(diag_output_t *out) {
    diag_header(out, "Spinlocks with Long Waiters (>10ms)");

    spinlock_info_t locks[32];
    int nlocks = spinlock_get_stats(locks, 32);

    if (nlocks == 0) {
        diag_printf_user(out, "  No spinlock contention detected.\n");
        return;
    }

    diag_printf_user(out, "  #  Address    Name                Owner    Wait(ms)  Acq(us)  Contention\n");
    diag_printf_user(out, "  -  ---------  ------------------  -------- --------  -------  ----------\n");

    int long_waits = 0;
    for (int i = 0; i < nlocks; i++) {
        if (locks[i].avg_wait_ms >= 10) {
            long_waits++;
            diag_printf_user(out, "  %-2d  0x%08X  %-19s  %-8d  %-7d  %-7d  %.1f%%\n",
                i,
                locks[i].address,
                locks[i].name,
                locks[i].owner_tid,
                locks[i].avg_wait_ms,
                locks[i].avg_acquisition_us,
                locks[i].contention_pct);
        }
    }

    if (long_waits == 0) {
        diag_printf_user(out, "  No spinlocks with waiters >10ms\n");
    } else {
        diag_printf_user(out, "\n  %d spinlocks with significant contention\n", long_waits);
        diag_printf_user(out, "  Consider lock splitting or finer-grained locking.\n");
    }

    diag_printf_user(out, "\n  All Spinlocks (brief):\n");
    for (int i = 0; i < nlocks; i++) {
        diag_printf_user(out, "    %-16s: %u acquisitions, %.1fms avg wait, %.1f%% contention\n",
            locks[i].name,
            locks[i].acquisitions,
            locks[i].avg_wait_ms,
            locks[i].contention_pct);
    }
}

/* ================================================================
 * TIMING DISPATCHER
 * ================================================================ */
void diag_timing_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_TIME_UPTIME:
            diag_time_uptime(out);
            break;
        case DIAG_TIME_TIMERS:
            diag_time_timers(out);
            break;
        case DIAG_TIME_SCHED_TICK:
            diag_time_sched_tick(out);
            break;
        case DIAG_TIME_HPET_TSC:
            diag_time_hpet_tsc(out);
            break;
        case DIAG_TIME_PERF_COUNTERS:
            diag_time_perf_counters(out);
            break;
        case DIAG_TIME_CACHE_MISS:
            diag_time_cache_miss(out);
            break;
        case DIAG_TIME_TLB_MISS:
            diag_time_tlb_miss(out);
            break;
        case DIAG_TIME_CPU_FREQ:
            diag_time_cpu_freq(out);
            break;
        case DIAG_TIME_PROFILING:
            diag_time_profiling(out);
            break;
        case DIAG_TIME_SPINLOCKS:
            diag_time_spinlocks(out);
            break;
        case 0: /* All timing diagnostics */
            diag_time_uptime(out);
            diag_time_timers(out);
            diag_time_sched_tick(out);
            diag_time_hpet_tsc(out);
            diag_time_perf_counters(out);
            diag_time_cache_miss(out);
            diag_time_tlb_miss(out);
            diag_time_cpu_freq(out);
            diag_time_profiling(out);
            diag_time_spinlocks(out);
            break;
        default:
            diag_header(out, "Timing & Performance Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
