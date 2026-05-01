#include "syscall.h"

#include "vfs.h"
#include "scheduler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static struct syscall_handler syscall_table[SYSCALL_MAX];
static uint32_t syscall_count;

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    syscall_count = 0;
}

int syscall_register(struct syscall_handler *handler) {
    if (handler == NULL || handler->number >= SYSCALL_MAX) {
        return -1;
    }
    
    syscall_table[handler->number] = *handler;
    ++syscall_count;
    
    return 0;
}

int syscall_handle(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    if (num >= SYSCALL_MAX || syscall_table[num].handler == NULL) {
        return -1;
    }
    
    return syscall_table[num].handler(a0, a1, a2, a3, 0);
}

int syscall_exit(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    
    task_exit((int32_t)status);
    return 0;
}

int syscall_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, uint64_t a4) {
    (void)fd;
    (void)a3;
    (void)a4;
    
    if (buf == 0) {
        return -1;
    }
    
    struct vfs_file file;
    if (vfs_open(&file, "/dev/stdin", 0) < 0) {
        return -1;
    }
    
    size_t read_bytes = 0;
    int result = vfs_read(&file, (void *)buf, (size_t)count, &read_bytes);
    vfs_close(&file);
    
    return result >= 0 ? (int)read_bytes : result;
}

int syscall_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, uint64_t a4) {
    (void)a3;
    (void)a4;
    
    if (buf == 0) {
        return -1;
    }
    
    struct vfs_file file;
    const char *path = (fd == 1) ? "/dev/stdout" : (fd == 2) ? "/dev/stderr" : "/dev/stdout";
    
    if (vfs_open(&file, path, 0) < 0) {
        return -1;
    }
    
    size_t written_bytes = 0;
    int result = vfs_write(&file, (const void *)buf, (size_t)count, &written_bytes);
    vfs_close(&file);
    
    return result >= 0 ? (int)written_bytes : result;
}

int syscall_open(uint64_t path, uint64_t flags, uint64_t mode, uint64_t a3, uint64_t a4) {
    (void)flags;
    (void)mode;
    (void)a3;
    (void)a4;
    
    struct vfs_file file;
    return vfs_open(&file, (const char *)path, 0);
}

int syscall_close(uint64_t fd, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    (void)fd;
    
    return 0;
}

int syscall_getpid(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    
    return (int)task_getpid();
}

int syscall_mkdir(uint64_t path, uint64_t mode, uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)mode;
    (void)a2;
    (void)a3;
    (void)a4;
    
    return vfs_mkdir((const char *)path, 0);
}

int syscall_rmdir(uint64_t path, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4) {
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    
    return vfs_rmdir((const char *)path);
}