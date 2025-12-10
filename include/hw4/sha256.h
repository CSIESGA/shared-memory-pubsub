#ifndef __SHA256_H__
#define __SHA256_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * Usage:
 *   void sha256(void *src, uint64_t len, char out[SHA256_LEN + 1]);
 *
 * Description:
 *   Compute the SHA-256 hash of `src` (length `len`) and write the
 *   hex digest into `out`. `out` will be a C-style string.
 */

#define SHA256_LEN 64

struct sha256_ctx {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[64];
};

static inline void __sha256_init(struct sha256_ctx *ctx);
static inline void __sha256_update(struct sha256_ctx *ctx, void *data, uint64_t len);
static inline void __sha256_final(struct sha256_ctx *ctx, uint8_t out[32]);
static inline void __sha256_tohex(uint8_t src[32], char out[SHA256_LEN + 1]);

static void
sha256(void *src, uint64_t len, char out[SHA256_LEN + 1]) {
    struct sha256_ctx ctx;
    uint8_t tmp_out[32];

    __sha256_init(&ctx);
    __sha256_update(&ctx, src, len);
    __sha256_final(&ctx, tmp_out);
    __sha256_tohex(tmp_out, out);

    out[SHA256_LEN] = '\0';
}

static inline uint32_t __rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
static inline uint32_t __Ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static inline uint32_t __Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline uint32_t __Sigma0(uint32_t x) { return __rotr(x, 2) ^ __rotr(x, 13) ^ __rotr(x, 22); }
static inline uint32_t __Sigma1(uint32_t x) { return __rotr(x, 6) ^ __rotr(x, 11) ^ __rotr(x, 25); }
static inline uint32_t __sigma0(uint32_t x) { return __rotr(x, 7) ^ __rotr(x, 18) ^ (x >> 3); }
static inline uint32_t __sigma1(uint32_t x) { return __rotr(x, 17) ^ __rotr(x, 19) ^ (x >> 10); }

static const uint32_t K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static inline void
__sha256_process_block(struct sha256_ctx *ctx) {
    uint32_t W[64];
    
    for (int i = 0; i < 16; ++i) {
        W[i] = ((((uint32_t)ctx->buffer[(i << 2) + 0]) << 24) |
                (((uint32_t)ctx->buffer[(i << 2) + 1]) << 16) |
                (((uint32_t)ctx->buffer[(i << 2) + 2]) <<  8) |
                (((uint32_t)ctx->buffer[(i << 2) + 3]) <<  0));
    }

    for (int t = 16; t < 64; ++t) {
        W[t] = __sigma1(W[t - 2]) + W[t - 7] + __sigma0(W[t - 15]) + W[t - 16];
    }

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];

    for (int t = 0; t < 64; ++t) {
        uint32_t T1 = h + __Sigma1(e) + __Ch(e, f, g) + K[t] + W[t];
        uint32_t T2 = __Sigma0(a) + __Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static inline void
__sha256_init(struct sha256_ctx *ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->bitcount = 0;
}

static inline void
__sha256_update(struct sha256_ctx *ctx, void *data, uint64_t len) {
    uint8_t *p = (uint8_t *)data;
    uint32_t idx = ((ctx->bitcount >> 3) & 63);
    ctx->bitcount += (len << 3);

    if (idx) {
        uint32_t fill = 64 - idx;
        if (len >= fill) {
            memcpy(ctx->buffer + idx, p, fill);
            __sha256_process_block(ctx);
            p += fill;
            len -= fill;
            idx = 0;
        } else {
            memcpy(ctx->buffer + idx, p, len);
            return;
        }
    }

    while (len >= 64) {
        memcpy(ctx->buffer, p, 64);
        __sha256_process_block(ctx);
        p += 64;
        len -= 64;
    }

    if (len) {
        memcpy(ctx->buffer, p, len);
        memset(ctx->buffer + len, 0, 64 - len);
    }
}

static inline void
__sha256_final(struct sha256_ctx *ctx, uint8_t out[32]) {
    uint8_t pad[64];
    uint32_t idx = ((ctx->bitcount >> 3) & 63);

    pad[0] = 0x80;
    memset(pad + 1, 0, 63);

    uint32_t need = (idx < 56) ? (56 - idx) : (56 + 64 - idx);
    uint64_t bits = ctx->bitcount;

    __sha256_update(ctx, pad, need);

    uint8_t len_be[8];

    len_be[0] = (uint8_t)(bits >> 56);
    len_be[1] = (uint8_t)(bits >> 48);
    len_be[2] = (uint8_t)(bits >> 40);
    len_be[3] = (uint8_t)(bits >> 32);
    len_be[4] = (uint8_t)(bits >> 24);
    len_be[5] = (uint8_t)(bits >> 16);
    len_be[6] = (uint8_t)(bits >>  8);
    len_be[7] = (uint8_t)(bits >>  0);

    __sha256_update(ctx, len_be, 8);

    for (int i = 0; i < 8; ++i) {
        out[(i << 2) + 0] = (uint8_t)(ctx->state[i] >> 24);
        out[(i << 2) + 1] = (uint8_t)(ctx->state[i] >> 16);
        out[(i << 2) + 2] = (uint8_t)(ctx->state[i] >>  8);
        out[(i << 2) + 3] = (uint8_t)(ctx->state[i] >>  0);
    }
}

static inline void
__sha256_tohex(uint8_t src[32], char out[SHA256_LEN + 1])
{
    static char hex[] = "0123456789abcdef";

    for (int i = 0; i < 32; ++i) {
        out[(i << 1) + 0] = hex[(src[i] >> 4) & 0xF];
        out[(i << 1) + 1] = hex[(src[i] >> 0) & 0xF];
    }
}

#endif /* __SHA256_H__ */
