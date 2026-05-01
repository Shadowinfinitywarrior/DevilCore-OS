#include "pci.h"
#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PCI_CLASS_MASS_STORAGE     0x01
#define PCI_CLASS_NETWORK       0x02
#define PCI_CLASS_DISPLAY      0x03
#define PCI_CLASS_BRIDGE      0x06
#define PCI_CLASS_SERIAL    0x07
#define PCI_CLASS_BASE_PERIPH  0x08
#define PCI_CLASS_DAQ       0x11

static uint8_t pci_device_count;
static struct pci_device pci_devices[32];

static inline uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (uint32_t)(offset & 0xfc) | 0x80000000U;
}

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    __asm__ volatile ("outl %0, %1" : : "a"(address), "Nd"(PCI_CONFIG_ADDRESS));
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(PCI_CONFIG_DATA));
    return value;
}

void pci_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = pci_make_address(bus, device, function, offset);
    __asm__ volatile ("outl %0, %1" : : "a"(address), "Nd"(PCI_CONFIG_ADDRESS));
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(PCI_CONFIG_DATA));
}

uint8_t pci_get_class(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_read(bus, device, function, 0x08) >> 24) & 0xff;
}

uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    return (pci_read(bus, device, function, 0x08) >> 16) & 0xff;
}

void pci_enable_mmio(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t cmd = pci_read(bus, device, function, 0x04);
    pci_write(bus, device, function, 0x04, cmd | 0x00000004U);
}

void pci_enable_bus_master(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t cmd = pci_read(bus, device, function, 0x04);
    pci_write(bus, device, function, 0x04, cmd | 0x00000004U);
}

static void pci_check_device(uint8_t bus, uint8_t device) {
    uint32_t vendor_device = pci_read(bus, device, 0, 0);
    uint16_t vendor_id = vendor_device & 0xffff;
    uint16_t device_id = (vendor_device >> 16) & 0xffff;

    if (vendor_id == 0xffff || vendor_id == 0x0000) {
        return;
    }

    uint32_t class_prog = pci_read(bus, device, 0, 0x08);
    uint8_t header_type = (pci_read(bus, device, 0, 0x0e) >> 16) & 0xff;

    uint8_t functions = (header_type & 0x80) ? 8 : 1;

    for (uint8_t function = 0; function < functions; ++function) {
        if (pci_device_count >= 32) {
            break;
        }

        vendor_device = pci_read(bus, device, function, 0);
        vendor_id = vendor_device & 0xffff;
        device_id = (vendor_device >> 16) & 0xffff;

        if (vendor_id == 0xffff || vendor_id == 0x0000) {
            continue;
        }

        class_prog = pci_read(bus, device, function, 0x08);
        header_type = (pci_read(bus, device, function, 0x0e) >> 16) & 0xff;

        pci_devices[pci_device_count].bus = bus;
        pci_devices[pci_device_count].device = device;
        pci_devices[pci_device_count].function = function;
        pci_devices[pci_device_count].vendor_id = vendor_id;
        pci_devices[pci_device_count].device_id = device_id;
        pci_devices[pci_device_count].class_code = (class_prog >> 24) & 0xff;
        pci_devices[pci_device_count].subclass_code = (class_prog >> 16) & 0xff;
        pci_devices[pci_device_count].prog_if = (class_prog >> 8) & 0xff;
        pci_devices[pci_device_count].header_type = header_type;

        ++pci_device_count;
    }
}

void pci_enumerate_devices(void (*callback)(struct pci_device *)) {
    for (uint32_t bus = 0; bus < PCI_MAX_BUSES; ++bus) {
        for (uint8_t device = 0; device < PCI_MAX_DEVICES; ++device) {
            pci_check_device(bus, device);
        }
    }

    for (uint8_t i = 0; i < pci_device_count; ++i) {
        if (callback != NULL) {
            callback(&pci_devices[i]);
        }
    }
}

void pci_init(void) {
    pci_device_count = 0;
}