#include "vma.h"
#include "slab.h"
#include "memory.h"
#include "scheduler.h"
#include <string.h>

void mm_init(struct mm_struct *mm) {
    if (!mm) return;
    memset(mm, 0, sizeof(struct mm_struct));
}

int mm_add_vma(struct mm_struct *mm, uint64_t start, uint64_t size, uint32_t flags) {
    if (!mm || size == 0) return -1;
    
    struct vma *new_vma = kmalloc_slab(sizeof(struct vma));
    if (!new_vma) return -1;
    
    new_vma->start = start;
    new_vma->end = start + size;
    new_vma->flags = flags;
    new_vma->next = NULL;
    
    // Add to sorted list
    if (!mm->vma_list || start < mm->vma_list->start) {
        new_vma->next = mm->vma_list;
        mm->vma_list = new_vma;
    } else {
        struct vma *curr = mm->vma_list;
        while (curr->next && curr->next->start < start) {
            curr = curr->next;
        }
        new_vma->next = curr->next;
        curr->next = new_vma;
    }
    
    return 0;
}

struct vma *mm_find_vma(struct mm_struct *mm, uint64_t addr) {
    if (!mm) return NULL;
    
    struct vma *curr = mm->vma_list;
    while (curr) {
        if (addr >= curr->start && addr < curr->end) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void mm_destroy(struct mm_struct *mm) {
    if (!mm) return;
    
    struct vma *curr = mm->vma_list;
    while (curr) {
        struct vma *next = curr->next;
        kfree_slab(curr);
        curr = next;
    }
    mm->vma_list = NULL;
}

// In vma.c - simple page fault handler for now
int handle_page_fault(uint64_t addr, uint64_t error_code) {
    struct task *current = task_get_current();
    if (!current || !current->mm) {
        return -1; // No MM structure for this task
    }
    
    struct vma *vma = mm_find_vma(current->mm, addr);
    if (!vma) {
        return -1; // Segmentation fault
    }
    
    // Check permissions (simplified)
    if ((error_code & 2) && !(vma->flags & VMA_WRITE)) {
        return -1; // Write to read-only page
    }
    
    // Lazy allocation: map a new physical page
    void *phys = pmm_alloc_page();
    if (!phys) return -1; // Out of memory
    
    zero_page(phys_to_virt((uint64_t)phys));
    
    uint64_t flags = PAGE_PRESENT;
    if (vma->flags & VMA_WRITE) flags |= PAGE_WRITABLE;
    if (vma->flags & VMA_USER) flags |= PAGE_USER;
    
    paging_map_page(addr & ~0xfffULL, (uint64_t)phys, flags);
    
    return 0;
}
