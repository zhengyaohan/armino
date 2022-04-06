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

#include "HAPPlatform+Init.h"
#include "HAPPlatformFeatures.h"

#include "HAPPlatformSetup+Init.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformServiceDiscoveryMock.h"
#include "Harness/HAPPlatformThreadMock.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/ThreadReattachHelper.h"
#include "Harness/UnitTestFramework.h"

#if !HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
int main() {
    return 0;
}
#else

#define kAppConfig_NumAccessoryServerSessionStorageBytes ((size_t) 256)
#define kAppConfig_NumThreadSessions                     ((size_t) kHAPThreadAccessoryServerStorage_MinSessions)
#define PREFERRED_ADVERTISING_INTERVAL                   (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))

static HAPAccessory InitAccessory(void) {
    HAPAccessory accessory = {
        .aid = 1,
        .category = kHAPAccessoryCategory_Locks,
        .name = "Acme Test",
        .productData = "03d8a775e3644573",
        .manufacturer = "Acme",
        .model = "Test1,1",
        .serialNumber = "099DB48E9E28",
        .firmwareVersion = "1",
        .hardwareVersion = "1",
    };
    static const HAPService* services[] = {
        &accessoryInformationService, &hapProtocolInformationService, &pairingService, &threadManagementService, NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

/**
 * Creates a BLE and Thread accessory server
 *
 * @return created accessory server
 */
static HAPAccessoryServer* CreateBleThreadAccessoryServer() {
    HAPAccessoryServerOptions serverOptions;
    HAPRawBufferZero(&serverOptions, sizeof serverOptions);

    // Thread transport storage
    static uint8_t sessionStorageBytes[kAppConfig_NumAccessoryServerSessionStorageBytes];
    HAPRawBufferZero(sessionStorageBytes, sizeof sessionStorageBytes);
    static HAPThreadSession threadSessions[kAppConfig_NumThreadSessions];
    HAPRawBufferZero(threadSessions, sizeof threadSessions);
    static HAPThreadAccessoryServerStorage threadAccessoryServerStorage;
    threadAccessoryServerStorage = (HAPThreadAccessoryServerStorage) {
        .sessions = threadSessions,
        .numSessions = HAPArrayCount(threadSessions),
    };

    // BLE storage
    static HAPBLEGATTTableElement gattTableElements[kAttributeCount];
    static HAPPairingBLESessionCacheEntry sessionCacheElements[kHAPBLESessionCache_MinElements];
    static HAPSession session;
    static uint8_t procedureBytes[2048];
    static HAPBLEProcedure procedures[1];

    static HAPBLEAccessoryServerStorage bleAccessoryServerStorage = {
        .gattTableElements = gattTableElements,
        .numGATTTableElements = HAPArrayCount(gattTableElements),
        .sessionCacheElements = sessionCacheElements,
        .numSessionCacheElements = HAPArrayCount(sessionCacheElements),
        .session = &session,
        .procedures = procedures,
        .numProcedures = HAPArrayCount(procedures),
        .procedureBuffer = { .bytes = procedureBytes, .numBytes = sizeof procedureBytes },
    };

    // Dependency objects
    static HAPPlatformThreadCoAPManager coapManager;
    static HAPPlatformServiceDiscovery threadServiceDiscovery;

    // Accessory server
    static HAPAccessoryServer accessoryServer;

    // Set server options
    serverOptions.maxPairings = kHAPPairingStorage_MinElements;

    serverOptions.sessionStorage.bytes = sessionStorageBytes;
    serverOptions.sessionStorage.numBytes = sizeof sessionStorageBytes;
    serverOptions.thread.transport = &kHAPAccessoryServerTransport_Thread;
    serverOptions.thread.accessoryServerStorage = &threadAccessoryServerStorage;

    serverOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    serverOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    serverOptions.ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    serverOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;

    static const HAPPlatformThreadDeviceCapabilities deviceType = kHAPPlatformThreadDeviceCapabilities_MED;
    serverOptions.thread.deviceParameters.deviceType = deviceType;
    serverOptions.thread.getNextReattachDelay = GetNextThreadReattachDelay;
    serverOptions.thread.getNextReattachDelayContext = (void*) &deviceType;

    // Initialize Thread platform used in accessory server
    platform.thread.coapManager = &coapManager;
    platform.thread.serviceDiscovery = &threadServiceDiscovery;

    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &serverCallbacks, NULL);

    return &accessoryServer;
}

static HAPSession CreateFakeBLEHAPSession(HAPAccessoryServer* server, HAPPlatformKeyValueStoreKey pairingKey) {
    HAPSession session = {
        .server = server,
    };
    session.hap.active = true;
    session.hap.pairingID = pairingKey;
    session.transportType = kHAPTransportType_BLE;
    return session;
}

TEST(TestHandleThreadControlWrite) {
    HAPAccessoryServer* server = CreateBleThreadAccessoryServer();
    HAPSession session = CreateFakeBLEHAPSession(server, 0);

    const HAPTLV8CharacteristicWriteRequest request = {
        .transportType = kHAPTransportType_BLE,
        .session = &session,
        .characteristic = &threadManagementControlCharacteristic,
        .service = &threadManagementService,
        .accessory = &accessory,
    };

    // Bad request with Start operation type only
    uint8_t requestBuffer[1024];
    HAPTLVWriter requestWriter;
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
    const uint8_t tlvBytes[] = { 0x01 };
    HAPTLV tlv = { .value = {
                           .bytes = tlvBytes,
                           .numBytes = 1,
                   },
                   .type = 1 };
    HAPError err = HAPTLVWriterAppend(&requestWriter, &tlv);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    void* requestBytes;
    size_t numRequestBytes;
    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

    HAPTLVReader requestReader;
    HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

    err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // Good request
    HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
    err = HAPTLVWriterAppend(&requestWriter, &tlv);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t subBuffer[1024];
    HAPTLVWriter subWriter;
    HAPTLVWriterCreate(&subWriter, subBuffer, sizeof subBuffer);
    HAPTLV networkNameTLV = { .value = {
                                      .bytes = "My Network",
                                      .numBytes = 10,
                              },
                              .type = 1 };
    err = HAPTLVWriterAppend(&subWriter, &networkNameTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t channelBytes[] = { 0x0B, 0x00 };
    HAPTLV channelTLV = { .value = {
                                  .bytes = channelBytes,
                                  .numBytes = 2,
                          },
                          .type = 2 };
    err = HAPTLVWriterAppend(&subWriter, &channelTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t panIdBytes[] = { 0xCD, 0xAB };
    HAPTLV panIdTLV = { .value = {
                                .bytes = panIdBytes,
                                .numBytes = 2,
                        },
                        .type = 3 };
    err = HAPTLVWriterAppend(&subWriter, &panIdTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t extPanIdBytes[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    HAPTLV extPanIdTLV = { .value = {
                                   .bytes = extPanIdBytes,
                                   .numBytes = 8,
                           },
                           .type = 4 };
    err = HAPTLVWriterAppend(&subWriter, &extPanIdTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t masterKeyBytes[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                                 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00 };
    HAPTLV masterKeyTLV = { .value = {
                                    .bytes = masterKeyBytes,
                                    .numBytes = 16,
                            },
                            .type = 5 };
    err = HAPTLVWriterAppend(&subWriter, &masterKeyTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPTLVWriterGetBuffer(&subWriter, &requestBytes, &numRequestBytes);

    HAPTLV networkCredentialsTLV = { .value = {
                                             .bytes = requestBytes,
                                             .numBytes = numRequestBytes,
                                     },
                                     .type = 2 };
    err = HAPTLVWriterAppend(&requestWriter, &networkCredentialsTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint8_t formingAllowedBytes[] = { 0 };
    HAPTLV formingAllowedTLV = { .value = {
                                         .bytes = formingAllowedBytes,
                                         .numBytes = 1,
                                 },
                                 .type = 3 };
    err = HAPTLVWriterAppend(&requestWriter, &formingAllowedTLV);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);
    HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

    // Write controller cannot be tested without emulating the whole server.
    // Just test the parser for now.
    HAPCharacteristicValue_ThreadManagementControl controlValue;
    err = HAPTLVReaderDecode(&requestReader, &kHAPCharacteristicTLVFormat_ThreadManagementControl, &controlValue);
    // err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

TEST(TestHandleThreadCurrentTransportRead) {
    HAPAccessoryServer* server = CreateBleThreadAccessoryServer();
    HAPSession session = CreateFakeBLEHAPSession(server, 0);

    // Request over Thread first
    HAPBoolCharacteristicReadRequest request = {
        .transportType = kHAPTransportType_Thread,
        .session = &session,
        .characteristic = &threadManagementCurrentTransportCharacteristic,
        .service = &threadManagementService,
        .accessory = &accessory,
    };

    bool value;
    HAPError err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(value, true);

    // Try over BLE
    request.transportType = kHAPTransportType_BLE;
    err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(value, false);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();
    HAPPlatformSetupDrivers();

    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    return EXECUTE_TESTS(argc, (const char**) argv);
}

#endif
