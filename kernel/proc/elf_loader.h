#ifndef DC_ELF_LOADER_H
#define DC_ELF_LOADER_H

#include "../fs/vfs.h"

/* Loads an ELF file from the VFS into the specified address space (PML4).
 * Returns its entry point or 0 on failure. */
uint64_t elf_load(vfs_node_t *file, uint64_t pml4);

#endif /* DC_ELF_LOADER_H */
