#include "arp.h"
#include "memory.h"
#include "timer.h"
#include <string.h>

#define ARP_MAX_ENTRIES 32
#define ETH_TYPE_ARP 0x0806

struct arp_entry {
    uint32_t ip;
    uint8_t mac[6];
    uint64_t timestamp;
    uint8_t valid;
};

static struct arp_entry arp_table[ARP_MAX_ENTRIES];

struct eth_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
} __attribute__((packed));

struct arp_packet {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_size;
    uint8_t protocol_size;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed));

void arp_init(void) {
    memset(arp_table, 0, sizeof(arp_table));
}

static void arp_update_table(uint32_t ip, uint8_t *mac) {
    // Check if exists
    for (int i = 0; i < ARP_MAX_ENTRIES; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].timestamp = timer_ticks();
            return;
        }
    }
    // Find empty or oldest slot
    int oldest = 0;
    for (int i = 0; i < ARP_MAX_ENTRIES; i++) {
        if (!arp_table[i].valid) {
            arp_table[i].ip = ip;
            memcpy(arp_table[i].mac, mac, 6);
            arp_table[i].timestamp = timer_ticks();
            arp_table[i].valid = 1;
            return;
        }
        if (arp_table[i].timestamp < arp_table[oldest].timestamp) {
            oldest = i;
        }
    }
    // Replace oldest
    arp_table[oldest].ip = ip;
    memcpy(arp_table[oldest].mac, mac, 6);
    arp_table[oldest].timestamp = timer_ticks();
    arp_table[oldest].valid = 1;
}

uint8_t* arp_lookup(uint32_t ip) {
    for (int i = 0; i < ARP_MAX_ENTRIES; i++) {
        if (arp_table[i].valid && arp_table[i].ip == ip) {
            return arp_table[i].mac;
        }
    }
    return NULL;
}

void arp_request(struct net_device *dev, uint32_t target_ip) {
    if (!dev) return;

    struct {
        struct eth_header eth;
        struct arp_packet arp;
    } __attribute__((packed)) packet;

    memset(&packet, 0, sizeof(packet));

    // Ethernet header
    memset(packet.eth.dest_mac, 0xff, 6); // Broadcast
    memcpy(packet.eth.src_mac, dev->mac_addr, 6);
    packet.eth.type = htons(ETH_TYPE_ARP);

    // ARP header
    packet.arp.hardware_type = htons(1); // Ethernet
    packet.arp.protocol_type = htons(0x0800); // IPv4
    packet.arp.hardware_size = 6;
    packet.arp.protocol_size = 4;
    packet.arp.opcode = htons(1); // Request

    memcpy(packet.arp.sender_mac, dev->mac_addr, 6);
    packet.arp.sender_ip = dev->ip_addr; // Note: assuming host byte order for internal storage, net_parse_ipv4 produces host byte order or network?
    // Actually net_parse_ipv4 produces big-endian (network byte order) if we look at its implementation. Let's assume it's network byte order.
    packet.arp.target_ip = target_ip; 

    net_send(dev, &packet, sizeof(packet));
}

void arp_handle_packet(struct net_device *dev, void *data, size_t length) {
    if (length < sizeof(struct arp_packet)) return;
    struct arp_packet *arp = (struct arp_packet *)data;

    if (ntohs(arp->hardware_type) != 1 || ntohs(arp->protocol_type) != 0x0800) return;

    // Update table with sender
    arp_update_table(arp->sender_ip, arp->sender_mac);

    if (ntohs(arp->opcode) == 1) { // Request
        if (arp->target_ip == dev->ip_addr) {
            // Reply
            struct {
                struct eth_header eth;
                struct arp_packet arp;
            } __attribute__((packed)) packet;

            memcpy(packet.eth.dest_mac, arp->sender_mac, 6);
            memcpy(packet.eth.src_mac, dev->mac_addr, 6);
            packet.eth.type = htons(ETH_TYPE_ARP);

            packet.arp.hardware_type = htons(1);
            packet.arp.protocol_type = htons(0x0800);
            packet.arp.hardware_size = 6;
            packet.arp.protocol_size = 4;
            packet.arp.opcode = htons(2); // Reply

            memcpy(packet.arp.sender_mac, dev->mac_addr, 6);
            packet.arp.sender_ip = dev->ip_addr;
            memcpy(packet.arp.target_mac, arp->sender_mac, 6);
            packet.arp.target_ip = arp->sender_ip;

            net_send(dev, &packet, sizeof(packet));
        }
    }
}
