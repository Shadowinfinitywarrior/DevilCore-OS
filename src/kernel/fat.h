#ifndef DEVILCORE_FAT_H
#define DEVILCORE_FAT_H

#include <stdint.h>

#include "vfs.h"

#define FAT_MAGIC       0xeb
#define FAT12         12
#define FAT16         16
#define FAT32         32

#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN   0x02
#define FAT_ATTR_SYSTEM   0x04
#define FAT_ATTR_VOLUME  0x08
#define FAT_ATTR_SUBDIR  0x10
#define FAT_ATTR_ARCHIVE 0x20

struct fat_bpb {
    uint8_t  jump[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t sectors_small;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t sectors_large;
    uint8_t  bios_drive;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  filesystem[8];
} __attribute__((packed));

struct fat_extended_bpb {
    uint32_t sectors_per_fat32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  filesystem[8];
} __attribute__((packed));

struct fat_dir_entry {
    uint8_t  name[11];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  create_time_ms;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t high_cluster;
    uint16_t modify_time;
    uint16_t modify_date;
    uint16_t low_cluster;
    uint32_t file_size;
} __attribute__((packed));

struct fat_private {
    struct ata_device *device;
    uint64_t lba_start;
    uint32_t sectors_total;
    uint32_t bytes_per_cluster;
    uint32_t fat_start;
    uint32_t fat_size;
    uint32_t data_start;
    uint32_t root_cluster;
    uint32_t free_clusters;
    uint32_t next_free;
    uint8_t  fat_type;
    uint32_t fat_copies;
    
    void *fat_buffer;
    uint64_t fat_lba;
};

int fat_mount(struct vfs_filesystem *fs, struct vfs_node *mount_point, const char *data);
struct vfs_node *fat_alloc_node(void);
void fat_free_node(struct vfs_node *node);

struct vfs_filesystem *fatfs_create(struct ata_device *device);

#endif