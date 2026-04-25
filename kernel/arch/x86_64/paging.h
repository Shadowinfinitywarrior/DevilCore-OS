#ifndef _PAGING_H
#define _PAGING_H

#include <devil/types.h>

void paging_init(void);
void *pmm_alloc(void);
void pmm_free(void *page);
void vmm_map_page(uint64_t vaddr, uint64_t paddr, uint64_t flags);
void vmm_unmap_page(uint64_t vaddr);
uint64_t vmm_get_physical(uint64_t vaddr);

#define PTE_PRESENT    0x01
#define PTE_WRITABLE   0x02
#define PTE_USER       0x04
#define PTE_WRITE_THROUGH 0x08
#define PTE_CACHE_DISABLE 0x10
#define PTE_ACCESSED    0x20
#define PTE_DIRTY      0x40
#define PTE_HUGE       0x80
#define PTE_GLOBAL     0x100

#define PML4_SHIFT     39
#define PDPT_SHIFT     30
#define PD_SHIFT       21
#define PT_SHIFT       12

#define PAGES_PER_PML4 512
#define PAGES_PER_PDPT 512
#define PAGES_PER_PD   512
#define PAGES_PER_PT   512

#endif // _PAGING_H