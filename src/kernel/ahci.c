#include "ahci.h"
#include "memory.h"
#include <string.h>

extern void serial_write_string(const char *text);
extern void serial_write_hex_u64(uint64_t value);

static HBA_MEM *hba_mem = NULL;

static int check_type(HBA_PORT *port) {
    uint32_t ssts = port->ssts;
    uint8_t ipm = (ssts >> 8) & 0x0f;
    uint8_t det = ssts & 0x0f;

    if (det != HBA_PORT_DET_PRESENT) return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NULL;

    switch (port->sig) {
        case SATA_SIG_ATAPI: return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:  return AHCI_DEV_SEMB;
        case SATA_SIG_PM:    return AHCI_DEV_PM;
        default:             return AHCI_DEV_SATA;
    }
}

void ahci_init(struct pci_device *pci) {
    serial_write_string("AHCI: Initializing...\n");
    
    // AHCI base address is BAR5
    uint32_t bar5 = pci_read(pci->bus, pci->device, pci->function, 0x24);
    hba_mem = (HBA_MEM *)phys_to_virt(bar5);

    serial_write_string("AHCI: BAR5 at ");
    serial_write_hex_u64(bar5);
    serial_write_string("\n");

    uint32_t pi = hba_mem->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            int dt = check_type(&hba_mem->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                serial_write_string("AHCI: SATA drive found on port ");
                // Convert i to string or just use serial_write_hex_u64 for simplicity
                serial_write_hex_u64(i);
                serial_write_string("\n");
            }
        }
    }
}

// Basic AHCI read implementation would go here. 
// For now, we just implement the initialization to verify detection.
bool ahci_read(int port_no, uint64_t startl, uint64_t starth, uint32_t count, uint16_t *buf) {
    (void)port_no; (void)startl; (void)starth; (void)count; (void)buf;
    return false;
}
