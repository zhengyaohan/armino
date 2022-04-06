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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef HAP_CRYPTO_H
#define HAP_CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

uint32_t HAP_load_bigendian(const uint8_t* x);
void HAP_store_bigendian(uint8_t x[4], uint32_t u);

#define ED25519_PUBLIC_KEY_BYTES 32
#define ED25519_SECRET_KEY_BYTES 32
#define ED25519_BYTES            64

void HAP_ed25519_public_key(uint8_t pk[ED25519_PUBLIC_KEY_BYTES], const uint8_t sk[ED25519_SECRET_KEY_BYTES]);
void HAP_ed25519_sign(
        uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t sk[ED25519_SECRET_KEY_BYTES],
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES]);
int HAP_ed25519_verify(
        const uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES]);

#define X25519_SCALAR_BYTES 32
#define X25519_BYTES        32

void HAP_X25519_scalarmult_base(uint8_t r[X25519_BYTES], const uint8_t n[X25519_SCALAR_BYTES]);
void HAP_X25519_scalarmult(
        uint8_t r[X25519_BYTES],
        const uint8_t n[X25519_SCALAR_BYTES],
        const uint8_t p[X25519_BYTES]);

#define CHACHA20_POLY1305_KEY_BYTES       32
#define CHACHA20_POLY1305_NONCE_BYTES_MAX 12
#define CHACHA20_POLY1305_TAG_BYTES       16

typedef void* HAP_chacha20_poly1305_ctx;

void HAP_chacha20_poly1305_init(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
void HAP_chacha20_poly1305_update_enc(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
void HAP_chacha20_poly1305_update_enc_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
void HAP_chacha20_poly1305_final_enc(HAP_chacha20_poly1305_ctx* ctx, uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]);
void HAP_chacha20_poly1305_update_dec(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
void HAP_chacha20_poly1305_update_dec_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
int HAP_chacha20_poly1305_final_dec(HAP_chacha20_poly1305_ctx* ctx, const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]);
int HAP_chacha20_poly1305_final_dec_truncated_tag(HAP_chacha20_poly1305_ctx* ctx, const uint8_t* tag, size_t t_len);

void HAP_chacha20_poly1305_encrypt(
        uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
int HAP_chacha20_poly1305_decrypt(
        const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
int HAP_chacha20_poly1305_decrypt_truncated_tag(
        const uint8_t* tag,
        size_t t_len,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
void HAP_chacha20_poly1305_encrypt_aad(
        uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
int HAP_chacha20_poly1305_decrypt_aad(
        const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES],
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);
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
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]);

#define SRP_PRIME_BYTES                384
#define SRP_SALT_BYTES                 16
#define SRP_VERIFIER_BYTES             384
#define SRP_SECRET_KEY_BYTES           32
#define SRP_PUBLIC_KEY_BYTES           384
#define SRP_SCRAMBLING_PARAMETER_BYTES 64
#define SRP_PREMASTER_SECRET_BYTES     384
#define SRP_SESSION_KEY_BYTES          64
#define SRP_PROOF_BYTES                64

void HAP_srp_verifier(
        uint8_t v[SRP_VERIFIER_BYTES],
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t* pass,
        size_t pass_len);
void HAP_srp_public_key(
        uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]);
void HAP_srp_scrambling_parameter(
        uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES]);
int HAP_srp_premaster_secret(
        uint8_t s[SRP_PREMASTER_SECRET_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]);
void HAP_srp_session_key(uint8_t k[SRP_SESSION_KEY_BYTES], const uint8_t s[SRP_PREMASTER_SECRET_BYTES]);
void HAP_srp_proof_m1(
        uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]);
void HAP_srp_proof_m2(
        uint8_t m2[SRP_PROOF_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]);

#define SHA1_BYTES 20

void HAP_sha1(uint8_t md[SHA1_BYTES], const uint8_t* data, size_t size);

#define SHA256_BYTES 32

void HAP_sha256(uint8_t md[SHA256_BYTES], const uint8_t* data, size_t size);

#define SHA512_BYTES 64

void HAP_sha512(uint8_t md[SHA512_BYTES], const uint8_t* data, size_t size);

#define HMAC_SHA1_BYTES SHA1_BYTES

void HAP_hmac_sha1_aad(
        uint8_t r[HMAC_SHA1_BYTES],
        const uint8_t* key,
        size_t key_len,
        const uint8_t* in,
        size_t in_len,
        const uint8_t* aad,
        size_t aad_len);

void HAP_hkdf_sha512(
        uint8_t* r,
        size_t r_len,
        const uint8_t* key,
        size_t key_len,
        const uint8_t* salt,
        size_t salt_len,
        const uint8_t* info,
        size_t info_len);

void HAP_pbkdf2_hmac_sha1(
        uint8_t* key,
        size_t key_len,
        const uint8_t* password,
        size_t password_len,
        const uint8_t* salt,
        size_t salt_len,
        uint32_t count);

typedef void* HAP_aes_ctr_ctx;

#define AES128_KEY_BYTES 16

void HAP_aes_ctr_init(HAP_aes_ctr_ctx* ctx, const uint8_t* key, int size, const uint8_t iv[16]);
void HAP_aes_ctr_encrypt(HAP_aes_ctr_ctx* ctx, uint8_t* ct, const uint8_t* pt, size_t pt_len);
void HAP_aes_ctr_decrypt(HAP_aes_ctr_ctx* ctx, uint8_t* pt, const uint8_t* ct, size_t ct_len);
void HAP_aes_ctr_done(HAP_aes_ctr_ctx* ctx);

int HAP_constant_time_equal(const void* x, const void* y, size_t length);
int HAP_constant_time_is_zero(const void* x, size_t length);
void HAP_constant_time_fill_zero(void* x, size_t length);
void HAP_constant_time_copy(void* x, const void* y, size_t length);

#ifdef __cplusplus
}
#endif

#endif
