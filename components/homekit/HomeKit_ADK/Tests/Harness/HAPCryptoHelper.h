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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#ifndef HAP_CRYPTO_HELPER_H
#define HAP_CRYPTO_HELPER_H

#ifdef __cplusplus
#include <functional>
#include <tuple>

extern "C" {
#endif

#include "HAPBase.h"
#include "HAPSession.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Encrypt a message
 *
 * @param key                 key
 * @param nonce               nonce
 * @param[out] encryptedBytes buffer to store the encrypted data
 * @param plaintextBytes_     input data
 * @param numPlaintextBytes_  number of bytes of input data
 * @param aadBytes_           additional authenticated data
 * @param numAADBytes         number of bytes of additional authenticated data
 */
void HAPChaCha20Poly1305EncryptHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* encryptedBytes_,
        const void* plaintextBytes_,
        size_t numPlaintextBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes);

/**
 * Decrypt a message
 *
 * @param key                  key
 * @param nonce                nonce
 * @param[out] plaintextBytes_ buffer to store the decrypted data
 * @param encryptedBytes_      input data
 * @param numEncryptedBytes    number of bytes of input data
 * @param aadBytes_            additional authenticated data
 * @param numAADBytes          number of bytes of the additional authenticated data
 */
HAPError HAPChaCha20Poly1305DecryptHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* plaintextBytes_,
        const void* encryptedBytes_,
        size_t numEncryptedBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes);

/**
 * Decrypt a message with a truncated tag
 *
 * @param key                  key
 * @param nonce                nonce
 * @param[out] plaintextBytes_ buffer to store the decrypted data
 * @param encryptedBytes_      input data
 * @param numEncryptedBytes    number of bytes of input data
 * @param aadBytes_            additional authenticated data
 * @param numAADBytes          number of bytes of the additional authenticated data
 * @param numTagBytes          number of truncated tag bytes
 */
HAPError HAPChaCha20Poly1305DecryptTruncatedTagHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* plaintextBytes_,
        const void* encryptedBytes_,
        size_t numEncryptedBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes,
        size_t numTagBytes);

#ifdef __cplusplus
}

/** Decoded BLE advertisement data structure */
struct HAPBLEAdvertisementData {
    bool hasValidFlags;
    bool hasValidManufacturerData;
    bool hasValidLocalName;
    uint8_t flags;
    uint16_t coId;
    uint8_t ty;
    uint8_t stl;
    bool isEncryptedNotification;
    union {
        struct {
            uint8_t sf;
            uint8_t cn;
            uint8_t cv;
            uint8_t deviceId[6];
            uint16_t acid;
            uint16_t gsn;
            uint32_t sh;
            bool isPaired;
        } regular;
        struct {
            uint8_t aaId[6];
            uint16_t gsn;
            uint16_t iid;
            uint8_t value[8];
        } encryptedNotification;
    };
    uint8_t localName[16];
    bool isCompleteLocalName;

    /**
     * Construct advertisement data from encoded bytes.
     *
     * @param bytes     advertisement data to decode
     * @param numBytes  number of advertisement data bytes
     * @param key       broadcast key
     * @param gsn       expected GSN. Note that if GSN mismatches, decryption would fail.
     *
     * @return error code and decoded advertisement data.
     */
    static std::tuple<HAPError, HAPBLEAdvertisementData>
            FromBytes(const uint8_t* bytes, size_t numBytes, const HAPSessionKey* key, uint16_t gsn);
};

/**
 * Decrypt an event message generated by the accessory
 *
 * @param eventBytes           event message generated by the accessory
 * @param numEventBytes        number of bytes of the event message
 * @param decryptKey           decryption key to decrypt the event message from accessory with
 * @param decryptNonce         decryption nonce
 * @param handleEventValue     handler for the event TLV value
 */
void HAPDecryptEventNotificationHelper(
        const void* eventBytes,
        size_t numEventBytes,
        const HAPSessionKey* decryptKey,
        uint64_t& decryptNonce,
        std::function<void(uint16_t, HAPTLV)> handleEventValue);

#endif // __cplusplus

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif
