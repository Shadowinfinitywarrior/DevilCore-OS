/*
 * DevilCore OS — RTL8139 Network Interface Card Driver
 */
#ifndef DC_RTL8139_H
#define DC_RTL8139_H

#include "../../drivers/pci.h"
#include "../../include/types.h"

/* Initialize the RTL8139 NIC if detected on the PCI bus */
void rtl8139_init(pci_device_t *dev);

/* Fetch the hardware MAC Address of the initialized NIC */
uint8_t* rtl8139_get_mac(void);

/* Transmit a raw Ethernet Frame */
void rtl8139_send_packet(uint8_t *data, uint32_t len);

#endif /* DC_RTL8139_H */
