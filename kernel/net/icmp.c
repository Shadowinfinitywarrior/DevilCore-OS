// kernel/net/icmp.c
// ICMP handling

#include <devil/types.h>
#include <net/socket.h>

#define ICMP_ECHO       0
#define ICMP_ECHOREPLY  8
#define ICMP_DEST_UNREACH 3
#define ICMP_TIME_EXCEEDED 11

typedef struct icmp_packet {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint32_t id;
    uint32_t seq;
} PACKED icmp_packet_t;

void icmp_send_echo_reply(uint32_t dst, uint16_t id, uint16_t seq) {
    uint8_t packet[64];
    icmp_packet_t *icmp = (icmp_packet_t*)packet;
    
    icmp->type = ICMP_ECHOREPLY;
    icmp->code = 0;
    icmp->id = id;
    icmp->seq = seq;
    icmp->checksum = 0;
    
    // Send via IP
}

void icmp_recv(uint8_t *packet, size_t len) {
    icmp_packet_t *icmp = (icmp_packet_t*)packet;
    
    switch (icmp->type) {
        case ICMP_ECHO:
            icmp_send_echo_reply(0, icmp->id, icmp->seq);
            break;
    }
}