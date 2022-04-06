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

// This Linux remote example is configured to advertise a set of example buttons to demonstrate a remote that can
// control Apple TV devices.
//
// The keyboard may be used to simulate button presses and releases and to cycle through the controllable targets.
// Updates to the remote state are logged as part of the regular diagnostic log for development purposes.
// The remote may be factory reset by sending the SIGUSR2 signal to the process.
//
// Button press events from remote accessories must not be stored on servers.
// Any servers used to relay button press events from the remote accessory to a HomeKit bridge must be App Transport
// Security compliant.
//
// To enable user interaction following POSIX signals or Buttons are used:
// - Button 1 or signal `SIGUSR1` to clear pairings.
// - Button 2 or signal `SIGUSR2` to trigger a factory reset.
// - Button 3 or Signal `SIGTERM` to trigger pairing mode.
// - Button 4 or Signal `SIGQUIT` unused.
//
// /!\ It is strongly recommended not to modify Remote.c or Siri.c, to simplify the integration of revised
// versions of these files.
//
// App.c (this file):
// - The list of supported buttons needs to be updated to match the buttons available by the hardware.
// - The keyboard handling code needs to be replaced with actual button input handling code.
// - The remote UI needs to be updated when the remote configuration changes.
// - The factory reset functionality needs to be adjusted.
//#if (HAP_SIRI_REMOTE== 1)
//
// HAPPlatformMicrophone.c
// - The microphone code may need adjustment. The reference example relies on the Linux ALSA component and libopus.
//
// Siri for Apple TV control is allowed for remote accessories implemented as a hardware entity only.
//#endif

#include "ApplicationFeatures.h"

#include <stdio.h>
#include <sys/stat.h>

#include "HAP.h"
#include "HAPBase.h"
#include "HAPPlatform+Init.h"
#include "HAPPlatformFileHandle.h"
#include "HAPPlatformKeyValueStore+Init.h"
#if (HAP_SIRI_REMOTE == 1)
#include "HAPPlatformMicrophone+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"
#endif

#include "AccessoryInformationServiceDB.h"
#include "App.h"
#include "AppBase.h"
#include "AppLED.h"
#if (HAP_TESTING == 1)
#include "AppUserInput.h"
#endif
#include "DB.h"
#include "StandardInputHandler.h"
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdate.h"
#include "FirmwareUpdateServiceDB.h"
#endif
#include "Remote.h"
#if (HAP_SIRI_REMOTE == 1)
#include "Siri.h"
#include "SiriDB.h"
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsService.h"
#include "DiagnosticsServiceDB.h"
#include "HAPDiagnostics.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreKey) 0x00)

static HAPAccessory accessory;
static Remote remotes[1];
static HAPPlatformKeyValueStore remoteKeyValueStores[HAPArrayCount(remotes)];
#if (HAP_SIRI_REMOTE == 1)
static HAPPlatformMicrophone remoteMicrophone;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IDs of supported buttons.
 */
HAP_ENUM_BEGIN(uint8_t, ButtonID) {
    kButtonID_01_Menu = 1, kButtonID_02_PlayPause, kButtonID_03_TVHome, kButtonID_04_Select, kButtonID_05_ArrowUp,
    kButtonID_06_ArrowRight, kButtonID_07_ArrowDown, kButtonID_08_ArrowLeft, kButtonID_09_VolumeUp,
    kButtonID_10_VolumeDown,
#if (HAP_SIRI_REMOTE == 1)
    kButtonID_11_Siri,
#endif
    kButtonID_12_Power, kButtonID_13_Generic
}
HAP_ENUM_END(uint8_t, ButtonID);

/**
 * List of supported buttons that are physically available on the remote.
 *
 * - When a button is pressed or released the function RemoteRaiseButtonEvent must be called
 *   to process the button event.
 *
 * - Note: In a real-world setting it may be sufficient to just configure the buttons 1-13, and
 *   to adapt kRemote_MaxButtons accordingly.
 *
 * - Keys 14 to 42 are not mapped.
 */
static const RemoteSupportedButton supportedButtonsRemote0[] = {
    { kButtonID_01_Menu, kRemoteButtonType_Menu },           { kButtonID_02_PlayPause, kRemoteButtonType_PlayPause },
    { kButtonID_03_TVHome, kRemoteButtonType_TVHome },       { kButtonID_04_Select, kRemoteButtonType_Select },
    { kButtonID_05_ArrowUp, kRemoteButtonType_ArrowUp },     { kButtonID_06_ArrowRight, kRemoteButtonType_ArrowRight },
    { kButtonID_07_ArrowDown, kRemoteButtonType_ArrowDown }, { kButtonID_08_ArrowLeft, kRemoteButtonType_ArrowLeft },
    { kButtonID_09_VolumeUp, kRemoteButtonType_VolumeUp },   { kButtonID_10_VolumeDown, kRemoteButtonType_VolumeDown },
#if (HAP_SIRI_REMOTE == 1)
    { kButtonID_11_Siri, kRemoteButtonType_Siri },
#endif
    { kButtonID_12_Power, kRemoteButtonType_Power },         { kButtonID_13_Generic, kRemoteButtonType_Generic }
};
HAP_STATIC_ASSERT(HAPArrayCount(supportedButtonsRemote0) <= kRemote_MaxButtons, supportedButtonsRemote0);

/**
 * Gets a description for a given button type.
 *
 * - The descriptions may be used in the remote UI if no target-specific name has been configured.
 *
 * @param      value                Button type.
 * @return                          Description of the button type.
 */
HAP_RESULT_USE_CHECK
static const char* GetButtonTypeDescription(RemoteButtonType value) {
    switch (value) {
        case kRemoteButtonType_Menu:
            return "Menu";
        case kRemoteButtonType_PlayPause:
            return "Play/Pause";
        case kRemoteButtonType_TVHome:
            return "TV/Home";
        case kRemoteButtonType_Select:
            return "Select";
        case kRemoteButtonType_ArrowUp:
            return "Arrow Up";
        case kRemoteButtonType_ArrowRight:
            return "Arrow Right";
        case kRemoteButtonType_ArrowDown:
            return "Arrow Down";
        case kRemoteButtonType_ArrowLeft:
            return "Arrow Left";
        case kRemoteButtonType_VolumeUp:
            return "Volume Up";
        case kRemoteButtonType_VolumeDown:
            return "Volume Down";
#if (HAP_SIRI_REMOTE == 1)
        case kRemoteButtonType_Siri:
            return "Siri";
#endif
        case kRemoteButtonType_Power:
            return "Power";
        case kRemoteButtonType_Generic:
            return "Generic";
        default: {
            HAPLogError(&kHAPLog_Default, "Invalid button type %u.", value);
        }
            HAPFatalError();
    }
}

/**
 * Prints the list of supported buttons to the diagnostic log.
 *
 * @param      buttons              Supported buttons.
 * @param      numButtons           Number of supported buttons.
 */
static void PrintSupportedButtons(const RemoteSupportedButton* buttons, size_t numButtons) {
    HAPLogInfo(&kHAPLog_Default, "Supported Buttons");
    for (size_t i = 0; i < numButtons; i++) {
        HAPLogInfo(&kHAPLog_Default, "  Button %d", buttons[i].buttonID);
        HAPLogInfo(&kHAPLog_Default, "   - Type: %s", GetButtonTypeDescription(buttons[i].buttonType));
    }
}

typedef struct {
    AppLEDIdentifier identifyPin;
} Device;

/**
 * Global accessory configuration.
 */
/**@{*/
typedef struct {
    HAPTime startTime;
    HAPAccessoryServer* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
    HAPPlatformFileHandleRef stdinFileHandle;
    int stdinFileDescriptor;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    struct {
        uint32_t diagnosticsSelectedModeState;
    } state;
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
    Device device;
    HAPPlatformTimerRef identifyTimer;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;
/**@}*/

#if (HAVE_ACCESSORY_REACHABILITY == 1)
static HAPAccessoryReachabilityConfiguration accessoryReachabilityConfig = {
    .sleepIntervalInMs = kAccessorySleepInterval,
};
#endif
//----------------------------------------------------------------------------------------------------------------------

static void PrintTargetButtonsCallback(
        void* _Nullable context HAP_UNUSED,
        Remote* remote HAP_UNUSED,
        const RemoteTargetConfiguration* targetConfiguration HAP_UNUSED,
        const RemoteButtonConfiguration* buttonConfiguration,
        bool* shouldContinue) {
    *shouldContinue = true;

    HAPLogInfo(&kHAPLog_Default, "  Button %u", buttonConfiguration->buttonID);
    if (buttonConfiguration->buttonType.isDefined) {
        HAPLogInfo(&kHAPLog_Default, "   - Type: %s", GetButtonTypeDescription(buttonConfiguration->buttonType.value));
    }
    if (buttonConfiguration->buttonName.isDefined) {
        HAPLogInfo(&kHAPLog_Default, "   - Name: %s", buttonConfiguration->buttonName.bytes);
    }
}

static void PrintTargetsCallback(
        void* _Nullable context HAP_UNUSED,
        Remote* remote,
        const RemoteTargetConfiguration* targetConfiguration,
        bool* shouldContinue) {
    HAPPrecondition(remote);
    HAPPrecondition(targetConfiguration);

    *shouldContinue = true;

    HAPLogInfo(&kHAPLog_Default, "Target: %lu", (unsigned long) targetConfiguration->targetIdentifier);
    if (targetConfiguration->targetCategory) {
        HAPLogInfo(&kHAPLog_Default, "  Category: %u", targetConfiguration->targetCategory);
    }
    if (targetConfiguration->targetName.isDefined) {
        HAPLogInfo(&kHAPLog_Default, "  Name:     %s", targetConfiguration->targetName.bytes);
    }

    RemoteEnumerateButtons(
            remote,
            targetConfiguration->targetIdentifier,
            PrintTargetButtonsCallback,
            /* context: */ NULL);
}

static void PrintTargets(Remote* remote) {
    HAPPrecondition(remote);

    HAPLogInfo(&kHAPLog_Default, "Targets configured for remote %p", (const void*) remote);
    RemoteEnumerateTargets(remote, PrintTargetsCallback, /* context: */ NULL);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Control duration of Identify indication.
 */
static void IdentifyTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    HAPPrecondition(!context);
    HAPPrecondition(timer == accessoryConfiguration.identifyTimer);

    accessoryConfiguration.identifyTimer = 0;

    AppLEDSet(accessoryConfiguration.device.identifyPin, false);
}

/**
 * Performs the Identify routine.
 */
static void DeviceIdentify(void) {
    HAPError err;

    if (accessoryConfiguration.identifyTimer) {
        HAPPlatformTimerDeregister(accessoryConfiguration.identifyTimer);
        accessoryConfiguration.identifyTimer = 0;
    }
    err = HAPPlatformTimerRegister(
            &accessoryConfiguration.identifyTimer,
            HAPPlatformClockGetCurrent() + 3 * HAPSecond,
            IdentifyTimerExpired,
            NULL);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
    }

    AppLEDSet(accessoryConfiguration.device.identifyPin, true);
}

/**
 * Identify routine. Used to locate the accessory.
 */
HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);
    /* VENDOR-TODO: Trigger LED/sound notification to identify accessory */
    DeviceIdentify();
    return kHAPError_None;
}

#if (HAP_TESTING == 1)

//----------------------------------------------------------------------------------------------------------------------

void SwitchToNextTarget(void) {
    RemoteSwitchToNextTarget(&remotes[0], /* context: */ NULL);
}

/**
 * Global user input handler.
 *
 * Button and signal mapping to kAppUserInputIdentifier can be found at Applications/Common/AppUserInput.h
 */
static void HandleUserInputEventCallback(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPAssert(contextSize == sizeof(AppUserInputEvent));

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    AppUserInputEvent buttonEvent = *((AppUserInputEvent*) context);
    switch (buttonEvent.id) {
        case kAppUserInputIdentifier_4: { // SIGQUIT or Button 4
            break;
        }
        default: {
            break;
        }
    }
}

#endif

/**
 * Unconfigure platform specific IO.
 */
static void UnconfigureIO(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    AppLEDDeinit();
}

/**
 * Configure platform specific IO.
 */
static void ConfigureIO(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

#if (HAP_TESTING == 1)
    // Setup user input event handler
    AppUserInputRegisterCallback(HandleUserInputEventCallback);
#endif

    // Assign LEDs.
    accessoryConfiguration.device.identifyPin = kAppLEDIdentifier_2;

    // Initialize LED driver
    AppLEDInit();
}

//----------------------------------------------------------------------------------------------------------------------

#if (HAP_SIRI_REMOTE == 1)
/**
 * Used to get the type of Siri Input used by the accessory.
 *
 * @param       server                 Accessory server.
 * @param[out]  value                  Representation of the Siri Input type.
 * @param       context                The context parameter given to the HAPAccessoryServerCreate
 * function.
 *
 * @return kHAPError_None             If successful.
 */
HAP_RESULT_USE_CHECK
HAPError GetSiriInputType(HAPAccessoryServer* server, uint8_t* value, void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(value);

    // The only supported input type is push button triggered AppleTV.
    *value = (uint8_t) kHAPCharacteristicValue_SiriInputType_PushButtonTriggeredAppleTV;

    HAPLog(&kHAPLog_Default, "%s: %u", __func__, *value);

    return kHAPError_None;
}
#endif

//----------------------------------------------------------------------------------------------------------------------

static void PrintButtonConfiguration(void) {
    HAPLogInfo(&kHAPLog_Default, "Button Configuration");
    HAPLogInfo(&kHAPLog_Default, "Action                      Down Up");
    HAPLogInfo(&kHAPLog_Default, "===================================");
    HAPLogInfo(&kHAPLog_Default, "Toggle Identifier:          m");
    HAPLogInfo(&kHAPLog_Default, "Set target to non-HomeKit:  0");
#if (HAP_SIRI_REMOTE == 1)
    HAPLogInfo(&kHAPLog_Default, "Siri:                       1    2");
#endif
    HAPLogInfo(&kHAPLog_Default, "Menu:                       q    w");
    HAPLogInfo(&kHAPLog_Default, "Play / Pause:               e    r");
    HAPLogInfo(&kHAPLog_Default, "TV / Home:                  t    y");
    HAPLogInfo(&kHAPLog_Default, "Select:                     u    i");
    HAPLogInfo(&kHAPLog_Default, "Arrow Up:                   o    p");
    HAPLogInfo(&kHAPLog_Default, "Arrow Right:                a    s");
    HAPLogInfo(&kHAPLog_Default, "Arrow Down:                 d    f");
    HAPLogInfo(&kHAPLog_Default, "Arrow Left:                 g    h");
    HAPLogInfo(&kHAPLog_Default, "Volume Up:                  j    k");
    HAPLogInfo(&kHAPLog_Default, "Volume Down:                l    ;");
    HAPLogInfo(&kHAPLog_Default, "Power:                      c    v");
    HAPLogInfo(&kHAPLog_Default, "Generic:                    b    n");
}

//----------------------------------------------------------------------------------------------------------------------
#if (HAP_TESTING == 1)
static void InputHandleCallback(char singleChar) {
    size_t remoteIndex = 0;
    Remote* targetRemote = &remotes[remoteIndex];
    HAPLogDebug(
            &kHAPLog_Default,
            "%s: Read action %c for remote #%u (%p).",
            __func__,
            singleChar,
            (uint32_t)(remoteIndex + 1),
            (const void*) targetRemote);

    HAPTime ts = HAPPlatformClockGetCurrent();

    switch (singleChar) {
        // Toggle Identifier
        case 'm': {
            RemoteSwitchToNextTarget(targetRemote, /* context: */ NULL);
            break;
        }

        // Set the identifier to non-HomeKit (0).
        case '0': {
            RemoteSetActiveTargetIdentifier(targetRemote, kRemoteTargetIdentifier_NonHomeKit, /* context: */ NULL);
            break;
        }

#if (HAP_SIRI_REMOTE == 1)
        // Button: AudioInput
        case '1': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_11_Siri, kRemoteButtonState_Down, ts);
            break;
        }

        case '2': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_11_Siri, kRemoteButtonState_Up, ts);
            break;
        }

#endif
        // Button: Menu
        case 'q': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_01_Menu, kRemoteButtonState_Down, ts);
            break;
        }

        case 'w': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_01_Menu, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: Play/Pause
        case 'e': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_02_PlayPause, kRemoteButtonState_Down, ts);
            break;
        }

        case 'r': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_02_PlayPause, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: TV/Home
        case 't': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_03_TVHome, kRemoteButtonState_Down, ts);
            break;
        }

        case 'y': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_03_TVHome, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: Select
        case 'u': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_04_Select, kRemoteButtonState_Down, ts);
            break;
        }

        case 'i': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_04_Select, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: ArrowUp
        case 'o': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_05_ArrowUp, kRemoteButtonState_Down, ts);
            break;
        }

        case 'p': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_05_ArrowUp, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: ArrowRight
        case 'a': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_06_ArrowRight, kRemoteButtonState_Down, ts);
            break;
        }

        case 's': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_06_ArrowRight, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: ArrowDown
        case 'd': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_07_ArrowDown, kRemoteButtonState_Down, ts);
            break;
        }

        case 'f': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_07_ArrowDown, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: ArrowLeft
        case 'g': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_08_ArrowLeft, kRemoteButtonState_Down, ts);
            break;
        }

        case 'h': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_08_ArrowLeft, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: VolumeUp
        case 'j': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_09_VolumeUp, kRemoteButtonState_Down, ts);
            break;
        }

        case 'k': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_09_VolumeUp, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: VolumeDown
        case 'l': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_10_VolumeDown, kRemoteButtonState_Down, ts);
            break;
        }

        case ';': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_10_VolumeDown, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: Power
        case 'c': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_12_Power, kRemoteButtonState_Down, ts);
            break;
        }

        case 'v': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_12_Power, kRemoteButtonState_Up, ts);
            break;
        }

        // Button: Generic
        case 'b': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_13_Generic, kRemoteButtonState_Down, ts);
            break;
        }

        case 'n': {
            RemoteRaiseButtonEvent(targetRemote, kButtonID_13_Generic, kRemoteButtonState_Up, ts);
            break;
        }
    }
}
#endif // HAP_TESTING

//----------------------------------------------------------------------------------------------------------------------

#if (HAP_APP_USES_HDS == 1)
#if (HAP_APP_USES_HDS_STREAM == 1)
static HAPStreamDataStreamProtocol streamDataStreamProtocol = {
    .base = &kHAPStreamDataStreamProtocol_Base,
    .numDataStreams = kApp_NumDataStreams,
    .applicationProtocols = (HAPStreamApplicationProtocol* const[]) { &streamProtocolUARP, NULL }
};
#endif // HAP_APP_USES_HDS_STREAM

static HAPTargetControlDataStreamProtocol targetControlDataStreamProtocol = {
    .base = &kHAPTargetControlDataStreamProtocol_Base,
    .callbacks = { .handleIdentifierUpdate = RemoteHandleTargetControlIdentifierUpdate }
};

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
static HAPDataSendStreamProtocolStreamAvailableCallbacks dataSendDataStreamProtocolAvailableCallbacks[] = {
    { .type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot,
      .handleStreamAvailable = HAPDiagnosticsHandleDataSendStreamAvailable }
};
static HAPDataSendDataStreamProtocolListener dataSendDataStreamProtocolListeners[kApp_NumDataStreams];
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

static HAPDataSendDataStreamProtocolContext dataSendDataStreamProtocolContexts[kApp_NumDataStreams];
static HAPDataSendDataStreamProtocol
        dataSendDataStreamProtocol = { .base = &kHAPDataSendDataStreamProtocol_Base,
                                       .storage = { .numDataStreams = kApp_NumDataStreams,
                                                    .protocolContexts = dataSendDataStreamProtocolContexts,
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                    .listeners = dataSendDataStreamProtocolListeners,
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
                                       },
                                       .callbacks = {
#if (HAP_SIRI_REMOTE == 1)
                                               .handleAccept = SiriHandleDataSendAccept,
                                               .handleInvalidate = SiriHandleDataSendInvalidate,
#endif // (HAP_SIRI_REMOTE == 1)
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                               .numStreamAvailableCallbacks = 1,
                                               .streamAvailableCallbacks = dataSendDataStreamProtocolAvailableCallbacks,
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
                                       },
};

static HAPDataStreamDescriptor dataStreamDescriptors[kApp_NumDataStreams];
static HAPDataStreamDispatcher dataStreamDispatcher;
const HAPDataStreamDispatcherStorage dataStreamDispatcherStorage = {
    .numDataStreams = kApp_NumDataStreams,
    .dataStreamDescriptors = dataStreamDescriptors,
    .dataStreamProtocols =
            (HAPDataStreamProtocol* const[]) {
                    &targetControlDataStreamProtocol,
                    &dataSendDataStreamProtocol,
#if (HAP_APP_USES_HDS_STREAM == 1)
                    &streamDataStreamProtocol,
#endif
                    NULL,
            },
};
#endif // HAP_APP_USES_HDS

//----------------------------------------------------------------------------------------------------------------------
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
/**
 * Initialize a default accessory state that validates with the device configuration.
 */
static void SetupDefaultAccessoryState(void) {
    HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
    accessoryConfiguration.state.diagnosticsSelectedModeState = kHAPCharacteristicValue_SelectedDiagnosticsModes_None;
}

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        SetupDefaultAccessoryState();
    }
}

/**
 * Save the accessory state to persistent memory
 */
static void SaveAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the Remote and Siri services.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Remotes,
                                  .name = "Acme Remote",
                                  .productData = "03d8a775e3644573",
                                  .manufacturer = "Acme",
                                  .model = "Remote1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &targetControlManagementService,
                                                                            &targetControlService,
#if (HAP_APP_USES_HDS == 1)
                                                                            &dataStreamTransportManagementService,
#endif
#if (HAP_SIRI_REMOTE == 1)
                                                                            &siriAudioStreamService,
                                                                            &siriService,
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                                                            &accessoryRuntimeInformationService,
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                                                                            &firmwareUpdateService,
#endif
#if (HAP_TESTING == 1)
                                                                            &debugService,
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                                                            &diagnosticsService,
#endif
                                                                            NULL },
#if (HAP_APP_USES_HDS == 1)
                                  .dataStream.delegate = { .callbacks = &kHAPDataStreamDispatcher_DataStreamCallbacks,
                                                           .context = &dataStreamDispatcher },
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                                  .reachabilityConfiguration = &accessoryReachabilityConfig,
#endif
                                  .callbacks = {
                                          .identify = IdentifyAccessory,
#if (HAVE_FIRMWARE_UPDATE == 1)
                                          .firmwareUpdate = { .getAccessoryState = FirmwareUpdateGetAccessoryState },
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                                          .diagnosticsConfig = { .getDiagnosticsConfig =
                                                                         GetAccessoryDiagnosticsConfig },
#endif
#if (HAP_SIRI_REMOTE == 1)
                                          .siri = { .getSiriInputType = GetSiriInputType },
#endif
                                  } };

//----------------------------------------------------------------------------------------------------------------------

static void HandleActiveChange(Remote* remote, void* _Nullable context HAP_UNUSED) {
    if (RemoteIsActive(remote)) {
        RemoteTargetIdentifier targetIdentifier = RemoteGetActiveTargetIdentifier(remote);
        RemoteTargetConfiguration targetConfiguration;
        bool found;
        RemoteGetTargetConfiguration(remote, targetIdentifier, &targetConfiguration, &found);

        HAPLog(&kHAPLog_Default,
               "Remote %p active. Target \"%s\" (%lu), category %u.",
               (const void*) remote,
               targetConfiguration.targetName.isDefined ? targetConfiguration.targetName.bytes : "Unknown",
               (unsigned long) targetConfiguration.targetIdentifier,
               targetConfiguration.targetCategory);
    } else {
        HAPLog(&kHAPLog_Default, "%s: Remote %p inactive.", __func__, (const void*) remote);
    }
}

static void HandleConfigurationChange(Remote* remote, void* _Nullable context HAP_UNUSED) {
    HAPLog(&kHAPLog_Default, "Remote %p configured targets changed.", (const void*) remote);
    PrintTargets(remote);
}

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
/**
 * Handle write request to the 'Selected Diagnostics Mode' characteristic of the Diagnostics service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSelectedDiagnosticsModesWrite(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicWriteRequest* request,
        uint32_t value,
        void* _Nullable context HAP_UNUSED) {
    return HandleSelectedDiagnosticsModesWriteHelper(
            server,
            request,
            value,
            context,
            &accessoryConfiguration.state.diagnosticsSelectedModeState,
            &SaveAccessoryState);
}
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)

//----------------------------------------------------------------------------------------------------------------------

void AppCreate(HAPAccessoryServer* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    accessoryConfiguration.startTime = HAPPlatformClockGetCurrent();

    ConfigureIO();

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    LoadAccessoryState();
#endif

    PrintButtonConfiguration();
#if (HAP_TESTING == 1)
    StdinPipeCreate(InputHandleCallback);
#endif

    static RemoteCallbacks remoteCallbacks = { .handleActiveChange = HandleActiveChange,
                                               .handleConfigurationChange = HandleConfigurationChange };

#if (HAP_SIRI_REMOTE == 1)
    static HAPPlatformMicrophoneStream microphoneStreams[HAPArrayCount(remotes)];
    HAPPlatformMicrophoneCreate(
            &remoteMicrophone,
            &(const HAPPlatformMicrophoneOptions) { .audioInputDevice = "plug:default:1",
                                                    .microphoneStreams = microphoneStreams,
                                                    .numMicrophoneStreams = HAPArrayCount(microphoneStreams) });

#endif
    // Remote 1
    {
        HAPPlatformKeyValueStoreCreate(
                &remoteKeyValueStores[0], &(const HAPPlatformKeyValueStoreOptions) { .rootDirectory = ".Remote1" });

        static RemoteOptions remoteOptions;
        HAPRawBufferZero(&remoteOptions, sizeof remoteOptions);

        remoteOptions.type = kRemoteType_Hardware;
        remoteOptions.server = server;
        remoteOptions.accessory = &accessory;
        remoteOptions.supportedButtons = supportedButtonsRemote0;
        remoteOptions.numSupportedButtons = HAPArrayCount(supportedButtonsRemote0);
        remoteOptions.remoteKeyValueStore = &remoteKeyValueStores[0];
#if (HAP_SIRI_REMOTE == 1)
        remoteOptions.remoteMicrophone = &remoteMicrophone;
#endif
        remoteOptions.remoteCallbacks = &remoteCallbacks;

        RemoteCreate(&remotes[0], &remoteOptions, /* context: */ NULL);

        PrintSupportedButtons(supportedButtonsRemote0, HAPArrayCount(supportedButtonsRemote0));
        PrintTargets(&remotes[0]);
    }

#if (HAP_APP_USES_HDS == 1)
    // Initialize HomeKit Data Stream dispatcher.
    HAPDataStreamDispatcherCreate(
            server,
            &dataStreamDispatcher,
            &(const HAPDataStreamDispatcherOptions) { .storage = &dataStreamDispatcherStorage });
#endif

#if (HAVE_FIRMWARE_UPDATE == 1)
    UARPInitialize(accessoryConfiguration.server, &accessory);

    FirmwareUpdateOptions fwupOptions = { 0 };
#if (HAP_TESTING == 1)
    fwupOptions.persistStaging = server->firmwareUpdate.persistStaging;
#endif
    FirmwareUpdateInitialize(
            accessoryConfiguration.server, &accessory, accessoryConfiguration.keyValueStore, &fwupOptions);
#endif // HAVE_FIRMWARE_UPDATE
}

void AppRelease(HAPAccessoryServer* _Nonnull server HAP_UNUSED, void* _Nullable context) {
#if (HAP_SIRI_REMOTE == 1)
    // Microphone.
    HAPPlatformMicrophoneRelease(&remoteMicrophone);
#endif

    // Release remote.
    RemoteRelease(&remotes[0], context);

#if (HAP_TESTING == 1)
    StdinPipeRelease();
#endif

    UnconfigureIO();
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
#if (HAVE_FIRMWARE_UPDATE == 1)
    FirmwareUpdateStart(accessoryConfiguration.server, &accessory);
#endif // HAVE_FIRMWARE_UPDATE
}

//----------------------------------------------------------------------------------------------------------------------

void AppHandleAccessoryServerStateUpdate(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
}

void AppHandleFactoryReset(HAPAccessoryServer* server HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    RemoteRestoreFactorySettings(&remoteKeyValueStores[0]);
}

void AppHandlePairingStateChange(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPPairingStateChange state,
        void* _Nullable context) {
    switch (state) {
        case kHAPPairingStateChange_Unpaired: {
            RemoteHandleUnpair(&remotes[0], context);
            PrintTargets(&remotes[0]);
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
            DiagnosticsHandlePairingStateChange(
                    state, &accessoryConfiguration.state.diagnosticsSelectedModeState, &SaveAccessoryState);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
            break;
        }
        default: {
            break;
        }
    }
}

const HAPAccessory* AppGetAccessoryInfo(void) {
    return &accessory;
}

const HAPAccessory* _Nonnull const* _Nullable AppGetBridgeAccessoryInfo(void) {
    return NULL;
}

void AccessoryServerHandleSessionAccept(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context) {
    RemoteHandleSessionAccept(server, session, context);
}

void AccessoryServerHandleSessionInvalidate(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context) {
    RemoteHandleSessionInvalidate(server, session, context);
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions HAP_UNUSED,
        HAPPlatform* hapPlatform HAP_UNUSED,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks) {
    hapAccessoryServerCallbacks->handleSessionAccept = AccessoryServerHandleSessionAccept;
    hapAccessoryServerCallbacks->handleSessionInvalidate = AccessoryServerHandleSessionInvalidate;
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    InitializeDiagnostics(&accessoryConfiguration.state.diagnosticsSelectedModeState, &accessory, hapPlatform);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}

void AppDeinitialize(void) {
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    DeinitializeDiagnostics(&accessory);
#endif // (HAVE_DIAGNOSTICS_SERVICE == 1)
}