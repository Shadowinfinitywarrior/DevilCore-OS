// kernel/proc/signal.c
// Signal handling

#include <devil/types.h>
#include "process.h"
#include "thread.h"

#define SIGTERM   15
#define SIGKILL    9
#define SIGSEGV   11
#define SIGINT    2
#define SIGCHLD  18

typedef struct sigaction {
    void (*handler)(int);
    uint64_t flags;
    void (*restorer)(void);
} sigaction_t;

static sigaction_t signal_handlers[64];

void signal_init(void) {
    for (int i = 0; i < 64; i++) {
        signal_handlers[i].handler = NULL;
    }
}

int signal_register(int signum, void (*handler)(int)) {
    if (signum < 0 || signum >= 64) return -1;
    signal_handlers[signum].handler = handler;
    return 0;
}

void signal_raise(int signum) {
    if (signum < 0 || signum >= 64) return;
    
    task_t *task = get_current_task();
    if (!task) return;
    
    void (*handler)(int) = signal_handlers[signum].handler;
    if (handler) {
        handler(signum);
    } else {
        // Default action
        if (signum == SIGKILL || signum == SIGTERM) {
            exit_process(128 + signum);
        } else if (signum == SIGSEGV) {
            panic("Segmentation fault");
        }
    }
}

int sigaction(int signum, const sigaction_t *act, sigaction_t *oldact) {
    if (signum < 0 || signum >= 64) return -1;
    
    if (oldact) {
        *oldact = signal_handlers[signum];
    }
    if (act) {
        signal_handlers[signum] = *act;
    }
    return 0;
}

int kill(pid_t pid, int sig) {
    task_t *task = find_task(pid);
    if (!task) return -1;
    
    signal_raise(sig);
    return 0;
}