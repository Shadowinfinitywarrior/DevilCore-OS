#include "fat.h"

#include "ata.h"
#include "memory.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static uint8_t fat_sector_buffer[512];
static struct fat_dir_entry fat_dir_buffer[16];

static int fat_open(struct vfs_node *node);
static int fat_read(struct vfs_node *node, void *buffer, size_t size, uint64_t offset);
static int fat_write(struct vfs_node *node, const void *buffer, size_t size, uint64_t offset);

static void fat_read_sector(struct fat_private *fat, uint64_t lba, void *buffer) {
    ata_read_sector(fat->device, fat->lba_start + lba, buffer);
}

static void fat_write_sector(struct fat_private *fat, uint64_t lba, const void *buffer) {
    ata_write_sector(fat->device, fat->lba_start + lba, buffer);
}

static uint32_t fat_read_fat_entry(struct fat_private *fat, uint32_t cluster) {
    if (fat->fat_type == FAT12) {
        uint32_t offset = (cluster * 3) / 2;
        uint16_t value = 0;
        fat_read_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
        memcpy(&value, &fat_sector_buffer[offset % 512], sizeof(uint16_t));
        return (cluster & 1) ? (value >> 12) : (value & 0xfff);
    } else if (fat->fat_type == FAT16) {
        uint32_t offset = cluster * 2;
        fat_read_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
        return ((uint16_t *)fat_sector_buffer)[offset % 512];
    } else {
        uint32_t offset = cluster * 4;
        fat_read_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
        return ((uint32_t *)fat_sector_buffer)[offset % 512];
    }
}

static void fat_write_fat_entry(struct fat_private *fat, uint32_t cluster, uint32_t value) {
    if (fat->fat_type == FAT12) {
    } else if (fat->fat_type == FAT16) {
        uint32_t offset = cluster * 2;
        fat_read_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
        ((uint16_t *)fat_sector_buffer)[offset % 512] = (uint16_t)value;
        fat_write_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
    } else {
        uint32_t offset = cluster * 4;
        fat_read_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
        ((uint32_t *)fat_sector_buffer)[offset % 512] = value;
        fat_write_sector(fat, fat->fat_start + (offset / 512), fat_sector_buffer);
    }
}

static uint32_t fat_cluster_to_lba(struct fat_private *fat, uint32_t cluster) {
    return fat->data_start + ((cluster - 2) * fat->bytes_per_cluster);
}

static uint32_t fat_alloc_cluster(struct fat_private *fat) __attribute__((unused));
static uint32_t fat_alloc_cluster(struct fat_private *fat) {
    uint32_t cluster = fat->next_free;
    
    for (uint32_t i = 0; i < fat->sectors_total; ++i) {
        uint32_t entry = fat_read_fat_entry(fat, cluster);
        if (entry == 0) {
            fat->next_free = cluster;
            fat_write_fat_entry(fat, cluster, 0xffffffff);
            return cluster;
        }
        cluster = (cluster + 1) % fat->sectors_total;
    }
    
    return 0xffffffff;
}

static void fat_free_cluster_chain(struct fat_private *fat, uint32_t start_cluster) __attribute__((unused));
static void fat_free_cluster_chain(struct fat_private *fat, uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    
    while (cluster < 0xffffff0) {
        uint32_t next = fat_read_fat_entry(fat, cluster);
        fat_write_fat_entry(fat, cluster, 0);
        cluster = next;
    }
}

static int fat_read_directory(struct fat_private *fat, uint32_t cluster, struct vfs_node *dir_node) {
    if (dir_node == NULL) {
        return -1;
    }
    
    (void)dir_node;
    uint32_t count = 0;
    
    while (count < 16) {
        fat_read_sector(fat, fat_cluster_to_lba(fat, cluster) + (count / 16), fat_dir_buffer);
        struct fat_dir_entry *entry = &fat_dir_buffer[count % 16];
        
        if (entry->name[0] == 0) {
            break;
        }
        
        if (entry->name[0] != 0xe5) {
            bool is_dir = (entry->attributes & FAT_ATTR_SUBDIR) != 0;
            bool is_hidden = (entry->attributes & (FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME)) != 0;
            
            if (!is_hidden) {
                char name[12];
                memcpy(name, entry->name, 11);
                name[11] = '\0';
                
                struct vfs_node *node = vfs_create_node(name, is_dir ? VFS_TYPE_DIR : VFS_TYPE_FILE);
                if (node != NULL) {
                    node->size = entry->file_size;
                    node->private_data = (void *)(uintptr_t)(((uint32_t)entry->high_cluster << 16) | entry->low_cluster);
                    node->read = fat_read;
                    node->write = fat_write;
                    node->open = fat_open;
                    vfs_add_child(dir_node, node);
                }
            }
        }
        
        ++count;
    }
    
    return 0;
}

static int fat_open(struct vfs_node *node) {
    (void)node;
    return 0;
}

static int fat_read(struct vfs_node *node, void *buffer, size_t size, uint64_t offset) {
    struct fat_private *fat = node->filesystem;
    if (fat == NULL) {
        return -1;
    }
    
    uint32_t start_cluster = (uint32_t)((uint64_t)node->private_data & 0xffffffff);
    if (start_cluster == 0) {
        return -1;
    }
    
    uint64_t remaining = size;
    uint64_t off = offset;
    uint32_t cluster = start_cluster;
    uint8_t *buf = buffer;
    
    while (off >= fat->bytes_per_cluster) {
        off -= fat->bytes_per_cluster;
        cluster = fat_read_fat_entry(fat, cluster);
        if (cluster >= 0xffffff0) {
            return -1;
        }
    }
    
    while (remaining > 0) {
        uint64_t lba = fat_cluster_to_lba(fat, cluster);
        fat_read_sector(fat, lba + (off / 512), fat_sector_buffer);
        
        size_t sector_offset = off % 512;
        size_t to_read = 512 - sector_offset;
        if (to_read > remaining) {
            to_read = remaining;
        }
        
        memcpy(buf, &fat_sector_buffer[sector_offset], to_read);
        
        buf += to_read;
        remaining -= to_read;
        off = 0;
        
        cluster = fat_read_fat_entry(fat, cluster);
        if (cluster >= 0xffffff0) {
            break;
        }
    }
    
    return (int)(size - remaining);
}

static int fat_write(struct vfs_node *node, const void *buffer, size_t size, uint64_t offset) {
    struct fat_private *fat = node->filesystem;
    if (fat == NULL) {
        return -1;
    }
    
    (void)buffer;
    (void)offset;
    (void)size;
    
    return -1;
}

int fat_mount(struct vfs_filesystem *fs, struct vfs_node *mount_point, const char *data) {
    (void)fs;
    (void)data;
    
    struct ata_device *device = NULL;
    for (int i = 0; i < ata_device_count; ++i) {
        if (ata_devices[i].present) {
            device = &ata_devices[i];
            break;
        }
    }
    
    if (device == NULL) {
        return -1;
    }
    
    struct fat_bpb bpb;
    ata_read_sector(device, 0, &bpb);
    
    if (bpb.jump[0] != 0xeb && bpb.jump[0] != 0xe9) {
        return -1;
    }
    
    struct fat_private *fat = kmalloc(sizeof(struct fat_private));
    if (fat == NULL) {
        return -1;
    }
    
    memset(fat, 0, sizeof(struct fat_private));
    fat->device = device;
    fat->lba_start = 0;
    
    uint32_t bps = ((uint16_t)bpb.bytes_per_sector);
    uint8_t spc = bpb.sectors_per_cluster;
    uint32_t rsecs = bpb.reserved_sectors;
    uint8_t fatcnt = bpb.fat_count;
    uint32_t root_ent = bpb.root_entries;
    uint32_t sectors_fat = bpb.sectors_per_fat;
    
    if (bps == 0 || spc == 0) {
        kfree(fat);
        return -1;
    }
    
    fat->bytes_per_cluster = bps * spc;
    
    if (sectors_fat == 0) {
        struct fat_extended_bpb ebpb;
        ata_read_sector(device, rsecs, &ebpb);
        
        fat->fat_type = FAT32;
        fat->fat_size = ((struct fat_extended_bpb *)&ebpb)->sectors_per_fat32;
        fat->fat_start = rsecs;
        fat->root_cluster = ebpb.root_cluster;
        fat->sectors_total = (device->type > 0) ? 0 : 0x2000000;
    } else if (root_ent != 0) {
        uint32_t root_sectors = (root_ent * 32 + bps - 1) / bps;
        uint32_t data_secs = bpb.sectors_small - rsecs - (fatcnt * sectors_fat) - root_sectors;
        uint32_t cluster_count = data_secs / spc;
        
        if (cluster_count < 4085) {
            fat->fat_type = FAT12;
        } else {
            fat->fat_type = FAT16;
        }
        
        fat->fat_size = sectors_fat;
        fat->fat_start = rsecs;
        fat->root_cluster = 0;
        fat->data_start = rsecs + (fatcnt * sectors_fat) + root_sectors;
        fat->sectors_total = bpb.sectors_small;
    } else {
        fat->fat_type = FAT32;
        fat->sectors_total = bpb.sectors_large;
    }
    
    fat->fat_copies = fatcnt;
    mount_point->filesystem = fat;
    
    fat_read_directory(fat, fat->root_cluster, mount_point);
    
    return 0;
}

struct vfs_node *fat_alloc_node(void) {
    return vfs_create_node("", VFS_TYPE_FILE);
}

void fat_free_node(struct vfs_node *node) {
    if (node != NULL && node->private_data != NULL) {
        kfree(node->private_data);
    }
}

struct vfs_filesystem *fatfs_create(struct ata_device *device) {
    (void)device;
    static struct vfs_filesystem fat_fs;
    
    fat_fs.name[0] = '\0';
    fat_fs.mount = fat_mount;
    fat_fs.alloc_node = fat_alloc_node;
    fat_fs.free_node = fat_free_node;
    fat_fs.next = NULL;
    
    return &fat_fs;
}