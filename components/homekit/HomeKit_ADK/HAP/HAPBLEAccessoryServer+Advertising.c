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
#include "HAPAccessorySetup.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer+Broadcast.h"
#include "HAPBLEPeripheralManager.h"
#include "HAPCharacteristic.h"
#include "HAPDeviceID.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEAccessoryServer" };

// Forward reference
HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerIncrementGSN(HAPAccessoryServer* server);

HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerClearAllStatesDidIncrementGSNFlag(HAPAccessoryServer* server);
HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerSetCurrentStateDidIncrementGSNFlag(HAPAccessoryServer* server, bool didIncrement);
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetCurrentStateDidIncrementGSNFlag(HAPAccessoryServer* server, bool* didIncrement);

// Functions to set/get "Did Increment GSN" flags for the connected or disconnected state
static void SetDidIncrementGSNFlagConnectedState(HAPAccessoryServer* server, bool didIncrement);
static void GetDidIncrementGSNFlagConnectedState(HAPAccessoryServer* server, bool* didIncrement);
HAP_RESULT_USE_CHECK
static HAPError SetDidIncrementGSNFlagDisconnectedState(HAPAccessoryServer* server, bool didIncrement);
HAP_RESULT_USE_CHECK
static HAPError GetDidIncrementGSNFlagDisconnectedState(HAPAccessoryServer* server, bool* didIncrement);

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetGSN(HAPPlatformKeyValueStoreRef keyValueStore, HAPBLEAccessoryServerGSN* gsn) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(gsn);

    HAPError err;

    bool found;
    size_t numBytes;
    uint8_t gsnBytes[sizeof(uint16_t) + sizeof(uint8_t)];
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEGSN,
            gsnBytes,
            sizeof gsnBytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    HAPRawBufferZero(gsn, sizeof *gsn);
    gsn->gsn = 1;
    if (found) {
        if (numBytes != sizeof gsnBytes) {
            HAPLog(&logObject, "Invalid GSN length %zu.", numBytes);
            return kHAPError_Unknown;
        }
        gsn->gsn = HAPReadLittleUInt16(&gsnBytes[0]);
        gsn->didIncrementInDisconnectedState = (gsnBytes[2] & 0x01U) == 0x01;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetAdvertisingParameters(
        HAPAccessoryServer* server,
        bool* shouldAdvertisementBeActive,
        uint16_t* advertisingInterval,
        void* advertisingBytes,
        size_t maxAdvertisingBytes,
        size_t* numAdvertisingBytes,
        void* scanResponseBytes,
        size_t maxScanResponseBytes,
        size_t* numScanResponseBytes)
        HAP_DIAGNOSE_ERROR(maxAdvertisingBytes < 31, "maxAdvertisingBytes must be at least 31")
                HAP_DIAGNOSE_WARNING(maxScanResponseBytes < 2, "maxScanResponseBytes should be at least 2") {
    HAPPrecondition(server);
    HAPPrecondition(shouldAdvertisementBeActive);
    HAPPrecondition(advertisingInterval);
    HAPPrecondition(advertisingBytes);
    HAPPrecondition(maxAdvertisingBytes >= 31);
    HAPPrecondition(numAdvertisingBytes);
    HAPPrecondition(scanResponseBytes);
    HAPPrecondition(numScanResponseBytes);

    HAPError err;

    *shouldAdvertisementBeActive = true;
    *advertisingInterval = 0;
    *numAdvertisingBytes = 0;
    *numScanResponseBytes = 0;

    // The accessory shall not advertise while it is connected to a HomeKit controller.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.1.4 Advertising Interval
    if (server->ble.adv.connected) {
        *shouldAdvertisementBeActive = false;
        return kHAPError_None;
    }

    if (server->ble.adv.broadcastedEvent.iid) {
        // HAP BLE Encrypted Notification Advertisement Format.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.2 HAP BLE Encrypted Notification Advertisement Format

        uint16_t keyExpirationGSN;
        HAPBLEAccessoryServerBroadcastEncryptionKey broadcastKey;
        HAPDeviceID advertisingID;
        err = HAPBLEAccessoryServerBroadcastGetParameters(server, &keyExpirationGSN, &broadcastKey, &advertisingID);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
        if (!keyExpirationGSN) {
            HAPLog(&logObject, "Started broadcasted event without valid key. Corrupted data?");
            return kHAPError_Unknown;
        }
        HAPBLEAccessoryServerGSN gsn;
        err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // Interval.
        *advertisingInterval = 0;
        switch (server->ble.adv.broadcastedEvent.interval) {
            case kHAPBLECharacteristicBroadcastInterval_20Ms: {
                *advertisingInterval = HAPBLEAdvertisingIntervalCreateFromMilliseconds(20);
                break;
            }
            case kHAPBLECharacteristicBroadcastInterval_1280Ms: {
                *advertisingInterval = HAPBLEAdvertisingIntervalCreateFromMilliseconds(1280);
                break;
            }
            case kHAPBLECharacteristicBroadcastInterval_2560Ms: {
                *advertisingInterval = HAPBLEAdvertisingIntervalCreateFromMilliseconds(2560);
                break;
            }
        }
        HAPAssert(*advertisingInterval);

        uint8_t* adv = advertisingBytes;

        // Flags.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.2.1 Flags
        /* 0x00   LEN */ *adv++ = 0x02;
        /* 0x01   ADT */ *adv++ = 0x01;
        /* 0x02 Flags */ *adv++ = 0U << 0U | //  NO: LE Limited Discoverable Mode.
                                  1U << 1U | // YES: LE General Discoverable Mode.
                                  1U << 2U | // YES: BR/EDR Not Supported.
                                  0U << 3U | //  NO: Simultaneous LE and BR/EDR to Same Device Capable (Controller).
                                  0U << 4U | //  NO: Simultaneous LE and BR/EDR to Same Device Capable (Host).
                                  0U << 5U | 0U << 6U | 0U << 7U; // Reserved.

        // Manufacturer data.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.2.2 Manufacturer Data
        /* 0x00   LEN */ *adv++ = 0x1B;
        /* 0x01   ADT */ *adv++ = 0xFF;
        /* 0x02  CoID */ HAPWriteLittleUInt16(adv, 0x004CU);
        adv += 2;
        /* 0x04    TY */ *adv++ = 0x11;
        /* 0x05   STL */ *adv++ = 0x36;
        /* 0x06 AdvID */ {
            HAPAssert(sizeof advertisingID.bytes == 6);
            HAPRawBufferCopyBytes(adv, advertisingID.bytes, sizeof advertisingID.bytes);
            adv += sizeof advertisingID.bytes;
        }
        uint8_t* encryptedBytes = adv;
        /* 0x0C   GSN */ HAPWriteLittleUInt16(adv, gsn.gsn);
        adv += 2;
        /* 0x0E   IID */ HAPWriteLittleUInt16(adv, server->ble.adv.broadcastedEvent.iid);
        adv += 2;
        /* 0x10 Value */ {
            HAPRawBufferCopyBytes(
                    adv, server->ble.adv.broadcastedEvent.value, sizeof server->ble.adv.broadcastedEvent.value);
            adv += sizeof server->ble.adv.broadcastedEvent.value;
        }
        /* 0x18   Tag */ {
            // See HomeKit Accessory Protocol Specification R17
            // Section 5.10 AEAD Algorithm.
            // See HomeKit Accessory Protocol Specification R17
            // Section 7.4.7.3 Broadcast Encryption Key Generation
            uint8_t tagBytes[CHACHA20_POLY1305_TAG_BYTES];
            uint8_t nonceBytes[] = { HAPExpandLittleUInt64((uint64_t) gsn.gsn) };
            HAP_chacha20_poly1305_encrypt_aad(
                    tagBytes,
                    encryptedBytes,
                    encryptedBytes,
                    (size_t)(adv - encryptedBytes),
                    advertisingID.bytes,
                    sizeof advertisingID.bytes,
                    nonceBytes,
                    sizeof nonceBytes,
                    broadcastKey.value);
            HAPRawBufferCopyBytes(adv, tagBytes, 4);
            adv += 4;
        }

        *numAdvertisingBytes = (size_t)(adv - (uint8_t*) advertisingBytes);
        HAPAssert(*numAdvertisingBytes <= maxAdvertisingBytes);

        // Log.
        adv = advertisingBytes;
        adv += 3;
        HAPLogInfo(
                &logObject,
                "HAP BLE Encrypted Notification Advertisement Format (Manufacturer Data).\n"
                "-   LEN = 0x%02X\n"
                "-   ADT = 0x%02X\n"
                "-  CoID = 0x%04X\n"
                "-    TY = 0x%02X\n"
                "-   STL = 0x%02X\n"
                "- AdvID = %02X:%02X:%02X:%02X:%02X:%02X\n"
                "-    Ev = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"
                "          -   GSN = %u\n"
                "          -   IID = 0x%04X\n"
                "          - Value = 0x%02X%02X%02X%02X%02X%02X%02X%02X\n"
                "-   Tag = 0x%02X%02X%02X%02X",
                adv[0x00],
                adv[0x01],
                HAPReadLittleUInt16(&adv[0x02]),
                adv[0x04],
                adv[0x05],
                adv[0x06],
                adv[0x07],
                adv[0x08],
                adv[0x09],
                adv[0x0A],
                adv[0x0B],
                adv[0x0C],
                adv[0x0D],
                adv[0x0E],
                adv[0x0F],
                adv[0x10],
                adv[0x11],
                adv[0x12],
                adv[0x13],
                adv[0x14],
                adv[0x15],
                adv[0x16],
                adv[0x17],
                gsn.gsn,
                server->ble.adv.broadcastedEvent.iid,
                server->ble.adv.broadcastedEvent.value[0],
                server->ble.adv.broadcastedEvent.value[1],
                server->ble.adv.broadcastedEvent.value[2],
                server->ble.adv.broadcastedEvent.value[3],
                server->ble.adv.broadcastedEvent.value[4],
                server->ble.adv.broadcastedEvent.value[5],
                server->ble.adv.broadcastedEvent.value[6],
                server->ble.adv.broadcastedEvent.value[7],
                adv[0x18],
                adv[0x19],
                adv[0x1A],
                adv[0x1B]);
    } else {
        // HAP BLE Regular Advertisement Format.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.1 HAP BLE Regular Advertisement Format

        // Interval.
        // - 20 ms for first 30 seconds after boot.
        //   See Accessory Design Guidelines for Apple Devices R7
        //   Section 11.5 Advertising Interval
        // - 20 ms for first 3 seconds after Disconnected Event.
        //   See HomeKit Accessory Protocol Specification R17
        //   Section 7.4.6.3 Disconnected Events
        // - Regular advertising interval, otherwise.
        *advertisingInterval = (server->ble.adv.timer || !server->ble.adv.fast_started || server->ble.adv.fast_timer) ?
                                       HAPBLEAdvertisingIntervalCreateFromMilliseconds(20) :
                                       server->ble.adv.interval;

        uint8_t* adv = advertisingBytes;

        // Get setup ID.
        HAPSetupID setupID;
        bool hasSetupID = false;
        HAPPlatformAccessorySetupLoadSetupID(server->platform.accessorySetup, &hasSetupID, &setupID);

        // Flags.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.1.1 Flags
        /* 0x00   LEN */ *adv++ = 0x02;
        /* 0x01   ADT */ *adv++ = 0x01;
        /* 0x02 Flags */ *adv++ = 0U << 0U | //  NO: LE Limited Discoverable Mode.
                                  1U << 1U | // YES: LE General Discoverable Mode.
                                  1U << 2U | // YES: BR/EDR Not Supported.
                                  0U << 3U | //  NO: Simultaneous LE and BR/EDR to Same Device Capable (Controller).
                                  0U << 4U | //  NO: Simultaneous LE and BR/EDR to Same Device Capable (Host).
                                  0U << 5U | 0U << 6U | 0U << 7U; // Reserved.

        // Manufacturer data.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.1.2 Manufacturer Data
        /* 0x00   LEN */ *adv++ = (uint8_t)(0x12 + (hasSetupID ? 4 : 0));
        /* 0x01   ADT */ *adv++ = 0xFF;
        /* 0x02  CoID */ HAPWriteLittleUInt16(adv, 0x004CU);
        adv += 2;
        /* 0x04    TY */ *adv++ = 0x06;
        /* 0x05   STL */ *adv++ = (uint8_t)(0x2D + (hasSetupID ? 4 : 0));
        /* 0x06    SF */ *adv++ = (uint8_t)(HAPAccessoryServerIsPaired(server) ? 0U << 0U : 1U << 0U);
        /* 0x07 DevID */ {
            HAPDeviceID deviceID;
            err = HAPDeviceIDGet(server, &deviceID);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
            HAPAssert(sizeof deviceID.bytes == 6);
            HAPRawBufferCopyBytes(adv, deviceID.bytes, sizeof deviceID.bytes);
            adv += sizeof deviceID.bytes;
        }
        /* 0x0D  ACID */ HAPWriteLittleUInt16(adv, (uint16_t) server->primaryAccessory->category);
        adv += 2;
        /* 0x0F   GSN */ {
            HAPBLEAccessoryServerGSN gsn;
            err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
            HAPWriteLittleUInt16(adv, gsn.gsn);
            adv += 2;
        }
        /* 0x11    CN */ {
            uint16_t cn;
            err = HAPAccessoryServerGetCN(server->platform.keyValueStore, &cn);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
            *adv++ = (uint8_t)((cn - 1) % UINT8_MAX + 1);
        }
        /* 0x12    CV */ *adv++ = 0x02;
        /* 0x13    SH */ {
            if (hasSetupID) {
                // Get Device ID string.
                HAPDeviceIDString deviceIDString;
                err = HAPDeviceIDGetAsString(server, &deviceIDString);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    return err;
                }
                // Get setup hash.
                HAPAccessorySetupSetupHash setupHash;
                HAPAccessorySetupGetSetupHash(&setupHash, &setupID, &deviceIDString);

                // Append.
                HAPAssert(sizeof setupHash.bytes == 4);
                HAPRawBufferCopyBytes(adv, setupHash.bytes, sizeof setupHash.bytes);
                adv += sizeof setupHash.bytes;
            }
        }

        // Name.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.2.1.3 Local Name
        size_t numNameBytes = HAPStringGetNumBytes(server->primaryAccessory->name);
        HAPAssert(numNameBytes < UINT8_MAX);
        size_t maxNameBytesInAdvertisingData = maxAdvertisingBytes - (size_t)(adv - (uint8_t*) advertisingBytes) - 2;
        if (numNameBytes > maxNameBytesInAdvertisingData) {
            // When the advertisement includes the shortened local name
            // the accessory should include the complete local name in the Scan Response.
            if (maxScanResponseBytes >= 2) {
                uint8_t* sr = scanResponseBytes;

                size_t maxNameBytesInScanResponseData = maxScanResponseBytes - 2;
                if (numNameBytes > maxNameBytesInScanResponseData) {
                    numNameBytes = maxNameBytesInScanResponseData;
                    /* 0x00   LEN */ *sr++ = (uint8_t)(numNameBytes + 1);
                    /* 0x01   ADT */ *sr++ = 0x08;
                } else {
                    /* 0x00   LEN */ *sr++ = (uint8_t)(numNameBytes + 1);
                    /* 0x01   ADT */ *sr++ = 0x09;
                }
                /* 0x02  Name */ {
                    HAPRawBufferCopyBytes(sr, server->primaryAccessory->name, numNameBytes);
                    sr += numNameBytes;
                }

                *numScanResponseBytes = (size_t)(sr - (uint8_t*) scanResponseBytes);
                HAPAssert(*numScanResponseBytes <= maxScanResponseBytes);
            }

            numNameBytes = maxNameBytesInAdvertisingData;
            /* 0x00   LEN */ *adv++ = (uint8_t)(numNameBytes + 1);
            /* 0x01   ADT */ *adv++ = 0x08;
        } else {
            /* 0x00   LEN */ *adv++ = (uint8_t)(numNameBytes + 1);
            /* 0x01   ADT */ *adv++ = 0x09;
        }
        /* 0x02  Name */ {
            HAPRawBufferCopyBytes(adv, server->primaryAccessory->name, numNameBytes);
            adv += numNameBytes;
        }

        *numAdvertisingBytes = (size_t)(adv - (uint8_t*) advertisingBytes);
        HAPAssert(*numAdvertisingBytes <= maxAdvertisingBytes);

        // Log.
        adv = advertisingBytes;
        adv += 3;
        char setupHashLog[12 + 4 * 2 + 1 + 1];
        if (hasSetupID) {
            err = HAPStringWithFormat(
                    setupHashLog,
                    sizeof setupHashLog,
                    "\n-    SH = 0x%02X%02X%02X%02X",
                    adv[0x13],
                    adv[0x14],
                    adv[0x15],
                    adv[0x16]);
            HAPAssert(!err);
        }
        HAPLogInfo(
                &logObject,
                "HAP BLE Regular Advertisement Format (Manufacturer Data).\n"
                "-   LEN = 0x%02X\n"
                "-   ADT = 0x%02X\n"
                "-  CoID = 0x%04X\n"
                "-    TY = 0x%02X\n"
                "-   STL = 0x%02X\n"
                "-    SF = 0x%02X\n"
                "- DevID = %02X:%02X:%02X:%02X:%02X:%02X\n"
                "-  ACID = %u\n"
                "-   GSN = %u\n"
                "-    CN = %u\n"
                "-    CV = 0x%02X"
                "%s",
                adv[0x00],
                adv[0x01],
                HAPReadLittleUInt16(&adv[0x02]),
                adv[0x04],
                adv[0x05],
                adv[0x06],
                adv[0x07],
                adv[0x08],
                adv[0x09],
                adv[0x0A],
                adv[0x0B],
                adv[0x0C],
                HAPReadLittleUInt16(&adv[0x0D]),
                HAPReadLittleUInt16(&adv[0x0F]),
                adv[0x11],
                adv[0x12],
                hasSetupID ? setupHashLog : "");
    }

    float advertisingIntervalMilliseconds = HAPBLEAdvertisingIntervalGetMilliseconds(*advertisingInterval);
    HAPLogBufferInfo(
            &logObject,
            advertisingBytes,
            *numAdvertisingBytes,
            "ADV data: Active = %d, Interval = %u.%03u ms.",
            *shouldAdvertisementBeActive,
            (uint16_t) advertisingIntervalMilliseconds,
            (uint16_t)((uint32_t)(advertisingIntervalMilliseconds * 1000) % 1000));
    if (scanResponseBytes && *numScanResponseBytes) {
        HAPLogBufferInfo(&logObject, scanResponseBytes, *numScanResponseBytes, "SR data.");
    }

    return kHAPError_None;
}

static void AdvertisingTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    if (timer == server->ble.adv.fast_timer) {
        HAPLogDebug(&logObject, "Fast advertisement timer expired.");
        server->ble.adv.fast_timer = 0;
    } else if (timer == server->ble.adv.timer) {
        if (server->ble.adv.timerExpiryClock == 0 || HAPPlatformClockGetCurrent() < server->ble.adv.timerExpiryClock) {
            HAPLogDebug(&logObject, "Ignoring obsolete advertisement timer expiry");
            return;
        }
        HAPLogDebug(&logObject, "Advertisement timer expired.");
        server->ble.adv.timer = 0;
    } else {
        // This could legitimately happen as a race condition.
        HAPLogDebug(&logObject, "Ignoring obsolete advertisement timer expiry");
        return;
    }

    HAPError err;

    HAPAssert(!server->ble.adv.connected);

    // If no controller connects to the accessory within the 3 second broadcast period then the accessory must fall
    // back to the Disconnected Events advertisement rule with its current GSN as specified in `Disconnected Events`.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.6.2 Broadcasted Events
    if (server->ble.adv.broadcastedEvent.iid && !server->ble.adv.timer) {
        HAPRawBufferZero(&server->ble.adv.broadcastedEvent, sizeof server->ble.adv.broadcastedEvent);

        // Clear DidIncrementGSN flag so that next time a characteristic value changes, GSN would increment.
        // This is an additional change to the R17 spec.
        // The reasoning is that broadcast might have been received by a controller but acknowledge failed somehow,
        // in which case, if GSN stayed the same for another event, the controller would falsely consider
        // no change in the state.
        err = HAPBLEAccessoryServerSetCurrentStateDidIncrementGSNFlag(server, false);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(
                    &logObject, "%s: Failed to clear GSN increment flag. Future GSN might go out of sync.", __func__);
        }

        // If there are one or more queued broadcast events, increment GSN,
        // so that the GSN would indicate more characteristic changes beyond the last broadcasted event.
        if (server->ble.adv.queuedBroadcastEvents.numQueuedEvents > 0) {
            err = HAPBLEAccessoryServerIncrementGSN(server);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
            }
        }

        // Purge all queued broadcast events as well.
        HAPRawBufferZero(&server->ble.adv.queuedBroadcastEvents, sizeof server->ble.adv.queuedBroadcastEvents);

        // After updating the GSN as specified in Section `HAP BLE Regular Advertisement Format` in the disconnected
        // state the accessory must use a 20 ms advertising interval for at least 3 seconds.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.6.3 Disconnected Events
        server->ble.adv.timerExpiryClock = HAPPlatformClockGetCurrent() + HAPMillisecond * server->ble.adv.ev_duration;
        err = HAPPlatformTimerRegister(
                &server->ble.adv.timer, server->ble.adv.timerExpiryClock, AdvertisingTimerExpired, server);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject, "Not enough resources to start disconnected event timer!");
        }
    }

    // Update advertisement parameters.
    HAPAccessoryServerUpdateAdvertisingData(server);
}

void HAPBLEAccessoryAdvertisingTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPLog(&logObject, "Advertising timer expired");
    AdvertisingTimerExpired(timer, context);
}

void HAPBLEAccessoryServerDidStartAdvertising(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    // For first 30 seconds after boot, use 20ms as regular advertisement interval.
    if (!server->ble.adv.fast_started) {
        HAPAssert(!server->ble.adv.fast_timer);
        server->ble.adv.fast_started = true;
        err = HAPPlatformTimerRegister(
                &server->ble.adv.fast_timer,
                HAPPlatformClockGetCurrent() + 30 * HAPSecond,
                AdvertisingTimerExpired,
                server);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject,
                   "Not enough resources to start fast initial advertisement timer. Using regular interval!");
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidConnect(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(!server->ble.adv.connected);

    HAPError err;

    server->ble.adv.connected = true;

    // Stop fast advertisement timer.
    if (server->ble.adv.fast_timer) {
        HAPPlatformTimerDeregister(server->ble.adv.fast_timer);
        server->ble.adv.fast_timer = 0;
    }

    // Stop timer for disconnected and broadcasted events.
    // If a controller connects to the accessory before the completion of the 3 second advertising period the accessory
    // should abort the encrypted advertisement and continue with its regular advertisement at the regular advertising
    // period after the controller disconnects.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.6.2 Broadcasted Events
    if (server->ble.adv.timer) {
        HAPPlatformTimerDeregister(server->ble.adv.timer);
        server->ble.adv.timer = 0;
        server->ble.adv.timerExpiryClock = 0;
    }

    // Reset flag, disconnected events coalescing.
    err = HAPBLEAccessoryServerClearAllStatesDidIncrementGSNFlag(server);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Reset broadcasted events.
    HAPRawBufferZero(&server->ble.adv.broadcastedEvent, sizeof server->ble.adv.broadcastedEvent);

    // Update advertisement parameters.
    HAPAccessoryServerUpdateAdvertisingData(server);
    return kHAPError_None;
}

/**
 * Process a queued broadcast event if any
 *
 * @param server    accessory server
 */
static void ProcessQueuedBroadcastEvent(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (server->ble.adv.queuedBroadcastEvents.numQueuedEvents > 0) {
        // Dequeue and raise an event
        const HAPCharacteristic* characteristic = server->ble.adv.queuedBroadcastEvents.queue[0].characteristic;
        const HAPService* service = server->ble.adv.queuedBroadcastEvents.queue[0].service;
        const HAPAccessory* accessory = server->ble.adv.queuedBroadcastEvents.queue[0].accessory;
        if (server->ble.adv.queuedBroadcastEvents.numQueuedEvents > 1) {
            HAPRawBufferCopyBytes(
                    &server->ble.adv.queuedBroadcastEvents.queue[0],
                    &server->ble.adv.queuedBroadcastEvents.queue[1],
                    (sizeof server->ble.adv.queuedBroadcastEvents.queue[0]) *
                                    server->ble.adv.queuedBroadcastEvents.numQueuedEvents -
                            1);
        }
        server->ble.adv.queuedBroadcastEvents.numQueuedEvents--;

        HAPError error = HAPBLEAccessoryServerDidRaiseEvent(server, characteristic, service, accessory, NULL);
        HAPAssert(!error);
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidDisconnect(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->ble.adv.connected);

    HAPError err;

    server->ble.adv.connected = false;

    HAPAssert(server->ble.adv.fast_started);
    HAPAssert(!server->ble.adv.fast_timer);
    HAPAssert(!server->ble.adv.timer);

    // Allow quick reconnection.
    err = HAPPlatformTimerRegister(
            &server->ble.adv.fast_timer,
            HAPPlatformClockGetCurrent() + server->ble.adv.ev_duration,
            AdvertisingTimerExpired,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Not enough resources to start quick reconnection timer. Using regular interval!");
    }

    // Reset GSN update coalescing.
    err = HAPBLEAccessoryServerClearAllStatesDidIncrementGSNFlag(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    HAPAssert(!server->ble.adv.broadcastedEvent.iid);

    // Process a queued broadcast event before updating advertisement parameters
    // so that the advertisement parameters could reflect the broadcast event if any.
    if (server->state == kHAPAccessoryServerState_Running && !server->ble.shouldStopTransportWhenDisconnected) {
        ProcessQueuedBroadcastEvent(server);
        if (!server->ble.adv.timer) {
            // Update advertisement parameters only if advertisement wasn't already started from processing queued
            // broadcast event.
            HAPAccessoryServerUpdateAdvertisingData(server);
        }
    } else {
        // Update advertisement parameters.
        HAPAccessoryServerUpdateAdvertisingData(server);
    }

    // Proceed with shutdown, if requested.
    if (server->state != kHAPAccessoryServerState_Running) {
        HAPLogInfo(&logObject, "BLE connection disconnected. Proceeding with shutdown.");
        HAPAccessoryServerForceStop(server);
    } else if (server->ble.shouldStopTransportWhenDisconnected) {
        HAPLogInfo(&logObject, "BLE connection disconnected. Proceeding with transport shutdown.");
        HAPAccessoryServerStopBLETransport(server);
    }
    return kHAPError_None;
}

/**
 * Increments GSN, invalidating broadcast encryption key if necessary.
 *
 * @param      server              Accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerIncrementGSN(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    // Get key expiration GSN.
    uint16_t keyExpirationGSN;
    err = HAPBLEAccessoryServerBroadcastGetParameters(server, &keyExpirationGSN, NULL, NULL);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Get GSN.
    HAPBLEAccessoryServerGSN gsn;
    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Expire broadcast encryption key if necessary.
    if (gsn.gsn == keyExpirationGSN) {
        err = HAPBLEAccessoryServerBroadcastExpireKey(server->platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    // Increment GSN.
    if (gsn.gsn == UINT16_MAX) {
        gsn.gsn = 1;
    } else {
        gsn.gsn++;
    }

    if (server->ble.adv.connected) {
        SetDidIncrementGSNFlagConnectedState(server, true);
    } else {
        if (!gsn.didIncrementInDisconnectedState) {
            gsn.didIncrementInDisconnectedState = true;
        } else {
            HAPLogInfo(
                    &logObject,
                    "%s: GSN increment flag is already set to 1. GSN may have been unnecessarily incremented.",
                    __func__);
        }
    }

    HAPLogInfo(&logObject, "New GSN: %u.", gsn.gsn);

    // Save GSN state.
    uint8_t gsnBytes[] = { HAPExpandLittleUInt16(gsn.gsn),
                           gsn.didIncrementInDisconnectedState ? (uint8_t) 0x01 : (uint8_t) 0x00 };
    err = HAPPlatformKeyValueStoreSet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEGSN,
            gsnBytes,
            sizeof gsnBytes);
    if (err) {
        HAPLogError(&logObject, "Failed to update GSN and GSN increment flag. Future GSN might go out of sync.");
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
// GSN "Did Increment" Flag Functions
// -Functions to automatically set/get flag value based on the connection state.
// -Functions to set/get flag value for  current connected/disconnected state.
//----------------------------------------------------------------------------------------------------------------------

/**
 * Clears the value of the "Did Increment GSN" flags for both the connected and disconnected state.
 *
 * @param      server                   Accessory server.
 *
 * @return kHAPError_None if successfully cleared, otherwise relevant error code.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerClearAllStatesDidIncrementGSNFlag(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPError err;

    // Clear Did Increment GSN flag for Connected State
    SetDidIncrementGSNFlagConnectedState(server, false);

    // Clear Did Increment GSN flag for Disconnected State
    err = SetDidIncrementGSNFlagDisconnectedState(server, false);
    if (err) {
        HAPLogError(&logObject, "%s: Unable to clear GSN increment flag for Disconnected state.", __func__);
        return err;
    }

    return kHAPError_None;
}

/**
 * Sets the value of the "Did Increment GSN" flag for the current connected/disconnected state.
 *
 * -If the current state is the Connected State, sets the value of the "Did Increment GSN" flag for
 * the connected state.
 * - * -If the current state is the Disconnected State, sets the value of the "Did Increment GSN" flag for
 * the disconnected state.
 *
 * If the GSN has been incremented in the current state, the value is set to true. If the GSN
 * has not been incremented, the value is false.
 *
 * @param      server                   Accessory server.
 * @param      didIncrement             True if GSN has been incremented, False if not.
 *
 * @return kHAPError_None if successfully set, otherwise relevant error code.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPBLEAccessoryServerSetCurrentStateDidIncrementGSNFlag(HAPAccessoryServer* server, bool didIncrement) {
    HAPPrecondition(server);

    if (server->ble.adv.connected) {
        // In connected state
        SetDidIncrementGSNFlagConnectedState(server, didIncrement);
        return kHAPError_None;
    } else {
        // In disconnected state
        return SetDidIncrementGSNFlagDisconnectedState(server, didIncrement);
    }
}

/**
 * Gets the value of the "Did Increment GSN" flag for the current connected/disconnected state.
 *
 * -If the current state is the Connected State, gets the value of the "Did Increment GSN" flag for
 * the connected state.
 * - * -If the current state is the Disconnected State, gets the value of the "Did Increment GSN" flag for
 * the disconnected state.
 *
 * If the GSN has been incremented in the current state, the value is true. If the GSN
 * has not been incremented, the value is false.
 *
 * @param      server                   Accessory server.
 * @param[out] didIncrement             True if GSN has been incremented, False if not.
 *
 * @return kHAPError_None if successfully retrieved, otherwise relevant error code.
 */
HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerGetCurrentStateDidIncrementGSNFlag(HAPAccessoryServer* server, bool* didIncrement) {
    HAPPrecondition(server);
    HAPPrecondition(didIncrement);

    if (server->ble.adv.connected) {
        // In connected state
        GetDidIncrementGSNFlagConnectedState(server, didIncrement);
    } else {
        // In disconnected state
        HAPError err = GetDidIncrementGSNFlagDisconnectedState(server, didIncrement);
        if (err) {
            HAPLogError(&logObject, "%s: Unable to get GSN increment flag for Disconnected state.", __func__);
            return err;
        }
    }
    return kHAPError_None;
}

/**
 * Sets the value of the "Did Increment GSN" flag for connected state.
 *
 * If the GSN has been incremented in the current connect cycle, this function needs to be called to set the value to
 * true. If the connect/disconnect state changes, this function needs to be called to set the value to false.
 *
 * @param      server                   Accessory server.
 * @param      didIncrement             True to set value to true, False to set value to false
 */
static void SetDidIncrementGSNFlagConnectedState(HAPAccessoryServer* server, bool didIncrement) {
    HAPPrecondition(server);

    // If flag is already set to desired value, no need to update flag.
    if (server->ble.adv.connectedState.didIncrementGSN == didIncrement) {

        // If flag is already true, we may be unnecessarily incrementing the GSN. Log a message
        // for informational purposes.
        if (server->ble.adv.connectedState.didIncrementGSN) {
            HAPLogInfo(
                    &logObject,
                    "%s: GSN increment flag is already set to 1. GSN may have been unnecessarily incremented.",
                    __func__);
        }
        return;
    }

    HAPLog(&logObject,
           "%s: Updating GSN increment flag from %d to %d.",
           __func__,
           server->ble.adv.connectedState.didIncrementGSN,
           didIncrement);

    server->ble.adv.connectedState.didIncrementGSN = didIncrement;
}

/**
 * Gets the value of the "Did Increment GSN" flag for connected state.
 *
 * If the GSN has been incremented in the current connect cycle, the value is true. If the GSN
 * has not been incremented, the value is false.
 *
 * @param      server                   Accessory server.
 * @param[out] didIncrement             True if GSN has been incremented, False if not.
 */
static void GetDidIncrementGSNFlagConnectedState(HAPAccessoryServer* server, bool* didIncrement) {
    HAPPrecondition(server);
    HAPPrecondition(didIncrement);

    *didIncrement = server->ble.adv.connectedState.didIncrementGSN;
}

/**
 * Sets the value of the "Did Increment GSN" flag for disconnected state.
 *
 * If the GSN has been incremented in the current disconnect cycle, this function needs to be called to set the value to
 * true. If the connect/disconnect state changes, this function needs to be called to set the value to false.
 *
 * @param      server                   Accessory server.
 * @param      didIncrement             True to set value to true, False to set value to false
 *
 * @return kHAPError_None if successfully set, otherwise relevant error code.
 */
HAP_RESULT_USE_CHECK
static HAPError SetDidIncrementGSNFlagDisconnectedState(HAPAccessoryServer* server, bool didIncrement) {
    HAPPrecondition(server);

    HAPBLEAccessoryServerGSN gsn;
    HAPError err;

    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    if (err) {
        HAPLogError(&logObject, "%s: Failed to retrieve GSN.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // If flag is already set to desired value, no need to update flag.
    if (gsn.didIncrementInDisconnectedState == didIncrement) {
        // If flag is already true, we may be unnecessarily incrementing the GSN. Log a message
        // for informational purposes.
        if (gsn.didIncrementInDisconnectedState) {
            HAPLogInfo(
                    &logObject,
                    "%s: GSN increment flag is already set to 1. GSN may have been unnecessarily incremented.",
                    __func__);
        }
        return kHAPError_None;
    }

    HAPLog(&logObject,
           "%s: Updating GSN increment flag from %d to %d.",
           __func__,
           gsn.didIncrementInDisconnectedState,
           didIncrement);

    gsn.didIncrementInDisconnectedState = didIncrement;
    uint8_t gsnBytes[] = { HAPExpandLittleUInt16(gsn.gsn),
                           gsn.didIncrementInDisconnectedState ? (uint8_t) 0x01 : (uint8_t) 0x00 };
    err = HAPPlatformKeyValueStoreSet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEGSN,
            gsnBytes,
            sizeof gsnBytes);
    if (err) {
        HAPLogError(&logObject, "%s: Failed to update GSN increment flag. Future GSN might go out of sync.", __func__);
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

/**
 * Gets the value of the "Did Increment GSN" flag for disconnected state.
 *
 * If the GSN has been incremented in the current disconnect cycle, the value is true. If the GSN
 * has not been incremented, the value is false.
 *
 * @param      server                   Accessory server.
 * @param[out] didIncrement             True if GSN has been incremented, False if not.
 *
 * @return kHAPError_None if successfully retrieved, otherwise relevant error code.
 */
HAP_RESULT_USE_CHECK
static HAPError GetDidIncrementGSNFlagDisconnectedState(HAPAccessoryServer* server, bool* didIncrement) {
    HAPPrecondition(server);
    HAPPrecondition(didIncrement);

    HAPBLEAccessoryServerGSN gsn;
    HAPError err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    *didIncrement = gsn.didIncrementInDisconnectedState;

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
// Event Handling
//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* _Nullable session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(accessory->aid == 1);

    HAPError err;

    bool didUpdateAdvertisingData = false;
    if (characteristic->properties.ble.supportsBroadcastNotification) {
        // Broadcasted event.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.6.2 Broadcasted Events

        // If a controller connects to the accessory before the completion of the 3 second advertising period the
        // accessory should abort the encrypted advertisement and continue with its regular advertisement at the regular
        // advertising period after the controller disconnects.

        // Section 7.4.6.2 is going to be modified as follows:
        //
        // If a controller is connected, the broadcast event should be queued up to three characteristics
        // and broadcast will start upon disconnecting the controller.
        // If a broadcast of another characteristic is on-going, leave it as is and queue the new event.
        // If no controller connects to the accessory before the completion of the 3 second advertising period,
        // discard all the queued broadcast events.

        uint16_t keyExpirationGSN;
        err = HAPBLEAccessoryServerBroadcastGetParameters(server, &keyExpirationGSN, NULL, NULL);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
        HAPBLEAccessoryServerGSN gsn;
        err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // Characteristic changes while in a broadcast encryption key expired state shall not use broadcasted events
        // and must fall back to disconnected/connected events until the controller has re-generated a new broadcast
        // encryption key and re-registered characteristics for broadcasted notification.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.7.4 Broadcast Encryption Key expiration and refresh
        if (keyExpirationGSN && keyExpirationGSN != gsn.gsn) {
            HAPBLECharacteristicBroadcastInterval interval;
            bool enabled;
            err = HAPBLECharacteristicGetBroadcastConfiguration(
                    characteristic, service, accessory, &enabled, &interval, server->platform.keyValueStore);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }

            if (enabled) {
                bool queueEvent = false;
                if (server->ble.adv.connected) {
                    // Connected state
                    queueEvent = true;
                } else if (
                        server->ble.adv.timer && server->ble.adv.broadcastedEvent.iid != 0 &&
                        server->ble.adv.broadcastedEvent.iid != (uint16_t) characteristic->iid) {
                    // Another characteristic is broadcasted at the moment.
                    queueEvent = true;
                }

                if (!queueEvent) {
                    // For additional characteristic changes before the completion of the 3 second period and before
                    // a controller connection, the GSN should be updated again and the accessory must reflect the
                    // latest changed characteristic value in its encrypted advertisement and continue to broadcast
                    // for an additional 3 seconds from the last change.
                    // See HomeKit Accessory Protocol Specification R17
                    // Section 7.4.6.2 Broadcasted Events
                    if (server->ble.adv.timer) {
                        HAPPlatformTimerDeregister(server->ble.adv.timer);
                        server->ble.adv.timer = 0;
                        server->ble.adv.timerExpiryClock = 0;
                    }

                    // Cancel current broadcast.
                    server->ble.adv.broadcastedEvent.iid = 0;
                    HAPRawBufferZero(
                            server->ble.adv.broadcastedEvent.value, sizeof server->ble.adv.broadcastedEvent.value);
                    HAPAccessoryServerUpdateAdvertisingData(server);

                    // Fetch characteristic value.
                    // When the characteristic value is less than 8 bytes the remaining bytes shall be set to 0.
                    // See HomeKit Accessory Protocol Specification R17
                    // Section 7.4.2.2.2 Manufacturer Data
                    err = kHAPError_Unknown;
                    uint8_t* bytes = server->ble.adv.broadcastedEvent.value;
                    HAPAssert(sizeof server->ble.adv.broadcastedEvent.value == 8);
                    switch (characteristic->format) {
                        case kHAPCharacteristicFormat_Data: {
                            // Characteristics with format of string or data/tlv8 cannot be used
                            // in broadcast notifications
                            // See HomeKit Accessory Protocol Specification R17
                            // Section 7.4.2.2.2 Manufacturer Data
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "%s characteristic cannot be used in broadcast notifications.",
                                    "Data");
                            break;
                        }
                        case kHAPCharacteristicFormat_Bool: {
                            bool value;
                            err = HAPBoolCharacteristicHandleRead(
                                    server,
                                    &(const HAPBoolCharacteristicReadRequest) { .transportType = kHAPTransportType_BLE,
                                                                                .session = NULL,
                                                                                .characteristic = characteristic_,
                                                                                .service = service,
                                                                                .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            bytes[0] = (uint8_t)(value ? 1 : 0);
                            break;
                        }
                        case kHAPCharacteristicFormat_UInt8: {
                            uint8_t value;
                            err = HAPUInt8CharacteristicHandleRead(
                                    server,
                                    &(const HAPUInt8CharacteristicReadRequest) { .transportType = kHAPTransportType_BLE,
                                                                                 .session = NULL,
                                                                                 .characteristic = characteristic_,
                                                                                 .service = service,
                                                                                 .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            bytes[0] = value;
                            if (characteristic->properties.supportsEventNotificationContextInformation &&
                                ((HAPUInt8Characteristic*) characteristic)
                                        ->contextCallbacks.handleReadContextDataCompact) {
                                size_t remainingBytes = 7;
                                size_t startIndex = 8 - remainingBytes;
                                uint8_t compactContext[remainingBytes];
                                HAPRawBufferZero((void*) compactContext, remainingBytes);
                                err = ((HAPUInt8Characteristic*) characteristic)
                                              ->contextCallbacks.handleReadContextDataCompact(
                                                      server,
                                                      characteristic,
                                                      compactContext,
                                                      remainingBytes,
                                                      server->context);
                                if (err) {
                                    break;
                                }
                                HAPRawBufferCopyBytes(&bytes[startIndex], compactContext, remainingBytes);
                            }
                            break;
                        }
                        case kHAPCharacteristicFormat_UInt16: {
                            uint16_t value;
                            err = HAPUInt16CharacteristicHandleRead(
                                    server,
                                    &(const HAPUInt16CharacteristicReadRequest) { .transportType =
                                                                                          kHAPTransportType_BLE,
                                                                                  .session = NULL,
                                                                                  .characteristic = characteristic_,
                                                                                  .service = service,
                                                                                  .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            HAPWriteLittleUInt16(bytes, value);
                            break;
                        }
                        case kHAPCharacteristicFormat_UInt32: {
                            uint32_t value;
                            err = HAPUInt32CharacteristicHandleRead(
                                    server,
                                    &(const HAPUInt32CharacteristicReadRequest) { .transportType =
                                                                                          kHAPTransportType_BLE,
                                                                                  .session = NULL,
                                                                                  .characteristic = characteristic_,
                                                                                  .service = service,
                                                                                  .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            HAPWriteLittleUInt32(bytes, value);
                            break;
                        }
                        case kHAPCharacteristicFormat_UInt64: {
                            uint64_t value;
                            err = HAPUInt64CharacteristicHandleRead(
                                    server,
                                    &(const HAPUInt64CharacteristicReadRequest) { .transportType =
                                                                                          kHAPTransportType_BLE,
                                                                                  .session = NULL,
                                                                                  .characteristic = characteristic_,
                                                                                  .service = service,
                                                                                  .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            HAPWriteLittleUInt64(bytes, value);
                            break;
                        }
                        case kHAPCharacteristicFormat_Int: {
                            int32_t value;
                            err = HAPIntCharacteristicHandleRead(
                                    server,
                                    &(const HAPIntCharacteristicReadRequest) { .transportType = kHAPTransportType_BLE,
                                                                               .session = NULL,
                                                                               .characteristic = characteristic_,
                                                                               .service = service,
                                                                               .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            HAPWriteLittleInt32(bytes, value);
                            break;
                        }
                        case kHAPCharacteristicFormat_Float: {
                            float value;
                            err = HAPFloatCharacteristicHandleRead(
                                    server,
                                    &(const HAPFloatCharacteristicReadRequest) { .transportType = kHAPTransportType_BLE,
                                                                                 .session = NULL,
                                                                                 .characteristic = characteristic_,
                                                                                 .service = service,
                                                                                 .accessory = accessory },
                                    &value,
                                    server->context);
                            if (err) {
                                break;
                            }
                            uint32_t bitPattern = HAPFloatGetBitPattern(value);
                            HAPWriteLittleUInt32(bytes, bitPattern);
                            break;
                        }
                        case kHAPCharacteristicFormat_String: {
                            // Characteristics with format of string or data/tlv8 cannot be used
                            // in broadcast notifications.
                            // See HomeKit Accessory Protocol Specification R17
                            // Section 7.4.2.2.2 Manufacturer Data
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "%s characteristic cannot be used in broadcast notifications.",
                                    "String");
                        }
                            HAPFatalError();
                        case kHAPCharacteristicFormat_TLV8: {
                            // Characteristics with format of string or data/tlv8 cannot be used
                            // in broadcast notifications.
                            // See HomeKit Accessory Protocol Specification R17
                            // Section 7.4.2.2.2 Manufacturer Data
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "%s characteristic cannot be used in broadcast notifications.",
                                    "TLV8");
                        }
                            HAPFatalError();
                    }
                    if (err) {
                        HAPAssert(
                                err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                                err == kHAPError_OutOfResources || err == kHAPError_Busy);
                        HAPLogCharacteristic(
                                &logObject,
                                characteristic,
                                service,
                                accessory,
                                "Value for broadcast notification could not be received. Skipping event!");
                    } else {
                        // Increment GSN.
                        err = HAPBLEAccessoryServerIncrementGSN(server);
                        if (err) {
                            HAPAssert(err == kHAPError_Unknown);
                            return err;
                        }

                        server->ble.adv.timerExpiryClock =
                                HAPPlatformClockGetCurrent() + HAPMillisecond * server->ble.adv.ev_duration;
                        err = HAPPlatformTimerRegister(
                                &server->ble.adv.timer,
                                server->ble.adv.timerExpiryClock,
                                AdvertisingTimerExpired,
                                server);
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            HAPLogCharacteristicError(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Not enough resources to start broadcast event timer. Skipping event!");
                        } else {
                            // Initialize broadcasted event.
                            server->ble.adv.broadcastedEvent.interval = interval;
                            HAPAssert(characteristic->iid <= UINT16_MAX);
                            server->ble.adv.broadcastedEvent.iid = (uint16_t) characteristic->iid;
                            HAPLogCharacteristicInfo(
                                    &logObject, characteristic, service, accessory, "Broadcasted Event.");
                        }
                    }

                    // Update advertisement parameters.
                    HAPAccessoryServerUpdateAdvertisingData(server);
                    didUpdateAdvertisingData = true;
                } else {
                    // Queue the broadcast event for the future.
                    size_t i = 0;
                    for (; i < server->ble.adv.queuedBroadcastEvents.numQueuedEvents; i++) {
                        if (server->ble.adv.queuedBroadcastEvents.queue[i].characteristic == characteristic_ &&
                            server->ble.adv.queuedBroadcastEvents.queue[i].service == service &&
                            server->ble.adv.queuedBroadcastEvents.queue[i].accessory == accessory) {
                            HAPLogCharacteristicInfo(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Broadcasted Event - Already queued.");
                            // Though advertising data is not actually updated here,
                            // the GSN will be incremented later when the queued event is advertised.
                            // Hence, suppress disconnected event handling by setting the following flag.
                            didUpdateAdvertisingData = true;
                            break;
                        }
                    }
                    if (i == server->ble.adv.queuedBroadcastEvents.numQueuedEvents) {
                        if (server->ble.adv.queuedBroadcastEvents.numQueuedEvents < kMaxNumQueuedBLEBroadcastEvents) {
                            server->ble.adv.queuedBroadcastEvents.queue[i].characteristic = characteristic_;
                            server->ble.adv.queuedBroadcastEvents.queue[i].service = service;
                            server->ble.adv.queuedBroadcastEvents.queue[i].accessory = accessory;
                            server->ble.adv.queuedBroadcastEvents.numQueuedEvents++;
                            HAPLogCharacteristicInfo(
                                    &logObject, characteristic, service, accessory, "Broadcasted Event - Queued.");
                            // Though advertising data is not actually updated here,
                            // the GSN will be incremented later when the queued event is advertised.
                            // Hence, suppress disconnected event handling by setting the following flag.
                            didUpdateAdvertisingData = true;
                        } else {
                            HAPLogCharacteristicInfo(
                                    &logObject,
                                    characteristic,
                                    service,
                                    accessory,
                                    "Broadcasted Event - Skipped: Queue full.");
                        }
                    }
                }
            } else {
                HAPLogCharacteristicInfo(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Broadcasted Event - Skipping: Broadcasts disabled.");
            }
        } else {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Broadcasted Event - Skipping: Broadcast Key expired.");
        }
    }

    // Connected event is processed after broadcasted event in order to remove from the broadcast queue
    // if connected event is sent.
    if (characteristic->properties.supportsEventNotification) {
        // Connected event.
        if (server->ble.connection.connected && (!session || session == server->ble.storage->session)) {
            if (HAPCharacteristicIsHandlingWrite(
                        server, server->ble.storage->session, characteristic_, service, accessory)) {
                HAPLogCharacteristicInfo(
                        &logObject,
                        characteristic,
                        service,
                        accessory,
                        "Suppressing notification as the characteristic is currently being written.");
            } else {
                HAPBLEPeripheralManagerRaiseEvent(server, characteristic_, service, accessory);
            }
        }
    }

    if (characteristic->properties.ble.supportsDisconnectedNotification && !didUpdateAdvertisingData) {
        // Disconnected event.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.6.3 Disconnected Events

        bool gsnDidIncrement = false;
        err = HAPBLEAccessoryServerGetCurrentStateDidIncrementGSNFlag(server, &gsnDidIncrement);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // The GSN should increment only once for multiple characteristic value changes while in in disconnected state
        // until the accessory state changes from disconnected to connected.
        // See HomeKit Accessory Protocol Specification R17
        // Section 7.4.6.3 Disconnected Events
        if (!gsnDidIncrement) {
            HAPAssert(!server->ble.adv.broadcastedEvent.iid);

            if (server->ble.adv.timer) {
                // Advertisement timer may be running when gsnDidIncrement is false for three seconds after
                // gsnDidIncrement was cleared on purpose, after a broadcasted event failed to get acknowledged, i.e.,
                // for three seconds after broadcast notification timer expired without a controller connecting to the
                // accessory.
                // Note that gsnDidIncrement was cleared in order to increment GSN again for a disconnected event
                // so that the controller would know of a change in case it indeed did receive the broadcast
                // notification but failed to acknowledge for some reason.
                // These are the planned deviation from HomeKit Accessory Protocol Specification R17.
                HAPPlatformTimerDeregister(server->ble.adv.timer);
                server->ble.adv.timer = 0;
                server->ble.adv.timerExpiryClock = 0;
            }

            err = HAPBLEAccessoryServerIncrementGSN(server);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }

            if (!server->ble.adv.connected) {
                HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Disconnected Event.");

                // After updating the GSN as specified in Section HAP BLE Regular Advertisement Format in the
                // disconnected state the accessory must use a 20 ms advertising interval for at least 3 seconds.
                // See HomeKit Accessory Protocol Specification R17
                // Section 7.4.6.3 Disconnected Events
                server->ble.adv.timerExpiryClock =
                        HAPPlatformClockGetCurrent() + HAPMillisecond * server->ble.adv.ev_duration;
                err = HAPPlatformTimerRegister(
                        &server->ble.adv.timer, server->ble.adv.timerExpiryClock, AdvertisingTimerExpired, server);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLogCharacteristic(
                            &logObject,
                            characteristic,
                            service,
                            accessory,
                            "Not enough resources to start disconnected event timer!");
                }

                // Update advertisement parameters.
                HAPAccessoryServerUpdateAdvertisingData(server);
            } else {
                HAPLogCharacteristicInfo(
                        &logObject, characteristic, service, accessory, "Disconnected Event - Connected (no adv).");
            }
        } else {
            HAPLogCharacteristicInfo(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "Disconnected Event - Skipping: GSN already incremented.");
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPBLEAccessoryServerDidSendEventNotification(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic_,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(server->ble.adv.connected);
    HAPPrecondition(characteristic_);
    const HAPBaseCharacteristic* characteristic = characteristic_;
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(accessory->aid == 1);

    HAPError err;

    // Remove queued broadcast event for the characteristic if any,
    // so that the broadcast event won't be redundantly notified after connected event.
    HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(server, characteristic_, service, accessory);

    // After the first characteristic change on characteristics that are registered for Bluetooth LE indications in the
    // current connected state, the GSN shall also be incremented by 1 and reflected in the subsequent advertisements
    // after the current connection is disconnected. The GSN must increment only once for multiple characteristic
    // changes while in the current connected state.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.6.1 Connected Events

    bool gsnDidIncrement = false;
    err = HAPBLEAccessoryServerGetCurrentStateDidIncrementGSNFlag(server, &gsnDidIncrement);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // The GSN should increment only once for multiple characteristic value changes while in in disconnected state
    // until the accessory state changes from disconnected to connected.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.6.3 Disconnected Events
    if (!gsnDidIncrement) {
        HAPAssert(!server->ble.adv.broadcastedEvent.iid);
        HAPAssert(!server->ble.adv.timer);

        err = HAPBLEAccessoryServerIncrementGSN(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }

        // Not connected, so no advertisement parameter update necessary.
        HAPLogCharacteristicInfo(&logObject, characteristic, service, accessory, "Connected Event - GSN incremented.");
    } else {
        HAPLogCharacteristicInfo(
                &logObject, characteristic, service, accessory, "Connected Event - Skipping: GSN already incremented.");
    }

    return kHAPError_None;
}

void HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);

    size_t i;
    bool eventRemoved = false;
    for (i = 0; i < server->ble.adv.queuedBroadcastEvents.numQueuedEvents; i++) {
        if (server->ble.adv.queuedBroadcastEvents.queue[i].characteristic == characteristic &&
            server->ble.adv.queuedBroadcastEvents.queue[i].service == service &&
            server->ble.adv.queuedBroadcastEvents.queue[i].accessory == accessory) {
            HAPLogCharacteristicInfo(
                    &logObject, characteristic, service, accessory, "Broadcasted Event - Removed from queue");
            break;
        }
    }

    if (i < server->ble.adv.queuedBroadcastEvents.numQueuedEvents) {
        if (i + 1 < server->ble.adv.queuedBroadcastEvents.numQueuedEvents) {
            HAPRawBufferCopyBytes(
                    &server->ble.adv.queuedBroadcastEvents.queue[i],
                    &server->ble.adv.queuedBroadcastEvents.queue[i + 1],
                    (sizeof server->ble.adv.queuedBroadcastEvents.queue[0]) *
                            (server->ble.adv.queuedBroadcastEvents.numQueuedEvents - i - 1));
        }
        server->ble.adv.queuedBroadcastEvents.numQueuedEvents--;
        eventRemoved = true;
    }

    if (eventRemoved) {
        bool gsnDidIncrement = false;
        HAPError err = HAPBLEAccessoryServerGetCurrentStateDidIncrementGSNFlag(server, &gsnDidIncrement);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
        } else if (!gsnDidIncrement) {
            // The GSN should increment for changes to characteristics with Broadcast Notifications
            // enabled, even if the notification is purged before broadcast.
            err = HAPBLEAccessoryServerIncrementGSN(server);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
            }
        }
    }
}

#endif
