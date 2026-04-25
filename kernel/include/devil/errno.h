# kernel/include/errno.h
#ifndef _ERRNO_H
#define _ERRNO_H

#define ENOENT     2
#define EIO        5
#define EBADF      9
#define EAGAIN    11
#define ENOMEM    12
#define EACCES    13
#define EFAULT    14
#define EEXIST    17
#define ENODEV    19
#define ENOTDIR   20
#define EISDIR    21
#define EINVAL    22
#define EPIPE    32
#define ENOSYS    38
#define ENOTEMPTY 39

extern int errno;
#define set_errno(e) (errno = (e))
#define get_errno() (errno)

#endif // _ERRNO_H