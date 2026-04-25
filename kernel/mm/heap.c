/*
 * DevilCore OS — Kernel Heap Allocator
 * A simple linked-list based memory allocator.
 * Requests memory from the PMM when the heap runs out of space.
 */
#include "heap.h"
#include "pmm.h"
#include "../libc/string.h"

#define HEAP_MAGIC 0xDEADC0DE
#define HEAP_MIN_SIZE (PAGE_SIZE * 4)

typedef struct heap_block {
    uint32_t magic;
    size_t size; /* Size of the data (excluding this header) */
    bool is_free;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *heap_head = NULL;

void heap_init(void) {
    /* Allocate initial pages for the heap */
    uint64_t phys1 = pmm_alloc_frame();
    (void)pmm_alloc_frame();
    (void)pmm_alloc_frame();
    (void)pmm_alloc_frame();
    
    if (!phys1) return; /* Panic */

    /* For identity-mapped kernel, virtual address == physical */
    heap_head = (heap_block_t *)phys1;
    heap_head->magic = HEAP_MAGIC;
    heap_head->size = (PAGE_SIZE * 4) - sizeof(heap_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
}

static void split_block(heap_block_t *block, size_t size) {
    if (block->size <= size + sizeof(heap_block_t) + 32) {
        return; /* Not worth splitting */
    }

    heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = block->size - size - sizeof(heap_block_t);
    new_block->is_free = true;
    new_block->next = block->next;

    block->size = size;
    block->next = new_block;
}

static void merge_blocks(void) {
    heap_block_t *curr = heap_head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(heap_block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

static heap_block_t *expand_heap(size_t size) {
    size_t required = size + sizeof(heap_block_t);
    size_t frames = (required + PAGE_SIZE - 1) / PAGE_SIZE;
    
    uint64_t first_frame = pmm_alloc_blocks(frames);
    if (!first_frame) return NULL;

    heap_block_t *new_block = (heap_block_t *)first_frame;
    new_block->magic = HEAP_MAGIC;
    new_block->size = (frames * PAGE_SIZE) - sizeof(heap_block_t);
    new_block->is_free = true;
    new_block->next = heap_head;
    heap_head = new_block;
    
    return heap_head;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    if (!heap_head) heap_init();

    size = ALIGN_UP(size, 16); /* Align to 16 bytes for speed */
    heap_block_t *curr = heap_head;
    
    /* Search for free block */
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            split_block(curr, size);
            curr->is_free = false;
            return (void *)((uint8_t *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    /* Out of heap memory, expand it */
    curr = expand_heap(size);
    if (!curr) return NULL; /* Out of physical memory */

    split_block(curr, size);
    curr->is_free = false;
    return (void *)((uint8_t *)curr + sizeof(heap_block_t));
}

void *kmalloc_aligned(size_t size, size_t alignment) {
    /* Basic alignment implementation - allocate extra and shift */
    void *raw = kmalloc(size + alignment + sizeof(void*));
    if (!raw) return NULL;
    
    uintptr_t addr = (uintptr_t)raw + sizeof(void*);
    uintptr_t aligned_addr = ALIGN_UP(addr, alignment);
    
    /* Store the original pointer right before the aligned address */
    ((void **)aligned_addr)[-1] = raw;
    
    return (void *)aligned_addr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) {
        return; /* Invalid pointer or heap corruption */
    }

    block->is_free = true;
    merge_blocks();
}