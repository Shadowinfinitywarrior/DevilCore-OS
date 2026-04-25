/*
 * DevilCore OS — Virtual File System (VFS) Implementation
 */
#include "vfs.h"
#include "../drivers/ata.h"
#include "../mm/heap.h"
#include "../libc/string.h"

static vfs_node_t *g_vfs_root = NULL;

void vfs_init(void) {
    /* Initialize VFS structures */
    g_vfs_root = NULL;
}

void vfs_mount_root(vfs_node_t *node) {
    g_vfs_root = node;
}

int vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return -1;
}

int vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return -1;
}

void vfs_close(vfs_node_t *node) {
    if (node && node->close) {
        node->close(node);
    }
}

vfs_node_t* vfs_finddir(vfs_node_t *node, char *name) {
    if (node && node->finddir) {
        return node->finddir(node, name);
    }
    return NULL;
}

vfs_node_t* vfs_lookup(const char *path) {
    if (!path || !g_vfs_root) return NULL;
    if (strcmp(path, "/") == 0) return g_vfs_root;

    /* Skip leading slash */
    if (*path == '/') path++;

    char name[64];
    vfs_node_t *curr = g_vfs_root;

    while (*path) {
        int i = 0;
        while (*path && *path != '/' && i < 63) {
            name[i++] = *path++;
        }
        name[i] = 0;

        if (*path == '/') path++;

        vfs_node_t *next = vfs_finddir(curr, name);
        if (!next) return NULL;
        curr = next;
    }

    return curr;
}

/* ==========================================================
 *  Pseudo-Block Device mapping the Raw ATA hard drive
 * ========================================================== */
static int dev_ata_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    uint32_t lba = offset / 512;
    uint32_t sector_count = (size + 511) / 512;
    
    uint8_t *temp = (uint8_t*)kmalloc_aligned(sector_count * 512, 64);
    if (!temp) return -1;

    if (ata_read_sectors(lba, sector_count, temp) == 0) {
        /* Copy precise requested bytes */
        uint32_t sector_offset = offset % 512;
        memcpy(buffer, temp + sector_offset, size);
        kfree(temp);
        return size;
    }
    kfree(temp);
    return -1;
}

static int dev_ata_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    /* Basic pass-through, doesn't handle RMW for unaligned writes yet */
    uint32_t lba = offset / 512;
    uint32_t sector_count = (size + 511) / 512;
    if (offset % 512 == 0 && size % 512 == 0) {
        return ata_write_sectors(lba, sector_count, buffer) == 0 ? size : -1;
    }
    return -1;
}

/* API that higher layers use to mount the generic /dev/sda equivalent */
vfs_node_t* dev_create_ata_node(void) {
    vfs_node_t *node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    memset(node, 0, sizeof(vfs_node_t));
    
    strcpy(node->name, "sda");
    node->type = VFS_BLOCKDEVICE;
    node->read = dev_ata_read;
    node->write = dev_ata_write;
    
    return node;
}