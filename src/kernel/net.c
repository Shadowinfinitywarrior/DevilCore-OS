#include "net.h"

#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IPV4 0x0800

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

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

struct ipv4_header {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

static struct net_device *net_devices[NET_MAX_DEVICES];
static uint32_t net_device_count;
static net_rx_handler_t net_rx_handlers[NET_MAX_DEVICES];

uint8_t net_broadcast_mac[NET_MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t net_null_mac[NET_MAC_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint32_t net_parse_ipv4(const char *str) {
    uint32_t addr = 0;
    uint32_t part = 0;
    uint8_t parts = 0;
    
    while (*str != '\0' && parts < 4) {
        if (*str >= '0' && *str <= '9') {
            part = part * 10 + (*str - '0');
        } else if (*str == '.') {
            addr = (addr << 8) | (part & 0xff);
            ++parts;
            part = 0;
        }
        ++str;
    }
    
    if (parts < 3) {
        addr = (addr << 8) | (part & 0xff);
    }
    
    return addr;
}

static uint16_t net_checksum_ipv4(struct ipv4_header *header, size_t length) __attribute__((unused));
static uint16_t net_checksum_ipv4(struct ipv4_header *header, size_t length) {
    uint32_t sum = 0;
    const uint8_t *ptr = (const uint8_t *)header;
    
    for (size_t i = 0; i < length - 1; i += 2) {
        sum += (uint16_t)ptr[i] | ((uint16_t)ptr[i+1] << 8);
    }
    
    if (length & 1) {
        sum += (uint16_t)ptr[length - 1];
    }
    
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

static void net_handle_arp(struct net_device *dev, struct arp_packet *arp) {
    if (arp->opcode == ARP_OP_REQUEST) {
        if (arp->target_ip == dev->ip_addr) {
            uint8_t sender_mac[6] = {0};
            memcpy(sender_mac, dev->mac_addr, 6);
            
            arp->opcode = ARP_OP_REPLY;
            memcpy(arp->target_mac, arp->sender_mac, 6);
            memcpy(arp->sender_mac, sender_mac, 6);
            arp->target_ip = arp->sender_ip;
            arp->sender_ip = dev->ip_addr;
            
            struct net_packet packet;
            memset(&packet, 0, sizeof(packet));
            memcpy(packet.dest_mac, arp->target_mac, 6);
            memcpy(packet.src_mac, sender_mac, 6);
            packet.protocol = ETH_TYPE_ARP;
            packet.data = arp;
            packet.length = sizeof(struct arp_packet);
            
            dev->send_packet(dev, &packet, packet.length);
        }
    }
}

static void net_handle_ipv4(struct net_device *dev, struct ipv4_header *ipv4, size_t length) {
    (void)dev;
    (void)ipv4;
    (void)length;
}

void net_rx_handler(struct net_packet *packet) {
    if (packet == NULL || packet->device == NULL) {
        return;
    }
    
    struct eth_header *eth = (struct eth_header *)packet->data;
    
    if (eth->type == ETH_TYPE_ARP) {
        struct arp_packet *arp = (struct arp_packet *)((uint8_t *)eth + sizeof(struct eth_header));
        net_handle_arp(packet->device, arp);
    } else if (eth->type == ETH_TYPE_IPV4) {
        struct ipv4_header *ipv4 = (struct ipv4_header *)((uint8_t *)eth + sizeof(struct eth_header));
        net_handle_ipv4(packet->device, ipv4, packet->length - sizeof(struct eth_header));
    }
}

int net_init(void) {
    memset(net_devices, 0, sizeof(net_devices));
    net_device_count = 0;
    
    return 0;
}

int net_register_device(struct net_device *dev) {
    if (dev == NULL || net_device_count >= NET_MAX_DEVICES) {
        return -1;
    }
    
    net_devices[net_device_count++] = dev;
    
    return 0;
}

struct net_device *net_get_device(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < net_device_count; ++i) {
        if (strncmp(net_devices[i]->name, name, 16) == 0) {
            return net_devices[i];
        }
    }
    
    return NULL;
}

int net_send(struct net_device *dev, void *data, size_t length) {
    if (dev == NULL || !dev->up || dev->send_packet == NULL) {
        return -1;
    }
    
    return dev->send_packet(dev, data, length);
}

int net_set_rx_handler(struct net_device *dev, net_rx_handler_t handler) {
    if (dev == NULL) {
        return -1;
    }
    
    for (uint32_t i = 0; i < net_device_count; ++i) {
        if (net_devices[i] == dev) {
            net_rx_handlers[i] = handler;
            return 0;
        }
    }
    
    return -1;
}

int net_ipv4_send(uint32_t dest_ip, uint16_t protocol, const void *data, size_t length) {
    (void)dest_ip;
    (void)protocol;
    (void)data;
    (void)length;
    
    return -1;
}

int net_ipv4_recv(struct net_packet *packet) {
    (void)packet;
    return -1;
}