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
#include "HAP.h"
#include "HAPPlatform+Init.h"
#include "HAPPlatformFeatures.h"
#include "HAPPlatformWiFiManager.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
#include "HAP+KeyValueStoreDomains.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/HAPTestController.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#include "Harness/HAPPlatformTCPStreamManagerMock.h"
#include "Harness/HAPPlatformWiFiManagerMock.h"

#include "Harness/HAPPlatformServiceDiscoveryMock.h"

extern "C" {

static HAPAccessory InitAccessory(void) {
    HAPAccessory accessory = { .aid = 1,
                               .category = kHAPAccessoryCategory_Lighting,
                               .name = "Acme Test",
                               .productData = "03d8a775e3644573",
                               .manufacturer = "Acme",
                               .model = "Test1,1",
                               .serialNumber = "099DB48E9E28",
                               .firmwareVersion = "1",
                               .hardwareVersion = "1" };

    static const HAPService* services[] = {
        &accessoryInformationService, &hapProtocolInformationService, &pairingService, &wiFiTransportService, NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

} // extern "C"

/**
 * Creates an IP accessory server
 *
 * @return Created accessory server
 */
static void CreateIPAccessoryServer(HAPAccessoryServer* server) {
    static HAPAccessoryServerOptions serverOptions;
    HAPRawBufferZero(&serverOptions, sizeof serverOptions);

    static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    static IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
    static HAPIPReadContext ipReadContexts[kAttributeCount];
    static HAPIPWriteContext ipWriteContexts[kAttributeCount];
    static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer }
    };

    serverOptions.maxPairings = kHAPPairingStorage_MinElements;
    serverOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    serverOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    static HAPPlatformServiceDiscovery ipServiceDiscovery;
    platform.ip.serviceDiscovery = &ipServiceDiscovery;
    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };

    HAPAccessoryServerCreate(server, &serverOptions, &platform, &serverCallbacks, NULL);
    server->ip.storage = &ipAccessoryServerStorage;
}

/**
 * Setup step before a test
 */
TEST_SETUP() {
    // Clear all pairings
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Teardown step after a test so that next test can run in a clean state.
 */
TEST_TEARDOWN() {
    // Clears all timers
    HAPPlatformTimerDeregisterAll();
}

/**
 * Adds a pairing entry into key value store if the entry does not exist.
 *
 * @param controllerPairingID  Controller pairing ID
 */
static void AddPairingEntry(HAPPlatformKeyValueStoreKey controllerPairingID) {
    bool found;
    HAPError err = HAPPlatformKeyValueStoreGet(
            platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings, controllerPairingID, NULL, 0, NULL, &found);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    if (found) {
        // Pairing already exists.
        return;
    }

    HAPControllerPublicKey controllerPublicKey;
    HAPControllerPairingIdentifier controllerPairingIdentifier;
    HAPRawBufferZero(&controllerPairingIdentifier, sizeof controllerPairingIdentifier);
    controllerPairingIdentifier.numBytes = sizeof controllerPairingIdentifier.bytes;
    HAPPlatformRandomNumberFill(controllerPairingIdentifier.bytes, sizeof controllerPairingIdentifier.bytes);

    HAPPlatformRandomNumberFill(controllerPublicKey.bytes, sizeof controllerPublicKey.bytes);

    err = HAPLegacyImportControllerPairing(
            platform.keyValueStore,
            controllerPairingID,
            &controllerPairingIdentifier,
            &controllerPublicKey,
            /* isAdmin: */ true);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Sets up fake session keys
 *
 * @param sessions HAPSession array
 * @param count    Number of sessions in the array
 */
static void SetupFakeSessionKeys(HAPSession* sessions, size_t count) {
    for (size_t i = 0; i < count; i++) {
        HAPPlatformRandomNumberFill(
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_SK,
                sizeof(((HAPSession*) &sessions[i])->state.pairVerify.cv_SK));
        HAP_X25519_scalarmult_base(
                ((HAPSession*) &sessions[i])->state.pairVerify.Controller_cv_PK,
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_SK);
        HAPPlatformRandomNumberFill(
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_SK,
                sizeof(((HAPSession*) &sessions[i])->state.pairVerify.cv_SK));
        HAP_X25519_scalarmult_base(
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_PK,
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_SK);
        HAP_X25519_scalarmult(
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_KEY,
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_SK,
                ((HAPSession*) &sessions[i])->state.pairVerify.Controller_cv_PK);
        HAPRawBufferCopyBytes(
                ((HAPSession*) &sessions[i])->hap.cv_KEY,
                ((HAPSession*) &sessions[i])->state.pairVerify.cv_KEY,
                sizeof(((HAPSession*) &sessions[i])->hap.cv_KEY));
        HAPRawBufferZero(
                &((HAPSession*) &sessions[i])->state.pairVerify,
                sizeof(((HAPSession*) &sessions[i])->state.pairVerify));
    }
}

/**
 * Emulate a Read characteristic for Current Transport
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param responsevalue        Response value
 */
static void TriggerReadCurrentTransportCharacteristics(
        HAPSession* session,
        HAPAccessoryServer* server,
        bool* responseValue) {
    const HAPBoolCharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                       .session = session,
                                                       .characteristic = &wiFiTransportCurrentTransportCharacteristic,
                                                       .service = &wiFiTransportService,
                                                       .accessory = &accessory };
    HAPError err = request.characteristic->callbacks.handleRead(server, &request, responseValue, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Emulate a Read characteristic for WiFi Capability
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param responsevalue        Response value
 */
static void TriggerReadWiFiCapabilityCharacteristics(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint32_t* responseValue) {
    const HAPUInt32CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                         .session = session,
                                                         .characteristic = &wiFiTransportWiFiCapabilityCharacteristic,
                                                         .service = &wiFiTransportService,
                                                         .accessory = &accessory };
    HAPError err = request.characteristic->callbacks.handleRead(server, &request, responseValue, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Emulate a Read characteristic for wiFi configuration control
 *
 * @param session              HAP session
 * @param server               Accessory server
 */
static void TriggerReadWiFiConfigurationControlCharacteristic(
        HAPSession* session,
        HAPAccessoryServer* server,
        HAPPlatformWiFiManagerCookie* cookie,
        HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
    static const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static HAPTLVWriter responseWriter;
    static uint8_t bytes[1024];
    static size_t numBytes = sizeof bytes;
    HAPTLVWriterCreate(&responseWriter, bytes, numBytes);

    HAPError err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static void* responseBytes;
    static size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    for (size_t i = 0; i < 2; i++) {
        static HAPTLV tlv;
        static bool valid;
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT_EQUAL(valid, true);

        switch (tlv.type) {
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_Cookie: {
                *cookie = HAPReadLittleUInt16(tlv.value.bytes);
                break;
            }
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_UpdateStatus: {
                *updateStatus = HAPReadLittleUInt32(tlv.value.bytes);
                break;
            }
        }
    }
}

/**
 * Emulate a Read characteristic for wiFi configuration control read configuration
 *
 * @param session              HAP session
 * @param server               Accessory Server
 * @param[out] cookie          Cookie value stored
 * @param[out] updateStatus    Last persisted update status
 * @param[out] countryCode     Currently persited country code
 * @param[out] ssid            Currently persited wiFi ssid
 * @param[out] securityMode    Currently persited security mode (WPA-PSK, etc.)
 * @param[out] psk             Empty string indicates presence of PSK
 */
static void TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
        HAPSession* session,
        HAPAccessoryServer* server,
        HAPPlatformWiFiManagerCookie* cookie,
        HAPPlatformWiFiManagerUpdateStatus* updateStatus,
        char* countryCode,
        char* ssid,
        uint8_t* securityMode,
        char* psk) {
    static const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static HAPTLVWriter responseWriter;
    static uint8_t bytes[1024];
    static size_t numBytes = sizeof bytes;
    HAPTLVWriterCreate(&responseWriter, bytes, numBytes);

    HAPError err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static void* responseBytes;
    static size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);
    static HAPTLV tlv;
    static bool valid;

    for (size_t i = 0; i < 4; i++) {
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT_EQUAL(valid, true);

        switch (tlv.type) {
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_Cookie: {
                *cookie = HAPReadLittleUInt16(tlv.value.bytes);
                break;
            }
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_UpdateStatus: {
                *updateStatus = HAPReadLittleUInt32(tlv.value.bytes);
                break;
            }
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_CountryCode: {
                HAPRawBufferCopyBytes(countryCode, tlv.value.bytes, tlv.value.numBytes);
                break;
            }
            case kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig: {
                static HAPTLVReader nestedReader;
                static HAPTLV nestedTLV;
                static bool nestedValid;
                HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);
                for (;;) {
                    err = HAPTLVReaderGetNext(&nestedReader, &nestedValid, &nestedTLV);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);

                    if (!nestedValid) {
                        break;
                    }

                    switch (nestedTLV.type) {
                        case kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SSID: {
                            HAPRawBufferCopyBytes(ssid, nestedTLV.value.bytes, nestedTLV.value.numBytes);
                            break;
                        }
                        case kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SecurityMode: {
                            *securityMode = HAPReadUInt8(nestedTLV.value.bytes);
                            break;
                        }
                        case kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_PSK: {
                            HAPRawBufferCopyBytes(psk, nestedTLV.value.bytes, nestedTLV.value.numBytes);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
            default:
                break;
        }
    }
}

/**
 * Emulate a Write characteristic for read configuration for wiFi configuration control
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param operationTypeValue   Operation Type for the write request
 * @param cookieValue          Cookie value for the write request
 */
static void TriggerWriteForWiFiConfigurationControlCharacteristic(
        HAPSession* session,
        HAPAccessoryServer* server,
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationTypeValue,
        HAPPlatformWiFiManagerCookie cookieValue) {
    static const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static uint8_t requestBuffer[1024];
    static HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType[] = { operationTypeValue };

    static const HAPTLV hapTlvOperationType = {
        .value = { .bytes = operationType, .numBytes = sizeof operationType },
        .type = kHAPCharacteristicTLVType_WiFiConfigurationControl_OperationType
    };

    HAPError err = HAPTLVWriterAppend(&requestWriter, &hapTlvOperationType);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static HAPPlatformWiFiManagerCookie cookie[] = { cookieValue };
    static const HAPTLV hapTlvCookie = {
        .value = { .bytes = cookie, .numBytes = sizeof cookie },
        .type = kHAPCharacteristicTLVType_WiFiConfigurationControl_Cookie,
    };
    err = HAPTLVWriterAppend(&requestWriter, &hapTlvCookie);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static void* requestBytes;
    static size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, requestBytes, numRequestBytes);

    err = request.characteristic->callbacks.handleWrite(server, &request, &responseReader, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Emulate a Write to the characteristic for update configurations for wiFi configuration control
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param operationTypeValue   Operation Type for the write request
 * @param cookieValue          Cookie value for the write request
 * @param ssid                 SSID to configure on the accessory
 * @param countryCode          Country code for the configuration
 * @param securityMode         Security mode for the configuration (WPA-PSK, etc.)
 * @param psk                  WPA passphrase
 */
static HAPError TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
        HAPSession* session,
        HAPAccessoryServer* server,
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationTypeValue,
        HAPPlatformWiFiManagerCookie cookieValue,
        char* _Nonnull ssidValue,
        char* _Nullable countryCodeValue,
        uint8_t securityModeValue,
        char* pskValue) {
    static const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static uint8_t requestBuffer[1024];
    static HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    HAPCharacteristicValue_WiFiConfigurationControl wiFiConfigurationControl;
    HAPRawBufferZero(&wiFiConfigurationControl, sizeof wiFiConfigurationControl);
    wiFiConfigurationControl.operationTypeIsSet = true;
    wiFiConfigurationControl.operationType = operationTypeValue;

    wiFiConfigurationControl.cookieIsSet = true;
    wiFiConfigurationControl.cookie = cookieValue;
    wiFiConfigurationControl.updateStatusIsSet = false;

    if (countryCodeValue) {
        wiFiConfigurationControl.countryCodeIsSet = true;
        wiFiConfigurationControl.countryCode = countryCodeValue;
    } else {
        wiFiConfigurationControl.countryCodeIsSet = false;
    }

    wiFiConfigurationControl.stationConfigIsSet = true;

    wiFiConfigurationControl.stationConfig.ssidIsSet = true;
    wiFiConfigurationControl.stationConfig.ssid = ssidValue;

    wiFiConfigurationControl.stationConfig.securityModeIsSet = true;
    wiFiConfigurationControl.stationConfig.securityMode = securityModeValue;

    wiFiConfigurationControl.stationConfig.pskIsSet = true;
    wiFiConfigurationControl.stationConfig.psk = pskValue;

    HAPError err = HAPTLVWriterEncode(
            &requestWriter, &kHAPCharacteristicTLVFormat_WiFiConfigurationControl, &wiFiConfigurationControl);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static void* requestBytes;
    static size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, requestBytes, numRequestBytes);

    err = request.characteristic->callbacks.handleWrite(server, &request, &responseReader, NULL);
    return err;
}

/**
 * Emulate a Write to the characteristic for update configurations for wiFi configuration control with empty station
 * config TLV.
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param operationTypeValue   Operation Type for the write request
 * @param cookieValue          Cookie value for the write request
 * @param countryCode          Country code for the configuration
 */
static HAPError TriggerWriteForUpdateWiFiConfigurationControlCharacteristicWithEmptyTLV(
        HAPSession* session,
        HAPAccessoryServer* server,
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationTypeValue,
        HAPPlatformWiFiManagerCookie cookieValue,
        char* _Nullable countryCodeValue) {
    static const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static uint8_t requestBuffer[1024];
    static HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    HAPCharacteristicValue_WiFiConfigurationControl wiFiConfigurationControl;
    HAPRawBufferZero(&wiFiConfigurationControl, sizeof wiFiConfigurationControl);
    wiFiConfigurationControl.operationTypeIsSet = true;
    wiFiConfigurationControl.operationType = operationTypeValue;

    wiFiConfigurationControl.cookieIsSet = true;
    wiFiConfigurationControl.cookie = cookieValue;
    wiFiConfigurationControl.updateStatusIsSet = false;

    if (countryCodeValue) {
        wiFiConfigurationControl.countryCodeIsSet = true;
        wiFiConfigurationControl.countryCode = countryCodeValue;
    } else {
        wiFiConfigurationControl.countryCodeIsSet = false;
    }

    wiFiConfigurationControl.stationConfigIsSet = false;

    HAPError err = HAPTLVWriterEncode(
            &requestWriter, &kHAPCharacteristicTLVFormat_WiFiConfigurationControl, &wiFiConfigurationControl);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static void* requestBytes;
    static size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, requestBytes, numRequestBytes);

    err = request.characteristic->callbacks.handleWrite(server, &request, &responseReader, NULL);
    return err;
}

/**
 * Emulate a Write to the characteristic for update configurations for wiFi configuration control
 *
 * @param session              HAP session
 * @param server               Accessory server
 * @param requestBytes         TLV bytes
 * @param numRequestBytes      number of bytes in the TLV
 */
static HAPError TriggerTLVWriteForUpdateWiFiConfigurationControlCharacteristic(
        HAPSession* session,
        HAPAccessoryServer* server,
        void* requestBytes,
        size_t numRequestBytes) {
    static const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &wiFiTransportWiFiConfigurationControlCharacteristic,
        .service = &wiFiTransportService,
        .accessory = &accessory
    };
    static uint8_t requestBuffer[1024];
    static HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);

    static HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, requestBytes, numRequestBytes);

    HAPError err = request.characteristic->callbacks.handleWrite(server, &request, &responseReader, NULL);
    return err;
}

/**
 * Test reading Current Transport characteristics
 */
TEST(TestReadCurrentTransport) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));

    EXPECT(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).AtLeast(1).AtMost(1).Return(false);
    bool value = false;
    TriggerReadCurrentTransportCharacteristics(&sessions[0], &server, &value);
    TEST_ASSERT_EQUAL(value, false);
    VERIFY_ALL(tcpStreamManagerMock);

    EXPECT(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).AtLeast(1).AtMost(1).Return(true);
    TriggerReadCurrentTransportCharacteristics(&sessions[0], &server, &value);
    TEST_ASSERT_EQUAL(value, true);

    VERIFY_ALL(tcpStreamManagerMock);
    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test reading WiFi Capability characteristics
 */
TEST(TestReadWiFiCapability) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    EXPECT(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetWiFiCapability)
            .AtLeast(1)
            .AtMost(1)
            .Return((HAPPlatformWiFiCapability) { .supports2_4GHz = true, .supports5GHz = false });
    uint32_t value = 0;
    TriggerReadWiFiCapabilityCharacteristics(&sessions[0], &server, &value);
    TEST_ASSERT_EQUAL(value, (uint32_t) 9);
    VERIFY_ALL(tcpStreamManagerMock);

    EXPECT(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetWiFiCapability)
            .AtLeast(1)
            .AtMost(1)
            .Return((HAPPlatformWiFiCapability) { .supports2_4GHz = true, .supports5GHz = true });
    TriggerReadWiFiCapabilityCharacteristics(&sessions[0], &server, &value);
    TEST_ASSERT_EQUAL(value, (uint32_t) 11);

    VERIFY_ALL(tcpStreamManagerMock);
    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test reading WiFi Configuration Control characteristics
 */
TEST(TestReadWiFiConfigurationControl) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 1;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });
    HAPPlatformWiFiManagerCookie cookie = 0;
    HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;
    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 1);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    VERIFY_ALL(wifiManagerMock);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 9999;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });
    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 9999);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Read configuration
 */
TEST(TestWriteWiFiConfigurationControlReadConfiguration) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read;

    HAPPlatformWiFiManagerCookie cookieValue = 1000;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 1000;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "US";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[8] = "WiFiABC";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 7);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 8);
                ssid->numBytes = 7;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 1000);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    TEST_ASSERT_EQUAL(countryCode, "US");
    TEST_ASSERT_EQUAL(ssid, "WiFiABC");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfiguration) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue = 40;
    static char newCountry[3] = "UK";
    static char newWiFi[8] = "WiFiNew";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 40;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 40);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    VERIFY_ALL(wifiManagerMock);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 40;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "UK";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[8] = "WiFiNew";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 7);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 8);
                ssid->numBytes = 7;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read;
    cookieValue = 40;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 40);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    TEST_ASSERT_EQUAL(countryCode, "UK");
    TEST_ASSERT_EQUAL(ssid, "WiFiNew");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Success case where
 * transport is WiFi and commit message comes before timer expires)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationSuccess1) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 100;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196708;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 196708);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(30 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 100;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 8650852;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "US";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiNew1";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 8650852);
    TEST_ASSERT_EQUAL(countryCode, "US");
    TEST_ASSERT_EQUAL(ssid, "WiFiNew1");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Success case where
 * transport is Ethernet and WiFi is connected before timer expires)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationSuccess2) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(false);
    ALWAYS(wifiManagerMock, HAPPlatformWiFiManagerIsWiFiLinkEstablished).Return(true);
    ALWAYS(wifiManagerMock, HAPPlatformWiFiManagerIsWiFiNetworkConfigured).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 101;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 101;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(2)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 65637;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 101);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 65637);

    HAPPlatformTimerEmulateClockAdvances(30 * HAPSecond);
    VERIFY_ALL(wifiManagerMock);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read;
    cookieValue = 101;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 101;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 14942309;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "US";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiNew1";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 101);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 14942309);
    TEST_ASSERT_EQUAL(countryCode, "US");
    TEST_ASSERT_EQUAL(ssid, "WiFiNew1");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Failure case where
 * transport is WiFi and HAPPlatformWiFiManagerApplyConfiguration fails)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationFailure1) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_Unknown);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerRemoveConfiguration).AtLeast(1).AtMost(1);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 500;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_Unknown);

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 500;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(2)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 197108;
                return kHAPError_None;
            });
    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 500);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 197108);

    HAPPlatformTimerEmulateClockAdvances(60 * HAPSecond);
    VERIFY_ALL(wifiManagerMock);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 500;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 500;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 524788;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "UK";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiABC";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 500);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 524788);
    TEST_ASSERT_EQUAL(countryCode, "UK");
    TEST_ASSERT_EQUAL(ssid, "WiFiABC");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Failure case where
 * transport is WiFi and commit message doesn't come before the timer expires)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationFailure2) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerRemoveConfiguration).AtLeast(1).AtMost(1);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 700;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 700;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(5)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 197308;
                return kHAPError_None;
            });
    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 700);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 197308);

    HAPPlatformTimerEmulateClockAdvances(65 * HAPSecond);
    VERIFY_ALL(wifiManagerMock);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 700;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 700;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 524988;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "UK";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiABC";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 700);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 524988);
    TEST_ASSERT_EQUAL(countryCode, "UK");
    TEST_ASSERT_EQUAL(ssid, "WiFiABC");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Failure case where
 * transport is Ethernet and WiFi fails to connect while the timer expires)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationFailure3) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(false);
    ALWAYS(wifiManagerMock, HAPPlatformWiFiManagerIsWiFiLinkEstablished).Return(false);
    ALWAYS(wifiManagerMock, HAPPlatformWiFiManagerIsWiFiNetworkConfigured).Return(false);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 701;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 701;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(5)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 66237;
                return kHAPError_None;
            });
    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 701);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 66237);

    HAPPlatformTimerEmulateClockAdvances(65 * HAPSecond);
    VERIFY_ALL(wifiManagerMock);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read;
    cookieValue = 701;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 701;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 524989;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "UK";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiABC";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 701);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 524989);
    TEST_ASSERT_EQUAL(countryCode, "UK");
    TEST_ASSERT_EQUAL(ssid, "WiFiABC");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration (Success case where
 * transport is WiFi and commit message comes with incorrect cookie value)
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateConfigurationFailure4) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 102;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 102;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196710;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 102);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 196710);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(30 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 103;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerRemoveConfiguration).AtLeast(1).AtMost(1);

    HAPPlatformTimerEmulateClockAdvances(35 * HAPSecond);
    VERIFY_ALL(wifiManagerMock);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read;
    cookieValue = 102;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 102;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 524390;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "UK";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiABC";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 102);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 524390);
    TEST_ASSERT_EQUAL(countryCode, "UK");
    TEST_ASSERT_EQUAL(ssid, "WiFiABC");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration followed by another fail
 * safe update where we get resource busy. Sent a commit message and expect that the first fail safe update succeeds
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateErrorBusy) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 100;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196708;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 196708);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(10 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue1 = 101;
    static char newCountry1[3] = "US";
    static char newWiFi1[9] = "WiFiNew2";
    static char newPSK1[9] = "PassNew2";
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196708;
                return kHAPError_None;
            });
    err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue1,
            newWiFi1,
            newCountry1,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK1);
    TEST_ASSERT_EQUAL(err, kHAPError_Busy);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(10 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 100;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 8650852;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "US";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiNew1";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 8650852);
    TEST_ASSERT_EQUAL(countryCode, "US");
    TEST_ASSERT_EQUAL(ssid, "WiFiNew1");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Fail safe Update configuration followed by another simple
 * update where we get resource busy. Sent a commit message and expect that the first fail safe update succeeds
 */
TEST(TestWriteWiFiConfigurationControlFailSafeUpdateSimpleUpdateErrorBusy) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsWiFiCurrentTransport).Return(true);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerBackUpConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe;
    HAPPlatformWiFiManagerCookie cookieValue = 100;
    static char newCountry[3] = "US";
    static char newWiFi[9] = "WiFiNew1";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196708;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 196708);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(10 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue1 = 101;
    static char newCountry1[3] = "US";
    static char newWiFi1[9] = "WiFiNew2";
    static char newPSK1[9] = "PassNew2";
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 196708;
                return kHAPError_None;
            });
    err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue1,
            newWiFi1,
            newCountry1,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK1);
    TEST_ASSERT_EQUAL(err, kHAPError_Busy);
    VERIFY_ALL(wifiManagerMock);

    HAPPlatformTimerEmulateClockAdvances(10 * HAPSecond);

    operationType = kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit;
    cookieValue = 100;
    TriggerWriteForWiFiConfigurationControlCharacteristic(&sessions[0], &server, operationType, cookieValue);

    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 100;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 8650852;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetRegulatoryDomain)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformRegulatoryDomain* regulatoryDomain) {
                static const char country[3] = "US";
                HAPRawBufferCopyBytes(regulatoryDomain->stringValue, country, 3);
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetSSID)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid) {
                static const char wiFi[9] = "WiFiNew1";
                *isConfigured = true;
                HAPRawBufferCopyBytes(ssid->bytes, wiFi, 8);
                HAPRawBufferCopyBytes(ssid->stringValue, wiFi, 9);
                ssid->numBytes = 8;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerIsPSKConfigured)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPSKConfigured) {
                *isPSKConfigured = true;
                return kHAPError_None;
            });

    cookie = 0;
    updateStatus = 0;
    static char countryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    static char ssid[HAPPlatformSSID_MaxBytes + 1] = { 0 };
    static uint8_t securityMode = 0;
    static char psk[10] = { 0 };

    TriggerReadWiFiConfigurationControlCharacteristicReadConfig(
            &sessions[0], &server, &cookie, &updateStatus, countryCode, ssid, &securityMode, psk);
    TEST_ASSERT_EQUAL(cookie, 100);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 8650852);
    TEST_ASSERT_EQUAL(countryCode, "US");
    TEST_ASSERT_EQUAL(ssid, "WiFiNew1");
    TEST_ASSERT_EQUAL(securityMode, 1);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with invalid PSK
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationInvalidPSK) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue = 40;
    static char newCountry[3] = "UK";
    static char newWiFi[8] = "WiFiNew";
    static char newPSK[8] = "PassNew";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with invalid country code
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationInvalidCountryCode) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue = 40;
    static char newCountry[3] = "U";
    static char newWiFi[8] = "WiFiNew";
    static char newPSK[8] = "PassNew";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            newCountry,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with null country code
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationNullCountryCode) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue = 40;
    static char* countryCode = NULL;
    static char newWiFi[8] = "WiFiNew";
    static char newPSK[9] = "PassNew1";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0],
            &server,
            operationType,
            cookieValue,
            newWiFi,
            countryCode,
            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK,
            newPSK);

    TEST_ASSERT_EQUAL(err, kHAPError_None);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with ssid containing emoji
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationSSIDWithEmoji) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(1).AtMost(1).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    uint8_t requestTLV[40] = { 0x01, 0x01, 0x03, 0x02, 0x02, 0x7a, 0x00, 0x04, 0x01, 0x14, 0x0a, 0x02, 0x55, 0x53,
                               0x0b, 0x18, 0x01, 0x09, 0x43, 0x61, 0x74, 0x7a, 0x32, 0xf0, 0x9f, 0x90, 0x88, 0x02,
                               0x01, 0x01, 0x03, 0x08, 0x54, 0x65, 0x73, 0x74, 0x31, 0x32, 0x33, 0x34 };

    HAPError err = TriggerTLVWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0], &server, (void*) requestTLV, sizeof(requestTLV));

    TEST_ASSERT_EQUAL(err, kHAPError_None);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with invalid ssid
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationInvalidSSID) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    uint8_t requestTLV[40] = { 0x01, 0x01, 0x03, 0x02, 0x02, 0x7a, 0x00, 0x04, 0x01, 0x14, 0x0a, 0x02, 0x55, 0x53,
                               0x0b, 0x18, 0x01, 0x09, 0xC1, 0x61, 0x74, 0x7a, 0x32, 0xf0, 0x9f, 0x90, 0x88, 0x02,
                               0x01, 0x01, 0x03, 0x08, 0x54, 0x65, 0x73, 0x74, 0x31, 0x32, 0x33, 0x34 };

    HAPError err = TriggerTLVWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0], &server, (void*) requestTLV, sizeof(requestTLV));

    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with psk set for security mode
 * set to None
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfiguration_PSK_SecurityMode_None) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    uint8_t requestTLV[40] = { 0x01, 0x01, 0x03, 0x02, 0x02, 0x7a, 0x00, 0x04, 0x01, 0x14, 0x0a, 0x02, 0x55, 0x53,
                               0x0b, 0x18, 0x01, 0x09, 0xC1, 0x61, 0x74, 0x7a, 0x32, 0xf0, 0x9f, 0x90, 0x88, 0x02,
                               0x01, 0x00, 0x03, 0x08, 0x54, 0x65, 0x73, 0x74, 0x31, 0x32, 0x33, 0x34 };

    HAPError err = TriggerTLVWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0], &server, (void*) requestTLV, sizeof(requestTLV));

    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with no psk set for security
 * mode set to WPA2
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfiguration_NoPSK_SecurityMode_None) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    uint8_t requestTLV[40] = { 0x01, 0x01, 0x03, 0x02, 0x02, 0x7a, 0x00, 0x04, 0x01, 0x14, 0x0a,
                               0x02, 0x55, 0x53, 0x0b, 0x18, 0x01, 0x09, 0xC1, 0x61, 0x74, 0x7a,
                               0x32, 0xf0, 0x9f, 0x90, 0x88, 0x02, 0x01, 0x01, 0x03, 0x00 };

    HAPError err = TriggerTLVWriteForUpdateWiFiConfigurationControlCharacteristic(
            &sessions[0], &server, (void*) requestTLV, sizeof(requestTLV));

    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

/**
 * Test writing WiFi Configuration Control characteristics - Simple Update configuration with empty TLV
 */
TEST(TestWriteWiFiConfigurationControlSimpleUpdateConfigurationWithEmptyTLV) {
    HAP_PLATFORM_TCPSTREAMMANAGER_MOCK(tcpStreamManagerMock);
    HAP_PLATFORM_WIFIMANAGER_MOCK(wifiManagerMock);

    bool isListenerOpen = false;
    HAPTime tcpTimeout;

    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerSetTCPUserTimeout)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPTime tcpUserTimeout) {
                tcpTimeout = tcpUserTimeout;
            });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerOpenListener)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager,
                    HAPPlatformTCPStreamListenerCallback callback,
                    void* _Nullable context) { isListenerOpen = true; });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerIsListenerOpen)
            .Do([&](HAPPlatformTCPStreamManagerRef tcpStreamManager) { return (isListenerOpen == true); });
    ALWAYS(tcpStreamManagerMock, HAPPlatformTCPStreamManagerGetListenerPort).Return(kHAPNetworkPort_Any);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerApplyConfiguration).AtLeast(0).AtMost(0).Return(kHAPError_None);

    static HAPAccessoryServer server;
    CreateIPAccessoryServer(&server);
    // Start accessory server.
    HAPAccessoryServerStart(&server, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&server), kHAPAccessoryServerState_Running);

    // Add a pairing
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    AddPairingEntry(controllerPairingID);

    // Create a fake security session
    static HAPSession sessions[1] = {};
    sessions[0].server = &server;
    sessions[0].hap.active = true;
    sessions[0].hap.pairingID = controllerPairingID;
    sessions[0].transportType = kHAPTransportType_IP;

    // Init session keys
    SetupFakeSessionKeys(sessions, HAPArrayCount(sessions));
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple;
    HAPPlatformWiFiManagerCookie cookieValue = 1001;
    static char countryCodeValue[3] = "US";
    HAPError err = TriggerWriteForUpdateWiFiConfigurationControlCharacteristicWithEmptyTLV(
            &sessions[0], &server, operationType, cookieValue, countryCodeValue);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetCookie)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerCookie* cookie) {
                *cookie = 1001;
                return kHAPError_None;
            });
    EXPECT(wifiManagerMock, HAPPlatformWiFiManagerGetUpdateStatus)
            .AtLeast(1)
            .AtMost(1)
            .Do([&](HAPPlatformWiFiManagerRef _Nonnull wiFiManager, HAPPlatformWiFiManagerUpdateStatus* updateStatus) {
                *updateStatus = 0;
                return kHAPError_None;
            });

    static HAPPlatformWiFiManagerCookie cookie = 0;
    static HAPPlatformWiFiManagerUpdateStatus updateStatus = 0;

    TriggerReadWiFiConfigurationControlCharacteristic(&sessions[0], &server, &cookie, &updateStatus);
    TEST_ASSERT_EQUAL(cookie, 1001);
    TEST_ASSERT_EQUAL(updateStatus, (HAPPlatformWiFiManagerUpdateStatus) 0);
    VERIFY_ALL(wifiManagerMock);

    // Set state to idle
    server.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&server);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();

    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    return EXECUTE_TESTS(argc, (const char**) argv);
}
#else
int main() {
    HAPPlatformCreate();
    HAPLogInfo(&kHAPLog_Default, "This test is not supported");
    return 0;
}
#endif
