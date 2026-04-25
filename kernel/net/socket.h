#ifndef _NET_H
#define _NET_H

#include <devil/types.h>
#include <devil/config.h>

#define ETH_ALEN     6
#define ETH_FRAME_LEN 1514
#define ETH_TYPE_IPV4  0x0800
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IPV6 0x86DD

typedef struct eth_header {
    uint8_t  dhost[ETH_ALEN];
    uint8_t  shost[ETH_ALEN];
    uint16_t type;
} PACKED eth_header_t;

typedef struct ipv4_header {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags_offset;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} PACKED ipv4_header_t;

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} PACKED udp_header_t;

typedef struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  offset_res;
    uint8_t  flags;
    uint16_t win;
    uint16_t checksum;
    uint16_t urgent;
} PACKED tcp_header_t;

#define TCP_FIN  0x01
#define TCP_SYN  0x02
#define TCP_RST  0x04
#define TCP_PSH  0x08
#define TCP_ACK  0x10
#define TCP_URG 0x20

typedef struct socket {
    int      domain;
    int      type;
    int      protocol;
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t peer_addr;
    uint16_t peer_port;
    int      state;
    void    *rx_buffer;
    void    *tx_buffer;
} socket_t;

int net_init(void);
int ethernet_send(uint8_t *dhost, uint16_t type, uint8_t *data, size_t len);
socket_t *socket_create(int domain, int type, int protocol);
int socket_bind(socket_t *sock, uint32_t addr, uint16_t port);
int socket_connect(socket_t *sock, uint32_t addr, uint16_t port);
int socket_listen(socket_t *sock, int backlog);
socket_t *socket_accept(socket_t *sock);
ssize_t socket_send(socket_t *sock, const void *buf, size_t len);
ssize_t socket_recv(socket_t *sock, void *buf, size_t len);
int socket_close(socket_t *sock);

#endif // _NET_H