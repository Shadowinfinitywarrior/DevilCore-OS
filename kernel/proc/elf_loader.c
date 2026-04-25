// kernel/proc/elf_loader.c
// ELF binary loader for user processes

#include <devil/types.h>
#include "process.h"
#include <fs/vfs.h>

typedef struct {
    uint8_t  magic[4];
    uint8_t  class_;
    uint8_t  data;
    uint8_t  version;
    uint8_t  osabi;
    uint8_t  abiversion;
    uint8_t  padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version_;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} PACKED Elf64_Ehdr;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
    uint64_t pheader;
} PACKED Elf64_Phdr;

#define ELF_MAGIC 0x464C457F
#define ELF_TYPE_EXEC 2

static int elf_validate(void *buf) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)buf;
    return ehdr->magic[0] == 0x7F && ehdr->magic[1] == 'E' &&
           ehdr->magic[2] == 'L' && ehdr->magic[3] == 'F';
}

int elf_load(const char *path, char *const argv[]) {
    void *file = vfs_open(path, 0);
    if (!file) return -1;
    
    char buf[4096];
    vfs_read(file, buf, sizeof(buf));
    vfs_close(file);
    
    if (!elf_validate(buf)) return -1;
    
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)buf;
    Elf64_Phdr *phdr = (Elf64_Phdr*)((uint8_t*)buf + ehdr->phoff);
    
    task_t *task = get_current_task();
    if (!task) return -1;
    
    for (int i = 0; i < ehdr->phnum; i++) {
        if (phdr[i].type == 1) { // PT_LOAD
            // Map memory for segment
            // Copy code to user space
        }
    }
    
    task->rip = ehdr->entry;
    return 0;
}