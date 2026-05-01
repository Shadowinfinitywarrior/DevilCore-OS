#ifndef DEVILCORE_UDP_H
#define DEVILCORE_UDP_H

#include "net.h"
#include <stdint.h>

#define UDP_MAX_SOCKETS 16

struct udp_socket {
    uint16_t local_port;
    uint8_t used;
    // Basic receive buffer (in a real OS this would be a queue)
    uint8_t rx_buffer[2048];
    size_t rx_len;
    uint32_t rx_src_ip;
    uint16_t rx_src_port;
    uint8_t rx_ready;
};

void udp_init(void);
int udp_bind(uint16_t port);
int udp_send(int sock, uint32_t dest_ip, uint16_t dest_port, const void *data, size_t length);
int udp_recv(int sock, void *buf, size_t maxlen, uint32_t *src_ip, uint16_t *src_port);
void udp_handle_packet(struct net_device *dev, void *ipv4_hdr, void *data, size_t length);

#endif
