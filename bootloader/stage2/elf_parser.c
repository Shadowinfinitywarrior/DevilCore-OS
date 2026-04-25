// bootloader/stage2/elf_parser.c
// ELF binary parser

#include <devil/types.h>

typedef struct {
    uint8_t  magic[4];
    uint8_t  class_;
    uint8_t  data;
    uint8_t  version;
    uint8_t  osabi;
    uint8_t  abiversion;
    uint8_t  padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version_;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} PACKED Elf64_Ehdr;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} PACKED Elf64_Phdr;

#define ELF_MAGIC 0x464C457F
#define ELF_CLASS64 2
#define ELF_DATA_LSB 1
#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_X86_64 0x3E

int elf_validate(Elf64_Ehdr *ehdr) {
    if (ehdr->magic[0] != 0x7F || 
        ehdr->magic[1] != 'E' ||
        ehdr->magic[2] != 'L' ||
        ehdr->magic[3] != 'F') {
        return -1;
    }
    if (ehdr->class_ != ELF_CLASS64) {
        return -1;
    }
    return 0;
}

int elf_load(Elf64_Ehdr *ehdr, void *base) {
    if (elf_validate(ehdr) < 0) {
        return -1;
    }
    
    Elf64_Phdr *phdr = (Elf64_Phdr*)((uint8_t*)ehdr + ehdr->phoff);
    
    for (int i = 0; i < ehdr->phnum; i++) {
        if (phdr[i].type == 1) { // PT_LOAD
            uint8_t *src = (uint8_t*)ehdr + phdr[i].offset;
            uint8_t *dest = (uint8_t*)phdr[i].vaddr;
            
            for (uint64_t j = 0; j < phdr[i].filesz; j++) {
                dest[j] = src[j];
            }
            
            // Zero .bss
            for (uint64_t j = phdr[i].filesz; j < phdr[i].memsz; j++) {
                dest[j] = 0;
            }
        }
    }
    
    return 0;
}