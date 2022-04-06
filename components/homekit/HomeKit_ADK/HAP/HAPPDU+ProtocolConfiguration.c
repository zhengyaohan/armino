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
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer+Broadcast.h"
#include "HAPDeviceID.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Protocol" };

/**
 * Protocol configuration request types.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-32 Protocol Configuration Request Types
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUTLVType_ProtocolConfiguration) {
    /** Generate-Broadcast-Encryption-Key. */
    kHAPPDUTLVType_ProtocolConfiguration_GenerateBroadcastEncryptionKey = 0x01,

    /** Get-All-Params. */
    kHAPPDUTLVType_ProtocolConfiguration_GetAllParams = 0x02,

    /** Set-Accessory-Advertising-Identifier. */
    kHAPPDUTLVType_ProtocolConfiguration_SetAccessoryAdvertisingIdentifier = 0x03
} HAP_ENUM_END(uint8_t, HAPPDUTLVType_ProtocolConfiguration);

HAP_RESULT_USE_CHECK
HAPError HAPPDUHandleProtocolConfigurationRequest(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVReader* requestReader,
        bool* didRequestGetAll,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(requestReader);
    HAPPrecondition(didRequestGetAll);
    HAPPrecondition(keyValueStore);

    HAPError err;

    HAPTLV generateKeyTLV, getAllTLV, setAdvertisingIDTLV;
    generateKeyTLV.type = kHAPPDUTLVType_ProtocolConfiguration_GenerateBroadcastEncryptionKey;
    getAllTLV.type = kHAPPDUTLVType_ProtocolConfiguration_GetAllParams;
    setAdvertisingIDTLV.type = kHAPPDUTLVType_ProtocolConfiguration_SetAccessoryAdvertisingIdentifier;
    err = HAPTLVReaderGetAll(
            requestReader, (HAPTLV* const[]) { &generateKeyTLV, &getAllTLV, &setAdvertisingIDTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Generate-Broadcast-Encryption-Key.
    bool generateKey = false;
    if (generateKeyTLV.value.bytes) {
        if (generateKeyTLV.value.numBytes) {
            HAPLog(&logObject,
                   "Generate-Broadcast-Encryption-Key has invalid length (%lu).",
                   (unsigned long) generateKeyTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        generateKey = true;
    }

    // Get-All-Params.
    *didRequestGetAll = false;
    if (getAllTLV.value.bytes) {
        if (getAllTLV.value.numBytes) {
            HAPLog(&logObject, "Get-All-Params has invalid length (%lu).", (unsigned long) getAllTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        *didRequestGetAll = true;
    }

    // Set-Accessory-Advertising-Identifier.
    const HAPDeviceID* advertisingID = NULL;
    if (setAdvertisingIDTLV.value.bytes) {
        if (setAdvertisingIDTLV.value.numBytes != sizeof advertisingID->bytes) {
            HAPLog(&logObject,
                   "Set-Accessory-Advertising-Identifier has invalid length (%lu).",
                   (unsigned long) setAdvertisingIDTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        advertisingID = setAdvertisingIDTLV.value.bytes;
    }

    // Handle request.
    if (generateKey) {
        // Generate key and set advertising identifier.
        err = HAPBLEAccessoryServerBroadcastGenerateKey(session, advertisingID);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    } else if (advertisingID) {
        // Only set advertising identifier.
        err = HAPBLEAccessoryServerBroadcastSetAdvertisingID(server->platform.keyValueStore, advertisingID);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    } else {
        // Do nothing.
    }

    return kHAPError_None;
}

/**
 * Protocol configuration parameter types.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 7-34 Protocol Configuration Parameter Types
 */
HAP_ENUM_BEGIN(uint8_t, HAPPDUTLVType_ProtocolConfiguration_Response) {
    /** HAP-Param-Current-State-Number. */
    kHAPPDUTLVType_ProtocolConfiguration_Response_CurrentStateNumber = 0x01,

    /** HAP-Param-Current-Config-Number. */
    kHAPPDUTLVType_ProtocolConfiguration_Response_CurrentConfigNumber = 0x02,

    /** HAP-Param-Accessory-Advertising-Identifier. */
    kHAPPDUTLVType_ProtocolConfiguration_Response_AccessoryAdvertisingIdentifier = 0x03,

    /** HAP-Param-Broadcast-Encryption-Key. */
    kHAPPDUTLVType_ProtocolConfiguration_Response_BroadcastEncryptionKey = 0x04
} HAP_ENUM_END(uint8_t, HAPPDUTLVType_ProtocolConfiguration_Response);

HAP_RESULT_USE_CHECK
HAPError HAPPDUGetProtocolConfigurationResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter,
        HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);
    HAPPrecondition(keyValueStore);

    HAPError err;

    // HAP-Param-Current-State-Number.
    HAPBLEAccessoryServerGSN gsn;
    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    uint8_t gsnBytes[] = { HAPExpandLittleUInt16(gsn.gsn) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ProtocolConfiguration_Response_CurrentStateNumber,
                              .value = { .bytes = gsnBytes, .numBytes = sizeof gsnBytes } });
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

    uint8_t cnBytes[] = { (uint8_t)((cn - 1) % UINT8_MAX + 1) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ProtocolConfiguration_Response_CurrentConfigNumber,
                              .value = { .bytes = cnBytes, .numBytes = sizeof cnBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // HAP-Param-Accessory-Advertising-Identifier, HAP-Param-Broadcast-Encryption-Key.
    uint16_t keyExpirationGSN;
    HAPBLEAccessoryServerBroadcastEncryptionKey broadcastKey;
    HAPDeviceID advertisingID;
    err = HAPBLEAccessoryServerBroadcastGetParameters(server, &keyExpirationGSN, &broadcastKey, &advertisingID);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPPDUTLVType_ProtocolConfiguration_Response_AccessoryAdvertisingIdentifier,
                              .value = { .bytes = advertisingID.bytes, .numBytes = sizeof advertisingID.bytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    if (keyExpirationGSN) {
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPPDUTLVType_ProtocolConfiguration_Response_BroadcastEncryptionKey,
                                  .value = { .bytes = broadcastKey.value, .numBytes = sizeof broadcastKey.value } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

#endif
