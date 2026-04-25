/*
 * DevilCore OS — System Call Interface
 */
#ifndef DC_SYSCALL_H
#define DC_SYSCALL_H

#include "../include/types.h"

/* Syscall numbers (POSIX-inspired) */
#define SYS_READ    0
#define SYS_WRITE   1
#define SYS_OPEN    2
#define SYS_CLOSE   3
#define SYS_STAT    4
#define SYS_FSTAT   5
#define SYS_LSTAT   6
#define SYS_MMAP    9
#define SYS_MPROTECT 10
#define SYS_MUNMAP  11
#define SYS_BRK     12
#define SYS_IOCTL   16
#define SYS_YIELD   24
#define SYS_EXIT    60
#define SYS_FORK    57
#define SYS_EXECVE  59

/* Initialize the syscall MSRs */
void syscall_init(void);

#endif /* DC_SYSCALL_H */