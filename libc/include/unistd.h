# libc/include/unistd.h
#ifndef _UNISTD_H
#define _UNISTD_H

#include <devil/types.h>

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#define STDIN_FILENO    0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int close(int fd);

int chdir(const char *path);
int fchdir(int fd);
char *getcwd(char *buf, size_t size);

int dup(int oldfd);
int dup2(int oldfd, int newfd);

int execve(const char *path, char *const argv[], char *const envp[]);
pid_t fork(void);
pid_t vfork(void);

int pipe(int filedes[2]);

int rmdir(const char *path);
int unlink(const char *path);
int link(const char *old, const char *new);
int symlink(const char *old, const char *new);
ssize_t readlink(const char *path, char *buf, size_t bufsize);

int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);

int chroot(const char *path);
int sync(void);

unsigned int sleep(unsigned int seconds);
int usleep(unsigned int microseconds);

long sysconf(int name);

#endif // _UNISTD_H