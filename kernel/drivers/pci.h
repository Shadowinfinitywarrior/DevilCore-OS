/*
 * DevilCore OS — PCI Bus Enumeration
 */
#ifndef DC_PCI_H
#define DC_PCI_H

#include "../include/types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_id;
    uint8_t  subclass_id;
    uint8_t  prog_if;
    uint8_t  revision_id;
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
} pci_device_t;

/* Initialize PCI subsystem and enumerate devices */
void pci_init(void);

/* Get the number of detected PCI devices */
int pci_get_device_count(void);

/* Get a specific PCI device by index */
pci_device_t* pci_get_device(int index);

/* Read raw configuration space value */
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

#endif /* DC_PCI_H */
