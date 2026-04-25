# libc/include/signal.h
#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <devil/types.h>

typedef void (*sighandler_t)(int);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

#define SIGABRT  1
#define SIGFPE   2
#define SIGILL   3
#define SIGINT   4
#define SIGSEGV  5
#define SIGTERM  6

sighandler_t signal(int signum, sighandler_t handler);
int raise(int sig);
int kill(pid_t pid, int sig);

#endif // _SIGNAL_H