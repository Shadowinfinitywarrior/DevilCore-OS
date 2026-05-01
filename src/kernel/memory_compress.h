// DevilCore Memory Compression System Header
#ifndef DEVILCORE_MEMORY_COMPRESS_H
#define DEVILCORE_MEMORY_COMPRESS_H

#include <stdint.h>

// Initialize the compression system
void memory_compression_init(void);

// Enable/disable compression
void memory_compression_enable(uint8_t enable);
uint8_t memory_compression_is_enabled(void);

// Compress/decompress functions
uint32_t memory_compress(const uint8_t *src, uint32_t src_len, 
                         uint8_t *dst, uint32_t dst_len);
int32_t memory_decompress(const uint8_t *src, uint32_t src_len,
                          uint8_t *dst, uint32_t dst_len);

// Page-level compression for swapping
int32_t memory_compress_page(uint32_t page_addr);
int32_t memory_decompress_page(uint32_t original_addr);

// Statistics
void memory_compression_stats(uint64_t *saved, uint32_t *ratio, uint32_t *blocks);
void memory_compression_format_stats(char *buf, uint32_t buf_size);
uint32_t memory_compress_all(void);

#endif
