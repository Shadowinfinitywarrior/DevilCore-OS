#ifndef _CRYPTO_H
#define _CRYPTO_H

#include <devil/types.h>

void sha256_init(void *ctx);
void sha256_update(void *ctx, const void *data, size_t len);
void sha256_final(void *ctx, uint8_t *digest);

void sha512_init(void *ctx);
void sha512_update(void *ctx, const void *data, size_t len);
void sha512_final(void *ctx, uint8_t *digest);

void aes_encrypt(const uint8_t *key, const uint8_t *iv, const void * plaintext, void *ciphertext, size_t len);
void aes_decrypt(const uint8_t *key, const uint8_t *iv, const void *ciphertext, void *plaintext, size_t len);

void chacha20(const uint8_t *key, const uint8_t *nonce, void *data, size_t len);
void poly1305(const uint8_t *key, const void *data, size_t len, uint8_t *mac);

void random_bytes(void *buf, size_t len);
uint64_t random_u64(void);

#endif // _CRYPTO_H