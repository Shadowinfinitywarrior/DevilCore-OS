#include "diag.h"
#include "../include/ports.h"
#include "../drivers/pci.h"
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/gdt.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../libc/string.h"

/* Declare extern globals */
extern uint64_t g_total_memory_bytes;

/* ================================================================
 * CPUID UTILITY
 * ================================================================ */
static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile ("cpuid"
        : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
        : "a" (leaf), "c" (subleaf));
}

/* ================================================================
 * CPU VENDOR AND FEATURES (Item 1)
 * ================================================================ */
static void diag_cpu_vendor_features(diag_output_t *out) {
    diag_header(out, "CPU Vendor and Features");

    char vendor[13] = {0};
    uint32_t eax, ebx, ecx, edx;

    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    memcpy(vendor, &ebx, 4);
    memcpy(vendor + 4, &edx, 4);
    memcpy(vendor + 8, &ecx, 4);

    diag_printf_user(out, "  CPU Vendor: %s\n", vendor);
    diag_printf_user(out, "  CPU Max Basic Leaf: %u\n", eax);

    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    uint32_t family = (eax >> 8) & 0xF;
    uint32_t model = (eax >> 4) & 0xF;
    uint32_t stepping = eax & 0xF;
    if (family == 0xF) {
        family += (eax >> 20) & 0xFF;
        model = ((eax >> 16) & 0xF) | ((eax >> 4) & 0xF0);
    }
    diag_printf_user(out, "  CPU Family: %u\n", family);
    diag_printf_user(out, "  CPU Model: %u\n", model);
    diag_printf_user(out, "  Stepping: %u\n", stepping);

    /* Feature flags */
    diag_printf_user(out, "\n  Feature Flags (EDX):\n");
    #define CHECK_FEAT(bit, name) if (edx & (1U << (bit))) diag_printf_user(out, "    %s\n", name)
    CHECK_FEAT(0, "FPU");
    CHECK_FEAT(4, "APIC");
    CHECK_FEAT(5, "CMPXCHG8B");
    CHECK_FEAT(6, "APIC on Chip");
    CHECK_FEAT(7, "SEP (SYSENTER)");
    CHECK_FEAT(8, "MTRR");
    CHECK_FEAT(9, "PGE");
    CHECK_FEAT(11, "PAT");
    CHECK_FEAT(13, "ACPI");
    CHECK_FEAT(16, "CMOV");
    CHECK_FEAT(22, "MMX");
    CHECK_FEAT(23, "FXSR");
    CHECK_FEAT(24, "SSE");
    CHECK_FEAT(25, "SSE2");
    CHECK_FEAT(26, "SSE3");
    CHECK_FEAT(28, "SSSE3");
    #undef CHECK_FEAT

    diag_printf_user(out, "\n  Extended Feature Flags (ECX):\n");
    #define CHECK_EXT(bit, name) if (ecx & (1U << (bit))) diag_printf_user(out, "    %s\n", name)
    CHECK_EXT(4, "SSSE3");
    CHECK_EXT(6, "SSE4.1");
    CHECK_EXT(7, "SSE4.2");
    CHECK_EXT(8, "MOVBE");
    CHECK_EXT(9, "POPCNT");
    CHECK_EXT(12, "F16C");
    CHECK_EXT(13, "RDRAND");
    CHECK_EXT(28, "AVX");
    #undef CHECK_EXT

    cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    if (eax >= 1) {
        diag_printf_user(out, "\n  Extended CPU Features (Leaf 7, EBX):\n");
        #define CHECK_LEAF7_EBX(bit, name) if (ebx & (1U << (bit))) diag_printf_user(out, "    %s\n", name)
        CHECK_LEAF7_EBX(3, "BMI1");
        CHECK_LEAF7_EBX(5, "AVX2");
        CHECK_LEAF7_EBX(8, "BMI2");
        CHECK_LEAF7_EBX(9, "x86_64");
        CHECK_LEAF7_EBX(16, "RDSEED");
        CHECK_LEAF7_EBX(18, "ADX");
        CHECK_LEAF7_EBX(26, "AVX512F");
        #undef CHECK_LEAF7_EBX
    }
}

/* ================================================================
 * MEMORY MAP FROM BOOTLOADER (Item 2)
 * ================================================================ */
static void diag_cpu_memory_map(diag_output_t *out) {
    diag_header(out, "Memory Map (from Bootloader)");

    diag_printf_user(out, "  Total Memory: %u MB (%u bytes)\n",
        (uint32_t)(g_total_memory_bytes / (1024 * 1024)),
        (uint32_t)g_total_memory_bytes);
}

/* ================================================================
 * PCI DEVICES (Item 3)
 * ================================================================ */
static void diag_cpu_pci_devices(diag_output_t *out) {
    diag_header(out, "Detected PCI Devices");

    int count = pci_get_device_count();
    if (count == 0) {
        diag_printf_user(out, "  No PCI devices detected.\n");
        return;
    }

    diag_printf_user(out, "  Bus Dev Fun  Vendor  Device   Class\n");
    diag_printf_user(out, "  --- ---- ----  ------  -------  ------\n");
    for (int i = 0; i < count && i < 64; i++) {
        pci_device_t *d = pci_get_device(i);
        if (d) {
            diag_printf_user(out, "  %02X %03X %03X  0x%04X  0x%04X  0x%02X\n",
                d->bus, d->device, d->function,
                d->vendor_id, d->device_id, d->class_id);
        }
    }
    diag_printf_user(out, "\n  Total: %d\n", count);
}

/* ================================================================
 * IDT TABLE (Item 4)
 * ================================================================ */
static void diag_cpu_idt(diag_output_t *out) {
    diag_header(out, "Interrupt Descriptor Table (IDT)");

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr;

    __asm__ volatile ("sidt %0" : "=m"(idtr));

    void *idt = (void*)idtr.base;
    diag_printf_user(out, "  IDTR Base: 0x%016llX\n", (uint64_t)idt);
    diag_printf_user(out, "  IDT Limit: %u\n", idtr.limit);
}

/* ================================================================
 * GDT ENTRIES (Item 5)
 * ================================================================ */
static void diag_cpu_gdt(diag_output_t *out) {
    diag_header(out, "Global Descriptor Table (GDT)");

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) gdtr;

    __asm__ volatile ("sgdt %0" : "=m"(gdtr));

    diag_printf_user(out, "  GDTR Base: 0x%016llX\n", gdtr.base);
    diag_printf_user(out, "  GDT Limit: %u\n", gdtr.limit);

    uint16_t cs;
    __asm__ volatile ("mov %%cs, %0" : "=r" (cs));
    diag_printf_user(out, "  CS Register: 0x%04X\n", cs);
}

/* ================================================================
 * ACPI VERSION (Item 6)
 * ================================================================ */
static void diag_cpu_acpi(diag_output_t *out) {
    diag_header(out, "ACPI Support");
    diag_printf_user(out, "  ACPI: Not fully implemented in this kernel version.\n");
}

/* ================================================================
 * SMBIOS/DMI (Item 7)
 * ================================================================ */
static void diag_cpu_smbios(diag_output_t *out) {
    diag_header(out, "SMBIOS/DMI Information");
    diag_printf_user(out, "  SMBIOS: Not implemented.\n");
}

/* ================================================================
 * APICS (Item 8)
 * ================================================================ */
static void diag_cpu_apics(diag_output_t *out) {
    diag_header(out, "CPU Cores and APICs");
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    uint32_t apic_id = (ebx >> 24) & 0xFF;
    diag_printf_user(out, "  Local APIC ID: %u\n", apic_id);
}

/* ================================================================
 * CR3 VALUE (Item 9)
 * ================================================================ */
static void diag_cpu_cr3(diag_output_t *out) {
    diag_header(out, "Current Paging Structure (CR3)");
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3));
    diag_printf_user(out, "  CR3 (PML4 Address): 0x%016llX\n", cr3);
}

/* ================================================================
 * RSDP/RSDT/XSDT (Item 10)
 * ================================================================ */
static void diag_cpu_rsdp(diag_output_t *out) {
    diag_header(out, "RSDP and RSDT/XSDT");
    diag_printf_user(out, "  ACPI RSDP: Not yet implemented.\n");
}

/* ================================================================
 * CPU DISPATCHER
 * ================================================================ */
void diag_cpu_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_CPU_VENDOR_FEATURES: diag_cpu_vendor_features(out); break;
        case DIAG_CPU_MEMORY_MAP:      diag_cpu_memory_map(out); break;
        case DIAG_CPU_PCI_DEVICES:     diag_cpu_pci_devices(out); break;
        case DIAG_CPU_IDT:             diag_cpu_idt(out); break;
        case DIAG_CPU_GDT:             diag_cpu_gdt(out); break;
        case DIAG_CPU_ACPI:            diag_cpu_acpi(out); break;
        case DIAG_CPU_SMBIOS:          diag_cpu_smbios(out); break;
        case DIAG_CPU_APICS:           diag_cpu_apics(out); break;
        case DIAG_CPU_CR3:             diag_cpu_cr3(out); break;
        case DIAG_CPU_RSDP:            diag_cpu_rsdp(out); break;
        default:
            diag_header(out, "CPU Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
}
