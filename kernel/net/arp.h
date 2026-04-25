/*
 * DevilCore OS — Address Resolution Protocol (ARP)
 */
#ifndef DC_ARP_H
#define DC_ARP_H

#include "../include/types.h"
#include "eth.h"

/* Standard ARP Protocol Header */
#pragma pack(push, 1)
typedef struct {
    uint16_t hw_type;       /* Hardware Type (e.g., 1 for Ethernet) */
    uint16_t proto_type;    /* Protocol Type (e.g., 0x0800 for IPv4) */
    uint8_t  hw_addr_len;   /* Hardware Address Length (6) */
    uint8_t  proto_addr_len;/* Protocol Address Length (4) */
    uint16_t opcode;        /* Operation (1 = Request, 2 = Reply) */
    mac_address_t src_mac;  /* Sender hardware address */
    uint32_t      src_ip;   /* Sender protocol address */
    mac_address_t dest_mac; /* Target hardware address */
    uint32_t      dest_ip;  /* Target protocol address */
} arp_header_t;
#pragma pack(pop)

#endif /* DC_ARP_H */
