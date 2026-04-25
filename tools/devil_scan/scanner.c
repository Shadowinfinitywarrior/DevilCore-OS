// tools/devil_scan/port_scanner.c
// Port Scanner (nmap-like)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_THREADS 64
#define TIMEOUT_MS  2000

typedef struct scan_result {
    uint32_t ip;
    uint16_t port;
    int      state;
} scan_result_t;

static int syn_scan(uint32_t ip, uint16_t port, int timeout) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) return -1;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = port;
    
    // Send SYN
    struct tcphdr tcp;
    memset(&tcp, 0, sizeof(tcp));
    tcp.th_sport = htons(getpid());
    tcp.th_dport = port;
    tcp.th_flags = TH_SYN;
    
    sendto(sock, &tcp, sizeof(tcp), 0, (struct sockaddr*)&addr, sizeof(addr));
    
    // Wait for response (simplified - would use poll/select)
    char buf[1024];
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    int ret = recv(sock, buf, sizeof(buf), 0);
    close(sock);
    
    return ret > 0 ? 1 : 0;
}

static int connect_scan(uint32_t ip, uint16_t port, int timeout) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = port;
    
    // Set non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    // Wait for connection
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    
    int ret = select(sock + 1, NULL, &wfds, NULL, &tv);
    
    close(sock);
    
    return ret > 0 ? 1 : 0;
}

static void scan_port(uint32_t ip, uint16_t port, int type) {
    int state = 0;
    
    switch (type) {
        case 0: // SYN scan (requires root)
            state = syn_scan(ip, port, TIMEOUT_MS);
            break;
        case 1: // Connect scan
            state = connect_scan(ip, port, TIMEOUT_MS);
            break;
        default:
            state = connect_scan(ip, port, TIMEOUT_MS);
    }
    
    if (state > 0) {
        printf("[*] Port %d OPEN\n", port);
    }
}

static void usage(const char *prog) {
    printf("Usage: %s [options] <target>\n", prog);
    printf("Options:\n");
    printf("  -p <ports>  Port range (e.g., 1-1000 or 80,443,8080)\n");
    printf("  -t <type>   Scan type (0=syn, 1=connect)\n");
    printf("  -o <file>  Output file\n");
    printf("Examples:\n");
    printf("  %s 192.168.1.1\n", prog);
    printf("  %s -p 1-1000 10.0.0.1\n", prog);
}

int main(int argc, char **argv) {
    const char *target = NULL;
    char *port_spec = "1-1000";
    int scan_type = 1;
    const char *output = NULL;
    
    int opt;
    while ((opt = getopt(argc, argv, "p:t:o:h")) != -1) {
        switch (opt) {
            case 'p':
                port_spec = optarg;
                break;
            case 't':
                scan_type = atoi(optarg);
                break;
            case 'o':
                output = optarg;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    if (optind >= argc) {
        usage(argv[0]);
        return 1;
    }
    
    target = argv[optind];
    uint32_t ip = inet_addr(target);
    
    printf("[DevilScan] Scanning %s (%s)\n", target, port_spec);
    
    // Parse ports
    if (strchr(port_spec, '-')) {
        int start = atoi(port_spec);
        int end = start;
        char *end_str = strchr(port_spec, '-');
        if (end_str) end = atoi(end_str + 1);
        
        for (int p = start; p <= end; p++) {
            scan_port(ip, htons(p), scan_type);
        }
    }
    
    printf("[DevilScan] Scan complete\n");
    return 0;
}