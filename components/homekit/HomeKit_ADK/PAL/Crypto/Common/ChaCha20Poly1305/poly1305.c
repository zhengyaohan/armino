/**
 * \file poly1305.c
 *
 * \brief Poly1305 authentication algorithm.
 *
 *  Copyright (C) 2006-2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#include "poly1305.h"
#include <string.h>

#if (defined(__ARMCC_VERSION) || defined(_MSC_VER)) && !defined(inline) && !defined(__cplusplus)
#define inline __inline
#endif

#define INTERNAL_VALIDATE_RET(cond, ret) \
    do { \
    } while (0)
#define INTERNAL_VALIDATE(cond) \
    do { \
    } while (0)

/* Parameter validation macros */
#define POLY1305_VALIDATE_RET(cond) INTERNAL_VALIDATE_RET(cond, ERR_POLY1305_BAD_INPUT_DATA)
#define POLY1305_VALIDATE(cond)     INTERNAL_VALIDATE(cond)

#define POLY1305_BLOCK_SIZE_BYTES (16U)

#define BYTES_TO_U32_LE(data, offset) \
    ((uint32_t)(data)[offset] | (uint32_t)((uint32_t)(data)[(offset) + 1] << 8) | \
     (uint32_t)((uint32_t)(data)[(offset) + 2] << 16) | (uint32_t)((uint32_t)(data)[(offset) + 3] << 24))

/*
 * Our implementation is tuned for 32-bit platforms with a 64-bit multiplier.
 * However we provided an alternative for platforms without such a multiplier.
 */
#if defined(NO_64BIT_MULTIPLICATION)
static uint64_t mul64(uint32_t a, uint32_t b) {
    /* a = al + 2**16 ah, b = bl + 2**16 bh */
    const uint16_t al = (uint16_t) a;
    const uint16_t bl = (uint16_t) b;
    const uint16_t ah = a >> 16;
    const uint16_t bh = b >> 16;

    /* ab = al*bl + 2**16 (ah*bl + bl*bh) + 2**32 ah*bh */
    const uint32_t lo = (uint32_t) al * bl;
    const uint64_t me = (uint64_t)((uint32_t) ah * bl) + (uint32_t) al * bh;
    const uint32_t hi = (uint32_t) ah * bh;

    return (lo + (me << 16) + ((uint64_t) hi << 32));
}
#else
static inline uint64_t mul64(uint32_t a, uint32_t b) {
    return ((uint64_t) a * b);
}
#endif

/**
 * \brief                   Process blocks with Poly1305.
 *
 * \param ctx               The Poly1305 context.
 * \param nblocks           Number of blocks to process. Note that this
 *                          function only processes full blocks.
 * \param input             Buffer containing the input block(s).
 * \param needs_padding     Set to 0 if the padding bit has already been
 *                          applied to the input data before calling this
 *                          function.  Otherwise, set this parameter to 1.
 */
static void poly1305_process(
        poly1305_context_custom* ctx,
        size_t nblocks,
        const unsigned char* input,
        uint32_t needs_padding) {
    uint64_t d0, d1, d2, d3;
    uint32_t acc0, acc1, acc2, acc3, acc4;
    uint32_t r0, r1, r2, r3;
    uint32_t rs1, rs2, rs3;
    size_t offset = 0U;
    size_t i;

    r0 = ctx->r[0];
    r1 = ctx->r[1];
    r2 = ctx->r[2];
    r3 = ctx->r[3];

    rs1 = r1 + (r1 >> 2U);
    rs2 = r2 + (r2 >> 2U);
    rs3 = r3 + (r3 >> 2U);

    acc0 = ctx->acc[0];
    acc1 = ctx->acc[1];
    acc2 = ctx->acc[2];
    acc3 = ctx->acc[3];
    acc4 = ctx->acc[4];

    /* Process full blocks */
    for (i = 0U; i < nblocks; i++) {
        /* The input block is treated as a 128-bit little-endian integer */
        d0 = BYTES_TO_U32_LE(input, offset + 0);
        d1 = BYTES_TO_U32_LE(input, offset + 4);
        d2 = BYTES_TO_U32_LE(input, offset + 8);
        d3 = BYTES_TO_U32_LE(input, offset + 12);

        /* Compute: acc += (padded) block as a 130-bit integer */
        d0 += (uint64_t) acc0;
        d1 += (uint64_t) acc1 + (d0 >> 32U);
        d2 += (uint64_t) acc2 + (d1 >> 32U);
        d3 += (uint64_t) acc3 + (d2 >> 32U);
        acc0 = (uint32_t) d0;
        acc1 = (uint32_t) d1;
        acc2 = (uint32_t) d2;
        acc3 = (uint32_t) d3;
        acc4 += (uint32_t)(d3 >> 32U) + needs_padding;

        /* Compute: acc *= r */
        d0 = mul64(acc0, r0) + mul64(acc1, rs3) + mul64(acc2, rs2) + mul64(acc3, rs1);
        d1 = mul64(acc0, r1) + mul64(acc1, r0) + mul64(acc2, rs3) + mul64(acc3, rs2) + mul64(acc4, rs1);
        d2 = mul64(acc0, r2) + mul64(acc1, r1) + mul64(acc2, r0) + mul64(acc3, rs3) + mul64(acc4, rs2);
        d3 = mul64(acc0, r3) + mul64(acc1, r2) + mul64(acc2, r1) + mul64(acc3, r0) + mul64(acc4, rs3);
        acc4 *= r0;

        /* Compute: acc %= (2^130 - 5) (partial remainder) */
        d1 += (d0 >> 32);
        d2 += (d1 >> 32);
        d3 += (d2 >> 32);
        acc0 = (uint32_t) d0;
        acc1 = (uint32_t) d1;
        acc2 = (uint32_t) d2;
        acc3 = (uint32_t) d3;
        acc4 = (uint32_t)(d3 >> 32) + acc4;

        d0 = (uint64_t) acc0 + (acc4 >> 2) + (acc4 & 0xFFFFFFFCU);
        acc4 &= 3U;
        acc0 = (uint32_t) d0;
        d0 = (uint64_t) acc1 + (d0 >> 32U);
        acc1 = (uint32_t) d0;
        d0 = (uint64_t) acc2 + (d0 >> 32U);
        acc2 = (uint32_t) d0;
        d0 = (uint64_t) acc3 + (d0 >> 32U);
        acc3 = (uint32_t) d0;
        d0 = (uint64_t) acc4 + (d0 >> 32U);
        acc4 = (uint32_t) d0;

        offset += POLY1305_BLOCK_SIZE_BYTES;
    }

    ctx->acc[0] = acc0;
    ctx->acc[1] = acc1;
    ctx->acc[2] = acc2;
    ctx->acc[3] = acc3;
    ctx->acc[4] = acc4;
}

/**
 * \brief                   Compute the Poly1305 MAC
 *
 * \param ctx               The Poly1305 context.
 * \param mac               The buffer to where the MAC is written. Must be
 *                          big enough to contain the 16-byte MAC.
 */
static void poly1305_compute_mac(const poly1305_context_custom* ctx, unsigned char mac[16]) {
    uint64_t d;
    uint32_t g0, g1, g2, g3, g4;
    uint32_t acc0, acc1, acc2, acc3, acc4;
    uint32_t mask;
    uint32_t mask_inv;

    acc0 = ctx->acc[0];
    acc1 = ctx->acc[1];
    acc2 = ctx->acc[2];
    acc3 = ctx->acc[3];
    acc4 = ctx->acc[4];

    /* Before adding 's' we ensure that the accumulator is mod 2^130 - 5.
     * We do this by calculating acc - (2^130 - 5), then checking if
     * the 131st bit is set. If it is, then reduce: acc -= (2^130 - 5)
     */

    /* Calculate acc + -(2^130 - 5) */
    d = ((uint64_t) acc0 + 5U);
    g0 = (uint32_t) d;
    d = ((uint64_t) acc1 + (d >> 32));
    g1 = (uint32_t) d;
    d = ((uint64_t) acc2 + (d >> 32));
    g2 = (uint32_t) d;
    d = ((uint64_t) acc3 + (d >> 32));
    g3 = (uint32_t) d;
    g4 = acc4 + (uint32_t)(d >> 32U);

    /* mask == 0xFFFFFFFF if 131st bit is set, otherwise mask == 0 */
    mask = (uint32_t) 0U - (g4 >> 2U);
    mask_inv = ~mask;

    /* If 131st bit is set then acc=g, otherwise, acc is unmodified */
    acc0 = (acc0 & mask_inv) | (g0 & mask);
    acc1 = (acc1 & mask_inv) | (g1 & mask);
    acc2 = (acc2 & mask_inv) | (g2 & mask);
    acc3 = (acc3 & mask_inv) | (g3 & mask);

    /* Add 's' */
    d = (uint64_t) acc0 + ctx->s[0];
    acc0 = (uint32_t) d;
    d = (uint64_t) acc1 + ctx->s[1] + (d >> 32U);
    acc1 = (uint32_t) d;
    d = (uint64_t) acc2 + ctx->s[2] + (d >> 32U);
    acc2 = (uint32_t) d;
    acc3 += ctx->s[3] + (uint32_t)(d >> 32U);

    /* Compute MAC (128 least significant bits of the accumulator) */
    mac[0] = (unsigned char) (acc0);
    mac[1] = (unsigned char) (acc0 >> 8);
    mac[2] = (unsigned char) (acc0 >> 16);
    mac[3] = (unsigned char) (acc0 >> 24);
    mac[4] = (unsigned char) (acc1);
    mac[5] = (unsigned char) (acc1 >> 8);
    mac[6] = (unsigned char) (acc1 >> 16);
    mac[7] = (unsigned char) (acc1 >> 24);
    mac[8] = (unsigned char) (acc2);
    mac[9] = (unsigned char) (acc2 >> 8);
    mac[10] = (unsigned char) (acc2 >> 16);
    mac[11] = (unsigned char) (acc2 >> 24);
    mac[12] = (unsigned char) (acc3);
    mac[13] = (unsigned char) (acc3 >> 8);
    mac[14] = (unsigned char) (acc3 >> 16);
    mac[15] = (unsigned char) (acc3 >> 24);
}

void poly1305_init_custom(poly1305_context_custom* ctx) {
    POLY1305_VALIDATE(ctx != NULL);

    memset(ctx, 0, sizeof(poly1305_context_custom));
}

void poly1305_free_custom(poly1305_context_custom* ctx) {
    if (ctx == NULL)
        return;

    memset(ctx, 0, sizeof(poly1305_context_custom));
}

int poly1305_starts_custom(poly1305_context_custom* ctx, const unsigned char key[32]) {
    POLY1305_VALIDATE_RET(ctx != NULL);
    POLY1305_VALIDATE_RET(key != NULL);

    /* r &= 0x0ffffffc0ffffffc0ffffffc0fffffff */
    ctx->r[0] = BYTES_TO_U32_LE(key, 0) & 0x0FFFFFFFU;
    ctx->r[1] = BYTES_TO_U32_LE(key, 4) & 0x0FFFFFFCU;
    ctx->r[2] = BYTES_TO_U32_LE(key, 8) & 0x0FFFFFFCU;
    ctx->r[3] = BYTES_TO_U32_LE(key, 12) & 0x0FFFFFFCU;

    ctx->s[0] = BYTES_TO_U32_LE(key, 16);
    ctx->s[1] = BYTES_TO_U32_LE(key, 20);
    ctx->s[2] = BYTES_TO_U32_LE(key, 24);
    ctx->s[3] = BYTES_TO_U32_LE(key, 28);

    /* Initial accumulator state */
    ctx->acc[0] = 0U;
    ctx->acc[1] = 0U;
    ctx->acc[2] = 0U;
    ctx->acc[3] = 0U;
    ctx->acc[4] = 0U;

    /* Queue initially empty */
    memset(ctx->queue, 0, sizeof(ctx->queue));
    ctx->queue_len = 0U;

    return (0);
}

int poly1305_update_custom(poly1305_context_custom* ctx, const unsigned char* input, size_t ilen) {
    size_t offset = 0U;
    size_t remaining = ilen;
    size_t queue_free_len;
    size_t nblocks;
    POLY1305_VALIDATE_RET(ctx != NULL);
    POLY1305_VALIDATE_RET(ilen == 0 || input != NULL);

    if ((remaining > 0U) && (ctx->queue_len > 0U)) {
        queue_free_len = (POLY1305_BLOCK_SIZE_BYTES - ctx->queue_len);

        if (ilen < queue_free_len) {
            /* Not enough data to complete the block.
             * Store this data with the other leftovers.
             */
            memcpy(&ctx->queue[ctx->queue_len], input, ilen);

            ctx->queue_len += ilen;

            remaining = 0U;
        } else {
            /* Enough data to produce a complete block */
            memcpy(&ctx->queue[ctx->queue_len], input, queue_free_len);

            ctx->queue_len = 0U;

            poly1305_process(ctx, 1U, ctx->queue, 1U); /* add padding bit */

            offset += queue_free_len;
            remaining -= queue_free_len;
        }
    }

    if (remaining >= POLY1305_BLOCK_SIZE_BYTES) {
        nblocks = remaining / POLY1305_BLOCK_SIZE_BYTES;

        poly1305_process(ctx, nblocks, &input[offset], 1U);

        offset += nblocks * POLY1305_BLOCK_SIZE_BYTES;
        remaining %= POLY1305_BLOCK_SIZE_BYTES;
    }

    if (remaining > 0U) {
        /* Store partial block */
        ctx->queue_len = remaining;
        memcpy(ctx->queue, &input[offset], remaining);
    }

    return (0);
}

int poly1305_finish_custom(poly1305_context_custom* ctx, unsigned char mac[16]) {
    POLY1305_VALIDATE_RET(ctx != NULL);
    POLY1305_VALIDATE_RET(mac != NULL);

    /* Process any leftover data */
    if (ctx->queue_len > 0U) {
        /* Add padding bit */
        ctx->queue[ctx->queue_len] = 1U;
        ctx->queue_len++;

        /* Pad with zeroes */
        memset(&ctx->queue[ctx->queue_len], 0, POLY1305_BLOCK_SIZE_BYTES - ctx->queue_len);

        poly1305_process(
                ctx,
                1U, /* Process 1 block */
                ctx->queue,
                0U); /* Already padded above */
    }

    poly1305_compute_mac(ctx, mac);

    return (0);
}

int poly1305_mac_custom(const unsigned char key[32], const unsigned char* input, size_t ilen, unsigned char mac[16]) {
    poly1305_context_custom ctx;
    int ret;
    POLY1305_VALIDATE_RET(key != NULL);
    POLY1305_VALIDATE_RET(mac != NULL);
    POLY1305_VALIDATE_RET(ilen == 0 || input != NULL);

    poly1305_init_custom(&ctx);

    ret = poly1305_starts_custom(&ctx, key);
    if (ret != 0)
        goto cleanup;

    ret = poly1305_update_custom(&ctx, input, ilen);
    if (ret != 0)
        goto cleanup;

    ret = poly1305_finish_custom(&ctx, mac);

cleanup:
    poly1305_free_custom(&ctx);
    return (ret);
}
