// kernel/net/tcp.c
// TCP handling

#include <devil/types.h>
#include <net/socket.h>

#define TCP_CLOSED      0
#define TCP_LISTEN     1
#define TCP_SYN_SENT  2
#define TCP_SYN_RCVD  3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1 5
#define TCP_FIN_WAIT2 6
#define TCP_CLOSE_WAIT 7
#define TCP_LAST_ACK  8
#define TCP_TIME_WAIT 9

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

static uint32_t tcp_next_seq[65536];

void tcp_listen(socket_t *sock) {
    sock->state = TCP_LISTEN;
}

void tcp_connect(socket_t *sock) {
    sock->state = TCP_SYN_SENT;
    // Send SYN
}

void tcp_accept(socket_t *sock) {
    sock->state = TCP_SYN_RCVD;
    // Send SYN-ACK
}

void tcp_send(socket_t *sock, const void *data, size_t len) {
    if (sock->state != TCP_ESTABLISHED) return;
    // Send data
}

void tcp_recv(socket_t *sock, void *data, size_t len) {
    if (sock->state != TCP_ESTABLISHED) return;
    // Receive data
}

void tcp_close(socket_t *sock) {
    if (sock->state == TCP_ESTABLISHED) {
        // Send FIN
    }
    sock->state = TCP_CLOSED;
}