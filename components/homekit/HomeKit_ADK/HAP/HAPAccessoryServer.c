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

#include "HAP+KeyValueStoreDomains.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetupInfo.h"
#include "HAPAccessoryValidation.h"
#include "HAPBLEAccessoryServer.h"
#include "HAPCharacteristic.h"
#include "HAPCheckedPlatformWiFiRouter.h"
#include "HAPDataStream.h"
#include "HAPIPAccessoryServer.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPNotification+Delivery.h"
#include "HAPServiceTypes.h"
#include "HAPStringBuilder.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#include "HAPPlatformDiagnostics.h"
#endif

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "AccessoryServer" };

#define kHeartBeatUpdateIntervalRandomRangeBegin (HAPMinute * 240) /**< Heart beat update minimum random interval */
#define kHeartBeatUpdateIntervalRandomRange      (HAPMinute * 60)  /**< Range of heart beat update random interval */

#ifndef kUnpairedStateTimerDuration
#define kUnpairedStateTimerDuration (HAPMinute * 10) /**< Maximum duration for which accessory can stay unpaired */
#endif

// forward reference
static void HandleHeartBeatUpdateTimer(HAPPlatformTimerRef timer, void* _Nullable data);
static void _HAPAccessoryServerStop(void* _Nullable context_, size_t contextSize);
static void HAPAccessoryServerPrintAccessoryConfiguration(
        const HAPPlatform* platform,
        const HAPAccessory* primaryAccessory);

/**
 * Completes accessory server shutdown after HAPAccessoryServerStop.
 *
 * @param      server              Accessory server.
 */
static void CompleteShutdown(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Reset Pair Setup procedure state.
    HAPAssert(!server->pairSetup.sessionThatIsCurrentlyPairing);
    HAPAccessorySetupInfoHandleAccessoryServerStop(server);

    // Reset state.
    server->primaryAccessory = NULL;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    server->ip.bridgedAccessories = NULL;

    // Check that everything is cleaned up.
    if (server->transports.ip) {
        HAPLogInfo(&logObject, "Accessory server shutdown with IP");
        HAPAssert(!server->ip.discoverableService);
        HAPAssert(!server->ip.wac.wiFiConfiguration.isSet);
        HAPAssert(!server->ip.wac.softwareAccessPointIsActive);
    }
#endif

    // Shutdown complete.
    HAPLogInfo(&logObject, "Accessory server shutdown completed.");
    server->state = kHAPAccessoryServerState_Idle;
    HAPAssert(server->callbacks.handleUpdatedState);
    server->callbacks.handleUpdatedState(server, server->context);
}

static void CallbackTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->callbackTimer);
    server->callbackTimer = 0;

    HAPAccessorySetupInfoHandleAccessoryServerStateUpdate(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    // Complete shutdown if accessory server has been stopped using a server engine.
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->stop && HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle) {
            CompleteShutdown(server);
            return;
        }
    }
#endif

    // Invoke handleUpdatedState callback.
    HAPAssert(server->callbacks.handleUpdatedState);
    server->callbacks.handleUpdatedState(server, server->context);
}

void HAPAccessoryServerDelegateScheduleHandleUpdatedState(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    if (server->callbackTimer) {
        return;
    }
    err = HAPPlatformTimerRegister(&server->callbackTimer, 0, CallbackTimerExpired, server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to allocate accessory server callback timer.");
        HAPFatalError();
    }
}

void HAPAccessoryServerCreate(
        HAPAccessoryServer* server,
        const HAPAccessoryServerOptions* options,
        const HAPPlatform* platform,
        const HAPAccessoryServerCallbacks* callbacks,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(options);
    HAPPrecondition(platform);
    HAPPrecondition(callbacks);

    HAPLogDebug(&logObject, "Storage configuration: server = %lu", (unsigned long) sizeof *server);

    HAPRawBufferZero(server, sizeof *server);

    // Copy generic options.
    HAPPrecondition(options->maxPairings >= kHAPPairingStorage_MinElements);
    server->maxPairings = options->maxPairings;
    server->sessionKeyExpiry = options->sessionKeyExpiry;

    // Copy platform.
    HAPAssert(sizeof *platform == sizeof server->platform);
    HAPRawBufferCopyBytes(&server->platform, platform, sizeof server->platform);
    HAPPrecondition(server->platform.keyValueStore);
    HAPPrecondition(server->platform.accessorySetup);
    HAPMFiHWAuthCreate(&server->mfi, server->platform.authentication.mfiHWAuth);

    // Copy callbacks.
    HAPPrecondition(callbacks->handleUpdatedState);
    HAPAssert(sizeof *callbacks == sizeof server->callbacks);
    HAPRawBufferCopyBytes(&server->callbacks, callbacks, sizeof server->callbacks);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Initialize session storage.
    if (options->sessionStorage.bytes) {
        HAPThreadSessionStorageCreate(
                server, HAPNonnullVoid(options->sessionStorage.bytes), options->sessionStorage.numBytes);
    }
#endif

    // One transport must be supported.
    int numTransports = 0;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (options->ip.transport) {
        numTransports++;
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (options->ble.transport) {
        numTransports++;
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (options->thread.transport) {
        numTransports++;
    }
#endif
    HAPPrecondition(numTransports > 0);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    // Copy IP parameters.
    server->transports.ip = options->ip.transport;
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->create(server, options);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Copy Bluetooth LE parameters.
    server->transports.ble = options->ble.transport;
    if (server->transports.ble) {
        HAPNonnull(server->transports.ble)->create(server, options);
    } else {
        HAPRawBufferZero(&server->platform.ble, sizeof server->platform.ble);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Copy Thread parameters.
    server->transports.thread = options->thread.transport;

    if (server->transports.thread) {
        HAPAssert(
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_BR ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_REED ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_FED ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_MED ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_SED ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_FTD ||
                options->thread.deviceParameters.deviceType == kHAPPlatformThreadDeviceCapabilities_MTD);
        server->thread.deviceParameters.deviceType = options->thread.deviceParameters.deviceType;
        server->thread.deviceParameters.txPowerdbm = options->thread.deviceParameters.txPowerdbm;
        server->thread.deviceParameters.childTimeout = options->thread.deviceParameters.childTimeout;
        server->thread.suppressUnpairedThreadAdvertising = options->thread.suppressUnpairedThreadAdvertising;
        HAPNonnull(server->transports.thread)->create(server, options);
    } else {
        HAPRawBufferZero(&server->platform.thread, sizeof server->platform.thread);
    }
#endif

    HAPAccessorySetupOwnershipCreate(&server->setupOwnership);

    // Set up Data Stream, if set.
    if (0) {
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    else if (options->ip.dataStream.tcpStreamManager) {
        HAPDataStreamCreateWithTCPManager(
                server,
                options->ip.dataStream.tcpStreamManager,
                options->dataStream.dataStreams,
                options->dataStream.numDataStreams);

        HAPLog(&logObject,
               "Configured HomeKit Data Stream for TCP with %lu concurrent streams.",
               (unsigned long) server->dataStream.numDataStreams);
    }
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    else if (options->dataStream.dataStreams) {
        HAPDataStreamCreateWithHAP(server, options->dataStream.dataStreams, options->dataStream.numDataStreams);

        HAPLog(&logObject,
               "Configured HomeKit Data Stream for HAP with %lu concurrent streams.",
               (unsigned long) server->dataStream.numDataStreams);
    }
#endif

#if (HAP_TESTING == 1)
    server->firmwareUpdate.persistStaging = options->firmwareUpdate.persistStaging;
#endif

    // Copy client context.
    server->context = context;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->init) {
            serverEngine->init(server);
        }
    }
#endif
}

void HAPAccessoryServerRelease(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPAccessoryServerForceStop(server);

    HAPDataStreamStop(server);

    if (server->callbackTimer) {
        HAPPlatformTimerDeregister(server->callbackTimer);
        server->callbackTimer = 0;
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        HAPAssert(server->platform.ble.blePeripheralManager);
        HAPNonnull(server->transports.ble)->peripheralManager.release(server);
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->deinit) {
            HAPError err = serverEngine->deinit(server);
            if (err) {
                HAPFatalError();
            }
        }
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        if (server->ble.adv.fast_timer) {
            HAPPlatformTimerDeregister(server->ble.adv.fast_timer);
            server->ble.adv.fast_timer = 0;
        }
        if (server->ble.adv.timer) {
            HAPPlatformTimerDeregister(server->ble.adv.timer);
            server->ble.adv.timer = 0;
            server->ble.adv.timerExpiryClock = 0;
        }
    }
#endif

    HAPMFiHWAuthRelease(&server->mfi);

    HAPAccessorySetupOwnershipRelease(&server->setupOwnership);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->serverEngine.uninstall(server);
    }
#endif

    HAPRawBufferZero(server, sizeof *server);
}

HAP_RESULT_USE_CHECK
HAPAccessoryServerState HAPAccessoryServerGetState(HAPAccessoryServer* server) {
    HAPPrecondition(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->get_state) {
            return serverEngine->get_state(server);
        }
    }
#endif

    return server->state;
}

HAP_RESULT_USE_CHECK
void* _Nullable HAPAccessoryServerGetClientContext(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    return server->context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HAPAccessoryServerSetOwnershipProofTokenRequired(HAPAccessoryServer* server, bool tokenRequired) {
    HAPPrecondition(server);

    HAPAccessorySetupOwnershipSetTokenRequired(&server->setupOwnership, tokenRequired);
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerGenerateOwnershipProofToken(
        HAPAccessoryServer* server,
        HAPAccessorySetupOwnershipProofToken* ownershipToken) {
    HAPPrecondition(server);
    HAPPrecondition(ownershipToken);

    if (HAPAccessoryServerIsPaired(server)) {
        HAPLogError(&logObject, "Cannot generate ownership proof token: Accessory server is already paired.");
        return kHAPError_InvalidState;
    }

    return HAPAccessorySetupOwnershipGenerateToken(&server->setupOwnership, ownershipToken);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Parses a version string where each element is capped at 2^32-1.
 *
 * @param      version              Version string.
 * @param[out] major                Major version number.
 * @param[out] minor                Minor version number.
 * @param[out] revision             Revision version number.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the version string is malformed.
 */
HAP_RESULT_USE_CHECK
static HAPError ParseVersionString(const char* version, uint32_t* major, uint32_t* minor, uint32_t* revision) {
    HAPPrecondition(version);
    HAPPrecondition(major);
    HAPPrecondition(minor);
    HAPPrecondition(revision);

    *major = 0;
    *minor = 0;
    *revision = 0;

    // Read numbers.
    uint32_t* numbers[3] = { major, minor, revision };
    size_t i = 0;
    bool first = true;
    for (const char* c = version; *c; c++) {
        if (!first && *c == '.') {
            // Advance to next number.
            if (i >= 2) {
                HAPLog(&logObject, "Invalid version string: %s.", version);
                return kHAPError_InvalidData;
            }
            i++;
            first = true;
            continue;
        }
        first = false;

        // Add digit.
        if (!HAPASCIICharacterIsNumber(*c)) {
            HAPLog(&logObject, "Invalid version string: %s.", version);
            return kHAPError_InvalidData;
        }
        if (*numbers[i] > UINT32_MAX / 10) {
            HAPLog(&logObject, "Invalid version string: %s.", version);
            return kHAPError_InvalidData;
        }
        (*numbers[i]) *= 10;
        if (*numbers[i] > UINT32_MAX - (uint32_t)(*c - '0')) {
            HAPLog(&logObject, "Invalid version string: %s.", version);
            return kHAPError_InvalidData;
        }
        (*numbers[i]) += (uint32_t)(*c - '0');
    }
    if (first) {
        HAPLog(&logObject, "Invalid version string: %s.", version);
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

void HAPAccessoryServerLoadLTSK(HAPAccessoryServer* server, HAPAccessoryServerLongTermSecretKey* ltsk) {
    HAPPrecondition(server);
    HAPPrecondition(ltsk);

    HAPError err;

    // An attacker who gains application processor code execution privileges can:
    // - Control any accessory functionality.
    // - List, add, remove, and modify HAP pairings.
    // - Provide a service to sign arbitrary messages with the accessory LTSK.
    // These assumptions remain valid even when a separate Trusted Execution Environment (TEE) is present,
    // because as of HomeKit Accessory Protocol Specification R17, HAP only defines transport security.
    // Augmenting the HAP protocol with true end-to-end security for HAP pairings would require a protocol change.
    //
    // The raw accessory LTSK could theoretically be stored in a TEE,
    // but given the user impact when an attacker takes control of the application processor
    // there does not seem to be a realistic threat that can be mitigated if this would be done.
    // The attacker could still set up a service to sign arbitrary messages with the accessory LTSK
    // when the accessory LTSK is stored in a TEE, and could use this service to impersonate the accessory.
    //
    // The only security that can currently be provided is to store all secrets in secure memory
    // so that they cannot easily be extracted at rest (without having code execution privileges or RAM access).
    // It is left up to the platform implementation to store the HAPPlatformKeyValueStore content securely.
    //
    // Note: If this mechanism is ever replaced to redirect to a TEE for the LTSK,
    // an upgrade path must be specified for the following scenarios:
    // - LTSK was stored in HAPPlatformKeyValueStore, and needs to be migrated into a TEE.
    // - HAP protocol gets extended with real TEE support, and LTSK needs to be migrated into a new TEE.

    HAPPlatformKeyValueStoreRef keyValueStore = server->platform.keyValueStore;
    bool found;
    size_t numBytes;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_LTSK,
            ltsk->bytes,
            sizeof ltsk->bytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Reading LTSK failed.");
        HAPFatalError();
    }
    if (!found) {
        // Reset pairings.
        err = HAPPairingRemoveAll(server);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Purge of pairing domain failed.");
            HAPFatalError();
        }

        // Generate new LTSK.
        HAPPlatformRandomNumberFill(ltsk->bytes, sizeof ltsk->bytes);
        HAPLogSensitiveBufferInfo(&logObject, ltsk->bytes, sizeof ltsk->bytes, "Generated new LTSK.");

        // Store new LTSK.
        err = HAPPlatformKeyValueStoreSet(
                keyValueStore,
                kHAPKeyValueStoreDomain_Configuration,
                kHAPKeyValueStoreKey_Configuration_LTSK,
                ltsk->bytes,
                sizeof ltsk->bytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Storing LTSK failed.");
            HAPFatalError();
        }
    } else {
        if (numBytes != sizeof ltsk->bytes) {
            HAPLogError(&logObject, "Invalid LTSK length in key-value store: %zu.", numBytes);
            HAPFatalError();
        }
    }
}

/**
 * Prepares starting the accessory server.
 *
 * @param      server               Accessory server.
 * @param      primaryAccessory     Primary accessory to host.
 * @param      bridgedAccessories   NULL-terminated array of bridged accessories for a bridge accessory. NULL otherwise.
 */
static void HAPAccessoryServerPrepareStart(
        HAPAccessoryServer* server,
        const HAPAccessory* primaryAccessory,
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.keyValueStore);
    HAPPrecondition(server->state == kHAPAccessoryServerState_Idle);
    HAPPrecondition(!server->primaryAccessory);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    HAPPrecondition(!server->ip.bridgedAccessories);
#endif
    HAPPrecondition(primaryAccessory);

    HAPError err;

    // Verify compatibility of key-value store.
    if (!HAPIsKeyValueStoreCompatible(server->platform.keyValueStore)) {
        HAPLogError(&logObject, "Key-value store is not compatible! Not starting accessory sever.");
        return;
    }

    // Start accessory server.
    HAPLogInfo(&logObject, "Accessory server starting.");
    server->state = kHAPAccessoryServerState_Running;
    HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);

    // Reset state.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->prepareStart(server);
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        if (!server->transports.thread)
#endif
            HAPNonnull(server->transports.ble)->prepareStart(server);
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.thread) {
        HAPNonnull(server->transports.thread)->prepareStart(server);
    }
#endif

    // Firmware version check.
    {
        // Read firmware version.
        HAPAssert(primaryAccessory->firmwareVersion);
        uint32_t major;
        uint32_t minor;
        uint32_t revision;
        err = ParseVersionString(primaryAccessory->firmwareVersion, &major, &minor, &revision);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPFatalError();
        }

        // Check for configuration change.
        uint8_t bytes[3 * 4];
        bool found;
        size_t numBytes;
        err = HAPPlatformKeyValueStoreGet(
                server->platform.keyValueStore,
                kHAPKeyValueStoreDomain_Configuration,
                kHAPKeyValueStoreKey_Configuration_FirmwareVersion,
                bytes,
                sizeof bytes,
                &numBytes,
                &found);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        bool saveVersion = false;
        if (found) {
            if (numBytes != sizeof bytes) {
                HAPLogError(
                        &logObject,
                        "Key-value store corrupted - unexpected length for firmware revision: %lu.",
                        (unsigned long) numBytes);
                HAPFatalError();
            }
            uint32_t previousMajor = HAPReadLittleUInt32(&bytes[0]);
            uint32_t previousMinor = HAPReadLittleUInt32(&bytes[4]);
            uint32_t previousRevision = HAPReadLittleUInt32(&bytes[8]);
            if (major != previousMajor || minor != previousMinor || revision != previousRevision) {
                if (major < previousMajor || (major == previousMajor && minor < previousMinor) ||
                    (major == previousMajor && minor == previousMinor && revision < previousRevision)) {
                    // Accessories must not allow a firmware image to be downgraded after a successful firmware update.
                    // See HomeKit Accessory Protocol Specification R17
                    // Section 3.4 Firmware Updates
                    //
                    // Additionally, when the storage format in the HAPPlatformKeyValueStore is changed,
                    // we only have upgrade paths; we don't have logic for downgrade from future data formats in place.
                    //
                    // If it is absolutely necessary to downgrade HomeKit firmware the following methods exist:
                    // 1. Only start the accessory server once the new firmware is known to be good.
                    //    In this case, automated data migrations are not performed yet, so downgrade is possible.
                    // 2. HAPKeystoreRestoreFactorySettings can be called before starting the accessory server.
                    //    In this case, firmware downgrade is allowed. However, all HomeKit pairings are reset.
                    // 3. The HAPPlatformKeyValueStore could be restored to the exact version that was used by the
                    //    previous firmware version. Potential data already saved by the new firmware is lost, though.
                    //
                    // The HAPIsKeyValueStoreCompatible function may be used to determine whether a downgraded firmware
                    // is no longer compatible with the provided key-value store.
                    HAPLogError(
                            &logObject,
                            "[%lu.%lu.%lu > %lu.%lu.%lu] Accessory must not allow a firmware image to be downgraded!",
                            (unsigned long) previousMajor,
                            (unsigned long) previousMinor,
                            (unsigned long) previousRevision,
                            (unsigned long) major,
                            (unsigned long) minor,
                            (unsigned long) revision);
                }

                HAPLogInfo(
                        &logObject,
                        "[%lu.%lu.%lu > %lu.%lu.%lu] Performing post firmware update tasks.",
                        (unsigned long) previousMajor,
                        (unsigned long) previousMinor,
                        (unsigned long) previousRevision,
                        (unsigned long) major,
                        (unsigned long) minor,
                        (unsigned long) revision);
                err = HAPHandleFirmwareUpdate(server);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPFatalError();
                }
                saveVersion = true;
            }
        } else {
            HAPLogInfo(
                    &logObject,
                    "[%lu.%lu.%lu] Storing initial firmware version.",
                    (unsigned long) major,
                    (unsigned long) minor,
                    (unsigned long) revision);
            saveVersion = true;
        }
        if (saveVersion) {
            HAPWriteLittleUInt32(&bytes[0], major);
            HAPWriteLittleUInt32(&bytes[4], minor);
            HAPWriteLittleUInt32(&bytes[8], revision);
            err = HAPPlatformKeyValueStoreSet(
                    server->platform.keyValueStore,
                    kHAPKeyValueStoreDomain_Configuration,
                    kHAPKeyValueStoreKey_Configuration_FirmwareVersion,
                    bytes,
                    sizeof bytes);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }
        }
    }

    // Register accessory.
    HAPLogDebug(&logObject, "Registering accessories.");
    server->primaryAccessory = primaryAccessory;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    server->ip.bridgedAccessories = bridgedAccessories;

    // Check IP Camera requirements.
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->willStart(server);
    }
#endif

// Set up Wi-Fi router.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
    if (server->platform.ip.wiFiRouter) {
        HAPPlatformWiFiRouterDelegate delegate;
        HAPRawBufferZero(&delegate, sizeof delegate);
        delegate.context = server;
        delegate.handleReadyStateChanged = HAPWiFiRouterHandleReadyStateChanged;
        delegate.handleManagedNetworkStateChanged = HAPWiFiRouterHandleManagedNetworkStateChanged;
        delegate.handleWANConfigurationChanged = HAPWiFiRouterHandleWANConfigurationChanged;
        delegate.handleWANStatusChanged = HAPWiFiRouterHandleWANStatusChanged;
        delegate.handleAccessViolationMetadataChanged = HAPWiFiRouterHandleAccessViolationMetadataChanged;
        delegate.handleSatelliteStatusChanged = HAPWiFiRouterHandleSatelliteStatusChanged;
        HAPCheckedPlatformWiFiRouterSetDelegate(HAPNonnull(server->platform.ip.wiFiRouter), &delegate);
    }
#endif

    // Prepare the Data Stream subsystem (does nothing interesting if not needed)
    HAPDataStreamPrepareStart(server);

    // Load LTSK.
    HAPLogDebug(&logObject, "Loading accessory identity.");
    {
        HAPAccessoryServerLoadLTSK(server, &server->identity.ed_LTSK);
        HAP_ed25519_public_key(server->identity.ed_LTPK, server->identity.ed_LTSK.bytes);
    }

    // Cleanup pairings.
    err = HAPAccessoryServerCleanupPairings(server);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Cleanup pairings failed.");
        HAPFatalError();
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->start(server);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->sessionStorage.bytes) {
        // Initialize session storage
        HAPThreadSessionStorageHandleAccessoryServerWillStart(server);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        if (!server->transports.thread)
#endif
            HAPNonnull(server->transports.ble)->start(server);
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.thread) {
        HAPNonnull(server->transports.thread)->start(server);
    }
#endif

    // Update setup payload.
    HAPAccessorySetupInfoHandleAccessoryServerStart(server);

    // Update advertising state.
    HAPAccessoryServerUpdateAdvertisingData(server);

    // Kick off unpaired state timer if necessary.
    HAPAccessoryServerUpdateUnpairedStateTimer(server);

    // Print accessory configuration
    HAPAccessoryServerPrintAccessoryConfiguration(&server->platform, primaryAccessory);
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
static void HandleStreamStateChanged(HAPPlatformCameraRef camera, size_t streamIndex, void* _Nullable context) {
    HAPPrecondition(camera);
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;

    // Get accessory corresponding to camera.
    const HAPAccessory* accessory = NULL;
    if (camera == server->platform.ip.camera) {
        accessory = server->primaryAccessory;
    } else {
        if (server->ip.bridgedAccessories && server->ip.bridgedCameras) {
            const HAPPlatformCameraRef _Nullable* cameras = server->ip.bridgedCameras;
            for (size_t i = 0; server->ip.bridgedAccessories[i]; i++) {
                if (camera == cameras[i]) {
                    accessory = server->ip.bridgedAccessories[i];
                }
            }
        }
    }
    HAPAssert(accessory);

    // Get streaming service and status characteristic corresponding to streamIndex.
    size_t index = 0;
    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* s = accessory->services[i];
            if (HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_CameraRTPStreamManagement)) {
                if (index == streamIndex) {
                    for (size_t j = 0; s->characteristics[j]; j++) {
                        const HAPCharacteristic* c = s->characteristics[j];
                        if (HAPUUIDAreEqual(
                                    ((const HAPBaseCharacteristic*) c)->characteristicType,
                                    &kHAPCharacteristicType_StreamingStatus)) {
                            HAPLog(&logObject, "Raise streaming status event.");
                            HAPAccessoryServerRaiseEvent(server, c, s, accessory);
                        }
                    }
                }
                index++;
            }
        }
    }
}

static void SetupCameraConfiguration(
        HAPAccessoryServer* server,
        const HAPAccessory* accessory,
        HAPPlatformCameraRef _Nullable camera,
        size_t* _Nullable numCameraServices) {
    // Count Camera RTP Stream Management services.
    size_t numIPCameraStreamServices = 0;

    if (accessory->services) {
        for (size_t i = 0; accessory->services[i]; i++) {
            const HAPService* s = accessory->services[i];

            // Count number of streaming services before the addressed one.
            if (HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_CameraRTPStreamManagement)) {
                numIPCameraStreamServices++;
            }
        }
    }

    // Check IP Camera requirements.
    if (numIPCameraStreamServices) {
        // Check configurations.
        HAPPrecondition(accessory->cameraStreamConfigurations);
        size_t numConfigurations = 0;
        for (numConfigurations = 0; accessory->cameraStreamConfigurations[numConfigurations]; numConfigurations++) {
        }
        HAPPrecondition(numConfigurations == numIPCameraStreamServices);

        // Install streaming events delegate.
        HAPPrecondition(camera);
        HAPPlatformCameraDelegate delegate = { server, HandleStreamStateChanged };
        HAPPlatformCameraSetDelegate(HAPNonnull(camera), &delegate);
    }

    if (numCameraServices) {
        *numCameraServices = numIPCameraStreamServices;
    }
}
#endif

void HAPAccessoryServerStart(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);
    HAPLog(&logObject, "Starting Accessory Server");

    HAPLogDebug(
            &logObject,
            "Checking accessory definition. "
            "If this crashes, verify that service and characteristic lists are properly NULL-terminated.");
    HAPPrecondition(HAPRegularAccessoryIsValid(server, accessory));
    HAPLogDebug(&logObject, "Accessory definition ok.");

// Check camera configuration.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    size_t numIPCameraStreamServices;
    SetupCameraConfiguration(server, accessory, server->platform.ip.camera, &numIPCameraStreamServices);
    HAPPrecondition(server->ip.camera.streamingSessionStorage.numSessions >= numIPCameraStreamServices);
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Check Bluetooth LE requirements.
    if (server->transports.ble) {
        HAPNonnull(server->transports.ble)->validateAccessory(accessory);
    }
#endif

    // Start accessory server.
    HAPAccessoryServerPrepareStart(server, accessory, /* bridgedAccessories: */ NULL);
    if (server->state != kHAPAccessoryServerState_Running) {
        HAPAssert(server->state == kHAPAccessoryServerState_Idle);
        return;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->start) {
            serverEngine->start(server);
        }
    }
#endif
}

void HAPAccessoryServerStartBLETransport(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (!server->transports.ble) {
        return;
    }

    if (server->ble.isTransportRunning) {
        if (server->ble.isTransportStopping) {
            // Transport is stopping. Flag that it needs to restart after stopping
            server->ble.shouldStartTransport = true;
        } else {
            // already started
            server->ble.shouldStartTransport = false;
        }
        server->ble.shouldStopTransportWhenDisconnected = false;
        return;
    }

    HAPNonnull(server->transports.ble)->prepareStart(server);
    HAPNonnull(server->transports.ble)->start(server);

    // Update setup payload.
    HAPAccessorySetupInfoHandleAccessoryServerStart(server);

    // Update advertising state.
    HAPAccessoryServerUpdateAdvertisingData(server);
#endif
}

void HAPAccessoryServerStartThreadTransport(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (!server->transports.thread) {
        return;
    }

    if (server->thread.isTransportRunning) {
        if (server->thread.isTransportStopping) {
            // Transport is stopping. Flag that it needs to restart after stopping
            server->thread.shouldStartTransport = true;
        } else {
            // already started
            server->thread.shouldStartTransport = false;
        }
        return;
    }

    server->thread.shouldStartTransport = false;

    HAPNonnull(server->transports.thread)->prepareStart(server);

    if (server->sessionStorage.bytes) {
        // Initialize session storage
        HAPThreadSessionStorageHandleAccessoryServerWillStart(server);
    }

    HAPNonnull(server->transports.thread)->start(server);

    // Update setup payload.
    HAPAccessorySetupInfoHandleAccessoryServerStart(server);

    // Update advertising state.
    HAPAccessoryServerUpdateAdvertisingData(server);
#endif
}

void HAPAccessoryServerStartBridge(
        HAPAccessoryServer* server,
        const HAPAccessory* bridgeAccessory,
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories,
        bool configurationChanged) {
    HAPPrecondition(server);
    HAPPrecondition(bridgeAccessory);

    HAPError err;

    HAPLogDebug(
            &logObject,
            "Checking accessory definition. "
            "If this crashes, verify that accessory, service and characteristic lists are properly NULL-terminated.");
    HAPPrecondition(HAPRegularAccessoryIsValid(server, bridgeAccessory));
    if (bridgedAccessories) {
        size_t i;
        for (i = 0; bridgedAccessories[i]; i++) {
            HAPPrecondition(HAPBridgedAccessoryIsValid(bridgedAccessories[i]));
        }
        HAPPrecondition(i <= kHAPAccessoryServerMaxBridgedAccessories);
    }
    HAPLogDebug(&logObject, "Accessory definition ok.");

    HAPAccessoryServerPrepareStart(server, bridgeAccessory, bridgedAccessories);
    if (server->state != kHAPAccessoryServerState_Running) {
        HAPAssert(server->state == kHAPAccessoryServerState_Idle);
        return;
    }

    // Increment configuration number if necessary.
    if (configurationChanged) {
        HAPLogInfo(&logObject, "Configuration changed. Incrementing CN.");
        err = HAPAccessoryServerIncrementCN(server->platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->start) {
            serverEngine->start(server);
        }
    }
#endif
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
void HAPAccessoryServerStartCameraBridge(
        HAPAccessoryServer* server,
        const HAPAccessory* bridgeAccessory,
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories,
        const HAPPlatformCameraRef _Nullable* _Nullable bridgedCameras,
        bool configurationChanged) {
    HAPPrecondition(server);
    HAPPrecondition(bridgeAccessory);

    HAPError err;

    HAPLogDebug(
            &logObject,
            "Checking accessory definition. "
            "If this crashes, verify that accessory, service and characteristic lists are properly NULL-terminated.");
    HAPPrecondition(HAPRegularAccessoryIsValid(server, bridgeAccessory));
    if (bridgedAccessories) {
        size_t i;
        for (i = 0; bridgedAccessories[i]; i++) {
            HAPPrecondition(HAPBridgedAccessoryIsValid(bridgedAccessories[i]));
        }
        HAPPrecondition(i <= kHAPAccessoryServerMaxBridgedAccessories);
    }
    HAPLogDebug(&logObject, "Accessory definition ok.");

    // Check camera configurations.
    size_t totalCameraStreamServices = 0;
    if (server->platform.ip.camera) {
        SetupCameraConfiguration(server, bridgeAccessory, server->platform.ip.camera, &totalCameraStreamServices);
    }
    if (bridgedAccessories) {
        HAPPrecondition(bridgedCameras);
        for (size_t i = 0; bridgedAccessories[i]; i++) {
            size_t numIPCameraStreamServices;
            SetupCameraConfiguration(server, bridgedAccessories[i], bridgedCameras[i], &numIPCameraStreamServices);
            totalCameraStreamServices += numIPCameraStreamServices;
        }
    }
    HAPPrecondition(server->ip.camera.streamingSessionStorage.numSessions >= totalCameraStreamServices);

    server->ip.bridgedCameras = bridgedCameras;

    HAPAccessoryServerPrepareStart(server, bridgeAccessory, bridgedAccessories);
    if (server->state != kHAPAccessoryServerState_Running) {
        HAPAssert(server->state == kHAPAccessoryServerState_Idle);
        return;
    }

    // Increment configuration number if necessary.
    if (configurationChanged) {
        HAPLogInfo(&logObject, "Configuration changed. Incrementing CN.");
        err = HAPAccessoryServerIncrementCN(server->platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }

    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->start) {
            serverEngine->start(server);
        }
    }
}
#endif

static void TryDeregisterUnpairedStateTimer(HAPAccessoryServer* server) {
    if (server->unpairedStateTimer) {
        HAPPlatformTimerDeregister(server->unpairedStateTimer);
        server->unpairedStateTimer = 0;
        HAPLogDebug(&logObject, "Unpaired state timer stopped.");
    }
}

static void _HAPAccessoryServerStop(void* _Nullable context_, size_t contextSize) {
    HAPPrecondition(context_);
    HAPPrecondition(contextSize == sizeof(void*));
    HAPAccessoryServer* server = *((HAPAccessoryServer**) context_);

    if (server->state == kHAPAccessoryServerState_Idle) {
        return;
    }
    if (server->state != kHAPAccessoryServerState_Stopping) {
        HAPAssert(server->state == kHAPAccessoryServerState_Running);
        HAPLogInfo(&logObject, "Accessory server shutting down.");
        server->state = kHAPAccessoryServerState_Stopping;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        if (!server->transports.ip || !HAPNonnull(server->transports.ip)->serverEngine.get(server)) {
            server->callbacks.handleUpdatedState(server, server->context);
        }
#endif
    }

    TryDeregisterUnpairedStateTimer(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    // Stop advertising.
    if (server->transports.ble) {
        HAPAccessoryServerUpdateAdvertisingData(server);
    }
#endif

// Remove Wi-Fi router delegate.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)
    if (server->platform.ip.wiFiRouter) {
        HAPCheckedPlatformWiFiRouterSetDelegate(HAPNonnull(server->platform.ip.wiFiRouter), NULL);
    }
#endif

    // Stop the Data Stream subsystem (does nothing interesting if not needed)
    HAPDataStreamPrepareStop(server);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        HAPNonnull(server->transports.ip)->prepareStop(server);
    }
#endif

    bool didStopBLE = true;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        HAPNonnull(server->transports.ble)->tryStop(server, &didStopBLE);
    }
#endif

    bool didStopThread = true;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.thread) {
        HAPNonnull(server->transports.thread)->tryStop(server, &didStopThread);
    }
#endif

    if (!didStopBLE || !didStopThread) {
        return;
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    // Inform server engine.
    // Server engine will complete the shutdown process.
    // - _serverEngine->stop
    // - ...
    // - HAPAccessoryServerDelegateScheduleHandleUpdatedState => kHAPAccessoryServerState_Idle.
    // - CompleteShutdown.
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->stop) {
            HAPError err = serverEngine->stop(server);
            if (err) {
                HAPFatalError();
            }
            return;
        }
    }
#endif

    // Complete shutdown.
    CompleteShutdown(server);
}

void HAPAccessoryServerStop(HAPAccessoryServer* server) {
    HAPAccessoryServer** serverPtr = &server;
    HAPError err = HAPPlatformRunLoopScheduleCallback(_HAPAccessoryServerStop, serverPtr, sizeof(serverPtr));
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPAccessoryServerStop event scheduling failed");
        HAPFatalError();
    }
}

void HAPAccessoryServerForceStop(HAPAccessoryServer* server) {
    HAPAccessoryServer** serverPtr = &server;
    HAPLogDebug(&kHAPLog_Default, "HAPAccessoryServerForceStop");
    _HAPAccessoryServerStop(serverPtr, sizeof(serverPtr));
}

void HAPAccessoryServerKeepBleOn(HAPAccessoryServer* server, bool on HAP_UNUSED) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    server->ble.didUserRequestToKeepTransportOn = on;

    if (!server->transports.ble || !server->transports.thread) {
        // The feature is relevant only with concurrent BLE and Thread
        return;
    }

    if (server->state != kHAPAccessoryServerState_Running) {
        // BLE transport must not be started or stopped while server is not running yet.
        return;
    }

    if (on) {
        HAPAccessoryServerStartBLETransport(server);
    } else if (server->thread.isTransportRunning && !server->thread.isTransportStopping) {
        // Do not stop BLE if thread isn't running.
        HAPAccessoryServerStopBLETransport(server);
    }
#endif
}

void HAPAccessoryServerStopBLETransport(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->state == kHAPAccessoryServerState_Idle || server->state == kHAPAccessoryServerState_Stopping) {
        return;
    }

    if (!server->ble.isTransportRunning || server->ble.isTransportStopping) {
        return;
    }

    server->ble.isTransportStopping = true;

    HAPLogInfo(&logObject, "Accessory server shutting down BLE transport.");

    // Stop advertising.
    if (server->transports.ble) {
        HAPAccessoryServerUpdateAdvertisingData(server);
    }

    if (server->transports.ble) {
        bool didStop;
        HAPNonnull(server->transports.ble)->tryStop(server, &didStop);
    }
#endif
}

void HAPAccessoryServerStopBLETransportWhenDisconnected(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    HAPLogDebug(&logObject, "%s", __func__);

    if (server->state == kHAPAccessoryServerState_Idle || server->state == kHAPAccessoryServerState_Stopping) {
        return;
    }

    if (!server->ble.isTransportRunning || server->ble.isTransportStopping) {
        return;
    }

    if (server->ble.connection.connected) {
        // Currently connected. Wait till connection is disconnected.
        server->ble.shouldStopTransportWhenDisconnected = true;
    } else {
        // Not connected. Stop the transport immediately.
        HAPAccessoryServerStopBLETransport(server);
    }
#endif
}

void HAPAccessoryServerStopThreadTransport(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->state == kHAPAccessoryServerState_Idle || server->state == kHAPAccessoryServerState_Stopping) {
        return;
    }

    HAPLogInfo(&logObject, "Accessory server shutting down Thread transport.");

    if (server->transports.thread) {
        if (!server->thread.isTransportRunning) {
            // already stopped
            return;
        }

        if (server->thread.isTransportStopping) {
            // Stop retrial is centralized with timer.
            return;
        }
        bool didStop;
        HAPNonnull(server->transports.thread)->tryStop(server, &didStop);
    }
#endif
}

void HAPAccessoryServerUpdateAdvertisingData(HAPAccessoryServer* server) {
    HAPPrecondition(server);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        HAPNonnull(server->transports.ble)->updateAdvertisingData(server, false);
    }
#endif
}

typedef struct {
    bool exists; /**< Pairing found. */
} PairingExistsEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError PairingExistsEnumerateCallback(
        void* _Nullable context,
        HAPAccessoryServer* server HAP_UNUSED,
        HAPPairingIndex pairingIndex HAP_UNUSED,
        bool* shouldContinue) {
    HAPPrecondition(context);
    PairingExistsEnumerateContext* arguments = context;
    HAPPrecondition(shouldContinue);

    arguments->exists = true;
    *shouldContinue = false;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
bool HAPAccessoryServerIsPaired(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    // Enumerate pairings.
    PairingExistsEnumerateContext context = { .exists = false };
    err = HAPPairingEnumerate(server, PairingExistsEnumerateCallback, &context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Unable to enumerate pairings.", __func__);
        HAPFatalError();
    }
    return context.exists;
}

void HAPAccessoryServerRefreshSetupPayload(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupDisplay);

    HAPAccessorySetupInfoRefreshSetupPayload(server);
}

void HAPAccessoryServerEnterNFCPairingMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupNFC);

    HAPAccessorySetupInfoEnterNFCPairingMode(server);
}

void HAPAccessoryServerExitNFCPairingMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupNFC);

    HAPAccessorySetupInfoExitNFCPairingMode(server);
}

HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsMFiHWAuth(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    return HAPMFiHWAuthIsAvailable(&server->mfi);
}

HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsMFiTokenAuth(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    bool supportsSoftwareAuthentication = false;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
    HAPError err;
    if (server->platform.authentication.mfiTokenAuth) {
        err = HAPPlatformMFiTokenAuthLoad(
                HAPNonnull(server->platform.authentication.mfiTokenAuth),
                &supportsSoftwareAuthentication,
                NULL,
                NULL,
                0,
                NULL);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "HAPPlatformMFiTokenAuthLoad failed: %u.", err);
            HAPFatalError();
        }
    }
#endif
    return supportsSoftwareAuthentication;
}

HAP_RESULT_USE_CHECK
uint8_t HAPAccessoryServerGetPairingFeatureFlags(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    // Serialize response.
    // See HomeKit Accessory Protocol Specification R17
    // Table 5-15 Pairing Feature Flags
    uint8_t featureFlags = 0;
    if (HAPAccessoryServerSupportsMFiHWAuth(server)) {
        featureFlags |= (uint8_t) kHAPCharacteristicValue_PairingFeatures_SupportsAppleAuthenticationCoprocessor;
    }
    if (HAPAccessoryServerSupportsMFiTokenAuth(server)) {
        featureFlags |= (uint8_t) kHAPCharacteristicValue_PairingFeatures_SupportsSoftwareAuthentication;
    }
    return featureFlags;
}

/**
 * Status flags.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 6-8 Bonjour TXT Status Flags
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.4.2.1.2 Manufacturer Data
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPAccessoryServerStatusFlags) {
    /** Accessory has not been paired with any controllers. */
    kHAPAccessoryServerStatusFlags_NotPaired = 1U << 0U,

    /**
     * Accessory has not been configured to join a Wi-Fi network.
     *
     * - Used by accessories supporting HAP over IP (Ethernet / Wi-Fi) only.
     */
    kHAPAccessoryServerStatusFlags_WiFiNotConfigured = 1U << 1U,

    /**
     * A problem has been detected on the accessory.
     *
     * - Used by accessories supporting HAP over IP (Ethernet / Wi-Fi) only.
     */
    kHAPAccessoryServerStatusFlags_ProblemDetected = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPAccessoryServerStatusFlags);

HAP_RESULT_USE_CHECK
uint8_t HAPAccessoryServerGetStatusFlags(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    uint8_t statusFlags = 0;
    if (!HAPAccessoryServerIsPaired(server)) {
        statusFlags |= (uint8_t) kHAPAccessoryServerStatusFlags_NotPaired;
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip && HAPAccessoryServerIsInWACMode(server)) {
        statusFlags |= (uint8_t) kHAPAccessoryServerStatusFlags_WiFiNotConfigured;
    }
#endif
    return statusFlags;
}

typedef struct {
    bool hasPairings : 1;
    bool adminFound : 1;
} FindAdminPairingEnumerateContext;

HAP_RESULT_USE_CHECK
static HAPError FindAdminPairingEnumerateCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        HAPPairingIndex pairingIndex,
        bool* shouldContinue) {
    HAPPrecondition(server);
    FindAdminPairingEnumerateContext* arguments = context;
    HAPPrecondition(arguments);
    HAPPrecondition(!arguments->adminFound);
    HAPPrecondition(shouldContinue);

    HAPError err;

    bool exists;
    HAPPairing pairing;
    err = HAPPairingGet(server, pairingIndex, &pairing, &exists);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!exists) {
        HAPLogError(&logObject, "%s: Pairing not found: %u.", __func__, (unsigned) pairingIndex);
        HAPFatalError();
    }
    arguments->hasPairings = true;

    // Check if admin.
    if (pairing.permissions & 0x01U) {
        arguments->adminFound = true;
        *shouldContinue = false;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerCleanupPairings(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPError err;

    HAPLogDebug(&logObject, "Checking if admin pairing exists.");

    // Look for admin pairing.
    FindAdminPairingEnumerateContext context;
    HAPRawBufferZero(&context, sizeof context);
    context.adminFound = false;
    err = HAPPairingEnumerate(server, FindAdminPairingEnumerateCallback, &context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // If there is no admin, delete all pairings.
    if (!context.adminFound) {
        if (context.hasPairings) {
            // Remove all pairings.
            HAPLogInfo(&logObject, "No admin pairing found. Removing all pairings.");
            HAPAccessoryServerDelegateScheduleHandleUpdatedState(server);
            err = HAPPairingRemoveAll(server);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }
        }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
        // Purge Pair Resume cache.
        if (server->transports.ble) {
            HAPRawBufferZero(
                    server->ble.storage->sessionCacheElements,
                    server->ble.storage->numSessionCacheElements * sizeof *server->ble.storage->sessionCacheElements);
        }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
        // Invalidate all HomeKit Data Streams.
        HAPDataStreamInvalidateAllForHAPSession(server, NULL);
#endif

        // Purge broadcast encryption key and advertising identifier.
        // See HomeKit Certification Test Cases R7.2
        // Test Case TCB052
        err = HAPPlatformKeyValueStoreRemove(
                server->platform.keyValueStore,
                kHAPKeyValueStoreDomain_Configuration,
                kHAPKeyValueStoreKey_Configuration_BLEBroadcastParameters);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerGetCN(HAPPlatformKeyValueStoreRef keyValueStore, uint16_t* cn) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(cn);

    HAPError err;

    // Prior to HomeKit Accessory Protocol Specification R12:
    // - CN was 32-bit for IP:  1 - 4294967295, overflow to 1
    // - CN was  8-bit for BLE: 1 - 255, overflow to 1
    //
    // Since HomeKit Accessory Protocol Specification R12:
    // - CN is 16-bit for IP:  1 - 65535, overflow to 1
    // - CN is  8-bit for BLE: 1 - 255, overflow to 1
    // - CN is 16-bit for HAP-Info-Response: 1 - 65535, overflow to 1
    //
    // To avoid breaking compatibility with legacy versions, we store CN as UInt32
    // and derive the shorter CN variants from it while staying consistent w.r.t. the various overflows to 1.

    // Try to load configuration number.
    bool found;
    size_t numBytes;
    uint8_t cnBytes[sizeof(uint32_t)];
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
            cnBytes,
            sizeof cnBytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        // Initialize configuration number.
        HAPWriteLittleUInt32(cnBytes, 1U);

        // Store new configuration number.
        err = HAPPlatformKeyValueStoreSet(
                keyValueStore,
                kHAPKeyValueStoreDomain_Configuration,
                kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
                cnBytes,
                sizeof cnBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    } else {
        if (numBytes != sizeof cnBytes) {
            HAPLogError(&logObject, "Invalid configuration number length: %zu.", numBytes);
            return kHAPError_Unknown;
        }
    }

    // Downscale to UInt16.
    uint32_t cn32 = HAPReadLittleUInt32(cnBytes);
    *cn = (uint16_t)((cn32 - 1) % UINT16_MAX + 1);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerIncrementCN(HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(keyValueStore);

    HAPError err;

    // Get CN.
    bool found;
    size_t numBytes;
    uint8_t cnBytes[sizeof(uint32_t)];
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
            cnBytes,
            sizeof cnBytes,
            &numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    if (!found) {
        // Initialize configuration number.
        HAPWriteLittleUInt32(cnBytes, 1U);
    }

    // Increment CN.
    {
        uint32_t cn32 = HAPReadLittleUInt32(cnBytes);
        if (cn32 == UINT32_MAX) {
            cn32 = 1;
        } else {
            cn32++;
        }
        HAPWriteLittleUInt32(cnBytes, cn32);
        HAPLogInfo(&logObject, "Updated CN: %lu.", (unsigned long) cn32);
    }

    // Save CN.
    err = HAPPlatformKeyValueStoreSet(
            keyValueStore,
            kHAPKeyValueStoreDomain_Configuration,
            kHAPKeyValueStoreKey_Configuration_ConfigurationNumber,
            cnBytes,
            sizeof cnBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

void HAPAccessoryServerRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    if (server->state != kHAPAccessoryServerState_Running) {
        HAPLogDebug(&logObject, "%s: Ignored the event while server is not running", __func__);
        return;
    }

    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPError err;

    HAPLogCharacteristicDebug(&logObject, characteristic, service, accessory, "Marking characteristic as modified.");

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble && server->ble.isTransportRunning) {
        err = HAPNonnull(server->transports.ble)->didRaiseEvent(server, characteristic, service, accessory, NULL);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->raise_event) {
            err = serverEngine->raise_event(server, characteristic, service, accessory);
            if (err) {
                HAPFatalError();
            }
        }
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.thread && server->thread.isTransportRunning) {
        HAPNotificationHandleRaiseEvent(server, characteristic, service, accessory);
    }
#endif
}

void HAPAccessoryServerRaiseEventOnSession(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(session);

    HAPError err;

    HAPLogCharacteristicDebug(
            &logObject,
            characteristic,
            service,
            accessory,
            "Marking characteristic as modified for session %p.",
            (const void*) session);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble) {
        err = HAPNonnull(server->transports.ble)->didRaiseEvent(server, characteristic, service, accessory, session);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip) {
        const HAPAccessoryServerServerEngine* _Nullable serverEngine =
                HAPNonnull(server->transports.ip)->serverEngine.get(server);
        if (serverEngine && serverEngine->raise_event_on_session) {
            err = serverEngine->raise_event_on_session(server, characteristic, service, accessory, session);
            if (err) {
                HAPFatalError();
            }
        }
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.thread) {
        HAPNotificationHandleRaiseEventOnSession(server, characteristic, service, accessory, session);
    }
#endif
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
void HAPAccessoryServerIdentify(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPError err;

    const HAPService* service = NULL;
    for (size_t i = 0; server->primaryAccessory->services[i]; i++) {
        const HAPService* s = server->primaryAccessory->services[i];
        if ((s->iid == kHAPIPAccessoryProtocolIID_AccessoryInformation) &&
            HAPUUIDAreEqual(s->serviceType, &kHAPServiceType_AccessoryInformation)) {
            service = s;
            break;
        }
    }
    if (service) {
        const HAPBaseCharacteristic* characteristic = NULL;
        for (size_t i = 0; service->characteristics[i]; i++) {
            const HAPBaseCharacteristic* c = service->characteristics[i];
            if (HAPUUIDAreEqual(c->characteristicType, &kHAPCharacteristicType_Identify) &&
                (c->format == kHAPCharacteristicFormat_Bool) && c->properties.writable) {
                characteristic = c;
                break;
            }
        }
        if (characteristic) {
            HAPLogAccessoryDebug(&logObject, server->primaryAccessory, "Identifying.");
            HAPBoolCharacteristicWriteRequest request;
            HAPRawBufferZero(&request, sizeof request);
            request.transportType = session->transportType;
            request.session = session;
            request.characteristic = (const HAPBoolCharacteristic*) characteristic;
            request.service = service;
            request.accessory = HAPNonnull(server->primaryAccessory);
            request.remote = false;
            request.authorizationData.bytes = NULL;
            request.authorizationData.numBytes = 0;
            err = HAPBoolCharacteristicHandleWrite(server, &request, true, HAPAccessoryServerGetClientContext(server));
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                HAPLog(&logObject, "Identify failed: %u.", err);
            }
        } else {
            HAPLogService(&logObject, service, server->primaryAccessory, "Identify characteristic not found.");
        }
    } else {
        HAPLogAccessory(&logObject, server->primaryAccessory, "Accessory Information service not found.");
    }
}
#endif

void HAPAccessoryServerHandleSubscribe(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPLogCharacteristicDebug(
            &logObject, characteristic, service, accessory, "Informing application about %s of events.", "enabling");

    switch (((const HAPBaseCharacteristic*) characteristic)->format) {
        case kHAPCharacteristicFormat_Data: {
            HAPDataCharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPDataCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Bool: {
            HAPBoolCharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPBoolCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt8: {
            HAPUInt8CharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt8CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            HAPUInt16CharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt16CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            HAPUInt32CharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt32CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            HAPUInt64CharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt64CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            HAPIntCharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPIntCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            HAPFloatCharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPFloatCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_String: {
            HAPStringCharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPStringCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_TLV8: {
            HAPTLV8CharacteristicHandleSubscribe(
                    HAPNonnull(session->server),
                    &(const HAPTLV8CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
    }
}

void HAPAccessoryServerHandleUnsubscribe(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(characteristic);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    switch (((const HAPBaseCharacteristic*) characteristic)->format) {
        case kHAPCharacteristicFormat_Data: {
            HAPDataCharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPDataCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Bool: {
            HAPBoolCharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPBoolCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt8: {
            HAPUInt8CharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt8CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt16: {
            HAPUInt16CharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt16CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt32: {
            HAPUInt32CharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt32CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_UInt64: {
            HAPUInt64CharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPUInt64CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Int: {
            HAPIntCharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPIntCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_Float: {
            HAPFloatCharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPFloatCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_String: {
            HAPStringCharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPStringCharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
        case kHAPCharacteristicFormat_TLV8: {
            HAPTLV8CharacteristicHandleUnsubscribe(
                    HAPNonnull(session->server),
                    &(const HAPTLV8CharacteristicSubscriptionRequest) {
                            .transportType = session->transportType,
                            .session = session,
                            .characteristic = characteristic,
                            .service = service,
                            .accessory = accessory,
                    },
                    HAPAccessoryServerGetClientContext(HAPNonnull(session->server)));
            break;
        }
    }
}

HAP_RESULT_USE_CHECK
bool HAPAccessoryServerSupportsService(
        HAPAccessoryServer* server,
        HAPTransportType transportType,
        const HAPService* service) {
    HAPPrecondition(server);
    HAPPrecondition(HAPTransportTypeIsValid(transportType));
    HAPPrecondition(service);

    if (transportType == kHAPTransportType_IP && HAPUUIDAreEqual(service->serviceType, &kHAPServiceType_Pairing)) {
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetNumServiceInstances(HAPAccessoryServer* server, const HAPUUID* serviceType) {
    HAPPrecondition(server);
    HAPPrecondition(serviceType);

    HAPServiceTypeIndex serviceTypeIndex = 0;

    HAPPrecondition(server->primaryAccessory);
    {
        const HAPAccessory* acc = server->primaryAccessory;
        if (acc->services) {
            for (size_t i = 0; acc->services[i]; i++) {
                const HAPService* svc = acc->services[i];
                if (HAPUUIDAreEqual(svc->serviceType, serviceType)) {
                    serviceTypeIndex++;
                    HAPAssert(serviceTypeIndex); // No overflow.
                }
            }
        }
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->ip.bridgedAccessories) {
        for (size_t j = 0; server->ip.bridgedAccessories[j]; j++) {
            const HAPAccessory* acc = server->ip.bridgedAccessories[j];
            if (acc->services) {
                for (size_t i = 0; acc->services[i]; i++) {
                    const HAPService* svc = acc->services[i];
                    if (HAPUUIDAreEqual(svc->serviceType, serviceType)) {
                        serviceTypeIndex++;
                        HAPAssert(serviceTypeIndex); // No overflow.
                    }
                }
            }
        }
    }
#endif

    return serviceTypeIndex;
}

HAP_RESULT_USE_CHECK
HAPServiceTypeIndex HAPAccessoryServerGetServiceTypeIndex(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPServiceTypeIndex serviceTypeIndex = 0;

    HAPPrecondition(server->primaryAccessory);
    {
        const HAPAccessory* acc = server->primaryAccessory;
        if (acc->services) {
            for (size_t i = 0; acc->services[i]; i++) {
                const HAPService* svc = acc->services[i];
                if (svc == service && acc == accessory) {
                    return serviceTypeIndex;
                }
                if (HAPUUIDAreEqual(svc->serviceType, service->serviceType)) {
                    serviceTypeIndex++;
                    HAPAssert(serviceTypeIndex); // No overflow.
                }
            }
        }
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->ip.bridgedAccessories) {
        for (size_t j = 0; server->ip.bridgedAccessories[j]; j++) {
            const HAPAccessory* acc = server->ip.bridgedAccessories[j];
            if (acc->services) {
                for (size_t i = 0; acc->services[i]; i++) {
                    const HAPService* svc = acc->services[i];
                    if (svc == service && acc == accessory) {
                        return serviceTypeIndex;
                    }
                    if (HAPUUIDAreEqual(svc->serviceType, service->serviceType)) {
                        serviceTypeIndex++;
                        HAPAssert(serviceTypeIndex); // No overflow.
                    }
                }
            }
        }
    }
#endif

    HAPLogServiceError(&logObject, service, accessory, "Service not found in accessory server's attribute database.");
    HAPFatalError();
}

void HAPAccessoryServerGetServiceFromServiceTypeIndex(
        HAPAccessoryServer* server,
        const HAPUUID* serviceType,
        HAPServiceTypeIndex serviceTypeIndex,
        const HAPService** service,
        const HAPAccessory** accessory) {
    HAPPrecondition(server);
    HAPPrecondition(serviceType);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    HAPPrecondition(server->primaryAccessory);
    {
        const HAPAccessory* acc = server->primaryAccessory;
        if (acc->services) {
            for (size_t i = 0; acc->services[i]; i++) {
                const HAPService* svc = acc->services[i];
                if (HAPUUIDAreEqual(svc->serviceType, serviceType)) {
                    if (!serviceTypeIndex) {
                        *service = svc;
                        *accessory = acc;
                        return;
                    }
                    serviceTypeIndex--;
                }
            }
        }
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->ip.bridgedAccessories) {
        for (size_t j = 0; server->ip.bridgedAccessories[j]; j++) {
            const HAPAccessory* acc = server->ip.bridgedAccessories[j];
            if (acc->services) {
                for (size_t i = 0; acc->services[i]; i++) {
                    const HAPService* svc = acc->services[i];
                    if (HAPUUIDAreEqual(svc->serviceType, serviceType)) {
                        if (!serviceTypeIndex) {
                            *service = svc;
                            *accessory = acc;
                            return;
                        }
                        serviceTypeIndex--;
                    }
                }
            }
        }
    }
#endif

    HAPLogError(&logObject, "Service type index not found in accessory server's attribute database.");
    HAPFatalError();
}

//----------------------------------------------------------------------------------------------------------------------

void HAPAccessoryServerEnumerateConnectedSessions(
        HAPAccessoryServer* server,
        HAPAccessoryServerEnumerateSessionsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(callback);

    bool shouldContinue = true;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (server->transports.ble && server->ble.storage) {
        if (server->ble.storage->session && server->ble.connection.connected) {
            callback(context, server, HAPNonnull(server->ble.storage->session), &shouldContinue);
        }
        if (!shouldContinue) {
            return;
        }
    }
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (server->transports.ip && server->ip.storage) {
        for (size_t i = 0; shouldContinue && i < server->ip.storage->numSessions; i++) {
            HAPIPSession* ipSession = &server->ip.storage->sessions[i];
            HAPIPSessionDescriptor* session = &ipSession->descriptor;
            if (!session->server) {
                continue;
            }
            HAPAssert(session->server == server);
            if (session->securitySession.type != kHAPIPSecuritySessionType_HAP) {
                continue;
            }
            callback(context, server, &session->securitySession._.hap, &shouldContinue);
        }
        if (!shouldContinue) {
            return;
        }
    }
#endif
}

HAP_RESULT_USE_CHECK
size_t HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    size_t numCharacteristicsSupportingEventNotification = 0;

    size_t accessoryIndex = 0;
    const HAPAccessory* accessory = server->primaryAccessory;
    while (accessory) {
        if (accessory->services) {
            for (size_t i = 0; accessory->services[i]; i++) {
                const HAPService* service = accessory->services[i];
                if (service->characteristics) {
                    for (size_t j = 0; service->characteristics[j]; j++) {
                        const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                        if (characteristic->properties.supportsEventNotification) {
                            numCharacteristicsSupportingEventNotification++;
                            HAPAssert(numCharacteristicsSupportingEventNotification);
                        }
                    }
                }
            }
        }
        accessoryIndex++;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        accessory = server->ip.bridgedAccessories ? server->ip.bridgedAccessories[accessoryIndex - 1] : NULL;
#else
        accessory = NULL;
#endif
    }

    return numCharacteristicsSupportingEventNotification;
}

void HAPAccessoryServerGetEventNotificationIndex(
        HAPAccessoryServer* server,
        uint64_t aid,
        uint64_t iid,
        size_t* index,
        bool* indexFound) {
    HAPPrecondition(server);
    HAPPrecondition(index);
    HAPPrecondition(indexFound);

    *index = 0;
    *indexFound = false;

    size_t accessoryIndex = 0;
    const HAPAccessory* accessory = server->primaryAccessory;
    while (accessory) {
        if (accessory->services) {
            for (size_t i = 0; accessory->services[i]; i++) {
                const HAPService* service = accessory->services[i];
                if (service->characteristics) {
                    for (size_t j = 0; service->characteristics[j]; j++) {
                        const HAPBaseCharacteristic* characteristic = service->characteristics[j];
                        if (characteristic->properties.supportsEventNotification) {
                            if (accessory->aid == aid && characteristic->iid == iid) {
                                *indexFound = true;
                                return;
                            }
                            (*index)++;
                            HAPAssert(*index);
                        }
                    }
                }
            }
        }
        accessoryIndex++;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        accessory = server->ip.bridgedAccessories ? server->ip.bridgedAccessories[accessoryIndex - 1] : NULL;
#else
        accessory = NULL;
#endif
    }
}

/**
 * Start heart beat update timer
 */
static HAPError StartHeartBeatUpdateTimer(HAPAccessoryServer* server) {
    uint16_t random;
    HAPPlatformRandomNumberFill(&random, sizeof random);
    HAPTime interval =
            kHeartBeatUpdateIntervalRandomRangeBegin + (kHeartBeatUpdateIntervalRandomRange * random) / UINT16_MAX;
    HAPError err = HAPPlatformTimerRegister(
            &server->heartBeat.timer, HAPPlatformClockGetCurrent() + interval, HandleHeartBeatUpdateTimer, server);
    return err;
}

/**
 * Handle heart beat update timer expiration
 */
static void HandleHeartBeatUpdateTimer(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable data HAP_UNUSED) {
    HAPPrecondition(data);
    HAPAccessoryServer* server = (HAPAccessoryServer*) data;

    // Increment hear beat value
    server->heartBeat.value++;

    // Restart the timer
    HAPError err = StartHeartBeatUpdateTimer(server);
    HAPAssert(err == kHAPError_None);

    if (server->heartBeat.callback) {
        server->heartBeat.callback(server->heartBeat.data);
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerStartHeartBeat(
        HAPAccessoryServer* server,
        HAPAccessoryServerHeartBeatCallback callback,
        void* _Nullable data) {
    server->heartBeat.callback = callback;
    server->heartBeat.data = data;
    server->heartBeat.value = 1;
    if (server->heartBeat.timer) {
        HAPPlatformTimerDeregister(server->heartBeat.timer);
        server->heartBeat.timer = 0;
    }
    HAPError err = StartHeartBeatUpdateTimer(server);
    return err;
}

/**
 * Handle unpaired state timer expiration
 *
 * @param timer    expired timer
 * @param context  context
 */
static void
        HandleUnpairedStateTimerExpiration(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    server->unpairedStateTimer = 0;

    // Stop the server
    HAPAccessoryServer** serverPtr = &server;
    _HAPAccessoryServerStop(serverPtr, sizeof(serverPtr));
}

void HAPAccessoryServerUpdateUnpairedStateTimer(HAPAccessoryServer* server) {
    if (HAPAccessoryServerIsPaired(server)) {
        // Stop unpaired state timer if it is running
        TryDeregisterUnpairedStateTimer(server);
    } else {
        // Start unpaired state timer if it isn't running
        if (server->unpairedStateTimer == 0) {
            HAPError err = HAPPlatformTimerRegister(
                    &server->unpairedStateTimer,
                    HAPPlatformClockGetCurrent() + kUnpairedStateTimerDuration,
                    HandleUnpairedStateTimerExpiration,
                    server);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                // Due to security risk, the program cannot proceed.
                HAPLogError(&logObject, "Cannot start unpaired state timer.");
                HAPFatalError();
            }
            HAPLogDebug(&logObject, "Unpaired state timer started.");
        }
    }
}

void HAPAccessoryServerGetAccessoryConfigurationString(
        const HAPPlatform* platform,
        const HAPAccessory* primaryAccessory,
        char* configBuffer,
        size_t configBufferSize) {
    HAPPrecondition(platform);
    HAPPrecondition(primaryAccessory);

    HAPAssert(configBufferSize >= 1024);
    if (configBufferSize < 1024) {
        return;
    }
    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, configBuffer, configBufferSize);
    HAPStringBuilderAppend(&stringBuilder, "Version Information:");
    HAPStringBuilderAppend(
            &stringBuilder,
            "\n- ADK Version: %s (%s) - compatibility version %lu",
            HAPGetVersion(),
            HAPGetBuild(),
            (unsigned long) HAPGetCompatibilityVersion());
    HAPStringBuilderAppend(&stringBuilder, "\n- Extensions:");
#if (HAP_FEATURE_KEY_EXPORT == 1)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Key Export");
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HomeKit Data Stream (TCP)");
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HomeKit Data Stream (HAP)");
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HAP over BLE");
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HAP over Thread");
#endif
#if HAP_DYNAMIC_MEMORY_ALLOCATION
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Dynamic memory allocation");
#endif
    HAPStringBuilderAppend(&stringBuilder, "\n- Platform Information:");
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Type: %s", HAPPlatformGetIdentification());
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Compiler: %s", HAPGetCompilerVersion());
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Version: %s (%s)", HAPPlatformGetVersion(), HAPPlatformGetBuild());
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HAP Log:");
    HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Log Level: %d", HAP_LOG_LEVEL);
#if (HAP_ENABLE_BUFFER_LOGS == 1)
    HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Buffer Logs enabled");
#endif
#if (HAP_LOG_SENSITIVE == 1)
    HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Sensitive Logs enabled");
#endif
#if (HAP_TESTING == 1)
    HAPStringBuilderAppend(&stringBuilder, "\n\t- HAP Testing is enabled");
#endif
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Available features:");
    if (platform->keyValueStore) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Key-Value store");
    }
    if (platform->accessorySetup) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Accessory setup manager");
    }
    if (platform->setupDisplay) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Accessory setup display");
    }
    if (platform->setupNFC) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Accessory setup programmable NFC tag");
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    if (platform->ip.tcpStreamManager) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- TCP stream manager");
    }
    if (platform->ip.serviceDiscovery) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Service discovery");
    }
    if (platform->ip.wiFi.softwareAccessPoint) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Software Access Point manager");
    }
    if (platform->ip.wiFi.wiFiManager) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Wi-Fi manager");
    }
    if (platform->ip.camera) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- IP Camera provider");
    }
    if (platform->ip.wiFiRouter) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Wi-Fi Router");
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (platform->ble.blePeripheralManager) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- BLE peripheral manager");
    }
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    if (platform->thread.coapManager) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- CoAP manager");
    }
#endif
    if (platform->authentication.mfiHWAuth) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Apple Authentication Coprocessor provider");
    }
    if (platform->authentication.mfiTokenAuth) {
        HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Software Token provider");
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
    HAPStringBuilderAppend(&stringBuilder, "\n\t\t- Wi-Fi Reconfiguration Control");
#endif

    HAPStringBuilderAppend(&stringBuilder, "\n- Accessory Information:");
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Name             : %s", primaryAccessory->name);
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Manufacturer     : %s", primaryAccessory->manufacturer);
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Model            : %s", primaryAccessory->model);
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Serial Number    : %s", primaryAccessory->serialNumber);
    HAPStringBuilderAppend(&stringBuilder, "\n\t- Product Data     : %s", primaryAccessory->productData);

    {
        // Read firmware version.
        HAPAssert(primaryAccessory->firmwareVersion);
        uint32_t major;
        uint32_t minor;
        uint32_t revision;
        HAPError err = ParseVersionString(primaryAccessory->firmwareVersion, &major, &minor, &revision);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPFatalError();
        }
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n\t- Firmware version : %lu.%lu.%lu",
                (unsigned long) major,
                (unsigned long) minor,
                (unsigned long) revision);
    }

    {
        // Read Hardware version.
        HAPAssert(primaryAccessory->hardwareVersion);
        uint32_t major;
        uint32_t minor;
        uint32_t revision;
        HAPError err = ParseVersionString(primaryAccessory->firmwareVersion, &major, &minor, &revision);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPFatalError();
        }
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n\t- Hardware version : %lu.%lu.%lu",
                (unsigned long) major,
                (unsigned long) minor,
                (unsigned long) revision);
    }

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogError(&logObject, "Version information truncated.");
    }
}

static void HAPAccessoryServerPrintAccessoryConfiguration(
        const HAPPlatform* platform,
        const HAPAccessory* primaryAccessory) {
    HAPPrecondition(platform);
    HAPPrecondition(primaryAccessory);
    char configurationBytes[1024];
    HAPRawBufferZero(configurationBytes, sizeof configurationBytes);

    HAPAccessoryServerGetAccessoryConfigurationString(
            platform, primaryAccessory, configurationBytes, sizeof configurationBytes);
    HAPLog(&logObject, "%s", configurationBytes);
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
    HAPPlatformDiagnosticsWriteToConfigLog("%s", configurationBytes);
#endif
}
