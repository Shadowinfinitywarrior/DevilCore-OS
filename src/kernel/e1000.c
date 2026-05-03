#include "e1000.h"
#include "memory.h"
#include <string.h>

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);

static uint64_t mmio_base = 0;

static void write_reg(uint16_t reg, uint32_t val) {
    *(volatile uint32_t *)(mmio_base + reg) = val;
}

static uint32_t read_reg(uint16_t reg) {
    return *(volatile uint32_t *)(mmio_base + reg);
}

void e1000_init(struct pci_device *pci) {
    serial_write_string("E1000: Initializing...\n");

    // BAR0 is MMIO base
    uint32_t bar0 = pci_read(pci->bus, pci->device, pci->function, 0x10);
    uint64_t phys_base = (uint64_t)(bar0 & ~0xf);
    
    // Map E1000 MMIO region (typically 128KB)
    for (uint64_t offset = 0; offset < 0x20000; offset += PAGE_SIZE) {
        paging_map_page(0xFFFF800000000000ULL + phys_base + offset, phys_base + offset, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    mmio_base = 0xFFFF800000000000ULL + phys_base;

    serial_write_string("E1000: MMIO at ");
    serial_write_hex_u64(mmio_base);
    serial_write_string("\n");

    pci_enable_mmio(pci->bus, pci->device, pci->function);
    pci_enable_bus_master(pci->bus, pci->device, pci->function);

    // Get MAC address from EEPROM (simplified)
    uint32_t mac_low = read_reg(REG_RAL);
    uint32_t mac_high = read_reg(REG_RAH);

    serial_write_string("E1000: MAC Address: ");
    serial_write_hex_u64(mac_low);
    serial_write_hex_u64(mac_high);
    serial_write_string("\n");
    
    // Further initialization (TX/RX queues) would go here
    write_reg(REG_CTRL, read_reg(REG_CTRL) | (1 << 26)); // ASDE
}
