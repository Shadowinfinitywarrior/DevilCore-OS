#ifndef DEVILCORE_IPC_H
#define DEVILCORE_IPC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Message types
typedef enum {
    MSG_PING,
    MSG_PONG,
    
    // Display server messages
    MSG_SURFACE_CREATE,
    MSG_SURFACE_DESTROY,
    MSG_SURFACE_COMMIT,
    MSG_SURFACE_DAMAGE,
    MSG_SURFACE_RESIZE,
    MSG_SURFACE_MOVE,
    
    // Input events
    MSG_KEY_EVENT,
    MSG_MOUSE_EVENT,
    MSG_SCROLL_EVENT,
    MSG_TOUCH_EVENT,
    
    // Window decoration
    MSG_WINDOW_CLOSE,
    MSG_WINDOW_MINIMIZE,
    MSG_WINDOW_MAXIMIZE,
    MSG_WINDOW_FOCUS,
    
    // Application messages
    MSG_APP_LAUNCH,
    MSG_APP_QUIT,
    MSG_APP_READY,
    
    // File system
    MSG_FS_OPEN,
    MSG_FS_CLOSE,
    MSG_FS_READ,
    MSG_FS_WRITE,
    MSG_FS_STAT,
    MSG_FS_LIST,
    
    // Custom range
    MSG_USER_START = 0x1000
} msg_type_t;

// Fixed-size message header
typedef struct msg_header {
    msg_type_t type;
    uint32_t sender_pid;
    uint32_t receiver_pid;
    uint32_t seq_num;       // For request/reply matching
    uint32_t payload_size;
    uint32_t flags;
} msg_header_t;

// Message with inline payload (max 256 bytes)
#define MSG_MAX_INLINE 256

typedef struct message {
    msg_header_t header;
    uint8_t payload[MSG_MAX_INLINE];
    
    // For larger payloads: shared memory reference
    uint64_t shm_key;
    uint64_t shm_size;
} message_t;

// Spinlock for synchronization
typedef struct {
    volatile uint32_t locked;
} spinlock_t;

// Event for synchronization
typedef struct {
    volatile uint32_t signaled;
} sync_event_t;

// Message queue
typedef struct msg_queue {
    message_t *buffer;
    uint32_t head, tail;
    uint32_t capacity;
    uint32_t count;
    uint32_t owner_pid;
    
    // Synchronization
    spinlock_t lock;
    sync_event_t not_empty;    // Wake up receivers
    sync_event_t not_full;     // Wake up senders
} msg_queue_t;

// IPC channel (bidirectional)
typedef struct ipc_channel {
    uint32_t id;
    uint32_t server_pid;
    uint32_t client_pid;
    msg_queue_t *server_to_client;
    msg_queue_t *client_to_server;
} ipc_channel_t;

// Service registry entry
typedef struct service_entry {
    char name[64];
    uint32_t pid;
    ipc_channel_t *channel;
    struct service_entry *next;
} service_entry_t;

// IPC API
void ipc_init(void);

// Basic message operations
int ipc_send(uint32_t pid, message_t *msg, bool blocking);
int ipc_receive(message_t *msg, bool blocking);
int ipc_call(uint32_t pid, message_t *req, message_t *resp);  // Synchronous RPC

// Channel operations
ipc_channel_t *ipc_channel_create(uint32_t server_pid, uint32_t client_pid);
void ipc_channel_destroy(ipc_channel_t *ch);
ipc_channel_t *ipc_connect(const char *service_name);
void ipc_close(ipc_channel_t *ch);
int ipc_channel_send(ipc_channel_t *ch, message_t *msg, bool to_server);
int ipc_channel_receive(ipc_channel_t *ch, message_t *msg, bool from_server);

// Message queue operations
msg_queue_t *msg_queue_create(uint32_t owner_pid, uint32_t capacity);
void msg_queue_destroy(msg_queue_t *queue);
int msg_queue_send(msg_queue_t *queue, message_t *msg, bool blocking);
int msg_queue_receive(msg_queue_t *queue, message_t *msg, bool blocking);

// Service registry
int ipc_register_service(const char *name, uint32_t pid);
int ipc_unregister_service(const char *name);
uint32_t ipc_lookup_service(const char *name);

// Helper functions
static inline void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

static inline void spinlock_acquire(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        // Spin
    }
}

static inline void spinlock_release(spinlock_t *lock) {
    __sync_lock_release(&lock->locked);
}

static inline void event_init(sync_event_t *event) {
    event->signaled = 0;
}

static inline void event_signal(sync_event_t *event) {
    event->signaled = 1;
}

static inline void event_clear(sync_event_t *event) {
    event->signaled = 0;
}

static inline bool event_is_signaled(sync_event_t *event) {
    return event->signaled != 0;
}

// Convenience macros for message construction
#define MSG_INIT(msg, t, src, dst) do { \
    memset(&(msg), 0, sizeof(message_t)); \
    (msg).header.type = (t); \
    (msg).header.sender_pid = (src); \
    (msg).header.receiver_pid = (dst); \
    (msg).header.seq_num = 0; \
    (msg).header.payload_size = 0; \
    (msg).header.flags = 0; \
} while(0)

#define MSG_SET_PAYLOAD(msg, data, size) do { \
    uint32_t s = (size) > MSG_MAX_INLINE ? MSG_MAX_INLINE : (size); \
    memcpy((msg).payload, (data), s); \
    (msg).header.payload_size = s; \
} while(0)

#endif // DEVILCORE_IPC_H
