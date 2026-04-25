// bootloader/stage2/memory_map.c
// Memory map handling

#include <devil/types.h>

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} PACKED MemoryMapEntry;

typedef struct {
    uint64_t count;
    MemoryMapEntry entries[32];
} MemoryMap;

static MemoryMap memory_map;

int get_memory_map(MemoryMap *map) {
    map->count = 0;
    
    // BIOS INT 15h E820
    uint64_t next = 0x100000;
    uint32_t ebx = 0;
    
    for (int i = 0; i < 32 && ebx != 0; i++) {
        MemoryMapEntry *entry = &map->entries[i];
        
        __asm__ volatile(
            "mov eax, 0xE820\n"
            "mov ecx, 24\n"
            "mov edx, 0x534D4150\n"
            "int 0x15\n"
            : "=a"(entry->type), "=c"(ebx)
            : "a"(0xE820), "b"(ebx), "c"(24), "d"(0x534D4150), "D"(entry)
        );
        
        if (entry->type == 1 || entry->type == 2 || entry->type == 3) {
            map->entries[map->count++] = *entry;
        }
        
        if (ebx == 0) break;
    }
    
    return map->count;
}

uint64_t get_memory_size(void) {
    get_memory_map(&memory_map);
    
    uint64_t total = 0;
    for (uint64_t i = 0; i < memory_map.count; i++) {
        if (memory_map.entries[i].type == 1) { // Usable memory
            total += memory_map.entries[i].length;
        }
    }
    
    return total;
}