// kernel/arch/x86_64/paging.c
// Paging and Memory Management

#include <devil/types.h>
#include <devil/config.h>
#include "paging.h"

// Page table entries
typedef struct {
    uint64_t present     : 1;
    uint64_t writable    : 1;
    uint64_t user        : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed    : 1;
    uint64_t dirty       : 1;
    uint64_t huge        : 1;
    uint64_t global      : 1;
    uint64_t avail       : 3;
    uint64_t address     : 40;
} PACKED PTE;

static uint64_t *pml4;
static uint64_t *pdpt;
static uint64_t *pd;
static uint64_t *pt;

static uint64_t *pmm_bitmap;
static uint64_t pmm_pages_total;
static uint64_t pmm_pages_free;
static uint64_t pmm_next_free;

void paging_init(void) {
    // Get memory size from bootloader
    extern uint64_t memory_size;
    
    // Set up PML4
    pml4 = (uint64_t*)0x100000;
    pdpt = (uint64_t*)0x101000;
    pd = (uint64_t*)0x102000;
    pt = (uint64_t*)0x103000;
    
    // Clear all page tables
    for (int i = 0; i < 0x60000; i++)
        ((uint8_t*)0x100000)[i] = 0;
    
    // Identity map first 1GB
    for (uint64_t i = 0; i < 512; i++) {
        // PDPT[0] -> PD at 0x102000
        pdpt[i] = (uint64_t)&pd[i * 512] | 0x03;
        
        // Map 2MB pages
        for (uint64_t j = 0; j < 512; j++) {
            pd[i * 512 + j] = (i * 512 + j) * 0x200000 | 0x83;
        }
    }
    
    // Map kernel at higher half
    // PDPT[511] for kernel higher half
    pdpt[511] = (uint64_t)&pd[512 * 511] | 0x03;
    
    // PML4[511] -> kernel PDPT
    pml4[511] = (uint64_t)pdpt | 0x03;
    
    // Set up PML4 
    pml4[0] = (uint64_t)pdpt | 0x03;
    
    // Load PML4
    __asm__ volatile("mov cr3, %0" :: "r"(pml4));
    
    // Enable PAE
    uint64_t cr4;
    __asm__ volatile("mov cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 5);
    __asm__ volatile("mov cr4, %0" :: "r"(cr4));
    
    // Enable long mode
    uint64_t msr;
    __asm__ volatile("rdmsr" : "=a"(msr) : "c"(0xC0000080));
    msr |= (1 << 8);
    __asm__ volatile("wrmsr" :: "a"(msr), "c"(0xC0000080));
    
    // Enable paging
    uint64_t cr0;
    __asm__ volatile("mov cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31);
    __asm__ volatile("mov cr0, %0" :: "r"(cr0));
}

void vmm_map_page(uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    // Calculate page table indices
    uint64_t pml4_idx = (vaddr >> PML4_SHIFT) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> PDPT_SHIFT) & 0x1FF;
    uint64_t pd_idx = (vaddr >> PD_SHIFT) & 0x1FF;
    uint64_t pt_idx = (vaddr >> PT_SHIFT) & 0x1FF;
    
    // Create page tables if needed
    if (!pml4[pml4_idx]) {
        // Allocate new page directory pointer table
        uint64_t new_pdpt = (uint64_t)pmm_alloc();
        for (int i = 0; i < 4096; i++)
            ((uint8_t*)new_pdpt)[i] = 0;
        pml4[pml4_idx] = new_pdpt | 0x03;
    }
    
    // Get PDPT
    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    
    // Create page directory if needed
    if (!pdpt[pdpt_idx]) {
        uint64_t new_pd = (uint64_t)pmm_alloc();
        for (int i = 0; i < 4096; i++)
            ((uint8_t*)new_pd)[i] = 0;
        pdpt[pdpt_idx] = new_pd | 0x03;
    }
    
    // Get PD
    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    
    // Create page table if needed
    if (!pd[pd_idx]) {
        uint64_t new_pt = (uint64_t)pmm_alloc();
        for (int i = 0; i < 4096; i++)
            ((uint8_t*)new_pt)[i] = 0;
        pd[pd_idx] = new_pt | 0x03;
    }
    
    // Get PT
    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
    
    // Map the page
    pt[pt_idx] = paddr | flags | 0x01;
}

void vmm_unmap_page(uint64_t vaddr) {
    uint64_t pml4_idx = (vaddr >> PML4_SHIFT) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> PDPT_SHIFT) & 0x1FF;
    uint64_t pd_idx = (vaddr >> PD_SHIFT) & 0x1FF;
    uint64_t pt_idx = (vaddr >> PT_SHIFT) & 0x1FF;
    
    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    if (!pdpt) return;
    
    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    if (!pd) return;
    
    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
    if (!pt) return;
    
    pt[pt_idx] = 0;
    
    // Flush TLB
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr));
}

uint64_t vmm_get_physical(uint64_t vaddr) {
    uint64_t pml4_idx = (vaddr >> PML4_SHIFT) & 0x1FF;
    uint64_t pdpt_idx = (vaddr >> PDPT_SHIFT) & 0x1FF;
    uint64_t pd_idx = (vaddr >> PD_SHIFT) & 0x1FF;
    uint64_t pt_idx = (vaddr >> PT_SHIFT) & 0x1FF;
    
    uint64_t *pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    if (!pdpt) return 0;
    
    uint64_t *pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    if (!pd) return 0;
    
    uint64_t *pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
    if (!pt) return 0;
    
    return (pt[pt_idx] & ~0xFFF) + (vaddr & 0xFFF);
}