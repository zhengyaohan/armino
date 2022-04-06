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

#include "HAPAccessorySetupInfo.h"
#include "HAPAccessory.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPAccessorySetup.h"
#include "HAPLogSubsystem.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "AccessorySetupInfo" };

// Use Cases:
//
// 1. Accessory that does not have a display or programmable NFC tag.
//    - No dynamic setup code is generated, and no setup payloads need to be derived.
//
// 2. Pre-R10 accessory with a display that does not have a setup ID.
//    - A dynamic setup code is generated periodically that may be displayed in text form.
//    - Setup payloads may not be derived without a setup ID, so QR codes and NFC tags don't work.
//
// 3. Accessory with a programmable NFC tag but no display.
//    - Programmable NFC tag must only be enabled in response to user interaction.
//    - NFC pairing mode expires after 5 minutes.
//    - While NFC pairing mode is not active (or while accessory is paired) special setup payloads are generated
//      to guide the user into restarting / factory resetting the accessory on iOS.
//    - A static setup code is provisioned and also affixed to the accessory.
//
// 4. Accessory with a display.
//    - The dynamic setup code needs to be refreshed periodically (every 5 minutes).
//    - During a pairing attempt the protocol does not allow changing the setup code.
//      Therefore, the 5 minutes timer is best-effort only.
//    - A new setup code is generated for each pairing attempt, even when this is more frequently than every 5 minutes.
//    - If programmable NFC is available, the same setup payload needs to be used as for the display.
//
// 5. Accessory with complex UI.
//    - Accessories may opt to keep track of the current accessory setup information in background.
//    - When a pairing attempt is registered a popup may be shown that guides the user to the setup code screen.
//    - When a pairing attempt is canceled the UI may want to indicate that pairing failed / was successful.
//
// 6. Software Authentication.
//    - After a Transient Pair Setup procedure the setup code needs to be saved.
//      The next Split Pair Setup procedure will re-use the setup code from the previous pairing attempt.
//    - There is no timeout, if dynamic setup codes are used they cannot be refreshed until the next pairing attempt.
//
// 7. Legacy iOS behaviour.
//    - At start of pairing, iOS controllers first connect to the accessory and then ask for the setup code.
//    - However, if setup code is entered incorrectly, iOS first asks for the setup code and connects after entering it.
//      This makes it necessary to always have a setup code available to anticipate another pairing attempt.
//      Otherwise, the user would need to guess the next upcoming setup code.
//    - Since iOS 12, this was fixed and iOS always first connects and then asks for the setup code.
//
// Power considerations:
// - Constantly having a timer running to refresh displays has negligible energy impact.
// - Computing new SRP salts and verifiers is heavier. Therefore, it is only computed on demand.

//----------------------------------------------------------------------------------------------------------------------

static void SynchronizeDisplayAndNFC(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.accessorySetup);

    if (!server->platform.setupDisplay && !server->platform.setupNFC) {
        return;
    }

    // See HomeKit Accessory Protocol Specification R17
    // Section 4.4.2.1 Requirements

    // Derive setup payload flags.
    HAPAccessorySetupSetupPayloadFlags flags = {
        .isPaired = HAPAccessoryServerIsPaired(server),
        .threadSupported = kHAPSupportedTransports.thread_supported,
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
        .ipSupported = server->transports.ip != NULL,
#else
        .ipSupported = false,
#endif
        .bleSupported = kHAPSupportedTransports.ble_supported,
#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) && HAP_FEATURE_ENABLED(HAP_FEATURE_WAC)
        .wacSupported = kHAPSupportedTransports.wac_supported && server->caps.wac,
#else
        .wacSupported = false,
#endif
    };
    HAPSetupPayload nonPairablePayload;
    HAPAccessorySetupInfoGenerateSetupPayload(server, flags, NULL, false, &nonPairablePayload);

    // Fetch setup code.
    HAPSetupCode* _Nullable setupCode = NULL;
    if (server->accessorySetup.state.setupCodeIsAvailable) {
        setupCode = &server->accessorySetup.state.setupCode;
    }

    // Generate pairable setup payload if applicable.
    HAPSetupPayload pairablePayload;
    bool hasPairablePayload = false;
    if (setupCode &&
        (server->platform.setupDisplay || (server->platform.setupNFC && server->accessorySetup.nfcPairingModeTimer))) {
        hasPairablePayload = true;
        HAPAccessorySetupInfoGenerateSetupPayload(server, flags, setupCode, true, &pairablePayload);
    }

    // Update displays.
    if (server->platform.setupDisplay) {
        HAPLogSensitiveInfo(
                &logObject,
                "Updating display setup payload: %s.",
                hasPairablePayload ? pairablePayload.stringValue : "NULL");
        HAPPlatformAccessorySetupDisplayUpdateSetupPayload(
                HAPNonnull(server->platform.setupDisplay), hasPairablePayload ? &pairablePayload : NULL, setupCode);
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC)
    // Update programmable NFC tags.
    if (server->platform.setupNFC) {
        if (server->accessorySetup.nfcPairingModeTimer) {
            HAPLogSensitiveInfo(
                    &logObject,
                    "Updating NFC setup payload: %s.",
                    hasPairablePayload ? pairablePayload.stringValue : nonPairablePayload.stringValue);
            HAPPlatformAccessorySetupNFCUpdateSetupPayload(
                    HAPNonnull(server->platform.setupNFC),
                    hasPairablePayload ? &pairablePayload : &nonPairablePayload,
                    hasPairablePayload);
        } else {
            HAPLogSensitiveInfo(&logObject, "Updating NFC setup payload: %s.", nonPairablePayload.stringValue);
            HAPPlatformAccessorySetupNFCUpdateSetupPayload(
                    HAPNonnull(server->platform.setupNFC), &nonPairablePayload, /* isPairable: */ false);
        }
    }
#endif
}

//----------------------------------------------------------------------------------------------------------------------

static void DynamicSetupInfoExpired(HAPPlatformTimerRef timer, void* _Nullable context);

static void ClearSetupInfo(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable) {
        HAPLogDebug(&logObject, "Invalidating setup info.");
        HAPRawBufferZero(&server->accessorySetup.state, sizeof server->accessorySetup.state);
        if (server->accessorySetup.dynamicRefreshTimer) {
            HAPPlatformTimerDeregister(server->accessorySetup.dynamicRefreshTimer);
            server->accessorySetup.dynamicRefreshTimer = 0;
        }
    }
    HAPAssert(!server->accessorySetup.dynamicRefreshTimer);
}

static void PrepareSetupInfo(HAPAccessoryServer* server, bool lockSetupInfo) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.accessorySetup);
    HAPPrecondition(!HAPAccessoryServerIsPaired(server));

    HAPError err;

    if (server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable) {
        if (lockSetupInfo) {
            server->accessorySetup.state.lockSetupInfo = lockSetupInfo;

            if (server->accessorySetup.dynamicRefreshTimer) {
                HAPLogDebug(&logObject, "Locking dynamic setup code for pairing attempt.");
                HAPPlatformTimerDeregister(server->accessorySetup.dynamicRefreshTimer);
                server->accessorySetup.dynamicRefreshTimer = 0;
            }
        } else if (server->accessorySetup.state.lockSetupInfo) {
            HAPLogDebug(&logObject, "Keeping setup code locked for pairing attempt.");
        } else {
            // Setup code is not locked.
        }
        return;
    }

    HAPRawBufferZero(&server->accessorySetup.state, sizeof server->accessorySetup.state);
    server->accessorySetup.state.lockSetupInfo = lockSetupInfo;

    // Get setup info.
    if (server->platform.setupDisplay) {
        // See HomeKit Accessory Protocol Specification R17
        // Section 4.2.1.1 Generation of Setup Code
        // See HomeKit Accessory Protocol Specification R17
        // Section 5.7.2 M2: Accessory -> iOS Device - `SRP Start Response'
        HAPLogDebug(&logObject, "Generating dynamic setup code.");

        // Generate random setup code.
        HAPAccessorySetupGenerateRandomSetupCode(&server->accessorySetup.state.setupCode);
        server->accessorySetup.state.setupCodeIsAvailable = true;

        // Generation of SRP verifier is delayed until used for the first time.

        // Dynamic setup code needs to be refreshed periodically if it is allowed to change.
        if (!server->accessorySetup.state.lockSetupInfo) {
            HAPPrecondition(!server->accessorySetup.dynamicRefreshTimer);
            err = HAPPlatformTimerRegister(
                    &server->accessorySetup.dynamicRefreshTimer,
                    HAPPlatformClockGetCurrent() + kHAPAccessorySetupInfo_DynamicRefreshInterval,
                    DynamicSetupInfoExpired,
                    server);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                HAPLogError(&logObject, "Not enough resources to allocate timer.");
                HAPFatalError();
            }
        }
    } else {
        HAPLogDebug(&logObject, "Loading static setup code.");

        // Load static setup code (only available if programmable NFC tag is supported).
        if (server->platform.setupNFC) {
            HAPPlatformAccessorySetupLoadSetupCode(
                    server->platform.accessorySetup, &server->accessorySetup.state.setupCode);
            server->accessorySetup.state.setupCodeIsAvailable = true;
        }

        // Load static setup info.
        HAPPlatformAccessorySetupLoadSetupInfo(
                server->platform.accessorySetup, &server->accessorySetup.state.setupInfo);
        server->accessorySetup.state.setupInfoIsAvailable = true;
    }
    HAPAssert(server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable);

    SynchronizeDisplayAndNFC(server);
}

static void DynamicSetupInfoExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->accessorySetup.dynamicRefreshTimer);
    server->accessorySetup.dynamicRefreshTimer = 0;
    HAPPrecondition(!HAPAccessoryServerIsPaired(server));
    HAPPrecondition(!server->accessorySetup.state.lockSetupInfo);
    HAPPrecondition(
            server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable);

    HAPLogInfo(&logObject, "Dynamic setup code expired.");
    ClearSetupInfo(server);

    // Refresh setup code (legacy pairing mode needs explicit request to re-enter pairing mode).
    if (server->platform.setupDisplay) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ false);
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPSetupInfo* _Nullable HAPAccessorySetupInfoGetSetupInfo(HAPAccessoryServer* server, bool restorePrevious) {
    HAPPrecondition(server);

    // Load setup code if allowed.
    if (restorePrevious && !server->accessorySetup.state.keepSetupInfo) {
        HAPLog(&logObject, "Cannot restore setup code from previous pairing attempt.");
        return NULL;
    }

    if (!server->accessorySetup.state.setupInfoIsAvailable && !server->accessorySetup.state.setupCodeIsAvailable) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ true);
    }
    HAPAssert(server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable);

    if (!restorePrevious && server->accessorySetup.state.keepSetupInfo) {
        HAPLog(&logObject, "Discarding setup code from previous pairing attempt.");
        ClearSetupInfo(server);
        PrepareSetupInfo(server, /* lockSetupInfo: */ true);
        HAPAssert(!server->accessorySetup.state.keepSetupInfo);
    }

    // Generate SRP salt and derive SRP verifier for dynamic setup code if it has not yet been computed.
    if (!server->accessorySetup.state.setupInfoIsAvailable) {
        HAPLogDebug(&logObject, "Generating SRP verifier for dynamic setup code.");
        HAPPlatformRandomNumberFill(
                server->accessorySetup.state.setupInfo.salt, sizeof server->accessorySetup.state.setupInfo.salt);
        static const uint8_t srpUserName[] = "Pair-Setup";
        HAP_srp_verifier(
                server->accessorySetup.state.setupInfo.verifier,
                server->accessorySetup.state.setupInfo.salt,
                srpUserName,
                sizeof srpUserName - 1,
                (const uint8_t*) &server->accessorySetup.state.setupCode.stringValue,
                sizeof server->accessorySetup.state.setupCode.stringValue - 1);
        server->accessorySetup.state.setupInfoIsAvailable = true;
    }
    HAPAssert(server->accessorySetup.state.setupInfoIsAvailable);

    return &server->accessorySetup.state.setupInfo;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPAccessorySetupInfoHandleAccessoryServerStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "%s", __func__);

    // Start generating dynamic setup codes if we haven't already
    //  (legacy pairing mode needs explicit request to enter pairing mode).
    if (server->platform.setupDisplay && !HAPAccessoryServerIsPaired(server) &&
        !server->accessorySetup.dynamicRefreshTimer) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ false);
    }
}

void HAPAccessorySetupInfoHandleAccessoryServerStop(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "%s", __func__);

    if (server->accessorySetup.dynamicRefreshTimer) {
        HAPPlatformTimerDeregister(server->accessorySetup.dynamicRefreshTimer);
        server->accessorySetup.dynamicRefreshTimer = 0;
    }
    if (server->accessorySetup.nfcPairingModeTimer) {
        HAPPlatformTimerDeregister(server->accessorySetup.nfcPairingModeTimer);
        server->accessorySetup.nfcPairingModeTimer = 0;
    }
    HAPRawBufferZero(&server->accessorySetup, sizeof server->accessorySetup);
    SynchronizeDisplayAndNFC(server);
}

void HAPAccessorySetupInfoHandleAccessoryServerStateUpdate(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    if (!HAPAccessoryServerIsPaired(server)) {
        // Resume generating dynamic setup codes.
        if (server->platform.setupDisplay) {
            PrepareSetupInfo(server, /* lockSetupInfo: */ false);
        } else {
            SynchronizeDisplayAndNFC(server);
        }
    } else {
        // Exit NFC pairing mode.
        if (server->platform.setupNFC && server->accessorySetup.nfcPairingModeTimer) {
            HAPLogInfo(&logObject, "Pairing complete. Exiting NFC pairing mode.");
            HAPAccessorySetupInfoExitNFCPairingMode(server);
        }
    }
}

void HAPAccessorySetupInfoHandlePairingStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(!HAPAccessoryServerIsPaired(server));

    HAPLogDebug(&logObject, "Pairing attempt started.");

    // Lock setup code so that it cannot change during the pairing attempt.
    if (server->accessorySetup.state.setupInfoIsAvailable || server->accessorySetup.state.setupCodeIsAvailable) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ true);
    }
    HAPAssert(!server->accessorySetup.dynamicRefreshTimer);

    // Inform display that pairing is ongoing.
    if (server->platform.setupDisplay) {
        HAPPlatformAccessorySetupDisplayHandleStartPairing(HAPNonnull(server->platform.setupDisplay));
    }
}

void HAPAccessorySetupInfoHandlePairingStop(HAPAccessoryServer* server, bool keepSetupInfo) {
    HAPPrecondition(server);

    HAPLogDebug(&logObject, "Pairing attempt completed.");

    if (keepSetupInfo) {
        HAPLogInfo(&logObject, "Keeping setup code for next pairing attempt.");
        HAPAssert(server->accessorySetup.state.lockSetupInfo);
        server->accessorySetup.state.keepSetupInfo = true;
    } else {
        // Use a different code for next pairing attempt.
        ClearSetupInfo(server);
        SynchronizeDisplayAndNFC(server);
    }

    // Inform display that pairing has completed.
    if (server->platform.setupDisplay) {
        HAPPlatformAccessorySetupDisplayHandleStopPairing(HAPNonnull(server->platform.setupDisplay));
    }

    // Resume generating dynamic setup codes.
    if (server->platform.setupDisplay && !HAPAccessoryServerIsPaired(server)) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ false);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void HAPAccessorySetupInfoRefreshSetupPayload(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupDisplay);

    if (!server->accessorySetup.dynamicRefreshTimer) {
        HAPLog(&logObject, "Not refreshing setup payload: Current setup payload does not expire.");
        return;
    }

    HAPLogInfo(&logObject, "Refreshing setup payload.");
    ClearSetupInfo(server);
    PrepareSetupInfo(server, /* lockSetupInfo: */ false);
}

//----------------------------------------------------------------------------------------------------------------------

static void CompleteExitingNFCPairingMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupNFC);
    HAPPrecondition(!server->accessorySetup.nfcPairingModeTimer);

    // Clear setup code if it is not used for purposes other than NFC.
    if (!server->accessorySetup.state.lockSetupInfo && !server->accessorySetup.dynamicRefreshTimer) {
        ClearSetupInfo(server);
    }
    SynchronizeDisplayAndNFC(server);
}

static void NFCPairingModeExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* server = context;
    HAPPrecondition(timer == server->accessorySetup.nfcPairingModeTimer);
    server->accessorySetup.nfcPairingModeTimer = 0;

    HAPLogInfo(&logObject, "NFC pairing mode expired.");
    CompleteExitingNFCPairingMode(server);
}

void HAPAccessorySetupInfoEnterNFCPairingMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupNFC);

    HAPError err;

    if (HAPAccessoryServerIsPaired(server)) {
        HAPLog(&logObject, "Not entering NFC pairing mode: Already paired.");
        return;
    }

    // Set up NFC pairing mode timer.
    bool forceSynchronization = false;
    if (server->accessorySetup.nfcPairingModeTimer) {
        HAPLogInfo(&logObject, "Extending ongoing NFC pairing mode.");
        HAPPlatformTimerDeregister(server->accessorySetup.nfcPairingModeTimer);
        server->accessorySetup.nfcPairingModeTimer = 0;
    } else {
        HAPLogInfo(&logObject, "Entering NFC pairing mode.");
        forceSynchronization = true;
    }
    err = HAPPlatformTimerRegister(
            &server->accessorySetup.nfcPairingModeTimer,
            HAPPlatformClockGetCurrent() + kHAPAccessoryServer_NFCPairingModeDuration,
            NFCPairingModeExpired,
            server);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Not enough resources to allocate timer.");
        HAPFatalError();
    }

    // Prepare setup info.
    if (!server->accessorySetup.state.setupInfoIsAvailable && !server->accessorySetup.state.setupCodeIsAvailable) {
        PrepareSetupInfo(server, /* lockSetupInfo: */ false);
    } else if (forceSynchronization) {
        SynchronizeDisplayAndNFC(server);
    } else {
        // Display and NFC are already in sync.
    }
}

void HAPAccessorySetupInfoExitNFCPairingMode(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.setupNFC);

    if (!server->accessorySetup.nfcPairingModeTimer) {
        HAPLog(&logObject, "Exit NFC pairing mode ignored: NFC pairing mode is not active.");
        return;
    }

    HAPLogInfo(&logObject, "Exiting NFC pairing mode.");
    HAPPlatformTimerDeregister(server->accessorySetup.nfcPairingModeTimer);
    server->accessorySetup.nfcPairingModeTimer = 0;
    CompleteExitingNFCPairingMode(server);
}

void HAPAccessorySetupInfoGenerateSetupPayload(
        HAPAccessoryServer* server,
        HAPAccessorySetupSetupPayloadFlags flags,
        HAPSetupCode* _Nullable setupCode,
        bool pairable,
        HAPSetupPayload* payload) {
    HAPPrecondition(payload);
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);
    HAPAccessoryProductData productData;
    HAPRawBufferZero(&productData, sizeof(productData));

    // If we are tearing down the primary accessory won't be available.
    // Retrieve product number if the accessory is available
    if (server->primaryAccessory) {
        // Retrieve Product Data
        HAPAccessoryGetProductData(server->primaryAccessory, &productData);
    }

    if (!pairable) {
        // Generate non-pairable setup payload.
        HAPAccessorySetupGetSetupPayload(
                payload,
                /* setupCode: */ NULL,
                /* setupID: */ NULL,
                /* EUI */ NULL,
                &productData,
                flags,
                server->primaryAccessory->category);
    } else {
        HAPSetupID setupID;
        bool hasSetupID;
        HAPPlatformAccessorySetupLoadSetupID(server->platform.accessorySetup, &hasSetupID, &setupID);
        // If Thread, build with EUI
        if (flags.threadSupported) {
            HAPAssert(!flags.isPaired);
            HAPEui64 eui;
            HAPPlatformReadEui(&eui);
            HAPAccessorySetupGetSetupPayload(
                    payload, setupCode, NULL, &eui, &productData, flags, server->primaryAccessory->category);
        }
        // Otherwise build with a Setup ID
        else if (hasSetupID) {
            HAPAssert(!flags.isPaired);
            HAPAccessorySetupGetSetupPayload(
                    payload, setupCode, &setupID, NULL, &productData, flags, server->primaryAccessory->category);
        } else {
            HAPLog(&logObject, "QR code displays / NFC require a setup ID to be provisioned.");
        }
    }
}
