/*
 * DevilCore OS — Virtual Memory Manager (VMM)
 * Manages 4-level x86_64 Paging (PML4 -> PDP -> PD -> PT)
 */
#include "vmm.h"
#include "pmm.h"
#include "../libc/string.h"

/* The kernel's active PML4 table. In `boot/multiboot2.asm`, we set up `pml4_table`. 
 * We will reuse the one set up by the bootloader but manage it here. */
extern uint64_t pml4_table[];

/* Helper to flush TLB */
static inline void invlpg(uint64_t virt_addr) {
    __asm__ volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

/* Helper to load CR3 */
static inline void load_cr3(uint64_t phys_addr) {
    __asm__ volatile("mov %0, %%cr3" ::"r"(phys_addr) : "memory");
}

void vmm_init(void) {
    /* For now, we continue using the identity mapped 4GB set up in boot,
     * but the VMM is ready to dynamically map memory. */
}

uint64_t vmm_create_address_space(void) {
    uint64_t phys = pmm_alloc_frame();
    if (!phys) return 0;

    uint64_t *new_pml4 = (uint64_t *)phys;
    memset(new_pml4, 0, 4096);

    /* Copy kernel mappings (first 512GB via PML4[0])
     * This ensures the new process can still see kernel code/data. */
    new_pml4[0] = pml4_table[0];

    return phys;
}

void vmm_switch_pml4(uint64_t phys) {
    load_cr3(phys);
}

/* Extract indices from virtual address */
#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDP_INDEX(v)  (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

static uint64_t *get_or_create_table(uint64_t *parent_table, uint32_t index) {
    uint64_t entry = parent_table[index];
    if (entry & PAGE_PRESENT) {
        /* Already mapped, return virtual pointer to the table */
        uint64_t phys = entry & ~0xFFFULL;
        /* Since we have identity mapping for lower memory, phys == virt for these tables */
        return (uint64_t *)phys;
    }

    /* Need to allocate a new table */
    uint64_t phys_table = pmm_alloc_frame();
    if (!phys_table) return NULL; /* OOM */

    memset((void*)phys_table, 0, 4096); /* Zero the table */
    parent_table[index] = phys_table | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    
    return (uint64_t *)phys_table;
}

void vmm_map_page_in(uint64_t pml4_phys, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    uint64_t *pml4 = (uint64_t *)pml4_phys;
    uint64_t *pdp = get_or_create_table(pml4, PML4_INDEX(virt_addr));
    if (!pdp) return;
    uint64_t *pd = get_or_create_table(pdp, PDP_INDEX(virt_addr));
    if (!pd) return;
    uint64_t *pt = get_or_create_table(pd, PD_INDEX(virt_addr));
    if (!pt) return;

    /* Map the actual page */
    pt[PT_INDEX(virt_addr)] = (phys_addr & ~0xFFFULL) | (flags & 0xFFF) | PAGE_PRESENT;
    /* Only flush if it's the current address space */
    /* invlpg(virt_addr); */
}

void vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    vmm_map_page_in((uint64_t)pml4_table, virt_addr, phys_addr, flags);
    invlpg(virt_addr);
}

void vmm_unmap_page(uint64_t virt_addr) {
    uint64_t pml4_entry = pml4_table[PML4_INDEX(virt_addr)];
    if (!(pml4_entry & PAGE_PRESENT)) return;
    
    uint64_t *pdp = (uint64_t *)(pml4_entry & ~0xFFFULL);
    uint64_t pdp_entry = pdp[PDP_INDEX(virt_addr)];
    if (!(pdp_entry & PAGE_PRESENT)) return;
    
    uint64_t *pd = (uint64_t *)(pdp_entry & ~0xFFFULL);
    uint64_t pd_entry = pd[PD_INDEX(virt_addr)];
    if (!(pd_entry & PAGE_PRESENT)) return;

    /* If it's a huge 2MB page, unmap at PD level */
    if (pd_entry & PAGE_HUGE) {
        pd[PD_INDEX(virt_addr)] = 0;
        invlpg(virt_addr);
        return;
    }
    
    uint64_t *pt = (uint64_t *)(pd_entry & ~0xFFFULL);
    pt[PT_INDEX(virt_addr)] = 0;
    invlpg(virt_addr);
}

uint64_t vmm_get_phys(uint64_t virt_addr) {
    uint64_t pml4_entry = pml4_table[PML4_INDEX(virt_addr)];
    if (!(pml4_entry & PAGE_PRESENT)) return 0;
    
    uint64_t *pdp = (uint64_t *)(pml4_entry & ~0xFFFULL);
    uint64_t pdp_entry = pdp[PDP_INDEX(virt_addr)];
    if (!(pdp_entry & PAGE_PRESENT)) return 0;
    
    uint64_t *pd = (uint64_t *)(pdp_entry & ~0xFFFULL);
    uint64_t pd_entry = pd[PD_INDEX(virt_addr)];
    if (!(pd_entry & PAGE_PRESENT)) return 0;
    
    if (pd_entry & PAGE_HUGE) {
        return (pd_entry & ~0x1FFFFFULL) + (virt_addr & 0x1FFFFF);
    }
    
    uint64_t *pt = (uint64_t *)(pd_entry & ~0xFFFULL);
    uint64_t pt_entry = pt[PT_INDEX(virt_addr)];
    if (!(pt_entry & PAGE_PRESENT)) return 0;
    
    return (pt_entry & ~0xFFFULL) + (virt_addr & 0xFFF);
}