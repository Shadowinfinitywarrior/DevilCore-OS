#include "udp.h"
#include "memory.h"
#include <string.h>

static struct udp_socket sockets[UDP_MAX_SOCKETS];

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

void udp_init(void) {
    memset(sockets, 0, sizeof(sockets));
}

int udp_bind(uint16_t port) {
    for (int i = 0; i < UDP_MAX_SOCKETS; i++) {
        if (!sockets[i].used) {
            sockets[i].used = 1;
            sockets[i].local_port = port;
            sockets[i].rx_ready = 0;
            return i;
        }
    }
    return -1;
}

int udp_send(int sock, uint32_t dest_ip, uint16_t dest_port, const void *data, size_t length) {
    if (sock < 0 || sock >= UDP_MAX_SOCKETS || !sockets[sock].used) return -1;
    
    size_t packet_len = sizeof(struct udp_header) + length;
    uint8_t *packet = kmalloc(packet_len);
    if (!packet) return -1;

    struct udp_header *udp = (struct udp_header *)packet;
    udp->src_port = htons(sockets[sock].local_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons((uint16_t)packet_len);
    udp->checksum = 0; // Optional in IPv4

    memcpy(packet + sizeof(struct udp_header), data, length);

    int ret = net_ipv4_send(dest_ip, 17, packet, packet_len); // 17 is UDP
    kfree(packet);
    return ret;
}

int udp_recv(int sock, void *buf, size_t maxlen, uint32_t *src_ip, uint16_t *src_port) {
    if (sock < 0 || sock >= UDP_MAX_SOCKETS || !sockets[sock].used) return -1;
    
    if (!sockets[sock].rx_ready) return 0; // Non-blocking

    size_t copy_len = sockets[sock].rx_len;
    if (copy_len > maxlen) copy_len = maxlen;

    memcpy(buf, sockets[sock].rx_buffer, copy_len);
    if (src_ip) *src_ip = sockets[sock].rx_src_ip;
    if (src_port) *src_port = sockets[sock].rx_src_port;

    sockets[sock].rx_ready = 0; // Consume
    return (int)copy_len;
}

void udp_handle_packet(struct net_device *dev, void *ipv4_hdr_raw, void *data, size_t length) {
    (void)dev;
    if (!data || length < sizeof(struct udp_header)) return;

    struct ipv4_header *ipv4 = (struct ipv4_header *)ipv4_hdr_raw;
    struct udp_header *udp = (struct udp_header *)data;

    uint16_t dest_port = ntohs(udp->dest_port);
    uint16_t src_port = ntohs(udp->src_port);

    size_t payload_len = length - sizeof(struct udp_header);
    void *payload = (uint8_t *)data + sizeof(struct udp_header);

    // Find socket
    for (int i = 0; i < UDP_MAX_SOCKETS; i++) {
        if (sockets[i].used && sockets[i].local_port == dest_port) {
            // Buffer it
            if (payload_len <= sizeof(sockets[i].rx_buffer)) {
                memcpy(sockets[i].rx_buffer, payload, payload_len);
                sockets[i].rx_len = payload_len;
                sockets[i].rx_src_ip = ipv4->src_ip;
                sockets[i].rx_src_port = src_port;
                sockets[i].rx_ready = 1;
            }
            break;
        }
    }
}
