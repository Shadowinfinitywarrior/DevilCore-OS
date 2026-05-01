#include "ata.h"

#include "pci.h"
#include "memory.h"
#include "io.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct ata_device ata_devices[4];
int ata_device_count;

#define ata_inb(p) inb(p)
#define ata_outb(p, v) outb(p, v)

static inline void ata_insl(uint16_t port, void *buffer, uint32_t count) {
    __asm__ volatile ("rep insl" : 
        "=D"(buffer), "=c"(count) : 
        "d"(port), "0"(buffer), "1"(count));
}

static inline void ata_outsl(uint16_t port, const void *buffer, uint32_t count) {
    __asm__ volatile ("rep outsl" : 
        "=S"(buffer), "=c"(count) : 
        "d"(port), "0"(buffer), "1"(count));
}

static void ata_wait_busy(struct ata_device *dev) {
    for (int i = 0; i < 100000; ++i) {
        uint8_t status = ata_inb(dev->base + 7);
        if ((status & ATA_STATUS_BSY) == 0) {
            return;
        }
    }
}

__attribute__((unused)) static void ata_wait_ready(struct ata_device *dev) {
    for (int i = 0; i < 100000; ++i) {
        uint8_t status = ata_inb(dev->base + 7);
        if ((status & ATA_STATUS_BSY) == 0 && (status & ATA_STATUS_DRDY) != 0) {
            return;
        }
    }
}

int ata_identify_device(struct ata_device *dev, struct ata_identify *info) {
    if (dev == NULL || !dev->present || info == NULL) {
        return -1;
    }

    ata_outb(dev->base + 6, 0xe0 | ((dev->slave & 1) << 4));
    ata_outb(dev->base + 7, ATA_CMD_IDENTIFY);
    
    uint8_t status = ata_inb(dev->base + 7);
    if (status == 0) {
        return -1;
    }

    ata_wait_busy(dev);
    
    for (int i = 0; i < 10000; ++i) {
        status = ata_inb(dev->base + 7);
        if ((status & ATA_STATUS_ERR)) {
            return -1;
        }
        if ((status & ATA_STATUS_DRQ)) {
            break;
        }
    }

    if (!(status & ATA_STATUS_DRQ)) {
        return -1;
    }

    ata_insl(dev->base, info, 256);
    
    ata_inb(dev->base + 7);
    ata_inb(dev->base + 7);

    return 0;
}

int ata_read_sector(struct ata_device *dev, uint64_t lba, void *buffer) {
    if (dev == NULL || !dev->present || buffer == NULL) {
        return -1;
    }

    ata_wait_busy(dev);
    
    ata_outb(dev->base + 6, 0xe0 | ((dev->slave & 1) << 4));
    ata_outb(dev->base + 2, 0);
    ata_outb(dev->base + 3, (uint8_t)(lba & 0xff));
    ata_outb(dev->base + 4, (uint8_t)((lba >> 8) & 0xff));
    ata_outb(dev->base + 5, (uint8_t)((lba >> 16) & 0xff));
    ata_outb(dev->base + 7, ATA_CMD_READ_PIO);

    ata_wait_busy(dev);

    uint8_t status = ata_inb(dev->base + 7);
    if (status & ATA_STATUS_ERR) {
        return -1;
    }

    for (int i = 0; i < 1000; ++i) {
        status = ata_inb(dev->base + 7);
        if ((status & ATA_STATUS_DRQ)) {
            break;
        }
    }

    ata_insl(dev->base, buffer, 256);

    return 0;
}

int ata_write_sector(struct ata_device *dev, uint64_t lba, const void *buffer) {
    if (dev == NULL || !dev->present || buffer == NULL) {
        return -1;
    }

    ata_wait_busy(dev);

    ata_outb(dev->base + 6, 0xe0 | ((dev->slave & 1) << 4));
    ata_outb(dev->base + 2, 0);
    ata_outb(dev->base + 3, (uint8_t)(lba & 0xff));
    ata_outb(dev->base + 4, (uint8_t)((lba >> 8) & 0xff));
    ata_outb(dev->base + 5, (uint8_t)((lba >> 16) & 0xff));
    ata_outb(dev->base + 7, ATA_CMD_WRITE_PIO);

    ata_wait_busy(dev);

    ata_outsl(dev->base, buffer, 256);

    ata_outb(dev->base + 7, 0xe7);

    ata_wait_busy(dev);

    return 0;
}

static void ata_probe_channel(uint8_t channel, uint16_t base, uint16_t ctrl) {
    struct ata_device *dev = &ata_devices[ata_device_count];
    
    dev->channel = channel;
    dev->base = base;
    dev->ctrl = ctrl;
    dev->present = 0;
    dev->slave = 0;

    ata_outb(base + 6, 0xa0);
    ata_outb(ctrl, 0);
    
    uint8_t status = ata_inb(base + 7);
    if (status == 0 || status == 0xff) {
        return;
    }

    ata_outb(base + 6, 0xb0);
    
    for (int i = 0; i < 1000; ++i) {
        status = ata_inb(base + 7);
    }

    for (uint8_t slave = 0; slave <= 1; ++slave) {
        dev = &ata_devices[ata_device_count];
        
        dev->channel = channel;
        dev->base = base;
        dev->ctrl = ctrl;
        dev->slave = slave;
        dev->present = 0;

        ata_outb(base + 6, 0xa0 | (slave << 4));
        ata_outb(base + 7, 0);
        
        ata_outb(base + 7, ATA_CMD_IDENTIFY);
        
        uint8_t status = ata_inb(base + 7);
        if (status == 0) {
            continue;
        }

        for (int i = 0; i < 100000; ++i) {
            status = ata_inb(base + 7);
            if (!(status & ATA_STATUS_BSY)) {
                break;
            }
        }

        if (status & ATA_STATUS_ERR) {
            continue;
        }

        dev->present = 1;
        ++ata_device_count;
    }
}

int ata_init(void) {
    ata_device_count = 0;

    ata_probe_channel(0, ATA_PORT_PRIMARY, ATA_PORT_DCR);
    ata_probe_channel(1, ATA_PORT_SECONDARY, ATA_PORT_DCR);

    return ata_device_count;
}