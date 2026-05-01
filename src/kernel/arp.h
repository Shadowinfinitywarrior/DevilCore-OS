#ifndef DEVILCORE_ARP_H
#define DEVILCORE_ARP_H

#include "net.h"
#include <stdint.h>

void arp_init(void);
void arp_request(struct net_device *dev, uint32_t target_ip);
uint8_t* arp_lookup(uint32_t ip);
void arp_handle_packet(struct net_device *dev, void *data, size_t length);

#endif
