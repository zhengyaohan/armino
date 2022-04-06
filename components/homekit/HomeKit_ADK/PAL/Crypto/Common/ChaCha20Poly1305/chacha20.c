/**
 * \file chacha20.c
 *
 * \brief ChaCha20 cipher.
 *
 * \author Daniel King <damaki.gh@gmail.com>
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

#include "chacha20.h"
#include <stddef.h>
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
#define CHACHA20_VALIDATE_RET(cond) INTERNAL_VALIDATE_RET(cond, ERR_CHACHA20_BAD_INPUT_DATA)
#define CHACHA20_VALIDATE(cond)     INTERNAL_VALIDATE(cond)

#define BYTES_TO_U32_LE(data, offset) \
    ((uint32_t)(data)[offset] | (uint32_t)((uint32_t)(data)[(offset) + 1] << 8) | \
     (uint32_t)((uint32_t)(data)[(offset) + 2] << 16) | (uint32_t)((uint32_t)(data)[(offset) + 3] << 24))

#define ROTL32(value, amount) ((uint32_t)((value) << (amount)) | ((value) >> (32 - (amount))))

#define CHACHA20_CTR_INDEX (12U)

#define CHACHA20_BLOCK_SIZE_BYTES (4U * 16U)

/**
 * \brief           ChaCha20 quarter round operation.
 *
 *                  The quarter round is defined as follows (from RFC 7539):
 *                      1.  a += b; d ^= a; d <<<= 16;
 *                      2.  c += d; b ^= c; b <<<= 12;
 *                      3.  a += b; d ^= a; d <<<= 8;
 *                      4.  c += d; b ^= c; b <<<= 7;
 *
 * \param state     ChaCha20 state to modify.
 * \param a         The index of 'a' in the state.
 * \param b         The index of 'b' in the state.
 * \param c         The index of 'c' in the state.
 * \param d         The index of 'd' in the state.
 */
static inline void chacha20_quarter_round(uint32_t state[16], size_t a, size_t b, size_t c, size_t d) {
    /* a += b; d ^= a; d <<<= 16; */
    state[a] += state[b];
    state[d] ^= state[a];
    state[d] = ROTL32(state[d], 16);

    /* c += d; b ^= c; b <<<= 12 */
    state[c] += state[d];
    state[b] ^= state[c];
    state[b] = ROTL32(state[b], 12);

    /* a += b; d ^= a; d <<<= 8; */
    state[a] += state[b];
    state[d] ^= state[a];
    state[d] = ROTL32(state[d], 8);

    /* c += d; b ^= c; b <<<= 7; */
    state[c] += state[d];
    state[b] ^= state[c];
    state[b] = ROTL32(state[b], 7);
}

/**
 * \brief           Perform the ChaCha20 inner block operation.
 *
 *                  This function performs two rounds: the column round and the
 *                  diagonal round.
 *
 * \param state     The ChaCha20 state to update.
 */
static void chacha20_inner_block(uint32_t state[16]) {
    chacha20_quarter_round(state, 0, 4, 8, 12);
    chacha20_quarter_round(state, 1, 5, 9, 13);
    chacha20_quarter_round(state, 2, 6, 10, 14);
    chacha20_quarter_round(state, 3, 7, 11, 15);

    chacha20_quarter_round(state, 0, 5, 10, 15);
    chacha20_quarter_round(state, 1, 6, 11, 12);
    chacha20_quarter_round(state, 2, 7, 8, 13);
    chacha20_quarter_round(state, 3, 4, 9, 14);
}

/**
 * \brief               Generates a keystream block.
 *
 * \param initial_state The initial ChaCha20 state (key, nonce, counter).
 * \param keystream     Generated keystream bytes are written to this buffer.
 */
static void chacha20_block(const uint32_t initial_state[16], unsigned char keystream[64]) {
    uint32_t working_state[16];
    size_t i;

    memcpy(working_state, initial_state, CHACHA20_BLOCK_SIZE_BYTES);

    for (i = 0U; i < 10U; i++)
        chacha20_inner_block(working_state);

    working_state[0] += initial_state[0];
    working_state[1] += initial_state[1];
    working_state[2] += initial_state[2];
    working_state[3] += initial_state[3];
    working_state[4] += initial_state[4];
    working_state[5] += initial_state[5];
    working_state[6] += initial_state[6];
    working_state[7] += initial_state[7];
    working_state[8] += initial_state[8];
    working_state[9] += initial_state[9];
    working_state[10] += initial_state[10];
    working_state[11] += initial_state[11];
    working_state[12] += initial_state[12];
    working_state[13] += initial_state[13];
    working_state[14] += initial_state[14];
    working_state[15] += initial_state[15];

    for (i = 0U; i < 16; i++) {
        size_t offset = i * 4U;

        keystream[offset] = (unsigned char) (working_state[i]);
        keystream[offset + 1U] = (unsigned char) (working_state[i] >> 8);
        keystream[offset + 2U] = (unsigned char) (working_state[i] >> 16);
        keystream[offset + 3U] = (unsigned char) (working_state[i] >> 24);
    }

    memset(working_state, 0, sizeof(working_state));
}

void chacha20_init_custom(chacha20_context_custom* ctx) {
    CHACHA20_VALIDATE(ctx != NULL);

    memset(ctx->state, 0, sizeof(ctx->state));
    memset(ctx->keystream8, 0, sizeof(ctx->keystream8));

    /* Initially, there's no keystream bytes available */
    ctx->keystream_bytes_used = CHACHA20_BLOCK_SIZE_BYTES;
}

void chacha20_free_custom(chacha20_context_custom* ctx) {
    if (ctx != NULL) {
        memset(ctx, 0, sizeof(chacha20_context_custom));
    }
}

int chacha20_setkey_custom(chacha20_context_custom* ctx, const unsigned char key[32]) {
    CHACHA20_VALIDATE_RET(ctx != NULL);
    CHACHA20_VALIDATE_RET(key != NULL);

    /* ChaCha20 constants - the string "expand 32-byte k" */
    ctx->state[0] = 0x61707865;
    ctx->state[1] = 0x3320646e;
    ctx->state[2] = 0x79622d32;
    ctx->state[3] = 0x6b206574;

    /* Set key */
    ctx->state[4] = BYTES_TO_U32_LE(key, 0);
    ctx->state[5] = BYTES_TO_U32_LE(key, 4);
    ctx->state[6] = BYTES_TO_U32_LE(key, 8);
    ctx->state[7] = BYTES_TO_U32_LE(key, 12);
    ctx->state[8] = BYTES_TO_U32_LE(key, 16);
    ctx->state[9] = BYTES_TO_U32_LE(key, 20);
    ctx->state[10] = BYTES_TO_U32_LE(key, 24);
    ctx->state[11] = BYTES_TO_U32_LE(key, 28);

    return (0);
}

int chacha20_starts_custom(chacha20_context_custom* ctx, const unsigned char nonce[12], uint32_t counter) {
    CHACHA20_VALIDATE_RET(ctx != NULL);
    CHACHA20_VALIDATE_RET(nonce != NULL);

    /* Counter */
    ctx->state[12] = counter;

    /* Nonce */
    ctx->state[13] = BYTES_TO_U32_LE(nonce, 0);
    ctx->state[14] = BYTES_TO_U32_LE(nonce, 4);
    ctx->state[15] = BYTES_TO_U32_LE(nonce, 8);

    memset(ctx->keystream8, 0, sizeof(ctx->keystream8));

    /* Initially, there's no keystream bytes available */
    ctx->keystream_bytes_used = CHACHA20_BLOCK_SIZE_BYTES;

    return (0);
}

int chacha20_update_custom(
        chacha20_context_custom* ctx,
        size_t size,
        const unsigned char* input,
        unsigned char* output) {
    size_t offset = 0U;
    size_t i;

    CHACHA20_VALIDATE_RET(ctx != NULL);
    CHACHA20_VALIDATE_RET(size == 0 || input != NULL);
    CHACHA20_VALIDATE_RET(size == 0 || output != NULL);

    /* Use leftover keystream bytes, if available */
    while (size > 0U && ctx->keystream_bytes_used < CHACHA20_BLOCK_SIZE_BYTES) {
        output[offset] = input[offset] ^ ctx->keystream8[ctx->keystream_bytes_used];

        ctx->keystream_bytes_used++;
        offset++;
        size--;
    }

    /* Process full blocks */
    while (size >= CHACHA20_BLOCK_SIZE_BYTES) {
        /* Generate new keystream block and increment counter */
        chacha20_block(ctx->state, ctx->keystream8);
        ctx->state[CHACHA20_CTR_INDEX]++;

        for (i = 0U; i < 64U; i += 8U) {
            output[offset + i] = input[offset + i] ^ ctx->keystream8[i];
            output[offset + i + 1] = input[offset + i + 1] ^ ctx->keystream8[i + 1];
            output[offset + i + 2] = input[offset + i + 2] ^ ctx->keystream8[i + 2];
            output[offset + i + 3] = input[offset + i + 3] ^ ctx->keystream8[i + 3];
            output[offset + i + 4] = input[offset + i + 4] ^ ctx->keystream8[i + 4];
            output[offset + i + 5] = input[offset + i + 5] ^ ctx->keystream8[i + 5];
            output[offset + i + 6] = input[offset + i + 6] ^ ctx->keystream8[i + 6];
            output[offset + i + 7] = input[offset + i + 7] ^ ctx->keystream8[i + 7];
        }

        offset += CHACHA20_BLOCK_SIZE_BYTES;
        size -= CHACHA20_BLOCK_SIZE_BYTES;
    }

    /* Last (partial) block */
    if (size > 0U) {
        /* Generate new keystream block and increment counter */
        chacha20_block(ctx->state, ctx->keystream8);
        ctx->state[CHACHA20_CTR_INDEX]++;

        for (i = 0U; i < size; i++) {
            output[offset + i] = input[offset + i] ^ ctx->keystream8[i];
        }

        ctx->keystream_bytes_used = size;
    }

    return (0);
}

int chacha20_crypt_custom(
        const unsigned char key[32],
        const unsigned char nonce[12],
        uint32_t counter,
        size_t data_len,
        const unsigned char* input,
        unsigned char* output) {
    chacha20_context_custom ctx;
    int ret;

    CHACHA20_VALIDATE_RET(key != NULL);
    CHACHA20_VALIDATE_RET(nonce != NULL);
    CHACHA20_VALIDATE_RET(data_len == 0 || input != NULL);
    CHACHA20_VALIDATE_RET(data_len == 0 || output != NULL);

    chacha20_init_custom(&ctx);

    ret = chacha20_setkey_custom(&ctx, key);
    if (ret != 0)
        goto cleanup;

    ret = chacha20_starts_custom(&ctx, nonce, counter);
    if (ret != 0)
        goto cleanup;

    ret = chacha20_update_custom(&ctx, data_len, input, output);

cleanup:
    chacha20_free_custom(&ctx);
    return (ret);
}
