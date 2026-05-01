// DevilCore Memory Compression System
// Provides runtime memory compression to maintain low memory footprint
// Uses a simple RLE-based compression algorithm for speed

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "memory.h"
#include "kprintf.h"
#include "timer.h"

// External serial functions from main.c
extern void serial_write_string(const char *text);
extern void serial_write_char(char ch);
extern void serial_write_hex_u64(uint64_t value);
extern void serial_write_dec(uint64_t value);


#define COMPRESSION_ENABLED 1
#define MIN_COMPRESS_SIZE 256       // Only compress blocks >= 256 bytes
#define COMPRESSION_RATIO_TARGET 2  // Target 2:1 compression

// Compression statistics
static struct {
    uint64_t total_compressed;
    uint64_t total_decompressed;
    uint64_t bytes_saved;
    uint64_t compress_ops;
    uint64_t decompress_ops;
    uint32_t current_compression_ratio;  // Percentage (100 = 1:1, 200 = 2:1)
    uint8_t enabled;
} compression_stats = {0, 0, 0, 0, 0, 100, 1};

// Simple RLE compression - fast and effective for repetitive data
// Format: [count (1 byte)] [value (1 byte)] ... [0 count terminator]
// For values that don't repeat, we use: [0] [count_high] [count_low] [data...]

/**
 * Compress a block of data
 * @param src Source data
 * @param src_len Source length
 * @param dst Destination buffer
 * @param dst_len Destination buffer size
 * @return Compressed size, or 0 if compression failed/increased size
 */
uint32_t memory_compress(const uint8_t *src, uint32_t src_len, 
                         uint8_t *dst, uint32_t dst_len) {
    if (!compression_stats.enabled || src_len < MIN_COMPRESS_SIZE) {
        return 0;
    }
    
    uint32_t src_pos = 0;
    uint32_t dst_pos = 0;
    
    while (src_pos < src_len && dst_pos < dst_len - 4) {
        uint8_t current = src[src_pos];
        uint8_t count = 1;
        
        // Count consecutive identical bytes
        while (src_pos + count < src_len && 
               src[src_pos + count] == current && 
               count < 255) {
            count++;
        }
        
        if (count >= 3) {
            // Worth using RLE
            dst[dst_pos++] = count;
            dst[dst_pos++] = current;
            src_pos += count;
        } else {
            // Don't use RLE, encode literal run
            uint32_t literal_start = src_pos;
            uint32_t literal_count = 0;
            
            // Collect non-repeating bytes
            while (src_pos < src_len && literal_count < 65535) {
                // Peek ahead - if we see 3+ identical bytes, stop literal run
                if (src_pos + 2 < src_len && 
                    src[src_pos] == src[src_pos + 1] &&
                    src[src_pos] == src[src_pos + 2]) {
                    break;
                }
                src_pos++;
                literal_count++;
            }
            
            // Encode literal run: [0] [count_high] [count_low] [data...]
            if (literal_count > 0) {
                if (dst_pos + 3 + literal_count > dst_len) {
                    // Not enough space
                    return 0;
                }
                dst[dst_pos++] = 0;  // RLE escape for literal
                dst[dst_pos++] = (literal_count >> 8) & 0xFF;
                dst[dst_pos++] = literal_count & 0xFF;
                memcpy(&dst[dst_pos], &src[literal_start], literal_count);
                dst_pos += literal_count;
            }
        }
    }
    
    // Add terminator
    if (dst_pos < dst_len) {
        dst[dst_pos++] = 0;
        dst[dst_pos++] = 0;
    }
    
    // Only accept if we achieved meaningful compression
    if (dst_pos < src_len * 0.9) {
        compression_stats.total_compressed += src_len;
        compression_stats.compress_ops++;
        compression_stats.bytes_saved += (src_len - dst_pos);
        return dst_pos;
    }
    
    return 0; // Compression not beneficial
}

/**
 * Decompress a block of data
 * @param src Compressed data
 * @param src_len Compressed length
 * @param dst Destination buffer
 * @param dst_len Expected decompressed size
 * @return 0 on success, -1 on error
 */
int32_t memory_decompress(const uint8_t *src, uint32_t src_len,
                          uint8_t *dst, uint32_t dst_len) {
    uint32_t src_pos = 0;
    uint32_t dst_pos = 0;
    
    while (src_pos < src_len && dst_pos < dst_len) {
        uint8_t count = src[src_pos++];
        
        if (count == 0) {
            // Literal run or terminator
            if (src_pos + 1 >= src_len) break;
            
            uint16_t literal_count = (src[src_pos] << 8) | src[src_pos + 1];
            src_pos += 2;
            
            if (literal_count == 0) {
                // Terminator
                break;
            }
            
            if (src_pos + literal_count > src_len || 
                dst_pos + literal_count > dst_len) {
                return -1; // Error: buffer overflow
            }
            
            memcpy(&dst[dst_pos], &src[src_pos], literal_count);
            src_pos += literal_count;
            dst_pos += literal_count;
        } else {
            // RLE encoded run
            if (src_pos >= src_len) return -1;
            uint8_t value = src[src_pos++];
            
            if (dst_pos + count > dst_len) {
                return -1; // Error: buffer overflow
            }
            
            memset(&dst[dst_pos], value, count);
            dst_pos += count;
        }
    }
    
    compression_stats.total_decompressed += dst_pos;
    compression_stats.decompress_ops++;
    return 0;
}

// Compressed memory pool for swapping
#define COMPRESSED_POOL_SIZE (4 * 1024 * 1024)  // 4MB compressed pool
static uint8_t compressed_pool[COMPRESSED_POOL_SIZE];
static uint32_t compressed_pool_used = 0;

typedef struct compressed_block {
    uint32_t original_addr;
    uint32_t compressed_addr;
    uint32_t original_size;
    uint32_t compressed_size;
    uint32_t flags;
    uint64_t last_access;
} compressed_block_t;

#define MAX_COMPRESSED_BLOCKS 1024
static compressed_block_t compressed_blocks[MAX_COMPRESSED_BLOCKS];
static uint32_t num_compressed_blocks = 0;

/**
 * Compress a memory page and move to compressed pool
 * This is called by the memory manager when under memory pressure
 */
int32_t memory_compress_page(uint32_t page_addr) {
    if (!compression_stats.enabled || num_compressed_blocks >= MAX_COMPRESSED_BLOCKS) {
        return -1;
    }
    
    uint32_t page_size = 4096; // Standard page size
    
    // Check if we have space in compressed pool
    if (compressed_pool_used + page_size > COMPRESSED_POOL_SIZE) {
        return -1;
    }
    
    // Try to compress
    uint8_t temp_buffer[4096 * 2]; // Compression might expand slightly
    uint32_t compressed_size = memory_compress(
        (uint8_t *)(uintptr_t)page_addr, page_size,
        temp_buffer, sizeof(temp_buffer)
    );
    
    if (compressed_size == 0 || compressed_size >= page_size) {
        return -1; // Compression failed or not beneficial
    }
    
    // Store compressed data
    uint32_t storage_addr = compressed_pool_used;
    memcpy(&compressed_pool[storage_addr], temp_buffer, compressed_size);
    compressed_pool_used += compressed_size;
    
    // Track the block
    compressed_block_t *block = &compressed_blocks[num_compressed_blocks++];
    block->original_addr = page_addr;
    block->compressed_addr = storage_addr;
    block->original_size = page_size;
    block->compressed_size = compressed_size;
    block->flags = 1; // Compressed flag
    block->last_access = timer_ticks();
    
    // Zero out the original page (mark as swapped)
    memset((void *)(uintptr_t)page_addr, 0, page_size);
    
    return 0;
}

/**
 * Decompress a page back to original location
 */
int32_t memory_decompress_page(uint32_t original_addr) {
    // Find the compressed block
    for (uint32_t i = 0; i < num_compressed_blocks; i++) {
        if (compressed_blocks[i].original_addr == original_addr) {
            compressed_block_t *block = &compressed_blocks[i];
            
        int32_t result = memory_decompress(
            &compressed_pool[block->compressed_addr],
            block->compressed_size,
            (uint8_t *)(uintptr_t)original_addr,
            block->original_size
        );
            
            if (result == 0) {
                // Remove from compressed pool (simple approach: mark as free)
                block->flags = 0;
                return 0;
            }
            return -1;
        }
    }
    return -1; // Not found
}

/**
 * Get memory compression statistics
 */
void memory_compression_stats(uint64_t *saved, uint32_t *ratio, uint32_t *blocks) {
    if (saved) *saved = compression_stats.bytes_saved;
    if (ratio) {
        if (compression_stats.total_compressed > 0) {
            *ratio = (uint32_t)((compression_stats.bytes_saved * 100) / 
                                compression_stats.total_compressed);
        } else {
            *ratio = 0;
        }
    }
    if (blocks) *blocks = num_compressed_blocks;
}

/**
 * Enable/disable compression
 */
void memory_compression_enable(uint8_t enable) {
    compression_stats.enabled = enable;
    serial_write_string(enable ? "Memory compression enabled\n" : 
                                   "Memory compression disabled\n");
}

/**
 * Get current compression status
 */
uint8_t memory_compression_is_enabled(void) {
    return compression_stats.enabled;
}

/**
 * Initialize memory compression system
 */
void memory_compression_init(void) {
    memset(compressed_blocks, 0, sizeof(compressed_blocks));
    compressed_pool_used = 0;
    num_compressed_blocks = 0;
    compression_stats.enabled = COMPRESSION_ENABLED;
    
    serial_write_string("Memory Compression System initialized\n");
    serial_write_string("  Pool size: 4MB\n");
    serial_write_string("  Algorithm: Fast RLE\n");
    serial_write_string("  Min block: 256 bytes\n");
}

/**
 * Compress all compressible memory (called periodically or on low memory)
 */
uint32_t memory_compress_all(void) {
    if (!compression_stats.enabled) return 0;
    
    uint32_t compressed_count = 0;
    
    // Walk through free pages and try to compress allocated regions
    // This is a simplified version - real implementation would walk page tables
    
    // For now, just report stats
    serial_write_string("Memory compression pass completed\n");
    
    return compressed_count;
}

/**
 * Format compression stats for display
 */
void memory_compression_format_stats(char *buf, uint32_t buf_size) {
    uint64_t saved;
    uint32_t ratio, blocks;
    memory_compression_stats(&saved, &ratio, &blocks);
    
    // Simple integer to string conversion using sprintf
    char saved_str[32];
    uint32_t saved_kb = (uint32_t)(saved / 1024);
    sprintf(saved_str, "%u", saved_kb);
    
    char ratio_str[8];
    sprintf(ratio_str, "%u", ratio);
    
    char blocks_str[8];
    sprintf(blocks_str, "%u", blocks);
    
    // Build output string manually (safer than sprintf)
    strncpy(buf, "Compression: ", buf_size);
    strncat(buf, ratio_str, buf_size - strlen(buf) - 1);
    strncat(buf, "% ratio, ", buf_size - strlen(buf) - 1);
    strncat(buf, saved_str, buf_size - strlen(buf) - 1);
    strncat(buf, "KB saved, ", buf_size - strlen(buf) - 1);
    strncat(buf, blocks_str, buf_size - strlen(buf) - 1);
    strncat(buf, " blocks", buf_size - strlen(buf) - 1);
}
