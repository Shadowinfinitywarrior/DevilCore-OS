// kernel/fs/fat32/fat32.c

#include <devil/types.h>
#include <fs/vfs.h>

typedef struct {
    uint8_t  jump[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_copies;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} PACKED fat32_bpb_t;

static void *fat32_mount(const char *device, const char *mount_point) {
    return NULL;
}

static int fat32_open(const char *path, int flags) {
    return 0;
}

static ssize_t fat32_read(void *file, void *buf, size_t count) {
    return 0;
}

static ssize_t fat32_write(void *file, const void *buf, size_t count) {
    return 0;
}

file_system_t fat32_fs = {
    .name = "fat32",
    .mount = fat32_mount,
};