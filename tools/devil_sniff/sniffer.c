// tools/devil_sniff/packet_sniffer.c
// Packet Sniffer (tcpdump-like)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <pcap.h>

static void print_ethernet(struct ether_header *eth) {
    printf("SRC: %02x:%02x:%02x:%02x:%02x:%02x -> ",
          eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],
          eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);
    printf("DST: %02x:%02x:%02x:%02x:%02x:%02x ",
          eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],
          eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);
}

static void print_ip(struct ip *ip) {
    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &ip->ip_src, src, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip->ip_dst, dst, INET_ADDRSTRLEN);
    
    printf("IP %s -> %s ", src, dst);
}

static void print_tcp(struct tcphdr *tcp) {
    printf("TCP %d -> %d ", ntohs(tcp->th_sport), ntohs(tcp->th_dport));
    if (tcp->th_flags & TH_SYN) printf("[SYN] ");
    if (tcp->th_flags & TH_ACK) printf("[ACK] ");
    if (tcp->th_flags & TH_FIN) printf("[FIN] ");
    if (tcp->th_flags & TH_RST) printf("[RST] ");
    if (tcp->th_flags & TH_PUSH) printf("[PSH] ");
}

static void print_udp(struct udphdr *udp) {
    printf("UDP %d -> %d ", ntohs(udp->uh_sport), ntohs(udp->uh_dport));
}

void packet_handler(u_char *user, const struct pcap_pkthdr *h, const u_char *packet) {
    struct ether_header *eth = (struct ether_header*)packet;
    
    printf("%02d:%02d.%02d ", 0, 0, 0);
    print_ethernet(eth);
    
    uint16_t etype = ntohs(eth->ether_type);
    
    if (etype == ETHERTYPE_IP) {
        struct ip *ip = (struct ip*)(packet + sizeof(struct ether_header));
        print_ip(ip);
        
        switch (ip->ip_p) {
            case IPPROTO_TCP: {
                struct tcphdr *tcp = (struct tcphdr*)((uint8_t*)ip + ip->ip_hl * 4);
                print_tcp(tcp);
                break;
            }
            case IPPROTO_UDP: {
                struct udphdr *udp = (struct udphdr*)((uint8_t*)ip + ip->ip_hl * 4);
                print_udp(udp);
                break;
            }
        }
    }
    
    printf("\n");
}

static void usage(const char *prog) {
    printf("Usage: %s [options] [filter]\n", prog);
    printf("Options:\n");
    printf("  -i <iface>  Interface (default: any)\n");
    printf("  -c <count>  Packet count\n");
    printf("  -v          Verbose\n");
    printf("Examples:\n");
    printf("  %s\n", prog);
    printf("  %s -i eth0 tcp\n", prog);
}

int main(int argc, char **argv) {
    const char *iface = NULL;
    const char *filter = "";
    int count = -1;
    int verbose = 0;
    
    int opt;
    while ((opt = getopt(argc, argv, "i:c:vh")) != -1) {
        switch (opt) {
            case 'i':
                iface = optarg;
                break;
            case 'c':
                count = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    if (optind < argc) {
        filter = argv[optind];
    }
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    
    if (iface) {
        handle = pcap_open_live(iface, 65535, 1, 1000, errbuf);
    } else {
        handle = pcap_open_live("any", 65535, 1, 1000, errbuf);
    }
    
    if (!handle) {
        fprintf(stderr, "Error: %s\n", errbuf);
        return 1;
    }
    
    struct bpf_program fp;
    if (pcap_compile(handle, &fp, filter, 0, 0) == -1) {
        fprintf(stderr, "Filter error: %s\n", pcap_geterr(handle));
        return 1;
    }
    
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Setfilter error: %s\n", pcap_geterr(handle));
        return 1;
    }
    
    printf("[DevilSniff] Sniffing on %s\n", iface ? iface : "any");
    pcap_loop(handle, count, packet_handler, (u_char*)&verbose);
    
    pcap_close(handle);
    return 0;
}