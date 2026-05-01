#include "slab.h"
#include "memory.h"
#include <string.h>
#include <stdbool.h>

extern void kprintf(const char *fmt, ...);

// Pre-defined caches
slab_cache_t *cache_8 = NULL;
slab_cache_t *cache_16 = NULL;
slab_cache_t *cache_32 = NULL;
slab_cache_t *cache_64 = NULL;
slab_cache_t *cache_128 = NULL;
slab_cache_t *cache_256 = NULL;
slab_cache_t *cache_512 = NULL;
slab_cache_t *cache_1024 = NULL;
slab_cache_t *cache_2048 = NULL;
slab_cache_t *cache_4096 = NULL;

// Slab cache list for tracking
static slab_cache_t *cache_list = NULL;

// Page size (4KB)
#define PAGE_SIZE 4096

// Calculate objects per slab
static uint32_t calc_objects_per_slab(uint32_t obj_size, uint32_t align) {
    // Account for slab header and bitmap
    uint32_t overhead = sizeof(slab_t) + ((PAGE_SIZE / obj_size) + 7) / 8;
    uint32_t available = PAGE_SIZE - overhead;
    return available / obj_size;
}

// Initialize a slab cache
static void slab_cache_init(slab_cache_t *cache, const char *name,
                            uint32_t size, uint32_t align,
                            void (*ctor)(void*),
                            void (*dtor)(void*)) {
    strncpy(cache->name, name, 31);
    cache->name[31] = '\0';
    cache->object_size = size;
    cache->align = (align > 0) ? align : 1;
    cache->objects_per_slab = calc_objects_per_slab(size, cache->align);
    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->full = NULL;
    cache->partial = NULL;
    cache->empty = NULL;
    cache->total_slabs = 0;
    cache->free_objects = 0;
    cache->total_objects = 0;
    cache->next = cache_list;
    cache_list = cache;
}

void slab_init(void) {
    kprintf("Initializing slab allocator...\n");
    
    // Create static cache structures
    static slab_cache_t caches[10];
    
    slab_cache_init(&caches[0], "size-8", 8, 8, NULL, NULL);
    cache_8 = &caches[0];
    
    slab_cache_init(&caches[1], "size-16", 16, 16, NULL, NULL);
    cache_16 = &caches[1];
    
    slab_cache_init(&caches[2], "size-32", 32, 32, NULL, NULL);
    cache_32 = &caches[2];
    
    slab_cache_init(&caches[3], "size-64", 64, 64, NULL, NULL);
    cache_64 = &caches[3];
    
    slab_cache_init(&caches[4], "size-128", 128, 128, NULL, NULL);
    cache_128 = &caches[4];
    
    slab_cache_init(&caches[5], "size-256", 256, 256, NULL, NULL);
    cache_256 = &caches[5];
    
    slab_cache_init(&caches[6], "size-512", 512, 512, NULL, NULL);
    cache_512 = &caches[6];
    
    slab_cache_init(&caches[7], "size-1024", 1024, 1024, NULL, NULL);
    cache_1024 = &caches[7];
    
    slab_cache_init(&caches[8], "size-2048", 2048, 2048, NULL, NULL);
    cache_2048 = &caches[8];
    
    slab_cache_init(&caches[9], "size-4096", 4096, 4096, NULL, NULL);
    cache_4096 = &caches[9];
    
    kprintf("Slab allocator initialized\n");
}

slab_cache_t *slab_cache_create(const char *name, 
                                 uint32_t size,
                                 uint32_t align,
                                 void (*ctor)(void*),
                                 void (*dtor)(void*)) {
    slab_cache_t *cache = kmalloc(sizeof(slab_cache_t));
    if (!cache) return NULL;
    
    slab_cache_init(cache, name, size, align, ctor, dtor);
    return cache;
}

// Allocate a new slab
static slab_t *alloc_slab(slab_cache_t *cache) {
    // Allocate page for slab
    void *page = pmm_alloc_page();
    if (!page) return NULL;
    
    void *virt = phys_to_virt((uint64_t)page);
    
    // Allocate slab structure separately
    slab_t *slab = kmalloc(sizeof(slab_t));
    if (!slab) {
        pmm_free_page(page);
        return NULL;
    }
    
    slab->cache = cache;
    slab->memory = virt;
    slab->num_pages = 1;
    
    // Allocate bitmap
    uint32_t bitmap_size = (cache->objects_per_slab + 7) / 8;
    slab->bitmap = kmalloc(bitmap_size);
    if (!slab->bitmap) {
        kfree(slab);
        pmm_free_page(page);
        return NULL;
    }
    
    // Clear bitmap (all objects free)
    memset(slab->bitmap, 0, bitmap_size);
    slab->free_count = cache->objects_per_slab;
    slab->first_free = 0;
    
    cache->total_slabs++;
    cache->free_objects += cache->objects_per_slab;
    cache->total_objects += cache->objects_per_slab;
    
    return slab;
}

// Free a slab
static void free_slab(slab_t *slab) {
    if (!slab) return;
    
    slab_cache_t *cache = slab->cache;
    
    // Call destructors for any used objects
    for (uint32_t i = 0; i < cache->objects_per_slab; i++) {
        if (slab->bitmap[i / 8] & (1 << (i % 8))) {
            void *obj = (char*)slab->memory + i * cache->object_size;
            if (cache->dtor) {
                cache->dtor(obj);
            }
        }
    }
    
    // Free memory
    if (slab->bitmap) kfree(slab->bitmap);
    
    // Free physical page
    uint64_t phys = virt_to_phys(slab->memory);
    pmm_free_page((void*)phys);
    
    cache->total_slabs--;
    cache->free_objects -= slab->free_count;
    cache->total_objects -= cache->objects_per_slab;
    
    kfree(slab);
}

// Find first free object in bitmap
static uint32_t find_first_free(uint8_t *bitmap, uint32_t num_objects) {
    for (uint32_t i = 0; i < num_objects; i++) {
        if (!(bitmap[i / 8] & (1 << (i % 8)))) {
            return i;
        }
    }
    return num_objects;  // None free
}

void *slab_alloc(slab_cache_t *cache) {
    if (!cache) return NULL;
    
    // Try to find a partial slab
    slab_t *slab = cache->partial;
    
    // If no partial slabs, try an empty slab
    if (!slab) {
        slab = cache->empty;
        if (slab) {
            // Move from empty to partial list
            cache->empty = slab->next;
            if (slab->next) slab->next->prev = NULL;
            
            slab->next = cache->partial;
            slab->prev = NULL;
            if (cache->partial) cache->partial->prev = slab;
            cache->partial = slab;
        }
    }
    
    // If still no slab, allocate a new one
    if (!slab) {
        slab = alloc_slab(cache);
        if (!slab) return NULL;
        
        // Add to partial list
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) cache->partial->prev = slab;
        cache->partial = slab;
    }
    
    // Allocate object
    uint32_t idx = find_first_free(slab->bitmap, cache->objects_per_slab);
    if (idx >= cache->objects_per_slab) {
        return NULL;  // Should not happen if free_count is correct
    }
    
    // Mark as used
    slab->bitmap[idx / 8] |= (1 << (idx % 8));
    slab->free_count--;
    cache->free_objects--;
    
    // Update first_free for next allocation
    slab->first_free = find_first_free(slab->bitmap, cache->objects_per_slab);
    
    // If slab is now full, move to full list
    if (slab->free_count == 0) {
        // Remove from partial
        if (slab->prev) slab->prev->next = slab->next;
        else cache->partial = slab->next;
        if (slab->next) slab->next->prev = slab->prev;
        
        // Add to full
        slab->next = cache->full;
        slab->prev = NULL;
        if (cache->full) cache->full->prev = slab;
        cache->full = slab;
    }
    
    // Get object pointer
    void *obj = (char*)slab->memory + idx * cache->object_size;
    
    // Call constructor
    if (cache->ctor) {
        cache->ctor(obj);
    }
    
    return obj;
}

void slab_free(slab_cache_t *cache, void *obj) {
    if (!cache || !obj) return;
    
    // Find which slab this object belongs to
    // We need to search through all slabs (can be optimized)
    slab_t *slab = NULL;
    
    // Check partial slabs
    for (slab_t *s = cache->partial; s; s = s->next) {
        if (obj >= s->memory && 
            obj < (char*)s->memory + cache->objects_per_slab * cache->object_size) {
            slab = s;
            break;
        }
    }
    
    // Check full slabs
    if (!slab) {
        for (slab_t *s = cache->full; s; s = s->next) {
            if (obj >= s->memory && 
                obj < (char*)s->memory + cache->objects_per_slab * cache->object_size) {
                slab = s;
                break;
            }
        }
    }
    
    if (!slab) {
        kprintf("slab_free: object not found in any slab\n");
        return;
    }
    
    // Calculate index
    uint32_t idx = ((char*)obj - (char*)slab->memory) / cache->object_size;
    
    // Call destructor
    if (cache->dtor) {
        cache->dtor(obj);
    }
    
    // Mark as free
    slab->bitmap[idx / 8] &= ~(1 << (idx % 8));
    slab->free_count++;
    cache->free_objects++;
    
    // Update first_free
    if (idx < slab->first_free) {
        slab->first_free = idx;
    }
    
    // If was full, move to partial
    uint8_t was_full = (slab->free_count == 1);
    if (was_full) {
        // Remove from full
        if (slab->prev) slab->prev->next = slab->next;
        else cache->full = slab->next;
        if (slab->next) slab->next->prev = slab->prev;
        
        // Add to partial
        slab->next = cache->partial;
        slab->prev = NULL;
        if (cache->partial) cache->partial->prev = slab;
        cache->partial = slab;
    }
    
    // If now empty, move to empty list
    if (slab->free_count == cache->objects_per_slab) {
        // Remove from partial
        if (slab->prev) slab->prev->next = slab->next;
        else cache->partial = slab->next;
        if (slab->next) slab->next->prev = slab->prev;
        
        // Add to empty
        slab->next = cache->empty;
        slab->prev = NULL;
        if (cache->empty) cache->empty->prev = slab;
        cache->empty = slab;
        
        // TODO: Consider freeing empty slabs after a threshold
    }
}

void slab_cache_destroy(slab_cache_t *cache) {
    if (!cache) return;
    
    // Free all slabs
    slab_t *slab;
    
    while ((slab = cache->full) != NULL) {
        cache->full = slab->next;
        free_slab(slab);
    }
    
    while ((slab = cache->partial) != NULL) {
        cache->partial = slab->next;
        free_slab(slab);
    }
    
    while ((slab = cache->empty) != NULL) {
        cache->empty = slab->next;
        free_slab(slab);
    }
    
    // Remove from cache list
    if (cache_list == cache) {
        cache_list = cache->next;
    }
}

uint32_t slab_cache_free_count(slab_cache_t *cache) {
    if (!cache) return 0;
    return cache->free_objects;
}

uint32_t slab_cache_used_count(slab_cache_t *cache) {
    if (!cache) return 0;
    return cache->total_objects - cache->free_objects;
}

void slab_print_stats(void) {
    kprintf("=== Slab Allocator Statistics ===\n");
    
    for (slab_cache_t *cache = cache_list; cache; cache = cache->next) {
        uint32_t used = slab_cache_used_count(cache);
        uint32_t total = cache->total_objects;
        uint32_t slabs = cache->total_slabs;
        
        kprintf("%-12s: %u/%u used (%u%%) in %u slabs\n",
                cache->name, used, total, 
                total > 0 ? (used * 100) / total : 0, slabs);
    }
}

// Size-optimized general purpose allocator
void *kmalloc_slab(size_t size) {
    slab_cache_t *cache = NULL;
    size_t total_size = size + sizeof(slab_cache_t *);
    
    // Find appropriate cache
    if (total_size <= 8) cache = cache_8;
    else if (total_size <= 16) cache = cache_16;
    else if (total_size <= 32) cache = cache_32;
    else if (total_size <= 64) cache = cache_64;
    else if (total_size <= 128) cache = cache_128;
    else if (total_size <= 256) cache = cache_256;
    else if (total_size <= 512) cache = cache_512;
    else if (total_size <= 1024) cache = cache_1024;
    else if (total_size <= 2048) cache = cache_2048;
    else if (total_size <= 4096) cache = cache_4096;
    
    if (cache) {
        void *ptr = slab_alloc(cache);
        if (ptr) {
            *(slab_cache_t **)ptr = cache;
            return (void *)((char *)ptr + sizeof(slab_cache_t *));
        }
    }
    
    // Too large for slabs, use regular kmalloc
    return kmalloc(size);
}

void *kzalloc_slab(size_t size) {
    void *ptr = kmalloc_slab(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree_slab(void *ptr) {
    if (!ptr) return;
    
    // Check if it might be a slab allocation by looking for a valid cache pointer
    slab_cache_t **cache_ptr = (slab_cache_t **)((char *)ptr - sizeof(slab_cache_t *));
    slab_cache_t *cache = *cache_ptr;
    
    // Verify it's a valid cache by checking against cache_list
    bool valid = false;
    for (slab_cache_t *c = cache_list; c; c = c->next) {
        if (c == cache) {
            valid = true;
            break;
        }
    }
    
    if (valid) {
        slab_free(cache, (void *)cache_ptr);
    } else {
        kfree(ptr);
    }
}

void *krealloc_slab(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc_slab(new_size);
    if (new_size == 0) {
        kfree_slab(ptr);
        return NULL;
    }
    
    slab_cache_t **cache_ptr = (slab_cache_t **)((char *)ptr - sizeof(slab_cache_t *));
    slab_cache_t *cache = *cache_ptr;
    
    // Verify it's a valid cache
    bool valid = false;
    for (slab_cache_t *c = cache_list; c; c = c->next) {
        if (c == cache) {
            valid = true;
            break;
        }
    }
    
    size_t old_size = 0;
    if (valid) {
        old_size = cache->object_size - sizeof(slab_cache_t *);
    } else {
        // We don't know the size of regular kmalloc blocks
        old_size = new_size; // Best effort
    }
    
    if (valid && new_size <= old_size) {
        return ptr; // Already large enough
    }
    
    void *new_ptr = kmalloc_slab(new_size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    kfree_slab(ptr);
    
    return new_ptr;
}
