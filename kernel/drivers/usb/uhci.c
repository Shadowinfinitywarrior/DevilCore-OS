/*
 * DevilCore OS — Universal Host Controller Interface (UHCI) Implementation
 */
#include "uhci.h"
#include "../../include/ports.h"

/* Basic UHCI Ports relative to base */
#define UHCI_CMD      0x00
#define UHCI_STS      0x02
#define UHCI_INTR     0x04
#define UHCI_FRNUM    0x06
#define UHCI_FRBASEADD 0x08
#define UHCI_SOFMOD   0x0C
#define UHCI_PORTSC1  0x10
#define UHCI_PORTSC2  0x12

/* To properly read the base IO port of the PCI device, we need a helper in pci.c. 
   For now, we just implement the stub logic.
*/

extern uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void uhci_init(pci_device_t *dev) {
    /* Read BAR4 to get the I/O base address for the UHCI controller */
    uint32_t bar4 = pci_read_config(dev->bus, dev->device, dev->function, 0x20);
    
    if (!(bar4 & 1)) {
        /* Not an I/O space BAR, unsupported for basic UHCI in this implementation */
        return;
    }
    
    uint16_t io_base = (bar4 & ~0x03);

    /* Reset the controller */
    outw(io_base + UHCI_CMD, 0x0002); /* Global Reset */
    
    /* Wait a bit (approximate delay using loops) */
    for (volatile int i = 0; i < 100000; i++);
    
    /* Clear reset */
    outw(io_base + UHCI_CMD, 0x0000);
    
    /* Enable interrupts (if we were using them) */
    outw(io_base + UHCI_INTR, 0x000F);
    
    /* Start the controller */
    outw(io_base + UHCI_CMD, 0x0001);
}
