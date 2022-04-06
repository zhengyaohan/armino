// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#include <string.h>

#include "HAPBase.h"
#include "HAPCrypto.h"

uint32_t HAP_load_bigendian(const uint8_t* x) {
    uint32_t r = x[3];
    r |= (((uint32_t) x[2]) << 8);
    r |= (((uint32_t) x[1]) << 16);
    r |= (((uint32_t) x[0]) << 24);
    return r;
}

void HAP_store_bigendian(uint8_t x[4], uint32_t u) {
    x[0] = (uint8_t)(u >> 24);
    x[1] = (uint8_t)(u >> 16);
    x[2] = (uint8_t)(u >> 8);
    x[3] = (uint8_t) u;
}

int HAP_constant_time_equal(const void* x, const void* y, size_t length) {
    const uint8_t* px = (const uint8_t*) x;
    const uint8_t* py = (const uint8_t*) y;
    uint32_t a = 0;
    while (length--) {
        a |= (*px++) ^ (*py++);
    }
    return !a;
}

int HAP_constant_time_is_zero(const void* x, size_t length) {
    const uint8_t* p = (const uint8_t*) x;
    uint32_t a = 0;
    while (length--) {
        a |= *p++;
    }
    return !a;
}

void HAP_constant_time_fill_zero(void* x, size_t length) {
    HAPRawBufferZero(x, length);
}

void HAP_constant_time_copy(void* x, const void* y, size_t length) {
    HAPRawBufferCopyBytes(x, y, length);
}

// Single shot API for ChaCha20/Poly1305

#ifndef HAVE_CUSTOM_SINGLE_SHOT_CHACHA20_POLY1305

void HAP_chacha20_poly1305_encrypt_aad(
        uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    HAP_chacha20_poly1305_ctx ctx;
    HAP_chacha20_poly1305_init(&ctx, n, n_len, k);
    if (a_len > 0) {
        HAP_chacha20_poly1305_update_enc_aad(&ctx, a, a_len, n, n_len, k);
    }
    HAP_chacha20_poly1305_update_enc(&ctx, c, m, m_len, n, n_len, k);
    HAP_chacha20_poly1305_final_enc(&ctx, tag);
}

int HAP_chacha20_poly1305_decrypt_aad(
        const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    HAP_chacha20_poly1305_ctx ctx;
    HAP_chacha20_poly1305_init(&ctx, n, n_len, k);
    if (a_len > 0) {
        HAP_chacha20_poly1305_update_dec_aad(&ctx, a, a_len, n, n_len, k);
    }
    HAP_chacha20_poly1305_update_dec(&ctx, m, c, c_len, n, n_len, k);
    return HAP_chacha20_poly1305_final_dec(&ctx, tag);
}

int HAP_chacha20_poly1305_decrypt_aad_truncated_tag(
        const uint8_t* tag,
        size_t t_len,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    HAP_chacha20_poly1305_ctx ctx;
    HAP_chacha20_poly1305_init(&ctx, n, n_len, k);
    if (a_len > 0) {
        HAP_chacha20_poly1305_update_dec_aad(&ctx, a, a_len, n, n_len, k);
    }
    HAP_chacha20_poly1305_update_dec(&ctx, m, c, c_len, n, n_len, k);
    return HAP_chacha20_poly1305_final_dec_truncated_tag(&ctx, tag, t_len);
}

#endif

void HAP_chacha20_poly1305_encrypt(
        uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    HAP_chacha20_poly1305_encrypt_aad(tag, c, m, m_len, NULL, 0, n, n_len, k);
}

int HAP_chacha20_poly1305_decrypt(
        const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    return HAP_chacha20_poly1305_decrypt_aad(tag, m, c, c_len, NULL, 0, n, n_len, k);
}

int HAP_chacha20_poly1305_decrypt_truncated_tag(
        const uint8_t* tag,
        size_t t_len,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    return HAP_chacha20_poly1305_decrypt_aad_truncated_tag(tag, t_len, m, c, c_len, NULL, 0, n, n_len, k);
}
