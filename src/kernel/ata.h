#ifndef DEVILCORE_ATA_H
#define DEVILCORE_ATA_H

#include <stdint.h>

#define ATA_PORT_PRIMARY     0x1f0
#define ATA_PORT_SECONDARY 0x170
#define ATA_PORT_DCR       0x3f6
#define ATA_PORT_SCR       0x3f0

#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO  0x30
#define ATA_CMD_IDENTIFY   0xec

#define ATA_STATUS_ERR  0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_SRV 0x10
#define ATA_STATUS_DF  0x20
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_BSY  0x80

#define ATA_ERROR_ABRT 0x02

struct ata_device {
    uint8_t channel;
    uint8_t slave;
    uint16_t type;
    uint32_t base;
    uint32_t ctrl;
    uint32_t sc157;
    uint8_t present;
};

struct ata_identify {
    uint16_t config;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors_track;
    uint16_t total_sectors_low;
    uint16_t total_sectors_high;
    uint8_t  serial[20];
    uint8_t  firmware[8];
    uint8_t  model[40];
    uint8_t  caps;
    uint8_t  eide_caps;
    uint8_t  fields_valid;
    uint8_t  piecelists_caps;
    uint16_t major_cmd_sets;
    uint16_t minor_cmd_sets;
    uint32_t cmdsets_enabled;
    uint32_t cmdsets_enabled2;
    uint16_t udma_modes;
    uint8_t  sermin_u;
    uint8_t  sectors_48bit[8];
    uint32_t lba48_sectors;
    uint16_t smrdt_word;
    uint8_t  _objs[390];
} __attribute__((packed));

int ata_init(void);

int ata_read_sector(struct ata_device *dev, uint64_t lba, void *buffer);
int ata_write_sector(struct ata_device *dev, uint64_t lba, const void *buffer);

int ata_identify_device(struct ata_device *dev, struct ata_identify *info);

extern struct ata_device ata_devices[4];
extern int ata_device_count;

#endif