// kernel/security/seccomp.c
// Syscall filtering

#include <devil/types.h>
#include <syscall/syscall.h>

#define SECCOMP_MODE_DISABLED  0
#define SECCOMP_MODE_STRICT  1
#define SECCOMP_MODE_FILTER 2

static int seccomp_mode;

void seccomp_init(void) {
    seccomp_mode = SECCOMP_MODE_DISABLED;
}

int seccomp_enable(int mode) {
    seccomp_mode = mode;
    return 0;
}

int seccomp_check(long nr) {
    if (seccomp_mode == SECCOMP_MODE_DISABLED) return 0;
    
    if (seccomp_mode == SECCOMP_MODE_STRICT) {
        // Only allow read, write, exit, sigreturn
        if (nr == SYS_read || nr == SYS_write || 
            nr == SYS_exit || nr == SYS_rt_sigreturn) {
            return 0;
        }
        return -1;
    }
    
    return 0;
}