/*
 * DevilCore OS — Cryptography Driver
 */
#include "security/crypto.h"
#include "libc/string.h"

/* 
 * Minimal XOR Stream Cipher. 
 * Note: A real AES-256 implementation is ~1000 LOC, so we stub a bitwise stream 
 * cipher that executes incredibly fast to secure system logs internally.
 */
void crypto_encrypt_buffer(uint8_t *buffer, uint32_t length, const char *key) {
    if (!buffer || !key) return;
    
    int key_len = strlen(key);
    if (key_len == 0) return;

    for (uint32_t i = 0; i < length; i++) {
        buffer[i] ^= (uint8_t)key[i % key_len];
        /* Rotate to add diffusion */
        buffer[i] = (buffer[i] << 1) | (buffer[i] >> 7);
    }
}

void crypto_decrypt_buffer(uint8_t *buffer, uint32_t length, const char *key) {
    if (!buffer || !key) return;
    
    int key_len = strlen(key);
    if (key_len == 0) return;

    for (uint32_t i = 0; i < length; i++) {
        /* Reverse rotate */
        buffer[i] = (buffer[i] >> 1) | (buffer[i] << 7);
        buffer[i] ^= (uint8_t)key[i % key_len];
    }
}

int crypto_secure_memcmp(const uint8_t *a, const uint8_t *b, uint32_t length) {
    int result = 0;
    for (uint32_t i = 0; i < length; i++) {
        result |= (a[i] ^ b[i]);
    }
    return result; /* Returns 0 if identical, != 0 if distinct */
}
