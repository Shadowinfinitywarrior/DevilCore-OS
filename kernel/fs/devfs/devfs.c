// kernel/fs/devfs/devfs.c

#include <devil/types.h>
#include <fs/vfs.h>

static dev_t devices[256];
static int dev_count;

typedef struct devfs_entry {
    const char *name;
    dev_t dev;
    int type;
} devfs_entry_t;

static devfs_entry_t dev_list[] = {
    {"null", 0, 0},
    {"zero", 1, 0},
    {"full", 2, 0},
    {"random", 3, 0},
    {"urandom", 4, 0},
    {"tty", 5, 0},
    {"tty0", 6, 0},
    {NULL, 0, 0}
};

static void *devfs_mount(const char *device, const char *mount_point) {
    return (void*)1;
}

static void *devfs_open(const char *path, int flags) {
    return (void*)1;
}

static ssize_t devfs_read(void *file, void *buf, size_t count) {
    return count;
}

static ssize_t devfs_write(void *file, const void *buf, size_t count) {
    return count;
}

file_system_t devfs = {
    .name = "devfs",
    .mount = devfs_mount,
    .open = devfs_open,
    .read = devfs_read,
    .write = devfs_write,
};