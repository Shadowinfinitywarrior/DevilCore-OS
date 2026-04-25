/*
 * DevilCore OS — Application Sandboxing Hook
 */
#ifndef DC_SANDBOX_H
#define DC_SANDBOX_H

#include "include/types.h"

/* Privilege Levels */
#define PRIV_RING_0_KERNEL 0
#define PRIV_RING_3_USER   3

/* Active Process Sandbox Settings */
typedef struct {
    uint8_t current_ring;
    bool    can_network;
    bool    can_write_disk;
    bool    can_spawn_threads;
} sandbox_policy_t;

/* Determine if current execution scope has required permissions */
bool sandbox_check_permission(uint8_t required_privilege);

#endif /* DC_SANDBOX_H */
