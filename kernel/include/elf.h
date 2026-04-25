/*
 * DevilCore OS — ELF64 Binary Format Definitions
 * Based on the System V ABI for x86_64.
 */
#ifndef DC_ELF_H
#define DC_ELF_H

#include "types.h"

#define ELF_MAGIC 0x464C457F /* 0x7F 'E' 'L' 'F' */

/* ELF Header (64-bit) */
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;      /* 1=Relocatable, 2=Executable, 3=Shared, 4=Core */
    uint16_t e_machine;   /* 0x3E for x86_64 */
    uint32_t e_version;
    uint64_t e_entry;     /* Entry point address */
    uint64_t e_phoff;     /* Program header table offset */
    uint64_t e_shoff;     /* Section header table offset */
    uint32_t e_flags;
    uint16_t e_ehsize;    /* Size of this header */
    uint16_t e_phentsize; /* Size of one program header table entry */
    uint16_t e_phnum;     /* Number of program header table entries */
    uint16_t e_shentsize; /* Size of one section header table entry */
    uint16_t e_shnum;     /* Number of section header table entries */
    uint16_t e_shstrndx;  /* Section header table index of the section name string table */
} elf64_header_t;

/* Program Header (64-bit) */
typedef struct {
    uint32_t p_type;   /* 1=LOAD, 2=DYNAMIC, 3=INTERP, 4=NOTE, etc. */
    uint32_t p_flags;  /* 1=X, 2=W, 4=R */
    uint64_t p_offset; /* Offset in file */
    uint64_t p_vaddr;  /* Virtual address in memory */
    uint64_t p_paddr;  /* Reserved */
    uint64_t p_filesz; /* Size in file */
    uint64_t p_memsz;  /* Size in memory */
    uint64_t p_align;  /* Alignment */
} elf64_phdr_t;

#define PT_LOAD    1
#define PF_X       0x1
#define PF_W       0x2
#define PF_R       0x4

#endif /* DC_ELF_H */
