/**
 * \file poly1305.h
 *
 * \brief   This file contains Poly1305 definitions and functions.
 *
 *          Poly1305 is a one-time message authenticator that can be used to
 *          authenticate messages. Poly1305-AES was created by Daniel
 *          Bernstein https://cr.yp.to/mac/poly1305-20050329.pdf The generic
 *          Poly1305 algorithm (not tied to AES) was also standardized in RFC
 *          7539.
 *
 * \author Daniel King <damaki.gh@gmail.com>
 */

/*  Copyright (C) 2006-2018, Arm Limited (or its affiliates), All Rights Reserved.
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
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

#ifndef POLY1305_H
#define POLY1305_H

#include <stddef.h>
#include <stdint.h>

#define ERR_POLY1305_BAD_INPUT_DATA -0x0057 /**< Invalid input parameter(s). */

/* ERR_POLY1305_FEATURE_UNAVAILABLE is deprecated and should not be
 * used. */
#define ERR_POLY1305_FEATURE_UNAVAILABLE \
    -0x0059 /**< Feature not available. For example, s part of the API is not implemented. */

/* ERR_POLY1305_HW_ACCEL_FAILED is deprecated and should not be used.
 */
#define ERR_POLY1305_HW_ACCEL_FAILED -0x005B /**< Poly1305 hardware accelerator failed. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct poly1305_context_custom {
    uint32_t r[4];     /** The value for 'r' (low 128 bits of the key). */
    uint32_t s[4];     /** The value for 's' (high 128 bits of the key). */
    uint32_t acc[5];   /** The accumulator number. */
    uint8_t queue[16]; /** The current partial block of data. */
    size_t queue_len;  /** The number of bytes stored in 'queue'. */
} poly1305_context_custom;

/**
 * \brief           This function initializes the specified Poly1305 context.
 *
 *                  It must be the first API called before using
 *                  the context.
 *
 *                  It is usually followed by a call to
 *                  \c poly1305_starts_custom(), then one or more calls to
 *                  \c poly1305_update_custom(), then one call to
 *                  \c poly1305_finish_custom(), then finally
 *                  \c poly1305_free_custom().
 *
 * \param ctx       The Poly1305 context to initialize. This must
 *                  not be \c NULL.
 */
void poly1305_init_custom(poly1305_context_custom* ctx);

/**
 * \brief           This function releases and clears the specified
 *                  Poly1305 context.
 *
 * \param ctx       The Poly1305 context to clear. This may be \c NULL, in which
 *                  case this function is a no-op. If it is not \c NULL, it must
 *                  point to an initialized Poly1305 context.
 */
void poly1305_free_custom(poly1305_context_custom* ctx);

/**
 * \brief           This function sets the one-time authentication key.
 *
 * \warning         The key must be unique and unpredictable for each
 *                  invocation of Poly1305.
 *
 * \param ctx       The Poly1305 context to which the key should be bound.
 *                  This must be initialized.
 * \param key       The buffer containing the \c 32 Byte (\c 256 Bit) key.
 *
 * \return          \c 0 on success.
 * \return          A negative error code on failure.
 */
int poly1305_starts_custom(poly1305_context_custom* ctx, const unsigned char key[32]);

/**
 * \brief           This functions feeds an input buffer into an ongoing
 *                  Poly1305 computation.
 *
 *                  It is called between \c poly1305_starts_custom() and
 *                  \c poly1305_finish_custom().
 *                  It can be called repeatedly to process a stream of data.
 *
 * \param ctx       The Poly1305 context to use for the Poly1305 operation.
 *                  This must be initialized and bound to a key.
 * \param ilen      The length of the input data in Bytes.
 *                  Any value is accepted.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if `ilen == 0`.
 *
 * \return          \c 0 on success.
 * \return          A negative error code on failure.
 */
int poly1305_update_custom(poly1305_context_custom* ctx, const unsigned char* input, size_t ilen);

/**
 * \brief           This function generates the Poly1305 Message
 *                  Authentication Code (MAC).
 *
 * \param ctx       The Poly1305 context to use for the Poly1305 operation.
 *                  This must be initialized and bound to a key.
 * \param mac       The buffer to where the MAC is written. This must
 *                  be a writable buffer of length \c 16 Bytes.
 *
 * \return          \c 0 on success.
 * \return          A negative error code on failure.
 */
int poly1305_finish_custom(poly1305_context_custom* ctx, unsigned char mac[16]);

/**
 * \brief           This function calculates the Poly1305 MAC of the input
 *                  buffer with the provided key.
 *
 * \warning         The key must be unique and unpredictable for each
 *                  invocation of Poly1305.
 *
 * \param key       The buffer containing the \c 32 Byte (\c 256 Bit) key.
 * \param ilen      The length of the input data in Bytes.
 *                  Any value is accepted.
 * \param input     The buffer holding the input data.
 *                  This pointer can be \c NULL if `ilen == 0`.
 * \param mac       The buffer to where the MAC is written. This must be
 *                  a writable buffer of length \c 16 Bytes.
 *
 * \return          \c 0 on success.
 * \return          A negative error code on failure.
 */
int poly1305_mac_custom(const unsigned char key[32], const unsigned char* input, size_t ilen, unsigned char mac[16]);

#ifdef __cplusplus
}
#endif

#endif /* POLY1305_H */
