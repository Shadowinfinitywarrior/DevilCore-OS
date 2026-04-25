// kernel/arch/x86_64/gdt.c

#include <devil/types.h>
#include "gdt.h"

// GDT entry structure
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} PACKED GDTEntry;

// 64-bit TSS descriptor (16 bytes)
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} PACKED TSSDescriptor;

// GDT pointer
typedef struct {
    uint16_t limit;
    uint64_t base;
} PACKED GDTPointer;

// TSS structure
typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;          // Kernel stack for ring 0
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;          // Interrupt Stack Table
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} PACKED TSS;

// GDT entries:
// 0: Null
// 1: Kernel Code (0x08)
// 2: Kernel Data (0x10)
// 3: User Code   (0x18) -- with DPL=3
// 4: User Data   (0x20) -- with DPL=3
// 5-6: TSS       (0x28) -- 16 bytes

#define GDT_ENTRIES 7
static GDTEntry gdt[GDT_ENTRIES] ALIGNED(16);
static GDTPointer gdt_ptr;
static TSS tss ALIGNED(16);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
    gdt[index].base_low    = base & 0xFFFF;
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = limit & 0xFFFF;
    gdt[index].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access       = access;
}

// External assembly function
extern void gdt_flush(uint64_t gdt_ptr);

void gdt_init(void) {
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Kernel Code Segment (64-bit, ring 0)
    gdt_set_entry(1, 0, 0xFFFFF,
        0x9A,       // Present, DPL=0, Code, Exec+Read
        0xA0);      // Long mode, 4KB granularity

    // Kernel Data Segment (64-bit, ring 0)
    gdt_set_entry(2, 0, 0xFFFFF,
        0x92,       // Present, DPL=0, Data, Read+Write
        0xC0);      // 4KB granularity

    // User Code Segment (64-bit, ring 3)
    gdt_set_entry(3, 0, 0xFFFFF,
        0xFA,       // Present, DPL=3, Code, Exec+Read
        0xA0);      // Long mode

    // User Data Segment (64-bit, ring 3)
    gdt_set_entry(4, 0, 0xFFFFF,
        0xF2,       // Present, DPL=3, Data, Read+Write
        0xC0);

    // TSS Descriptor (occupies entries 5 and 6)
    uint64_t tss_addr = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(TSS) - 1;

    // Clear TSS
    for (size_t i = 0; i < sizeof(TSS); i++)
        ((uint8_t*)&tss)[i] = 0;

    tss.iopb_offset = sizeof(TSS);

    // Set TSS descriptor (16 bytes)
    TSSDescriptor *tss_desc = (TSSDescriptor*)&gdt[5];
    tss_desc->limit_low   = tss_limit & 0xFFFF;
    tss_desc->base_low    = tss_addr & 0xFFFF;
    tss_desc->base_middle = (tss_addr >> 16) & 0xFF;
    tss_desc->access      = 0x89;   // Present, TSS (Available)
    tss_desc->granularity = (tss_limit >> 16) & 0x0F;
    tss_desc->base_high   = (tss_addr >> 24) & 0xFF;
    tss_desc->base_upper  = (tss_addr >> 32) & 0xFFFFFFFF;
    tss_desc->reserved    = 0;

    // Load GDT
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdt_ptr);

    // Load TSS
    __asm__ volatile("ltr %%ax" : : "a"(0x28));
}

void tss_set_kernel_stack(uint64_t stack) {
    tss.rsp0 = stack;
}

void tss_init(void) {
    // Set default kernel stack
    extern uint8_t stack_top[];
    tss.rsp0 = (uint64_t)stack_top;

    // Set IST entries for special interrupt stacks
    // IST1: Double fault stack
    // IST2: NMI stack
    // IST3: Page fault stack
    // (Allocate these stacks during memory manager init)
}
