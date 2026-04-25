// kernel/net/ip.c
// IPv4 handling

#include <devil/types.h>
#include <net/socket.h>

#define IP_VERSION_4 4
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

typedef struct ip_packet {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} PACKED ip_packet_t;

static uint32_t my_ip = 0x0100A8C0; // 192.168.0.1
static uint32_t netmask = 0x00FFFFFF; // 255.255.255.0
static uint32_t gateway = 0x0100A8C0;

void ip_init(void) {
    my_ip = 0;
}

void ip_recv(uint8_t *packet, size_t len) {
    ip_packet_t *ip = (ip_packet_t*)packet;
    
    if (ip->ver_ihl != (IP_VERSION_4 << 4 | 5)) return;
    
    switch (ip->proto) {
        case IP_PROTO_ICMP:
            // Handle ICMP
            break;
        case IP_PROTO_TCP:
            // Forward to TCP
            break;
        case IP_PROTO_UDP:
            // Forward to UDP
            break;
    }
}

int ip_send(uint32_t dst, uint8_t proto, uint8_t *data, size_t len) {
    ip_packet_t *ip = (ip_packet_t*)data;
    
    ip->ver_ihl = (IP_VERSION_4 << 4) | 5;
    ip->total_len = 20 + len;
    ip->ttl = 64;
    ip->proto = proto;
    ip->src = my_ip;
    ip->dst = dst;
    ip->checksum = 0;
    
    return 0;
}