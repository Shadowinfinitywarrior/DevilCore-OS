// kernel/proc/ipc.c
// Inter-process communication

#include <devil/types.h>
#include "process.h"

#define IPCmsgget   100
#define IPCmsgctl  101
#define IPCmsgsnd  102
#define IPCmsgrcv  103

#define IPCsemget  200
#define IPCsemop  201
#define IPCsemctl 202

#define IPCshmget  300
#define IPCshmat  301
#define IPCshmdt  302
#define IPCshmctl  303

typedef struct ipc_msg {
    long mtype;
    char mtext[256];
} ipc_msg_t;

typedef struct msgq {
    int id;
    key_t key;
    ipc_msg_t messages[16];
    int msg_count;
} msgq_t;

typedef struct shm {
    int id;
    key_t key;
    void *addr;
    size_t size;
} shm_t;

static msgq_t msgqs[16];
static shm_t shms[16];

int ipc_msgget(key_t key, int msgflg) {
    for (int i = 0; i < 16; i++) {
        if (msgqs[i].id == 0) {
            msgqs[i].id = i + 1;
            msgqs[i].key = key;
            msgqs[i].msg_count = 0;
            return i + 1;
        }
    }
    return -1;
}

int ipc_msgsnd(int msgqid, const void *msgp, size_t msgsz, int msgflg) {
    if (msgqid <= 0 || msgqid > 16) return -1;
    msgq_t *q = &msgqs[msgqid - 1];
    
    if (q->msg_count >= 16) return -1;
    
    ipc_msg_t *msg = (ipc_msg_t*)msgp;
    q->messages[q->msg_count++] = *msg;
    return 0;
}

ssize_t ipc_msgrcv(int msgqid, void *msgp, size_t msgsz, long msgtyp, int msgflg) {
    if (msgqid <= 0 || msgqid > 16) return -1;
    msgq_t *q = &msgqs[msgqid - 1];
    
    if (q->msg_count == 0) return -1;
    
    ipc_msg_t *msg = (ipc_msg_t*)msgp;
    *msg = q->messages[0];
    
    for (int i = 0; i < q->msg_count - 1; i++) {
        q->messages[i] = q->messages[i + 1];
    }
    q->msg_count--;
    
    return 0;
}

void *ipc_shmat(int shmid, const void *shmaddr, int shmflg) {
    if (shmid <= 0 || shmid > 16) return NULL;
    shm_t *s = &shms[shmid - 1];
    
    if (s->addr == NULL) {
        s->size = 4096;
        extern void *pmm_alloc(void);
        s->addr = pmm_alloc();
    }
    
    return s->addr;
}