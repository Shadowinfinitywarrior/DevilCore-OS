/*
 * DevilCore OS — ATA / IDE Storage Driver
 */
#ifndef DC_ATA_H
#define DC_ATA_H

#include "../include/types.h"

/* Primary ATA Bus I/O ports */
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRV_HEAD     0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_CMD          0x1F7
#define ATA_PRIMARY_ALT_STATUS   0x3F6

/* Initialize ATA subsystem */
void ata_init(void);

/* Check if Primary Master drive is present */
bool ata_is_ready(void);

/* Read consecutive 512-byte sectors from the hard drive (LBA28 PIO mode) */
int ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer);

/* Write consecutive 512-byte sectors to the hard drive (LBA28 PIO mode) */
int ata_write_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer);

#endif /* DC_ATA_H */
