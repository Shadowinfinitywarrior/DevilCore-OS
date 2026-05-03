#include "ipc.h"
#include "memory.h"
#include "slab.h"
#include "scheduler.h"
#include <string.h>

extern void kprintf(const char *fmt, ...);
extern pid_t task_getpid(void);

static service_entry_t *service_registry = NULL;
static uint32_t next_channel_id = 1;
static uint8_t ipc_initialized = 0;

void ipc_init(void) {
    if (ipc_initialized) return;
    
    kprintf("Initializing IPC system...\n");
    
    service_registry = NULL;
    next_channel_id = 1;
    ipc_initialized = 1;
    
    kprintf("IPC system initialized\n");
}

// Message queue implementation
msg_queue_t *msg_queue_create(uint32_t owner_pid, uint32_t capacity) {
    msg_queue_t *queue = slab_alloc(cache_128);
    if (!queue) return NULL;
    
    queue->buffer = kmalloc(capacity * sizeof(message_t));
    if (!queue->buffer) {
        slab_free(cache_128, queue);
        return NULL;
    }
    
    queue->head = 0;
    queue->tail = 0;
    queue->capacity = capacity;
    queue->count = 0;
    queue->owner_pid = owner_pid;
    
    spin_init(&queue->lock);
    event_init(&queue->not_empty);
    event_init(&queue->not_full);
    
    return queue;
}

void msg_queue_destroy(msg_queue_t *queue) {
    if (!queue) return;
    
    if (queue->buffer) kfree(queue->buffer);
    slab_free(cache_128, queue);
}

int msg_queue_send(msg_queue_t *queue, message_t *msg, bool blocking) {
    if (!queue || !msg) return -1;
    
    spin_lock(&queue->lock);
    
    // Check if full
    if (queue->count >= queue->capacity) {
        spin_unlock(&queue->lock);
        if (blocking) {
            // Wait for space (simplified - should use proper blocking)
            while (queue->count >= queue->capacity) {
                scheduler_yield();
            }
            return msg_queue_send(queue, msg, false);
        }
        return -1;  // Would block
    }
    
    // Add message
    queue->buffer[queue->tail] = *msg;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;
    
    // Signal that queue is not empty
    event_signal(&queue->not_empty);
    
    spin_unlock(&queue->lock);
    return 0;
}

int msg_queue_receive(msg_queue_t *queue, message_t *msg, bool blocking) {
    if (!queue || !msg) return -1;
    
    spin_lock(&queue->lock);
    
    // Check if empty
    if (queue->count == 0) {
        spin_unlock(&queue->lock);
        if (blocking) {
            // Wait for message
            while (queue->count == 0) {
                scheduler_yield();
            }
            return msg_queue_receive(queue, msg, false);
        }
        return -1;  // Would block
    }
    
    // Remove message
    *msg = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    
    // Signal that queue is not full
    event_signal(&queue->not_full);
    
    spin_unlock(&queue->lock);
    return 0;
}

// Channel implementation
ipc_channel_t *ipc_channel_create(uint32_t server_pid, uint32_t client_pid) {
    ipc_channel_t *ch = slab_alloc(cache_128);
    if (!ch) return NULL;
    
    ch->id = next_channel_id++;
    ch->server_pid = server_pid;
    ch->client_pid = client_pid;
    
    // Create queues
    ch->server_to_client = msg_queue_create(server_pid, 64);
    ch->client_to_server = msg_queue_create(client_pid, 64);
    
    if (!ch->server_to_client || !ch->client_to_server) {
        if (ch->server_to_client) msg_queue_destroy(ch->server_to_client);
        if (ch->client_to_server) msg_queue_destroy(ch->client_to_server);
        slab_free(cache_128, ch);
        return NULL;
    }
    
    return ch;
}

void ipc_channel_destroy(ipc_channel_t *ch) {
    if (!ch) return;
    
    if (ch->server_to_client) msg_queue_destroy(ch->server_to_client);
    if (ch->client_to_server) msg_queue_destroy(ch->client_to_server);
    
    slab_free(cache_128, ch);
}

ipc_channel_t *ipc_connect(const char *service_name) {
    uint32_t server_pid = ipc_lookup_service(service_name);
    if (server_pid == 0) return NULL;
    
    uint32_t client_pid = task_getpid();
    
    ipc_channel_t *ch = ipc_channel_create(server_pid, client_pid);
    if (!ch) return NULL;
    
    // Send connection request
    message_t msg;
    MSG_INIT(msg, MSG_PING, client_pid, server_pid);
    msg_queue_send(ch->client_to_server, &msg, true);
    
    return ch;
}

void ipc_close(ipc_channel_t *ch) {
    if (!ch) return;
    ipc_channel_destroy(ch);
}

int ipc_channel_send(ipc_channel_t *ch, message_t *msg, bool to_server) {
    if (!ch || !msg) return -1;
    
    if (to_server) {
        return msg_queue_send(ch->client_to_server, msg, true);
    } else {
        return msg_queue_send(ch->server_to_client, msg, true);
    }
}

int ipc_channel_receive(ipc_channel_t *ch, message_t *msg, bool from_server) {
    if (!ch || !msg) return -1;
    
    if (from_server) {
        return msg_queue_receive(ch->client_to_server, msg, true);
    } else {
        return msg_queue_receive(ch->server_to_client, msg, true);
    }
}

// Basic IPC operations
int ipc_send(uint32_t pid, message_t *msg, bool blocking) {
    (void)blocking;
    if (!msg) return -1;
    
    // For now, just print (would need proper PID-based routing)
    kprintf("IPC send to PID %u: type=%d\n", pid, msg->header.type);
    return 0;
}

int ipc_receive(message_t *msg, bool blocking) {
    (void)blocking;
    if (!msg) return -1;
    
    // Simplified - would need message queue per process
    return -1;
}

int ipc_call(uint32_t pid, message_t *req, message_t *resp) {
    if (!req || !resp) return -1;
    
    // Send request
    int ret = ipc_send(pid, req, true);
    if (ret != 0) return ret;
    
    // Wait for response
    uint32_t original_seq = req->header.seq_num;
    
    while (1) {
        ret = ipc_receive(resp, true);
        if (ret == 0 && resp->header.seq_num == original_seq) {
            return 0;
        }
    }
}

// Service registry
int ipc_register_service(const char *name, uint32_t pid) {
    if (!name || !ipc_initialized) return -1;
    
    // Check if already exists
    service_entry_t *existing = service_registry;
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            return -1;  // Already registered
        }
        existing = existing->next;
    }
    
    // Create new entry
    service_entry_t *entry = slab_alloc(cache_64);
    if (!entry) return -1;
    
    strncpy(entry->name, name, 63);
    entry->name[63] = '\0';
    entry->pid = pid;
    entry->channel = NULL;
    entry->next = service_registry;
    service_registry = entry;
    
    kprintf("Registered service '%s' at PID %u\n", name, pid);
    return 0;
}

int ipc_unregister_service(const char *name) {
    if (!name || !ipc_initialized) return -1;
    
    service_entry_t **current = &service_registry;
    
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            service_entry_t *to_remove = *current;
            *current = (*current)->next;
            slab_free(cache_64, to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;  // Not found
}

uint32_t ipc_lookup_service(const char *name) {
    if (!name || !ipc_initialized) return 0;
    
    service_entry_t *entry = service_registry;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry->pid;
        }
        entry = entry->next;
    }
    
    return 0;  // Not found
}
