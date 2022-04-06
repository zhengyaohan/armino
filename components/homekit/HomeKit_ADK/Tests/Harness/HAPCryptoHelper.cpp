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

#include "HAPCryptoHelper.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPCrypto.h"
#include "HAPPDU+TLV.h"

extern "C" {

void HAPChaCha20Poly1305EncryptHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* encryptedBytes_,
        const void* plaintextBytes_,
        size_t numPlaintextBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes) {
    HAPAssert(key);
    HAPAssert(encryptedBytes_);
    uint8_t* encryptedBytes = (uint8_t*) encryptedBytes_;
    HAPAssert(plaintextBytes_);
    const uint8_t* plaintextBytes = (const uint8_t*) plaintextBytes_;
    HAPAssert(!numAADBytes || aadBytes_);
    const uint8_t* _Nullable aadBytes = (const uint8_t*) aadBytes_;

    // Encrypt message. Tag is appended to cipher text.
    uint8_t noncebytes[] = { HAPExpandLittleUInt64(nonce) };
    if (aadBytes) {
        HAP_chacha20_poly1305_encrypt_aad(
                /* tag: */ &encryptedBytes[numPlaintextBytes],
                encryptedBytes,
                plaintextBytes,
                numPlaintextBytes,
                aadBytes,
                numAADBytes,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
    } else {
        HAP_chacha20_poly1305_encrypt(
                /* tag: */ &encryptedBytes[numPlaintextBytes],
                encryptedBytes,
                plaintextBytes,
                numPlaintextBytes,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
    }
}

HAPError HAPChaCha20Poly1305DecryptHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* plaintextBytes_,
        const void* encryptedBytes_,
        size_t numEncryptedBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes) {
    HAPAssert(key);
    HAPAssert(plaintextBytes_);
    uint8_t* plaintextBytes = (uint8_t*) plaintextBytes_;
    HAPAssert(encryptedBytes_);
    const uint8_t* encryptedBytes = (const uint8_t*) encryptedBytes_;
    HAPAssert(!numAADBytes || aadBytes_);
    const uint8_t* aadBytes = (const uint8_t*) aadBytes_;

    // Decrypt message. Tag is appended to cipher text.
    HAPAssert(numEncryptedBytes >= CHACHA20_POLY1305_TAG_BYTES);

    uint8_t noncebytes[] = { HAPExpandLittleUInt64(nonce) };
    if (aadBytes) {
        int e = HAP_chacha20_poly1305_decrypt_aad(
                /* tag: */ &encryptedBytes[numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES],
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES,
                aadBytes,
                numAADBytes,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
        if (e) {
            HAPAssert(e == -1);
            return kHAPError_InvalidData;
        }
    } else {
        int e = HAP_chacha20_poly1305_decrypt(
                /* tag: */ &encryptedBytes[numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES],
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - CHACHA20_POLY1305_TAG_BYTES,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
        if (e) {
            HAPAssert(e == -1);
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

HAPError HAPChaCha20Poly1305DecryptTruncatedTagHelper(
        const HAPSessionKey* key,
        uint64_t nonce,
        void* plaintextBytes_,
        const void* encryptedBytes_,
        size_t numEncryptedBytes,
        const void* _Nullable aadBytes_,
        size_t numAADBytes,
        size_t numTagBytes) {
    HAPAssert(key);
    HAPAssert(plaintextBytes_);
    uint8_t* plaintextBytes = (uint8_t*) plaintextBytes_;
    HAPAssert(encryptedBytes_);
    const uint8_t* encryptedBytes = (const uint8_t*) encryptedBytes_;
    HAPAssert(!numAADBytes || aadBytes_);
    const uint8_t* aadBytes = (const uint8_t*) aadBytes_;

    // Decrypt message. Tag is appended to cipher text.
    HAPAssert(numTagBytes <= CHACHA20_POLY1305_TAG_BYTES);
    HAPAssert(numEncryptedBytes >= numTagBytes);

    uint8_t noncebytes[] = { HAPExpandLittleUInt64(nonce) };
    if (aadBytes) {
        int e = HAP_chacha20_poly1305_decrypt_aad_truncated_tag(
                /* tag: */ &encryptedBytes[numEncryptedBytes - numTagBytes],
                numTagBytes,
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - numTagBytes,
                aadBytes,
                numAADBytes,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
        if (e) {
            HAPAssert(e == -1);
            return kHAPError_InvalidData;
        }
    } else {
        int e = HAP_chacha20_poly1305_decrypt_truncated_tag(
                /* tag: */ &encryptedBytes[numEncryptedBytes - numTagBytes],
                numTagBytes,
                plaintextBytes,
                encryptedBytes,
                /* c_len: */ numEncryptedBytes - numTagBytes,
                noncebytes,
                sizeof noncebytes,
                key->bytes);
        if (e) {
            HAPAssert(e == -1);
            return kHAPError_InvalidData;
        }
    }

    return kHAPError_None;
}

} // extern "C"

std::tuple<HAPError, HAPBLEAdvertisementData> HAPBLEAdvertisementData::FromBytes(
        const uint8_t* bytes,
        size_t numBytes,
        const HAPSessionKey* key,
        uint16_t gsn) {
    HAPBLEAdvertisementData decoded = {
        0,
    };
    while (numBytes > 0) {
        uint8_t length = bytes[0];
        if (length == 0) {
            return std::make_tuple(kHAPError_InvalidData, decoded);
        }
        if (length >= numBytes) {
            return std::make_tuple(kHAPError_InvalidData, decoded);
        }
        uint8_t adType = bytes[1];
        switch (adType) {
            case 0x01: {
                // flags
                if (length != 2) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                if (decoded.hasValidFlags) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                decoded.flags = bytes[2];
                // General Discoverable Mode bit must be set
                if ((decoded.flags & 2) != 2) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                decoded.hasValidFlags = true;
                break;
            }
            case 0xff: {
                // manufacturer data
                if (decoded.hasValidManufacturerData) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                if (length < 0x16) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                decoded.coId = HAPReadLittleUInt16(&bytes[2]);
                if (decoded.coId != 0x004C) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                decoded.ty = bytes[4];
                decoded.stl = bytes[5];
                if (length == 0x16) {
                    if (decoded.ty != 0x06 || decoded.stl != ((1 << 5) | 17)) {
                        return std::make_tuple(kHAPError_InvalidData, decoded);
                    }
                    decoded.regular.sf = bytes[6];
                    if ((decoded.regular.sf & 0xfe) != 0) {
                        return std::make_tuple(kHAPError_InvalidData, decoded);
                    }
                    if (decoded.regular.sf & 1) {
                        decoded.regular.isPaired = true;
                    }
                    HAPRawBufferCopyBytes(decoded.regular.deviceId, &bytes[7], sizeof decoded.regular.deviceId);
                    decoded.regular.acid = HAPReadLittleUInt16(&bytes[13]);
                    decoded.regular.gsn = HAPReadLittleUInt16(&bytes[15]);
                    decoded.regular.cn = bytes[17];
                    decoded.regular.cv = bytes[18];
                    if (decoded.regular.cv != 2) {
                        return std::make_tuple(kHAPError_InvalidData, decoded);
                    }
                    decoded.regular.sh = HAPReadLittleUInt32(&bytes[19]);
                } else if (length == 0x1B) {
                    if (decoded.ty != 0x11) {
                        return std::make_tuple(kHAPError_InvalidData, decoded);
                    }
                    decoded.isEncryptedNotification = true;
                    if (decoded.stl != 0x36) {
                        return std::make_tuple(kHAPError_InvalidData, decoded);
                    }
                    HAPRawBufferCopyBytes(
                            decoded.encryptedNotification.aaId, &bytes[6], sizeof decoded.encryptedNotification.aaId);
                    uint64_t nonce = (uint64_t) gsn;
                    uint8_t decodeBuffer[12];
                    HAPError err = HAPChaCha20Poly1305DecryptTruncatedTagHelper(
                            key,
                            nonce,
                            decodeBuffer,
                            &bytes[12],
                            16,
                            decoded.encryptedNotification.aaId,
                            sizeof decoded.encryptedNotification.aaId,
                            4);
                    if (err != kHAPError_None) {
                        return std::make_tuple(err, decoded);
                    }
                    decoded.encryptedNotification.gsn = HAPReadLittleUInt16(&decodeBuffer[0]);
                    decoded.encryptedNotification.iid = HAPReadLittleUInt16(&decodeBuffer[2]);
                    HAPRawBufferCopyBytes(
                            decoded.encryptedNotification.value,
                            &decodeBuffer[4],
                            sizeof decoded.encryptedNotification.value);
                } else {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                decoded.hasValidManufacturerData = true;
                break;
            }
            case 0x08: // HAP_FALLTHROUGH
            case 0x09: {
                // local name
                if (decoded.hasValidLocalName) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                if (adType == 0x09) {
                    decoded.isCompleteLocalName = true;
                }
                if (length >= sizeof decoded.localName) {
                    return std::make_tuple(kHAPError_InvalidData, decoded);
                }
                HAPRawBufferCopyBytes(decoded.localName, &bytes[2], length - 1);
                decoded.hasValidLocalName = true;
                break;
            }
            default: {
                HAPLogInfo(&kHAPLog_Default, "[WARN] Unknown BLE AdType 0x%02x", adType);
            }
        }
        numBytes -= 1 + length;
        bytes += 1 + length;
    }
    return std::make_tuple(kHAPError_None, decoded);
}

void HAPDecryptEventNotificationHelper(
        const void* eventBytes,
        size_t numEventBytes,
        const HAPSessionKey* decryptKey,
        uint64_t& decryptNonce,
        std::function<void(uint16_t, HAPTLV)> handleEventValue) {

    uint8_t decryptedBytes[numEventBytes];

    HAPError err = HAPChaCha20Poly1305DecryptHelper(
            decryptKey, decryptNonce, decryptedBytes, eventBytes, numEventBytes, NULL, 0);
    HAPAssert(err == kHAPError_None);
    decryptNonce++;
    numEventBytes -= CHACHA20_POLY1305_TAG_BYTES;

    HAPAssert(numEventBytes >= 5);
    HAPAssert(decryptedBytes[0] == 0x04); // notification
    uint16_t characteristicIID = (uint16_t) decryptedBytes[1] + (((uint16_t) decryptedBytes[2]) << 8);

    size_t bodyLength = (size_t) decryptedBytes[3] + ((size_t) decryptedBytes[4] << 8);
    HAPAssert(numEventBytes - 5 == bodyLength);

    HAPTLVReader bodyReader;
    HAPTLVReaderCreate(&bodyReader, &decryptedBytes[5], bodyLength);

    for (;;) {
        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(&bodyReader, &valid, &tlv);
        HAPAssert(!err);
        if (!valid) {
            break;
        }
        switch (tlv.type) {
            case kHAPPDUTLVType_Value: {
                if (handleEventValue) {
                    handleEventValue(characteristicIID, tlv);
                }
                break;
            }
            default: {
                // The following should always fail
                HAPAssert(tlv.type == kHAPPDUTLVType_Value);
                break;
            }
        }
    }
}
