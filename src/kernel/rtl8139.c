#include "rtl8139.h"

#include "pci.h"
#include "memory.h"
#include "net.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef RTL8139_MTU
#define RTL8139_MTU 1500
#endif

uint32_t net_parse_ipv4(const char *str);

static inline uint8_t rtl8139_readb(struct rtl8139_private *priv, uint8_t offset) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(priv->io_base + offset));
    return value;
}

static inline uint16_t rtl8139_readw(struct rtl8139_private *priv, uint8_t offset) {
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(priv->io_base + offset));
    return value;
}

static inline uint32_t rtl8139_readl(struct rtl8139_private *priv, uint8_t offset) {
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(priv->io_base + offset));
    return value;
}

static inline void rtl8139_writeb(struct rtl8139_private *priv, uint8_t offset, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(priv->io_base + offset));
}

static inline void rtl8139_writew(struct rtl8139_private *priv, uint8_t offset, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(priv->io_base + offset));
}

static inline void rtl8139_writel(struct rtl8139_private *priv, uint8_t offset, uint32_t value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(priv->io_base + offset));
}

static void rtl8139_reset(struct rtl8139_private *priv) {
    rtl8139_writeb(priv, RTL8139_CMD, RTL8139_CMD_RESET);
    
    for (int i = 0; i < 1000; ++i) {
        if ((rtl8139_readb(priv, RTL8139_CMD) & RTL8139_CMD_RESET) == 0) {
            break;
        }
    }
}

static void rtl8139_set_mac(struct rtl8139_private *priv, uint8_t *mac) {
    for (int i = 0; i < 6; ++i) {
        rtl8139_writeb(priv, RTL8139_MAC0 + i, mac[i]);
    }
}

static int rtl8139_transmit(struct net_device *dev, void *data, size_t length) {
    struct rtl8139_private *priv = (struct rtl8139_private *)dev;
    
    if (priv->tx_busy[priv->tx_cur]) {
        return -1;
    }
    
    if (length > RTL8139_TX_BUF_SIZE) {
        length = RTL8139_TX_BUF_SIZE;
    }
    
    memcpy(priv->tx_buffer, data, length);
    
    rtl8139_writel(priv, RTL8139_TSAD0 + (priv->tx_cur * 4), (uint32_t)(uintptr_t)priv->tx_buffer);
    rtl8139_writel(priv, RTL8139_TSD0 + (priv->tx_cur * 4), (uint32_t)length);
    
    priv->tx_busy[priv->tx_cur] = 1;
    priv->tx_cur = (priv->tx_cur + 1) & 3;
    
    return (int)length;
}

static void rtl8139_receive(struct rtl8139_private *priv) {
    uint16_t rx_status = rtl8139_readw(priv, RTL8139_ISR);
    
    if (rx_status & RTL8139_ISR_ROK) {
        uint32_t rx_head = rtl8139_readw(priv, 0x18) & 0x0fff;
        
        while (rx_head != (rtl8139_readw(priv, 0x1a) & 0x0fff)) {
            uint32_t ring_offset = rx_head;
            
            uint32_t packet_status = *(uint32_t *)((uint8_t *)priv->rx_buffer + ring_offset);
            uint32_t packet_length = (packet_status >> 16) & 0x3fff;
            
            if ((packet_status & 0x40000000) == 0 || (packet_status & 0x80000000)) {
                break;
            }
            
            uint32_t data_length = packet_length - 4; // exclude CRC
            uint8_t *packet_data = (uint8_t *)priv->rx_buffer + ring_offset + 4; // skip 4 byte header
            
            // Handle wrap-around
            uint8_t *linear_buffer = kmalloc(data_length);
            if (linear_buffer) {
                if (ring_offset + 4 + data_length > RTL8139_RX_BUF_SIZE) {
                    uint32_t chunk1 = RTL8139_RX_BUF_SIZE - (ring_offset + 4);
                    uint32_t chunk2 = data_length - chunk1;
                    memcpy(linear_buffer, packet_data, chunk1);
                    memcpy(linear_buffer + chunk1, priv->rx_buffer, chunk2);
                } else {
                    memcpy(linear_buffer, packet_data, data_length);
                }
                
                struct net_packet net_pkt;
                net_pkt.device = &priv->net_dev;
                net_pkt.data = linear_buffer;
                net_pkt.length = data_length;
                
                // Dispatch
                extern void net_rx_handler(struct net_packet *packet);
                net_rx_handler(&net_pkt);
                
                kfree(linear_buffer);
            }
            
            rx_head = (rx_head + packet_length + 4 + 3) & ~3; // align to 4 bytes
            if (rx_head >= RTL8139_RX_BUF_SIZE) {
                rx_head -= RTL8139_RX_BUF_SIZE;
            }
            
            rtl8139_writew(priv, 0x18, rx_head);
        }
        
        rtl8139_writew(priv, RTL8139_ISR, RTL8139_ISR_ROK);
    }
}

void rtl8139_handler(struct rtl8139_private *priv) {
    uint16_t isr = rtl8139_readw(priv, RTL8139_ISR);
    
    if (isr & RTL8139_ISR_ROK) {
        rtl8139_receive(priv);
    }
    
    if (isr & RTL8139_ISR_TOK) {
        for (int i = 0; i < 4; ++i) {
            uint32_t tsd = rtl8139_readl(priv, RTL8139_TSD0 + (i * 4));
            if ((tsd & 0x80000000) == 0) {
                priv->tx_busy[i] = 0;
            }
        }
        
        rtl8139_writew(priv, RTL8139_ISR, RTL8139_ISR_TOK);
    }
    
    rtl8139_writew(priv, RTL8139_ISR, isr);
}

static int rtl8139_open(struct net_device *dev) {
    struct rtl8139_private *priv = (struct rtl8139_private *)dev;
    
    rtl8139_reset(priv);
    
    rtl8139_set_mac(priv, priv->net_dev.mac_addr);
    
    rtl8139_writel(priv, RTL8139_RBSTART, (uint32_t)(uintptr_t)priv->rx_buffer);
    rtl8139_writeb(priv, RTL8139_CR, RTL8139_CMD_RECV_EN | RTL8139_CMD_XMIT_EN);
    
    priv->net_dev.up = 1;
    
    return 0;
}

static int rtl8139_close(struct net_device *dev) __attribute__((unused));
static int rtl8139_close(struct net_device *dev) {
    struct rtl8139_private *priv = (struct rtl8139_private *)dev;
    
    rtl8139_writeb(priv, RTL8139_CMD, 0);
    
    priv->net_dev.up = 0;
    
    return 0;
}

int rtl8139_init(struct pci_device *pci) {
    if (pci == NULL) {
        return -1;
    }
    
    uint16_t io_base = 0;
    
    uint32_t bar0 = pci_read(pci->bus, pci->device, pci->function, 0x10);
    if ((bar0 & 1) == 0) {
        io_base = bar0 & ~1;
    }
    
    if (io_base == 0) {
        io_base = RTL8139_IO_BASE;
    }
    
    struct rtl8139_private *priv = kmalloc(sizeof(struct rtl8139_private));
    if (priv == NULL) {
        return -1;
    }
    
    memset(priv, 0, sizeof(struct rtl8139_private));
    
    priv->io_base = io_base;
    priv->rx_buffer = (void *)(uintptr_t)pmm_alloc_page();
    priv->tx_buffer = (void *)(uintptr_t)pmm_alloc_page();
    
    if (priv->rx_buffer == NULL || priv->tx_buffer == NULL) {
        if (priv->rx_buffer != NULL) pmm_free_page(priv->rx_buffer);
        if (priv->tx_buffer != NULL) pmm_free_page(priv->tx_buffer);
        kfree(priv);
        return -1;
    }
    
    strncpy(priv->net_dev.name, "eth0", 15);
    priv->net_dev.mtu = RTL8139_MTU;
    priv->net_dev.up = 0;
    priv->net_dev.send_packet = rtl8139_transmit;
    priv->net_dev.private_data = priv;
    
    for (int i = 0; i < 6; ++i) {
        priv->net_dev.mac_addr[i] = rtl8139_readb(priv, RTL8139_MAC0 + i);
    }
    
    priv->net_dev.ip_addr = net_parse_ipv4("192.168.1.100");
    priv->net_dev.netmask = net_parse_ipv4("255.255.255.0");
    priv->net_dev.gateway = net_parse_ipv4("192.168.1.1");
    
    pci_enable_bus_master(pci->bus, pci->device, pci->function);
    pci_enable_mmio(pci->bus, pci->device, pci->function);
    
    net_register_device(&priv->net_dev);
    
    rtl8139_open(&priv->net_dev);
    
    return 0;
}