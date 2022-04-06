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

#include "HAP+KeyValueStoreDomains.h"
#include "HAP.h"
#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if (HAP_FEATURE_KEY_EXPORT == 1)

#include "HAPLogSubsystem.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Export" };

HAP_RESULT_USE_CHECK
HAPError HAPExportDeviceID(
        HAPPlatformKeyValueStoreRef keyValueStore,
        bool* valid,
        HAPAccessoryServerDeviceID* deviceID) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(valid);
    HAPPrecondition(deviceID);

    HAPError err;

    // Try to load Device ID.
    size_t numBytes;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_DeviceID,
            deviceID->bytes,
            sizeof deviceID->bytes,
            &numBytes,
            valid);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to load %s from key-value store.", "Device ID");
        return err;
    }
    if (*valid && numBytes != sizeof deviceID->bytes) {
        HAPLogError(&logObject, "Invalid %s in key-value store.", "Device ID");
        return kHAPError_Unknown;
    }
    if (!*valid) {
        HAPLog(&logObject, "No %s is provisioned in key-value store.", "Device ID");
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPExportLongTermSecretKey(
        HAPPlatformKeyValueStoreRef keyValueStore,
        bool* valid,
        HAPAccessoryServerLongTermSecretKey* longTermSecretKey) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(valid);
    HAPPrecondition(longTermSecretKey);

    HAPError err;

    // Try to load LTSK.
    size_t numBytes;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_LTSK,
            longTermSecretKey->bytes,
            sizeof longTermSecretKey->bytes,
            &numBytes,
            valid);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to load %s from key-value store.", "long-term secret key");
        return err;
    }
    if (*valid && numBytes != sizeof longTermSecretKey->bytes) {
        HAPLogError(&logObject, "Invalid %s in key-value store.", "long-term secret key");
        return kHAPError_Unknown;
    }
    if (!*valid) {
        HAPLog(&logObject, "No %s is provisioned in key-value store.", "long-term secret key");
    }

    return kHAPError_None;
}

typedef struct {
    HAPExportControllerPairingsCallback callback;
    void* _Nullable context;
} EnumerateControllerPairingsEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError ExportControllerPairingsEnumerateCallback(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue) {
    HAPPrecondition(context);
    EnumerateControllerPairingsEnumerateContext* arguments = context;
    HAPPrecondition(arguments->callback);
    HAPPrecondition(keyValueStore);
    HAPPrecondition(domain == kHAPKeyValueStoreDomain_Pairings);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    // Get pairing.
    bool found;
    size_t numBytes;
    uint8_t pairingBytes[sizeof(HAPPairingID) + sizeof(uint8_t) + sizeof(HAPPairingPublicKey) + sizeof(uint8_t)];
    err = HAPPlatformKeyValueStoreGet(keyValueStore, domain, key, pairingBytes, sizeof pairingBytes, &numBytes, &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to load pairing 0x%02X.", key);
        return err;
    }
    HAPAssert(found);
    if (numBytes != sizeof pairingBytes) {
        HAPLogError(&logObject, "Invalid pairing 0x%02X size %lu.", key, (unsigned long) numBytes);
        return kHAPError_Unknown;
    }
    HAPPairing pairing;
    HAPRawBufferZero(&pairing, sizeof pairing);
    HAPAssert(sizeof pairing.identifier.bytes == 36);
    HAPRawBufferCopyBytes(pairing.identifier.bytes, &pairingBytes[0], 36);
    pairing.numIdentifierBytes = pairingBytes[36];
    HAPAssert(sizeof pairing.publicKey.value == 32);
    HAPRawBufferCopyBytes(pairing.publicKey.value, &pairingBytes[37], 32);
    pairing.permissions = pairingBytes[69];
    if (pairing.numIdentifierBytes > sizeof pairing.identifier.bytes) {
        HAPLogError(&logObject, "Invalid pairing 0x%02X ID size %u.", key, pairing.numIdentifierBytes);
        return kHAPError_Unknown;
    }

    // Copy result.
    HAPControllerPairingIdentifier pairingIdentifier;
    HAPRawBufferZero(&pairingIdentifier, sizeof pairingIdentifier);
    HAPAssert(sizeof pairingIdentifier.bytes >= pairing.numIdentifierBytes);
    HAPRawBufferCopyBytes(pairingIdentifier.bytes, pairing.identifier.bytes, pairing.numIdentifierBytes);
    pairingIdentifier.numBytes = pairing.numIdentifierBytes;

    const HAPControllerPublicKey* publicKey;
    HAPAssert(sizeof publicKey->bytes == sizeof pairing.publicKey.value);
    publicKey = (const HAPControllerPublicKey*) &pairing.publicKey.value;

    bool isAdmin = (pairing.permissions & 0x01U) != 0;

    // Invoke callback.
    arguments->callback(arguments->context, keyValueStore, &pairingIdentifier, publicKey, isAdmin, shouldContinue);

    HAPRawBufferZero(&pairing, sizeof pairing);
    HAPRawBufferZero(&pairingIdentifier, sizeof pairingIdentifier);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPExportControllerPairings(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPExportControllerPairingsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(callback);

    HAPError err;

    EnumerateControllerPairingsEnumerateContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.callback = callback;
    enumerateContext.context = context;
    err = HAPPlatformKeyValueStoreEnumerate(
            keyValueStore,
            kHAPKeyValueStoreDomain_Pairings,
            ExportControllerPairingsEnumerateCallback,
            &enumerateContext);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

#endif
