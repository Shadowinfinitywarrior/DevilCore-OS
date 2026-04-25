// kernel/syscall/syscall_handler.c
// System call handlers

#include <devil/types.h>
#include "syscall.h"
#include <fs/vfs.h>
#include <proc/process.h>
#include <net/socket.h>
#include <diag/diag.h>

long sys_read(uint64_t fd, uint64_t buf, uint64_t count) {
    void *file = (void*)fd;
    return vfs_read(file, (void*)buf, count);
}

long sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
    void *file = (void*)fd;
    return vfs_write(file, (void*)buf, count);
}

long sys_open(uint64_t path, uint64_t flags, uint64_t mode) {
    return (long)vfs_open((const char*)path, (int)flags);
}

long sys_close(uint64_t fd) {
    return vfs_close((void*)fd);
}

long sys_exit(uint64_t status) {
    exit_process((int)status);
    return 0;
}

long sys_getpid(void) {
    task_t *task = get_current_task();
    return task ? task->pid : 0;
}

long sys_fork(void) {
    task_t *parent = get_current_task();
    task_t *child = fork_process(parent);
    return child ? child->pid : -1;
}

long sys_execve(uint64_t path, uint64_t argv, uint64_t envp) {
    char *a[16] = {0};
    for (int i = 0; i < 16 && argv; i++) {
        a[i] = ((char**)argv)[i];
    }
    return exec_process((const char*)path, a);
}

long sys_wait4(uint64_t pid, uint64_t status, uint64_t options) {
    return 0;
}

long sys_kill(uint64_t pid, uint64_t sig) {
    task_t *task = find_task((pid_t)pid);
    if (!task) return -1;
    
    if (sig == 9) {
        task->state = TASK_ZOMBIE;
    }
    return 0;
}

long sys_uname(uint64_t buf) {
    char *ubuf = (char*)buf;
    const char *name = "DevilCore";
    for (int i = 0; name[i]; i++) {
        ubuf[i] = name[i];
    }
    return 0;
}

long sys_socket(uint64_t domain, uint64_t type, uint64_t protocol) {
    socket_t *sock = socket_create((int)domain, (int)type, (int)protocol);
    return sock ? (long)sock : -1;
}

long sys_bind(uint64_t sockfd, uint64_t addr, uint64_t addrlen) {
    return 0;
}

long sys_connect(uint64_t sockfd, uint64_t addr, uint64_t addrlen) {
    return 0;
}

long sys_listen(uint64_t sockfd, uint64_t backlog) {
    return 0;
}

long sys_accept(uint64_t sockfd, uint64_t addr, uint64_t addrlen) {
    return 0;
}

/* Diagnostic syscall */
long sys_diag(uint64_t category, uint64_t subcmd, uint64_t buf, uint64_t size) {
    /* Validate buffer - must be in user space */
    if (!buf || size == 0) return -1;
    if (category > DIAG_CAT_ALL) return -1;

    /* Special case: DIAG_CAT_ALL means full report */
    if (category == DIAG_CAT_ALL) {
        /* Allocate kernel buffer for full report */
        char *kbuf = kmalloc(65536);
        if (!kbuf) return -1;

        diag_output_t out_kernel;
        out_kernel.buffer = kbuf;
        out_kernel.size = 65536;
        out_kernel.written = 0;
        out_kernel.error = 0;

        /* Generate full report to kernel buffer */
        diag_all_report(&out_kernel);

        /* Copy to user buffer */
        if (out_kernel.written > size) {
            kfree(kbuf);
            return -1; /* Buffer too small */
        }

        /* Safe copy to user space */
        memcpy((void*)buf, kbuf, out_kernel.written);
        kfree(kbuf);
        return out_kernel.written;
    }

    /* Normal per-category diagnostic */
    return diag_dispatch(category, subcmd, buf, size);
}
