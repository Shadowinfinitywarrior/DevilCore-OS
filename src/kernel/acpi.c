#include "acpi.h"
#include "memory.h"
#include <stddef.h>

static void *rsdt_xsdt = NULL;
static int is_xsdt = 0;

#define PAGE_WRITABLE (1ULL << 1)

static inline void *phys_to_virt_acpi(uint64_t physical_address) {
    // Map the physical address just in case it's not mapped by Limine.
    // We map a 4K page around it.
    uint64_t phys_aligned = physical_address & ~0xfffULL;
    uint64_t virt_aligned = phys_aligned + hhdm_offset();
    paging_map_page(virt_aligned, phys_aligned, PAGE_WRITABLE);
    
    return (void *)(physical_address + hhdm_offset());
}

void acpi_init(void *rsdp_ptr) {
    if (!rsdp_ptr) {
        // Fallback for BIOS: search 0xE0000 - 0xFFFFF for "RSD PTR "
        for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 16) {
            void *ptr = phys_to_virt_acpi(addr);
            if (((char*)ptr)[0] == 'R' && ((char*)ptr)[1] == 'S' &&
                ((char*)ptr)[2] == 'D' && ((char*)ptr)[3] == ' ' &&
                ((char*)ptr)[4] == 'P' && ((char*)ptr)[5] == 'T' &&
                ((char*)ptr)[6] == 'R' && ((char*)ptr)[7] == ' ') {
                rsdp_ptr = ptr;
                break;
            }
        }
    }
    
    if (!rsdp_ptr) return;
    
    // Map the page containing RSDP
    uint64_t rsdp_virt = (uint64_t)(uintptr_t)rsdp_ptr;
    uint64_t rsdp_phys = rsdp_virt - hhdm_offset();
    paging_map_page(rsdp_virt & ~0xfffULL, rsdp_phys & ~0xfffULL, PAGE_WRITABLE);
    
    struct rsdp_descriptor *rsdp = (struct rsdp_descriptor *)rsdp_ptr;
    
    if (rsdp->revision >= 2) {
        struct rsdp_descriptor_v2 *rsdp2 = (struct rsdp_descriptor_v2 *)rsdp;
        if (rsdp2->xsdt_address) {
            rsdt_xsdt = phys_to_virt_acpi(rsdp2->xsdt_address);
            is_xsdt = 1;
            return;
        }
    }
    
    if (rsdp->rsdt_address) {
        rsdt_xsdt = phys_to_virt_acpi(rsdp->rsdt_address);
        is_xsdt = 0;
    }
}

void *acpi_find_table(const char *signature) {
    if (!rsdt_xsdt) return NULL;
    
    struct sdt_header *header = (struct sdt_header *)rsdt_xsdt;
    int entries = (header->length - sizeof(struct sdt_header)) / (is_xsdt ? 8 : 4);
    
    for (int i = 0; i < entries; i++) {
        struct sdt_header *target = NULL;
        if (is_xsdt) {
            uint64_t *ptr = (uint64_t *)((uint8_t *)header + sizeof(struct sdt_header));
            target = (struct sdt_header *)phys_to_virt_acpi(ptr[i]);
        } else {
            uint32_t *ptr = (uint32_t *)((uint8_t *)header + sizeof(struct sdt_header));
            target = (struct sdt_header *)phys_to_virt_acpi(ptr[i]);
        }
        
        if (target->signature[0] == signature[0] &&
            target->signature[1] == signature[1] &&
            target->signature[2] == signature[2] &&
            target->signature[3] == signature[3]) {
            return target;
        }
    }
    
    return NULL;
}
