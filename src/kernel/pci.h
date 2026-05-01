#ifndef DEVILCORE_PCI_H
#define DEVILCORE_PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA    0xcfc

#define PCI_MAX_BUSES     256
#define PCI_MAX_DEVICES  32
#define PCI_MAX_FUNCTIONS 8

struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t prog_if;
    uint8_t header_type;
};

struct pci_device_iter {
    struct pci_device device;
    uint8_t current;
    uint8_t found;
};

void pci_init(void);

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

void pci_enable_mmio(uint8_t bus, uint8_t device, uint8_t function);
void pci_enable_bus_master(uint8_t bus, uint8_t device, uint8_t function);

uint8_t pci_get_class(uint8_t bus, uint8_t device, uint8_t function);
uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function);

void pci_enumerate_devices(void (*callback)(struct pci_device *));

#endif