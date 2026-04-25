// kernel/syscall/syscall_table.c

#include <devil/types.h>
#include "syscall.h"

typedef long (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

extern long sys_read(uint64_t fd, uint64_t buf, uint64_t count);
extern long sys_write(uint64_t fd, uint64_t buf, uint64_t count);
extern long sys_open(uint64_t path, uint64_t flags, uint64_t mode);
extern long sys_close(uint64_t fd);
extern long sys_exit(uint64_t status);
extern long sys_getpid(void);
extern long sys_fork(void);
extern long sys_execve(uint64_t path, uint64_t argv, uint64_t envp);
extern long sys_wait4(uint64_t pid, uint64_t status, uint64_t options);
extern long sys_kill(uint64_t pid, uint64_t sig);
extern long sys_uname(uint64_t buf);
extern long sys_socket(uint64_t domain, uint64_t type, uint64_t protocol);
extern long sys_bind(uint64_t sockfd, uint64_t addr, uint64_t addrlen);
extern long sys_connect(uint64_t sockfd, uint64_t addr, uint64_t addrlen);
extern long sys_listen(uint64_t sockfd, uint64_t backlog);
extern long sys_accept(uint64_t sockfd, uint64_t addr, uint64_t addrlen);

static syscall_handler_t syscall_table[NR_SYSCALLS] = {
    [SYS_read]        = (syscall_handler_t)sys_read,
    [SYS_write]       = (syscall_handler_t)sys_write,
    [SYS_open]        = (syscall_handler_t)sys_open,
    [SYS_close]       = (syscall_handler_t)sys_close,
    [SYS_exit]        = (syscall_handler_t)sys_exit,
    [SYS_getpid]     = (syscall_handler_t)sys_getpid,
    [SYS_fork]       = (syscall_handler_t)sys_fork,
    [SYS_execve]     = (syscall_handler_t)sys_execve,
    [SYS_wait4]     = (syscall_handler_t)sys_wait4,
    [SYS_kill]       = (syscall_handler_t)sys_kill,
    [SYS_uname]      = (syscall_handler_t)sys_uname,
    [SYS_socket]     = (syscall_handler_t)sys_socket,
    [SYS_bind]       = (syscall_handler_t)sys_bind,
    [SYS_connect]    = (syscall_handler_t)sys_connect,
    [SYS_listen]     = (syscall_handler_t)sys_listen,
    [SYS_accept]     = (syscall_handler_t)sys_accept,
};

void *get_syscall_handler(long num) {
    if (num < 0 || num >= NR_SYSCALLS) {
        return NULL;
    }
    return (void*)syscall_table[num];
}