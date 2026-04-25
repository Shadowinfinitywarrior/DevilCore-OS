/*
 * DevilCore OS — Ethernet Layer 2 Frame Logic
 */
#include "eth.h"
#include "net.h"
#include "../drivers/net/rtl8139.h"
#include "../mm/heap.h"
#include "../libc/string.h"

/* Helper to handle Big Endian network bytes */
static inline uint16_t htons(uint16_t v) {
    return ((v & 0xFF) << 8) | ((v >> 8) & 0xFF);
}

static inline uint16_t ntohs(uint16_t v) {
    return htons(v);
}

void eth_receive(uint8_t *data, uint32_t len) {
    if (len < sizeof(eth_header_t)) return;
    
    eth_header_t *hdr = (eth_header_t*)data;
    uint16_t type = ntohs(hdr->type);
    
    if (type == ETH_PROTOCOL_IPv4) {
        /* Route to IPv4 engine */
    } else if (type == ETH_PROTOCOL_ARP) {
        /* Route to ARP solver */
    }
}

void eth_send(mac_address_t *dest, uint16_t type, uint8_t *payload, uint32_t payload_len) {
    uint32_t frame_len = sizeof(eth_header_t) + payload_len;
    uint8_t *frame = kmalloc(frame_len);
    if (!frame) return;
    
    eth_header_t *hdr = (eth_header_t*)frame;
    
    /* Fetch our local MAC via the NIC driver */
    uint8_t *our_mac = rtl8139_get_mac();
    memcpy(&hdr->src.mac_addr, our_mac, 6);
    memcpy(&hdr->dest.mac_addr, dest->mac_addr, 6);
    hdr->type = htons(type);
    
    memcpy(frame + sizeof(eth_header_t), payload, payload_len);
    
    /* Hand over to Device Core */
    net_send(frame, frame_len);
    
    kfree(frame);
}
