/*
 * DevilCore OS — Kernel Heap Allocator Header
 */
#ifndef DC_HEAP_H
#define DC_HEAP_H

#include "../include/types.h"

/* Initialize the kernel heap */
void heap_init(void);

/* Allocate a block of memory of at least 'size' bytes */
void *kmalloc(size_t size);

/* Allocate a block of memory aligned to a specific boundary */
void *kmalloc_aligned(size_t size, size_t alignment);

/* Free a block of memory previously allocated by kmalloc */
void kfree(void *ptr);

#endif /* DC_HEAP_H */