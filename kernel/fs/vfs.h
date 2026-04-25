/*
 * DevilCore OS — Virtual File System (VFS)
 */
#ifndef DC_VFS_H
#define DC_VFS_H

#include "../include/types.h"

#define VFS_FILE        1
#define VFS_DIRECTORY   2
#define VFS_BLOCKDEVICE 3

struct vfs_node;

typedef struct vfs_node {
    char name[64];
    uint32_t length;
    uint32_t type;
    uint32_t inode;
    struct vfs_node *ptr; /* FS-specific pointer */
    
    /* FS Operations */
    int (*read)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    int (*write)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
    void (*close)(struct vfs_node *node);
} vfs_node_t;

/* Initialize VFS */
void vfs_init(void);

/* Mount Root Filesystem */
void vfs_mount_root(vfs_node_t *node);

/* Standard File API */
int vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
int vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void vfs_close(vfs_node_t *node);

#endif /* DC_VFS_H */