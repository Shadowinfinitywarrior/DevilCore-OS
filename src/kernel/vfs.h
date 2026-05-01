#ifndef DEVILCORE_VFS_H
#define DEVILCORE_VFS_H

#include <stddef.h>
#include <stdint.h>

#define VFS_NAME_MAX  256
#define VFS_PATH_MAX 4096

#define VFS_TYPE_FILE     0x1
#define VFS_TYPE_DIR     0x2
#define VFS_TYPE_DEVICE  0x4
#define VFS_TYPE_PIPE    0x8

struct vfs_node;
struct vfs_filesystem;

typedef int (*vfs_read_fn)(struct vfs_node *, void *, size_t, uint64_t);
typedef int (*vfs_write_fn)(struct vfs_node *, const void *, size_t, uint64_t);
typedef int (*vfs_open_fn)(struct vfs_node *);
typedef int (*vfs_close_fn)(struct vfs_node *);
typedef int (*vfs_ioctl_fn)(struct vfs_node *, uint64_t, uint64_t);

struct vfs_node {
    char name[VFS_NAME_MAX];
    uint32_t type;
    uint32_t flags;
    uint64_t size;
    uint64_t inode;
    
    struct vfs_node *parent;
    struct vfs_node *sibling;
    struct vfs_node *child;
    
    void *device;
    void *filesystem;
    void *private_data;
    
    vfs_read_fn read;
    vfs_write_fn write;
    vfs_open_fn open;
    vfs_close_fn close;
    vfs_ioctl_fn ioctl;
};

struct vfs_mount {
    char target[VFS_PATH_MAX];
    struct vfs_node *root;
    struct vfs_filesystem *filesystem;
    struct vfs_mount *next;
};

struct vfs_filesystem {
    char name[32];
    int (*mount)(struct vfs_filesystem *, struct vfs_node *, const char *);
    struct vfs_node *(*alloc_node)(void);
    void (*free_node)(struct vfs_node *);
    struct vfs_filesystem *next;
};

struct vfs_file {
    struct vfs_node *node;
    uint64_t position;
    uint32_t flags;
};

int vfs_init(void);

int vfs_register_filesystem(struct vfs_filesystem *fs);
int vfs_mount_filesystem(const char *target, struct vfs_filesystem *fs, const char *data);

struct vfs_node *vfs_create_node(const char *name, uint32_t type);
struct vfs_node *vfs_lookup(const char *path);

int vfs_add_child(struct vfs_node *parent, struct vfs_node *child);
int vfs_remove_child(struct vfs_node *parent, struct vfs_node *child);

int vfs_open(struct vfs_file *file, const char *path, uint32_t flags);
int vfs_close(struct vfs_file *file);
int vfs_read(struct vfs_file *file, void *buffer, size_t size, size_t *read_bytes);
int vfs_write(struct vfs_file *file, const void *buffer, size_t size, size_t *written_bytes);
int vfs_seek(struct vfs_file *file, int64_t offset, int whence);
int vfs_tell(struct vfs_file *file, uint64_t *position);

int vfs_mkdir(const char *path, uint32_t mode);
int vfs_rmdir(const char *path);
int vfs_create(const char *path, uint32_t mode);
int vfs_unlink(const char *path);

int vfs_read_directory(struct vfs_node *dir, struct vfs_node **entries, uint32_t max_entries);

extern struct vfs_node *vfs_root;

#endif