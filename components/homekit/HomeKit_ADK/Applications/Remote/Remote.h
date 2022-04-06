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

#ifndef REMOTE_H
#define REMOTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ApplicationFeatures.h"

#include "HAP.h"

#if (HAP_SIRI_REMOTE == 1)
#include "HAPPlatformMicrophone.h"
#endif

#if (HAP_SIRI_REMOTE == 1)
#include "Siri.h"
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * The maximum number of supported buttons per remote.
 *
 * Affects the size of the IP buffers and the Remote.
 */
#define kRemote_MaxButtons ((size_t) 13)

/**
 * The maximum length of an identifier.
 *
 * Affects the size of the IP buffers and the Remote.
 */
#define kRemote_MaxIdentifierBytes ((size_t) 64 * 4)

/**
 * The maximum number of button events queued per HomeKit session.
 *
 * Affects the size of the Remote.
 */
#define kRemote_MaxQueuedButtonEvents ((size_t) 10)

/**
 * The maximum number of Targets.
 *
 * Affects the size of the IP buffers and the Remote.
 */
#define kRemote_MaxTargets ((size_t) 32)

/**
 * The maximum number of sessions.
 *
 * Affects the size of the Remote.
 */
#define kRemote_MaxSessions ((size_t) kRemote_MaxTargets)

/**
 * Remote Control Button ID.
 */
typedef uint8_t RemoteButtonID;
HAP_STATIC_ASSERT(sizeof(RemoteButtonID) == sizeof(uint8_t), RemoteButtonID);

/*
 * Remote button state.
 */
HAP_ENUM_BEGIN(uint8_t, RemoteButtonState) { kRemoteButtonState_Up = 0,
                                             kRemoteButtonState_Down = 1 } HAP_ENUM_END(uint8_t, RemoteButtonState);
HAP_STATIC_ASSERT(
        kRemoteButtonState_Up == (RemoteButtonState) kHAPCharacteristicValue_ButtonEvent_ButtonState_Up,
        kRemoteButtonState_Up);
HAP_STATIC_ASSERT(
        kRemoteButtonState_Down == (RemoteButtonState) kHAPCharacteristicValue_ButtonEvent_ButtonState_Down,
        kRemoteButtonState_Down);

/**
 * Remote type
 */
HAP_ENUM_BEGIN(uint8_t, RemoteType) {
    kRemoteType_Software = 1,
    kRemoteType_Hardware = 2,
} HAP_ENUM_END(uint8_t, RemoteType);

/*
 * Remote button type.
 */
HAP_ENUM_BEGIN(uint8_t, RemoteButtonType) {
    kRemoteButtonType_Menu = 1, kRemoteButtonType_PlayPause = 2, kRemoteButtonType_TVHome = 3,
    kRemoteButtonType_Select = 4, kRemoteButtonType_ArrowUp = 5, kRemoteButtonType_ArrowRight = 6,
    kRemoteButtonType_ArrowDown = 7, kRemoteButtonType_ArrowLeft = 8, kRemoteButtonType_VolumeUp = 9,
    kRemoteButtonType_VolumeDown = 10,
#if (HAP_SIRI_REMOTE == 1)
    kRemoteButtonType_Siri = 11,
#endif
    kRemoteButtonType_Power = 12, kRemoteButtonType_Generic = 13
}
HAP_ENUM_END(uint8_t, RemoteButtonType);

/**
 * Target Identifier.
 */
typedef uint32_t RemoteTargetIdentifier;

/**
 * Target Identifier when a non-HomeKit identity is being controlled.
 */
#define kRemoteTargetIdentifier_NonHomeKit ((RemoteTargetIdentifier) 0)

/**
 * Remote.
 *
 * The size of this struct is affected by:
 *  - kRemote_MaxButtons.
 *  - kRemote_MaxIdentifierBytes.
 *  - kRemote_MaxQueuedButtonEvents.
 *  - kRemote_MaxTargets.
 *  - kRemote_MaxSessions.
#if (HAP_SIRI_REMOTE== 1)
 *  - the Microphone PAL.
 *  - the Siri Data Stream context.
#endif
 *
 * Adapt if any of the values have changed. The architecture and compiler used can also play a role.
 */
struct _Remote;
typedef struct _Remote Remote;

/**
 * Supported button configuration.
 */
typedef struct {
    RemoteButtonID buttonID;
    RemoteButtonType buttonType;
} RemoteSupportedButton;

/**
 * Button configuration.
 */
typedef struct {
    RemoteButtonID buttonID;
    struct {
        bool isDefined;
        RemoteButtonType value;
    } buttonType;
    struct {
        bool isDefined;
        char bytes[kRemote_MaxIdentifierBytes + 1]; /* Null terminated string */
    } buttonName;
} RemoteButtonConfiguration;

/**
 * Target configuration.
 */
typedef struct {
    /** Target identifier */
    RemoteTargetIdentifier targetIdentifier;
    /** Target category */
    HAPAccessoryCategory targetCategory;
    /** Target name */
    struct {
        bool isDefined;
        char bytes[kRemote_MaxIdentifierBytes + 1]; /* Null terminated string */
    } targetName;
} RemoteTargetConfiguration;

/**
 * Callbacks to handle configuration changes in the remote.
 */
typedef struct {
    /**
     * Callback that is invoked when the remote's Active state changes.
     *
     * - Use the RemoteIsActive function to determine whether the remote is currently active.
     *
     * - While the remote is inactive, no button events may be reported to the remote
     *   and button input hardware may be put into power saving mode.
     *
     * @param      remote               Remote.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleActiveChange)(Remote* remote, void* _Nullable context);

    /**
     * Callback that is invoked when the remote's target configuration changes.
     *
     * - Use the RemoteEnumerateTargets function to receive the current target configuration.
     *   The remote UI should reflect the current configuration.
     *
     * - Use the RemoteGetActiveTargetIdentifier function to query for the currently selected target.
     *   Note that an active target may become deactivated due to configuration changes.
     *
     * @param      remote               Remote.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleConfigurationChange)(Remote* remote, void* _Nullable context);

    /**
     * Callback that is invoked to check if a Remote is reachable.
     *
     * Only configure this callback if a Remote is bridged.
     *
     * @param      remote               Remote.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     *
     * @return true                     If the Remote is reachable.
     * @return false                    Otherwise.
     */
    bool (*_Nullable isReachable)(Remote* remote, void* _Nullable context);
} RemoteCallbacks;

/**
 * Remote Configuration Options
 */
typedef struct {
    /** Type of the Remote. */
    RemoteType type;

    /** Accessory server. */
    HAPAccessoryServer* server;

    /** Accessory that hosts the remote. Note that only one remote may be set up per accessory at this time. */
    const HAPAccessory* accessory;

    /** List of supported buttons that are physically available. */
    const RemoteSupportedButton* supportedButtons;

    /** Number of supported buttons. */
    size_t numSupportedButtons;

    /**
     * Key-value store that should be used to store the remote's target configuration.
     *
     * - /!\ A dedicated key-value store must be provided for each remote.
     *   The accessory server's key-value store must not be supplied here.
     */
    HAPPlatformKeyValueStoreRef remoteKeyValueStore;

#if (HAP_SIRI_REMOTE == 1)
    /**
     * Microphone for the remote.
     *
     * Siri for Apple TV control is allowed for remote accessories implemented as a hardware entity only.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 11.115 Target Control Supported Configuration
     */
    HAPPlatformMicrophoneRef _Nullable remoteMicrophone;

#endif
    /** Remote callbacks. */
    const RemoteCallbacks* _Nullable remoteCallbacks;
} RemoteOptions;

/**
 * HAP session information associated to the Remote.
 */
/**@{*/
typedef struct {
    HAPTime timestamp;
    RemoteTargetIdentifier activeIdentifier;
    RemoteButtonID buttonID;
    RemoteButtonState buttonState;
} ButtonEvent;

typedef struct {
    ButtonEvent queuedEvents[kRemote_MaxQueuedButtonEvents];
    size_t numEvents; // Number of events currently in queue
    size_t tailIndex; // Index of next event to be enqueued, if not full
    size_t headIndex; // Index of next event to be dequeued, if not empty
    // headIndex == tailIndex => (numEvents == 0) || (numEvents == kRemote_MaxQueuedButtonEvents)
} QueuedButtonEvents;

typedef struct {
    bool inUse : 1;
#if (HAP_SIRI_REMOTE == 1)
    bool siriChannelActive : 1;
#endif
    bool buttonEventsSubscribed : 1;
    bool targetControlListControlPointWritten : 1;
#if (HAP_SIRI_REMOTE == 1)
    HAPSiriDataStreamContext siriDataStreamContext;
#endif
    const HAPSession* session;
    QueuedButtonEvents queuedButtonEvents;
} RemoteSession;
/**@}*/

/**
 * Remote Control.
 */
/**@{*/
typedef struct _Remote {
    Remote* _Nullable nextRemote;
    struct {
        const RemoteSupportedButton* buttons;
        size_t numButtons;
    } supportedButtons;
    struct {
        RemoteTargetIdentifier activeIdentifier;
    } state;
    HAPCharacteristicValue_TargetControl_Type type;
    const RemoteCallbacks* callbacks;
    HAPPlatformKeyValueStoreRef keyValueStore;
#if (HAP_SIRI_REMOTE == 1)
    HAPPlatformMicrophoneRef _Nullable microphone;
#endif
    RemoteSession registeredSessions[kRemote_MaxSessions];
    RemoteSession* _Nullable activeSessionInfo;
#if (HAP_SIRI_REMOTE == 1)
    RemoteSession* _Nullable voiceSession;
#endif
    const HAPAccessory* accessory;
    HAPAccessoryServer* server;
#if (HAP_SIRI_REMOTE == 1)
    HAPSiriAudioContext siriAudioContext;
#endif
} Remote;
/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * Create the remote.
 *
 * @param      remote               Remote.
 * @param      options              Remote options.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void RemoteCreate(Remote* remote, const RemoteOptions* options, void* _Nullable context);

/**
 * Releases the remote.
 *
 * @param      remote               Remote.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void RemoteRelease(Remote* remote, void* _Nullable context);

/**
 * Unpair handling when the HomeKit accessory has been unpaired.
 *
 * @param      remote               Remote.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void RemoteHandleUnpair(Remote* remote, void* _Nullable context);

/**
 * Restore the factory settings on the key value store associated to the remote.
 * - The remote must be released before restoring factory settings!
 *
 * @param      keyValueStore        Key value store associated to remote.
 */
void RemoteRestoreFactorySettings(HAPPlatformKeyValueStoreRef keyValueStore);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Target enumeration callback.
 *
 * @param      context              Context.
 * @param      remote               Remote.
 * @param      targetConfig         Target configuration.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*RemoteEnumerateTargetsCallback)(
        void* _Nullable context,
        Remote* remote,
        const RemoteTargetConfiguration* targetConfig,
        bool* shouldContinue);

/**
 * Enumerate the targets of a remote.
 *
 * @param      remote               Remote.
 * @param      callback             Callback to be called with each remote.
 * @param      context              Context.
 */
void RemoteEnumerateTargets(Remote* remote, RemoteEnumerateTargetsCallback callback, void* _Nullable context);

/**
 * Button enumeration callback.
 *
 * @param      context              Context.
 * @param      remote               Remote.
 * @param      targetConfiguration  Target Configuration.
 * @param      buttonConfiguration  Button Configuration.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*RemoteEnumerateTargetButtonsCallback)(
        void* _Nullable context,
        Remote* remote,
        const RemoteTargetConfiguration* targetConfiguration,
        const RemoteButtonConfiguration* buttonConfiguration,
        bool* shouldContinue);

/**
 * Enumerate the buttons of a target of a remote.
 *
 * @param      remote               Remote.
 * @param      targetIdentifier     Target identifier of the target to enumerate.
 * @param      callback             Callback to be called for each button.
 * @param      context              Context.
 */
void RemoteEnumerateButtons(
        Remote* remote,
        RemoteTargetIdentifier targetIdentifier,
        RemoteEnumerateTargetButtonsCallback callback,
        void* _Nullable context);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns whether or not the remote is currently active and can handle button events.
 *
 * @param      remote               Remote.
 *
 * @return true                     If the remote is currently active.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool RemoteIsActive(Remote* remote);

/**
 * Get the active target identifier of the remote.
 *
 * @param      remote               Remote.
 *
 * @return                          Identifier of the currently controlled target. May return
 *                                  kRemoteTargetIdentifier_NonHomeKit if no HomeKit target is being controlled.
 */
RemoteTargetIdentifier RemoteGetActiveTargetIdentifier(Remote* remote);

/**
 * Sets the target identifier of a remote.
 *
 * May be set to kRemoteTargetIdentifier_NonHomeKit if the remote is currently controlling a non-HomeKit entity.
 *
 * @param      remote               Remote.
 * @param      targetIdentifier     Identifier to be set.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void RemoteSetActiveTargetIdentifier(Remote* remote, RemoteTargetIdentifier targetIdentifier, void* _Nullable context);

/**
 * Switches to the next available configured target.
 *
 * @param      remote               Remote.
 */
void RemoteSwitchToNextTarget(Remote* remote, void* _Nullable context);

/**
 * Reads the target configuration.
 *
 * @param      remote               Remote.
 * @param      targetIdentifier     Target identifier of the target configuration to be read.
 * @param[out] targetConfiguration  The target configuration buffer.
 * @param[out] found                Successful.
 */
void RemoteGetTargetConfiguration(
        Remote* remote,
        RemoteTargetIdentifier targetIdentifier,
        RemoteTargetConfiguration* targetConfiguration,
        bool* found);

/**
 * Raises a button event to subscribed accessories.
 *
 * - If button events are raised on a button that is currently configured as an Audio Input button a voice audio stream
 *   is automatically opened to transmit Siri data.
 * - Button events are dropped while the remote is not active.
 *
 * @param      remote               Remote.
 * @param      buttonID             Button ID.
 * @param      buttonState          Button state.
 * @param      timestamp            Timestamp.
 */
void RemoteRaiseButtonEvent(Remote* remote, RemoteButtonID buttonID, RemoteButtonState buttonState, HAPTime timestamp);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Characteristics handled by the Remote.
 */
/**@{*/
HAP_RESULT_USE_CHECK
HAPError RemoteHandleTargetControlSupportedConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleTargetControlListWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleTargetControlListRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleActiveIdentifierRead(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicReadRequest* request,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

HAP_RESULT_USE_CHECK
HAPError RemoteHandleButtonEventRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED);

void RemoteHandleButtonEventSubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context);

void RemoteHandleButtonEventUnsubscribe(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicSubscriptionRequest* request,
        void* _Nullable context);
/**@}*/

/**
 * Callback when a session is accepted on the HAP server.
 */
void RemoteHandleSessionAccept(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context);

/**
 * Callback when a session is invalidated on the HAP server.
 */
void RemoteHandleSessionInvalidate(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context);

/**
 * The callback used to advertise the controller's Target Control identifier.
 */
void RemoteHandleTargetControlIdentifierUpdate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPTargetControlDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPTargetControlDataStreamProtocolTargetIdentifier targetIdentifier,
        void* _Nullable context);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
