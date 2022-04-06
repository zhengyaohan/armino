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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPCrypto.h"
#include "HAPLogSubsystem.h"
#include "HAPPairing.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "LegacyImport" };

HAP_RESULT_USE_CHECK
HAPError
        HAPLegacyImportDeviceID(HAPPlatformKeyValueStoreRef keyValueStore, const HAPAccessoryServerDeviceID* deviceID) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(deviceID);

    HAPError err;

    // Check if key-value store has already been configured.
    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_DeviceID,
            NULL,
            0,
            NULL,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPPrecondition(!found);

    // Store Device ID.
    HAPLogBufferInfo(&logObject, deviceID->bytes, sizeof deviceID->bytes, "Importing legacy Device ID.");
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_DeviceID,
            deviceID->bytes,
            sizeof deviceID->bytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportConfigurationNumber(HAPPlatformKeyValueStoreRef keyValueStore, uint32_t configurationNumber) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(configurationNumber);

    HAPError err;

    // Check if key-value store has already been configured.
    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
            NULL,
            0,
            NULL,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPPrecondition(!found);

    // Store configuration number.
    HAPLogInfo(&logObject, "Importing legacy configuration number: %lu.", (unsigned long) configurationNumber);
    uint8_t cnBytes[] = { HAPExpandLittleUInt32(configurationNumber) };
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
            cnBytes,
            sizeof cnBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_STATIC_ASSERT(
        sizeof(HAPAccessoryServerLongTermSecretKey) == ED25519_SECRET_KEY_BYTES,
        HAPAccessoryServerLongTermSecretKey_MatchesCrypto);

HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportLongTermSecretKey(
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPAccessoryServerLongTermSecretKey* longTermSecretKey) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(longTermSecretKey);

    HAPError err;

    // Check if key-value store has already been configured.
    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_LTSK,
            NULL,
            0,
            NULL,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPPrecondition(!found);

    // Store LTSK.
    HAPLogSensitiveBufferInfo(
            &logObject,
            longTermSecretKey->bytes,
            sizeof longTermSecretKey->bytes,
            "Importing legacy long-term secret key.");
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_LTSK,
            longTermSecretKey->bytes,
            sizeof longTermSecretKey->bytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportUnsuccessfulAuthenticationAttemptsCounter(
        HAPPlatformKeyValueStoreRef keyValueStore,
        uint8_t numAuthAttempts) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(numAuthAttempts <= 100);

    HAPError err;

    // Check if key-value store has already been configured.
    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
            NULL,
            0,
            NULL,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPPrecondition(!found);

    // Store unsuccessful authentication attempts counter.
    HAPLogSensitiveInfo(
            &logObject, "Importing legacy unsuccessful authentication attempts counter: %u.", numAuthAttempts);
    uint8_t numAuthAttemptsBytes[] = { numAuthAttempts };
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_NumUnsuccessfulAuthAttempts,
            numAuthAttemptsBytes,
            sizeof numAuthAttemptsBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_STATIC_ASSERT(sizeof(HAPControllerPublicKey) == ED25519_PUBLIC_KEY_BYTES, HAPControllerPublicKey_MatchesCrypto);

HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportControllerPairing(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreKey pairingIndex,
        const HAPControllerPairingIdentifier* pairingIdentifier,
        const HAPControllerPublicKey* publicKey,
        bool isAdmin) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(pairingIdentifier);
    HAPPrecondition(pairingIdentifier->numBytes <= sizeof pairingIdentifier->bytes);
    HAPPrecondition(pairingIdentifier->numBytes <= sizeof(HAPPairingID));
    HAPPrecondition(publicKey);

    HAPError err;

    // Check if key-value store has already been configured.
    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore, kHAPKeyValueStoreDomain_Pairings, pairingIndex, NULL, 0, NULL, &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPPrecondition(!found);

    // Store pairing.
    HAPLogInfo(&logObject, "Importing legacy pairing (%s).", isAdmin ? "admin" : "regular");
    HAPLogSensitiveBufferInfo(&logObject, pairingIdentifier->bytes, pairingIdentifier->numBytes, "Pairing identifier.");
    HAPLogSensitiveBufferInfo(&logObject, publicKey->bytes, sizeof publicKey->bytes, "Public key.");
    uint8_t pairingBytes[sizeof(HAPPairingID) + sizeof(uint8_t) + sizeof(HAPPairingPublicKey) + sizeof(uint8_t)];
    HAPRawBufferZero(pairingBytes, sizeof pairingBytes);
    HAPAssert(sizeof pairingIdentifier->bytes == 36);
    HAPRawBufferCopyBytes(&pairingBytes[0], pairingIdentifier->bytes, pairingIdentifier->numBytes);
    pairingBytes[36] = (uint8_t) pairingIdentifier->numBytes;
    HAPAssert(sizeof publicKey->bytes == 32);
    HAPRawBufferCopyBytes(&pairingBytes[37], publicKey->bytes, 32);
    pairingBytes[69] = isAdmin ? (uint8_t) 0x01 : (uint8_t) 0x00;

    err = HAPPlatformKeyValueStoreSet(
            keyValueStore, kHAPKeyValueStoreDomain_Pairings, pairingIndex, pairingBytes, sizeof pairingBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}
