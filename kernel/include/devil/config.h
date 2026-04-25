#ifndef _DEVIL_CONFIG_H
#define _DEVIL_CONFIG_H

// Kernel configuration
#define KERNEL_VERSION_MAJOR    0
#define KERNEL_VERSION_MINOR    1
#define KERNEL_VERSION_PATCH    0
#define KERNEL_NAME             "DevilKernel"
#define OS_NAME                 "DevilCore"

// Memory configuration
#define KERNEL_VIRTUAL_BASE     0xFFFFFFFF80000000ULL  // Higher half
#define KERNEL_HEAP_START       0xFFFFFF0000000000ULL
#define KERNEL_HEAP_INITIAL     (16 * 1024 * 1024)     // 16MB initial heap
#define KERNEL_HEAP_MAX         (256 * 1024 * 1024)    // 256MB max heap
#define KERNEL_STACK_SIZE       (64 * 1024)            // 64KB per kernel stack
#define USER_STACK_SIZE         (8 * 1024 * 1024)      // 8MB user stack

// Process limits
#define MAX_PROCESSES           4096
#define MAX_THREADS             16384
#define MAX_FDS_PER_PROCESS     256
#define MAX_FILENAME            255
#define MAX_PATH                4096

// Scheduler
#define SCHEDULER_TICK_HZ       1000    // 1ms tick
#define DEFAULT_TIMESLICE_MS    10      // 10ms default timeslice

// Filesystem
#define MAX_MOUNT_POINTS        64
#define MAX_OPEN_FILES          65536
#define BLOCK_SIZE              4096

// Network
#define MAX_SOCKETS             4096
#define MAX_NETWORK_INTERFACES  16
#define NETWORK_BUFFER_SIZE     65536

// Security
#define ENABLE_MAC              1       // Mandatory Access Control
#define ENABLE_SECCOMP          1       // Syscall filtering
#define ENABLE_AUDIT            1       // Audit logging
#define ENABLE_ENCRYPTION       1       // Full disk encryption support

// Privacy
#define ENABLE_TOR              1       // Built-in Tor support
#define ENABLE_VPN              1       // Built-in VPN
#define ENABLE_DNS_PRIVACY      1       // DNS over HTTPS
#define ENABLE_MAC_RANDOMIZE    1       // MAC address randomization

#endif // _DEVIL_CONFIG_H
