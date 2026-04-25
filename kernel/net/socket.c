// kernel/net/socket.c
// Socket API

#include <devil/types.h>
#include <devil/config.h>
#include "socket.h"

static socket_t sockets[MAX_SOCKETS];

static uint16_t next_port = 1024;

int net_init(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        sockets[i].state = 0;
    }
    return 0;
}

socket_t *socket_create(int domain, int type, int protocol) {
    socket_t *sock = NULL;
    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i].state == 0) {
            sock = &sockets[i];
            break;
        }
    }
    
    if (!sock) return NULL;
    
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    sock->local_port = 0;
    sock->peer_port = 0;
    sock->state = 1;
    
    return sock;
}

int socket_bind(socket_t *sock, uint32_t addr, uint16_t port) {
    if (!sock || sock->state == 0) return -1;
    
    // Check port availability
    if (port == 0) {
        port = next_port++;
    }
    
    sock->local_addr = addr;
    sock->local_port = port;
    
    return 0;
}

int socket_connect(socket_t *sock, uint32_t addr, uint16_t port) {
    if (!sock || sock->state == 0) return -1;
    
    sock->peer_addr = addr;
    sock->peer_port = port;
    sock->state = 2;
    
    // Send SYN
    return 0;
}

int socket_listen(socket_t *sock, int backlog) {
    if (!sock || sock->state == 0) return -1;
    
    sock->state = 3;
    return 0;
}

socket_t *socket_accept(socket_t *sock) {
    if (!sock || sock->state != 3) return NULL;
    
    // Accept incoming connection
    return NULL;
}

ssize_t socket_send(socket_t *sock, const void *buf, size_t len) {
    if (!sock || sock->state < 2) return -1;
    
    ipv4_header_t iph;
    iph.src = sock->local_addr;
    iph.dst = sock->peer_addr;
    
    // Send packet
    return len;
}

ssize_t socket_recv(socket_t *sock, void *buf, size_t len) {
    if (!sock || sock->state < 2) return -1;
    
    // Receive data
    return 0;
}

int socket_close(socket_t *sock) {
    if (!sock || sock->state == 0) return -1;
    
    // Send FIN
    sock->state = 0;
    return 0;
}