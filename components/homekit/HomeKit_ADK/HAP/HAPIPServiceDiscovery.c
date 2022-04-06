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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

#include <stdlib.h>
#include <string.h>
#include "util_base64.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetup.h"
#include "HAPAccessoryValidation.h"
#include "HAPDeviceID.h"
#include "HAPIPAccessoryServer.h"
#include "HAPIPGlobalStateNumber.h"
#include "HAPIPServiceDiscovery.h"
#include "HAPLogSubsystem.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "IPServiceDiscovery" };

/**
 * _hap service.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.4 Discovery
 */
#define kServiceDiscoveryProtocol_HAP "_hap._tcp"

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

/** Number of TXT Record keys for _hap service. */
#define kHAPTXTRecordKey_NumKeys (9)

/* Includes NULL terminator for string */
#define kHAPAccessory_ServiceName_MaxBytes (kHAPAccessory_MaxNameBytes + 1)

typedef struct {
    bool setCN;
} HAPServiceConfigurationOptions;

static void SetHAPService(HAPAccessoryServer* server, HAPServiceConfigurationOptions config);

void HAPIPServiceDiscoverySetHAPService(HAPAccessoryServer* server) {
    HAPServiceConfigurationOptions options;
    HAPRawBufferZero(&options, sizeof options);
    options.setCN = true;
    SetHAPService(server, options);
}

void GetHAPServiceName(
        HAPAccessoryServer* server,
        HAPServiceConfigurationOptions options HAP_UNUSED,
        char* serviceName,
        size_t serviceNameMaxBytes) {
    HAPPrecondition(server);
    HAPPrecondition(serviceName);

    // Make sure the incoming buffer is large enough (include need for NULL terminator)
    HAPAssert(serviceNameMaxBytes >= HAPStringGetNumBytes(server->primaryAccessory->name) + 1);
    HAPRawBufferCopyBytes(
            serviceName, server->primaryAccessory->name, HAPStringGetNumBytes(server->primaryAccessory->name) + 1);
}

/**
 * Common function to update Bonjour txt records and publish _hap service
 *
 * @param      server               Accessory Server
 * @param      configOptions        Provides options to include or exclude certain bonjour txt records
 */
static void SetHAPService(HAPAccessoryServer* server, HAPServiceConfigurationOptions configOptions) {
    HAPPrecondition(server);
    HAPPrecondition(
            !server->ip.discoverableService || server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 6.4 Discovery

    bool isActiveSession = false;
    for (size_t i = 0; i < server->ip.storage->numSessions; i++) {
        HAPIPSession* ipSession = &server->ip.storage->sessions[i];
        HAPIPSessionDescriptor* session = &ipSession->descriptor;

        if (session->tcpStreamIsOpen) {
            isActiveSession = true;
        }
    }

    bool shouldIncrementGSN = true;

    // Increment state number.
    if (shouldIncrementGSN == true) {
        if (!HAPAccessoryServerIsPaired(server) || isActiveSession == true) {
            HAPLogInfo(
                    &logObject,
                    "Not incrementing IP GSN because either accessory server is not paired or an active IP session "
                    "exists.");
        } else {
            err = HAPIPGlobalStateNumberIncrement(server);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "Failed to increment IP GSN.");
            }
        }
    }

    HAPPlatformServiceDiscoveryTXTRecord txtRecords[kHAPTXTRecordKey_NumKeys];
    size_t numTXTRecords = 0;

    uint16_t configurationNumber;
    char configurationNumberBytes[kHAPUInt16_MaxDescriptionBytes];
    // Configuration number.
    if (configOptions.setCN) {
        err = HAPAccessoryServerGetCN(server->platform.keyValueStore, &configurationNumber);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        err = HAPUInt64GetDescription(configurationNumber, configurationNumberBytes, sizeof configurationNumberBytes);
        HAPAssert(!err);
        HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
        txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
            .key = kHAPTXTRecordKey_ConfigurationNumber,
            .value = { .bytes = configurationNumberBytes, .numBytes = HAPStringGetNumBytes(configurationNumberBytes) }
        };
    }

    // Pairing Feature flags.
    uint8_t pairingFeatureFlags = HAPAccessoryServerGetPairingFeatureFlags(server);
    char pairingFeatureFlagsBytes[kHAPUInt8_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(pairingFeatureFlags, pairingFeatureFlagsBytes, sizeof pairingFeatureFlagsBytes);
    HAPAssert(!err);
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
        .key = kHAPTXTRecordKey_PairingFeatureFlags,
        .value = { .bytes = pairingFeatureFlagsBytes, .numBytes = HAPStringGetNumBytes(pairingFeatureFlagsBytes) }
    };

    // Device ID.
    HAPDeviceIDString deviceIDString;
    err = HAPDeviceIDGetAsString(server, &deviceIDString);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
        .key = kHAPTXTRecordKey_DeviceID,
        .value = { .bytes = deviceIDString.stringValue, .numBytes = HAPStringGetNumBytes(deviceIDString.stringValue) }
    };

    // Model.
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
        .key = kHAPTXTRecordKey_Model,
        .value = { .bytes = server->primaryAccessory->model,
                   .numBytes = HAPStringGetNumBytes(server->primaryAccessory->model) }
    };

    // Protocol version.
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
        .key = kHAPTXTRecordKey_ProtocolVersion,
        .value = { .bytes = kHAPShortProtocolVersion_IP, .numBytes = HAPStringGetNumBytes(kHAPShortProtocolVersion_IP) }
    };

    // Current state number.
    HAPIPGlobalStateNumber ipGSN;
    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLog(&logObject, "Current state number could not be fetched. Reporting as 1.");
        ipGSN = 1;
    }

    char stateNumberBytes[kHAPUInt16_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(ipGSN, stateNumberBytes, sizeof stateNumberBytes);
    HAPAssert(!err);
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] =
            (HAPPlatformServiceDiscoveryTXTRecord) { .key = kHAPTXTRecordKey_StateNumber,
                                                     .value = { .bytes = stateNumberBytes,
                                                                .numBytes = HAPStringGetNumBytes(stateNumberBytes) } };

    // Status flags.
    uint8_t statusFlags = HAPAccessoryServerGetStatusFlags(server);
    char statusFlagsBytes[kHAPUInt8_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(statusFlags, statusFlagsBytes, sizeof statusFlagsBytes);
    HAPAssert(!err);
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] =
            (HAPPlatformServiceDiscoveryTXTRecord) { .key = kHAPTXTRecordKey_StatusFlags,
                                                     .value = { .bytes = statusFlagsBytes,
                                                                .numBytes = HAPStringGetNumBytes(statusFlagsBytes) } };

    // Category.
    char categoryBytes[kHAPUInt16_MaxDescriptionBytes];
    err = HAPUInt64GetDescription(server->primaryAccessory->category, categoryBytes, sizeof categoryBytes);
    HAPAssert(!err);
    HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
    txtRecords[numTXTRecords++] =
            (HAPPlatformServiceDiscoveryTXTRecord) { .key = kHAPTXTRecordKey_Category,
                                                     .value = { .bytes = categoryBytes,
                                                                .numBytes = HAPStringGetNumBytes(categoryBytes) } };

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
        HAPAssert(numTXTRecords < HAPArrayCount(txtRecords));
        txtRecords[numTXTRecords++] = (HAPPlatformServiceDiscoveryTXTRecord) {
            .key = kHAPTXTRecordKey_SetupHash,
            .value = { .bytes = setupHashBytes, .numBytes = HAPStringGetNumBytes(setupHashBytes) }
        };
    }

    char serviceName[kHAPAccessory_ServiceName_MaxBytes];
    GetHAPServiceName(server, configOptions, serviceName, kHAPAccessory_ServiceName_MaxBytes);

    // Register service.
    HAPAssert(numTXTRecords <= HAPArrayCount(txtRecords));
    if (!server->ip.discoverableService) {
        server->ip.discoverableService = kHAPIPServiceDiscoveryType_HAP;
        HAPLogInfo(&logObject, "Registering %s service for %s.", kServiceDiscoveryProtocol_HAP, serviceName);
        HAPPlatformServiceDiscoveryRegister(
                HAPNonnull(server->platform.ip.serviceDiscovery),
                serviceName,
                kServiceDiscoveryProtocol_HAP,
                HAPPlatformTCPStreamManagerGetListenerPort(HAPNonnull(server->platform.ip.tcpStreamManager)),
                txtRecords,
                numTXTRecords);
    } else {
        HAPAssert(server->ip.discoverableService == kHAPIPServiceDiscoveryType_HAP);
        HAPLogInfo(&logObject, "Updating %s service for %s.", kServiceDiscoveryProtocol_HAP, serviceName);
        HAPPlatformServiceDiscoveryUpdateTXTRecords(
                HAPNonnull(server->platform.ip.serviceDiscovery), txtRecords, numTXTRecords);
    }
}

void HAPIPServiceDiscoveryStop(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (server->ip.discoverableService) {
        HAPLogInfo(&logObject, "Stopping service discovery.");
        HAPPlatformServiceDiscoveryStop(HAPNonnull(server->platform.ip.serviceDiscovery));
        server->ip.discoverableService = kHAPIPServiceDiscoveryType_None;
    }
}

#endif
