/**
 * \file chachapoly.c
 *
 * \brief ChaCha20-Poly1305 AEAD construction based on RFC 7539.
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

#include "chachapoly.h"
#include <string.h>

#define INTERNAL_VALIDATE_RET(cond, ret) \
    do { \
    } while (0)
#define INTERNAL_VALIDATE(cond) \
    do { \
    } while (0)

/* Parameter validation macros */
#define CHACHAPOLY_VALIDATE_RET(cond) INTERNAL_VALIDATE_RET(cond, ERR_POLY1305_BAD_INPUT_DATA)
#define CHACHAPOLY_VALIDATE(cond)     INTERNAL_VALIDATE(cond)

#define CHACHAPOLY_STATE_INIT       (0)
#define CHACHAPOLY_STATE_AAD        (1)
#define CHACHAPOLY_STATE_CIPHERTEXT (2) /* Encrypting or decrypting */
#define CHACHAPOLY_STATE_FINISHED   (3)

/**
 * \brief           Adds nul bytes to pad the AAD for Poly1305.
 *
 * \param ctx       The ChaCha20-Poly1305 context.
 */
static int chachapoly_pad_aad(chachapoly_context_custom* ctx) {
    uint32_t partial_block_len = (uint32_t)(ctx->aad_len % 16U);
    unsigned char zeroes[15];

    if (partial_block_len == 0U)
        return (0);

    memset(zeroes, 0, sizeof(zeroes));

    return (poly1305_update_custom(&ctx->poly1305_ctx, zeroes, 16U - partial_block_len));
}

/**
 * \brief           Adds nul bytes to pad the ciphertext for Poly1305.
 *
 * \param ctx       The ChaCha20-Poly1305 context.
 */
static int chachapoly_pad_ciphertext(chachapoly_context_custom* ctx) {
    uint32_t partial_block_len = (uint32_t)(ctx->ciphertext_len % 16U);
    unsigned char zeroes[15];

    if (partial_block_len == 0U)
        return (0);

    memset(zeroes, 0, sizeof(zeroes));
    return (poly1305_update_custom(&ctx->poly1305_ctx, zeroes, 16U - partial_block_len));
}

void chachapoly_init_custom(chachapoly_context_custom* ctx) {
    CHACHAPOLY_VALIDATE(ctx != NULL);

    chacha20_init_custom(&ctx->chacha20_ctx);
    poly1305_init_custom(&ctx->poly1305_ctx);
    ctx->aad_len = 0U;
    ctx->ciphertext_len = 0U;
    ctx->state = CHACHAPOLY_STATE_INIT;
    ctx->mode = CHACHAPOLY_ENCRYPT;
}

void chachapoly_free_custom(chachapoly_context_custom* ctx) {
    if (ctx == NULL)
        return;

    chacha20_free_custom(&ctx->chacha20_ctx);
    poly1305_free_custom(&ctx->poly1305_ctx);
    ctx->aad_len = 0U;
    ctx->ciphertext_len = 0U;
    ctx->state = CHACHAPOLY_STATE_INIT;
    ctx->mode = CHACHAPOLY_ENCRYPT;
}

int chachapoly_setkey_custom(chachapoly_context_custom* ctx, const unsigned char key[32]) {
    int ret;
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(key != NULL);

    ret = chacha20_setkey_custom(&ctx->chacha20_ctx, key);

    return (ret);
}

int chachapoly_starts_custom(
        chachapoly_context_custom* ctx,
        const unsigned char nonce[12],
        chachapoly_mode_custom_t mode) {
    int ret;
    unsigned char poly1305_key[64];
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(nonce != NULL);

    /* Set counter = 0, will be update to 1 when generating Poly1305 key */
    ret = chacha20_starts_custom(&ctx->chacha20_ctx, nonce, 0U);
    if (ret != 0)
        goto cleanup;

    /* Generate the Poly1305 key by getting the ChaCha20 keystream output with
     * counter = 0.  This is the same as encrypting a buffer of zeroes.
     * Only the first 256-bits (32 bytes) of the key is used for Poly1305.
     * The other 256 bits are discarded.
     */
    memset(poly1305_key, 0, sizeof(poly1305_key));
    ret = chacha20_update_custom(&ctx->chacha20_ctx, sizeof(poly1305_key), poly1305_key, poly1305_key);
    if (ret != 0)
        goto cleanup;

    ret = poly1305_starts_custom(&ctx->poly1305_ctx, poly1305_key);

    if (ret == 0) {
        ctx->aad_len = 0U;
        ctx->ciphertext_len = 0U;
        ctx->state = CHACHAPOLY_STATE_AAD;
        ctx->mode = mode;
    }

cleanup:
    memset(poly1305_key, 0, 64U);
    return (ret);
}

int chachapoly_update_aad_custom(chachapoly_context_custom* ctx, const unsigned char* aad, size_t aad_len) {
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(aad_len == 0 || aad != NULL);

    if (ctx->state != CHACHAPOLY_STATE_AAD)
        return (ERR_CHACHAPOLY_BAD_STATE);

    ctx->aad_len += aad_len;

    return (poly1305_update_custom(&ctx->poly1305_ctx, aad, aad_len));
}

int chachapoly_update_custom(
        chachapoly_context_custom* ctx,
        size_t len,
        const unsigned char* input,
        unsigned char* output) {
    int ret;
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(len == 0 || input != NULL);
    CHACHAPOLY_VALIDATE_RET(len == 0 || output != NULL);

    if ((ctx->state != CHACHAPOLY_STATE_AAD) && (ctx->state != CHACHAPOLY_STATE_CIPHERTEXT)) {
        return (ERR_CHACHAPOLY_BAD_STATE);
    }

    if (ctx->state == CHACHAPOLY_STATE_AAD) {
        ctx->state = CHACHAPOLY_STATE_CIPHERTEXT;

        ret = chachapoly_pad_aad(ctx);
        if (ret != 0)
            return (ret);
    }

    ctx->ciphertext_len += len;

    if (ctx->mode == CHACHAPOLY_ENCRYPT) {
        ret = chacha20_update_custom(&ctx->chacha20_ctx, len, input, output);
        if (ret != 0)
            return (ret);

        ret = poly1305_update_custom(&ctx->poly1305_ctx, output, len);
        if (ret != 0)
            return (ret);
    } else /* DECRYPT */
    {
        ret = poly1305_update_custom(&ctx->poly1305_ctx, input, len);
        if (ret != 0)
            return (ret);

        ret = chacha20_update_custom(&ctx->chacha20_ctx, len, input, output);
        if (ret != 0)
            return (ret);
    }

    return (0);
}

int chachapoly_finish_custom(chachapoly_context_custom* ctx, unsigned char mac[16]) {
    int ret;
    unsigned char len_block[16];
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(mac != NULL);

    if (ctx->state == CHACHAPOLY_STATE_INIT) {
        return (ERR_CHACHAPOLY_BAD_STATE);
    }

    if (ctx->state == CHACHAPOLY_STATE_AAD) {
        ret = chachapoly_pad_aad(ctx);
        if (ret != 0)
            return (ret);
    } else if (ctx->state == CHACHAPOLY_STATE_CIPHERTEXT) {
        ret = chachapoly_pad_ciphertext(ctx);
        if (ret != 0)
            return (ret);
    }

    ctx->state = CHACHAPOLY_STATE_FINISHED;

    /* The lengths of the AAD and ciphertext are processed by
     * Poly1305 as the final 128-bit block, encoded as little-endian integers.
     */
    len_block[0] = (unsigned char) (ctx->aad_len);
    len_block[1] = (unsigned char) (ctx->aad_len >> 8);
    len_block[2] = (unsigned char) (ctx->aad_len >> 16);
    len_block[3] = (unsigned char) (ctx->aad_len >> 24);
    len_block[4] = (unsigned char) (ctx->aad_len >> 32);
    len_block[5] = (unsigned char) (ctx->aad_len >> 40);
    len_block[6] = (unsigned char) (ctx->aad_len >> 48);
    len_block[7] = (unsigned char) (ctx->aad_len >> 56);
    len_block[8] = (unsigned char) (ctx->ciphertext_len);
    len_block[9] = (unsigned char) (ctx->ciphertext_len >> 8);
    len_block[10] = (unsigned char) (ctx->ciphertext_len >> 16);
    len_block[11] = (unsigned char) (ctx->ciphertext_len >> 24);
    len_block[12] = (unsigned char) (ctx->ciphertext_len >> 32);
    len_block[13] = (unsigned char) (ctx->ciphertext_len >> 40);
    len_block[14] = (unsigned char) (ctx->ciphertext_len >> 48);
    len_block[15] = (unsigned char) (ctx->ciphertext_len >> 56);

    ret = poly1305_update_custom(&ctx->poly1305_ctx, len_block, 16U);
    if (ret != 0)
        return (ret);

    ret = poly1305_finish_custom(&ctx->poly1305_ctx, mac);

    return (ret);
}

static int chachapoly_crypt_and_tag(
        chachapoly_context_custom* ctx,
        chachapoly_mode_custom_t mode,
        size_t length,
        const unsigned char nonce[12],
        const unsigned char* aad,
        size_t aad_len,
        const unsigned char* input,
        unsigned char* output,
        unsigned char tag[16]) {
    int ret;

    ret = chachapoly_starts_custom(ctx, nonce, mode);
    if (ret != 0)
        goto cleanup;

    ret = chachapoly_update_aad_custom(ctx, aad, aad_len);
    if (ret != 0)
        goto cleanup;

    ret = chachapoly_update_custom(ctx, length, input, output);
    if (ret != 0)
        goto cleanup;

    ret = chachapoly_finish_custom(ctx, tag);

cleanup:
    return (ret);
}

int chachapoly_encrypt_and_tag_custom(
        chachapoly_context_custom* ctx,
        size_t length,
        const unsigned char nonce[12],
        const unsigned char* aad,
        size_t aad_len,
        const unsigned char* input,
        unsigned char* output,
        unsigned char tag[16]) {
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(nonce != NULL);
    CHACHAPOLY_VALIDATE_RET(tag != NULL);
    CHACHAPOLY_VALIDATE_RET(aad_len == 0 || aad != NULL);
    CHACHAPOLY_VALIDATE_RET(length == 0 || input != NULL);
    CHACHAPOLY_VALIDATE_RET(length == 0 || output != NULL);

    return (chachapoly_crypt_and_tag(ctx, CHACHAPOLY_ENCRYPT, length, nonce, aad, aad_len, input, output, tag));
}

int chachapoly_auth_decrypt_custom(
        chachapoly_context_custom* ctx,
        size_t length,
        const unsigned char nonce[12],
        const unsigned char* aad,
        size_t aad_len,
        const unsigned char tag[16],
        const unsigned char* input,
        unsigned char* output) {
    int ret;
    unsigned char check_tag[16];
    size_t i;
    int diff;
    CHACHAPOLY_VALIDATE_RET(ctx != NULL);
    CHACHAPOLY_VALIDATE_RET(nonce != NULL);
    CHACHAPOLY_VALIDATE_RET(tag != NULL);
    CHACHAPOLY_VALIDATE_RET(aad_len == 0 || aad != NULL);
    CHACHAPOLY_VALIDATE_RET(length == 0 || input != NULL);
    CHACHAPOLY_VALIDATE_RET(length == 0 || output != NULL);

    if ((ret = chachapoly_crypt_and_tag(
                 ctx, CHACHAPOLY_DECRYPT, length, nonce, aad, aad_len, input, output, check_tag)) != 0) {
        return (ret);
    }

    /* Check tag in "constant-time" */
    for (diff = 0, i = 0; i < sizeof(check_tag); i++)
        diff |= tag[i] ^ check_tag[i];

    if (diff != 0) {
        memset(output, 0, length);
        return (ERR_CHACHAPOLY_AUTH_FAILED);
    }

    return (0);
}
