/*
 * DevilCore OS — Ethernet Layer 2 (MAC)
 */
#ifndef DC_ETH_H
#define DC_ETH_H

#include "../include/types.h"

#define ETH_PROTOCOL_IPv4 0x0800
#define ETH_PROTOCOL_ARP  0x0806
#define ETH_PROTOCOL_IPv6 0x86DD

/* MAC Address structure */
typedef struct {
    uint8_t mac_addr[6];
} mac_address_t;

/* Standard Ethernet Frame Header */
#pragma pack(push, 1)
typedef struct {
    mac_address_t dest;
    mac_address_t src;
    uint16_t      type; /* Protocol (e.g. 0x0800 for IPv4) */
} eth_header_t;
#pragma pack(pop)

/* Process an incoming ethernet frame payload */
void eth_receive(uint8_t *data, uint32_t len);

/* Construct and encapsulate an ethernet frame sending it out */
void eth_send(mac_address_t *dest, uint16_t type, uint8_t *payload, uint32_t payload_len);

#endif /* DC_ETH_H */
