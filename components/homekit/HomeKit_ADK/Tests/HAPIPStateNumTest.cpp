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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPCharacteristic.h"
#include "HAPIPGlobalStateNumber.h"
#include "HAPLog+Attributes.h"
#include "HAPPlatform+Init.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPPlatformTimerHelper.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

// Use TEST_ASSERT for mock assertions
#define MOCK_ASSERT TEST_ASSERT

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

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
        &accessoryInformationService, &hapProtocolInformationService, &pairingService, &lightBulbService, NULL,
    };
    accessory.services = services;
    accessory.callbacks.identify = IdentifyAccessoryHelper;
    return accessory;
}

static const HAPAccessory accessory = InitAccessory();

HAPAccessoryServer* server = NULL;

/**
 * Test: StateNumInitialValue
 * Description: Checks initial S# value
 * Expected: S# should be >= 1
 */
TEST(StateNumInitialValue) {
    HAPError err;
    HAPIPGlobalStateNumber ipGSN;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(ipGSN >= 1);
}

/**
 * Test: NotificationNoPairingNoActiveConnection
 * Description:
 *       1. No pairing exists
 *       2. No active TCP connection
 *       3. Trigger event notification
 * Expected: S# should not be incremented
 */
TEST(NotificationNoPairingNoActiveConnection) {
    HAPError err;
    HAPIPGlobalStateNumber ipGSN;
    HAPIPGlobalStateNumber ipGSN2;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformClockAdvance(HAPSecond);

    err = HAPIPGlobalStateNumberGet(server, &ipGSN2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(ipGSN, ipGSN2);
}

/**
 * Test: FirstNotificationWithPairingNoActiveConnection
 * Description:
 *       1. Pairing exists
 *       2. No active TCP connection
 *       3. Trigger first event notification
 * Expected: S# should be incremented by 1
 */
TEST(FirstNotificationWithPairingNoActiveConnection) {
    HAPError err;
    HAPIPGlobalStateNumber ipGSN;
    HAPIPGlobalStateNumber ipGSN2;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Prepare key-value store with controller pairing.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    {
        // Import controller pairing.
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

    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformClockAdvance(HAPSecond);

    err = HAPIPGlobalStateNumberGet(server, &ipGSN2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(ipGSN + 1, ipGSN2);
}

/**
 * Test: SecondNotificationWithPairingNoActiveConnection
 * Description:
 *       1. Pairing exists
 *       2. No active TCP connection
 *       3. Trigger second event notification
 * Expected: S# should not be incremented
 */
TEST(SecondNotificationWithPairingNoActiveConnection) {
    HAPError err;
    HAPIPGlobalStateNumber ipGSN;
    HAPIPGlobalStateNumber ipGSN2;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Prepare key-value store with controller pairing.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    {
        // Import controller pairing.
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

    HAPAccessoryServerRaiseEvent(server, &lightBulbBrightnessCharacteristic, &lightBulbService, &accessory);
    HAPPlatformClockAdvance(HAPSecond);

    err = HAPIPGlobalStateNumberGet(server, &ipGSN2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(ipGSN, ipGSN2);
}

/**
 * Test: NotificationWithPairingAndActiveConnection
 * Description:
 *       1. Pairing exists
 *       2. Active TCP connection exists
 *       3. Trigger IP service discovery
 * Expected: S# should not be incremented
 */
TEST(NotificationWithPairingAndActiveConnection) {
    HAPError err;
    HAPIPGlobalStateNumber ipGSN;
    HAPIPGlobalStateNumber ipGSN2;

    err = HAPIPGlobalStateNumberGet(server, &ipGSN);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Prepare key-value store with controller pairing.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    {
        // Import controller pairing.
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

    server->ip.storage->sessions[0].descriptor.tcpStreamIsOpen = true;
    HAPIPServiceDiscoverySetHAPService(server);
    HAPPlatformClockAdvance(HAPSecond);

    err = HAPIPGlobalStateNumberGet(server, &ipGSN2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT_EQUAL(ipGSN, ipGSN2);
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
    // Clears all timers.
    HAPPlatformTimerDeregisterAll();
}
#endif

int main(int argc, char** argv) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    HAPPlatformCreate();

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultInboundBufferSize];
    uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_DefaultOutboundBufferSize];
    HAPIPEventNotification ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }

    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    HAPIPAccessoryServerStorage ipAccessoryServerStorage = { .sessions = ipSessions,
                                                             .numSessions = HAPArrayCount(ipSessions),
                                                             .readContexts = ipReadContexts,
                                                             .numReadContexts = HAPArrayCount(ipReadContexts),
                                                             .writeContexts = ipWriteContexts,
                                                             .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                                             .scratchBuffer = { .bytes = ipScratchBuffer,
                                                                                .numBytes = sizeof ipScratchBuffer } };

    static HAPAccessoryServer accessoryServer;

    // Set server options
    HAPAccessoryServerOptions serverOptions;
    HAPRawBufferZero(&serverOptions, sizeof serverOptions);

    serverOptions.maxPairings = kHAPPairingStorage_MinElements;
    serverOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    serverOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    static const HAPAccessoryServerCallbacks serverCallbacks = {
        .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState,
    };

    HAPAccessoryServerCreate(&accessoryServer, &serverOptions, &platform, &serverCallbacks, NULL);

    // Start accessory server.
    HAPLog(&kHAPLog_Default, "Starting accessory server.");
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&accessoryServer), kHAPAccessoryServerState_Running);

    TEST_ASSERT(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    server = &accessoryServer;
    TEST_ASSERT(server);
    return EXECUTE_TESTS(argc, (const char**) argv);
#else
    HAPLog(&kHAPLog_Default, "This test is not supported.");
    return 0;
#endif
}
