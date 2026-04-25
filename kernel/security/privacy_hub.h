/*
 * DevilCore OS — Privacy Hub (SOCKS5/Tor Routing Stub)
 */
#ifndef DC_PRIVACY_HUB_H
#define DC_PRIVACY_HUB_H

#include "include/types.h"

/* Enable Tor Circuit Emulation mode over TCP Stack */
void privacy_init(void);

/* Pass a raw ethernet/TCP packet into the SOCKS5 loopback */
int privacy_route_packet(uint8_t *payload, uint32_t len);

/* Retrieve active proxy endpoint stats */
const char* privacy_get_status(void);

#endif /* DC_PRIVACY_HUB_H */
