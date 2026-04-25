/*
 * DevilCore OS — Networking Diagnostics
 * Items 91-100
 */

#include "diag.h"
#include "../net/net.h"
#include "../net/socket.h"
#include "../net/ethernet.h"
#include "../net/ip.h"
#include "../net/arp.h"
#include "../net/tcp.h"
#include "../net/udp.h"
#include "../include/string.h"

/* ================================================================
 * NETWORK INTERFACES (Item 91)
 * ================================================================ */
static void diag_net_interfaces(diag_output_t *out) {
    diag_header(out, "Network Interfaces");

    net_interface_t ifaces[MAX_NET_IFACES];
    int nifaces = net_get_interfaces(ifaces, MAX_NET_IFACES);

    if (nifaces == 0) {
        diag_printf_user(out, "  No network interfaces detected.\n");
        diag_printf_user(out, "  Check if network driver is loaded.\n");
        return;
    }

    diag_printf_user(out, "  IF    Type       HW Addr           IP Addr       Mask        Status   MTU\n");
    diag_printf_user(out, "  --    ---------  -----------------  ------------  ----------  -------  ----\n");

    for (int i = 0; i < nifaces; i++) {
        const char *status = "DOWN";
        if (ifaces[i].flags & IFF_UP)    status = "UP";
        if (ifaces[i].flags & IFF_RUNNING) status = "RUNNING";

        diag_printf_user(out, "  %-2d    %-10s  %02X:%02X:%02X:%02X:%02X:%02X  %-12s  %-10s  %-7s  %d\n",
            i,
            ifaces[i].type == IFACE_ETHERNET ? "Ethernet" :
            ifaces[i].type == IFACE_LOOPBACK ? "Loopback" : "Unknown",
            ifaces[i].hwaddr[0], ifaces[i].hwaddr[1],
            ifaces[i].hwaddr[2], ifaces[i].hwaddr[3],
            ifaces[i].hwaddr[4], ifaces[i].hwaddr[5],
            ip_to_str(ifaces[i].ip_addr),
            ip_to_str(ifaces[i].netmask),
            status,
            ifaces[i].mtu);
    }

    diag_printf_user(out, "\n  Total interfaces: %d\n", nifaces);
    diag_printf_user(out, "  Active: %d\n", net_active_interfaces());
    diag_printf_user(out, "  Link Speed: %s\n", net_get_link_speed_str());
}

/* ================================================================
 * ARP CACHE TABLE (Item 92)
 * ================================================================ */
static void diag_net_arp(diag_output_t *out) {
    diag_header(out, "ARP Cache Table");

    arp_entry_t arp_cache[32];
    int narcs = arp_get_cache(arp_cache, 32);

    if (narcs == 0) {
        diag_printf_user(out, "  ARP cache is empty.\n");
        return;
    }

    diag_printf_user(out, "  IP Addr            HW Addr           Interface  Timestamp\n");
    diag_printf_user(out, "  -----------------  ----------------  ---------  ------------------\n");

    for (int i = 0; i < narcs; i++) {
        diag_printf_user(out, "  %-20s  %02X:%02X:%02X:%02X:%02X:%02X   %-9d  %u\n",
            ip_to_str(arp_cache[i].ip),
            arp_cache[i].hwaddr[0], arp_cache[i].hwaddr[1],
            arp_cache[i].hwaddr[2], arp_cache[i].hwaddr[3],
            arp_cache[i].hwaddr[4], arp_cache[i].hwaddr[5],
            arp_cache[i].iface,
            arp_cache[i].timestamp);
    }

    diag_printf_user(out, "\n  Total ARP entries: %d\n", narcs);
    diag_printf_user(out, "  ARP requests sent: %llu\n", arp_requests_sent());
    diag_printf_user(out, "  ARP replies received: %llu\n", arp_replies_received());
}

/* ================================================================
 * ALL SOCKETS (Item 93)
 * ================================================================ */
static void diag_net_sockets(diag_output_t *out) {
    diag_header(out, "Socket Table");

    socket_info_t sinfo[MAX_SOCKETS];
    int nsocks = socket_get_info(sinfo, MAX_SOCKETS);

    if (nsocks == 0) {
        diag_printf_user(out, "  No sockets open.\n");
        return;
    }

    diag_printf_user(out, "  #  FD     Domain   Type      Proto    Local            Remote           State\n");
    diag_printf_user(out, "  -  -----  --------  --------  -------  ---------------  ---------------  ------\n");

    for (int i = 0; i < nsocks; i++) {
        const char *state_str = "UNKNOWN";
        switch (sinfo[i].state) {
            case SOCKET_CREATED:   state_str = "CREATED"; break;
            case SOCKET_BOUND:     state_str = "BOUND"; break;
            case SOCKET_CONNECTING: state_str = "CONNECT"; break;
            case SOCKET_CONNECTED: state_str = "CONNECTED"; break;
            case SOCKET_LISTENING: state_str = "LISTEN"; break;
            case SOCKET_CLOSING:   state_str = "CLOSING"; break;
            case SOCKET_CLOSED:    state_str = "CLOSED"; break;
        }

        const char *type_str = "UNK";
        switch (sinfo[i].type) {
            case SOCK_STREAM:  type_str = "TCP"; break;
            case SOCK_DGRAM:   type_str = "UDP"; break;
            case SOCK_RAW:     type_str = "RAW"; break;
        }

        diag_printf_user(out, "  %-2d  %-5d  %-8d  %-8s  %-7d  %-15s  %-15s  %s\n",
            i,
            sinfo[i].fd,
            sinfo[i].domain,
            type_str,
            sinfo[i].protocol,
            ip_port_to_str(sinfo[i].local_addr, sinfo[i].local_port),
            ip_port_to_str(sinfo[i].remote_addr, sinfo[i].remote_port),
            state_str);
    }

    diag_printf_user(out, "\n  Total sockets: %d\n", nsocks);
    diag_printf_user(out, "  TCP sockets: %d\n", socket_count_type(SOCK_STREAM));
    diag_printf_user(out, "  UDP sockets: %d\n", socket_count_type(SOCK_DGRAM));
}

/* ================================================================
 * TCP CONNECTION TABLE (Item 94)
 * ================================================================ */
static void diag_net_tcp_conn(diag_output_t *out) {
    diag_header(out, "TCP Connection Table");

    tcp_connection_t conns[MAX_TCP_CONNS];
    int nconns = tcp_get_connections(conns, MAX_TCP_CONNS);

    if (nconns == 0) {
        diag_printf_user(out, "  No TCP connections.\n");
        return;
    }

    diag_printf_user(out, "  #  Local Addr:Port       Remote Addr:Port      State      Seq       Ack       Win\n");
    diag_printf_user(out, "  -  -------------------   -------------------   --------   --------  --------  ----\n");

    for (int i = 0; i < nconns; i++) {
        const char *tcp_state = "UNKNOWN";
        switch (conns[i].state) {
            case TCP_CLOSED:     tcp_state = "CLOSED"; break;
            case TCP_LISTEN:     tcp_state = "LISTEN"; break;
            case TCP_SYN_SENT:   tcp_state = "SYN-SENT"; break;
            case TCP_SYN_RECV:   tcp_state = "SYN-RECV"; break;
            case TCP_ESTABLISHED:tcp_state = "ESTAB"; break;
            case TCP_FIN_WAIT1:  tcp_state = "FIN-W1"; break;
            case TCP_FIN_WAIT2:  tcp_state = "FIN-W2"; break;
            case TCP_TIME_WAIT:  tcp_state = "TIME-WAIT"; break;
            case TCP_CLOSE_WAIT: tcp_state = "CLOSE-WAIT"; break;
            case TCP_LAST_ACK:   tcp_state = "LAST-ACK"; break;
            case TCP_CLOSING:    tcp_state = "CLOSING"; break;
        }

        diag_printf_user(out, "  %-2d  %-20s  %-20s  %-9s  0x%08X  0x%08X  %5u\n",
            i,
            ip_port_to_str(conns[i].local_ip, conns[i].local_port),
            ip_port_to_str(conns[i].remote_ip, conns[i].remote_port),
            tcp_state,
            conns[i].seq_num,
            conns[i].ack_num,
            conns[i].window_size);
    }

    diag_printf_user(out, "\n  Total TCP connections: %d\n", nconns);
    diag_printf_user(out, "  TIME-WAIT sockets: %d\n", tcp_count_state(TCP_TIME_WAIT));
}

/* ================================================================
 * PACKET BUFFER RING (Item 95)
 * ================================================================ */
static void diag_net_pkt_buffer(diag_output_t *out) {
    diag_header(out, "Packet Buffer Ring (Ring Buffer)");

    net_ring_buffer_t ring;
    net_get_ring_buffer_stats(&ring);

    diag_printf_user(out, "  Receive Ring Buffer:\n");
    diag_printf_user(out, "    Capacity:  %u packets\n", ring.capacity);
    diag_printf_user(out, "    Enqueued:  %u packets\n", ring.enqueued);
    diag_printf_user(out, "    Dequeued:  %u packets\n", ring.dequeued);
    diag_printf_user(out, "    Free:      %u packets\n", ring.capacity - ring.enqueued);

    diag_printf_user(out, "\n  Head:  %u\n", ring.head);
    diag_printf_user(out, "  Tail:  %u\n", ring.tail);
    diag_printf_user(out, "  Overruns: %u\n", ring.overruns);

    diag_printf_user(out, "\n  Packet Sizes:\n");
    diag_printf_user(out, "    Small (<64B):   %u\n", ring.pkt_counts[0]);
    diag_printf_user(out, "    Medium (64-256):%u\n", ring.pkt_counts[1]);
    diag_printf_user(out, "    Large (>256):   %u\n", ring.pkt_counts[2]);

    diag_printf_user(out, "\n  Drop Policy: %s\n",
        ring.policy == RING_DROP_NEWEST ? "Drop newest" :
        ring.policy == RING_DROP_OLDEST ? "Drop oldest" : "None");
}

/* ================================================================
 * NETWORK DRIVER ERROR COUNTERS (Item 96)
 * ================================================================ */
static void diag_net_errors(diag_output_t *out) {
    diag_header(out, "Network Driver Error Counters");

    net_error_stats_t err;
    net_get_error_stats(&err);

    diag_printf_user(out, "  Ethernet Driver (e1000/eepro100):\n");
    diag_printf_user(out, "    CRC Errors:        %llu\n", err.crc_errors);
    diag_printf_user(out, "    Frame Align Errors:%llu\n", err.frame_align_errors);
    diag_printf_user(out, "    Collisions:        %llu\n", err.collisions);
    diag_printf_user(out, "    Late Collisions:   %llu\n", err.late_collisions);
    diag_printf_user(out, "    Carrier Loss:      %llu\n", err.carrier_loss);
    diag_printf_user(out, "    FIFO Overruns:     %llu\n", err.fifo_overruns);
    diag_printf_user(out, "    Missed Packets:    %llu\n", err.missed_packets);
    diag_printf_user(out, "    Tx Underrun:       %llu\n", err.tx_underrun);
    diag_printf_user(out, "    Rx Overrun:        %llu\n", err.rx_overrun);
    diag_printf_user(out, "    DMA Errors:        %llu\n", err.dma_errors);

    diag_printf_user(out, "\n  Total Errors: %llu\n", err.total_errors);
    diag_printf_user(out, "  Error Rate: %.2f errors/packet\n",
        (double)err.total_errors / (double)(err.packets_rx + err.packets_tx + 1));
}

/* ================================================================
 * FIREWALL RULES (Item 97)
 * ================================================================ */
static void diag_net_firewall(diag_output_t *out) {
    diag_header(out, "Firewall Rules");

#ifdef CONFIG_FIREWALL
    fw_rule_t rules[64];
    int nrules = fw_get_rules(rules, 64);

    if (nrules == 0) {
        diag_printf_user(out, "  No firewall rules (firewall disabled).\n");
        return;
    }

    diag_printf_user(out, "  #  Chain     Action   Proto   Src IP           Dst IP           Ports\n");
    diag_printf_user(out, "  -  --------  -------  ------  ---------------  ---------------  ------\n");

    for (int i = 0; i < nrules; i++) {
        const char *action = rules[i].action ? "ACCEPT" : "DROP";
        const char *proto = "ANY";
        if (rules[i].proto == IPPROTO_TCP) proto = "TCP";
        else if (rules[i].proto == IPPROTO_UDP) proto = "UDP";
        else if (rules[i].proto == IPPROTO_ICMP) proto = "ICMP";

        diag_printf_user(out, "  %-2d  %-8s  %-7s  %-6s  %-15s  %-15s  %s\n",
            i,
            rules[i].chain == FW_INPUT ? "INPUT" :
            rules[i].chain == FW_OUTPUT ? "OUTPUT" : "FORWARD",
            action,
            proto,
            ip_to_str(rules[i].src_ip),
            ip_to_str(rules[i].dst_ip),
            rules[i].port_range);
    }

    diag_printf_user(out, "\n  Total rules: %d\n", nrules);
    diag_printf_user(out, "  Packets matched: %llu\n", fw_total_matches());
    diag_printf_user(out, "  Packets dropped: %llu\n", fw_total_drops());
#else
    diag_printf_user(out, "  Firewall: Not implemented\n");
#endif
}

/* ================================================================
 * DNS RESOLVER CACHE (Item 98)
 * ================================================================ */
static void diag_net_dns(diag_output_t *out) {
    diag_header(out, "DNS Resolver Cache");

#ifdef CONFIG_DNS
    dns_cache_entry_t dns_entries[32];
    int ndns = dns_get_cache(dns_entries, 32);

    if (ndns == 0) {
        diag_printf_user(out, "  DNS cache is empty.\n");
        return;
    }

    diag_printf_user(out, "  #  Hostname               IP Addr           TTL    LastUsed\n");
    diag_printf_user(out, "  -  ---------------------  ---------------  -----  --------\n");

    for (int i = 0; i < ndns; i++) {
        time_t t = dns_entries[i].last_used;
        diag_printf_user(out, "  %-2d  %-20s  %-15s  %-5d  %u\n",
            i,
            dns_entries[i].hostname,
            ip_to_str(dns_entries[i].ip),
            dns_entries[i].ttl,
            (uint32_t)t);
    }

    diag_printf_user(out, "\n  DNS Queries: %llu total\n", dns_query_count());
    diag_printf_user(out, "  Cache Hits:  %llu (%.1f%%)\n",
        dns_cache_hits(),
        dns_cache_hits() * 100.0 / (dns_query_count() + 1));
    diag_printf_user(out, "  Cache Misses:%llu\n", dns_cache_misses());
    diag_printf_user(out, "  Resolver:    %s\n", dns_get_resolver_ip_str());
#else
    diag_printf_user(out, "  DNS Resolver: Not implemented\n");
#endif
}

/* ================================================================
 * NETWORK THROUGHPUT (Item 99)
 * ================================================================ */
static void diag_net_throughput(diag_output_t *out) {
    diag_header(out, "Network Throughput");

    net_throughput_t stats;
    net_get_throughput(&stats);

    diag_printf_user(out, "  Statistics (last 60 seconds):\n");
    diag_printf_user(out, "    RX: %.2f Mbps (%.1f MB/s)\n",
        stats.rx_mbps, stats.rx_mbps / 8.0);
    diag_printf_user(out, "    TX: %.2f Mbps (%.1f MB/s)\n",
        stats.tx_mbps, stats.tx_mbps / 8.0);
    diag_printf_user(out, "    Total: %.2f Mbps\n", stats.rx_mbps + stats.tx_mbps);

    diag_printf_user(out, "\n  Packets:\n");
    diag_printf_user(out, "    RX: %llu packets\n", stats.rx_packets);
    diag_printf_user(out, "    TX: %llu packets\n", stats.tx_packets);
    diag_printf_user(out, "    Dropped RX: %llu\n", stats.rx_dropped);
    diag_printf_user(out, "    Dropped TX: %llu\n", stats.tx_dropped);

    diag_printf_user(out, "\n  Interface Utilization:\n");
    for (int i = 0; i < stats.nifaces; i++) {
        diag_printf_user(out, "    %s: %.1f%% of bandwidth\n",
            stats.iface_name[i],
            stats.utilization_pct[i]);
    }
}

/* ================================================================
 * ROUTING TABLE (Item 100)
 * ================================================================ */
static void diag_net_routing(diag_output_t *out) {
    diag_header(out, "Routing Table");

    route_entry_t routes[32];
    int nroutes = net_get_routes(routes, 32);

    if (nroutes == 0) {
        diag_printf_user(out, "  No routing entries.\n");
        diag_printf_user(out, "  Only local/subnet routes available.\n");
        return;
    }

    diag_printf_user(out, "  #  Destination     Gateway         Genmask          IFace  Metric\n");
    diag_printf_user(out, "  -  --------------  --------------  ---------------  -----  ------\n");

    for (int i = 0; i < nroutes; i++) {
        diag_printf_user(out, "  %-2d  %-15s  %-15s  %-15s  %-5d  %6d\n",
            i,
            ip_to_str(routes[i].destination),
            routes[i].gateway ? ip_to_str(routes[i].gateway) : "0.0.0.0",
            ip_to_str(routes[i].netmask),
            routes[i].iface,
            routes[i].metric);
    }

    diag_printf_user(out, "\n  Total routes: %d\n", nroutes);
    diag_printf_user(out, "  Default route: %s\n",
        net_has_default_route() ? "Yes" : "No");
    diag_printf_user(out, "  Gateway: %s\n", ip_to_str(net_get_default_gateway()));
}

/* ================================================================
 * NETWORK DISPATCHER
 * ================================================================ */
void diag_network_impl(uint64_t subcmd, diag_output_t *out) {
    switch (subcmd) {
        case DIAG_NET_INTERFACES:
            diag_net_interfaces(out);
            break;
        case DIAG_NET_ARP:
            diag_net_arp(out);
            break;
        case DIAG_NET_SOCKETS:
            diag_net_sockets(out);
            break;
        case DIAG_NET_TCP_CONN:
            diag_net_tcp_conn(out);
            break;
        case DIAG_NET_PKT_BUFFER:
            diag_net_pkt_buffer(out);
            break;
        case DIAG_NET_ERRORS:
            diag_net_errors(out);
            break;
        case DIAG_NET_FIREWALL:
            diag_net_firewall(out);
            break;
        case DIAG_NET_DNS:
            diag_net_dns(out);
            break;
        case DIAG_NET_THROUGHPUT:
            diag_net_throughput(out);
            break;
        case DIAG_NET_ROUTING:
            diag_net_routing(out);
            break;
        case 0: /* All network diagnostics */
            diag_net_interfaces(out);
            diag_net_arp(out);
            diag_net_sockets(out);
            diag_net_tcp_conn(out);
            diag_net_pkt_buffer(out);
            diag_net_errors(out);
            diag_net_firewall(out);
            diag_net_dns(out);
            diag_net_throughput(out);
            diag_net_routing(out);
            break;
        default:
            diag_header(out, "Network Diagnostics");
            diag_printf_user(out, "  Unknown subcommand: %lu\n", subcmd);
            break;
    }
}
