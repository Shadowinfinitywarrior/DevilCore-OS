/*
 * DevilCore OS — Network Core Subsystem
 */
#ifndef DC_NET_H
#define DC_NET_H

#include "../include/types.h"
#include "../drivers/pci.h"

#define MAX_PACKET_QUEUE 64

/* Sk_buff equivalent for network data routing */
typedef struct net_packet {
    uint8_t *payload;
    uint32_t length;
    uint32_t head_offset;
    struct net_packet *next;
} net_packet_t;

/* Initialize global network stack */
void net_init(void);

/* Pass a received packet into the OS network stack */
void net_receive(uint8_t *data, uint32_t len);

/* Output a packet through the initialized hardware NIC */
void net_send(uint8_t *data, uint32_t len);

#endif /* DC_NET_H */
