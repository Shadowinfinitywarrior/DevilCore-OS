#ifndef DEVILCORE_VMA_H
#define DEVILCORE_VMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VMA_READ    (1 << 0)
#define VMA_WRITE   (1 << 1)
#define VMA_EXEC    (1 << 2)
#define VMA_USER    (1 << 3)
#define VMA_STACK   (1 << 4)
#define VMA_HEAP    (1 << 5)

struct vma {
    uint64_t start;
    uint64_t end;
    uint32_t flags;
    struct vma *next;
};

struct mm_struct {
    struct vma *vma_list;
    uint64_t pml4_phys;
    
    // Quick access to common regions
    uint64_t stack_base;
    uint64_t heap_base;
    uint64_t heap_current;
};

void mm_init(struct mm_struct *mm);
int mm_add_vma(struct mm_struct *mm, uint64_t start, uint64_t size, uint32_t flags);
struct vma *mm_find_vma(struct mm_struct *mm, uint64_t addr);
void mm_destroy(struct mm_struct *mm);

// Page fault handler helper
int handle_page_fault(uint64_t addr, uint64_t error_code);

#endif
