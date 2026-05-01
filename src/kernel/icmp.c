#include "icmp.h"
#include "memory.h"
#include <string.h>

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_ECHO_REQUEST 8

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

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed));

static uint16_t calculate_checksum(void *data, size_t length) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;
    
    for (size_t i = 0; i < length / 2; i++) {
        sum += ntohs(ptr[i]);
    }
    if (length & 1) {
        sum += ((uint8_t *)data)[length - 1] << 8;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    
    return htons((uint16_t)~sum);
}

void icmp_handle_packet(struct net_device *dev, void *ipv4_hdr_raw, void *data, size_t length) {
    if (!dev || !data || length < sizeof(struct icmp_header)) return;

    struct ipv4_header *ipv4 = (struct ipv4_header *)ipv4_hdr_raw;
    struct icmp_header *icmp = (struct icmp_header *)data;

    if (icmp->type == ICMP_TYPE_ECHO_REQUEST && icmp->code == 0) {
        // Prepare reply
        size_t reply_len = length;
        uint8_t *reply_data = kmalloc(reply_len);
        if (!reply_data) return;

        memcpy(reply_data, data, length);
        struct icmp_header *reply_icmp = (struct icmp_header *)reply_data;

        reply_icmp->type = ICMP_TYPE_ECHO_REPLY;
        reply_icmp->code = 0;
        reply_icmp->checksum = 0;
        reply_icmp->checksum = calculate_checksum(reply_data, reply_len);

        net_ipv4_send(ipv4->src_ip, 1, reply_data, reply_len);

        kfree(reply_data);
    }
}

void icmp_send_ping(struct net_device *dev, uint32_t dest_ip) {
    (void)dev;
    size_t length = sizeof(struct icmp_header) + 32; // 32 bytes of payload
    uint8_t *data = kmalloc(length);
    if (!data) return;

    struct icmp_header *icmp = (struct icmp_header *)data;
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->id = htons(0x1234);
    icmp->seq = htons(1);
    
    for (int i = 0; i < 32; i++) {
        data[sizeof(struct icmp_header) + i] = 'A' + (i % 26);
    }
    
    icmp->checksum = 0;
    icmp->checksum = calculate_checksum(data, length);

    net_ipv4_send(dest_ip, 1, data, length);

    kfree(data);
}
