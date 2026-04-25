/*
 * DevilCore OS — Physical Memory Manager (PMM) Header
 */
#ifndef DC_PMM_H
#define DC_PMM_H

#include "../include/types.h"

#define PAGE_SIZE 4096
#ifndef ALIGN_UP
#define ALIGN_UP(s, a)   (((s) + (a) - 1) & ~((a) - 1))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(s, a) ((s) & ~((a) - 1))
#endif

/* Initialize the PMM using the maximum memory given by the bootloader */
void pmm_init(uint64_t mem_size);

/* Mark a region of memory as free (available for allocation) */
void pmm_init_region(uint64_t base, uint64_t size);

/* Mark a region of memory as used or reserved */
void pmm_deinit_region(uint64_t base, uint64_t size);

/* Allocate a single 4KB frame and return its physical address, or 0 if out of memory */
uint64_t pmm_alloc_frame(void);

/* Allocate a contiguous block of frames */
uint64_t pmm_alloc_blocks(size_t count);

/* Free a previously allocated 4KB frame */
void pmm_free_frame(uint64_t phys_addr);

/* Return total available memory in bytes */
uint64_t pmm_get_free_memory(void);
uint64_t pmm_get_total_memory(void);

#endif /* DC_PMM_H */