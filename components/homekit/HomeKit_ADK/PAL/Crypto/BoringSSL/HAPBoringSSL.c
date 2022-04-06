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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "HAPCrypto.h"

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/hkdf.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stdlib.h>
#include <string.h>

#define HAP_CRYPTO_BORINGSSL

#ifndef HAVE_CUSTOM_CHACHA20POLY1305
#include "ChaCha20Poly1305/chacha20.c"
#include "ChaCha20Poly1305/chachapoly.c"
#include "ChaCha20Poly1305/poly1305.c"
#endif

#if !defined(BORINGSSL_API_VERSION) || (BORINGSSL_API_VERSION < 10)
#define BORINGSSL_API_VERSION_LESS_THAN_TEN 1
#else
#define BORINGSSL_API_VERSION_LESS_THAN_TEN 0
#endif

#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 1)
#include <openssl/curve25519.h>
#endif

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // Embedding a directive within macro arguments
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wembedded-directive")

static void hash_init(EVP_MD_CTX** ctx, const EVP_MD* type) {
    *ctx = EVP_MD_CTX_create();
    int ret = EVP_DigestInit_ex(*ctx, type, NULL);
    HAPAssert(ret == 1);
}

static void hash_update(EVP_MD_CTX** ctx, const uint8_t* data, size_t size) {
    int ret = EVP_DigestUpdate(*ctx, data, size);
    HAPAssert(ret == 1);
}

static void hash_final(EVP_MD_CTX** ctx, uint8_t* md) {
    int ret = EVP_DigestFinal_ex(*ctx, md, NULL);
    HAPAssert(ret == 1);
    EVP_MD_CTX_destroy(*ctx);
    *ctx = NULL;
}

#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 1)

#ifndef HAVE_CUSTOM_ED25519
#include "Ed25519/curve25519_mehdi.c"
#include "Ed25519/curve25519_order.c"
#include "Ed25519/curve25519_utils.c"
#include "Ed25519/ed25519_sign.c"
#include "Ed25519/ed25519_verify.c"
#endif

#define USE_AND_CLEAR(name, size, X) \
    do { \
        uint8_t name[size]; \
        X; \
        memset(name, 0, sizeof name); \
    } while (0)

#define WITH_BLINDING(X) \
    do { \
        uint8_t seed[64]; \
        HAPPlatformRandomNumberFill(seed, sizeof seed); \
        EDP_BLINDING_CTX ctx; \
        ed25519_Blinding_Init(&ctx, seed, sizeof seed); \
        X; \
        ed25519_Blinding_Finish(&ctx); \
    } while (0)

#endif // BORINGSSL_API_VERSION < 10

#define WITH(type, name, init, free, X) \
    do { \
        type* name = init; \
        X; \
        free(name); \
    } while (0)

#define WITH_PKEY(name, init, X) WITH(EVP_PKEY, name, init, EVP_PKEY_free, X)
#define WITH_CTX(type, init, X)  WITH(type, ctx, init, type##_free, X)

void HAP_ed25519_public_key(uint8_t pk[ED25519_PUBLIC_KEY_BYTES], const uint8_t sk[ED25519_SECRET_KEY_BYTES]) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, sk, ED25519_SECRET_KEY_BYTES), {
        size_t len = ED25519_PUBLIC_KEY_BYTES;
        int ret = EVP_PKEY_get_raw_public_key(key, pk, &len);
        HAPAssert(ret == 1 && len == ED25519_SECRET_KEY_BYTES);
    });
#else
    WITH_BLINDING(
            { USE_AND_CLEAR(privKey, ed25519_private_key_size, { ed25519_CreateKeyPair(pk, privKey, &ctx, sk); }); });
    return;
#endif
}

void HAP_ed25519_sign(
        uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t sk[ED25519_SECRET_KEY_BYTES],
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES] HAP_UNUSED) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, sk, ED25519_SECRET_KEY_BYTES), {
        WITH_CTX(EVP_MD_CTX, EVP_MD_CTX_new(), {
            int ret = EVP_DigestSignInit(ctx, NULL, NULL, NULL, key);
            HAPAssert(ret == 1);
            size_t len = ED25519_BYTES;
            EVP_DigestSign(ctx, sig, &len, m, m_len);
            HAPAssert(len == ED25519_BYTES);
        });
    });
#else
    WITH_BLINDING({
        USE_AND_CLEAR(privKey, ed25519_private_key_size, {
            memcpy(privKey, sk, ED25519_SECRET_KEY_BYTES);
            memcpy(privKey + ED25519_SECRET_KEY_BYTES, pk, ED25519_PUBLIC_KEY_BYTES);
            ed25519_SignMessage(sig, privKey, &ctx, m, m_len);
        });
    });
#endif
}

int HAP_ed25519_verify(
        const uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES]) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    int ret;
    WITH_PKEY(key, EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, pk, ED25519_PUBLIC_KEY_BYTES), {
        WITH_CTX(EVP_MD_CTX, EVP_MD_CTX_new(), {
            ret = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, key);
            HAPAssert(ret == 1);
            ret = EVP_DigestVerify(ctx, sig, ED25519_BYTES, m, m_len);
        });
    });
    return (ret == 1) ? 0 : -1;
#else
    int ret = ed25519_VerifySignature(sig, pk, m, m_len);
    return (ret == 1) ? 0 : -1;
#endif
}

void HAP_X25519_scalarmult_base(uint8_t r[X25519_BYTES], const uint8_t n[X25519_SCALAR_BYTES]) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, n, X25519_SCALAR_BYTES), {
        size_t len = X25519_BYTES;
        int ret = EVP_PKEY_get_raw_public_key(key, r, &len);
        HAPAssert(ret == 1 && len == X25519_BYTES);
    });
#else
    X25519_public_from_private(r, n);
    return;
#endif
}

void HAP_X25519_scalarmult(
        uint8_t r[X25519_BYTES],
        const uint8_t n[X25519_SCALAR_BYTES],
        const uint8_t p[X25519_BYTES]) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    WITH_PKEY(pkey, EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, n, X25519_SCALAR_BYTES), {
        WITH_PKEY(peer, EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL, p, X25519_BYTES), {
            WITH_CTX(EVP_PKEY_CTX, EVP_PKEY_CTX_new(pkey, NULL), {
                int ret = EVP_PKEY_derive_init(ctx);
                HAPAssert(ret == 1);
                ret = EVP_PKEY_derive_set_peer(ctx, peer);
                HAPAssert(ret == 1);
                size_t r_len;
                ret = EVP_PKEY_derive(ctx, r, &r_len);
                HAPAssert(ret == 1 && r_len == X25519_BYTES);
            });
        });
    });
#else
    int ret = X25519(r, n, p);
    HAPAssert(ret == 1);
    return;
#endif
}

// BoringSSL doesn't support SRP6a with SHA512 so we have to do this on foot

// SRP 3072-bit prime number
static const uint8_t N_3072[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9, 0x0f, 0xda, 0xa2, 0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6, 0x62,
    0x8b, 0x80, 0xdc, 0x1c, 0xd1, 0x29, 0x02, 0x4e, 0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6, 0x3b, 0x13,
    0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e, 0x34, 0x04, 0xdd, 0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a, 0x43, 0x1b, 0x30,
    0x2b, 0x0a, 0x6d, 0xf2, 0x5f, 0x14, 0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45, 0xe4, 0x85, 0xb5, 0x76,
    0x62, 0x5e, 0x7e, 0xc6, 0xf4, 0x4c, 0x42, 0xe9, 0xa6, 0x37, 0xed, 0x6b, 0x0b, 0xff, 0x5c, 0xb6, 0xf4, 0x06, 0xb7,
    0xed, 0xee, 0x38, 0x6b, 0xfb, 0x5a, 0x89, 0x9f, 0xa5, 0xae, 0x9f, 0x24, 0x11, 0x7c, 0x4b, 0x1f, 0xe6, 0x49, 0x28,
    0x66, 0x51, 0xec, 0xe4, 0x5b, 0x3d, 0xc2, 0x00, 0x7c, 0xb8, 0xa1, 0x63, 0xbf, 0x05, 0x98, 0xda, 0x48, 0x36, 0x1c,
    0x55, 0xd3, 0x9a, 0x69, 0x16, 0x3f, 0xa8, 0xfd, 0x24, 0xcf, 0x5f, 0x83, 0x65, 0x5d, 0x23, 0xdc, 0xa3, 0xad, 0x96,
    0x1c, 0x62, 0xf3, 0x56, 0x20, 0x85, 0x52, 0xbb, 0x9e, 0xd5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6d, 0x67, 0x0c, 0x35,
    0x4e, 0x4a, 0xbc, 0x98, 0x04, 0xf1, 0x74, 0x6c, 0x08, 0xca, 0x18, 0x21, 0x7c, 0x32, 0x90, 0x5e, 0x46, 0x2e, 0x36,
    0xce, 0x3b, 0xe3, 0x9e, 0x77, 0x2c, 0x18, 0x0e, 0x86, 0x03, 0x9b, 0x27, 0x83, 0xa2, 0xec, 0x07, 0xa2, 0x8f, 0xb5,
    0xc5, 0x5d, 0xf0, 0x6f, 0x4c, 0x52, 0xc9, 0xde, 0x2b, 0xcb, 0xf6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7c,
    0xea, 0x95, 0x6a, 0xe5, 0x15, 0xd2, 0x26, 0x18, 0x98, 0xfa, 0x05, 0x10, 0x15, 0x72, 0x8e, 0x5a, 0x8a, 0xaa, 0xc4,
    0x2d, 0xad, 0x33, 0x17, 0x0d, 0x04, 0x50, 0x7a, 0x33, 0xa8, 0x55, 0x21, 0xab, 0xdf, 0x1c, 0xba, 0x64, 0xec, 0xfb,
    0x85, 0x04, 0x58, 0xdb, 0xef, 0x0a, 0x8a, 0xea, 0x71, 0x57, 0x5d, 0x06, 0x0c, 0x7d, 0xb3, 0x97, 0x0f, 0x85, 0xa6,
    0xe1, 0xe4, 0xc7, 0xab, 0xf5, 0xae, 0x8c, 0xdb, 0x09, 0x33, 0xd7, 0x1e, 0x8c, 0x94, 0xe0, 0x4a, 0x25, 0x61, 0x9d,
    0xce, 0xe3, 0xd2, 0x26, 0x1a, 0xd2, 0xee, 0x6b, 0xf1, 0x2f, 0xfa, 0x06, 0xd9, 0x8a, 0x08, 0x64, 0xd8, 0x76, 0x02,
    0x73, 0x3e, 0xc8, 0x6a, 0x64, 0x52, 0x1f, 0x2b, 0x18, 0x17, 0x7b, 0x20, 0x0c, 0xbb, 0xe1, 0x17, 0x57, 0x7a, 0x61,
    0x5d, 0x6c, 0x77, 0x09, 0x88, 0xc0, 0xba, 0xd9, 0x46, 0xe2, 0x08, 0xe2, 0x4f, 0xa0, 0x74, 0xe5, 0xab, 0x31, 0x43,
    0xdb, 0x5b, 0xfc, 0xe0, 0xfd, 0x10, 0x8e, 0x4b, 0x82, 0xd1, 0x20, 0xa9, 0x3a, 0xd2, 0xca, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
};

static BN_ULONG bn_generator_5_value[] = { 5 };
static BIGNUM bn_generator_5 = { bn_generator_5_value, 1, 1, 0, BN_FLG_STATIC_DATA };

/*
 * Internal structure storing N and g pair
 */
typedef struct SRP_gN_st {
    char* id;
    const BIGNUM* g;
    const BIGNUM* N;
} SRP_gN;

static SRP_gN knowngN = { .id = "3072", .g = &bn_generator_5, .N = NULL };

static SRP_gN* Get_gN_3072(BIGNUM* N) {
    HAPAssert(N != NULL);
    BN_bin2bn(N_3072, sizeof N_3072, N);
    knowngN.N = N;
    return &knowngN;
}

BIGNUM* SRP_Calc_server_key(BIGNUM* A, BIGNUM* v, BIGNUM* u, BIGNUM* b, const BIGNUM* N) {
    BIGNUM *tmp = NULL, *S = NULL;
    BN_CTX* bn_ctx;

    if (u == NULL || A == NULL || v == NULL || b == NULL || N == NULL)
        return NULL;

    if ((bn_ctx = BN_CTX_new()) == NULL || (tmp = BN_new()) == NULL || (S = BN_new()) == NULL)
        goto err;

    /* S = (A*v**u) ** b */

    if (!BN_mod_exp(tmp, v, u, N, bn_ctx))
        goto err;
    if (!BN_mod_mul(tmp, A, tmp, N, bn_ctx))
        goto err;
    if (!BN_mod_exp(S, tmp, b, N, bn_ctx))
        goto err;
err:
    BN_CTX_free(bn_ctx);
    BN_clear_free(tmp);
    return S;
}

static void
        Calc_x(uint8_t x[SHA512_BYTES],
               const uint8_t salt[SRP_SALT_BYTES],
               const uint8_t* user,
               size_t user_len,
               const uint8_t* pass,
               size_t pass_len) {
    EVP_MD_CTX* ctx;

    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, user, user_len);
    hash_update(&ctx, (const uint8_t*) ":", 1);
    hash_update(&ctx, pass, pass_len);
    hash_final(&ctx, x);

    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, salt, SRP_SALT_BYTES);
    hash_update(&ctx, x, SHA512_BYTES);
    hash_final(&ctx, x);
}

#define WITH_BN(name, init, X) WITH(BIGNUM, name, init, BN_clear_free, X)

void HAP_srp_verifier(
        uint8_t v[SRP_VERIFIER_BYTES],
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t* pass,
        size_t pass_len) {
    uint8_t h[SHA512_BYTES];
    Calc_x(h, salt, user, user_len, pass, pass_len);
    WITH_BN(x, BN_bin2bn(h, sizeof h, NULL), {
        WITH_BN(verifier, BN_new(), {
            WITH_BN(N, BN_new(), {
                SRP_gN* gN = Get_gN_3072(N);
                WITH_CTX(BN_CTX, BN_CTX_new(), {
                    int ret = BN_mod_exp(verifier, gN->g, x, gN->N, ctx);
                    HAPAssert(!!ret);
                });
            });
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
            int ret = BN_bn2binpad(verifier, v, SRP_VERIFIER_BYTES);
            HAPAssert(ret == SRP_VERIFIER_BYTES);
#else
            int ret = BN_bn2bin_padded(v, SRP_VERIFIER_BYTES, verifier);
            HAPAssert(ret == 1);
#endif
        });
    });
}

static BIGNUM* Calc_k(SRP_gN* gN) {
    uint8_t N[SRP_PRIME_BYTES];
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    int ret = BN_bn2binpad(gN->N, N, sizeof N);
    HAPAssert(ret == sizeof N);
#else
    int ret = BN_bn2bin_padded(N, sizeof N, gN->N);
    HAPAssert(ret == 1);
#endif
    uint8_t g[SRP_PRIME_BYTES];

#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    ret = BN_bn2binpad(gN->g, g, sizeof g);
    HAPAssert(ret == sizeof g);
#else
    ret = BN_bn2bin_padded(g, sizeof g, gN->g);
    HAPAssert(ret == 1);
#endif
    uint8_t k[SHA512_BYTES];
    EVP_MD_CTX* ctx;
    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, N, SRP_PRIME_BYTES);
    hash_update(&ctx, g, SRP_PRIME_BYTES);
    hash_final(&ctx, k);
    return BN_bin2bn(k, sizeof k, NULL);
}

static BIGNUM* Calc_B(BIGNUM* b, BIGNUM* v) {
    BIGNUM* B = BN_new();
    WITH_BN(N, BN_new(), {
        SRP_gN* gN = Get_gN_3072(N);
        WITH_CTX(BN_CTX, BN_CTX_new(), {
            WITH_BN(gb, BN_new(), {
                int ret = BN_mod_exp(gb, gN->g, b, gN->N, ctx);
                HAPAssert(!!ret);
                WITH_BN(k, Calc_k(gN), {
                    WITH_BN(kv, BN_new(), {
                        ret = BN_mod_mul(kv, v, k, gN->N, ctx);
                        HAPAssert(!!ret);
                        ret = BN_mod_add(B, gb, kv, gN->N, ctx);
                        HAPAssert(!!ret);
                    });
                });
            });
        });
    });
    return B;
}

void HAP_srp_public_key(
        uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]) {
    WITH_BN(b, BN_bin2bn(priv_b, SRP_SECRET_KEY_BYTES, NULL), {
        WITH_BN(verifier, BN_bin2bn(v, SRP_VERIFIER_BYTES, NULL), {
            WITH_BN(B, Calc_B(b, verifier), {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
                int ret = BN_bn2binpad(B, pub_b, SRP_PUBLIC_KEY_BYTES);
                HAPAssert(ret == SRP_PUBLIC_KEY_BYTES);
#else
                int ret = BN_bn2bin_padded(pub_b, SRP_PUBLIC_KEY_BYTES, B);
                HAPAssert(ret == 1);
#endif
            });
        });
    });
}

void HAP_srp_scrambling_parameter(
        uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES]) {
    EVP_MD_CTX* ctx;
    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, pub_a, SRP_PUBLIC_KEY_BYTES);
    hash_update(&ctx, pub_b, SRP_PUBLIC_KEY_BYTES);
    hash_final(&ctx, u);
}

int HAP_srp_premaster_secret(
        uint8_t s[SRP_PREMASTER_SECRET_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]) {
    bool isAValid = false;
    WITH_BN(A, BN_bin2bn(pub_a, SRP_PUBLIC_KEY_BYTES, NULL), {
        // Refer RFC 5054: https://tools.ietf.org/html/rfc5054
        // Section 2.5.4
        // Fail if A%N == 0
        WITH_CTX(BN_CTX, BN_CTX_new(), {
            WITH_BN(rem, BN_new(), {
                WITH_BN(N, BN_new(), {
                    int ret = BN_nnmod(rem, A, Get_gN_3072(N)->N, ctx);
                    HAPAssert(!!ret);
                    if (BN_is_zero(rem) == 0) {
                        isAValid = true;
                    }
                });
            });
        });

        WITH_BN(b, BN_bin2bn(priv_b, SRP_SECRET_KEY_BYTES, NULL), {
            WITH_BN(u_, BN_bin2bn(u, SRP_SCRAMBLING_PARAMETER_BYTES, NULL), {
                WITH_BN(v_, BN_bin2bn(v, SRP_VERIFIER_BYTES, NULL), {
                    WITH_BN(N, BN_new(), {
                        WITH_BN(s_, SRP_Calc_server_key(A, v_, u_, b, Get_gN_3072(N)->N), {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
                            int ret = BN_bn2binpad(s_, s, SRP_PREMASTER_SECRET_BYTES);
                            HAPAssert(ret == SRP_PREMASTER_SECRET_BYTES);
#else
                            int ret = BN_bn2bin_padded(s, SRP_PREMASTER_SECRET_BYTES, s_);
                            HAPAssert(ret == 1);
#endif
                        });
                    });
                });
            });
        });
    });
    return (isAValid) ? 0 : 1;
}

static size_t Count_Leading_Zeroes(const uint8_t* start, size_t n) {
    const uint8_t* p = start;
    const uint8_t* stop = start + n;
    while (p < stop && !*p) {
        p++;
    }
    return p - start;
}

void HAP_srp_session_key(uint8_t k[SRP_SESSION_KEY_BYTES], const uint8_t s[SRP_PREMASTER_SECRET_BYTES]) {
    size_t z = Count_Leading_Zeroes(s, SRP_PREMASTER_SECRET_BYTES);
    HAP_sha512(k, s + z, SRP_PREMASTER_SECRET_BYTES - z);
}

static void Xor(int* x, const int* a, const int* b, size_t n) {
    while (n-- > 0) {
        *x++ = *a++ ^ *b++;
    }
}

void HAP_srp_proof_m1(
        uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]) {
    WITH_BN(N, BN_new(), {
        SRP_gN* gN = Get_gN_3072(N);
        uint8_t N[SRP_PRIME_BYTES];
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
        int ret = BN_bn2binpad(gN->N, N, SRP_PRIME_BYTES);
        HAPAssert(ret == SRP_PRIME_BYTES);
#else
        int ret = BN_bn2bin_padded(N, SRP_PRIME_BYTES, gN->N);
        HAPAssert(ret == 1);
#endif
        uint8_t g[1];
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
        ret = BN_bn2binpad(gN->g, g, sizeof g);
        HAPAssert(ret == sizeof g);
#else
        ret = BN_bn2bin_padded(g, sizeof g, gN->g);
        HAPAssert(ret == 1);
#endif
        uint8_t H_N[SHA512_BYTES];
        HAP_sha512(H_N, N, sizeof N);
        uint8_t H_g[SHA512_BYTES];
        HAP_sha512(H_g, g, sizeof g);
        uint8_t H_Ng[SHA512_BYTES];
        Xor((int*) H_Ng, (const int*) H_N, (const int*) H_g, SHA512_BYTES / sizeof(int));
        uint8_t H_U[SHA512_BYTES];
        HAP_sha512(H_U, user, user_len);
        size_t z_A = Count_Leading_Zeroes(pub_a, SRP_PUBLIC_KEY_BYTES);
        size_t z_B = Count_Leading_Zeroes(pub_b, SRP_PUBLIC_KEY_BYTES);
        EVP_MD_CTX* ctx;
        hash_init(&ctx, EVP_sha512());
        hash_update(&ctx, H_Ng, sizeof H_Ng);
        hash_update(&ctx, H_U, sizeof H_U);
        hash_update(&ctx, salt, SRP_SALT_BYTES);
        hash_update(&ctx, pub_a + z_A, SRP_PUBLIC_KEY_BYTES - z_A);
        hash_update(&ctx, pub_b + z_B, SRP_PUBLIC_KEY_BYTES - z_B);
        hash_update(&ctx, k, SRP_SESSION_KEY_BYTES);
        hash_final(&ctx, m1);
    });
}

void HAP_srp_proof_m2(
        uint8_t m2[SRP_PROOF_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]) {
    size_t z_A = Count_Leading_Zeroes(pub_a, SRP_PUBLIC_KEY_BYTES);
    EVP_MD_CTX* ctx;
    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, pub_a + z_A, SRP_PUBLIC_KEY_BYTES - z_A);
    hash_update(&ctx, m1, SRP_PROOF_BYTES);
    hash_update(&ctx, k, SRP_SESSION_KEY_BYTES);
    hash_final(&ctx, m2);
}

static void hash(const EVP_MD* type, uint8_t* md, const uint8_t* data, size_t size) {
    EVP_MD_CTX* ctx;
    hash_init(&ctx, type);
    hash_update(&ctx, data, size);
    hash_final(&ctx, md);
}

void HAP_sha1(uint8_t md[SHA1_BYTES], const uint8_t* data, size_t size) {
    hash(EVP_sha1(), md, data, size);
}

void HAP_sha256(uint8_t md[SHA256_BYTES], const uint8_t* data, size_t size) {
    hash(EVP_sha256(), md, data, size);
}

void HAP_sha512(uint8_t md[SHA512_BYTES], const uint8_t* data, size_t size) {
    hash(EVP_sha512(), md, data, size);
}

void HAP_hmac_sha1_aad(
        uint8_t r[HMAC_SHA1_BYTES],
        const uint8_t* key,
        size_t key_len,
        const uint8_t* in,
        size_t in_len,
        const uint8_t* aad,
        size_t aad_len) {
#if (BORINGSSL_API_VERSION_LESS_THAN_TEN == 0)
    WITH_CTX(HMAC_CTX, HMAC_CTX_new(), {
        int ret = HMAC_Init_ex(ctx, key, key_len, EVP_sha1(), NULL);
        HAPAssert(ret == 1);
        ret = HMAC_Update(ctx, in, in_len);
        HAPAssert(ret == 1);
        ret = HMAC_Update(ctx, aad, aad_len);
        HAPAssert(ret == 1);
        unsigned int r_len = HMAC_SHA1_BYTES;
        ret = HMAC_Final(ctx, r, &r_len);
        HAPAssert(ret == 1 && r_len == HMAC_SHA1_BYTES);
    });
#else
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    int ret = HMAC_Init_ex(&ctx, key, key_len, EVP_sha1(), NULL);
    HAPAssert(ret == 1);
    ret = HMAC_Update(&ctx, in, in_len);
    HAPAssert(ret == 1);
    ret = HMAC_Update(&ctx, aad, aad_len);
    HAPAssert(ret == 1);
    unsigned int r_len = HMAC_SHA1_BYTES;
    ret = HMAC_Final(&ctx, r, &r_len);
    HAPAssert(ret == 1 && r_len == HMAC_SHA1_BYTES);
    HMAC_CTX_cleanup(&ctx);
#endif
}

void HAP_hkdf_sha512(
        uint8_t* r,
        size_t r_len,
        const uint8_t* key,
        size_t key_len,
        const uint8_t* salt,
        size_t salt_len,
        const uint8_t* info,
        size_t info_len) {
    int ret = HKDF(r, r_len, EVP_sha512(), key, key_len, salt, salt_len, info, info_len);
    HAPAssert(ret == 1);
}

void HAP_pbkdf2_hmac_sha1(
        uint8_t* key,
        size_t key_len,
        const uint8_t* password,
        size_t password_len,
        const uint8_t* salt,
        size_t salt_len,
        uint32_t count) {
    PKCS5_PBKDF2_HMAC_SHA1((const char*) password, password_len, salt, salt_len, count, key_len, key);
}

typedef struct {
    EVP_CIPHER_CTX* ctx;
} EVP_CIPHER_CTX_Handle;

typedef struct {
    chachapoly_context_custom* ctx;
} chachapoly_context_Handle;

HAP_STATIC_ASSERT(sizeof(HAP_chacha20_poly1305_ctx) >= sizeof(chachapoly_context_Handle), HAP_chacha20_poly1305_ctx);

static void chacha20_poly1305_update(
        HAP_chacha20_poly1305_ctx* ctx,
        chachapoly_mode_custom_t mode,
        uint8_t* output,
        const uint8_t* input,
        size_t input_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chachapoly_context_Handle* handle = (chachapoly_context_Handle*) ctx;
    int ret;
    if (!handle->ctx) {
        handle->ctx = malloc(sizeof(chachapoly_context_custom));
        chachapoly_init_custom(handle->ctx);
        ret = chachapoly_setkey_custom(handle->ctx, k);
        HAPAssert(ret == 0);
        if (n_len >= CHACHA20_POLY1305_NONCE_BYTES_MAX) {
            n_len = CHACHA20_POLY1305_NONCE_BYTES_MAX;
        }
        // pad nonce
        uint8_t nonce[CHACHA20_POLY1305_NONCE_BYTES_MAX];
        memset(nonce, 0, sizeof nonce);
        memcpy(nonce + sizeof nonce - n_len, n, n_len);
        ret = chachapoly_starts_custom(handle->ctx, nonce, mode);
        HAPAssert(ret == 0);
    }
    if (input_len > 0) {
        ret = chachapoly_update_custom(handle->ctx, input_len, input, output);
        HAPAssert(ret == 0);
    }
}

static void chacha20_poly1305_update_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        chachapoly_mode_custom_t mode,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chacha20_poly1305_update(ctx, mode, NULL, NULL, 0, n, n_len, k);
    chachapoly_context_Handle* handle = (chachapoly_context_Handle*) ctx;
    int ret = chachapoly_update_aad_custom(handle->ctx, a, a_len);
    HAPAssert(ret == 0);
}

static void chacha20_poly1305_final(HAP_chacha20_poly1305_ctx* ctx, uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]) {
    chachapoly_context_Handle* handle = (chachapoly_context_Handle*) ctx;
    int ret = chachapoly_finish_custom(handle->ctx, tag);
    HAPAssert(ret == 0);
    chachapoly_free_custom(handle->ctx);
    free(handle->ctx);
    handle->ctx = NULL;
}

void HAP_chacha20_poly1305_init(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* n HAP_UNUSED,
        size_t n_len HAP_UNUSED,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES] HAP_UNUSED) {
    chachapoly_context_Handle* handle = (chachapoly_context_Handle*) ctx;
    handle->ctx = NULL;
}

void HAP_chacha20_poly1305_update_enc(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chacha20_poly1305_update(ctx, CHACHAPOLY_ENCRYPT, c, m, m_len, n, n_len, k);
}

void HAP_chacha20_poly1305_update_enc_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chacha20_poly1305_update_aad(ctx, CHACHAPOLY_ENCRYPT, a, a_len, n, n_len, k);
}

void HAP_chacha20_poly1305_final_enc(HAP_chacha20_poly1305_ctx* ctx, uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]) {
    chacha20_poly1305_final(ctx, tag);
}

void HAP_chacha20_poly1305_update_dec(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chacha20_poly1305_update(ctx, CHACHAPOLY_DECRYPT, m, c, c_len, n, n_len, k);
}

void HAP_chacha20_poly1305_update_dec_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
    chacha20_poly1305_update_aad(ctx, CHACHAPOLY_DECRYPT, a, a_len, n, n_len, k);
}

int HAP_chacha20_poly1305_final_dec(HAP_chacha20_poly1305_ctx* ctx, const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]) {
    uint8_t tag2[CHACHA20_POLY1305_TAG_BYTES];
    chacha20_poly1305_final(ctx, tag2);
    return HAP_constant_time_equal(tag, tag2, CHACHA20_POLY1305_TAG_BYTES) ? 0 : -1;
}

int HAP_chacha20_poly1305_final_dec_truncated_tag(HAP_chacha20_poly1305_ctx* ctx, const uint8_t* tag, size_t t_len) {
    uint8_t tag2[CHACHA20_POLY1305_TAG_BYTES];
    chacha20_poly1305_final(ctx, tag2);
    return HAP_constant_time_equal(tag, tag2, t_len) ? 0 : -1;
}

HAP_STATIC_ASSERT(sizeof(HAP_aes_ctr_ctx) >= sizeof(EVP_CIPHER_CTX_Handle), HAP_aes_ctr_ctx);

void HAP_aes_ctr_init(HAP_aes_ctr_ctx* ctx, const uint8_t* key, int size, const uint8_t iv[16]) {
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    HAPAssert(size == 16 || size == 32);
    handle->ctx = EVP_CIPHER_CTX_new();
    int ret = EVP_EncryptInit_ex(handle->ctx, (size == 16) ? EVP_aes_128_ctr() : EVP_aes_256_ctr(), NULL, key, iv);
    HAPAssert(ret == 1);
    EVP_CIPHER_CTX_set_padding(handle->ctx, 0);
}

void HAP_aes_ctr_encrypt(HAP_aes_ctr_ctx* ctx, uint8_t* ct, const uint8_t* pt, size_t pt_len) {
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int ct_len;
    int ret = EVP_EncryptUpdate(handle->ctx, ct, &ct_len, pt, pt_len);
    HAPAssert(ret == 1 && (size_t) ct_len == pt_len);
}

void HAP_aes_ctr_decrypt(HAP_aes_ctr_ctx* ctx, uint8_t* pt, const uint8_t* ct, size_t ct_len) {
    HAP_aes_ctr_encrypt(ctx, pt, ct, ct_len);
}

void HAP_aes_ctr_done(HAP_aes_ctr_ctx* ctx) {
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    EVP_CIPHER_CTX_free(handle->ctx);
    handle->ctx = NULL;
}
