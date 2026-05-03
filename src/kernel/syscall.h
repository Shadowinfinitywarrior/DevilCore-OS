#ifndef DEVILCORE_SYSCALL_H
#define DEVILCORE_SYSCALL_H

#include <stdint.h>

#define SYSCALL_MAX 128

#define SYSCALL_EXIT         1
#define SYSCALL_FORK        2
#define SYSCALL_READ       3
#define SYSCALL_WRITE      4
#define SYSCALL_OPEN      5
#define SYSCALL_CLOSE     6
#define SYSCALL_WAITPID   7
#define SYSCALL_CREAT    8
#define SYSCALL_LINK     9
#define SYSCALL_UNLINK  10
#define SYSCALL_CHDIR   12
#define SYSCALL_GETPID  20
#define SYSCALL_MKDIR   39
#define SYSCALL_RMDIR   40
#define SYSCALL_DUP     41
#define SYSCALL_DUP2    42
#define SYSCALL_GETCWD  17
#define SYSCALL_MMAP    90
#define SYSCALL_MUNMAP  91
#define SYSCALL_MPROTECT 92
#define SYSCALL_EXECVE  59

struct syscall_handler {
    uint64_t number;
    const char *name;
    int (*handler)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
};

struct registers;

void syscall_init(void);
int syscall_register(struct syscall_handler *handler);
int syscall_handle(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, struct registers *regs);

int syscall_exit(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
int syscall_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, uint64_t a4);
int syscall_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, uint64_t a4);
int syscall_open(uint64_t path, uint64_t flags, uint64_t mode, uint64_t a3, uint64_t a4);
int syscall_close(uint64_t fd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
int syscall_getpid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
int syscall_mkdir(uint64_t path, uint64_t mode, uint64_t a2, uint64_t a3, uint64_t a4);
int syscall_rmdir(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
int syscall_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd);
int syscall_execve(uint64_t path, uint64_t argv, uint64_t envp, struct registers *regs);

#endif