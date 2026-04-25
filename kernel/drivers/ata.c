/*
 * DevilCore OS — ATA / IDE PIO Storage Driver
 */
#include "ata.h"
#include "../include/ports.h"

static bool g_ata_primary_master_ready = false;

static void ata_wait_bsy(void) {
    while (inb(ATA_PRIMARY_STATUS) & 0x80);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_PRIMARY_STATUS) & 0x08));
}

void ata_init(void) {
    /* Select master drive */
    outb(ATA_PRIMARY_DRV_HEAD, 0xA0);
    
    /* Flush out old status */
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    inb(ATA_PRIMARY_STATUS);
    
    /* Send IDENTIFY command */
    outb(ATA_PRIMARY_SECCOUNT, 0);
    outb(ATA_PRIMARY_LBA_LO, 0);
    outb(ATA_PRIMARY_LBA_MID, 0);
    outb(ATA_PRIMARY_LBA_HI, 0);
    outb(ATA_PRIMARY_CMD, 0xEC);

    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) {
        g_ata_primary_master_ready = false;
        return; /* No drive */
    }

    /* Wait for BSY to clear */
    while ((inb(ATA_PRIMARY_STATUS) & 0x80));
    
    /* Check LBAmid and LBAhi for ATAPI (CD-ROM) signature */
    uint8_t lba_mid = inb(ATA_PRIMARY_LBA_MID);
    uint8_t lba_hi = inb(ATA_PRIMARY_LBA_HI);
    if (lba_mid != 0 && lba_hi != 0) {
        /* Not an ATA hard disk (possibly ATAPI) */
        g_ata_primary_master_ready = false;
        return;
    }

    /* Wait for DRQ or ERR */
    while (1) {
        status = inb(ATA_PRIMARY_STATUS);
        if (status & 0x01) { /* ERR */
            g_ata_primary_master_ready = false;
            return;
        }
        if (status & 0x08) break; /* DRQ */
    }

    /* Read the IDENTIFY data (256 16-bit values) */
    for (int i = 0; i < 256; i++) {
        uint16_t port = ATA_PRIMARY_DATA;
        uint16_t val;
        __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
        /* We can parse IDENTIFY string data later, for now we just flush the 512 bytes */
    }

    g_ata_primary_master_ready = true;
}

bool ata_is_ready(void) {
    return g_ata_primary_master_ready;
}

int ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!g_ata_primary_master_ready) return -1;

    ata_wait_bsy();
    
    outb(ATA_PRIMARY_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); /* Master, LBA mode, LBA[24:27] */
    outb(ATA_PRIMARY_SECCOUNT, sector_count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t) lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_CMD, 0x20); /* Command: READ SECTORS */

    for (int i = 0; i < sector_count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        
        uint16_t *buf16 = (uint16_t *)(buffer + (i * 512));
        for (int j = 0; j < 256; j++) {
            uint16_t port = ATA_PRIMARY_DATA;
            uint16_t val;
            __asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
            buf16[j] = val;
        }
    }
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!g_ata_primary_master_ready) return -1;
    
    ata_wait_bsy();
    
    outb(ATA_PRIMARY_DRV_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, sector_count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t) lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_CMD, 0x30); /* Command: WRITE SECTORS */

    for (int i = 0; i < sector_count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        
        uint16_t *buf16 = (uint16_t *)(buffer + (i * 512));
        for (int j = 0; j < 256; j++) {
            uint16_t port = ATA_PRIMARY_DATA;
            __asm__ volatile("outw %0, %1" : : "a"(buf16[j]), "Nd"(port));
        }
    }
    
    /* Flush Cache Command */
    outb(ATA_PRIMARY_CMD, 0xE7);
    ata_wait_bsy();
    
    return 0;
}
