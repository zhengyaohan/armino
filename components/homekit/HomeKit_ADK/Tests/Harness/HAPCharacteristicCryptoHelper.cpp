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

#include "HAPCharacteristicCryptoHelper.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPCryptoHelper.h"
#include "HAPPDU+TLV.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
void HAPReadCharacteristicOverThreadHelper(
        HAPAccessoryServer* server,
        HAPPlatformThreadCoAPManager* coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerRequestCallback requestCallback,
        void* requestContext,
        uint8_t transactionId,
        uint16_t iid,
        const HAPSessionKey* encryptKey,
        uint64_t& encryptNonce,
        const HAPSessionKey* decryptKey,
        uint64_t& decryptNonce,
        std::function<void(HAPPlatformThreadCoAPManagerResponseCode)> handleResponseCode,
        std::function<void(uint8_t)> handleResponseStatus,
        std::function<void(HAPTLV)> handleResponseValue) {
    // Original PDU
    uint8_t pdu[5];
    pdu[0] = 0x00; // control field - no fragmentation - request
    pdu[1] = 0x03; // HAP-Characteristic-Read
    pdu[2] = transactionId;
    pdu[3] = iid & 0xff;
    pdu[4] = (iid >> 8) & 0xff;
    uint8_t requestBytes[sizeof pdu + CHACHA20_POLY1305_TAG_BYTES];

    HAPChaCha20Poly1305EncryptHelper(encryptKey, encryptNonce, requestBytes, pdu, sizeof(pdu), NULL, 0);
    encryptNonce++;

    HAPPlatformThreadCoAPManagerResponseCode responseCode;
    uint8_t responseBytes[512];
    size_t numResponseBytes;
    requestCallback(
            coapManager,
            peer,
            server->thread.coap.secureMessageResource,
            kHAPPlatformThreadCoAPManagerRequestCode_POST,
            requestBytes,
            sizeof requestBytes,
            &responseCode,
            responseBytes,
            sizeof responseBytes,
            &numResponseBytes,
            NULL,
            requestContext);

    if (handleResponseCode) {
        handleResponseCode(responseCode);
    }

    if ((responseCode >> 5) != 2) {
        // Response is an error. Do not continue with decrypting the response.
        HAPAssert(numResponseBytes == 0);
        return;
    }

    HAPError err = HAPChaCha20Poly1305DecryptHelper(
            decryptKey, decryptNonce, responseBytes, responseBytes, numResponseBytes, NULL, 0);
    HAPAssert(err == kHAPError_None);
    decryptNonce++;
    numResponseBytes -= CHACHA20_POLY1305_TAG_BYTES;

    HAPAssert(numResponseBytes >= 3);
    HAPAssert(responseBytes[0] == 0x02); // Read response
    HAPAssert(responseBytes[1] == transactionId);
    if (handleResponseStatus) {
        handleResponseStatus(responseBytes[2]);
    }

    if (numResponseBytes > 3) {
        HAPAssert(numResponseBytes >= 5);
        size_t bodyLength = (size_t) responseBytes[3] + ((size_t) responseBytes[4] << 8);
        HAPAssert(numResponseBytes - 5 == bodyLength);

        HAPTLVReader bodyReader;
        HAPTLVReaderCreate(&bodyReader, &responseBytes[5], bodyLength);

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
                    // The value response must contain data uint16 value
                    if (handleResponseValue) {
                        handleResponseValue(tlv);
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
}
#endif
