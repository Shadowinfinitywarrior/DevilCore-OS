// kernel/crypto/sha256.c
// SHA-256 implementation

#include <devil/types.h>
#include "random.h"

typedef struct sha256_ctx {
    uint32_t h[8];
    uint64_t len;
    uint8_t  buf[64];
} sha256_ctx_t;

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x550c7dc3, 0x80deb1fb,
    0x9b05688c, 0x2e1b2138, 0x4d2c6dfc, 0x5ac260a5,
    0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8,
    0xbf597fc7, 0xc6e00bf33, 0xd5a79147, 0x06ca6351,
    0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e,
    0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70,
    0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585,
    0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c,
    0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814,
    0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2, 0xca273ece, 0xd186b8c7, 0xeada7dd6,
    0xf57d4f7f, 0x06f067aa, 0x0a637dc5, 0x113f9804,
    0x1b710f35, 0x28db77f5, 0x32caab7b, 0x3c9ebe0a,
    0x431d67c4, 0x4cc5d4a4, 0x597f299c, 0x5fcb6fab,
    0x6c44198c, 0x7a325914, 0x084e391a
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & ((y) ^ (z))) ^ (z))
#define MAJ(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static void sha256_block(sha256_ctx_t *ctx, const uint8_t *block) {
    uint32_t w[64];
    
    for (int i = 0; i < 16; i++) {
        w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) |
               (block[i*4+2] << 8) | block[i*4+3];
    }
    
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    uint32_t a = ctx->h[0], b = ctx->h[1], c = ctx->h[2], d = ctx->h[3];
    uint32_t e = ctx->h[4], f = ctx->h[5], g = ctx->h[6], h = ctx->h[7];
    
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    ctx->h[0] += a; ctx->h[1] += b; ctx->h[2] += c; ctx->h[3] += d;
    ctx->h[4] += e; ctx->h[5] += f; ctx->h[6] += g; ctx->h[7] += h;
}

void sha256_init(sha256_ctx_t *ctx) {
    ctx->h[0] = 0x6a09e667;
    ctx->h[1] = 0xbb67ae85;
    ctx->h[2] = 0x3c6ef372;
    ctx->h[3] = 0xa54ff53a;
    ctx->h[4] = 0x510e527f;
    ctx->h[5] = 0x9b05688c;
    ctx->h[6] = 0x1f83d9ab;
    ctx->h[7] = 0x5be0cd19;
    ctx->len = 0;
}

void sha256_update(sha256_ctx_t *ctx, const void *data, size_t len) {
    const uint8_t *d = data;
    size_t i = 0;
    
    if (ctx->len % 64) {
        size_t n = 64 - (ctx->len % 64);
        if (n > len) n = len;
        for (size_t j = 0; j < n; j++) {
            ctx->buf[ctx->len % 64 + j] = d[j];
        }
        ctx->len += n;
        i = n;
    }
    
    for (; i + 64 <= len; i += 64) {
        sha256_block(ctx, d + i);
    }
    
    for (; i < len; i++) {
        ctx->buf[ctx->len % 64] = d[i];
        ctx->len++;
    }
}

void sha256_final(sha256_ctx_t *ctx, uint8_t *digest) {
    uint64_t bits = ctx->len * 8;
    
    ctx->buf[ctx->len % 64] = 0x80;
    ctx->len++;
    
    if (ctx->len % 64 > 56) {
        for (size_t i = ctx->len % 64; i < 64; i++) {
            ctx->buf[i] = 0;
        }
        sha256_block(ctx, ctx->buf);
        ctx->len = 0;
    }
    
    for (size_t i = ctx->len % 64; i < 56; i++) {
        ctx->buf[i] = 0;
    }
    
    for (int i = 0; i < 8; i++) {
        ctx->buf[56 + i] = (bits >> (56 - i*8)) & 0xFF;
    }
    sha256_block(ctx, ctx->buf);
    
    for (int i = 0; i < 32; i++) {
        digest[i] = (ctx->h[i/4] >> (24 - (i%4)*8)) & 0xFF;
    }
}