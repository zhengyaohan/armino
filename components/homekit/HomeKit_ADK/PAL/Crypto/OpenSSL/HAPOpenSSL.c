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

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/srp.h>
#include "HAP+API.h"
#include "HAPCrypto.h"
#include "HAPLogSubsystem.h"

#define HAP_CRYPTO_OPENSSL

#if OPENSSL_VERSION_NUMBER >= 0x10101000L
#define OPENSSL_VERSION_1_1_1
#include <openssl/kdf.h>
#else
#define OPENSSL_VERSION_1_0_1
#include "Curve25519/curve25519.c"
#include "HKDF/hkdf.c"

#ifndef HAVE_CUSTOM_CHACHA20POLY1305
#include "ChaCha20Poly1305/chacha20.c"
#include "ChaCha20Poly1305/chachapoly.c"
#include "ChaCha20Poly1305/poly1305.c"
#endif

typedef enum { big, little } endianess_t;

/* ignore negative */
static int bn2binpad(const BIGNUM* a, unsigned char* to, int tolen, endianess_t endianess) {
    int n;
    size_t i, lasti, j, atop, mask;
    BN_ULONG l;

    /*
     * In case |a| is fixed-top, BN_num_bytes can return bogus length,
     * but it's assumed that fixed-top inputs ought to be "nominated"
     * even for padded output, so it works out...
     */
    n = BN_num_bytes(a);
    if (tolen == -1) {
        tolen = n;
    } else if (tolen < n) { /* uncommon/unlike case */
        BIGNUM temp = *a;

        bn_correct_top(&temp);
        n = BN_num_bytes(&temp);
        if (tolen < n)
            return -1;
    }

    /* Swipe through whole available data and don't give away padded zero. */
    atop = a->dmax * BN_BYTES;
    if (atop == 0) {
        OPENSSL_cleanse(to, tolen);
        return tolen;
    }

    lasti = atop - 1;
    atop = a->top * BN_BYTES;
    if (endianess == big)
        to += tolen; /* start from the end of the buffer */
    for (i = 0, j = 0; j < (size_t) tolen; j++) {
        unsigned char val;
        l = a->d[i / BN_BYTES];
        mask = 0 - ((j - atop) >> (8 * sizeof(i) - 1));
        val = (unsigned char) (l >> (8 * (i % BN_BYTES)) & mask);
        if (endianess == big)
            *--to = val;
        else
            *to++ = val;
        i += (i - lasti) >> (8 * sizeof(i) - 1); /* stay on last limb */
    }

    return tolen;
}

int BN_bn2binpad(const BIGNUM* a, unsigned char* to, int tolen) {
    if (tolen < 0)
        return -1;
    return bn2binpad(a, to, tolen, big);
}
#endif // OPENSSL_VERSION_1_0_1

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

#ifdef OPENSSL_VERSION_1_0_1
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
#endif

#define WITH(type, name, init, free, X) \
    do { \
        type* name = init; \
        X; \
        free(name); \
    } while (0)

#define WITH_PKEY(name, init, X) WITH(EVP_PKEY, name, init, EVP_PKEY_free, X)

void HAP_ed25519_public_key(uint8_t pk[ED25519_PUBLIC_KEY_BYTES], const uint8_t sk[ED25519_SECRET_KEY_BYTES]) {
#ifdef OPENSSL_VERSION_1_1_1
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, sk, ED25519_SECRET_KEY_BYTES), {
        size_t len = ED25519_PUBLIC_KEY_BYTES;
        int ret = EVP_PKEY_get_raw_public_key(key, pk, &len);
        HAPAssert(ret == 1 && len == ED25519_SECRET_KEY_BYTES);
    });
#endif
#ifdef OPENSSL_VERSION_1_0_1
    WITH_BLINDING(
            { USE_AND_CLEAR(privKey, ed25519_private_key_size, { ed25519_CreateKeyPair(pk, privKey, &ctx, sk); }); });
    return;
#endif
}

#define WITH_CTX(type, init, X) WITH(type, ctx, init, type##_free, X)

void HAP_ed25519_sign(
        uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t sk[ED25519_SECRET_KEY_BYTES],
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES] HAP_UNUSED) {
#ifdef OPENSSL_VERSION_1_1_1
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, sk, ED25519_SECRET_KEY_BYTES), {
        WITH_CTX(EVP_MD_CTX, EVP_MD_CTX_new(), {
            int ret = EVP_DigestSignInit(ctx, NULL, NULL, NULL, key);
            HAPAssert(ret == 1);
            size_t len = ED25519_BYTES;
            EVP_DigestSign(ctx, sig, &len, m, m_len);
            HAPAssert(len == ED25519_BYTES);
        });
    });
#endif
#ifdef OPENSSL_VERSION_1_0_1
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
#ifdef OPENSSL_VERSION_1_1_1
    int ret;
    WITH_PKEY(key, EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, pk, ED25519_PUBLIC_KEY_BYTES), {
        WITH_CTX(EVP_MD_CTX, EVP_MD_CTX_new(), {
            ret = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, key);
            HAPAssert(ret == 1);
            ret = EVP_DigestVerify(ctx, sig, ED25519_BYTES, m, m_len);
        });
    });
    return (ret == 1) ? 0 : -1;
#endif
#ifdef OPENSSL_VERSION_1_0_1
    int ret = ed25519_VerifySignature(sig, pk, m, m_len);
    return (ret == 1) ? 0 : -1;
#endif
}

void HAP_X25519_scalarmult_base(uint8_t r[X25519_BYTES], const uint8_t n[X25519_SCALAR_BYTES]) {
#ifdef OPENSSL_VERSION_1_1_1
    WITH_PKEY(key, EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, n, X25519_SCALAR_BYTES), {
        size_t len = X25519_BYTES;
        int ret = EVP_PKEY_get_raw_public_key(key, r, &len);
        HAPAssert(ret == 1 && len == X25519_BYTES);
    });
#endif
#ifdef OPENSSL_VERSION_1_0_1
    X25519_public_from_private(r, n);
    return;
#endif
}

void HAP_X25519_scalarmult(
        uint8_t r[X25519_BYTES],
        const uint8_t n[X25519_SCALAR_BYTES],
        const uint8_t p[X25519_BYTES]) {
#ifdef OPENSSL_VERSION_1_1_1
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
#endif
#ifdef OPENSSL_VERSION_1_0_1
    int ret = X25519(r, n, p);
    HAPAssert(ret == 1);
    return;
#endif
}

// OpenSSL doesn't support SRP6a with SHA512 so we have to do this on foot

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

static SRP_gN* Get_gN_3072() {
    static SRP_gN* gN = NULL;
    if (!gN) {
        gN = SRP_get_default_gN("3072");
    }
    return gN;
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
        BN_set_flags(x, BN_FLG_CONSTTIME);
        WITH_BN(verifier, BN_new(), {
            SRP_gN* gN = Get_gN_3072();
            WITH_CTX(BN_CTX, BN_CTX_new(), {
                int ret = BN_mod_exp(verifier, gN->g, x, gN->N, ctx);
                HAPAssert(!!ret);
            });
            int ret = BN_bn2binpad(verifier, v, SRP_VERIFIER_BYTES);
            HAPAssert(ret == SRP_VERIFIER_BYTES);
        });
    });
}

static BIGNUM* Calc_k(SRP_gN* gN) {
    uint8_t N[SRP_PRIME_BYTES];
    int ret = BN_bn2binpad(gN->N, N, sizeof N);
    HAPAssert(ret == sizeof N);
    uint8_t g[SRP_PRIME_BYTES];
    ret = BN_bn2binpad(gN->g, g, sizeof g);
    HAPAssert(ret == sizeof g);
    uint8_t k[SHA512_BYTES];
    EVP_MD_CTX* ctx;
    hash_init(&ctx, EVP_sha512());
    hash_update(&ctx, N, SRP_PRIME_BYTES);
    hash_update(&ctx, g, SRP_PRIME_BYTES);
    hash_final(&ctx, k);
    return BN_bin2bn(k, sizeof k, NULL);
}

static BIGNUM* Calc_B(BIGNUM* b, BIGNUM* v) {
    SRP_gN* gN = Get_gN_3072();
    BIGNUM* B = BN_new();
    WITH_CTX(BN_CTX, BN_CTX_new(), {
        WITH_BN(gb, BN_new(), {
            BN_set_flags(b, BN_FLG_CONSTTIME);
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
    return B;
}

void HAP_srp_public_key(
        uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]) {
    WITH_BN(b, BN_bin2bn(priv_b, SRP_SECRET_KEY_BYTES, NULL), {
        WITH_BN(verifier, BN_bin2bn(v, SRP_VERIFIER_BYTES, NULL), {
            WITH_BN(B, Calc_B(b, verifier), {
                int ret = BN_bn2binpad(B, pub_b, SRP_PUBLIC_KEY_BYTES);
                HAPAssert(ret == SRP_PUBLIC_KEY_BYTES);
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
                int ret = BN_nnmod(rem, A, Get_gN_3072()->N, ctx);
                HAPAssert(!!ret);
                if (BN_is_zero(rem) == 0) {
                    isAValid = true;
                }
            });
        });

        WITH_BN(b, BN_bin2bn(priv_b, SRP_SECRET_KEY_BYTES, NULL), {
            WITH_BN(u_, BN_bin2bn(u, SRP_SCRAMBLING_PARAMETER_BYTES, NULL), {
                WITH_BN(v_, BN_bin2bn(v, SRP_VERIFIER_BYTES, NULL), {
                    WITH_BN(s_, SRP_Calc_server_key(A, v_, u_, b, Get_gN_3072()->N), {
                        int ret = BN_bn2binpad(s_, s, SRP_PREMASTER_SECRET_BYTES);
                        HAPAssert(ret == SRP_PREMASTER_SECRET_BYTES);
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
    SRP_gN* gN = Get_gN_3072();
    uint8_t N[SRP_PRIME_BYTES];
    int ret = BN_bn2binpad(gN->N, N, SRP_PRIME_BYTES);
    HAPAssert(ret == SRP_PRIME_BYTES);
    uint8_t g[1];
    ret = BN_bn2binpad(gN->g, g, sizeof g);
    HAPAssert(ret == sizeof g);
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
#ifdef OPENSSL_VERSION_1_0_1
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
#ifdef OPENSSL_VERSION_1_1_1
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
#ifdef OPENSSL_VERSION_1_0_1
    int ret = HKDF(r, r_len, EVP_sha512(), key, key_len, salt, salt_len, info, info_len);
    HAPAssert(ret == 1);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    WITH_CTX(EVP_PKEY_CTX, EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL), {
        int ret = EVP_PKEY_derive_init(ctx);
        HAPAssert(ret == 1);
        ret = EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha512());
        HAPAssert(ret == 1);
        ret = EVP_PKEY_CTX_set1_hkdf_salt(ctx, salt, salt_len);
        HAPAssert(ret == 1);
        ret = EVP_PKEY_CTX_set1_hkdf_key(ctx, key, key_len);
        HAPAssert(ret == 1);
        ret = EVP_PKEY_CTX_add1_hkdf_info(ctx, info, info_len);
        HAPAssert(ret == 1);
        size_t out_len = r_len;
        ret = EVP_PKEY_derive(ctx, r, &out_len);
        HAPAssert(ret == 1 && out_len == r_len);
    });
#endif
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

#ifdef OPENSSL_VERSION_1_0_1
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

#else
HAP_STATIC_ASSERT(sizeof(HAP_chacha20_poly1305_ctx) >= sizeof(EVP_CIPHER_CTX_Handle), HAP_chacha20_poly1305_ctx);

// OpenSSL doesn't like overlapping in/out buffers in EVP_EncryptUpdate/EVP_DecryptUpdate
static bool is_overlapping(const uint8_t* a, const uint8_t* b, size_t n) {
    return (a < b && a + n > b) || (b < a && b + n > a);
}

static uint8_t* use_temporary_if_overlapping(uint8_t** tmp, const uint8_t* in, uint8_t* out, size_t n) {
    *tmp = NULL;
    if (is_overlapping(in, out, n)) {
        return *tmp = malloc(n);
    }
    return out;
}

static void copy_and_free_if_overlapping(uint8_t** tmp, uint8_t** out, size_t n) {
    if (*tmp) {
        memcpy(*out, *tmp, n);
        free(*tmp);
        *tmp = NULL;
    }
}

#endif

void HAP_chacha20_poly1305_init(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* n HAP_UNUSED,
        size_t n_len HAP_UNUSED,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES] HAP_UNUSED) {
#ifdef OPENSSL_VERSION_1_0_1
    chachapoly_context_Handle* handle = (chachapoly_context_Handle*) ctx;
    handle->ctx = NULL;
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    handle->ctx = NULL;
#endif
}

void HAP_chacha20_poly1305_update_enc(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* c,
        const uint8_t* m,
        size_t m_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    chacha20_poly1305_update(ctx, CHACHAPOLY_ENCRYPT, c, m, m_len, n, n_len, k);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int ret;
    if (!handle->ctx) {
        handle->ctx = EVP_CIPHER_CTX_new();
        ret = EVP_EncryptInit_ex(handle->ctx, EVP_chacha20_poly1305(), 0, 0, 0);
        HAPAssert(ret == 1);
        ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_SET_TAG, CHACHA20_POLY1305_TAG_BYTES, NULL);
        HAPAssert(ret == 1);
        ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_SET_IVLEN, n_len, NULL);
        HAPAssert(ret == 1);
        ret = EVP_EncryptInit_ex(handle->ctx, NULL, NULL, k, n);
        HAPAssert(ret == 1);
    }
    if (m_len > 0) {
        uint8_t* tmp;
        int c_len;
        ret = EVP_EncryptUpdate(handle->ctx, use_temporary_if_overlapping(&tmp, m, c, m_len), &c_len, m, m_len);
        copy_and_free_if_overlapping(&tmp, &c, m_len);
        HAPAssert(ret == 1 && c_len == (int) m_len);
    }
#endif
}

void HAP_chacha20_poly1305_update_enc_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    chacha20_poly1305_update_aad(ctx, CHACHAPOLY_ENCRYPT, a, a_len, n, n_len, k);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    HAP_chacha20_poly1305_update_enc(ctx, NULL, NULL, 0, n, n_len, k);
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int a_out;
    int ret = EVP_EncryptUpdate(handle->ctx, NULL, &a_out, a, a_len);
    HAPAssert(ret == 1 && a_out == (int) a_len);
#endif
}

void HAP_chacha20_poly1305_final_enc(HAP_chacha20_poly1305_ctx* ctx, uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    chacha20_poly1305_final(ctx, tag);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int c_len;
    int ret = EVP_EncryptFinal_ex(handle->ctx, NULL, &c_len);
    HAPAssert(ret == 1 && !c_len);
    ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_GET_TAG, CHACHA20_POLY1305_TAG_BYTES, tag);
    HAPAssert(ret == 1);
    EVP_CIPHER_CTX_free(handle->ctx);
    handle->ctx = NULL;
#endif
}

void HAP_chacha20_poly1305_update_dec(
        HAP_chacha20_poly1305_ctx* ctx,
        uint8_t* m,
        const uint8_t* c,
        size_t c_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    chacha20_poly1305_update(ctx, CHACHAPOLY_DECRYPT, m, c, c_len, n, n_len, k);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int ret;
    if (!handle->ctx) {
        handle->ctx = EVP_CIPHER_CTX_new();
        int ret = EVP_DecryptInit_ex(handle->ctx, EVP_chacha20_poly1305(), 0, 0, 0);
        HAPAssert(ret == 1);
        ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_SET_IVLEN, n_len, NULL);
        HAPAssert(ret == 1);
        ret = EVP_DecryptInit_ex(handle->ctx, NULL, NULL, k, n);
        HAPAssert(ret == 1);
    }
    if (c_len > 0) {
        uint8_t* tmp;
        int m_len;
        ret = EVP_DecryptUpdate(handle->ctx, use_temporary_if_overlapping(&tmp, c, m, c_len), &m_len, c, c_len);
        copy_and_free_if_overlapping(&tmp, &m, c_len);
        HAPAssert(ret == 1);
    }
#endif
}

void HAP_chacha20_poly1305_update_dec_aad(
        HAP_chacha20_poly1305_ctx* ctx,
        const uint8_t* a,
        size_t a_len,
        const uint8_t* n,
        size_t n_len,
        const uint8_t k[CHACHA20_POLY1305_KEY_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    chacha20_poly1305_update_aad(ctx, CHACHAPOLY_DECRYPT, a, a_len, n, n_len, k);
#endif
#ifdef OPENSSL_VERSION_1_1_1
    HAP_chacha20_poly1305_update_dec(ctx, NULL, NULL, 0, n, n_len, k);
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int a_out;
    int ret = EVP_DecryptUpdate(handle->ctx, NULL, &a_out, a, a_len);
    HAPAssert(ret == 1 && a_out == (int) a_len);
#endif
}

int HAP_chacha20_poly1305_final_dec(HAP_chacha20_poly1305_ctx* ctx, const uint8_t tag[CHACHA20_POLY1305_TAG_BYTES]) {
#ifdef OPENSSL_VERSION_1_0_1
    uint8_t tag2[CHACHA20_POLY1305_TAG_BYTES];
    chacha20_poly1305_final(ctx, tag2);
    return HAP_constant_time_equal(tag, tag2, CHACHA20_POLY1305_TAG_BYTES) ? 0 : -1;
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_SET_TAG, CHACHA20_POLY1305_TAG_BYTES, (void*) tag);
    HAPAssert(ret == 1);
    int m_len;
    ret = EVP_DecryptFinal_ex(handle->ctx, NULL, &m_len);
    HAPAssert(m_len == 0);
    EVP_CIPHER_CTX_free(handle->ctx);
    handle->ctx = NULL;
    return (ret == 1) ? 0 : -1;
#endif
}

int HAP_chacha20_poly1305_final_dec_truncated_tag(HAP_chacha20_poly1305_ctx* ctx, const uint8_t* tag, size_t t_len) {
#ifdef OPENSSL_VERSION_1_0_1
    uint8_t tag2[CHACHA20_POLY1305_TAG_BYTES];
    chacha20_poly1305_final(ctx, tag2);
    return HAP_constant_time_equal(tag, tag2, t_len) ? 0 : -1;
#endif
#ifdef OPENSSL_VERSION_1_1_1
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    int ret = EVP_CIPHER_CTX_ctrl(handle->ctx, EVP_CTRL_AEAD_SET_TAG, t_len, (void*) tag);
    HAPAssert(ret == 1);
    int m_len;
    ret = EVP_DecryptFinal_ex(handle->ctx, NULL, &m_len);
    HAPAssert(m_len == 0);
    EVP_CIPHER_CTX_free(handle->ctx);
    handle->ctx = NULL;
    return (ret == 1) ? 0 : -1;
#endif
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
    HAPAssert(ret == 1 && ct_len == (int) pt_len);
}

void HAP_aes_ctr_decrypt(HAP_aes_ctr_ctx* ctx, uint8_t* pt, const uint8_t* ct, size_t ct_len) {
    HAP_aes_ctr_encrypt(ctx, pt, ct, ct_len);
}

void HAP_aes_ctr_done(HAP_aes_ctr_ctx* ctx) {
    EVP_CIPHER_CTX_Handle* handle = (EVP_CIPHER_CTX_Handle*) ctx;
    EVP_CIPHER_CTX_free(handle->ctx);
    handle->ctx = NULL;
}
