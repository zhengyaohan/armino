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
#include "HAPBLECharacteristic+Broadcast.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLECharacteristic" };

HAP_RESULT_USE_CHECK
bool HAPBLECharacteristicIsValidBroadcastInterval(uint8_t value) {
    switch (value) {
        case kHAPBLECharacteristicBroadcastInterval_20Ms:
        case kHAPBLECharacteristicBroadcastInterval_1280Ms:
        case kHAPBLECharacteristicBroadcastInterval_2560Ms: {
            return true;
        }
        default:
            return false;
    }
}

typedef struct {
    uint16_t aid;
    bool* found;
    void* bytes;
    size_t maxBytes;
    size_t* numBytes;
    HAPPlatformKeyValueStoreKey* key;
} GetBroadcastParametersEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError GetBroadcastConfigurationEnumerateCallback(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue) {
    HAPPrecondition(context);
    GetBroadcastParametersEnumerateContext* arguments = context;
    HAPPrecondition(arguments->aid);
    HAPPrecondition(arguments->aid == 1);
    HAPPrecondition(arguments->found);
    HAPPrecondition(!*arguments->found);
    HAPPrecondition(arguments->bytes);
    HAPPrecondition(arguments->maxBytes >= 2);
    HAPPrecondition(arguments->numBytes);
    HAPPrecondition(arguments->key);
    HAPPrecondition(domain == kHAPKeyValueStoreDomain_CharacteristicConfiguration);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    // Load.
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore, domain, key, arguments->bytes, arguments->maxBytes, arguments->numBytes, arguments->found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPAssert(arguments->found);
    if (*arguments->numBytes < 2 || *arguments->numBytes == arguments->maxBytes || (*arguments->numBytes - 2) % 3) {
        HAPLog(&logObject,
               "Invalid characteristic configuration 0x%02X size %lu.",
               key,
               (unsigned long) *arguments->numBytes);
        return kHAPError_Unknown;
    }

    // Check for match.
    if (HAPReadLittleUInt16(arguments->bytes) != arguments->aid) {
        *arguments->found = false;
        return kHAPError_None;
    }

    // Match found.
    *arguments->key = key;
    *shouldContinue = false;
    return kHAPError_None;
}

/**
 * Fetches the characteristic configuration for an accessory.
 *
 * @param      aid                  Accessory ID.
 * @param[out] found                Whether the characteristic configuration has been found.
 * @param[out] bytes                Buffer to store characteristic configuration, if found.
 * @param      maxBytes             Capacity of buffer. Must be at least 2 + 3 * #<concurrent active broadcasts> + 1.
 * @param[out] numBytes             Effective length of buffer, if found.
 * @param[out] key                  Key, if found.
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
static HAPError GetBroadcastConfiguration(
        uint16_t aid,
        bool* found,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        HAPPlatformKeyValueStoreKey* key,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(aid);
    HAPPrecondition(found);
    HAPPrecondition(bytes);
    HAPPrecondition(maxBytes >= 3);
    HAPPrecondition(numBytes);
    HAPPrecondition(key);
    HAPPrecondition(keyValueStore);

    HAPError err;

    *found = false;
    GetBroadcastParametersEnumerateContext context;
    HAPRawBufferZero(&context, sizeof context);
    context.aid = aid;
    context.found = found;
    context.bytes = bytes;
    context.maxBytes = maxBytes;
    context.numBytes = numBytes;
    context.key = key;
    err = HAPPlatformKeyValueStoreEnumerate(
            keyValueStore,
            kHAPKeyValueStoreDomain_CharacteristicConfiguration,
            GetBroadcastConfigurationEnumerateCallback,
            &context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicGetBroadcastConfiguration(
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        bool* broadcastsEnabled,
        HAPBLECharacteristicBroadcastInterval* broadcastInterval,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(characteristic->properties.ble.supportsBroadcastNotification);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(broadcastsEnabled);
    HAPPrecondition(broadcastInterval);
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPAssert(accessory->aid == 1);
    uint16_t aid = (uint16_t) accessory->aid;
    HAPAssert(characteristic->iid <= UINT16_MAX);
    uint16_t cid = (uint16_t) characteristic->iid;

    // Get configuration.
    HAPPlatformKeyValueStoreKey key;
    size_t numBytes;
    uint8_t bytes[2 + 3 * 42 + 1]; // 128 + 1, allows for 42 concurrent broadcasts on a single KVS key.
    bool found;
    err = GetBroadcastConfiguration(aid, &found, bytes, sizeof bytes, &numBytes, &key, keyValueStore);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        *broadcastsEnabled = false;
        return kHAPError_None;
    }
    HAPAssert(numBytes >= 2 && !((numBytes - 2) % 3));
    HAPAssert(HAPReadLittleUInt16(bytes) == aid);

    // Find characteristic.
    for (size_t i = 2; i < numBytes; i += 3) {
        uint16_t itemCID = HAPReadLittleUInt16(&bytes[i]);
        if (itemCID < cid) {
            continue;
        }
        if (itemCID > cid) {
            break;
        }

        // Found. Extract configuration.
        uint8_t broadcastConfiguration = bytes[i + 2];
        if (!HAPBLECharacteristicIsValidBroadcastInterval(broadcastConfiguration)) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Invalid stored broadcast interval: 0x%02x.",
                    broadcastConfiguration);
            return kHAPError_Unknown;
        }
        *broadcastsEnabled = true;
        *broadcastInterval = (HAPBLECharacteristicBroadcastInterval) broadcastConfiguration;
        return kHAPError_None;
    }

    // Not found.
    *broadcastsEnabled = false;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicEnableBroadcastNotifications(
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPBLECharacteristicBroadcastInterval broadcastInterval,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(characteristic->properties.ble.supportsBroadcastNotification);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(HAPBLECharacteristicIsValidBroadcastInterval(broadcastInterval));
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPLogCharacteristicInfo(
            &logObject,
            characteristic,
            service,
            accessory,
            "Enabling broadcasts (interval = 0x%02x).",
            broadcastInterval);

    HAPAssert(accessory->aid == 1);
    uint16_t aid = (uint16_t) accessory->aid;
    HAPAssert(characteristic->iid <= UINT16_MAX);
    uint16_t cid = (uint16_t) characteristic->iid;

    // Get configuration.
    HAPPlatformKeyValueStoreKey key;
    size_t numBytes;
    uint8_t bytes[2 + 3 * 42 + 1]; // 128 + 1, allows for 42 concurrent broadcasts on a single KVS key.
    bool found;
    err = GetBroadcastConfiguration(aid, &found, bytes, sizeof bytes, &numBytes, &key, keyValueStore);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        key = 0;
        HAPWriteLittleUInt16(bytes, aid);
        numBytes = 2;
    }
    HAPAssert(numBytes >= 2 && !((numBytes - 2) % 3));
    HAPAssert(HAPReadLittleUInt16(bytes) == aid);

    // Find characteristic.
    size_t i;
    for (i = 2; i < numBytes; i += 3) {
        uint16_t itemCID = HAPReadLittleUInt16(&bytes[i]);
        if (itemCID < cid) {
            continue;
        }
        if (itemCID > cid) {
            break;
        }

        // Found. Extract configuration.
        uint8_t broadcastConfiguration = bytes[i + 2];
        if (!HAPBLECharacteristicIsValidBroadcastInterval(broadcastConfiguration)) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Invalid stored broadcast interval: 0x%02x.",
                    broadcastConfiguration);
            return kHAPError_Unknown;
        }

        // Update configuration.
        if ((HAPBLECharacteristicBroadcastInterval) broadcastConfiguration == broadcastInterval) {
            return kHAPError_None;
        }
        bytes[i + 2] = broadcastInterval;
        err = HAPPlatformKeyValueStoreSet(
                keyValueStore, kHAPKeyValueStoreDomain_CharacteristicConfiguration, key, bytes, numBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
        return kHAPError_None;
    }

    // Add configuration.
    if (numBytes >= sizeof bytes - 1 - 3) {
        HAPLogCharacteristic(
                &logObject,
                characteristic,
                service,
                accessory,
                "Not enough space to store characteristic configuration.");
        return kHAPError_Unknown;
    }
    HAPRawBufferCopyBytes(&bytes[i + 3], &bytes[i], numBytes - i);
    HAPWriteLittleUInt16(&bytes[i], cid);
    bytes[i + 2] = broadcastInterval;
    numBytes += 3;
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore, kHAPKeyValueStoreDomain_CharacteristicConfiguration, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLECharacteristicDisableBroadcastNotifications(
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(characteristic->properties.ble.supportsBroadcastNotification);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Disabling broadcasts.");

    HAPAssert(accessory->aid == 1);
    uint16_t aid = (uint16_t) accessory->aid;
    HAPAssert(characteristic->iid <= UINT16_MAX);
    uint16_t cid = (uint16_t) characteristic->iid;

    // Get configuration.
    HAPPlatformKeyValueStoreKey key;
    size_t numBytes;
    uint8_t bytes[2 + 3 * 42 + 1]; // 128 + 1, allows for 42 concurrent broadcasts on a single KVS key.
    bool found;
    err = GetBroadcastConfiguration(aid, &found, bytes, sizeof bytes, &numBytes, &key, keyValueStore);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        return kHAPError_None;
    }
    HAPAssert(numBytes >= 2 && !((numBytes - 2) % 3));
    HAPAssert(HAPReadLittleUInt16(bytes) == aid);

    // Find characteristic.
    size_t i;
    for (i = 2; i < numBytes; i += 3) {
        uint16_t itemCID = HAPReadLittleUInt16(&bytes[i]);
        if (itemCID < cid) {
            continue;
        }
        if (itemCID > cid) {
            break;
        }

        // Found. Extract configuration.
        uint8_t broadcastConfiguration = bytes[i + 2];
        if (!HAPBLECharacteristicIsValidBroadcastInterval(broadcastConfiguration)) {
            HAPLogCharacteristic(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Invalid stored broadcast interval: 0x%02x.",
                    broadcastConfiguration);
            return kHAPError_Unknown;
        }

        // Remove configuration.
        numBytes -= 3;
        HAPRawBufferCopyBytes(&bytes[i], &bytes[i + 3], numBytes - i);
        if (numBytes == 2) {
            err = HAPPlatformKeyValueStoreRemove(
                    keyValueStore, kHAPKeyValueStoreDomain_CharacteristicConfiguration, key);
        } else {
            err = HAPPlatformKeyValueStoreSet(
                    keyValueStore, kHAPKeyValueStoreDomain_CharacteristicConfiguration, key, bytes, numBytes);
        }
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
        return kHAPError_None;
    }
    return kHAPError_None;
}

#endif
