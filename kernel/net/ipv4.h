/*
 * DevilCore OS — Internet Protocol version 4 (IPv4)
 */
#ifndef DC_IPV4_H
#define DC_IPV4_H

#include "../include/types.h"

#define IPV4_PROTO_ICMP 0x01
#define IPV4_PROTO_TCP  0x06
#define IPV4_PROTO_UDP  0x11

/* Standard IPv4 Protocol Header */
#pragma pack(push, 1)
typedef struct {
    uint8_t  ihl : 4;       /* Internet Header Length */
    uint8_t  version : 4;   /* Protocol Version (4) */
    uint8_t  tos;           /* Type of Service */
    uint16_t len;           /* Total Length */
    uint16_t id;            /* Identification */
    uint16_t frag_offset;   /* Fragment Offset */
    uint8_t  ttl;           /* Time to Live */
    uint8_t  protocol;      /* Next level protocol (ICMP, TCP, UDP) */
    uint16_t checksum;      /* Header Checksum */
    uint32_t src_ip;        /* Source IP Address */
    uint32_t dest_ip;       /* Destination IP Address */
} ipv4_header_t;
#pragma pack(pop)

#endif /* DC_IPV4_H */
