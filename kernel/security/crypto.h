/*
 * DevilCore OS — Cryptography Subsystem
 */
#ifndef DC_CRYPTO_H
#define DC_CRYPTO_H

#include "include/types.h"

/* Fast AES/XOR stub stream cipher for memory at rest */
void crypto_encrypt_buffer(uint8_t *buffer, uint32_t length, const char *key);

/* Fast AES/XOR stub decrypter */
void crypto_decrypt_buffer(uint8_t *buffer, uint32_t length, const char *key);

/* Constant time memory compare (mitigates timing attacks) */
int crypto_secure_memcmp(const uint8_t *a, const uint8_t *b, uint32_t length);

#endif /* DC_CRYPTO_H */
