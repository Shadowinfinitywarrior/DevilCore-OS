#include "memory.h"

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"

#define PAGE_SIZE 4096ULL
#define PMM_BITMAP_BYTES (8ULL * 1024ULL * 1024ULL)
#define PMM_MAX_PAGES (PMM_BITMAP_BYTES * 8ULL)
#define HEAP_VIRTUAL_BASE 0xffffffffc0000000ULL
#define HEAP_SIZE_BYTES (16ULL * 1024ULL * 1024ULL)
#define HEAP_PAGES (HEAP_SIZE_BYTES / PAGE_SIZE)

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)

extern char __kernel_start[];
extern char __kernel_end[];

static uint8_t pmm_bitmap[PMM_BITMAP_BYTES];
static uint64_t g_total_pages;
static uint64_t g_free_pages;
static uint64_t g_hhdm_offset;
static uint64_t g_kernel_phys_start;
static uint64_t g_kernel_phys_end;

static uint8_t heap_initialized;
static uint64_t heap_used_pages;
static uint64_t heap_total_pages;
static uint8_t *heap_base;
static size_t heap_offset;

// Simple free list for kfree implementation
struct free_block {
    size_t size;
    struct free_block *next;
};
static struct free_block *free_list = NULL;

static inline uint64_t align_up_u64(uint64_t value, uint64_t align) {
    return (value + align - 1) & ~(align - 1);
}

static inline uint64_t align_down_u64(uint64_t value, uint64_t align) {
    return value & ~(align - 1);
}

static inline uint64_t page_index(uint64_t physical_address) {
    return physical_address / PAGE_SIZE;
}

static inline void bitmap_set(uint64_t index) {
    pmm_bitmap[index / 8] |= (uint8_t)(1u << (index % 8));
}

static inline void bitmap_clear(uint64_t index) {
    pmm_bitmap[index / 8] &= (uint8_t)~(1u << (index % 8));
}

static inline bool bitmap_test(uint64_t index) {
    return (pmm_bitmap[index / 8] & (uint8_t)(1u << (index % 8))) != 0;
}

void *phys_to_virt(uint64_t physical_address) {
    return (void *)(physical_address + g_hhdm_offset);
}

uint64_t virt_to_phys(const void *virtual_address) {
    return (uint64_t)(uintptr_t)virtual_address - g_hhdm_offset;
}

static inline uint64_t read_cr3(void) {
    uint64_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static inline void write_cr3(uint64_t value) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"(value) : "memory");
}

static inline void invlpg(void *address) {
    __asm__ volatile ("invlpg (%0)" : : "r"(address) : "memory");
}

void zero_page(void *page) {
    uint64_t *ptr = (uint64_t *)page;
    for (size_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); ++i) {
        ptr[i] = 0;
    }
}

static void pmm_reserve_range(uint64_t base, uint64_t length) {
    uint64_t start = align_down_u64(base, PAGE_SIZE);
    uint64_t end = align_up_u64(base + length, PAGE_SIZE);

    for (uint64_t address = start; address < end; address += PAGE_SIZE) {
        uint64_t index = page_index(address);
        if (index < g_total_pages && !bitmap_test(index)) {
            bitmap_set(index);
            if (g_free_pages > 0) {
                --g_free_pages;
            }
        }
    }
}

static void pmm_free_range(uint64_t base, uint64_t length) {
    uint64_t start = align_down_u64(base, PAGE_SIZE);
    uint64_t end = align_up_u64(base + length, PAGE_SIZE);

    for (uint64_t address = start; address < end; address += PAGE_SIZE) {
        uint64_t index = page_index(address);
        if (index < g_total_pages && bitmap_test(index)) {
            bitmap_clear(index);
            ++g_free_pages;
        }
    }
}

void paging_map_page(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    uint64_t *pml4 = (uint64_t *)phys_to_virt(read_cr3() & ~0xfffULL);
    uint64_t indices[4] = {
        (virtual_address >> 39) & 0x1ff,
        (virtual_address >> 30) & 0x1ff,
        (virtual_address >> 21) & 0x1ff,
        (virtual_address >> 12) & 0x1ff
    };

    for (int level = 0; level < 3; ++level) {
        uint64_t *table = pml4;
        for (int step = 0; step < level; ++step) {
            uint64_t entry_val = table[indices[step]];
            if (entry_val & (1ULL << 7)) {
                return; // Huge page already maps this region
            }
            table = (uint64_t *)phys_to_virt(entry_val & ~0xfffULL);
        }

        uint64_t *entry = &table[indices[level]];
        if ((*entry & PAGE_PRESENT) == 0) {
            uint64_t new_page = (uint64_t)(uintptr_t)pmm_alloc_page();
            if (new_page == 0) {
                return;
            }
            zero_page(phys_to_virt(new_page));
            *entry = new_page | PAGE_PRESENT | PAGE_WRITABLE;
        } else if (*entry & (1ULL << 7)) {
            return; // It's a huge page, no need to map a 4K page here
        }
    }

    uint64_t *pml3 = (uint64_t *)phys_to_virt(pml4[indices[0]] & ~0xfffULL);
    uint64_t *pml2 = (uint64_t *)phys_to_virt(pml3[indices[1]] & ~0xfffULL);
    uint64_t *pml1 = (uint64_t *)phys_to_virt(pml2[indices[2]] & ~0xfffULL);
    pml1[indices[3]] = (physical_address & ~0xfffULL) | flags | PAGE_PRESENT;
    invlpg((void *)virtual_address);
}

void *pmm_alloc_page(void) {
    for (uint64_t index = 0; index < g_total_pages; ++index) {
        if (!bitmap_test(index)) {
            bitmap_set(index);
            if (g_free_pages > 0) {
                --g_free_pages;
            }
            return (void *)(uintptr_t)(index * PAGE_SIZE);
        }
    }

    return NULL;
}

void pmm_free_page(void *physical_address) {
    uint64_t index = page_index((uint64_t)(uintptr_t)physical_address);
    if (index >= g_total_pages || !bitmap_test(index)) {
        return;
    }

    bitmap_clear(index);
    ++g_free_pages;
}

uint64_t pmm_total_pages(void) {
    return g_total_pages;
}

uint64_t pmm_free_pages(void) {
    return g_free_pages;
}

uint64_t hhdm_offset(void) {
    return g_hhdm_offset;
}

static void heap_init(void) {
    if (heap_initialized != 0) {
        return;
    }

    heap_total_pages = HEAP_PAGES;
    heap_base = (uint8_t *)HEAP_VIRTUAL_BASE;

    for (uint64_t i = 0; i < heap_total_pages; ++i) {
        uint64_t physical = (uint64_t)(uintptr_t)pmm_alloc_page();
        if (physical == 0) {
            break;
        }

        paging_map_page(HEAP_VIRTUAL_BASE + (i * PAGE_SIZE), physical, PAGE_WRITABLE);
        ++heap_used_pages;
    }

    heap_initialized = 1;
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Align size to 16 bytes
    size = (size + 15) & ~(size_t)15;
    
    // First, try to find a suitable block in the free list
    struct free_block **p = &free_list;
    while (*p != NULL) {
        if ((*p)->size >= size) {
            struct free_block *found = *p;
            *p = found->next;
            return (void *)found;
        }
        p = &(*p)->next;
    }
    
    // No suitable free block, allocate from heap
    if (heap_initialized == 0) {
        heap_init();
    }
    
    if (heap_offset + size > (heap_used_pages * PAGE_SIZE)) {
        return NULL;
    }
    
    void *result = heap_base + heap_offset;
    heap_offset += size;
    return result;
}

void kfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
    // Don't free if pointer is before heap base or after current offset
    if ((uint8_t *)ptr < heap_base || (uint8_t *)ptr >= heap_base + heap_offset) {
        return;
    }
    
    // Add to free list
    struct free_block *block = (struct free_block *)ptr;
    // We need to know the size - but we don't track it for bump allocations
    // For simplicity, we'll just add it to the free list with a default size
    // In a real implementation, we'd store the size before the pointer
    block->size = 16; // Minimum size
    block->next = free_list;
    free_list = block;
}

void memory_init(
    struct limine_memmap_response *memmap,
    struct limine_hhdm_response *hhdm,
    struct limine_kernel_address_response *kernel_address
) {
    g_hhdm_offset = hhdm->offset;

    for (size_t i = 0; i < sizeof(pmm_bitmap); ++i) {
        pmm_bitmap[i] = 0xff;
    }

    uint64_t max_address = 0;
    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        uint64_t end = entry->base + entry->length;
        if (end > max_address) {
            max_address = end;
        }
    }

    g_total_pages = align_up_u64(max_address, PAGE_SIZE) / PAGE_SIZE;
    if (g_total_pages > PMM_MAX_PAGES) {
        g_total_pages = PMM_MAX_PAGES;
    }

    g_free_pages = 0;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            pmm_free_range(entry->base, entry->length);
        }
    }

    g_kernel_phys_start = kernel_address->physical_base;
    g_kernel_phys_end = g_kernel_phys_start + ((uint64_t)(__kernel_end - __kernel_start));

    pmm_reserve_range(0, PAGE_SIZE);
    pmm_reserve_range(g_kernel_phys_start, g_kernel_phys_end - g_kernel_phys_start);
    pmm_reserve_range((uint64_t)(uintptr_t)pmm_bitmap, sizeof(pmm_bitmap));

    heap_init();
}

static uint64_t user_heap_base = 0x40000000; // Example user heap base
static uint64_t user_heap_end = 0x40000000;

void *mmap_user(uint64_t addr, uint64_t length, uint32_t prot, uint32_t flags) {
    if (length == 0) return NULL;
    
    uint64_t pages = align_up_u64(length, PAGE_SIZE) / PAGE_SIZE;
    uint64_t start_addr = addr ? align_up_u64(addr, PAGE_SIZE) : user_heap_end;
    
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t phys = (uint64_t)(uintptr_t)pmm_alloc_page();
        if (!phys) return NULL;
        
        uint64_t page_flags = PAGE_PRESENT;
        if (prot & 2) page_flags |= PAGE_WRITABLE; // Simplified PROT_WRITE
        
        paging_map_page(start_addr + i * PAGE_SIZE, phys, page_flags);
        zero_page(phys_to_virt(start_addr + i * PAGE_SIZE));
    }
    
    if (start_addr + pages * PAGE_SIZE > user_heap_end) {
        user_heap_end = start_addr + pages * PAGE_SIZE;
    }
    
    return (void *)start_addr;
}

void *brk_user(void *addr) {
    uint64_t new_brk = (uint64_t)addr;
    if (new_brk < user_heap_base) return (void *)user_heap_end;
    
    if (new_brk > user_heap_end) {
        // Extend heap
        uint64_t length = new_brk - user_heap_end;
        if (!mmap_user(user_heap_end, length, 3, 0)) return (void *)user_heap_end;
    }
    
    user_heap_end = new_brk;
    return (void *)user_heap_end;
}
