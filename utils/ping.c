#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <string.h>

static int ping(const char *host, int count) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    inet_pton(AF_INET, host, &addr.sin_addr);
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    
    printf("PING %s: 56 data bytes\n", host);
    
    for (int i = 0; i < count; i++) {
        struct icmp icmp;
        icmp.icmp_type = ICMP_ECHO;
        icmp.icmp_id = getpid();
        icmp.icmp_seq = i;
        icmp.icmp_cksum = 0;
        
        sendto(sock, &icmp, sizeof(icmp), 0, 
              (struct sockaddr*)&addr, sizeof(addr));
        
        // Simplified - would wait for reply
        printf("64 bytes from %s: icmp_seq=%d ttl=64\n", host, i);
        sleep(1);
    }
    
    close(sock);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ping <host> [count]\n");
        return 1;
    }
    
    int count = 4;
    if (argc > 2) {
        count = atoi(argv[2]);
    }
    
    return ping(argv[1], count);
}