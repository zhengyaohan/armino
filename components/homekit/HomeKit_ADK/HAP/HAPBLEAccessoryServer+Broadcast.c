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
#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer+Broadcast.h"
#include "HAPDeviceID.h"
#include "HAPLogSubsystem.h"
#include "HAPSession.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEAccessoryServer" };

typedef struct {
    uint16_t keyExpirationGSN;
    HAPBLEAccessoryServerBroadcastEncryptionKey key;
    bool hasAdvertisingID;
    HAPDeviceID advertisingID;
} HAPBLEAccessoryServerBroadcastParameters;

#define HAP_BLE_ACCESSORY_SERVER_GET_BROADCAST_PARAMETERS_OR_RETURN_ERROR(keyValueStore, parameters) \
    do { \
        bool found; \
        size_t numBytes; \
        uint8_t parametersBytes \
                [sizeof(uint16_t) + sizeof(HAPBLEAccessoryServerBroadcastEncryptionKey) + sizeof(uint8_t) + \
                 sizeof(HAPDeviceID)]; \
        err = HAPPlatformKeyValueStoreGet( \
                (keyValueStore), \
                kHAPKeyValueStoreDomain_Configuration, \
                kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters, \
                parametersBytes, \
                sizeof parametersBytes, \
                &numBytes, \
                &found); \
        if (err) { \
            HAPAssert(err == kHAPError_Unknown); \
            return err; \
        } \
\
        HAPRawBufferZero((parameters), sizeof *(parameters)); \
        if (found) { \
            if (numBytes != sizeof parametersBytes) { \
                HAPLog(&logObject, "Invalid BLE broadcast state length: %zu.", numBytes); \
                return kHAPError_Unknown; \
            } \
            (parameters)->keyExpirationGSN = HAPReadLittleUInt16(&parametersBytes[0]); \
            HAPAssert(sizeof(parameters)->key.value == 32); \
            HAPRawBufferCopyBytes((parameters)->key.value, &parametersBytes[2], 32); \
            (parameters)->hasAdvertisingID = (uint8_t)(parametersBytes[34] & 0x01U) == 0x01; \
            HAPAssert(sizeof(parameters)->advertisingID.bytes == 6); \
            HAPRawBufferCopyBytes((parameters)->advertisingID.bytes, &parametersBytes[35], 6); \
        } \
    } while (0)

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerBroadcastGetParameters(
        HAPAccessoryServer* server,
        uint16_t* keyExpirationGSN,
        HAPBLEAccessoryServerBroadcastEncryptionKey* _Nullable broadcastKey,
        HAPDeviceID* _Nullable advertisingID) {
    HAPPrecondition(server);
    HAPPrecondition(keyExpirationGSN);

    HAPError err;

    // Get parameters.
    HAPBLEAccessoryServerBroadcastParameters parameters;
    HAP_BLE_ACCESSORY_SERVER_GET_BROADCAST_PARAMETERS_OR_RETURN_ERROR(server->platform.keyValueStore, &parameters);

    // Copy result.
    *keyExpirationGSN = parameters.keyExpirationGSN;
    if (parameters.keyExpirationGSN) {
        if (broadcastKey) {
            HAPRawBufferCopyBytes(HAPNonnull(broadcastKey), &parameters.key, sizeof *broadcastKey);
            HAPLogSensitiveBufferDebug(
                    &logObject,
                    parameters.key.value,
                    sizeof parameters.key.value,
                    "BLE Broadcast Encryption Key (Expires after GSN %u).",
                    parameters.keyExpirationGSN);
        }
    }
    if (advertisingID) {
        if (parameters.hasAdvertisingID) {
            HAPRawBufferCopyBytes(HAPNonnull(advertisingID), &parameters.advertisingID, sizeof *advertisingID);
        } else {
            // Fallback to Device ID.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.4.2.2.2 Manufacturer Data
            err = HAPDeviceIDGet(server, HAPNonnull(advertisingID));
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerBroadcastGenerateKey(HAPSession* session, const HAPDeviceID* _Nullable advertisingID) {
    HAPPrecondition(session);
    HAPPrecondition(session->server);
    HAPAccessoryServer* server = session->server;
    HAPPrecondition(HAPSessionIsSecured(session));

    HAPError err;

    // Get state.
    HAPBLEAccessoryServerBroadcastParameters parameters;
    HAP_BLE_ACCESSORY_SERVER_GET_BROADCAST_PARAMETERS_OR_RETURN_ERROR(server->platform.keyValueStore, &parameters);

    // Get GSN.
    HAPBLEAccessoryServerGSN gsn;
    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // The broadcast encryption key shall expire and automatically and must be discarded by the
    // accessory after 32,767 (2^15 - 1) increments in GSN after the current broadcast key was
    // generated. The controller will normally refresh the broadcast key to ensure that they key does
    // not expire automatically on the accessory.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.7.4 Broadcast Encryption Key expiration and refresh
    uint32_t keyExpirationGSN = (uint32_t)(gsn.gsn + 32767 - 1);
    if (keyExpirationGSN > UINT16_MAX) {
        keyExpirationGSN -= UINT16_MAX;
    }
    HAPAssert(keyExpirationGSN != 0 && keyExpirationGSN <= UINT16_MAX);
    parameters.keyExpirationGSN = (uint16_t) keyExpirationGSN;

    // Fetch controller's Ed25519 long term public key.
    HAPAssert(session->hap.pairingID >= 0);

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(session->server, (HAPPairingIndex) session->hap.pairingID, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPAssert(exists);
    {
        // Generate encryption key.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.7.3 Broadcast Encryption Key Generation
        static const uint8_t info[] = "Broadcast-Encryption-Key";
        HAP_hkdf_sha512(
                parameters.key.value,
                sizeof parameters.key.value,
                session->hap.cv_KEY,
                sizeof session->hap.cv_KEY,
                pairing.publicKey.value,
                sizeof pairing.publicKey.value,
                info,
                sizeof info - 1);
        HAPLogSensitiveBufferDebug(
                &logObject, session->hap.cv_KEY, sizeof session->hap.cv_KEY, "Curve25519 shared secret.");
        HAPLogSensitiveBufferDebug(
                &logObject, pairing.publicKey.value, sizeof pairing.publicKey.value, "Controller LTPK.");
        HAPLogSensitiveBufferDebug(
                &logObject, parameters.key.value, sizeof parameters.key.value, "BLE Broadcast Encryption Key.");
    }
    HAPRawBufferZero(&pairing, sizeof pairing);

    // Copy advertising identifier.
    if (advertisingID) {
        parameters.hasAdvertisingID = true;
        HAPAssert(sizeof parameters.advertisingID == sizeof *advertisingID);
        HAPRawBufferCopyBytes(&parameters.advertisingID, HAPNonnull(advertisingID), sizeof parameters.advertisingID);
    }

    // Save.
    uint8_t parametersBytes
            [sizeof(uint16_t) + sizeof(HAPBLEAccessoryServerBroadcastEncryptionKey) + sizeof(uint8_t) +
             sizeof(HAPDeviceID)];
    HAPWriteLittleUInt16(&parametersBytes[0], parameters.keyExpirationGSN);
    HAPAssert(sizeof parameters.key.value == 32);
    HAPRawBufferCopyBytes(&parametersBytes[2], parameters.key.value, 32);
    parametersBytes[34] = parameters.hasAdvertisingID ? (uint8_t) 0x01 : (uint8_t) 0x00;
    HAPAssert(sizeof parameters.advertisingID.bytes == 6);
    HAPRawBufferCopyBytes(&parametersBytes[35], parameters.advertisingID.bytes, 6);
    err = HAPPlatformKeyValueStoreSet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters,
            parametersBytes,
            sizeof parametersBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerBroadcastSetAdvertisingID(
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPDeviceID* advertisingID) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(advertisingID);

    HAPError err;

    // Get state.
    HAPBLEAccessoryServerBroadcastParameters parameters;
    HAP_BLE_ACCESSORY_SERVER_GET_BROADCAST_PARAMETERS_OR_RETURN_ERROR(keyValueStore, &parameters);

    // Copy advertising identifier.
    parameters.hasAdvertisingID = true;
    HAPAssert(sizeof parameters.advertisingID == sizeof *advertisingID);
    HAPRawBufferCopyBytes(&parameters.advertisingID, advertisingID, sizeof parameters.advertisingID);

    // Save.
    uint8_t parametersBytes
            [sizeof(uint16_t) + sizeof(HAPBLEAccessoryServerBroadcastEncryptionKey) + sizeof(uint8_t) +
             sizeof(HAPDeviceID)];
    HAPWriteLittleUInt16(&parametersBytes[0], parameters.keyExpirationGSN);
    HAPAssert(sizeof parameters.key.value == 32);
    HAPRawBufferCopyBytes(&parametersBytes[2], parameters.key.value, 32);
    parametersBytes[34] = parameters.hasAdvertisingID ? (uint8_t) 0x01 : (uint8_t) 0x00;
    HAPAssert(sizeof parameters.advertisingID.bytes == 6);
    HAPRawBufferCopyBytes(&parametersBytes[35], parameters.advertisingID.bytes, 6);
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters,
            parametersBytes,
            sizeof parametersBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerBroadcastExpireKey(HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPLogInfo(&logObject, "Expiring broadcast encryption key.");

    // Get state.
    HAPBLEAccessoryServerBroadcastParameters parameters;
    HAP_BLE_ACCESSORY_SERVER_GET_BROADCAST_PARAMETERS_OR_RETURN_ERROR(keyValueStore, &parameters);

    // Expire encryption key.
    parameters.keyExpirationGSN = 0;
    HAPRawBufferZero(&parameters.key, sizeof parameters.key);

    // Save.
    uint8_t parametersBytes
            [sizeof(uint16_t) + sizeof(HAPBLEAccessoryServerBroadcastEncryptionKey) + sizeof(uint8_t) +
             sizeof(HAPDeviceID)];
    HAPWriteLittleUInt16(&parametersBytes[0], parameters.keyExpirationGSN);
    HAPAssert(sizeof parameters.key.value == 32);
    HAPRawBufferCopyBytes(&parametersBytes[2], parameters.key.value, 32);
    parametersBytes[34] = parameters.hasAdvertisingID ? (uint8_t) 0x01 : (uint8_t) 0x00;
    HAPAssert(sizeof parameters.advertisingID.bytes == 6);
    HAPRawBufferCopyBytes(&parametersBytes[35], parameters.advertisingID.bytes, 6);
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters,
            parametersBytes,
            sizeof parametersBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

#endif
