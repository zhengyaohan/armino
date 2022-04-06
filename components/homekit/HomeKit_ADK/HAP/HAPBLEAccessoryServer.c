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

#include "HAPBLEAccessoryServer.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBLEAccessoryServer+Advertising.h"
#include "HAPBLEAccessoryServer+Broadcast.h"
#include "HAPBLEPeripheralManager.h"
#include "HAPBLEProcedure.h"
#include "HAPBLESession.h"
#include "HAPCharacteristic.h"
#include "HAPLogSubsystem.h"
#include "HAPMACAddress.h"
#include "HAPPairingBLESessionCache.h"
#include "HAPPlatformFeatures.h"
#include "HAPSession.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/** Delay between transport stop retry attempt */
#define kHAPBLEAccessoryServer_StopRetryTime ((HAPTime)(500 * HAPMillisecond))

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "BLEAccessoryServer" };

static void Create(HAPAccessoryServer* server, const HAPAccessoryServerOptions* options) {
    HAPPrecondition(server);
    HAPPrecondition(options);

    HAPPrecondition(server->platform.ble.blePeripheralManager);

    // Initialize BLE storage.
    HAPPrecondition(options->ble.accessoryServerStorage);
    HAPBLEAccessoryServerStorage* storage = options->ble.accessoryServerStorage;
    HAPPrecondition(storage->gattTableElements);
    HAPPrecondition(storage->sessionCacheElements);
    HAPPrecondition(storage->numSessionCacheElements >= kHAPBLESessionCache_MinElements);
    HAPPrecondition(storage->session);
    HAPPrecondition(storage->procedures);
    HAPPrecondition(storage->numProcedures >= 1);
    HAPPrecondition(storage->procedureBuffer.bytes);
    HAPPrecondition(storage->procedureBuffer.numBytes >= 1);
    HAPRawBufferZero(storage->gattTableElements, storage->numGATTTableElements * sizeof *storage->gattTableElements);
    HAPRawBufferZero(
            storage->sessionCacheElements, storage->numSessionCacheElements * sizeof *storage->sessionCacheElements);
    HAPRawBufferZero(storage->session, sizeof *storage->session);
    HAPRawBufferZero(storage->procedures, storage->numProcedures * sizeof *storage->procedures);
    HAPRawBufferZero(storage->procedureBuffer.bytes, storage->procedureBuffer.numBytes);
    server->ble.storage = storage;

    // Copy advertising configuration.
    HAPPrecondition(options->ble.preferredAdvertisingInterval >= kHAPBLEAdvertisingInterval_Minimum);
    HAPPrecondition(options->ble.preferredAdvertisingInterval <= kHAPBLEAdvertisingInterval_Maximum);
    HAPPrecondition(options->ble.preferredNotificationDuration >= kHAPBLENotification_MinDuration);
    server->ble.adv.interval = options->ble.preferredAdvertisingInterval;
    server->ble.adv.ev_duration = HAPBLEAdvertisingIntervalGetMilliseconds(options->ble.preferredNotificationDuration);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    // Copy Access Code service parameters
    if (options->accessCode.responseStorage) {
        HAPAssert(options->accessCode.handleOperation);
        HAPRawBufferCopyBytes(
                &server->accessCode.responseStorage,
                HAPNonnull(options->accessCode.responseStorage),
                sizeof server->accessCode.responseStorage);

        server->accessCode.handleOperation = options->accessCode.handleOperation;
        server->accessCode.operationCtx = options->accessCode.operationCtx;
    }
    server->accessCode.numResponseBytes = 0;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    // Copy NFC Access service parameters
    if (options->nfcAccess.responseStorage) {
        HAPRawBufferCopyBytes(
                &server->nfcAccess.responseStorage,
                HAPNonnull(options->nfcAccess.responseStorage),
                sizeof server->nfcAccess.responseStorage);
    }
    server->nfcAccess.numResponseBytes = 0;
#endif
}

static void ValidateAccessory(const HAPAccessory* accessory) {
    HAPPrecondition(accessory);

    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* service = accessory->services[i];
            HAPPrecondition(service->iid <= UINT16_MAX);
            if (service->characteristics) {
                for (size_t j = 0; service->characteristics[j]; j++) {
                    const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                    HAPPrecondition(characteristic->iid <= UINT16_MAX);
                }
            }
        }
    }
}

static void PrepareStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPBLEAccessoryServerStorage* storage = HAPNonnull(server->ble.storage);
    if (HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(server->platform.ble.blePeripheralManager)) {
        // If the platform does not allow refreshing services, previous GATT table must be retained.
        HAPRawBufferZero(
                storage->gattTableElements, storage->numGATTTableElements * sizeof *storage->gattTableElements);
    }
    HAPRawBufferZero(
            storage->sessionCacheElements, storage->numSessionCacheElements * sizeof *storage->sessionCacheElements);
    HAPRawBufferZero(storage->session, sizeof *storage->session);
    HAPRawBufferZero(storage->procedures, storage->numProcedures * sizeof *storage->procedures);
    HAPRawBufferZero(storage->procedureBuffer.bytes, storage->procedureBuffer.numBytes);
}

static void Start(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    HAPAssert(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManager* blePeripheralManager = server->platform.ble.blePeripheralManager;

    // Initialize the platform
    HAPPlatformBLEInitialize(blePeripheralManager);

    // Set BD_ADDR.
    HAPMACAddress deviceAddress;
    err = HAPMACAddressGetRandomStaticBLEDeviceAddress(
            server,
            /* bleInterface: */ NULL,
            &deviceAddress);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    HAPAssert(sizeof deviceAddress == sizeof(HAPPlatformBLEPeripheralManagerDeviceAddress));
    HAPLogBufferInfo(&logObject, deviceAddress.bytes, sizeof deviceAddress.bytes, "BD_ADDR");
    HAPPlatformBLEPeripheralManagerDeviceAddress bdAddr;
    HAPRawBufferZero(&bdAddr, sizeof bdAddr);
    bdAddr.bytes[0] = deviceAddress.bytes[5];
    bdAddr.bytes[1] = deviceAddress.bytes[4];
    bdAddr.bytes[2] = deviceAddress.bytes[3];
    bdAddr.bytes[3] = deviceAddress.bytes[2];
    bdAddr.bytes[4] = deviceAddress.bytes[1];
    bdAddr.bytes[5] = deviceAddress.bytes[0];
    HAPPlatformBLEPeripheralManagerSetDeviceAddress(blePeripheralManager, &bdAddr);

    // Set GAP device name.
    const HAPAccessory* primaryAccessory = HAPNonnull(server->primaryAccessory);
    HAPAssert(primaryAccessory->name);
    HAPAssert(HAPStringGetNumBytes(primaryAccessory->name) <= 64);
    HAPPlatformBLEPeripheralManagerSetDeviceName(blePeripheralManager, primaryAccessory->name);

    // Register GATT db.
    HAPBLEPeripheralManagerRegister(server);

    server->ble.isTransportRunning = true;
    server->ble.isTransportStopping = false;
    server->ble.shouldStopTransportWhenDisconnected = false;
    server->ble.shouldStartTransport = false;
    server->ble.adv.connectedState.didIncrementGSN = false;

    // Clear queued broadcast events
    HAPRawBufferZero(&server->ble.adv.queuedBroadcastEvents, sizeof server->ble.adv.queuedBroadcastEvents);
}

/**
 * Handles reattempts to stop BLE
 *
 * @param timer    timer
 * @param context  accessory server
 */
static void HandleStopRetryTimer(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    if (!server->transports.ble) {
        // Transport is no longer valid.
        return;
    }

    if (!server->ble.isTransportStopping) {
        HAPLogDebug(&logObject, "%s: Timer ignored as already stopped", __func__);
        return;
    }

    bool didStop;
    HAPNonnull(server->transports.ble)->tryStop(server, &didStop);
}

static void TryStop(HAPAccessoryServer* server, bool* didStop) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;
    HAPPrecondition(didStop);

    *didStop = false;
    server->ble.isTransportStopping = true;
    server->ble.shouldStopTransportWhenDisconnected = false;

    // Close all connections.
    if (server->ble.connection.connected) {
        HAPSession* session = server->ble.storage->session;
        if (HAPBLESessionIsSafeToDisconnect(&session->_.ble)) {
            HAPLogInfo(&logObject, "Disconnecting BLE connection - Server is shutting down.");
            HAPPlatformBLEPeripheralManagerCancelCentralConnection(
                    blePeripheralManager, server->ble.connection.connectionHandle);
        } else {
            HAPLogInfo(&logObject, "Waiting for pending BLE data to be written.");
        }
        HAPLogInfo(&logObject, "Delaying shutdown. Waiting for BLE connection to terminate.");
        HAPPlatformTimerRef timer;
        HAPError err = HAPPlatformTimerRegister(
                &timer,
                HAPPlatformClockGetCurrent() + kHAPBLEAccessoryServer_StopRetryTime,
                HandleStopRetryTimer,
                server);
        HAPAssert(!err);
        return;
    }

    // Stop listening.
    HAPPlatformBLEPeripheralManagerRemoveAllServices(blePeripheralManager);
    HAPPlatformBLEPeripheralManagerSetDelegate(blePeripheralManager, NULL);

    // Deinitialize the platform
    HAPPlatformBLEDeinitialize();

    *didStop = true;

    server->ble.isTransportRunning = false;
    server->ble.isTransportStopping = false;

    if (server->state == kHAPAccessoryServerState_Running) {
        if (server->ble.shouldStartTransport) {
            // Transport has to be restarted
            HAPLogInfo(&logObject, "BLE transport restart was pending");
            HAPAccessoryServerStartBLETransport(server);
        } else {
            HAPLogInfo(&logObject, "BLE transport stopped.");
        }
    }
}

static void UpdateAdvertisingData(HAPAccessoryServer* server, bool fastAdvertising) {
    HAPPrecondition(server);

    if (!server->ble.isTransportRunning) {
        return;
    }
    HAPError err;

    HAPAssert(server->platform.ble.blePeripheralManager);
    HAPPlatformBLEPeripheralManagerRef blePeripheralManager = server->platform.ble.blePeripheralManager;

    if (server->state == kHAPAccessoryServerState_Running && !server->ble.isTransportStopping) {
        // Fetch advertisement parameters.
        bool isActive;
        uint16_t advertisingInterval;
        uint8_t advertisingBytes[/* Maximum Bluetooth 4 limit: */ 31];
        size_t numAdvertisingBytes;
        uint8_t scanResponseBytes[/* Maximum Bluetooth 4 limit: */ 31];
        size_t numScanResponseBytes;
        err = HAPBLEAccessoryServerGetAdvertisingParameters(
                server,
                &isActive,
                &advertisingInterval,
                advertisingBytes,
                sizeof advertisingBytes,
                &numAdvertisingBytes,
                scanResponseBytes,
                sizeof scanResponseBytes,
                &numScanResponseBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Update advertisement.
        if (isActive) {
            HAPAssert(advertisingInterval);

            if (fastAdvertising) {
                advertisingInterval = HAPBLEAdvertisingIntervalCreateFromMilliseconds(HAP_BLE_MINIMUM_ADVERTISING_RATE);
            }

            HAPPlatformBLEPeripheralManagerStartAdvertising(
                    blePeripheralManager,
                    advertisingInterval,
                    advertisingBytes,
                    numAdvertisingBytes,
                    numScanResponseBytes ? scanResponseBytes : NULL,
                    numScanResponseBytes);

            if (fastAdvertising) {
                HAPLogInfo(
                        &logObject,
                        "Increasing advertising interval to %d ms for %d ms",
                        (int) HAPBLEAdvertisingIntervalGetMilliseconds(advertisingInterval),
                        server->ble.adv.ev_duration);
                server->ble.adv.timerExpiryClock =
                        HAPPlatformClockGetCurrent() + HAPMillisecond * server->ble.adv.ev_duration;
                err = HAPPlatformTimerRegister(
                        &server->ble.adv.timer,
                        server->ble.adv.timerExpiryClock,
                        HAPBLEAccessoryAdvertisingTimerExpired,
                        server);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    HAPLog(&logObject, "Not enough resource to start advertising timeout timer");
                }
            } else {
                // Mark advertisement started.
                HAPBLEAccessoryServerDidStartAdvertising(server);
            }
        } else {
            HAPPlatformBLEPeripheralManagerStopAdvertising(blePeripheralManager);
        }
    } else {
        HAPLogInfo(&logObject, "Stopping advertisement - Server is shutting down.");
        HAPPlatformBLEPeripheralManagerStopAdvertising(blePeripheralManager);
    }
}

void HAPBLEAccessoryServerStartFastAdvertising(HAPAccessoryServer* server) {
    UpdateAdvertisingData(server, true);
}

const HAPBLEAccessoryServerTransport kHAPAccessoryServerTransport_BLE = {
    .create = Create,
    .validateAccessory = ValidateAccessory,
    .prepareStart = PrepareStart,
    .start = Start,
    .tryStop = TryStop,
    .didRaiseEvent = HAPBLEAccessoryServerDidRaiseEvent,
    .updateAdvertisingData = UpdateAdvertisingData,
    .getGSN = HAPBLEAccessoryServerGetGSN,
    .broadcast = { .expireKey = HAPBLEAccessoryServerBroadcastExpireKey },
    .peripheralManager = { .release = HAPBLEPeripheralManagerRelease,
                           .handleSessionAccept = HAPBLEPeripheralManagerHandleSessionAccept,
                           .handleSessionInvalidate = HAPBLEPeripheralManagerHandleSessionInvalidate },
    .sessionCache = { .fetch = HAPPairingBLESessionCacheFetch,
                      .save = HAPPairingBLESessionCacheSave,
                      .invalidateEntriesForPairing = HAPPairingBLESessionCacheInvalidateEntriesForPairing },
    .session = { .create = HAPBLESessionCreate,
                 .release = HAPBLESessionRelease,
                 .invalidate = HAPBLESessionInvalidate,
                 .didStartPairingProcedure = HAPBLESessionDidStartPairingProcedure,
                 .didCompletePairingProcedure = HAPBLESessionDidCompletePairingProcedure,
                 .didPairSetupProcedure = HAPBLESessionDidPairSetupProcedure }
};

#endif
