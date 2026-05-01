#ifndef DEVILCORE_ICMP_H
#define DEVILCORE_ICMP_H

#include "net.h"
#include <stdint.h>

void icmp_handle_packet(struct net_device *dev, void *ipv4_hdr, void *data, size_t length);
void icmp_send_ping(struct net_device *dev, uint32_t dest_ip);

#endif
