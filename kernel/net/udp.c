// kernel/net/udp.c
// UDP handling

#include <devil/types.h>
#include <net/socket.h>

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} PACKED udp_header_t;

int udp_send(socket_t *sock, const void *data, size_t len) {
    if (sock->state == 0) return -1;
    return len;
}

int udp_recv(socket_t *sock, void *data, size_t len) {
    if (sock->state == 0) return -1;
    return 0;
}