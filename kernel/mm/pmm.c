/*
 * DevilCore OS — Physical Memory Manager (PMM)
 * Manages physical memory using a bitmap. Each bit represents a 4KB frame.
 */
#include "pmm.h"
#include "../libc/string.h"

#define FRAME_SIZE    PAGE_SIZE
#define BITMAP_SIZE   (1024 * 1024 * 4) /* Enough for 128GB of RAM tracking (1 bit per 4K) */
#define BLOCKS_PER_BYTE 8

/* Array holding the bitmap. 1 = Used/Reserved, 0 = Free */
/* Placed in BSS. It consumes ~128KB for 4GB RAM tracking support, max 4MB for 128GB. */
/* To preserve BSS space, we'll limit it to track up to 8GB of RAM for now = 256KB bitmap */
#define MAX_TRACKED_MEM (8ULL * 1024 * 1024 * 1024)
#define NUM_FRAMES      (MAX_TRACKED_MEM / FRAME_SIZE)
#define BITMAP_BYTES    (NUM_FRAMES / BLOCKS_PER_BYTE)

static uint8_t g_pmm_bitmap[BITMAP_BYTES];
static uint64_t g_pmm_total_memory = 0;
static uint64_t g_pmm_used_blocks = 0;
static uint64_t g_pmm_max_blocks = 0;

/* Inline helpers for bit manipulation */
static inline void bitmap_set(uint64_t bit) {
    if (bit < NUM_FRAMES)
        g_pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint64_t bit) {
    if (bit < NUM_FRAMES)
        g_pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int bitmap_test(uint64_t bit) {
    if (bit >= NUM_FRAMES) return 1; /* Treat out-of-bounds as used */
    return g_pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint64_t mem_size) {
    g_pmm_total_memory = mem_size;
    g_pmm_max_blocks = mem_size / FRAME_SIZE;
    if (g_pmm_max_blocks > NUM_FRAMES) {
        g_pmm_max_blocks = NUM_FRAMES; /* Clamp to our max tracked memory */
    }
    g_pmm_used_blocks = g_pmm_max_blocks; /* Assume all used until regions are freed */

    /* Mark all memory as used initially */
    memset(g_pmm_bitmap, 0xFF, BITMAP_BYTES);
}

void pmm_init_region(uint64_t base, uint64_t size) {
    uint64_t align_base = ALIGN_UP(base, FRAME_SIZE);
    uint64_t align_size = ALIGN_DOWN(size - (align_base - base), FRAME_SIZE);
    uint64_t blocks = align_size / FRAME_SIZE;
    uint64_t start_bit = align_base / FRAME_SIZE;

    for (uint64_t i = 0; i < blocks; i++) {
        bitmap_clear(start_bit + i);
        if (g_pmm_used_blocks > 0) g_pmm_used_blocks--;
    }
}

void pmm_deinit_region(uint64_t base, uint64_t size) {
    uint64_t align_base = ALIGN_DOWN(base, FRAME_SIZE);
    uint64_t align_size = ALIGN_UP(size + (base - align_base), FRAME_SIZE);
    uint64_t blocks = align_size / FRAME_SIZE;
    uint64_t start_bit = align_base / FRAME_SIZE;

    for (uint64_t i = 0; i < blocks; i++) {
        if (!bitmap_test(start_bit + i)) {
            bitmap_set(start_bit + i);
            g_pmm_used_blocks++;
        }
    }
}

uint64_t pmm_alloc_blocks(size_t count) {
    if (count == 0) return 0;
    if (count == 1) return pmm_alloc_frame();
    if (g_pmm_used_blocks + count > g_pmm_max_blocks) return 0;

    for (uint64_t i = 0; i <= g_pmm_max_blocks - count; i++) {
        bool found = true;
        for (uint64_t j = 0; j < count; j++) {
            if (bitmap_test(i + j)) {
                found = false;
                i += j; /* Skip to next potential start */
                break;
            }
        }
        if (found) {
            for (uint64_t j = 0; j < count; j++) bitmap_set(i + j);
            g_pmm_used_blocks += count;
            return i * FRAME_SIZE;
        }
    }
    return 0;
}

uint64_t pmm_alloc_frame(void) {
    if (g_pmm_used_blocks >= g_pmm_max_blocks) return 0;

    for (uint64_t i = 0; i < g_pmm_max_blocks / 8; i++) {
        if (g_pmm_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                uint64_t bit = i * 8 + j;
                if (!bitmap_test(bit)) {
                    bitmap_set(bit);
                    g_pmm_used_blocks++;
                    return bit * FRAME_SIZE;
                }
            }
        }
    }
    return 0;
}

void pmm_free_frame(uint64_t phys_addr) {
    uint64_t bit = phys_addr / FRAME_SIZE;
    if (bitmap_test(bit)) {
        bitmap_clear(bit);
        if (g_pmm_used_blocks > 0) g_pmm_used_blocks--;
    }
}

uint64_t pmm_get_free_memory(void) {
    return (g_pmm_max_blocks - g_pmm_used_blocks) * FRAME_SIZE;
}

uint64_t pmm_get_total_memory(void) {
    return g_pmm_total_memory;
}