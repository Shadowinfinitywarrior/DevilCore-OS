/*
 * DevilCore OS — Universal Host Controller Interface (UHCI)
 */
#ifndef DC_UHCI_H
#define DC_UHCI_H

#include "../pci.h"

/* Initialize UHCI controller from PCI device metadata */
void uhci_init(pci_device_t *dev);

#endif /* DC_UHCI_H */
