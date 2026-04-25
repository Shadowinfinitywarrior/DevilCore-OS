/*
 * DevilCore OS — Network Core Implementation
 */
#include "net.h"
#include "../drivers/net/rtl8139.h"
#include "../mm/heap.h"
#include "../libc/string.h"

static bool g_nic_available = false;

void net_init(void) {
    /* Auto-detect network interfaces from the PCI bus */
    int limit = pci_get_device_count();
    
    for (int i = 0; i < limit; i++) {
        pci_device_t *dev = pci_get_device(i);
        
        /* Check for Realtek RTL8139 Fast Ethernet */
        if (dev->vendor_id == 0x10EC && dev->device_id == 0x8139) {
            rtl8139_init(dev);
            g_nic_available = true;
            return; /* Support single NIC for now */
        }
        
        /* Note: Intel E1000 would be 0x8086 : 0x100E typically */
    }
}

void net_receive(uint8_t *data, uint32_t len) {
    /* 
     * Network driver interrupt hooks call this function.
     * From here, we peel off the Ethernet Header -> IPv4 Header -> TCP/UDP Payload. 
     */
    (void)data;
    (void)len;
}

void net_send(uint8_t *data, uint32_t len) {
    if (g_nic_available) {
        /* Route outgoing payload into the RTL8139 hardware */
        rtl8139_send_packet(data, len);
    }
}
