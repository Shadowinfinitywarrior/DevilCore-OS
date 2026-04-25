/*
 * DevilCore OS — USB Core Implementation
 */
#include "usb.h"
#include "uhci.h"
#include "../pci.h"
#include "../../mm/heap.h"
#include "../../libc/string.h"

static usb_device_t *g_usb_devices = NULL;

void usb_init(void) {
    g_usb_devices = NULL;

    /* Scan PCI bus for USB Host Controllers */
    int count = pci_get_device_count();
    
    for (int i = 0; i < count; i++) {
        pci_device_t *dev = pci_get_device(i);
        
        /* 
         * Class 0x0C = Serial Bus Controller 
         * Subclass 0x03 = USB Controller 
         */
        if (dev->class_id == 0x0C && dev->subclass_id == 0x03) {
            if (dev->prog_if == 0x00) {
                /* UHCI */
                uhci_init(dev);
            } else if (dev->prog_if == 0x10) {
                /* OHCI */
                /* Not implemented */
            } else if (dev->prog_if == 0x20) {
                /* EHCI (USB 2.0) */
                /* Not implemented */
            } else if (dev->prog_if == 0x30) {
                /* xHCI (USB 3.0) */
                /* Not implemented */
            }
        }
    }
}
