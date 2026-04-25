# libc/include/fcntl.h
#ifndef _FCNTL_H
#define _FCNTL_H

#include <devil/types.h>

#define O_RDONLY   0
#define O_WRONLY   1
#define O_RDWR    2

#define O_CREAT   0x0100
#define O_EXCL    0x0200
#define O_NOCTTY  0x0400
#define O_TRUNC  0x0800
#define O_APPEND 0x1000
#define O_NONBLOCK 0x2000
#define O_SYNC   0x4000

#define O_DIRECTORY 0x10000
#define O_NOFOLLOW 0x20000

int open(const char *path, int flags, ...);
int creat(const char *path, mode_t mode);

#endif // _FCNTL_H