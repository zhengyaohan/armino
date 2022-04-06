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

#include <stdlib.h>

#include "ocrypto_chacha20_poly1305.h"
#include "ocrypto_curve25519.h"
#include "ocrypto_ed25519.h"
#include "ocrypto_hkdf_sha512.h"
#include "ocrypto_sha1.h"
#include "ocrypto_sha256.h"
#include "ocrypto_sha512.h"
#include "ocrypto_srp.h"

#include "HAP+API.h"
#include "HAPCrypto.h"
#include "HAPLogSubsystem.h"

void HAP_ed25519_public_key(uint8_t pk[ED25519_PUBLIC_KEY_BYTES], const uint8_t sk[ED25519_SECRET_KEY_BYTES]) {
    ocrypto_ed25519_public_key(pk, sk);
}

void HAP_ed25519_sign(
        uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t sk[ED25519_SECRET_KEY_BYTES],
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES]) {
    ocrypto_ed25519_sign(sig, m, m_len, sk, pk);
}

int HAP_ed25519_verify(
        const uint8_t sig[ED25519_BYTES],
        const uint8_t* m,
        size_t m_len,
        const uint8_t pk[ED25519_PUBLIC_KEY_BYTES]) {
    return ocrypto_ed25519_verify(sig, m, m_len, pk);
}

void HAP_X25519_scalarmult(
        uint8_t r[X25519_BYTES],
        const uint8_t n[X25519_SCALAR_BYTES],
        const uint8_t p[X25519_BYTES]) {
    ocrypto_curve25519_scalarmult(r, n, p);
}

void HAP_X25519_scalarmult_base(uint8_t r[X25519_BYTES], const uint8_t n[X25519_SCALAR_BYTES]) {
    ocrypto_curve25519_scalarmult_base(r, n);
}

void HAP_srp_verifier(
        uint8_t v[SRP_VERIFIER_BYTES],
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t* pass,
        size_t pass_len) {
    ocrypto_srp_verifier(v, salt, user, user_len, pass, pass_len);
}

void HAP_srp_public_key(
        uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]) {
    ocrypto_srp_public_key(pub_b, priv_b, v);
}

void HAP_srp_scrambling_parameter(
        uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES]) {
    ocrypto_srp_scrambling_parameter(u, pub_a, pub_b);
}

int HAP_srp_premaster_secret(
        uint8_t s[SRP_PREMASTER_SECRET_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t priv_b[SRP_SECRET_KEY_BYTES],
        const uint8_t u[SRP_SCRAMBLING_PARAMETER_BYTES],
        const uint8_t v[SRP_VERIFIER_BYTES]) {
    return ocrypto_srp_premaster_secret(s, pub_a, priv_b, u, v);
}

void HAP_srp_session_key(uint8_t k[SRP_SESSION_KEY_BYTES], const uint8_t s[SRP_PREMASTER_SECRET_BYTES]) {
    ocrypto_srp_session_key(k, s);
}

void HAP_srp_proof_m1(
        uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t* user,
        size_t user_len,
        const uint8_t salt[SRP_SALT_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t pub_b[SRP_PUBLIC_KEY_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]) {
    ocrypto_srp_proof_m1(m1, user, user_len, salt, pub_a, pub_b, k);
}

void HAP_srp_proof_m2(
        uint8_t m2[SRP_PROOF_BYTES],
        const uint8_t pub_a[SRP_PUBLIC_KEY_BYTES],
        const uint8_t m1[SRP_PROOF_BYTES],
        const uint8_t k[SRP_SESSION_KEY_BYTES]) {
    ocrypto_srp_proof_m2(m2, pub_a, m1, k);
}

void HAP_sha1(uint8_t md[SHA1_BYTES], const uint8_t* data, size_t size) {
    ocrypto_sha1(md, data, size);
}

void HAP_sha256(uint8_t md[SHA256_BYTES], const uint8_t* data, size_t size) {
    ocrypto_sha256(md, data, size);
}

void HAP_sha512(uint8_t md[SHA512_BYTES], const uint8_t* data, size_t size) {
    ocrypto_sha512(md, data, size);
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
    ocrypto_hkdf_sha512(r, r_len, key, key_len, salt, salt_len, info, info_len);
}

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
    ocrypto_chacha20_poly1305_encrypt_aad(tag, c, m, m_len, a, a_len, n, n_len, k);
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
    return ocrypto_chacha20_poly1305_decrypt_aad(tag, m, c, c_len, a, a_len, n, n_len, k);
}
