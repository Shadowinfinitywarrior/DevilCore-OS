/*
 * DevilCore OS — Application Sandboxing Hook
 */
#include "sandbox.h"

sandbox_policy_t g_current_scope = {
    .current_ring = PRIV_RING_0_KERNEL, /* We are still in kernel space until usermode jump */
    .can_network = true,
    .can_write_disk = true,
    .can_spawn_threads = true
};

bool sandbox_check_permission(uint8_t required_privilege) {
    /* Ring 0 can do anything */
    if (g_current_scope.current_ring == PRIV_RING_0_KERNEL) {
        return true;
    }
    
    /* 
     * Ring 3 Process Logic Execution.
     * Check requested permissions against active sandboxed policy 
     */
    if (required_privilege <= g_current_scope.current_ring) {
        return true;
    }
    
    return false;
}
