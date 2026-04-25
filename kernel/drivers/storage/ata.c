// kernel/drivers/storage/ata.c
// ATA/IDE driver

#include <devil/types.h>
#include <asm/io.h>

#define ATA_PRIMARY_IO    0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE  0x30
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_ST_BUSY   0x80
#define ATA_ST_DRDY   0x40
#define ATA_ST_DF    0x20
#define ATA_ST_ERR   0x01

typedef struct ata_device {
    uint16_t io_base;
    uint8_t  slave;
    uint8_t  present;
    uint8_t  lba48;
    uint64_t size;
    char    model[40];
} ata_device_t;

static ata_device_t ata_primary;
static ata_device_t ata_secondary;

static void ata_wait_ready(uint16_t io) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(io + 7);
        if (!(status & ATA_ST_BUSY)) return;
    }
}

static uint8_t ata_read_status(uint16_t io) {
    inb(io + 7);
    return inb(io + 7);
}

static int ata_identify(uint16_t io, ata_device_t *dev) {
    outb(io + 6, dev->slave ? 0xB0 : 0xA0);
    outb(io + 7, ATA_CMD_IDENTIFY);
    
    uint8_t status = ata_read_status(io);
    if (status == 0) return 0;
    
    for (int i = 0; i < 256; i++) {
        uint16_t val = inw(io);
    }
    
    return 1;
}

void ata_init(void) {
    ata_primary.io_base = ATA_PRIMARY_IO;
    ata_primary.slave = 0;
    ata_primary.present = ata_identify(ATA_PRIMARY_IO, &ata_primary);
    
    if (ata_primary.present) {
        printf("[ATA] Primary: %s\n", ata_primary.model);
    }
}