// kernel/fs/ext2/ext2.c

#include <devil/types.h>
#include <fs/vfs.h>
#include <drivers/storage/ata.h>

typedef struct ext2_superblock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_magic;
    uint16_t s_state;
    uint16_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];
    uint8_t s_pad[460];
} PACKED ext2_superblock_t;

#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_ROOT_INO 2

static void *ext2_mount(const char *device, const char *mount_point) {
    return NULL;
}

static int ext2_statfs(const char *path, struct statfs *st) {
    return 0;
}

static int ext2_lookup(const char *path) {
    if (!strcmp(path, "/") || !strcmp(path, "")) {
        return EXT2_ROOT_INO;
    }
    return 0;
}

static int ext2_open(const char *path, int flags) {
    return ext2_lookup(path);
}

static ssize_t ext2_read_inode(ino_t ino, void *buf, size_t count, off_t offset) {
    return 0;
}

static int ext2_write_inode(ino_t ino, const void *buf, size_t count, off_t offset) {
    return 0;
}

file_system_t ext2_fs = {
    .name = "ext2",
    .mount = ext2_mount,
};