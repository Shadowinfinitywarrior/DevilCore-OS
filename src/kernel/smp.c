#include "smp.h"
#include "memory.h"
#include <stddef.h>

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);

static void ap_entry(struct limine_smp_info *smp_info) {
    (void)smp_info;
    
    // Disable interrupts on this AP for now
    __asm__ volatile ("cli");
    
    // APs should ideally have their own GDT, IDT, and stack
    // For now, we just loop to prove they are running
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void smp_init(struct limine_smp_response *smp) {
    if (!smp) return;

    serial_write_string("SMP: CPU count: ");
    serial_write_hex_u64(smp->cpu_count);
    serial_write_string("\n");

    for (uint64_t i = 0; i < smp->cpu_count; i++) {
        struct limine_smp_info *cpu = smp->cpus[i];
        if (cpu->lapic_id == smp->bsp_lapic_id) {
            continue; // Skip BSP (it's already running)
        }

        serial_write_string("SMP: Starting AP ");
        serial_write_hex_u64(cpu->processor_id);
        serial_write_string(" (LAPIC ID: ");
        serial_write_hex_u64(cpu->lapic_id);
        serial_write_string(")\n");

        cpu->goto_address = ap_entry;
    }
}
