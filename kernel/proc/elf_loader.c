/*
 * DevilCore OS — ELF64 Binary Loader
 */
#include "process.h"
#include "../include/elf.h"
#include "../fs/vfs.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../libc/string.h"

uint64_t elf_load(vfs_node_t *file, uint64_t pml4) {
    if (!file) return 0;

    elf64_header_t header;
    if (vfs_read(file, 0, sizeof(elf64_header_t), (uint8_t*)&header) != sizeof(elf64_header_t)) {
        return 0;
    }

    /* 1. Verify ELF identity */
    if (*(uint32_t*)header.e_ident != ELF_MAGIC) return 0;
    if (header.e_ident[4] != 2) return 0; /* Must be 64-bit */
    if (header.e_type != 2) return 0;     /* Must be Executable */

    /* 2. Load Program Segments */
    for (int i = 0; i < header.e_phnum; i++) {
        elf64_phdr_t phdr;
        uint32_t offset = header.e_phoff + (i * header.e_phentsize);
        vfs_read(file, offset, sizeof(elf64_phdr_t), (uint8_t*)&phdr);

        if (phdr.p_type == PT_LOAD) {
            /* Map segment into virtual memory */
            uint64_t pages = (phdr.p_memsz + 4095) / 4096;
            
            /* Allocate physical frames and map them */
            for (uint64_t p = 0; p < pages; p++) {
                uint64_t virt = phdr.p_vaddr + (p * 4096);
                uint64_t phys = pmm_alloc_frame();
                vmm_map_page_in(pml4, virt, phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            }

            /* Note: To copy data into a different address space, we temporarily
             * switch to it or use a kernel half mapping. 
             * For now, if pml4 is the current one, we can just memset/memcpy. */
            memset((void*)phdr.p_vaddr, 0, phdr.p_memsz);
            vfs_read(file, (uint32_t)phdr.p_offset, (uint32_t)phdr.p_filesz, (uint8_t*)phdr.p_vaddr);
        }
    }

    return header.e_entry;
}