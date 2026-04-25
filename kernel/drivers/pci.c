/*
 * DevilCore OS — PCI Bus Implementation
 */
#include "pci.h"
#include "../libc/string.h"
#include "pic.h" /* For inb, outb, etc. Wait, I should add generic I/O headers or use assembly directly. We have inline asm available. */

#define MAX_PCI_DEVICES 32

static pci_device_t g_pci_devices[MAX_PCI_DEVICES];
static int g_pci_device_count = 0;

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t val = pci_read_config(bus, slot, func, offset);
    return (uint16_t)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

static void check_device(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_read_word(bus, device, function, 0);
    if (vendor_id == 0xFFFF) return; /* Device doesn't exist */

    uint16_t device_id = pci_read_word(bus, device, function, 2);
    uint32_t class_reg = pci_read_config(bus, device, function, 8);
    
    uint8_t class_id = (class_reg >> 24) & 0xFF;
    uint8_t subclass_id = (class_reg >> 16) & 0xFF;
    uint8_t prog_if = (class_reg >> 8) & 0xFF;
    uint8_t revision = class_reg & 0xFF;

    if (g_pci_device_count < MAX_PCI_DEVICES) {
        pci_device_t *dev = &g_pci_devices[g_pci_device_count++];
        dev->bus = bus;
        dev->device = device;
        dev->function = function;
        dev->vendor_id = vendor_id;
        dev->device_id = device_id;
        dev->class_id = class_id;
        dev->subclass_id = subclass_id;
        dev->prog_if = prog_if;
        dev->revision_id = revision;
    }
}

void pci_init(void) {
    g_pci_device_count = 0;
    /* Bruteforce bus scan (Type 0 configuration) */
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            /* Check function 0 to see if device exists */
            uint16_t vendor = pci_read_word((uint8_t)bus, device, 0, 0);
            if (vendor == 0xFFFF) continue;
            
            check_device((uint8_t)bus, device, 0);
            
            /* Check if it's a multi-function device */
            uint32_t header_type_reg = pci_read_config((uint8_t)bus, device, 0, 0x0C);
            uint8_t header_type = (header_type_reg >> 16) & 0xFF;
            
            if (header_type & 0x80) {
                for (uint8_t func = 1; func < 8; func++) {
                    if (pci_read_word((uint8_t)bus, device, func, 0) != 0xFFFF) {
                        check_device((uint8_t)bus, device, func);
                    }
                }
            }
        }
    }
}

int pci_get_device_count(void) {
    return g_pci_device_count;
}

pci_device_t* pci_get_device(int index) {
    if (index >= 0 && index < g_pci_device_count) {
        return &g_pci_devices[index];
    }
    return NULL;
}
