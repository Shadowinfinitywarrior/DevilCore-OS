/*
 * DevilCore OS — Interprocess Communication Diagnostics
 * Items 71-80
 */

#include "diag.h"
#include "../proc/ipc.h"
#include "../include/string.h"

/* ================================================================
 * MESSAGE QUEUES (Item 71)
 * ================================================================ */
static void diag_ipc_msg_queues(diag_output_t *out) {
    diag_header(out, "Active Message Queues");

    msg_queue_t queues[32];
    int nqueues = ipc_get_msg_queues(queues, 32);

    if (nqueues == 0) {
        diag_printf_user(out, "  No message queues.\n");
        return;
    }

    diag_printf_user(out, "  #  Key      QID     Creator  Readers  Writers  Msgs   MaxMsgs\n");
    diag_printf_user(out, "  -  -------- ------- -------- -------- -------- ------ --------\n");

    for (int i = 0; i < nqueues; i++) {
        diag_printf_user(out, "  %-2d  0x%08X %-7d %-8d %-8d %-6d %8d\n",
            i,
            queues[i].key,
            queues[i].qid,
            queues[i].creator_pid,
            queues[i].reader_count,
            queues[i].writer_count,
            queues[i].msg_count,
            queues[i].max_msgs);
    }

    diag_printf_user(out, "\n  Total queues: %d\n", nqueues);
    diag_printf_user(out, "  Total messages in all queues: %u\n", ipc_total_queued_msgs());
}

/* ================================================================
 * PIPE BUFFERS (Item 72)
 * ================================================================ */
static void diag_ipc_pipes(diag_output_t *out) {
    diag_header(out, "Pipe Buffers");

    pipe_info_t pipes[32];
    int npipes = ipc_get_pipes(pipes, 32);

    if (npipes == 0) {
        diag_printf_user(out, "  No pipes.\n");
        return;
    }

    diag_printf_user(out, "  #  PID     FD      ReadPtr   WritePtr   Size     Capacity\n");
    diag_printf_user(out, "  -  ------  ------  --------  ---------  -------  --------\n");

    for (int i = 0; i < npipes; i++) {
        diag_printf_user(out, "  %-2d  %-6d  %-6d  0x%08X  0x%08X  %7u  %8u\n",
            i,
            pipes[i].pid,
            pipes[i].fd,
            pipes[i].read_ptr,
            pipes[i].write_ptr,
            pipes[i].size,
            pipes[i].capacity);
    }

    diag_printf_user(out, "\n  Pipe Buffer Usage:\n");
    diag_printf_user(out, "    Total Pipes: %d\n", npipes);
    diag_printf_user(out, "    Total Pipe Memory: %u KB\n", ipc_total_pipe_memory() / 1024);
    diag_printf_user(out, "    Blocked Readers: %d\n", ipc_count_blocked_readers());
    diag_printf_user(out, "    Blocked Writers: %d\n", ipc_count_blocked_writers());
}

/* ================================================================
 * SHARED MEMORY SEGMENTS (Item 73)
 * ================================================================ */
static void diag_ipc_shm(diag_output_t *out) {
    diag_header(out, "Shared Memory Segments");

    shm_region_t regions[32];
    int nregions = ipc_get_shm_regions(regions, 32);

    if (nregions == 0) {
        diag_printf_user(out, "  No shared memory segments.\n");
        return;
    }

    diag_printf_user(out, "  #  Key      Shmid   Creator  Size      Perms  Attach  Attrs\n");
    diag_printf_user(out, "  -  -------- ------- --------  --------  -----  ------  -----\n");

    for (int i = 0; i < nregions; i++) {
        diag_printf_user(out, "  %-2d  0x%08X %-7d %-8d  %8uKB  0%03o  %6d  %s\n",
            i,
            regions[i].key,
            regions[i].shmid,
            regions[i].creator_pid,
            regions[i].size / 1024,
            regions[i].perms,
            regions[i].nattachments,
            regions[i].locked ? "Locked" : "Normal");
    }

    diag_printf_user(out, "\n  Summary:\n");
    diag_printf_user(out, "    Total segments: %d\n", nregions);
    diag_printf_user(out, "    Total shared memory: %u MB\n",
        ipc_total_shm_kb() / 1024);
    diag_printf_user(out, "    Largest segment: %u KB\n",
        ipc_largest_shm_segment() / 1024);
}

/* ================================================================
 * SEMAPHORES (Item 74)
 * ================================================================ */
static void diag_ipc_semaphores(diag_output_t *out) {
    diag_header(out, "Semaphores");

    semaphore_t sems[32];
    int nsems = ipc_get_semaphores(sems, 32);

    if (nsems == 0) {
        diag_printf_user(out, "  No semaphores.\n");
        return;
    }

    diag_printf_user(out, "  #  Key      Semid   Creator  Value   Max     Waiters\n");
    diag_printf_user(out, "  -  -------- ------- --------  ------  ------  --------\n");

    for (int i = 0; i < nsems; i++) {
        diag_printf_user(out, "  %-2d  0x%08X %-7d %-8d  %6u  %6u  %8d\n",
            i,
            sems[i].key,
            sems[i].semid,
            sems[i].creator_pid,
            sems[i].value,
            sems[i].max_value,
            sems[i].waiters);
    }

    diag_printf_user(out, "\n  Total semaphores: %d\n", nsems);
    diag_printf_user(out, "  Semaphores at max: %d\n", ipc_count_sems_maxed());
}

/* ================================================================
 * DEADLOCK DETECTION (Item 75)
 * ================================================================ */
static void diag_ipc_deadlock(diag_output_t *out) {
    diag_header(out, "Deadlock Detection");

    deadlock_info_t deadlocks[10];
    int ndeadd = deadlock_detect(deadlocks, 10);

    if (ndeadd == 0) {
        diag_printf_user(out, "  No deadlocks detected.\n");
        diag_printf_user(out, "  Resource allocation graph is safe.\n");
        return;
    }

    diag_printf_user(out, "  WARNING: %d potential deadlock(s) found!\n", ndeadd);
    diag_printf_user(out, "\n  Deadlock Cycles:\n");
    for (int d = 0; d < ndeadd; d++) {
        diag_printf_user(out, "\n  Cycle %d:\n", d + 1);
        for (int i = 0; i < deadlocks[d].cycle_len; i++) {
            diag_printf_user(out, "    Process %d -> Resource %s -> Process %d\n",
                deadlocks[d].cycle[i].pid,
                deadlocks[d].cycle[i].resource_name,
                deadlocks[d].cycle[(i + 1) % deadlocks[d].cycle_len].pid);
        }
        diag_printf_user(out, "    (All processes waiting, none will release)\n");
    }

    diag_printf_user(out, "\n  Total blocked processes: %d\n", deadlocks[0].total_blocked);
    diag_printf_user(out, "  Detector runs: Every %u ms\n", deadlock_detector_interval_ms());
}

/* ================================================================
 * SIGNALS (Item 76)
 * ================================================================ */
static void diag_ipc_signals(diag_output_t *out) {
    diag_header(out, "Signal Handlers and Pending Signals");

    process_t *proc = get_process_list();
    diag_printf_user(out, "  PID   Process     Pending   Ignored   Blocked   Handlers\n");
    diag_printf_user(out, "  ----  ---------   --------  --------  --------  --------\n");

    while (proc) {
        signal_info_t sig_info;
        get_signal_info(proc->pid, &sig_info);

        diag_printf_user(out, "  %-5d %-12s  %-8u  %-8u  %-8u  %u\n",
            proc->pid,
            proc->name,
            sig_info.pending,
            sig_info.ignored,
            sig_info.blocked,
            sig_info.handlers_installed);
        proc = proc->next;
    }

    diag_printf_user(out, "\n  Signal Dispositions:\n");
    diag_printf_user(out, "    SIGKILL (9): Terminate (cannot be caught/ignored)\n");
    diag_printf_user(out, "    SIGSTOP (19): Stop (cannot be caught/ignored)\n");
    diag_printf_user(out, "    SIGSEGV (11): Core dump / termination\n");
    diag_printf_user(out, "    Others: User-installable handlers\n");
}

/* ================================================================
 * MUTEXES AND LOCKS (Item 77)
 * ================================================================ */
static void diag_ipc_mutexes(diag_output_t *out) {
    diag_header(out, "System-Wide Mutexes");

    mutex_info_t mutexes[64];
    int nmutexes = ipc_get_mutexes(mutexes, 64);

    if (nmutexes == 0) {
        diag_printf_user(out, "  No mutexes currently allocated.\n");
        return;
    }

    diag_printf_user(out, "  #  Address    Name                Owner    Recursion  Waiters\n");
    diag_printf_user(out, "  -  ---------  ------------------  -------- ---------  ------\n");

    for (int i = 0; i < nmutexes; i++) {
        diag_printf_user(out, "  %-2d  0x%08X  %-19s  %-8d  %-9d  %6d\n",
            i,
            mutexes[i].address,
            mutexes[i].name,
            mutexes[i].owner_pid,
            mutexes[i].recursion,
            mutexes[i].waiters);
    }

    diag_printf_user(out, "\n  Mutex Statistics:\n");
    diag_printf_user(out, "    Total mutexes: %d\n", nmutexes);
    diag_printf_user(out, "    Locked: %d\n", ipc_count_locked_mutexes());
    diag_printf_user(out, "    Average wait time: %u us\n", ipc_avg_mutex_wait_us());
    diag_printf_user(out, "    Contention rate: %.2f%%\n", ipc_mutex_contention_pct());
}

/* ================================================================
 * FUTEX WAIT QUEUES (Item 78)
 * ================================================================ */
static void diag_ipc_futex(diag_output_t *out) {
    diag_header(out, "Futex Wait Queues");

    futex_entry_t futexes[64];
    int nfutexes = ipc_get_futexes(futexes, 64);

    if (nfutexes == 0) {
        diag_printf_user(out, "  No futexes with waiters.\n");
        return;
    }

    diag_printf_user(out, "  #  Address    Value   Waiters  Op      PID\n");
    diag_printf_user(out, "  -  ---------  ------  --------  ------  ----\n");

    for (int i = 0; i < nfutexes; i++) {
        diag_printf_user(out, "  %-2d  0x%08X  %-6u  %8d  %-6s  %-4d\n",
            i,
            futexes[i].uaddr,
            futexes[i].val,
            futexes[i].waiters,
            futex_op_name(futexes[i].op),
            futexes[i].owner_pid);
    }

    diag_printf_user(out, "\n  Total futex objects: %d\n", ipc_total_futex_objects());
    diag_printf_user(out, "  Active waiters: %d\n", ipc_total_futex_waiters());
    diag_printf_user(out, "  Futex spurious wakeups: %u\n", ipc_futex_spurious_wakeups());
}

/* ================================================================
 * IPC THROUGHPUT (Item 79)
 * ================================================================ */
static void diag_ipc_throughput(diag_output_t *out) {
    diag_header(out, "Message Passing Throughput");

    ipc_throughput_t tp;
    ipc_get_throughput(&tp);

    diag_printf_user(out, "  Per-message cost:\n");
    diag_printf_user(out, "    msgget() + msgsnd() + msgrcv() + msgctl() average: %.1f us\n",
        tp.avg_msg_latency_us);
    diag_printf_user(out, "    Messages per second (peak): ~%.0f msgs/sec\n",
        1000000.0 / tp.avg_msg_latency_us);

    diag_printf_user(out, "\n  Pipe throughput:\n");
    diag_printf_user(out, "    Write throughput:  %.2f MB/s\n", tp.pipe_write_mbps);
    diag_printf_user(out, "    Read throughput:   %.2f MB/s\n", tp.pipe_read_mbps);
    diag_printf_user(out, "    Pipe buffer size:  %u bytes (default)\n", PIPE_BUF);

    diag_printf_user(out, "\n  Shared memory bandwidth: %.2f MB/s (theoretical)\n",
        tp.shm_bandwidth_mbps);
    diag_printf_user(out, "    (Limited by memory copy speed)\n");
}

/* ================================================================
 * IPC LATENCY (Item 80)
 * ================================================================ */
static void diag_ipc_latency(diag_output_t *out) {
    diag_header(out, "Average IPC Latency");

    ipc_latency_t lat;
    ipc_get_latency(&lat);

    diag_printf_user(out, "  IPC Mechanism Latencies (average):\n");
    diag_printf_user(out, "    Message Queue:  %8.1f cycles  (%6.2f us)\n",
        lat.msgqueue_cycles,
        lat.msgqueue_cycles / 2500.0);
    diag_printf_user(out, "    Pipe:           %8.1f cycles  (%6.2f us)\n",
        lat.pipe_cycles,
        lat.pipe_cycles / 2500.0);
    diag_printf_user(out, "    Shared Memory:  %8.1f cycles  (%6.2f us)  (read after write)\n",
        lat.shm_cycles,
        lat.shm_cycles / 2500.0);
    diag_printf_user(out, "    Semaphore:      %8.1f cycles  (%6.2f us)\n",
        lat.semaphore_cycles,
        lat.semaphore_cycles / 2500.0);
    diag_printf_user(out, "    Mutex:          %8.1f cycles  (%6.2f us)\n",
        lat.mutex_cycles,
        lat.mutex_cycles / 2500.0);
    diag_printf_user(out, "    Futex:          %8.1f cycles  (%6.2f us)\n",
        lat.futex_cycles,
        lat.futex_cycles / 2500.0);

    diag_printf_user(out, "\n  Fastest IPC: Shared memory (cache hit)\n");
    diag_printf_user(out, "  Slowest IPC: Message queue (kernel copy)\n");
}

/* ================================================================
 * IPC DISPATCHER
 * ================================================================ */
void diag_ipc_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_IPC_MSG_QUEUES:
            diag_ipc_msg_queues(out);
            break;
        case DIAG_IPC_PIPES:
            diag_ipc_pipes(out);
            break;
        case DIAG_IPC_SHM:
            diag_ipc_shm(out);
            break;
        case DIAG_IPC_SEMAPHORES:
            diag_ipc_semaphores(out);
            break;
        case DIAG_IPC_DEADLOCK:
            diag_ipc_deadlock(out);
            break;
        case DIAG_IPC_SIGNALS:
            diag_ipc_signals(out);
            break;
        case DIAG_IPC_MUTEXES:
            diag_ipc_mutexes(out);
            break;
        case DIAG_IPC_FUTEX:
            diag_ipc_futex(out);
            break;
        case DIAG_IPC_THROUGHPUT:
            diag_ipc_throughput(out);
            break;
        case DIAG_IPC_LATENCY:
            diag_ipc_latency(out);
            break;
        case 0: /* All IPC diagnostics */
            diag_ipc_msg_queues(out);
            diag_ipc_pipes(out);
            diag_ipc_shm(out);
            diag_ipc_semaphores(out);
            diag_ipc_deadlock(out);
            diag_ipc_signals(out);
            diag_ipc_mutexes(out);
            diag_ipc_futex(out);
            diag_ipc_throughput(out);
            diag_ipc_latency(out);
            break;
        default:
            diag_header(out, "IPC Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
