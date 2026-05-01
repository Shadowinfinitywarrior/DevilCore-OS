#include "vfs.h"

#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *dest, int value, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

struct vfs_node *vfs_root;
struct vfs_mount *vfs_mounts;
struct vfs_filesystem *vfs_filesystems;

static int default_open(struct vfs_node *node) {
    (void)node;
    return 0;
}

static int default_close(struct vfs_node *node) {
    (void)node;
    return 0;
}

static int default_read(struct vfs_node *node, void *buffer, size_t size, uint64_t offset) {
    (void)node;
    (void)buffer;
    (void)size;
    (void)offset;
    return -1;
}

static int default_write(struct vfs_node *node, const void *buffer, size_t size, uint64_t offset) {
    (void)node;
    (void)buffer;
    (void)size;
    (void)offset;
    return -1;
}

static int default_ioctl(struct vfs_node *node, uint64_t request, uint64_t arg) {
    (void)node;
    (void)request;
    (void)arg;
    return -1;
}

int vfs_register_filesystem(struct vfs_filesystem *fs) {
    if (fs == NULL) {
        return -1;
    }
    
    fs->next = vfs_filesystems;
    vfs_filesystems = fs;
    
    return 0;
}

int vfs_mount_filesystem(const char *target, struct vfs_filesystem *fs, const char *data) {
    if (target == NULL || fs == NULL) {
        return -1;
    }
    
    struct vfs_node *mount_point = vfs_lookup(target);
    if (mount_point == NULL) {
        return -1;
    }
    
    struct vfs_mount *mount = kmalloc(sizeof(struct vfs_mount));
    if (mount == NULL) {
        return -1;
    }
    
    strncpy(mount->target, target, VFS_PATH_MAX - 1);
    mount->target[VFS_PATH_MAX - 1] = '\0';
    mount->filesystem = fs;
    mount->root = NULL;
    mount->next = vfs_mounts;
    vfs_mounts = mount;
    
    if (fs->mount != NULL) {
        int ret = fs->mount(fs, mount_point, data);
        if (ret < 0) {
            vfs_mounts = mount->next;
            kfree(mount);
            return ret;
        }
        mount->root = mount_point;
    }
    
    return 0;
}

struct vfs_node *vfs_create_node(const char *name, uint32_t type) {
    struct vfs_node *node = kmalloc(sizeof(struct vfs_node));
    if (node == NULL) {
        return NULL;
    }
    
    memset(node, 0, sizeof(struct vfs_node));
    
    if (name != NULL) {
        strncpy(node->name, name, VFS_NAME_MAX - 1);
        node->name[VFS_NAME_MAX - 1] = '\0';
    }
    
    node->type = type;
    node->open = default_open;
    node->close = default_close;
    node->read = node->type == VFS_TYPE_FILE ? default_read : NULL;
    node->write = node->type == VFS_TYPE_FILE ? default_write : NULL;
    node->ioctl = default_ioctl;
    
    return node;
}

void vfs_free_node(struct vfs_node *node) {
    if (node == NULL) {
        return;
    }
    
    if (node->child != NULL) {
        struct vfs_node *child = node->child;
        while (child != NULL) {
            struct vfs_node *next = child->sibling;
            vfs_free_node(child);
            child = next;
        }
    }
    
    kfree(node);
}

static struct vfs_node *parse_path(const char *path, const char **remaining) {
    if (path == NULL || *path == '\0') {
        return vfs_root;
    }
    
    if (*path == '/') {
        ++path;
    }
    
    const char *slash = path;
    while (*slash != '\0' && *slash != '/') {
        ++slash;
    }
    
    size_t name_len = slash - path;
    if (name_len == 0) {
        if (remaining != NULL) {
            *remaining = slash;
        }
        return vfs_root;
    }
    
    struct vfs_node *current = vfs_root;
    char component[VFS_NAME_MAX];
    
    while (*path != '\0') {
        while (*path == '/') {
            ++path;
        }
        
        if (*path == '\0') {
            break;
        }
        
        slash = path;
        name_len = 0;
        while (*slash != '\0' && *slash != '/') {
            ++slash;
            ++name_len;
        }
        
        if (name_len >= VFS_NAME_MAX) {
            return NULL;
        }
        
        memcpy(component, path, name_len);
        component[name_len] = '\0';
        
        current = current->child;
        while (current != NULL) {
            if (strncmp(current->name, component, VFS_NAME_MAX) == 0) {
                break;
            }
            current = current->sibling;
        }
        
        if (current == NULL) {
            return NULL;
        }
        
        if (*slash == '\0') {
            if (remaining != NULL) {
                *remaining = slash;
            }
            return current;
        }
        
        if ((current->type & VFS_TYPE_DIR) == 0) {
            return NULL;
        }
        
        path = slash + 1;
    }
    
    if (remaining != NULL) {
        *remaining = path;
    }
    
    return current;
}

struct vfs_node *vfs_lookup(const char *path) {
    return parse_path(path, NULL);
}

int vfs_add_child(struct vfs_node *parent, struct vfs_node *child) {
    if (parent == NULL || child == NULL) {
        return -1;
    }
    
    if ((parent->type & VFS_TYPE_DIR) == 0) {
        return -1;
    }
    
    child->parent = parent;
    child->sibling = parent->child;
    parent->child = child;
    
    return 0;
}

int vfs_remove_child(struct vfs_node *parent, struct vfs_node *child) {
    if (parent == NULL || child == NULL) {
        return -1;
    }
    
    struct vfs_node *prev = NULL;
    struct vfs_node *current = parent->child;
    
    while (current != NULL) {
        if (current == child) {
            if (prev == NULL) {
                parent->child = current->sibling;
            } else {
                prev->sibling = current->sibling;
            }
            current->parent = NULL;
            current->sibling = NULL;
            return 0;
        }
        prev = current;
        current = current->sibling;
    }
    
    return -1;
}

int vfs_open(struct vfs_file *file, const char *path, uint32_t flags) {
    if (file == NULL || path == NULL) {
        return -1;
    }
    
    struct vfs_node *node = vfs_lookup(path);
    if (node == NULL) {
        return -1;
    }
    
    file->node = node;
    file->position = 0;
    file->flags = flags;
    
    if (node->open != NULL) {
        return node->open(node);
    }
    
    return 0;
}

int vfs_close(struct vfs_file *file) {
    if (file == NULL) {
        return -1;
    }
    
    if (file->node != NULL && file->node->close != NULL) {
        file->node->close(file->node);
    }
    
    file->node = NULL;
    file->position = 0;
    file->flags = 0;
    
    return 0;
}

int vfs_read(struct vfs_file *file, void *buffer, size_t size, size_t *read_bytes) {
    if (file == NULL || file->node == NULL || buffer == NULL) {
        return -1;
    }
    
    if (file->node->read == NULL) {
        return -1;
    }
    
    int result = file->node->read(file->node, buffer, size, file->position);
    if (result >= 0) {
        file->position += result;
        if (read_bytes != NULL) {
            *read_bytes = (size_t)result;
        }
    }
    
    return result;
}

int vfs_write(struct vfs_file *file, const void *buffer, size_t size, size_t *written_bytes) {
    if (file == NULL || file->node == NULL || buffer == NULL) {
        return -1;
    }
    
    if (file->node->write == NULL) {
        return -1;
    }
    
    int result = file->node->write(file->node, buffer, size, file->position);
    if (result >= 0) {
        file->position += result;
        if (written_bytes != NULL) {
            *written_bytes = (size_t)result;
        }
    }
    
    return result;
}

int vfs_seek(struct vfs_file *file, int64_t offset, int whence) {
    if (file == NULL || file->node == NULL) {
        return -1;
    }
    
    uint64_t new_pos = 0;
    
    switch (whence) {
        case 0:
            new_pos = (uint64_t)offset;
            break;
        case 1:
            new_pos = file->position + (uint64_t)offset;
            break;
        case 2:
            new_pos = file->node->size + (uint64_t)offset;
            break;
        default:
            return -1;
    }
    
    file->position = new_pos;
    return 0;
}

int vfs_tell(struct vfs_file *file, uint64_t *position) {
    if (file == NULL || position == NULL) {
        return -1;
    }
    
    *position = file->position;
    return 0;
}

int vfs_mkdir(const char *path, uint32_t mode) {
    (void)mode;
    
    const char *remaining = NULL;
    struct vfs_node *parent = parse_path(path, &remaining);
    
    if (parent == NULL || remaining == NULL || *remaining != '\0') {
        return -1;
    }
    
    const char *name = remaining;
    while (name > path && name[-1] != '/') {
        --name;
    }
    
    if (*name == '\0') {
        return -1;
    }
    
    struct vfs_node *node = vfs_create_node(name, VFS_TYPE_DIR);
    if (node == NULL) {
        return -1;
    }
    
    return vfs_add_child(parent, node);
}

int vfs_rmdir(const char *path) {
    struct vfs_node *node = vfs_lookup(path);
    if (node == NULL) {
        return -1;
    }
    
    if ((node->type & VFS_TYPE_DIR) == 0) {
        return -1;
    }
    
    if (node->parent != NULL) {
        return vfs_remove_child(node->parent, node);
    }
    
    return -1;
}

int vfs_create(const char *path, uint32_t mode) {
    (void)mode;
    
    const char *remaining = NULL;
    struct vfs_node *parent = parse_path(path, &remaining);
    
    if (parent == NULL || remaining == NULL || *remaining != '\0') {
        return -1;
    }
    
    const char *name = remaining;
    while (name > path && name[-1] != '/') {
        --name;
    }
    
    if (*name == '\0') {
        return -1;
    }
    
    struct vfs_node *node = vfs_create_node(name, VFS_TYPE_FILE);
    if (node == NULL) {
        return -1;
    }
    
    return vfs_add_child(parent, node);
}

int vfs_unlink(const char *path) {
    struct vfs_node *node = vfs_lookup(path);
    if (node == NULL) {
        return -1;
    }
    
    if ((node->type & VFS_TYPE_FILE) == 0) {
        return -1;
    }
    
    if (node->parent != NULL) {
        return vfs_remove_child(node->parent, node);
    }
    
    return -1;
}

int vfs_read_directory(struct vfs_node *dir, struct vfs_node **entries, uint32_t max_entries) {
    if (dir == NULL || entries == NULL || max_entries == 0) {
        return -1;
    }
    
    if ((dir->type & VFS_TYPE_DIR) == 0) {
        return -1;
    }
    
    uint32_t count = 0;
    struct vfs_node *child = dir->child;
    
    while (child != NULL && count < max_entries) {
        entries[count++] = child;
        child = child->sibling;
    }
    
    return (int)count;
}

int vfs_init(void) {
    vfs_root = vfs_create_node("/", VFS_TYPE_DIR);
    if (vfs_root == NULL) {
        return -1;
    }
    
    vfs_mounts = NULL;
    vfs_filesystems = NULL;
    
    return 0;
}