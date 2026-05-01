#ifndef DEVILCORE_MEMORY_H
#define DEVILCORE_MEMORY_H

#include <stddef.h>
#include <stdint.h>

// Define page table flags here to make them accessible
#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)

struct limine_memmap_response;
struct limine_hhdm_response;
struct limine_kernel_address_response;

void memory_init(
    struct limine_memmap_response *memmap,
    struct limine_hhdm_response *hhdm,
    struct limine_kernel_address_response *kernel_address
);

void *kmalloc(size_t size);
void kfree(void *ptr);

void *pmm_alloc_page(void);
void pmm_free_page(void *physical_address);

uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);
uint64_t hhdm_offset(void);

void *phys_to_virt(uint64_t physical_address);
uint64_t virt_to_phys(const void *virtual_address);

void paging_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);
void zero_page(void *page);

#endif
