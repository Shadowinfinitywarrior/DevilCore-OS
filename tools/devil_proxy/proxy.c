# tools/devil_proxy/proxy.c
// HTTP Proxy tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 8192

static int proxy_connect(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: devil_proxy <host> <port>\n");
        return 1;
    }
    
    int port = atoi(argv[2]);
    int client_sock = proxy_connect(argv[1], port);
    
    if (client_sock < 0) {
        printf("Connection failed\n");
        return 1;
    }
    
    printf("Connected to %s:%d\n", argv[1], port);
    
    char buf[BUFFER_SIZE];
    while (1) {
        int n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) break;
        write(client_sock, buf, n);
        
        n = read(client_sock, buf, sizeof(buf));
        if (n <= 0) break;
        write(STDOUT_FILENO, buf, n);
    }
    
    close(client_sock);
    return 0;
}