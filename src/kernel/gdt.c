#include "gdt.h"
#include <string.h>

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_ptr gdt_p;
static struct tss_entry tss;

extern void gdt_flush(uint64_t);
extern void tss_flush(void);

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F);

    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void write_tss(int32_t num, uint64_t base, uint32_t limit) {
    struct tss_gdt_entry *tss_gdt = (struct tss_gdt_entry *)&gdt[num];
    
    tss_gdt->length = limit & 0xFFFF;
    tss_gdt->base_low = base & 0xFFFF;
    tss_gdt->base_mid = (base >> 16) & 0xFF;
    tss_gdt->flags1 = 0x89; // Present, Executable, Accessed (TSS type)
    tss_gdt->flags2 = (limit >> 16) & 0x0F;
    tss_gdt->base_high = (base >> 24) & 0xFF;
    tss_gdt->base_upper = (base >> 32) & 0xFFFFFFFF;
    tss_gdt->reserved = 0;
}

void gdt_init(void) {
    gdt_p.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_p.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); // Kernel Code (64-bit) - Ring 0
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xA0); // Kernel Data (64-bit) - Ring 0
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); // User Code (64-bit) - Ring 3
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xA0); // User Data (64-bit) - Ring 3

    memset(&tss, 0, sizeof(struct tss_entry));
    tss.iopb_offset = sizeof(struct tss_entry);
    
    // TSS is entries 5 and 6 (it takes 16 bytes in 64-bit mode)
    write_tss(5, (uint64_t)&tss, sizeof(struct tss_entry) - 1);

    gdt_flush((uint64_t)&gdt_p);
    tss_flush();
}

void tss_set_rsp0(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}
