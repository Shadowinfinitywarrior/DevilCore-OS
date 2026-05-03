#include "elf.h"
#include "vfs.h"
#include "memory.h"
#include "vma.h"
#include "scheduler.h"
#include <string.h>

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);

int elf_load(const char *path, uint64_t *entry_point) {
    struct vfs_file file;
    if (vfs_open(&file, path, 0) < 0) {
        serial_write_string("ELF: Failed to open file ");
        serial_write_string(path);
        serial_write_string("\n");
        return -1;
    }

    Elf64_Ehdr header;
    size_t read_bytes = 0;
    if (vfs_read(&file, &header, sizeof(Elf64_Ehdr), &read_bytes) < 0 || read_bytes != sizeof(Elf64_Ehdr)) {
        vfs_close(&file);
        return -1;
    }

    if (memcmp(header.e_ident, "\x7F" "ELF", 4) != 0) {
        serial_write_string("ELF: Invalid magic\n");
        vfs_close(&file);
        return -1;
    }

    struct task *current = task_get_current();
    if (!current || !current->mm) {
        vfs_close(&file);
        return -1;
    }

    *entry_point = header.e_entry;

    for (int i = 0; i < header.e_phnum; i++) {
        Elf64_Phdr phdr;
        vfs_seek(&file, header.e_phoff + (i * header.e_phentsize), 0);
        if (vfs_read(&file, &phdr, sizeof(Elf64_Phdr), &read_bytes) < 0) continue;

        if (phdr.p_type == PT_LOAD) {
            uint32_t vma_flags = VMA_USER;
            if (phdr.p_flags & PF_R) vma_flags |= VMA_READ;
            if (phdr.p_flags & PF_W) vma_flags |= VMA_WRITE;
            if (phdr.p_flags & PF_X) vma_flags |= VMA_EXEC;

            mm_add_vma(current->mm, phdr.p_vaddr, phdr.p_memsz, vma_flags);

            // For now, let's load it immediately instead of pure demand paging for the code/data segments
            // This is safer until demand paging is 100% robust
            uint64_t num_pages = (phdr.p_memsz + 4095) / 4096;
            for (uint64_t p = 0; p < num_pages; p++) {
                void *phys = pmm_alloc_page();
                zero_page(phys_to_virt((uint64_t)phys));
                
                uint64_t map_flags = PAGE_PRESENT | PAGE_USER;
                if (phdr.p_flags & PF_W) map_flags |= PAGE_WRITABLE;
                
                paging_map_page((phdr.p_vaddr + p * 4096) & ~0xfffULL, (uint64_t)phys, map_flags);
            }

            // Read from file into the newly mapped memory
            // We need to temporarily switch CR3 or use hhdm to write to the physical pages.
            // Using HHDM is easier.
            vfs_seek(&file, phdr.p_offset, 0);
            for (uint64_t p = 0; p < num_pages; p++) {
                uint64_t bytes_to_read = 4096;
                if (p * 4096 + bytes_to_read > phdr.p_filesz) {
                    bytes_to_read = (phdr.p_filesz > p * 4096) ? (phdr.p_filesz - p * 4096) : 0;
                }
                
                if (bytes_to_read > 0) {
                    // Find physical address we just mapped
                    // Simplified for now: we know we just mapped it, but in a real kernel we'd walk tables
                    // For this iteration, let's just use the fact that we're in the same turn.
                    // Actually, let's do it properly by seeking and reading into a buffer, then copying.
                    // But wait, we can just read directly into the virtual address if we are in the correct CR3.
                    // Since task_create doesn't switch CR3 yet, we'll use a more robust way in the next pass.
                }
            }
            
            // Refined load:
            vfs_seek(&file, phdr.p_offset, 0);
            // This is a bit tricky because we're loading into another task's address space.
            // For the prototype, we assume we are loading into the 'current' task space.
            // We will use a temporary kernel buffer to bridge.
            uint8_t *buf = kmalloc(4096);
            for (uint64_t p = 0; p < num_pages; p++) {
                uint64_t bytes_in_file = (phdr.p_filesz > p * 4096) ? (phdr.p_filesz - p * 4096) : 0;
                if (bytes_in_file > 4096) bytes_in_file = 4096;
                
                memset(buf, 0, 4096);
                if (bytes_in_file > 0) {
                    vfs_read(&file, buf, bytes_in_file, &read_bytes);
                }
                
                // Copy to the virtual address (this works because paging_map_page mapped it into current CR3 if we are careful)
                // Actually, paging_map_page maps into the *current* CR3. 
                // So this only works if we load the ELF into the current task.
                memcpy((void*)((phdr.p_vaddr + p * 4096) & ~0xfffULL), buf, bytes_in_file);
            }
            kfree(buf);
        }
    }

    vfs_close(&file);
    return 0;
}
