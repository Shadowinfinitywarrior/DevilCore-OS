#ifndef DEVILCORE_SLAB_H
#define DEVILCORE_SLAB_H

#include <stddef.h>
#include <stdint.h>

// Slab cache for fixed-size objects
typedef struct slab_cache {
    char name[32];
    uint32_t object_size;
    uint32_t objects_per_slab;
    uint32_t align;
    
    // Constructors/destructors
    void (*ctor)(void *obj);
    void (*dtor)(void *obj);
    
    // Slab lists
    struct slab *full;      // All objects used
    struct slab *partial;   // Some objects free
    struct slab *empty;     // All objects free
    
    uint32_t total_slabs;
    uint32_t free_objects;
    uint32_t total_objects;
    
    struct slab_cache *next;
} slab_cache_t;

typedef struct slab {
    slab_cache_t *cache;
    void *memory;           // Actual memory page(s)
    uint32_t num_pages;
    
    // Free object bitmap
    uint8_t *bitmap;
    uint32_t free_count;
    uint32_t first_free;
    
    struct slab *next, *prev;
} slab_t;

// Pre-defined caches for common sizes
extern slab_cache_t *cache_8;
extern slab_cache_t *cache_16;
extern slab_cache_t *cache_32;
extern slab_cache_t *cache_64;
extern slab_cache_t *cache_128;
extern slab_cache_t *cache_256;
extern slab_cache_t *cache_512;
extern slab_cache_t *cache_1024;
extern slab_cache_t *cache_2048;
extern slab_cache_t *cache_4096;

// Slab API
void slab_init(void);
slab_cache_t *slab_cache_create(const char *name, 
                                 uint32_t size,
                                 uint32_t align,
                                 void (*ctor)(void*),
                                 void (*dtor)(void*));
void *slab_alloc(slab_cache_t *cache);
void slab_free(slab_cache_t *cache, void *obj);
void slab_cache_destroy(slab_cache_t *cache);

// Statistics
uint32_t slab_cache_free_count(slab_cache_t *cache);
uint32_t slab_cache_used_count(slab_cache_t *cache);
void slab_print_stats(void);

// Size-optimized general purpose allocator
void *kmalloc_slab(size_t size);
void *kzalloc_slab(size_t size);    // Zero-initialized
void kfree_slab(void *ptr);
void *krealloc_slab(void *ptr, size_t new_size);

#endif // DEVILCORE_SLAB_H
