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

#include "util_base64.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetup.h"
#include "HAPDeviceID.h"
#include "HAPIPAccessoryServer.h"
#include "HAPIPGlobalStateNumber.h"
#include "HAPServiceDiscoveryUtils.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
/**
 * TXT record keys for _hap service.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 6-7 _hap._tcp Bonjour TXT Record Keys
 */
/**@{*/
/** Current configuration number. */
#define kHAPTXTRecordKey_ConfigurationNumber "c#"

/** Pairing Feature flags. */
#define kHAPTXTRecordKey_PairingFeatureFlags "ff"

/** Device ID of the accessory. */
#define kHAPTXTRecordKey_DeviceID "id"

/** Model name of the accessory. */
#define kHAPTXTRecordKey_Model "md"

/** Protocol version string. */
#define kHAPTXTRecordKey_ProtocolVersion "pv"

/** Current state number. */
#define kHAPTXTRecordKey_StateNumber "s#"

/** Status flags. */
#define kHAPTXTRecordKey_StatusFlags "sf"

/** Accessory Category Identifier. */
#define kHAPTXTRecordKey_Category "ci"

/** Setup hash. */
#define kHAPTXTRecordKey_SetupHash "sh"
/**@}*/

#define kMaxNumTxtRecords 20

#define APPEND_TXT_RECORD(RECORDS, NUM, KEY, VAL) \
    HAPAssert(NUM < kMaxNumTxtRecords); \
    RECORDS[NUM++] = (HAPPlatformServiceDiscoveryTXTRecord) { \
        .key = KEY, .value = { .bytes = VAL, .numBytes = HAPStringGetNumBytes(VAL) } \
    };

void HAPServiceDiscoveryUtilsSetHAPService(HAPAccessoryServer* server, char* protocol, uint16_t port) {
    HAPPrecondition(server);

    HAPPrecondition(
            !server->thread.discoverableService ||
            server->thread.discoverableService == kHAPIPServiceDiscoveryType_HAP);

    HAPError err;
    HAPPlatformServiceDiscoveryTXTRecord txtRecords[kMaxNumTxtRecords];

    if (!HAPAccessoryServerIsPaired(server)) {
        HAPLogInfo(
                &kHAPLog_Default,
                "Not incrementing IP GSN because either accessory server is not paired or an active IP session "
                "exists.");
    } else {
        err = HAPIPGlobalStateNumberIncrement(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&kHAPLog_Default, "Failed to increment IP GSN.");
        }
    }

    size_t txtRecordsAdded = 0;

    // 1.  Configuration number.
    uint16_t configurationNumber;
    err = HAPAccessoryServerGetCN(server->platform.keyValueStore, &configurationNumber);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    char configurationNumberBytes[kHAPUInt16_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(configurationNumber, configurationNumberBytes, sizeof configurationNumberBytes);
    HAPAssert(!err); // configurationNumberBytes must have sufficient space for configurationNumber
    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_ConfigurationNumber, configurationNumberBytes);

    // Pairing Feature flags.
    uint8_t pairingFeatureFlags = HAPAccessoryServerGetPairingFeatureFlags(server);

    char pairingFeatureFlagsBytes[kHAPUInt8_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(pairingFeatureFlags, pairingFeatureFlagsBytes, sizeof pairingFeatureFlagsBytes);
    HAPAssert(!err); // pairingFeatureFlagsBytes must have sufficient space for pairingFeatureFlags

    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_PairingFeatureFlags, pairingFeatureFlagsBytes);

    // Device ID.
    HAPDeviceIDString deviceIDString;
    err = HAPDeviceIDGetAsString(server, &deviceIDString);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_DeviceID, deviceIDString.stringValue);

    // Model.
    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_Model, server->primaryAccessory->model);

    // Protocol Version
    if (server->transports.thread) {
        APPEND_TXT_RECORD(
                txtRecords, txtRecordsAdded, kHAPTXTRecordKey_ProtocolVersion, kHAPShortProtocolVersion_Thread);
    } else {
        HAPFatalError();
    }

    // Current state number.
    HAPIPGlobalStateNumber ipGSN;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLog(&kHAPLog_Default, "Current state number could not be fetched. Reporting as 1.");
        ipGSN = 1;
    }

    char stateNumberBytes[kHAPUInt16_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(ipGSN, stateNumberBytes, sizeof stateNumberBytes);
    HAPAssert(!err);
    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_StateNumber, stateNumberBytes);

    // Status flags.
    uint8_t statusFlags = HAPAccessoryServerGetStatusFlags(server);
    char statusFlagsBytes[kHAPUInt8_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(statusFlags, statusFlagsBytes, sizeof statusFlagsBytes);
    HAPAssert(!err); // statusFlagsBytes must have sufficient space for statusFlags
    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_StatusFlags, statusFlagsBytes);

    // Category
    char categoryBytes[kHAPUInt16_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(server->primaryAccessory->category, categoryBytes, sizeof categoryBytes);
    HAPAssert(!err);
    APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_Category, categoryBytes);

    // Setup hash. Optional.
    HAPSetupID setupID;
    bool hasSetupID = false;
    HAPPlatformAccessorySetupLoadSetupID(server->platform.accessorySetup, &hasSetupID, &setupID);
    char setupHashBytes[util_base64_encoded_len(sizeof(HAPAccessorySetupSetupHash)) + 1];
    if (hasSetupID) {
        // Get raw setup hash from setup ID.
        HAPAccessorySetupSetupHash setupHash;
        HAPAccessorySetupGetSetupHash(&setupHash, &setupID, &deviceIDString);

        // Base64 encode.
        size_t numSetupHashBytes;
        util_base64_encode(
                setupHash.bytes, sizeof setupHash.bytes, setupHashBytes, sizeof setupHashBytes, &numSetupHashBytes);
        HAPAssert(numSetupHashBytes == sizeof setupHashBytes - 1);
        setupHashBytes[sizeof setupHashBytes - 1] = '\0';

        // Append TXT record.
        APPEND_TXT_RECORD(txtRecords, txtRecordsAdded, kHAPTXTRecordKey_SetupHash, setupHashBytes);
    }

    if (!server->thread.discoverableService) {
        server->thread.discoverableService = kHAPIPServiceDiscoveryType_HAP;
        // Register the service
        HAPLogInfo(&kHAPLog_Default, "Registering %s service.", protocol);
        HAPPlatformServiceDiscoveryRegister(
                HAPNonnull(server->platform.thread.serviceDiscovery),
                server->primaryAccessory->name,
                protocol, // kServiceDiscoveryProtocol_HAP,
                port,     // Port number
                txtRecords,
                txtRecordsAdded);
    } else {
        // Update Text Records
        HAPLogInfo(&kHAPLog_Default, "Updating %s service.", protocol);
        HAPPlatformServiceDiscoveryUpdateTXTRecords(
                HAPNonnull(server->platform.thread.serviceDiscovery), txtRecords, txtRecordsAdded);
    }
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
