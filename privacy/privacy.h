#ifndef _PRIVACY_H
#define _PRIVACY_H

#include <devil/types.h>

void privacy_init(void);
void privacy_enable(void);
void privacy_disable(void);
int privacy_status(void);

void tor_connect(const char *bridge);
void tor_disconnect(void);
void tor_set_identity(const char *identity);

void vpn_connect(const char *server, const char *key);
void vpn_disconnect(void);
void vpn_set_dns(const char *dns);

void mac_spoof(const char *iface);
void mac_restore(const char *iface);

void dns_over_https(const char *server);
void traffic_obfuscate(int enable);

void mem_wipe(void *ptr, size_t size);
void file_shred(const char *path, int passes);

void audit_log(const char *event);

#endif // _PRIVACY_H