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

#include "HAPPlatform+Init.h"

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEPeripheralManager.h"
#include "HAPBLEProcedure.h"
#include "HAPPairingBLESessionCache.h"
#include "HAPPlatformSetup+Init.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

#include <functional>

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
        &accessoryRuntimeInformationService,
        NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

} // extern "C"

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

/** User data context to route callback into test lambda function */
typedef struct {
    std::function<void()> function;
} CallbackRouteContext;
extern "C" {
/** Hear beat change callback function to use for testing */
static void HandleHeartBeatChange(void* _Nullable context_) {
    HAPPrecondition(context_);
    CallbackRouteContext* context = (CallbackRouteContext*) context_;
    context->function();
}
}

static uint32_t ReadHeartBeat(HAPAccessoryServer* server, HAPSession* session) {
    const HAPUInt32CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_BLE,
        .session = session,
        .characteristic = &accessoryRuntimeInformationHeartBeatCharacteristic,
        .service = &accessoryRuntimeInformationService,
        .accessory = &accessory,
    };

    uint32_t value;
    HAPError err = request.characteristic->callbacks.handleRead(server, &request, &value, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    return value;
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
 * Test HAPAccessoryServerStartHeartBeat function
 */
TEST(TestHAPAccessoryServerStartHeartBeat) {
    // Create a server object
    HAPAccessoryServer* server = CreateBleAccessoryServer();
    HAPTime lastCallbackTime = 0;
    CallbackRouteContext context;
    context.function = [&]() { lastCallbackTime = HAPPlatformClockGetCurrent(); };
    HAPTime initialTime = HAPPlatformClockGetCurrent();
    HAPError err = HAPAccessoryServerStartHeartBeat(server, HandleHeartBeatChange, (void*) &context);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    HAPAccessoryServerStart(server, &accessory);

    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);

    // Create a fake security session
    HAPSession session = { .server = server };
    session.hap.active = true;
    session.hap.pairingID = 0;

    uint32_t heartBeat = ReadHeartBeat(server, &session);
    TEST_ASSERT_EQUAL(heartBeat, 1u);

    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 299);
    TEST_ASSERT_NE(lastCallbackTime, (HAPTime) 0);
    HAPTime interval = lastCallbackTime - initialTime;
    HAPLogDebug(&kHAPLog_Default, "First interval %lu mins", (unsigned long) (interval / HAPMinute));
    TEST_ASSERT(interval >= HAPMinute * 240 && interval <= HAPMinute * 300);
    heartBeat = ReadHeartBeat(server, &session);
    TEST_ASSERT_EQUAL(heartBeat, 2u);

    // Advance 300 ms from the lastCallbackTime
    initialTime = lastCallbackTime;
    HAPPlatformTimerEmulateClockAdvances(interval);
    interval = lastCallbackTime - initialTime;
    HAPLogDebug(&kHAPLog_Default, "Second interval %lu mins", (unsigned long) (interval / HAPMinute));
    TEST_ASSERT(interval >= HAPMinute * 240 && interval <= HAPMinute * 300);
    heartBeat = ReadHeartBeat(server, &session);
    TEST_ASSERT_EQUAL(heartBeat, 3u);

    for (size_t i = 0; i < 16; i++) {
        initialTime = lastCallbackTime;
        HAPPlatformTimerEmulateClockAdvances(interval);
        interval = lastCallbackTime - initialTime;
        HAPLogDebug(&kHAPLog_Default, "%zu'th interval %lu mins", (i + 3), (unsigned long) (interval / HAPMinute));
        TEST_ASSERT(interval >= HAPMinute * 240 && interval <= HAPMinute * 300);
        heartBeat = ReadHeartBeat(server, &session);
        TEST_ASSERT_EQUAL(heartBeat, 4 + i);
    }

    // Restart heart beat 2 minutes from now
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 2);
    initialTime = HAPPlatformClockGetCurrent();
    err = HAPAccessoryServerStartHeartBeat(server, HandleHeartBeatChange, (void*) &context);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 1);
    heartBeat = ReadHeartBeat(server, &session);
    TEST_ASSERT_EQUAL(heartBeat, 1u);
    HAPPlatformTimerEmulateClockAdvances(HAPMinute * 299);
    interval = lastCallbackTime - initialTime;
    HAPLogDebug(&kHAPLog_Default, "First interval %lu mins", (unsigned long) (interval / HAPMinute));
    TEST_ASSERT(interval >= HAPMinute * 240 && interval <= HAPMinute * 300);
    heartBeat = ReadHeartBeat(server, &session);
    TEST_ASSERT_EQUAL(heartBeat, 2u);
}

int main(int argc, char** argv) {
    HAPPlatformCreate();
    HAPPlatformSetupDrivers();

    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    return EXECUTE_TESTS(argc, (const char**) argv);
}
