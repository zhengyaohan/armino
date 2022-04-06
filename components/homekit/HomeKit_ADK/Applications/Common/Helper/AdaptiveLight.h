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

#ifndef APP_ADAPTIVE_LIGHT_H
#define APP_ADAPTIVE_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#include "ApplicationFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Maximum number of supported Transitions.
 */
#define kLightbulb_MaxSupportedTransitions ((size_t) 2)
/**
 * Maximum number of supported Transitions Points.
 */
#define kLightbulb_MaxSupportedTransitionPoints ((size_t) 52)
/**
 * Minimum target completion duration for a Transition Point.
 * Note: The first Transition Point can have target completion duration of 0ms.
 *
 * @see HomeKit Accessory Protocol Specification R17
 * Section 14.3.3.2 Transitions Procedure Reference Flow
 */
#define kLightbulb_MinTargetCompletionDuration ((uint64_t) 100)
/**
 * Maximum size of controller context set.
 */
#define kLightbulb_MaxControllerContextSize ((size_t) 256)
/**
 * Maximum size of TLV for transition configuration.
 */
#define kLightbulb_MaxTransitionTlvSize ((size_t) 1500)
/**
 * Update Interval for Transitions.
 */
#define kLightbulb_TransitionUpdateInterval ((HAPTime) 60 * HAPSecond)

/**
 * Maximum size of a chunk that stores transition data
 */
#define kLightbulb_MaxTransitionChunkSize ((size_t) 500)

/**
 * Maximum number of chunks for storing transition data
 */
#define kLightbulb_MaxKvsChunksPerTransition ((uint8_t) 10)

/**
 * Number of keys per transition
 */
#define kLightbulb_MaxKeysPerTransition ((uint8_t) 4 + kLightbulb_MaxKvsChunksPerTransition)

/**
 * Key value store domain for Adaptive Light.
 */
#define kAppKeyValueStoreDomain_AdaptiveLight ((HAPPlatformKeyValueStoreDomain) 0x01)

/**
 * Key for storing characteristic ID.
 */
#define kAppKeyValueStoreKey_characteristicID ((HAPPlatformKeyValueStoreKey) 0x00)

/**
 * Key for storing request time.
 */
#define kAppKeyValueStoreKey_RequestTime ((HAPPlatformKeyValueStoreKey) 0x01)

/**
 * Key for storing threshold.
 */
#define kAppKeyValueStoreKey_Threshold ((HAPPlatformKeyValueStoreKey) 0x03)

/**
 * Key for storing Service ID.
 */
#define kAppKeyValueStoreKey_ServiceID ((HAPPlatformKeyValueStoreKey) 0x04)

/**
 * First key for storing transition data.
 */
#define kAppKeyValueStoreKey_TransitionData_Start ((HAPPlatformKeyValueStoreKey) 0x05)

/**
 * Last key for storing transition data.
 */
#define kAppKeyValueStoreKey_TransitionData_End \
    (kAppKeyValueStoreKey_TransitionData_Start + kLightbulb_MaxKvsChunksPerTransition)

/**
 * Key for storing clock offset.
 */
#define kAppKeyValueStoreKey_ClockOffset ((HAPPlatformKeyValueStoreKey) 0xF0)

//----------------------------------------------------------------------------------------------------------------------
/**
 * Struct describing characteristic and its supported transition types.
 */
typedef struct {
    /**
     * HAP Instance ID of the characteristic.
     */
    uint64_t characteristicID;
    /**
     * Bitmask of supported transition types for given characteristic.
     */
    uint8_t transitionTypes;
} AdaptiveLightSupportedTransition;

/**
 * Controller Context
 */
typedef struct {
    /**
     * Blob to be stored by accessory
     */
    uint8_t bytes[kLightbulb_MaxControllerContextSize];
    /**
     * Size of the context blob.
     */
    size_t numBytes;
} AdaptiveLightControllerContext;

/**
 * Adaptive Light Transition Types.
 */
HAP_ENUM_BEGIN(uint8_t, AdaptiveLightTransitionType) {
    /**
     * Linear Transition. This transition doesn't depend on any other value.
     */
    kAdaptiveLightTransitionType_Linear =
            kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_Linear,
    /**
     * Linear Derived Transition. This transition depends on linear value of the other, aka. source, characteristic.
     */
    kAdaptiveLightTransitionType_LinearDerived =
            kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_LinearDerived,
} HAP_ENUM_END(uint8_t, AdaptiveLightTransitionType);

/**
 * Adaptive Light Transition Points.
 *
 * This struct describes transition points for Linear and Linear Derived transitions.
 */
typedef struct {
    union {
        /**
         * Parameters describing Linear Derived transitions.
         */
        struct {
            float scale;
            float offset;
        };
        /**
         * Parameters describing Linear transitions.
         */
        struct {
            int32_t target;
            uint8_t targetBufferLen;
            float rate;
        };
    };
    /**
     * Absolute time (in ms) when this transition begins.
     *
     * This would get updated repeatedly for transitions with 'Loop'.
     */
    uint64_t startTime;
    /**
     * Absolute time (in ms) when this transition ends.
     *
     * This would get updated repeatedly for transitions with 'Loop'.
     */
    uint64_t endTime;
    /**
     * Relative duration (in ms) between start of current transition point and end of previous.
     */
    uint64_t startDelayDuration;
    /**
     * Relative duration (in ms) required to complete the transition once it begins.
     */
    uint64_t completionDuration;
    /**
     * Flags.
     */
    struct {
        /**
         * Flag indicating if this transition point is valid.
         */
        bool isUsed : 1;

        /**
         * Flag indicating if this transition point should execute immediately.
         */
        bool executeImmediately;
    };
} AdaptiveLightTransitionPoint;

/**
 * Adaptive Light Transition.
 *
 * This struct describes transition for given characteristic.
 */
typedef struct {
    /**
     * HAP Instance ID the characteristic.
     *
     * Each characteristic can have only one transition set at a time.
     */
    uint64_t characteristicID;
    /**
     * Service Instance ID the characteristic.
     *
     * The IID of the service this transition belongs to.
     */
    uint64_t serviceID;
    /**
     * Transition type.
     */
    AdaptiveLightTransitionType transitionType;
    /**
     * Controller Context.
     *
     * This is set by controller when transition is defined. Accessory is expected to store this opaque structure and
     * return when controller queries Transition State.
     */
    AdaptiveLightControllerContext controllerContext;
    /**
     * HAP Instance ID of the source characteristic.
     *
     * Applicable for Linear Derived Transitions.
     */
    uint64_t sourceCharacteristicID;
    /**
     * Lower bound for source value.
     */
    int32_t lowerValue;
    uint8_t lowerValueBufferLen;
    /**
     * Upper bound for source value.
     */
    int32_t upperValue;
    uint8_t upperValueBufferLen;
    /**
     * Transition points for this transition.
     */
    AdaptiveLightTransitionPoint* _Nonnull points[kLightbulb_MaxSupportedTransitionPoints];
    /**
     * Number of active transition points defined for this transition.
     */
    size_t numPoints;
    /**
     * Threshold value for transition.
     *
     * Required when Start Condition is set.
     */
    int32_t threshold;

    /**
     * Start Condition.
     */
    HAPCharacteristicValue_TransitionControl_StartCondition startCondition;
    /**
     * End Behavior.
     *
     * This describes what happens when this transition is completed.
     */
    HAPCharacteristicValue_TransitionControl_EndBehavior endBehavior;
    /**
     * Absolute time (in ms) when transition was defined by controller.
     */
    uint64_t requestTime;
    /**
     * Current Value of the characteristic.
     */
    int32_t currentValue;
    /**
     * Active Transition point.
     *
     * Each transition goes through multiple phases, each defined by transition points. The Active Transition point is
     * one that is currently exercised.
     */
    size_t activePointID;
    /**
     * Transition Update Time Interval
     *
     * Time interval between two transition updates in milliseconds.
     */
    uint64_t updateInterval;
    /**
     * Timestamp for next scheduled transition update.
     */
    uint64_t nextTransitionUpdateAt;
    /**
     * Time interval threshold for notifications
     *
     * Minimum time interval between two notifications for given characteristic value updates.
     */
    uint64_t timeIntervalThresholdForNotification;
    /**
     * Timestamp for next scheduled notification
     *
     * The application shouldn't send notification before this time.
     */
    uint64_t nextNotificationAt;
    /**
     * Value change threshold for notifications
     *
     * Minimum change in the characteristic value before app sends out notification
     */
    int32_t valueChangeThresholdForNotification;
    /**
     * Last notified characteristic value
     */
    int32_t lastNotifiedCharacteristicValue;
} AdaptiveLightTransition;

/**
 * Callback functions provided by App for various requests/notifications.
 */
typedef struct {
    /**
     * Callback function invoked when characteristic value changes during transition.
     *
     * @param characteristicID   HAP Instance ID of the characteristic.
     * @param value              New value
     * @param sendNotification   Boolean to indicate if app should notify controller of value change
     */
    void (*handleCharacteristicValueUpdate)(uint64_t characteristicID, int32_t value, bool sendNotification);
    /**
     * Callback function invoked when a transition gets over.
     */
    void (*handleTransitionExpiry)(void);
    /**
     * Callback function invoked to obtain current value of given characteristic.
     *
     * @param characteristicID   HAP Instance ID of the characteristic.
     * @param value              Output value
     */
    void (*handleCharacteristicValueRequest)(uint64_t characteristicID, int32_t* value);
} AdaptiveLightCallbacks;

/**
 * Transition Control Read Response Type.
 */
HAP_ENUM_BEGIN(uint8_t, TransitionControlResponseType) {
    /**
     * Response Type: Characteristic Value Transition State
     *
     * The 'Read' handler of Transition Control must return 'Characteristic Value Transition State':
     *      - in response to 'Write' for Characteristic Value Transition Start.
     *      - in response to 'Read' request for Transition Control.
     */
    kTransitionControlResponseType_TransitionState = 0,
    /**
     * Response Type: Characteristic Value Transition
     *
     * The 'Read' handler of Transition Control must return Transition:
     *      - in response to 'Write' for Characteristic Value Transition Fetch.
     */
    kTransitionControlResponseType_Transition = 1,
} HAP_ENUM_END(uint8_t, TransitionControlResponseType);

/**
 * Runtime Context for handling Adaptive Light Transitions.
 */
typedef struct {
    /**
     * Threshold value for transition to be restored.
     */
    int32_t threshold;
    /**
     * Request time (in ms) of the transition to be restored.
     */
    uint64_t requestTime;

    /**
     * Service Instance ID the characteristic.
     */
    uint64_t serviceID;
} AdaptiveLightPersistentMemoryContext;

/**
 * Runtime Context for handling Adaptive Light Transitions.
 */
typedef struct {
    /**
     * Cached Session identifier required to identify 'write response'.
     */
    HAPSession* _Nullable session;
    /**
     * Response Type to be referred during read handler of Characteristic Value Transition.
     */
    uint8_t pendingResponseType;
    /**
     * Cached value of HAP Instance ID required to fetch appropriate response.
     */
    uint64_t fetchID;
    /**
     * Cached Service ID required to filter the response.
     */
    const HAPService* _Nullable service;

    /**
     * Context used while restoring transitions from persistent memory.
     *
     * This value will be NULL if not restoring.
     */
    AdaptiveLightPersistentMemoryContext* _Nullable restoreContext;
} AdaptiveLightRuntimeContext;

/**
 * Adaptive Light Timer Information.
 */
typedef struct {
    /**
     * Timer instance.
     */
    HAPPlatformTimerRef timer;
    HAPPlatformTimerRef persistClockTimer;
} AdaptiveLightTransitionSchedule;

/**
 * Adaptive Light Transition Storage.
 */
typedef struct {

    /**
     * Storage for Supported Transition descriptor.
     */
    AdaptiveLightSupportedTransition supportedTransitions[kLightbulb_MaxSupportedTransitions];
    /**
     * Number of supported transitions.
     */
    size_t numSupportedTransitions;

    /**
     * Callbacks provided by App.
     */
    AdaptiveLightCallbacks callbacks;

    /**
     * Storage for configured Transitions.
     */
    AdaptiveLightTransition transitions[kLightbulb_MaxSupportedTransitions];
    /**
     * Number of configured Transitions.
     */
    size_t numTransitions;

    /**
     * Storage for Transition Points.
     */
    AdaptiveLightTransitionPoint transitionPoints[kLightbulb_MaxSupportedTransitionPoints];
    /**
     * Number of configured Transition Points.
     */
    size_t numTransitionPoints;

    /**
     * Storage for Runtime Context.
     */
    AdaptiveLightRuntimeContext context;

    /**
     * Storage for Transition Timer Context.
     */
    AdaptiveLightTransitionSchedule schedule;

    /**
     * Offset from Current Clock Time
     */
    HAPTime clockOffset;
} AdaptiveLightTransitionStorage;

HAP_UNUSED static HAPPlatformKeyValueStoreRef accessoryKeyValueStore = NULL;
/**
 * Static Pointer pointing to Adaptive Light Storage.
 *
 * The app using Adaptive Light allocates memory for Storage and shared pointer to it during initialization.
 * This pointer is cached here.
 */
HAP_UNUSED static AdaptiveLightTransitionStorage* adaptiveLight = NULL;

/**
 * Static Pointer pointing to Temporary Adaptive Light Storage.
 *
 * This is used during configuration of Transitions. The accessory would initially store proposed transitions in
 * temporary storage, validate requested transitions, and accept if those are correct. On acceptance, configured
 * transitions are copied to Persistent Storage, ie Storage owned by App, and temporary storage is released.
 */
HAP_UNUSED static AdaptiveLightTransitionStorage* _Nullable tempTransitions = NULL;

//----------------------------------------------------------------------------------------------------------------------
/**
 * Print configured Transitions.
 *
 * Debug function which prints configured configurations to console.
 */
void PrintTransitions(void);

/**
 * Remove configured Transition.
 *
 * @param      id               HAP Instance ID for the corresponding characteristic
 */
void RemoveTransition(uint64_t id);

/**
 * Update the derived transition value due to change in source characteristic.
 *
 * @param      sourceId         HAP Instance ID of the source characteristic
 */
void UpdateDerivedTransitionValue(uint64_t sourceID);

/**
 * Initialize Adaptive Light Storage
 *
 * @param      adaptiveLight             Pointer to Persistent Storage
 * @param      supportedTransitions     Supported Transitions
 * @param      numSupportedTransitions  Number of Supported Transitions
 * @param      callback                 Callbacks to be invoked for requests and notifications
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If requested ID doesn't have any Transition.
 */
HAP_RESULT_USE_CHECK
HAPError InitializeAdaptiveLightParameters(
        HAPPlatformKeyValueStoreRef keyValueStore,
        AdaptiveLightTransitionStorage* adaptiveLight,
        AdaptiveLightSupportedTransition* supportedTransitions,
        size_t numSupportedTransitions,
        AdaptiveLightCallbacks* callbacks);

/**
 * Read Handler for 'Supported Characteristic Value Transition Configuration'.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleSupportedTransitionConfigurationRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED,
        HAPTLVWriter* responseWriter HAP_UNUSED,
        void* _Nullable context HAP_UNUSED);

/**
 * Read Handler for 'Characteristic Value Transition Control'.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleTransitionControlRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPTLV8CharacteristicReadRequest* request HAP_UNUSED,
        HAPTLVWriter* responseWriter HAP_UNUSED,
        void* _Nullable context HAP_UNUSED);

/**
 * Write Handler for 'Characteristic Value Transition Control'.
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleTransitionControlWrite(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPTLV8CharacteristicWriteRequest* request HAP_UNUSED,
        HAPTLVReader* requestReader HAP_UNUSED,
        void* _Nullable context HAP_UNUSED);

/**
 * Read Handler for 'Characteristic Value Transition Count'.
 */

HAPError HAPHandleTransitionCountRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED);

/**
 * Schedule Transition events
 */
HAP_RESULT_USE_CHECK
HAPError UpdateTransitionTimer(void);

AdaptiveLightTransition* _Nullable AllocTransition(void);

AdaptiveLightTransitionPoint* _Nullable AllocTransitionPoint(void);

void ReleaseTransitionPoint(AdaptiveLightTransitionPoint* pt);

void ReleaseTransition(AdaptiveLightTransition* t);

void LoadTransitionsFromPersistentMemory(void);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
