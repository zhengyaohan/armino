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

#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetup.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPDeviceID.h"
#include "HAPIPGlobalStateNumber.h"
#include "HAPLogSubsystem.h"
#include "HAPSession.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Accessory" };

/**
 * TLV types used in HAP-Info-Response.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-9 Info Parameter Types
 */
HAP_ENUM_BEGIN(uint8_t, HAPInfoResponseTLVType) { /**
                                                   * HAP-Param-Current-State-Number.
                                                   * 2 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_StateNumber = 0x01,

                                                  /**
                                                   * HAP-Param-Current-Config-Number.
                                                   * 2 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_ConfigNumber = 0x02,

                                                  /**
                                                   * HAP-Param-Device-Identifier.
                                                   * 6 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_DeviceIdentifier = 0x03,

                                                  /**
                                                   * HAP-Param-Feature-Flags.
                                                   * 1 byte.
                                                   */
                                                  kHAPInfoResponseTLVType_FeatureFlags = 0x04,

                                                  /**
                                                   * HAP-Param-Model-Name.
                                                   * UTF-8 string, maximum 255 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_ModelName = 0x05,

                                                  /**
                                                   * HAP-Param-Protocol-Version.
                                                   * UTF-8 string, maximum 255 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_ProtocolVersion = 0x06,

                                                  /**
                                                   * HAP-Param-Status-Flag.
                                                   * 1 byte.
                                                   */
                                                  kHAPInfoResponseTLVType_StatusFlag = 0x07,

                                                  /**
                                                   * HAP-Param-Category-Identifier.
                                                   * 2 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_CategoryIdentifier = 0x08,

                                                  /**
                                                   * HAP-Param-Setup-Hash.
                                                   * 4 bytes.
                                                   */
                                                  kHAPInfoResponseTLVType_SetupHash = 0x09
} HAP_ENUM_END(uint8_t, HAPInfoResponseTLVType);

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryGetInfoResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.17.6 HAP-Info-Response

    // Response seems to be based on the _hap._tcp Bonjour TXT Record Keys used by IP accessories.
    // See HomeKit Accessory Protocol Specification R17
    // Table 6-7 _hap._tcp Bonjour TXT Record Keys

    // HAP-Param-Current-State-Number.
    uint16_t sn = 0;
    switch (session->transportType) {
        case kHAPTransportType_Thread: // Fallthrough
        case kHAPTransportType_IP: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            err = HAPIPGlobalStateNumberGet(server, &sn);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "Failed to get IP GSN. Reporting 1.");
                sn = 1;
            }
            break;
#endif
        }
        case kHAPTransportType_BLE: {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
            HAPBLEAccessoryServerGSN gsn;
            err = HAPNonnull(server->transports.ble)->getGSN(server->platform.keyValueStore, &gsn);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "Failed to get BLE GSN.");
                return err;
            }
            sn = gsn.gsn;
            break;
#endif
        }
    }
    HAPAssert(sn);
    uint8_t snBytes[] = { HAPExpandLittleUInt16(sn) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_StateNumber,
                              .value = { .bytes = snBytes, .numBytes = sizeof snBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Current-Config-Number.
    uint16_t cn;
    err = HAPAccessoryServerGetCN(server->platform.keyValueStore, &cn);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    uint8_t cnBytes[] = { HAPExpandLittleUInt16(cn) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_ConfigNumber,
                              .value = { .bytes = cnBytes, .numBytes = sizeof cnBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Device-Identifier.
    HAPDeviceID deviceID;
    err = HAPDeviceIDGet(server, &deviceID);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_DeviceIdentifier,
                              .value = { .bytes = deviceID.bytes, .numBytes = sizeof deviceID.bytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Feature-Flags.
    uint8_t pairingFeatureFlags = HAPAccessoryServerGetPairingFeatureFlags(server);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_FeatureFlags,
                              .value = { .bytes = &pairingFeatureFlags, .numBytes = sizeof pairingFeatureFlags } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Model-Name.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPInfoResponseTLVType_ModelName,
                    .value = { .bytes = accessory->model, .numBytes = HAPStringGetNumBytes(accessory->model) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Protocol-Version.
    const char* pv = NULL;
    switch (session->transportType) {
        case kHAPTransportType_IP: {
            pv = kHAPProtocolVersion_IP;
            break;
        }
        case kHAPTransportType_BLE: {
            pv = kHAPProtocolVersion_BLE;
            break;
        }
        case kHAPTransportType_Thread: {
            pv = kHAPProtocolVersion_Thread;
            break;
        }
    }
    HAPAssert(pv);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_ProtocolVersion,
                              .value = { .bytes = pv, .numBytes = HAPStringGetNumBytes(pv) } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Status-Flag.
    uint8_t statusFlags = HAPAccessoryServerGetStatusFlags(server);
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPInfoResponseTLVType_StatusFlag,
                              .value = { .bytes = &statusFlags, .numBytes = sizeof statusFlags } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Category-Identifier.
    HAPAssert(accessory->category);
    uint8_t categoryIdentifierBytes[] = { HAPExpandLittleUInt16((uint16_t) accessory->category) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPInfoResponseTLVType_CategoryIdentifier,
                    .value = { .bytes = categoryIdentifierBytes, .numBytes = sizeof categoryIdentifierBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Setup-Hash.
    HAPSetupID setupID;
    bool hasSetupID;
    HAPPlatformAccessorySetupLoadSetupID(server->platform.accessorySetup, &hasSetupID, &setupID);
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

        // Append TLV.
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPInfoResponseTLVType_SetupHash,
                                  .value = { .bytes = setupHash.bytes, .numBytes = sizeof setupHash.bytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}
