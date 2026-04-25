// kernel/net/ethernet.c
// Ethernet driver

#include <devil/types.h>
#include "socket.h"
#include <asm/io.h>

static uint8_t mac_address[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

int net_init(void) {
    // Initialize network card (would load driver for specific NIC)
    return 0;
}

int ethernet_send(uint8_t *dhost, uint16_t type, uint8_t *data, size_t len) {
    // Hardware send (RTL8139, e1000, etc.)
    return 0;
}

void ethernet_recv(uint8_t *frame, size_t len) {
    eth_header_t *eth = (eth_header_t*)frame;
    
    switch (eth->type) {
        case ETH_TYPE_IPV4:
            // Pass to IP handler
            extern void ipv4_recv(uint8_t *packet, size_t len);
            ipv4_recv(frame + sizeof(eth_header_t), len - sizeof(eth_header_t));
            break;
        case ETH_TYPE_ARP:
            // Pass to ARP handler
            extern void arp_recv(uint8_t *packet, size_t len);
            arp_recv(frame + sizeof(eth_header_t), len - sizeof(eth_header_t));
            break;
    }
}