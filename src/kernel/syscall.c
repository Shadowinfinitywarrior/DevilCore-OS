#include "syscall.h"
#include "vfs.h"
#include "scheduler.h"
#include "vma.h"
#include "elf.h"
#include "isr.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static struct syscall_handler syscall_table[SYSCALL_MAX];
static uint32_t syscall_count;

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));
    syscall_count = 0;
    
    struct syscall_handler handlers[] = {
        {SYSCALL_EXIT, "exit", syscall_exit},
        {SYSCALL_READ, "read", syscall_read},
        {SYSCALL_WRITE, "write", syscall_write},
        {SYSCALL_OPEN, "open", syscall_open},
        {SYSCALL_CLOSE, "close", syscall_close},
        {SYSCALL_GETPID, "getpid", syscall_getpid},
        {SYSCALL_MKDIR, "mkdir", syscall_mkdir},
        {SYSCALL_RMDIR, "rmdir", syscall_rmdir},
        {SYSCALL_MMAP, "mmap", syscall_mmap},
    };
    
    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        syscall_register(&handlers[i]);
    }
}

int syscall_register(struct syscall_handler *handler) {
    if (handler == NULL || handler->number >= SYSCALL_MAX) {
        return -1;
    }
    
    syscall_table[handler->number] = *handler;
    ++syscall_count;
    
    return 0;
}

int syscall_handle(uint64_t num, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, struct registers *regs) {
    if (num == SYSCALL_EXECVE) {
        return syscall_execve(a0, a1, a2, regs);
    }

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

int syscall_execve(uint64_t path, uint64_t argv, uint64_t envp, struct registers *regs) {
    (void)argv;
    (void)envp;

    uint64_t entry_point = 0;
    if (elf_load((const char *)path, &entry_point) < 0) {
        return -1;
    }

    struct task *current = task_get_current();
    current->flags |= TASK_FLAG_USER;
    
    // Modify registers for iretq to return to user mode at the ELF entry point
    regs->rip = entry_point;
    regs->cs = 0x1B;      // User Code Selector | 3
    regs->rflags = 0x202; // IF bit set
    regs->rsp = (uint64_t)current->user_stack + USER_STACK_SIZE;
    regs->ss = 0x23;      // User Data Selector | 3
    
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

int syscall_mmap(uint64_t addr, uint64_t length, uint64_t prot, uint64_t flags, uint64_t fd) {
    (void)flags;
    (void)fd;
    
    struct task *current = task_get_current();
    if (!current || !current->mm) return -1;
    
    // If addr is 0, we should find a free region.
    if (addr == 0) {
        uint64_t max_addr = 0x40000000; // Start looking from 1GB
        struct vma *curr = current->mm->vma_list;
        while (curr) {
            if (curr->end > max_addr) max_addr = curr->end;
            curr = curr->next;
        }
        addr = (max_addr + 0xfff) & ~0xfffULL;
    }
    
    uint32_t vma_flags = 0;
    if (prot & 1) vma_flags |= VMA_READ;
    if (prot & 2) vma_flags |= VMA_WRITE;
    if (prot & 4) vma_flags |= VMA_EXEC;
    vma_flags |= VMA_USER;
    
    if (mm_add_vma(current->mm, addr, length, vma_flags) == 0) {
        return (int)addr;
    }
    
    return -1;
}
