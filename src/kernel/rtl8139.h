#ifndef DEVILCORE_RTL8139_H
#define DEVILCORE_RTL8139_H

#include <stdint.h>

#include "net.h"
#include "pci.h"

#define RTL8139_IO_BASE    0xc000
#define RTL8139_VENDOR_ID 0x10ec
#define RTL8139_DEVICE_ID 0x8139

#define RTL8139_CMD     0x00
#define RTL8139_IMR     0x02
#define RTL8139_ISR     0x02
#define RTL8139_TSD0    0x10
#define RTL8139_TSD1    0x14
#define RTL8139_TSD2    0x18
#define RTL8139_TSD3    0x1c
#define RTL8139_TSAD0   0x20
#define RTL8139_TSAD1   0x24
#define RTL8139_TSAD2   0x28
#define RTL8139_TSAD3   0x2c
#define RTL8139_RBSTART 0x30
#define RTL8139_CR     0x37
#define RTL8139_CONFIG 0x52
#define RTL8139_MAC0   0x00

#define RTL8139_CMD_RESET    0x10
#define RTL8139_CMD_RECV_EN  0x04
#define RTL8139_CMD_XMIT_EN 0x01

#define RTL8139_ISR_ROK    0x01
#define RTL8139_ISR_TOK    0x04
#define RTL8139_ISR_RXERR 0x01
#define RTL8139_ISR_TXERR 0x02

#define RTL8139_RX_BUF_SIZE  8192
#define RTL8139_TX_BUF_SIZE 8192

struct rtl8139_private {
    uint16_t io_base;
    void *rx_buffer;
    void *tx_buffer;
    uint8_t tx_busy[4];
    uint8_t tx_cur;
    struct net_device net_dev;
};

int rtl8139_init(struct pci_device *pci);
void rtl8139_handler(struct rtl8139_private *priv);

#endif