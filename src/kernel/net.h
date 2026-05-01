#ifndef DEVILCORE_NET_H
#define DEVILCORE_NET_H

#include <stddef.h>
#include <stdint.h>

#define NET_MAX_DEVICES  4
#define NET_MAC_ADDR_LEN  6
#define NET_MTU 1500

struct net_device {
    char name[16];
    uint8_t mac_addr[NET_MAC_ADDR_LEN];
    uint32_t ip_addr;
    uint32_t netmask;
    uint32_t gateway;
    uint16_t mtu;
    uint8_t up;
    
    int (*send_packet)(struct net_device *, void *, size_t);
    int (*recv_packet)(struct net_device *, void *, size_t);
    void *private_data;
};

struct net_packet {
    struct net_device *device;
    uint8_t dest_mac[NET_MAC_ADDR_LEN];
    uint8_t src_mac[NET_MAC_ADDR_LEN];
    uint16_t protocol;
    void *data;
    size_t length;
};

typedef void (*net_rx_handler_t)(struct net_packet *);

int net_init(void);
int net_register_device(struct net_device *dev);
struct net_device *net_get_device(const char *name);
int net_send(struct net_device *dev, void *data, size_t length);
int net_set_rx_handler(struct net_device *dev, net_rx_handler_t handler);

int net_ipv4_send(uint32_t dest_ip, uint16_t protocol, const void *data, size_t length);
int net_ipv4_recv(struct net_packet *packet);

extern uint8_t net_broadcast_mac[NET_MAC_ADDR_LEN];
extern uint8_t net_null_mac[NET_MAC_ADDR_LEN];

#endif