/*
 * DevilCore OS — Diagnostic Subsystem
 * Comprehensive system testing and monitoring framework
 *
 * This module provides 120 diagnostic checks across 12 categories:
 *  1. Hardware & Boot Testing (items 1-10)
 *  2. Memory Management (items 11-20)
 *  3. Process & Scheduling (items 21-30)
 *  4. File System (items 31-40)
 *  5. Graphics & GUI (items 41-50)
 *  6. Device Drivers & I/O (items 51-60)
 *  7. System Calls & Security (items 61-70)
 *  8. Interprocess Communication (items 71-80)
 *  9. Timing & Performance (items 81-90)
 * 10. Networking (items 91-100)
 * 11. Debugging & Diagnostics (items 101-110)
 * 12. Advanced/Extra Features (items 111-120)
 *
 * The diagnostic system is accessed via the SYS_DIAG syscall.
 */

#ifndef DC_DIAG_H
#define DC_DIAG_H

#include "../include/types.h"

/* ================================================================
 * DIAGNOSTIC CATEGORIES
 * ================================================================ */
typedef enum {
    DIAG_CAT_CPU = 1,          /* Hardware & Boot Testing */
    DIAG_CAT_MEMORY,           /* Memory Management */
    DIAG_CAT_PROCESS,          /* Process & Scheduling */
    DIAG_CAT_FILESYSTEM,       /* File System */
    DIAG_CAT_GRAPHICS,         /* Graphics & GUI */
    DIAG_CAT_IO,               /* Device Drivers & I/O */
    DIAG_CAT_SECURITY,         /* System Calls & Security */
    DIAG_CAT_IPC,              /* Interprocess Communication */
    DIAG_CAT_TIMING,           /* Timing & Performance */
    DIAG_CAT_NETWORK,          /* Networking */
    DIAG_CAT_DEBUG,            /* Debugging & Diagnostics */
    DIAG_CAT_EXTRA,            /* Advanced/Extra Features */
    DIAG_CAT_ALL               /* Run all diagnostics */
} diag_category_t;

/* ================================================================
 * SUBCOMMANDS PER CATEGORY
 * ================================================================ */

/* ---- CPU/Hardware (1-10) ---- */
typedef enum {
    DIAG_CPU_VENDOR_FEATURES = 1,    /* Detect CPU vendor and features (SSE, AVX, etc.) */
    DIAG_CPU_MEMORY_MAP,             /* Show memory map from UEFI/BIOS */
    DIAG_CPU_PCI_DEVICES,            /* List detected PCI devices */
    DIAG_CPU_IDT,                    /* Show current interrupt descriptor table (IDT) addresses */
    DIAG_CPU_GDT,                    /* Display GDT entries and current segment registers */
    DIAG_CPU_ACPI,                   /* What ACPI version is supported? */
    DIAG_CPU_SMBIOS,                 /* Show SMBIOS/DMI system information */
    DIAG_CPU_APICS,                  /* Detect and list all CPU cores/APICs */
    DIAG_CPU_CR3,                    /* Show current paging structure (CR3 value) */
    DIAG_CPU_RSDP                    /* Display RSDP and RSDT/XSDT addresses */
} diag_cpu_subcmd_t;

/* ---- Memory (11-20) ---- */
typedef enum {
    DIAG_MEM_USAGE_BY_REGION = 11,   /* Show memory usage by region (kernel, user, free) */
    DIAG_MEM_MAPPED_PAGES,           /* List all mapped pages with permissions */
    DIAG_MEM_HEAP_STATS,             /* Show heap allocator statistics (blocks, fragments) */
    DIAG_MEM_LEAKS,                  /* Detect memory leaks - list unfreed allocations */
    DIAG_MEM_SLAB_STATS,             /* Show slab/arena cache statistics */
    DIAG_MEM_TLB_INFO,               /* Display TLB information (size, associativity) */
    DIAG_MEM_KERNEL_ALLOCS,          /* List all kernel heap allocations > 4KB */
    DIAG_MEM_PAGE_FAULTS,            /* Show memory pages with page faults in last minute */
    DIAG_MEM_PHYS_BITMAP,            /* Display physical memory bitmap (used/free frames) */
    DIAG_MEM_MAX_CONTIGUOUS          /* What's the maximum contiguous physical memory block? */
} diag_mem_subcmd_t;

/* ---- Process & Scheduling (21-30) ---- */
typedef enum {
    DIAG_PROC_LIST = 21,             /* List all running processes (PID, priority, state) */
    DIAG_PROC_SCHED_QUEUE,           /* Show current scheduler queue and timeslices */
    DIAG_PROC_CONTEXT_SWITCHES,      /* Display context switch count and frequency */
    DIAG_PROC_CPU_USAGE,             /* What's the CPU usage per process? */
    DIAG_PROC_THREAD_STATES,         /* Show thread states (blocked, ready, running) */
    DIAG_PROC_SCHED_LATENCY,         /* Display scheduling latency (from ready to running) */
    DIAG_PROC_BLOCKED,               /* List all blocked processes and wait reasons */
    DIAG_PROC_STACK_USAGE,           /* Show process stack usage (kernel and user stacks) */
    DIAG_PROC_TREE,                  /* Display process tree (parent-child relationships) */
    DIAG_PROC_CS_OVERHEAD            /* What's the average context switch overhead (cycles)? */
} diag_proc_subcmd_t;

/* ---- File System (31-40) ---- */
typedef enum {
    DIAG_FS_ROOT_LIST = 31,          /* List root directory contents with sizes */
    DIAG_FS_MOUNTED,                 /* Show mounted file systems and types */
    DIAG_FS_FD_TABLE,                /* Display file descriptor table (open files per process) */
    DIAG_FS_DISK_USAGE,              /* Check disk usage and free space */
    DIAG_FS_INODE_CACHE,             /* Show inode cache statistics */
    DIAG_FS_FS_CACHES,               /* List all file system caches and hit rates */
    DIAG_FS_DIR_ENTRY_CACHE,         /* Display directory entry cache status */
    DIAG_FS_JOURNAL,                 /* Show file system journal log (if implemented) */
    DIAG_FS_OPEN_HANDLES,            /* List all open file handles system-wide */
    DIAG_FS_FRAGMENTATION            /* What's the fragmentation level of drive 0? */
} diag_fs_subcmd_t;

/* ---- Graphics & GUI (41-50) ---- */
typedef enum {
    DIAG_GFX_FRAMEBUFFER = 41,       /* Show framebuffer info (resolution, depth, pitch) */
    DIAG_GFX_WINDOWS,                /* List all active windows and their Z-order */
    DIAG_GFX_EVENT_QUEUE,            /* Display window manager event queue (pending messages) */
    DIAG_GFX_REDRAW_REGIONS,         /* Show screen redraw regions (dirty rectangles) */
    DIAG_GFX_WIDGETS,                /* List all GUI widgets and their handles */
    DIAG_GFX_FPS,                    /* What's the current frame rate (FPS) of compositor? */
    DIAG_GFX_MOUSE,                  /* Show mouse cursor position and hitbox */
    DIAG_GFX_FONT_CACHE,             /* Display font cache (bitmaps/metrics) */
    DIAG_GFX_SURFACES,               /* List all graphical surfaces and their pixel formats */
    DIAG_GFX_SWAP_CHAIN              /* Show double-buffer swap chain status */
} diag_gfx_subcmd_t;

/* ---- Device Drivers & I/O (51-60) ---- */
typedef enum {
    DIAG_IO_DRIVERS = 51,            /* List all registered device drivers and status */
    DIAG_IO_KEYBOARD,                /* Show keyboard scan code buffer (pending events) */
    DIAG_IO_MOUSE,                   /* Display mouse movement deltas and button states */
    DIAG_IO_PCI_IRQ,                 /* List all PCI devices with IRQ assignments */
    DIAG_IO_USB_TREE,                /* Show USB device tree (hubs, devices, endpoints) */
    DIAG_IO_AUDIO,                   /* Display audio buffer status (playback/recording) */
    DIAG_IO_DMA,                     /* List all DMA channels and transfers in progress */
    DIAG_IO_IRQ_MAP,                 /* Show IRQ to handler mapping and firing count */
    DIAG_IO_STORAGE_QUEUE,           /* Display storage queue depth and pending commands */
    DIAG_IO_IRQ_LATENCY              /* What's the average interrupt latency (cycles)? */
} diag_io_subcmd_t;

/* ---- System Calls & Security (61-70) ---- */
typedef enum {
    DIAG_SEC_SYSCALL_FREQ = 61,      /* Show system call frequency (top 10 syscalls) */
    DIAG_SEC_SYSCALL_VECTORS,        /* List all syscall vectors and security levels */
    DIAG_SEC_USER_KERNEL_TRANS,      /* Display user-to-kernel transition statistics */
    DIAG_SEC_CAPABILITIES,           /* Show capability/permission tables */
    DIAG_SEC_VIOLATIONS,             /* List all access violations (audit log last minute) */
    DIAG_SEC_STACK_CANARIES,         /* Display kernel stack canary status (detected overflows?) */
    DIAG_SEC_SMEP_SMAP,              /* Show SMEP/SMAP state and violations */
    DIAG_SEC_SHARED_MEM,             /* List all shared memory regions and permissions */
    DIAG_SEC_RING_LEVELS,            /* Display user process ring protection levels */
    DIAG_SEC_SYSCALL_OVERHEAD        /* What's the average syscall overhead (cycles)? */
} diag_sec_subcmd_t;

/* ---- IPC (71-80) ---- */
typedef enum {
    DIAG_IPC_MSG_QUEUES = 71,        /* List all active message queues and readers/writers */
    DIAG_IPC_PIPES,                  /* Show pipe buffers and pending data */
    DIAG_IPC_SHM,                    /* Display shared memory segment statistics */
    DIAG_IPC_SEMAPHORES,             /* List all semaphores and their values */
    DIAG_IPC_DEADLOCK,               /* Show deadlock detector status (cycles detected?) */
    DIAG_IPC_SIGNALS,                /* Display inter-process signal handlers and pending signals */
    DIAG_IPC_MUTEXES,                /* List all mutexes and held locks system-wide */
    DIAG_IPC_FUTEX,                  /* Show futex wait queues (blocked processes) */
    DIAG_IPC_THROUGHPUT,             /* Display message passing throughput (msgs/sec) */
    DIAG_IPC_LATENCY                 /* What's the average IPC latency (cycles)? */
} diag_ipc_subcmd_t;

/* ---- Timing & Performance (81-90) ---- */
typedef enum {
    DIAG_TIME_UPTIME = 81,           /* Show system uptime and idle time */
    DIAG_TIME_TIMERS,                /* Display timer callback list (pending, periodic) */
    DIAG_TIME_SCHED_TICK,            /* Show time since last scheduler tick */
    DIAG_TIME_HPET_TSC,              /* List all high-resolution timers (HPET/TSC) */
    DIAG_TIME_PERF_COUNTERS,         /* Display performance counters (instructions, branches) */
    DIAG_TIME_CACHE_MISS,            /* Show cache miss rate (L1/L2/L3) */
    DIAG_TIME_TLB_MISS,              /* What's the TLB miss rate (last 100,000 accesses)? */
    DIAG_TIME_CPU_FREQ,              /* Display CPU frequency scaling information */
    DIAG_TIME_PROFILING,             /* Show profiling data for kernel functions */
    DIAG_TIME_SPINLOCKS              /* List all spinlocks with longest waiters (>10ms) */
} diag_time_subcmd_t;

/* ---- Networking (91-100) ---- */
typedef enum {
    DIAG_NET_INTERFACES = 91,        /* Show network interfaces and link status */
    DIAG_NET_ARP,                    /* Display ARP cache table */
    DIAG_NET_SOCKETS,                /* List all sockets and states (TCP/UDP) */
    DIAG_NET_TCP_CONN,               /* Show TCP connection table (seq, ack, window) */
    DIAG_NET_PKT_BUFFER,             /* Display packet buffer ring statistics */
    DIAG_NET_ERRORS,                 /* Show network driver error counters (CRC, collisions) */
    DIAG_NET_FIREWALL,               /* List all firewall rules (if any) */
    DIAG_NET_DNS,                    /* Display DNS resolver cache */
    DIAG_NET_THROUGHPUT,             /* What's the network throughput (bits/sec)? */
    DIAG_NET_ROUTING                 /* Show routing table (direct/gateway routes) */
} diag_net_subcmd_t;

/* ---- Debugging & Diagnostics (101-110) ---- */
typedef enum {
    DIAG_DEBUG_LAST_EXCEPTION = 101, /* Dump register state of last exception/page fault */
    DIAG_DEBUG_CALL_STACK,           /* Show call stack for current process */
    DIAG_DEBUG_KMALLOC_TRACE,        /* Display kmalloc/kfree trace log (last 100 operations) */
    DIAG_DEBUG_KERNEL_THREADS,       /* List all kernel threads and their stack traces */
    DIAG_DEBUG_SPINLOCK_CONTENTION,  /* Show spinlock contention statistics */
    DIAG_DEBUG_ATOMIC_OPS,           /* Display atomic operation counts by type */
    DIAG_DEBUG_PANIC_HANDLERS,       /* List all registered panic handlers */
    DIAG_DEBUG_POISON_CANARIES,      /* Show memory poisoning canary violations */
    DIAG_DEBUG_KERNEL_LOG,           /* Display kernel log buffer (dmesg equivalent) */
    DIAG_DEBUG_LAST_ASSERT           /* What's the last assertion failure and location? */
} diag_debug_subcmd_t;

/* ---- Advanced/Extra (111-120) ---- */
typedef enum {
    DIAG_EXTRA_GPU_QUEUE = 111,      /* Show 3D GPU command queue status (if Vulkan/OpenGL) */
    DIAG_EXTRA_VMX_SVM,              /* Display virtual machine monitor (VMX/SVM) status */
    DIAG_EXTRA_POWER_STATES,         /* List all power management sleep states supported */
    DIAG_EXTRA_BATTERY,              /* Show battery status and power source (if laptop) */
    DIAG_EXTRA_MONITORS,             /* Display multi-monitor configuration (attached displays) */
    DIAG_EXTRA_PRINT_SPOOL,          /* Show print spooler queue (unprinted documents) */
    DIAG_EXTRA_SESSIONS,             /* List all user sessions and login times */
    DIAG_EXTRA_CLIPBOARD,            /* Display clipboard contents and formats */
    DIAG_EXTRA_DRAG_DROP,            /* Show drag-drop operation in progress */
    DIAG_EXTRA_HOTKEYS              /* What's the global hotkey registration table? */
} diag_extra_subcmd_t;

/* ================================================================
 * RESULT BUFFER STRUCTURE
 * ================================================================ */
typedef struct {
    char   *buffer;      /* User-provided buffer pointer */
    size_t size;         /* Total buffer size */
    size_t written;      /* Bytes written so far */
    int    error;        /* Error code if any */
} diag_output_t;

/* ================================================================
 * SYSCALL INTERFACE
 * ================================================================ */
#define SYS_DIAG 300  /* Syscall number - must match syscall.h and table */

/* Syscall signature:
 *   int diag_category = rdi
 *   int diag_subcmd = rsi
 *   void *user_buffer = rdx
 *   size_t buffer_size = rcx
 *
 * Returns: number of bytes written to buffer, or negative errno
 */

/* ================================================================
 * INTERNAL FUNCTIONS (kernel-internal)
 * ================================================================ */

/* Initialize diagnostic subsystem */
void diag_init(void);

/* Main dispatch function for sys_diag */
long diag_dispatch(uint64_t category, uint64_t subcmd, uint64_t buf, uint64_t size);

/* Helper: safe string copy to user buffer */
int diag_strcat_user(diag_output_t *out, const char *str);

/* Helper: format and append to user buffer */
int diag_printf_user(diag_output_t *out, const char *fmt, ...);

/* ================================================================
 * MODULE INITIALIZATION
 * ================================================================ */
extern void diag_register_cpu(void);
extern void diag_register_memory(void);
extern void diag_register_process(void);
extern void diag_register_fs(void);
extern void diag_register_graphics(void);
extern void diag_register_io(void);
extern void diag_register_security(void);
extern void diag_register_ipc(void);
extern void diag_register_timing(void);
extern void diag_register_network(void);
extern void diag_register_debug(void);
extern void diag_register_extra(void);

#endif /* DC_DIAG_H */
