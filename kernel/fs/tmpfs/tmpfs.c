// kernel/fs/tmpfs/tmpfs.c

#include <devil/types.h>
#include <devil/config.h>
#include <fs/vfs.h>

static file_system_t tmpfs_fs;

static inode_t *inodes[256];
static int inode_count;

typedef struct tmpfs_inode {
    ino_t ino;
    char name[256];
    mode_t mode;
    uid_t uid;
    gid_t gid;
    off_t size;
    time_t mtime;
    char data[4096];
    struct tmpfs_inode *children[32];
    int child_count;
} tmpfs_inode_t;

static tmpfs_inode_t root = {
    .ino = 1,
    .name = "/",
    .mode = S_IFDIR | 0755,
    .uid = 0,
    .gid = 0,
    .size = 4096,
};

static void *tmpfs_mount_device(const char *device, const char *mount_point) {
    return &root;
}

static int tmpfs_unmount(const char *mount_point) {
    return 0;
}

static void *tmpfs_open(const char *path, int flags) {
    return (void*)1;
}

static int tmpfs_close(void *file) {
    return 0;
}

static ssize_t tmpfs_read(void *file, void *buf, size_t count) {
    return 0;
}

static ssize_t tmpfs_write(void *file, const void *buf, size_t count) {
    return count;
}

static int tmpfs_seek(void *file, off_t offset, int whence) {
    return 0;
}

static int tmpfs_stat(const char *path, stat_t *st) {
    st->st_dev = 0;
    st->st_ino = 1;
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 2;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_size = 4096;
    return 0;
}

static int tmpfs_mkdir(const char *path, mode_t mode) {
    return 0;
}

static int tmpfs_rmdir(const char *path) {
    return 0;
}

static int tmpfs_unlink_path(const char *path) {
    return 0;
}

static int tmpfs_rename(const char *old, const char *new) {
    return 0;
}

static dirent_t *tmpfs_readdir(void *dir) {
    static dirent_t entry;
    entry.d_ino = 1;
    strcpy(entry.d_name, ".");
    return &entry;
}

file_system_t tmpfs_fs = {
    .name = "tmpfs",
    .mount = tmpfs_mount_device,
    .unmount = tmpfs_unmount,
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
    .seek = tmpfs_seek,
    .stat = tmpfs_stat,
    .mkdir = tmpfs_mkdir,
    .rmdir = tmpfs_rmdir,
    .unlink = tmpfs_unlink_path,
    .rename = tmpfs_rename,
    .readdir = tmpfs_readdir,
};