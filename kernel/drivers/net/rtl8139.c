/*
 * DevilCore OS — RTL8139 Fast Ethernet PCI Adapter
 */
#include "rtl8139.h"
#include "../../include/ports.h"
#include "../../mm/heap.h"
#include "../../mm/pmm.h"
#include "../../libc/string.h"

/* Registers */
#define RTL_MAC0     0x00
#define RTL_CMD      0x37
#define RTL_RBSTART  0x30
#define RTL_IMR      0x3C
#define RTL_ISR      0x3E
#define RTL_CONFIG1  0x52
#define RTL_RCR      0x44
#define RTL_TSAD0    0x20
#define RTL_TSD0     0x10

static uint16_t g_rtl_io_base = 0;
static uint8_t  g_mac_addr[6];
static uint8_t *g_rx_buffer;
static uint8_t  g_tx_idx = 0;

void rtl8139_init(pci_device_t *dev) {
    if (!dev) return;
    
    /* Fetch BAR0 for the I/O base */
    uint32_t bar0 = pci_read_config(dev->bus, dev->device, dev->function, 0x10);
    g_rtl_io_base = bar0 & ~0x03;
    
    if (g_rtl_io_base == 0) return; /* Device IO mapping issue */

    /* Basic PCI Bus Mastering Activation */
    uint32_t command = pci_read_config(dev->bus, dev->device, dev->function, 0x04);
    /* In a full OS, we'd write 0x04 (Bus Master) back to PCI_COMMAND via outl to PCI_DATA */
    
    /* Turn on the RTL8139 */
    outb(g_rtl_io_base + RTL_CONFIG1, 0x00);
    
    /* Software Reset */
    outb(g_rtl_io_base + RTL_CMD, 0x10);
    while((inb(g_rtl_io_base + RTL_CMD) & 0x10) != 0) {
        /* Wait for reset to complete */
    }

    /* Allocate Memory for the RX Ring Buffer (8KB + 16 bytes + 1.5KB wrap) */
    g_rx_buffer = kmalloc_aligned(8192 + 16 + 1500, 4096);
    memset(g_rx_buffer, 0, 8192 + 16 + 1500);
    outl(g_rtl_io_base + RTL_RBSTART, (uint32_t)(uintptr_t)g_rx_buffer);

    /* Set IMR + ISR (Interrupt setup) */
    outw(g_rtl_io_base + RTL_IMR, 0x0005); // Receive OK and Transmit OK
    
    /* RCR (Receive Configuration Register) - Accept Broadcast and Physical Match */
    outl(g_rtl_io_base + RTL_RCR, 0x0F | (1 << 7));
    
    /* Enable RX and TX */
    outb(g_rtl_io_base + RTL_CMD, 0x0C); 
    
    /* Read hardware MAC address */
    for (int i = 0; i < 6; i++) {
        g_mac_addr[i] = inb(g_rtl_io_base + RTL_MAC0 + i);
    }
}

uint8_t* rtl8139_get_mac(void) {
    return g_mac_addr;
}

void rtl8139_send_packet(uint8_t *data, uint32_t len) {
    if (!g_rtl_io_base) return;

    /* Write data to physical memory and point TSAD to it */
    outl(g_rtl_io_base + RTL_TSAD0 + (g_tx_idx * 4), (uint32_t)(uintptr_t)data);
    outl(g_rtl_io_base + RTL_TSD0  + (g_tx_idx * 4), len);
    
    g_tx_idx = (g_tx_idx + 1) % 4;
}
