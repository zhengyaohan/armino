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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.

#include "HAP+API.h"

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer+Broadcast.h"
#include "HAPBLEPeripheralManager.h"
#include "HAPBLEProcedure.h"
#include "HAPPairingBLESessionCache.h"
#include "HAPPlatform+Init.h"
#include "HAPSession.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPCryptoHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#include "Harness/HAPPlatformBLEMock.h"

#include <tuple>
#include <vector>

#define PREFERRED_ADVERTISING_INTERVAL (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))

extern "C" {

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
        &accessoryInformationService,
        &hapProtocolInformationService,
        &pairingService,
        &lightBulbService,
        &batteryService,
        NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

} // extern "C"

// Common mock object
static HAP_PLATFORM_BLE_MOCK(bleMock);

/**
 * Creates a BLE accessory server
 *
 * @return created accessory server
 */
static HAPAccessoryServer* CreateBleAccessoryServer() {
    HAPAccessoryServerOptions serverOptions;
    HAPRawBufferZero(&serverOptions, sizeof serverOptions);

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

    // Accessory server
    static HAPAccessoryServer accessoryServer;

    // Set server options
    serverOptions.maxPairings = kHAPPairingStorage_MinElements;

    serverOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    serverOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    serverOptions.ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    serverOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;

    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };
    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &serverCallbacks, NULL);

    return &accessoryServer;
}

/**
 * Adds a pairing entry into key value store if the entry does not exist.
 *
 * @param controllerPairingID  controller pairing ID
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
 * Sniff BLE attribute registration during accessory server start
 * in order to obtain value handle and ccc descriptor handle for certain characteristics.
 *
 * Test will fail if any of the designated characteristics is not registered.
 *
 * @param server               accessory server
 * @param accessory            accessory
 * @param characteristicTypes  UUIDs of characteristics
 *
 * @return vector of a pair of value handle and ccc descriptor handle for each characteristic.
 */
static std::vector<
        std::tuple<HAPPlatformBLEPeripheralManagerAttributeHandle, HAPPlatformBLEPeripheralManagerAttributeHandle>>
        StartServerAndGetBLEAttributeHandles(
                HAPAccessoryServer* server,
                const HAPAccessory* accessory,
                std::vector<const HAPUUID*> characteristicTypes) {
    std::vector<
            std::tuple<HAPPlatformBLEPeripheralManagerAttributeHandle, HAPPlatformBLEPeripheralManagerAttributeHandle>>
            result(characteristicTypes.size());

    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerAllowsServiceRefresh).Return(true).AtLeast(1);

    HAPPlatformBLEPeripheralManagerAttributeHandle nextHandle = 1;
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerAddCharacteristic)
            .Do([&](HAPPlatformBLEPeripheralManagerRef manager,
                    const HAPPlatformBLEPeripheralManagerUUID* type,
                    HAPPlatformBLEPeripheralManagerCharacteristicProperties props,
                    const void* constBytes,
                    size_t constNumBytes,
                    HAPPlatformBLEPeripheralManagerAttributeHandle* valueHandle,
                    HAPPlatformBLEPeripheralManagerAttributeHandle* cccDescriptorHandle) {
                if (valueHandle) {
                    *valueHandle = nextHandle++;
                }
                if (cccDescriptorHandle) {
                    *cccDescriptorHandle = nextHandle++;
                }
                size_t i = 0;
                for (auto characteristicType : characteristicTypes) {
                    if (HAPUUIDAreEqual((HAPUUID*) type, characteristicType)) {
                        TEST_ASSERT(valueHandle);
                        TEST_ASSERT(cccDescriptorHandle);
                        result[i] = std::make_tuple(*valueHandle, *cccDescriptorHandle);
                    }
                    i++;
                }
                return kHAPError_None;
            });
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerAddDescriptor)
            .Do([&](HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
                    const HAPPlatformBLEPeripheralManagerUUID* type,
                    HAPPlatformBLEPeripheralManagerDescriptorProperties properties,
                    const void* _Nullable constBytes,
                    size_t constNumBytes,
                    HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle) {
                if (descriptorHandle) {
                    *descriptorHandle = nextHandle++;
                }
                return kHAPError_None;
            });

    // Start accessory server
    HAPAccessoryServerStart(server, accessory);

    VERIFY_ALL(bleMock);
    for (auto entry : result) {
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle;
        HAPPlatformBLEPeripheralManagerAttributeHandle cccDescriptorHandle;
        std::tie(valueHandle, cccDescriptorHandle) = entry;
        TEST_ASSERT_NE(valueHandle, 0);
        TEST_ASSERT_NE(cccDescriptorHandle, 0);
    }

    return result;
}

/**
 * Create a fake BLE HAP session
 */
static HAPSession CreateFakeHAPSession(HAPAccessoryServer* server, HAPPlatformKeyValueStoreKey pairingKey) {
    HAPSession session = {
        .server = server,
    };
    session.hap.active = true;
    session.hap.pairingID = pairingKey;
    session.transportType = kHAPTransportType_BLE;
    return session;
}

/**
 * Set up a broadcast key
 */
static void SetupBroadcastKey(HAPAccessoryServer* server) {
    HAPSession session = CreateFakeHAPSession(server, 0);
    HAPDeviceID deviceId;
    HAPError err = HAPDeviceIDGet(server, &deviceId);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    err = HAPBLEAccessoryServerBroadcastGenerateKey(&session, &deviceId);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

// Common variables
static uint8_t advBytes[32];
static size_t numAdvBytes;
static HAPBLEAdvertisingInterval advInterval;
static bool advIsActive;
static HAPAccessoryServer* server;
static HAPPlatformBLEPeripheralManager* blePeripheralManager;
static bool bleIsOn;
static HAPPlatformBLEPeripheralManagerAttributeHandle brightnessValueHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle brightnessCccDescriptorHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle colorTemperatureValueHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle colorTemperatureCccDescriptorHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle batteryLevelValueHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle batteryLevelCccDescriptorHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle chargingStateValueHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle chargingStateCccDescriptorHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle statusLowBatteryValueHandle;
static HAPPlatformBLEPeripheralManagerAttributeHandle statusLowBatteryCccDescriptorHandle;
static HAPBLEAccessoryServerBroadcastEncryptionKey key;
static uint16_t initialGSN;

/**
 * Setup step before a test
 */
TEST_SETUP() {
    // Clear all pairings
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kHAPKeyValueStoreDomain_Pairings);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Add a pairing
    AddPairingEntry(0);

    // Reset common variable states
    advIsActive = false;
    blePeripheralManager = NULL;
    bleIsOn = false;

    // Create a server object
    server = CreateBleAccessoryServer();

    // Set up common mock functions
    ALWAYS(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising)
            .Do([&](HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
                    HAPBLEAdvertisingInterval advertisingInterval,
                    const void* advertisingBytes,
                    size_t numAdvertisingBytes,
                    const void* _Nullable scanResponseBytes,
                    size_t numScanResponseBytes) {
                advInterval = advertisingInterval;
                HAPPrecondition(numAdvertisingBytes <= sizeof(advBytes));
                HAPRawBufferCopyBytes(advBytes, advertisingBytes, numAdvertisingBytes);
                numAdvBytes = numAdvertisingBytes;
                advIsActive = true;
            });
    ALWAYS(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising)
            .Do([&](HAPPlatformBLEPeripheralManagerRef blePeripheralManager) { advIsActive = false; });
    ALWAYS(bleMock, HAPPlatformBLEInitialize).Do([&](HAPPlatformBLEPeripheralManager* manager) {
        blePeripheralManager = manager;
        bleIsOn = true;
    });
    ALWAYS(bleMock, HAPPlatformBLEPeripheralManagerSetDelegate)
            .Do([&](HAPPlatformBLEPeripheralManagerRef manager,
                    const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
                TEST_ASSERT(manager);
                if (delegate) {
                    manager->delegate = *delegate;
                } else {
                    HAPRawBufferZero(&manager->delegate, sizeof manager->delegate);
                }
            });
    ALWAYS(bleMock, HAPPlatformBLEDeinitialize).Do([&]() { bleIsOn = false; });

    // Start server and obtain registered BLE attribute handles for multiple characteristics,
    std::vector<const HAPUUID*> characteristicTypes = { &kHAPCharacteristicType_Brightness,
                                                        &kHAPCharacteristicType_ColorTemperature,
                                                        &kHAPCharacteristicType_BatteryLevel,
                                                        &kHAPCharacteristicType_ChargingState,
                                                        &kHAPCharacteristicType_StatusLowBattery };
    auto attribs = StartServerAndGetBLEAttributeHandles(server, &accessory, characteristicTypes);
    TEST_ASSERT_EQUAL(attribs.size(), characteristicTypes.size());
    std::tie(brightnessValueHandle, brightnessCccDescriptorHandle) = attribs[0];
    std::tie(colorTemperatureValueHandle, colorTemperatureCccDescriptorHandle) = attribs[1];
    std::tie(batteryLevelValueHandle, batteryLevelCccDescriptorHandle) = attribs[2];
    std::tie(chargingStateValueHandle, chargingStateCccDescriptorHandle) = attribs[3];
    std::tie(statusLowBatteryValueHandle, statusLowBatteryCccDescriptorHandle) = attribs[4];

    // Pretend that broadcast key was set and broadcast was enabled.
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 10);
    SetupBroadcastKey(server);
    err = HAPBLECharacteristicEnableBroadcastNotifications(
            &lightBulbBrightnessCharacteristic,
            &lightBulbService,
            &accessory,
            kHAPBLECharacteristicBroadcastInterval_20Ms,
            server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    err = HAPBLECharacteristicEnableBroadcastNotifications(
            &lightBulbColorTemperatureCharacteristic,
            &lightBulbService,
            &accessory,
            kHAPBLECharacteristicBroadcastInterval_20Ms,
            server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    err = HAPBLECharacteristicEnableBroadcastNotifications(
            &batteryLevelCharacteristic,
            &batteryService,
            &accessory,
            kHAPBLECharacteristicBroadcastInterval_20Ms,
            server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    err = HAPBLECharacteristicEnableBroadcastNotifications(
            &batteryChargingStateCharacteristic,
            &batteryService,
            &accessory,
            kHAPBLECharacteristicBroadcastInterval_20Ms,
            server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    err = HAPBLECharacteristicEnableBroadcastNotifications(
            &batteryStatusLowCharacteristic,
            &batteryService,
            &accessory,
            kHAPBLECharacteristicBroadcastInterval_20Ms,
            server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Read back the broadcast key
    uint16_t keyExpirationGSN;
    HAPDeviceID advertisingId;
    err = HAPBLEAccessoryServerBroadcastGetParameters(server, &keyExpirationGSN, &key, &advertisingId);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Verify server state
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 60);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(server), kHAPAccessoryServerState_Running);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(bleIsOn);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    // Retrieve initial GSN value from advertisement data
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) = HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    initialGSN = advData.regular.gsn;
}

/**
 * Teardown step after a test so that next test can run in a clean state.
 */
TEST_TEARDOWN() {
    // Clears all timers
    HAPPlatformTimerDeregisterAll();

    // Clear the didIncrement field of GSN
    // Reset disconnected events coalescing.
    HAPBLEAccessoryServerGSN gsn;
    HAPError err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    uint8_t gsnBytes[] = { HAPExpandLittleUInt16(gsn.gsn), (uint8_t) 0x00 };
    err = HAPPlatformKeyValueStoreSet(
            server->platform.keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_BLEGSN,
            gsnBytes,
            sizeof gsnBytes);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Fake Pair-Verify
 *
 * This function changes the BLE HAP session state to a secure state
 * as if pair-verify has succeeded.
 */
static void FakePairVerify(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->ble.connection.connected);
    HAPPrecondition(server->ble.storage);
    HAPSession* session = server->ble.storage->session;
    HAPPrecondition(session);
    session->hap.active = true;
    session->hap.isTransient = false;
}

/**
 * Enable connected notification
 *
 * @param blePeripheralManager      BLE peripheral manager
 * @param cccDescriptorHandle       ccc descriptor handle of the characteristic to enable notifications of
 * @param server                    accessory server
 */
static void EnableConnectedNotification(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerAttributeHandle cccDescriptorHandle,
        HAPAccessoryServer* server) {
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleWriteRequest);

    uint8_t bytes[] = { 2, 0 };
    HAPError err = blePeripheralManager->delegate.handleWriteRequest(
            blePeripheralManager, 0, cccDescriptorHandle, bytes, sizeof bytes, server);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Disable connected notification
 *
 * @param blePeripheralManager      BLE peripheral manager
 * @param cccDescriptorHandle       ccc descriptor handle of the characteristic to disable notifications of
 * @param server                    accessory server
 */
static void DisableConnectedNotification(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerAttributeHandle cccDescriptorHandle,
        HAPAccessoryServer* server) {
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleWriteRequest);

    uint8_t bytes[] = { 0, 0 };
    HAPError err = blePeripheralManager->delegate.handleWriteRequest(
            blePeripheralManager, 0, cccDescriptorHandle, bytes, sizeof bytes, server);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Test a typical broadcast notification in an unconnected state
 * and controller doesn't connect to acknowledge the broadcast notification.
 */
TEST(TestUnacknowledgedBroadcastNotification) {
    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Check that the advertisement continues for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds without connection, regular advertisement (Disconnected Event) begins
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);

    // Check that advertisement remains fast for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds, advertisement slows
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
}

/**
 * Test a typical broadcast notification in an unconnected state
 * and controller connects to acknowledge the broadcast notification.
 */
TEST(TestAcknowledgedBroadcastNotification) {
    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Acknowledge the broadcast notification with connection after a second
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Disconnect after a hundred millisecond and check that regular advertisement starts.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 100);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);

    // Check that advertisement remains fast for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds, advertisement slows
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
}

/** Test a typical connected notification */
TEST(TestConnectedNotification) {
    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Fake pair-verify to change the HAP session state to a secure state.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    FakePairVerify(server);
    VERIFY_ALL(bleMock);

    // Controller enables connected notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
    EnableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
    VERIFY_ALL(bleMock);

    // App raises an event which should trigger a zero-length indication
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication)
            .If([=](HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
                    HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
                    HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
                    const void* _Nullable bytes,
                    size_t numBytes) { return valueHandle == brightnessValueHandle && numBytes == 0; })
            .AtLeast(1)
            .AtMost(1);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    // Note that typical controller will read out the characteristic value here
    // but it is irrelevant to this unit test and hence is skipped.

    // Disconnect the controller. initialGSN must have been rolled up by one.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);
}

/**
 * Test a connected notification which initially fails due to low layer stack state.
 * This tests a situation where the app raises and event from a callback which was issued
 * in the middle of processing a GATT request.
 */
TEST(TestConnectedNotificationDuringGATTProcedure) {
    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Fake pair-verify to change the HAP session state to a secure state.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    FakePairVerify(server);
    VERIFY_ALL(bleMock);

    // Controller enables connected notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
    EnableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
    VERIFY_ALL(bleMock);

    // App raises an event which should trigger a zero-length indication
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication)
            .If([=](HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
                    HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
                    HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
                    const void* _Nullable bytes,
                    size_t numBytes) { return valueHandle == brightnessValueHandle && numBytes == 0; })
            .AtLeast(1)
            .Repeats(1)
            .Return(kHAPError_InvalidState);
    // The send handle value indication should be invoked again till it succeeds.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication)
            .If([=](HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
                    HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
                    HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
                    const void* _Nullable bytes,
                    size_t numBytes) { return valueHandle == brightnessValueHandle && numBytes == 0; })
            .AtLeast(1)
            .AtMost(1)
            .Return(kHAPError_None);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);

    // Note that typical controller will read out the characteristic value here
    // but it is irrelevant to this unit test and hence is skipped.

    // Disconnect the controller. initialGSN must have been rolled up by one.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);
}

/** Common subroutine to test a queued broadcast notification */
static void TestSingleQueuedBroadcastNotificationCommon(int variant) {
    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    if (variant == 0 || variant == 2) {
        // Note that no pair-verify is performed.

        // Pretend that controller enabled connected notification previously
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
        EnableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
        VERIFY_ALL(bleMock);
    } else if (variant == 1) {
        // Fake pair-verify to change the HAP session state to a secure state.
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
        FakePairVerify(server);
        VERIFY_ALL(bleMock);

        // Disable connected notification
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
        DisableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
        VERIFY_ALL(bleMock);
    }

    // App raises an event which should not trigger a zero-length indication.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    if (variant == 2) {
        // App raises the same event again which should not trigger a zero-length indication.
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
        HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
        VERIFY_ALL(bleMock);
    }

    // Disconnect the controller. Queued broadcast notification must be included in the advertisement.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Check that the advertisement continues for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds without connection, regular advertisement (Disconnected Event) begins
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);
}

/** Test a queued broadcast notification when controller did not establish a secure session */
TEST(TestSingleQueuedBroadcastNotificationForNoSecureSession) {
    TestSingleQueuedBroadcastNotificationCommon(0);
}

/** Test a queued broadcast notification when controller did not enable connected notification  */
TEST(TestSingleQueuedBroadcastNotificationForNoConnectedNotification) {
    TestSingleQueuedBroadcastNotificationCommon(1);
}

/** Test multiple variable value changes causing only a single broadcast */
TEST(TestSingleQueuedBroadcastNotificationForMultipleChangesOfSameCharacteristic) {
    TestSingleQueuedBroadcastNotificationCommon(2);
}

/** Common routine for multiple broadcast notifications test */
static void TestMultipleQueuedBroadcastNotificationsCommon(int variant) {
    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Note that no pair-verify is performed.

    // Pretend that controller enabled connected notification previously
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
    EnableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, batteryLevelCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, chargingStateCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, statusLowBatteryCccDescriptorHandle, server);
    VERIFY_ALL(bleMock);

    // App raises an event which should not trigger a zero-length indication.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    // App raises another event which should not trigger a zero-length indication either.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &batteryLevelCharacteristic, &batteryService, &accessory);
    VERIFY_ALL(bleMock);

    // App raises another event which should not trigger a zero-length indication either.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &batteryChargingStateCharacteristic, &batteryService, &accessory);
    VERIFY_ALL(bleMock);

    uint16_t gsnOffset = 0;

    if (variant == 2) {
        // Broadcast queue must be full by now.

        // App raises another event which should not trigger a zero-length indication either.
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
        HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
        VERIFY_ALL(bleMock);

        // In this case initialGSN shall be incremented by one more for the first broadcast to indicate that
        // there was another change of characteristic values beyond the broadcast notification.
        ++gsnOffset;
    }

    // IIDs of expected broadcast characteristics
    std::vector<uint64_t> expectedIids = { lightBulbBrightnessCharacteristic.iid,
                                           batteryLevelCharacteristic.iid,
                                           batteryChargingStateCharacteristic.iid };

    for (size_t i = 0; i < 3; i++) {
        // Disconnect the controller. Queued broadcast notification must be included in the advertisement.
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
        TEST_ASSERT(blePeripheralManager);
        TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
        blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

        ++gsnOffset;
        HAPError err;
        HAPBLEAdvertisementData advData;
        std::tie(err, advData) = HAPBLEAdvertisementData::FromBytes(
                advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + gsnOffset);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT(advData.hasValidManufacturerData);
        TEST_ASSERT(advData.isEncryptedNotification);
        TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + gsnOffset);
        bool found = false;
        for (auto it = expectedIids.begin(); it != expectedIids.end(); ++it) {
            if (*it == advData.encryptedNotification.iid) {
                expectedIids.erase(it);
                found = true;
                break;
            }
        }
        TEST_ASSERT(found);

        if (variant == 0 || variant == 2) {
            // Acknowledge the broadcast notification with connection after 50 millisecond
            EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
            EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
            HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 50);
            TEST_ASSERT(blePeripheralManager);
            TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
            blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
            VERIFY_ALL(bleMock);
            TEST_ASSERT(!advIsActive);
        } else if (variant == 1) {
            // Check that after three seconds without connection, regular advertisement (Disconnected Event) begins
            EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
            HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3 + HAPMillisecond * 50);
            VERIFY_ALL(bleMock);
            TEST_ASSERT(advIsActive);
            TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

            // Note that initialGSN must have incremented by one because other queued broadcasts were present.
            // That is, controller should be notified that there are other changes than the last broadcast notification.
            ++gsnOffset;
            std::tie(err, advData) = HAPBLEAdvertisementData::FromBytes(
                    advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + gsnOffset);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            TEST_ASSERT(advData.hasValidManufacturerData);
            TEST_ASSERT(!advData.isEncryptedNotification);
            TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + gsnOffset);

            // Connect controller
            EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
            EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
            HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 50);
            TEST_ASSERT(blePeripheralManager);
            TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
            blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
            VERIFY_ALL(bleMock);
            TEST_ASSERT(!advIsActive);
            break;
        }
    }

    // Disconnect after 2 seconds and check that regular advertisement starts.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + gsnOffset);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + gsnOffset);
}

/** Test maximum number of broadcast notifications */
TEST(TestMaximumNumberOfQueuedBroadcastNotifications) {
    TestMultipleQueuedBroadcastNotificationsCommon(0);
}

/** Test discarding queued broadcast notifications when unacknowledged */
TEST(TestDiscardingUnacknowledgedBroadcastNotifications) {
    TestMultipleQueuedBroadcastNotificationsCommon(1);
}

/** Test more notifications than can be queued */
TEST(TestFullBroadcastNotificationQueue) {
    TestMultipleQueuedBroadcastNotificationsCommon(2);
}

/** Test notification of another characteristic in the middle of a broadcast */
TEST(TestRaisedEventOfAnotherCharacterisitcWhileBroadcasting) {
    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Raise another event before controller acknowledges previous advertisement
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    // Note that previous advertisement should still continue.
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Wait another second and previous advertisement should still continue.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Acknowledge the broadcast notification with connection before 3 seconds expires
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 50);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Disconnect after a hundred millisecond and check that next broadcast advertisement starts.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 100);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbColorTemperatureCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 2);
}

/** Test another change of value fo the same characteristic being broadcast */
TEST(TestRaisedEventOfSameCharacteristicWhileBroadcasting) {
    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Raise another event of the same characteristic before controller acknowledges previous advertisement
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    // A new advertisement should have started, i.e., initialGSN should have incremented.
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Wait another second and previous advertisement should still continue.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Acknowledge the broadcast notification with connection before 3 seconds expires
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 50);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Disconnect after a hundred millisecond and check that regular advertisement starts.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 100);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 2);
}

/** Test handing of disconnected event while a broadcast event is queued. */
TEST(TestDisconnectedEventWhenBroadcastEventIsQueued) {
    // Disable broadcast notification for a characteristic to use exclusively for disconnected event.
    HAPError err = HAPBLECharacteristicDisableBroadcastNotifications(
            &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory, server->platform.keyValueStore);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Note that session is not secure. So broadcast notification should be queued.

    // App raises an event which should queue a broadcast notification.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    // App raises another event which disabled broadcast notification.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    // Disconnect the controller. Queued broadcast notification must be included in the advertisement.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));

    // Note that because another characteristic value changes without queued broadcast, initialGSN should be incremented
    // by 2.
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 2);
}

/**
 * Common subroutine to test that accessory send a disconnected event for a new non-broadcasted event after
 * unacknowledged broadcast notification and then broadcast notification again after controller connects and
 * disconnects.
 */
static void TestDisconnectedEventAfterUnacknowledgedBroadcastNotificationCommon(int variant) {
    // Disable broadcast notification for a characteristic to use exclusively for disconnected event.
    HAPError err = HAPBLECharacteristicDisableBroadcastNotifications(
            &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory, server->platform.keyValueStore);
    err = HAPBLECharacteristicDisableBroadcastNotifications(
            &batteryLevelCharacteristic, &batteryService, &accessory, server->platform.keyValueStore);

    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Check that the advertisement continues for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds without connection, regular advertisement (Disconnected Event) begins
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);

    if (variant == 0 || variant == 3) {
        // Check that advertisement remains fast for three seconds
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
        VERIFY_ALL(bleMock);

        // Check that after three seconds, advertisement slows
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    } else if (variant == 1) {
        // Check that the advertisement remains fast for one second
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    } else if (variant == 2) {
        // Check that advertisement remains fast for three seconds
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
        VERIFY_ALL(bleMock);

        // Make exact three seconds and create a race condition where the timer gets queued.
        HAPPlatformTimerEmulateQueuedTimerEvents(true);
        HAPPlatformClockAdvance(HAPMillisecond * 10);
    }

    // Raise a non-broadcast event to verify a disconnected event
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    if (variant == 0) {
        HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);
    }
    HAPAccessoryServerRaiseEvent(server, &batteryLevelCharacteristic, &batteryService, &accessory);
    if (variant == 2) {
        // Now that the event is raised, dequeue and execute the expired timer to complete the race condition.
        HAPPlatformTimerEmulateQueuedTimerEvents(false);
    }
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 2);

    // Raise another event with another non-broadcast characteristic to verify neither broadcasted event
    // nor disconnected event (i.e., no GSN increment)
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 2);
    HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    // Because GSN should not increment, Slow advertisement continues.
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    // GSN should not have incremented this time
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 2);

    if (variant == 0 || variant == 1 || variant == 2) {
        // Connect a controller
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);
        TEST_ASSERT(blePeripheralManager);
        TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
        blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(!advIsActive);

        // Disconnect after a hundred millisecond and check that regular advertisement starts.
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 100);
        TEST_ASSERT(blePeripheralManager);
        TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
        blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
        std::tie(err, advData) =
                HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT(advData.hasValidManufacturerData);
        TEST_ASSERT(!advData.isEncryptedNotification);
        TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 2);
    } else if (variant == 3) {
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
    }

    // Raise an event to verify broadcast notification after going through the connection above
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 3);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 3);
}

/**
 * Test that accessory send a disconnected event for a new event after a while from unacknowledged broadcast
 * notification and then broadcast notification again after controller connects and disconnects.
 */
TEST(TestDisconnectedEventOneMinuteAfterUnacknowledgedBroadcastNotification) {
    TestDisconnectedEventAfterUnacknowledgedBroadcastNotificationCommon(0);
}

/**
 * Test that accessory send a disconnected event for a new event before three seconds passed after unacknowledged
 * broadcast notification and then broadcast notification again after controller connects and disconnects.
 */
TEST(TestDisconnectedEventOneSecondAfterUnacknowledgedBroadcastNotification) {
    TestDisconnectedEventAfterUnacknowledgedBroadcastNotificationCommon(1);
}

/**
 * Test that accessory send a disconnected event for a new event at exact three seconds after unacknowledged
 * broadcast notification and then broadcast notification again after controller connects and disconnects.
 *
 * The purpose of this test is to verify that the timer logic handles race condition of one expiry and setting up
 * another.
 */
TEST(TestDisconnectedEventThreeSecondsAfterUnacknowledgedBroadcastNotification) {
    TestDisconnectedEventAfterUnacknowledgedBroadcastNotificationCommon(2);
}

/**
 * Test that accessory send a disconnected event and connected event for new events after a while from unacknowledged
 * broadcast notification.
 */
TEST(TestDisconnectedEventAndConnectedEventOneMinuteAfterUnacknowledgedBroadcastNotification) {
    TestDisconnectedEventAfterUnacknowledgedBroadcastNotificationCommon(0);
}

/**
 * Common subroutine to test that accessory send a broadcasted event for a new event after unacknowledged broadcast
 * notification and then broadcast notification again after controller connects and disconnects.
 */
static void TestBroadcastedEventAfterUnacknowledgedBroadcastNotificationCommon(int variant) {
    // Raise an event to verify broadcast notification
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    HAPError err;
    HAPBLEAdvertisementData advData;
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);

    // Check that the advertisement continues for three seconds
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
    VERIFY_ALL(bleMock);

    // Check that after three seconds without connection, regular advertisement (Disconnected Event) begins
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 1);

    if (variant == 0) {
        // Check that advertisement remains fast for three seconds
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
        VERIFY_ALL(bleMock);

        // Check that after three seconds, advertisement slows
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 10);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_NE(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    } else if (variant == 1) {
        // Check that the advertisement remains fast for one second
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond);
        VERIFY_ALL(bleMock);
        TEST_ASSERT(advIsActive);
        TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    } else if (variant == 2) {
        // Check that advertisement remains fast for three seconds
        EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStopAdvertising).AtMost(0);
        EXPECT(bleMock, HAPPlatformBLEPeripheralManagerStartAdvertising).AtMost(0);
        HAPPlatformTimerEmulateClockAdvances(HAPSecond * 2 + HAPMillisecond * 990);
        VERIFY_ALL(bleMock);

        // Make exact three seconds and create a race condition where the timer gets queued.
        HAPPlatformTimerEmulateQueuedTimerEvents(true);
        HAPPlatformClockAdvance(HAPMillisecond * 10);
    }

    // Raise an event again to verify a broadcasted event
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    if (variant == 0) {
        HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);
    }
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    if (variant == 2) {
        // Now that the event is raised, dequeue and execute the expired timer to complete the race condition.
        HAPPlatformTimerEmulateQueuedTimerEvents(false);
    }
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 2);

    // Raise another event with another characteristic that has broadcast enabled to verify broadcasted event
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 2);
    HAPAccessoryServerRaiseEvent(server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 3);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbColorTemperatureCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 3);

    // Connect a controller
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Disconnect after a hundred millisecond and check that regular advertisement starts.
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 100);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleDisconnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 3);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(!advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.regular.gsn, initialGSN + 3);

    // Raise an event to verify broadcast notification after going through the connection above
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(advIsActive);
    TEST_ASSERT_EQUAL(advInterval, HAPBLEAdvertisingIntervalCreateFromMilliseconds(20));
    std::tie(err, advData) =
            HAPBLEAdvertisementData::FromBytes(advBytes, numAdvBytes, (HAPSessionKey*) &key, initialGSN + 4);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(advData.hasValidManufacturerData);
    TEST_ASSERT(advData.isEncryptedNotification);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.iid, lightBulbBrightnessCharacteristic.iid);
    TEST_ASSERT_EQUAL(advData.encryptedNotification.gsn, initialGSN + 4);
}

/**
 * Test that accessory send a broadcast event for a new event after a while from unacknowledged broadcast
 * notification and then broadcast notification again after controller connects and disconnects.
 */
TEST(TestBroadcastedEventOneMinuteAfterUnacknowledgedBroadcastNotification) {
    TestBroadcastedEventAfterUnacknowledgedBroadcastNotificationCommon(0);
}

/**
 * Test that accessory send a broadcast event for a new event before three seconds passed after unacknowledged
 * broadcast notification and then broadcast notification again after controller connects and disconnects.
 */
TEST(TestBroadcastedEventOneSecondAfterUnacknowledgedBroadcastNotification) {
    TestBroadcastedEventAfterUnacknowledgedBroadcastNotificationCommon(1);
}

/**
 * Test that accessory send a broadcast event for a new event at exact three seconds after unacknowledged
 * broadcast notification and then broadcast notification again after controller connects and disconnects.
 *
 * The purpose of this test is to verify that the timer logic handles race condition of one expiry and setting up
 * another.
 */
TEST(TestBroadcastedEventThreeSecondsAfterUnacknowledgedBroadcastNotification) {
    TestBroadcastedEventAfterUnacknowledgedBroadcastNotificationCommon(2);
}

/**
 * Tests to ensure that the GSN is incremented, if not already, when a pending broadcast event is removed
 * from the queue.
 */
static void TestGSNAfterPurgingQueuedBroadcastEvents(int variant) {
    // Controller connects
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerCancelCentralConnection).AtMost(0);
    TEST_ASSERT(blePeripheralManager);
    TEST_ASSERT(blePeripheralManager->delegate.handleConnectedCentral);
    blePeripheralManager->delegate.handleConnectedCentral(blePeripheralManager, 0, server);
    VERIFY_ALL(bleMock);
    TEST_ASSERT(!advIsActive);

    // Note that no pair-verify is performed.

    // Pretend that controller enabled connected notification previously
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPMillisecond * 20);
    EnableConnectedNotification(blePeripheralManager, brightnessCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, batteryLevelCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, chargingStateCccDescriptorHandle, server);
    EnableConnectedNotification(blePeripheralManager, statusLowBatteryCccDescriptorHandle, server);
    VERIFY_ALL(bleMock);

    // App raises an event which should not trigger a zero-length indication.
    EXPECT(bleMock, HAPPlatformBLEPeripheralManagerSendHandleValueIndication).AtMost(0);
    EXPECT(bleMock, HAPPlatformBLEDeinitialize).AtMost(0);
    HAPPlatformTimerEmulateClockAdvances(HAPSecond * 3);
    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    VERIFY_ALL(bleMock);

    TEST_ASSERT_EQUAL((size_t) 1, server->ble.adv.queuedBroadcastEvents.numQueuedEvents);

    HAPError err;
    HAPBLEAccessoryServerGSN gsn;
    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    TEST_ASSERT(!err);
    uint16_t previousGSN = gsn.gsn;
    gsn.didIncrementInDisconnectedState = false;

    if (variant == 0) {
        HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(
                server, &lightBulbColorTemperatureCharacteristic, &lightBulbService, &accessory);
    } else if (variant == 1) {
        HAPBLEAccessoryServerRemoveQueuedBroadcastEvent(
                server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    }

    err = HAPBLEAccessoryServerGetGSN(server->platform.keyValueStore, &gsn);
    TEST_ASSERT(!err);
    if (variant == 0) {
        TEST_ASSERT_EQUAL(gsn.gsn, previousGSN);
        TEST_ASSERT_EQUAL((size_t) 1, server->ble.adv.queuedBroadcastEvents.numQueuedEvents);
    } else if (variant == 1) {
        TEST_ASSERT_EQUAL(gsn.gsn, previousGSN + 1);
        TEST_ASSERT_EQUAL((size_t) 0, server->ble.adv.queuedBroadcastEvents.numQueuedEvents);
    }
}

TEST(TestDoNotIncrementGSNAfterNotRemovingQueuedBroadcastEvent) {
    TestGSNAfterPurgingQueuedBroadcastEvents(0);
}

TEST(TestIncrementGSNAfterRemovingQueuedBroadcastEvent) {
    TestGSNAfterPurgingQueuedBroadcastEvents(1);
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
