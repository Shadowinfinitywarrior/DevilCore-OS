/*
 * DevilCore OS — Virtual Memory Manager (VMM) Header
 */
#ifndef DC_VMM_H
#define DC_VMM_H

#include "../include/types.h"

/* x86_64 Page Table Flags */
#define PAGE_PRESENT  0x01
#define PAGE_WRITABLE 0x02
#define PAGE_USER     0x04
#define PAGE_WRITE_T  0x08
#define PAGE_CACHE_D  0x10
#define PAGE_HUGE     0x80

/* Memory mapped regions */
#define KERNEL_VIRT_BASE 0xFFFF800000000000ULL

/* Initialize Virtual Memory Manager */
void vmm_init(void);

/* Map a virtual page to a physical frame with specific flags */
void vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

/* Unmap a virtual page */
void vmm_unmap_page(uint64_t virt_addr);

/* Get physical address of a mapped virtual page. Returns 0 if not mapped. */
uint64_t vmm_get_phys(uint64_t virt_addr);

#endif /* DC_VMM_H */