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

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <math.h> // round

#include "HAP.h"

#include "AdaptiveLight.h"
#include "HAPCharacteristic.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

// Most Lightbulb accessories do not persist time when they reboot.
// In those instances, ADK must make a 'best effort' to preserve current time.
// If accessory naturally persists over time (raspi for example) set this value to 1
#ifndef CLOCK_PERSISTS_TIME
#define CLOCK_PERSISTS_TIME 0
#endif

//----------------------------------------------------------------------------------------------------------------------

static void HandleTransitionTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context);
static HAPError RestoreTransition(HAPTLVReader* requestReader);
static HAPError RemoveCharacteristicTransitionFromPersistentMemory(uint64_t characteristicID);

#define CONVERT_DATA(dataStruct, data) \
    do { \
        (dataStruct).numBytes = HAPGetVariableIntEncodingLength(data); \
        (dataStruct).bytes = &(data); \
    } while (0)

// If the accessory does not natively persist time, make a 'best effort' by
// periodically saving time.  Note that any time that elapses while the accessory is powered down
// is 'lost'.  The accessory will pick up where it left off.
#if (CLOCK_PERSISTS_TIME == 0)

#define GET_ADAPTIVE_LIGHT_TIME() (HAPPlatformClockGetCurrent() + adaptiveLight->clockOffset)
#define NL_TO_SYSTEM_TIME(nlTime) (nlTime - adaptiveLight->clockOffset)

#define kPersistClockTimeout ((HAPTime)(10 * HAPMinute))
static void HandlePersistTime(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    HAPError err = HAPPlatformKeyValueStoreOverrideAndSet(
            accessoryKeyValueStore,
            kAppKeyValueStoreDomain_AdaptiveLight,
            kAppKeyValueStoreKey_ClockOffset,
            &nowNlTime,
            sizeof nowNlTime);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }

    // Restart Timer
    err = HAPPlatformTimerRegister(
            &(adaptiveLight->schedule.persistClockTimer),
            HAPPlatformClockGetCurrent() + kPersistClockTimeout,
            HandlePersistTime,
            NULL);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to register persist clock timer");
        HAPFatalError();
    }
}
#else
#define GET_ADAPTIVE_LIGHT_TIME() (HAPPlatformClockGetCurrent())
#define NL_TO_SYSTEM_TIME(nlTime) (nlTime)
#endif

/**
 * Convert a int32_t into a byte array of the specified size (inserting leading 0s if necessary).
 *
 * @param number                   The number to convert to a byte array.
 * @param byteArrayTargetLen       The desired length of the byte array.
 * @param byteArray[in/out]        The byte array.
 * @param byteArrayLen             The size of the byte array passed into the function.
 *
 * @return uint8_t                 The size of the copied byte array.
 */
static uint8_t
        ConvertInt32ToByteArray(int32_t number, uint8_t byteArrayTargetLen, uint8_t* byteArray, uint8_t byteArrayLen) {
    HAPPrecondition(byteArray);
    uint8_t* tempByteArray;

    if (byteArrayTargetLen <= 0 || byteArrayLen <= 0) {
        HAPLogError(&kHAPLog_Default, "%s buffer or byte array is less than or equal to 0.", __func__);
        return 0;
    }

    HAPRawBufferZero(byteArray, byteArrayLen);

    // Ensure the length of the buffer does not exceed the max number of bytes fin the byte array.
    if (byteArrayLen >= byteArrayTargetLen) {
        byteArrayLen = byteArrayTargetLen;
    }

    // Convert into byte array.
    tempByteArray = (uint8_t*) &(number);
    for (int i = 0; i < byteArrayLen; i++) {
        byteArray[i] = tempByteArray[i];
    }

    return byteArrayLen;
}

static AdaptiveLightTransition* _Nullable GetTransition(uint64_t id) {
    HAPPrecondition(adaptiveLight);
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        if (adaptiveLight->transitions[i].characteristicID == id && adaptiveLight->transitions[i].numPoints) {
            return &(adaptiveLight->transitions[i]);
        }
    }
    return NULL;
}

static AdaptiveLightTransition* _Nullable AllocateTransition(AdaptiveLightTransitionStorage* storage) {
    HAPPrecondition(storage);
    if (storage->numTransitions >= kLightbulb_MaxSupportedTransitions) {
        return NULL;
    }
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        if (storage->transitions[i].numPoints == 0) {
            storage->numTransitions++;
            return &(storage->transitions[i]);
        }
    }
    return NULL;
}

AdaptiveLightTransition* _Nullable AllocTransition(void) {
    return AllocateTransition(adaptiveLight);
}

static AdaptiveLightTransition* _Nullable AllocTempTransition(void) {
    return AllocateTransition(tempTransitions);
}

static AdaptiveLightTransitionPoint* _Nullable AllocateTransitionPoint(AdaptiveLightTransitionStorage* storage) {
    HAPPrecondition(storage);
    if (storage->numTransitionPoints >= kLightbulb_MaxSupportedTransitionPoints) {
        return NULL;
    }
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitionPoints; i++) {
        AdaptiveLightTransitionPoint* pt = &(storage->transitionPoints[i]);
        if (pt->isUsed == 0) {
            pt->isUsed = 1;
            storage->numTransitionPoints++;
            return pt;
        }
    }
    return NULL;
}

AdaptiveLightTransitionPoint* _Nullable AllocTransitionPoint(void) {
    return AllocateTransitionPoint(adaptiveLight);
}

static AdaptiveLightTransitionPoint* _Nullable AllocTempTransitionPoint(void) {
    return AllocateTransitionPoint(tempTransitions);
}

static bool IsCharacteristicSupportedByService(uint64_t charID, const HAPService* service) {
    HAPPrecondition(service);

    for (size_t i = 0; service->characteristics[i] != NULL; i++) {
        const HAPBaseCharacteristic* characteristic = (const HAPBaseCharacteristic*) service->characteristics[i];
        if (charID == characteristic->iid) {
            return true;
        }
    }
    return false;
}

static bool IsInt32CharacteristicValueRangeValid(
        uint64_t charID,
        const HAPService* service,
        int32_t lowerValue,
        int32_t upperValue) {
    HAPPrecondition(service);

    for (size_t i = 0; service->characteristics[i] != NULL; i++) {
        const HAPBaseCharacteristic* characteristic = (const HAPBaseCharacteristic*) service->characteristics[i];
        if (charID == characteristic->iid) {
            HAPAssert(characteristic->format == kHAPCharacteristicFormat_Int);
            const HAPIntCharacteristic* characteristicInt32 = (const HAPIntCharacteristic*) service->characteristics[i];
            if (lowerValue > upperValue) {
                return false;
            }
            if (lowerValue < characteristicInt32->constraints.minimumValue ||
                lowerValue > characteristicInt32->constraints.maximumValue) {
                return false;
            }
            if (upperValue < characteristicInt32->constraints.minimumValue ||
                upperValue > characteristicInt32->constraints.maximumValue) {
                return false;
            }
            return true;
        }
    }
    return false;
}

static bool IsSupported(uint64_t id) {
    HAPPrecondition(adaptiveLight);
    for (size_t i = 0; i < adaptiveLight->numSupportedTransitions && i < kLightbulb_MaxSupportedTransitions; i++) {
        if (adaptiveLight->supportedTransitions[i].characteristicID == id) {
            return true;
        }
    }
    return false;
}

static bool IsTransitionTypeSupported(uint64_t id, uint8_t type) {
    HAPPrecondition(adaptiveLight);
    for (size_t i = 0; i < adaptiveLight->numSupportedTransitions && i < kLightbulb_MaxSupportedTransitions; i++) {
        if (adaptiveLight->supportedTransitions[i].characteristicID == id &&
            (adaptiveLight->supportedTransitions[i].transitionTypes & type)) {
            return true;
        }
    }
    return false;
}

static bool ValidateLinearTransitionTargetValues(const AdaptiveLightTransition* t, const HAPService* service) {
    HAPPrecondition(t);
    HAPPrecondition(service);

    if (t->transitionType != kAdaptiveLightTransitionType_Linear) {
        return false;
    }

    int32_t minVal = 0;
    int32_t maxVal = 0;
    const uint64_t charID = t->characteristicID;

    for (size_t i = 0; service->characteristics[i] != NULL; i++) {
        const HAPBaseCharacteristic* characteristic = (const HAPBaseCharacteristic*) service->characteristics[i];
        if (charID == characteristic->iid) {
            HAPAssert(characteristic->format == kHAPCharacteristicFormat_Int);
            const HAPIntCharacteristic* characteristicInt32 = (const HAPIntCharacteristic*) service->characteristics[i];
            minVal = characteristicInt32->constraints.minimumValue;
            maxVal = characteristicInt32->constraints.maximumValue;
            break;
        }
    }

    for (size_t i = 0; i < t->numPoints; i++) {
        const AdaptiveLightTransitionPoint* pt = t->points[i];
        if (pt->target < minVal || pt->target > maxVal) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "[Point#%zu] Target value = %" PRId32 " is outside range (%" PRId32 ", %" PRId32 ")",
                    i,
                    pt->target,
                    minVal,
                    maxVal);
            return false;
        }
    }

    return true;
}

static int32_t ConvertToInt32(HAPCharacteristicValue_VariableLengthInteger* val) {
    HAPPrecondition(val);
    int32_t res = 0;
    HAPAssert(val->numBytes <= sizeof res);
    if (val->bytes == NULL) {
        HAPAssert(val->bytes);
        return res;
    }
    if (val->numBytes == 1) {
        res = HAPReadInt8(val->bytes);
    } else if (val->numBytes == 2) {
        res = HAPReadLittleInt16(val->bytes);
    } else if (val->numBytes == 3) {
        res = HAPReadLittleInt24(val->bytes);
    } else {
        res = HAPReadLittleInt32(val->bytes);
    }
    return res;
}

static uint64_t ConvertToUInt64(const HAPCharacteristicValue_VariableLengthInteger* val) {
    HAPPrecondition(val);
    uint64_t res = 0;
    const uint8_t* v = val->bytes;
    if (!v) {
        HAPAssert(v);
        return res;
    }
    HAPAssert(val->numBytes <= sizeof res);
    for (size_t i = 0; i < val->numBytes && i < sizeof res; i++) {
        res += v[i] << (8 * i);
    }
    return res;
}

void ReleaseTransitionPoint(AdaptiveLightTransitionPoint* pt) {
    HAPPrecondition(pt);
    HAPRawBufferZero(pt, sizeof *pt);
}

static void ReleaseTransitionPointsFor(AdaptiveLightTransition* t) {
    HAPPrecondition(t);
    for (size_t i = 0; i < t->numPoints && i < kLightbulb_MaxSupportedTransitionPoints && t->points[i]; i++) {
        ReleaseTransitionPoint(t->points[i]);
    }
    t->numPoints = 0;
}

void ReleaseTransition(AdaptiveLightTransition* t) {
    HAPPrecondition(t);
    size_t freePoints = t->numPoints;
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        if (t == &(adaptiveLight->transitions[i])) {
            HAPAssert(adaptiveLight->numTransitionPoints >= freePoints);
            adaptiveLight->numTransitionPoints -= freePoints;
            HAPAssert(adaptiveLight->numTransitions);
            adaptiveLight->numTransitions--;
        }
    }
    ReleaseTransitionPointsFor(t);
    HAPRawBufferZero(t, sizeof *t);
}

void PrintTransitions(void) {
    HAPPrecondition(adaptiveLight);
    HAPLogInfo(&kHAPLog_Default, "====Debug Print=====");
    HAPLogInfo(&kHAPLog_Default, "transitions count = %zu", adaptiveLight->numTransitions);
    HAPLogInfo(&kHAPLog_Default, "transition points count = %zu", adaptiveLight->numTransitionPoints);
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        AdaptiveLightTransition* t = &(adaptiveLight->transitions[i]);
        if (t->numPoints == 0) {
            continue;
        }
        HAPLogInfo(&kHAPLog_Default, "transitions ID = %llu", (unsigned long long) t->characteristicID);
        HAPLogInfo(
                &kHAPLog_Default,
                "transitions type = %s",
                (t->transitionType == kAdaptiveLightTransitionType_Linear) ? "linear" : "derived");
        HAPLogInfo(&kHAPLog_Default, "transitions points = %zu", t->numPoints);
    }
}

void PrintTransition(const AdaptiveLightTransition* t) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(t);
    HAPLogInfo(&kHAPLog_Default, "Characteristic ID = %llu", (unsigned long long) t->characteristicID);
    HAPLogInfo(
            &kHAPLog_Default,
            "Transitions Type = %s",
            (t->transitionType == kAdaptiveLightTransitionType_Linear) ? "Linear" : "Linear Derived");
    HAPLogInfo(
            &kHAPLog_Default,
            "Transition End Behavior = %s",
            t->endBehavior == kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange ? "Stop" : "Loop");
    HAPLogInfo(&kHAPLog_Default, "Transition Current Value = %i", (int) t->currentValue);
    HAPLogInfo(&kHAPLog_Default, "Controller Context = %zu", t->controllerContext.numBytes);
    HAPLogInfo(&kHAPLog_Default, "Value Update Time Interval = %llu", (unsigned long long) t->updateInterval);
    HAPLogInfo(&kHAPLog_Default, "Notify Value Change Threshold = %i", (int) t->valueChangeThresholdForNotification);
    HAPLogInfo(
            &kHAPLog_Default,
            "Notify Time Interval Threshold = %llu",
            (unsigned long long) t->timeIntervalThresholdForNotification);
    if (t->transitionType == kAdaptiveLightTransitionType_Linear) {
        HAPLogInfo(&kHAPLog_Default, "Transition Start Condition = %u", t->startCondition);
        HAPLogInfo(&kHAPLog_Default, "Transition Start Condition Threshold = %i", (int) t->threshold);
    } else {
        HAPLogInfo(&kHAPLog_Default, "Source id = %llu", (unsigned long long) t->sourceCharacteristicID);
        HAPLogInfo(&kHAPLog_Default, "Source Value Bound = (%i, %i)", (int) t->lowerValue, (int) t->upperValue);
    }
    HAPLogInfo(&kHAPLog_Default, "Transition Points Count = %zu", t->numPoints);
    for (size_t i = 0; i < t->numPoints; i++) {
        const AdaptiveLightTransitionPoint* pt = t->points[i];
        HAPLogInfo(
                &kHAPLog_Default,
                "[Point#%zu] Start, Completion Delay = %llu, %llu",
                i,
                (unsigned long long) pt->startDelayDuration,
                (unsigned long long) pt->completionDuration);
        HAPLogInfo(
                &kHAPLog_Default,
                "[Point#%zu] Start -> End Time = %llu -> %llu",
                i,
                (unsigned long long) pt->startTime,
                (unsigned long long) pt->endTime);
        if (t->transitionType == kAdaptiveLightTransitionType_Linear) {
            HAPLogInfo(&kHAPLog_Default, "[Point#%zu] Target value = %i", i, (int) pt->target);
            HAPLogInfo(&kHAPLog_Default, "[Point#%zu] Rate = %g", i, pt->rate);
        } else {
            HAPLogInfo(&kHAPLog_Default, "[Point#%zu] Scale = %g", i, pt->scale);
            HAPLogInfo(&kHAPLog_Default, "[Point#%zu] Offset = %g", i, pt->offset);
        }
    }
}

void RemoveTransition(uint64_t id) {
    HAPPrecondition(adaptiveLight);
    uint32_t prevTransitionCount = adaptiveLight->numTransitions;
    AdaptiveLightTransition* t = GetTransition(id);
    if (!t) {
        HAPLogInfo(&kHAPLog_Default, "%s transition for id = %llu not found", __func__, (unsigned long long) id);
        return;
    }
    uint64_t characteristicID = t->characteristicID;
    ReleaseTransition(t);
    HAPError err = RemoveCharacteristicTransitionFromPersistentMemory(characteristicID);
    if (err) {
        HAPLogError(
                &kHAPLog_Default,
                "All data may not have been erased from persistent memory for transition for id %llu",
                (unsigned long long) characteristicID);
    }

    // Controller will be notified when the 'Characteristic Value Transition Count' changes.
    if (prevTransitionCount != adaptiveLight->numTransitions) {
        adaptiveLight->callbacks.handleTransitionExpiry();
    }
}

/**
 * Update the start and end times for all the points in a transition.
 *
 * @param transition                 The transition whose point times will be updated.
 *
 * @return kHAPError_None            When successful.
 * @return kHAPError_InvalidData     On error.
 */
static HAPError UpdateTransitionPointTimes(AdaptiveLightTransition* transition) {
    HAPPrecondition(transition);

    if (transition->numPoints == 0) {
        return kHAPError_None;
    }
    uint64_t prevPointCompletionTime = 0;

    AdaptiveLightTransitionPoint* lastPoint = transition->points[transition->numPoints - 1];
    prevPointCompletionTime = lastPoint->endTime;
    // If end time for the last point is non-zero, this is an existing transition with Loop.
    // Otherwise, this is a new transition.
    if (prevPointCompletionTime == 0) {
        // If we are restoring from persistent memory, use requestTime from restore context
        if (adaptiveLight->context.restoreContext) {
            prevPointCompletionTime = adaptiveLight->context.restoreContext->requestTime;
        } else {
            prevPointCompletionTime = GET_ADAPTIVE_LIGHT_TIME();
        }
    }

    for (size_t i = 0; i < transition->numPoints; i++) {
        AdaptiveLightTransitionPoint* point = transition->points[i];
        if (point == NULL) {
            continue;
        }

        // Update the completion time to account for the previous point (if there was a previous point).
        if (i > 0) {
            if (transition->points[i - 1] == NULL) {
                continue;
            }
            AdaptiveLightTransitionPoint* prevPoint = transition->points[i - 1];
            prevPointCompletionTime = prevPoint->endTime;

            // This checks a case that should not occur, since we do not allow completionDuration
            // to be 0 (for any point but the first point). However, if completionDuration is 0,
            // this can lead to an infinite loop in UpdateTransitionTimer(), since the endTime does
            // not change.
            if (point->completionDuration == 0) {
                HAPLogError(&kHAPLog_Default, "%s: Completion duration for point %zu must be non-zero", __func__, i);
                return kHAPError_InvalidData;
            }
        }

        if (i == 0 && transition->numPoints > 1 && point->startDelayDuration == 0 && point->completionDuration == 0) {
            // On the first point, when both the startDelayDuration and completionDuration are 0, and the transition has
            // more than 1 point, it indicates that the point should execute immedately.
            point->executeImmediately = true;
        } else {
            point->executeImmediately = false;
        }

        point->startTime = prevPointCompletionTime + point->startDelayDuration;
        point->endTime = point->startTime + point->completionDuration;
    }
    return kHAPError_None;
}

static void UpdateLinearTransitionRate(AdaptiveLightTransition* t) {
    HAPPrecondition(t);

    if (t->numPoints == 0) {
        return;
    }

    if (t->transitionType == kAdaptiveLightTransitionType_LinearDerived) {
        return;
    }
    int32_t prevTarget = t->currentValue;

    for (size_t i = 0; i < t->numPoints; i++) {
        AdaptiveLightTransitionPoint* point = t->points[i];
        if (!point) {
            continue;
        }
        if (i > 0 && t->points[i - 1]) {
            prevTarget = t->points[i - 1]->target;
        }

        if (point->completionDuration) {
            point->rate = (float) (point->target - prevTarget) / (float) point->completionDuration;
        } else {
            point->rate = 0.0f;
        }
    }
}

// To retain transition data across reboots, ADK stores transitions to the persistent memory.
// ADK stores incoming transition TLVs as it is, so that it uses minimum amount of flash memory.
// On reboot, ADK reads TLVs for previously configured transitions and restores those to running state using
// other parameters like requestTime, threshold, and serviceID.

// Since ADK needs to support multiple transitions, it uses one 'slot' to store data for a single transition.
// Each 'slot' uses kLightbulb_MaxKeysPerTransition keys and it has following structure.

//  +--------------------+
//  | Characteristic ID  |
//  ----------------------
//  | Request Time       |
//  ----------------------
//  | Threshold          |
//  ----------------------
//  | Service ID         |
//  ----------------------
//  | Transition Chunk#0 |
//  ----------------------
//  | Transition Chunk#1 |
//  ----------------------
//  |    ...             |
//  ----------------------
//  | Transition Chunk#9 |
//  +--------------------+

// This structure gets repeated based on number of configured transitions.

void LoadTransitionsFromPersistentMemory(void) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPError err = kHAPError_None;
    bool found = false;
    size_t numBytes = 0;

    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        AdaptiveLightTransitionStorage tempTransitionsStorage;
        HAPRawBufferZero(&tempTransitionsStorage, sizeof tempTransitionsStorage);
        tempTransitions = &tempTransitionsStorage;
        AdaptiveLightPersistentMemoryContext restoreContext = { 0 };
        uint64_t kvsCharID = 0;
        uint8_t keyOffset = i * kLightbulb_MaxKeysPerTransition;
        uint8_t characteristicKey = kAppKeyValueStoreKey_characteristicID + keyOffset;
        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                characteristicKey,
                &kvsCharID,
                sizeof kvsCharID,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        if (!found || numBytes != sizeof kvsCharID) {
            if (found) {
                HAPLogError(&kHAPLog_Default, "Failed to read Adaptive Light Transition Characteristic ID");
            }
            // No transition exists at this key, continue to the next transition.
            continue;
        }

        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_RequestTime + keyOffset,
                &(restoreContext.requestTime),
                sizeof restoreContext.requestTime,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        if (!found || numBytes != sizeof restoreContext.requestTime) {
            if (found) {
                HAPLogError(&kHAPLog_Default, "Failed to read Adaptive Light Request Time");
            }
            restoreContext.requestTime = 0;
        }

        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_Threshold + keyOffset,
                &(restoreContext.threshold),
                sizeof restoreContext.threshold,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        if (!found || numBytes != sizeof restoreContext.threshold) {
            if (found) {
                HAPLogInfo(&kHAPLog_Default, "Couldn't read Adaptive Light Transition Threshold");
            }
            restoreContext.threshold = 0;
        }

        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_ServiceID + keyOffset,
                &(restoreContext.serviceID),
                sizeof restoreContext.serviceID,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        if (!found || numBytes != sizeof restoreContext.serviceID) {
            if (found) {
                HAPLogError(&kHAPLog_Default, "Failed to read Adaptive Light Transition Service ID");
            }
            restoreContext.serviceID = 0;
        }

        // Transition data could be stored over multiple consecutive chunks.
        uint8_t transitionData[kLightbulb_MaxTransitionTlvSize];
        uint8_t* tlvChunk = transitionData;
        size_t remainingBytes = sizeof transitionData;
        uint8_t chunkID = 0;
        while (remainingBytes) {
            uint8_t chunkKeyID = keyOffset + kAppKeyValueStoreKey_TransitionData_Start + chunkID;
            if (chunkKeyID >= keyOffset + kAppKeyValueStoreKey_TransitionData_End) {
                HAPLogError(
                        &kHAPLog_Default,
                        "Invalid Chunk Key = %u, expected key < %u",
                        chunkKeyID,
                        keyOffset + kAppKeyValueStoreKey_TransitionData_End);
                HAPFatalError();
            }
            err = HAPPlatformKeyValueStoreGet(
                    accessoryKeyValueStore,
                    kAppKeyValueStoreDomain_AdaptiveLight,
                    chunkKeyID,
                    tlvChunk,
                    remainingBytes,
                    &numBytes,
                    &found);

            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }
            if (!found) {
                // Completed reading all chunks
                break;
            }
            remainingBytes -= numBytes;
            tlvChunk += numBytes;
            chunkID++;
        }

        // If the transition data is empty, there is nothing to restore.
        if (remainingBytes == sizeof transitionData) {
            continue;
        }

        adaptiveLight->context.restoreContext = &restoreContext;

        HAPTLVReader tlvReader;
        HAPTLVReaderCreate(&tlvReader, (void*) (uintptr_t) &transitionData, sizeof transitionData - remainingBytes);
        err = RestoreTransition(&tlvReader);

        if (err) {
            HAPLogError(&kHAPLog_Default, "Failed to restore transition data from persistent memory");
            HAPAssert(!err);
        }
        adaptiveLight->context.restoreContext = NULL;
        tempTransitions = NULL;
    }

    err = HAPPlatformKeyValueStoreGet(
            accessoryKeyValueStore,
            kAppKeyValueStoreDomain_AdaptiveLight,
            kAppKeyValueStoreKey_ClockOffset,
            &(adaptiveLight->clockOffset),
            sizeof adaptiveLight->clockOffset,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof adaptiveLight->clockOffset) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Failed to read Adaptive Light clock offset");
        }
        adaptiveLight->clockOffset = 0;
    }
}

void SaveTransitionsToPersistentMemory(const uint8_t* transitionTlvData, size_t tlvSize, uint64_t charID) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPError err = kHAPError_None;
    bool found = false;
    size_t numBytes = 0;
    uint8_t keyOffset = 0;

    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        uint64_t kvsCharID = 0;
        keyOffset = i * kLightbulb_MaxKeysPerTransition;
        uint8_t characteristicKey = kAppKeyValueStoreKey_characteristicID + keyOffset;
        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                characteristicKey,
                &kvsCharID,
                sizeof kvsCharID,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        if (!found || numBytes != sizeof kvsCharID) {
            if (found) {
                HAPLogError(&kHAPLog_Default, "Failed to read Adaptive Light Transition Characteristic ID");
            }
        }
        if (found && kvsCharID != charID) {
            // If a different characteristic is found at this key, look for next slot in KVS
            continue;
        } else {
            // If this slot is empty or has transition data for the same characteristic, use this KVS slot.
            break;
        }
    }

    size_t chunkID = 0;
    while (tlvSize) {
        // Split transition data based on kLightbulb_MaxTransitionChunkSize
        size_t entrySize = tlvSize > kLightbulb_MaxTransitionChunkSize ? kLightbulb_MaxTransitionChunkSize : tlvSize;
        uint8_t chunkKeyID = keyOffset + kAppKeyValueStoreKey_TransitionData_Start + chunkID;
        if (chunkKeyID >= keyOffset + kAppKeyValueStoreKey_TransitionData_End) {
            HAPLogError(
                    &kHAPLog_Default,
                    "Invalid Chunk Key = %u, expected key < %u",
                    chunkKeyID,
                    keyOffset + kAppKeyValueStoreKey_TransitionData_End);
            HAPFatalError();
        }
        err = HAPPlatformKeyValueStoreOverrideAndSet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                chunkKeyID,
                transitionTlvData,
                entrySize);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        transitionTlvData += entrySize;
        tlvSize -= entrySize;
        chunkID++;
    }

    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        // Skip until we find transition for requested characteristic
        if (adaptiveLight->transitions[i].characteristicID != charID) {
            continue;
        }

        err = HAPPlatformKeyValueStoreOverrideAndSet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_characteristicID + keyOffset,
                &charID,
                sizeof charID);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        err = HAPPlatformKeyValueStoreOverrideAndSet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_ServiceID + keyOffset,
                &(adaptiveLight->transitions[i].serviceID),
                sizeof adaptiveLight->transitions[i].serviceID);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        err = HAPPlatformKeyValueStoreOverrideAndSet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                kAppKeyValueStoreKey_RequestTime + keyOffset,
                &(adaptiveLight->transitions[i].requestTime),
                sizeof adaptiveLight->transitions[i].requestTime);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        if (adaptiveLight->transitions[i].transitionType == kAdaptiveLightTransitionType_Linear) {
            err = HAPPlatformKeyValueStoreOverrideAndSet(
                    accessoryKeyValueStore,
                    kAppKeyValueStoreDomain_AdaptiveLight,
                    kAppKeyValueStoreKey_Threshold + keyOffset,
                    &(adaptiveLight->transitions[i].threshold),
                    sizeof adaptiveLight->transitions[i].threshold);

            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }
        }
    }

    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    err = HAPPlatformKeyValueStoreOverrideAndSet(
            accessoryKeyValueStore,
            kAppKeyValueStoreDomain_AdaptiveLight,
            kAppKeyValueStoreKey_ClockOffset,
            &nowNlTime,
            sizeof nowNlTime);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

static HAPError FindCharacteristicKeyOffsetFromPersistentMemory(uint64_t characteristicID, uint8_t* keyOffset) {
    bool found = false;
    size_t numBytes = 0;
    uint8_t tempKeyOffset = 0;
    HAPError err;

    // iterates on all stored transitions
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        uint64_t kvsCharID = 0;
        tempKeyOffset = i * kLightbulb_MaxKeysPerTransition;
        uint8_t characteristicKey = kAppKeyValueStoreKey_characteristicID + tempKeyOffset;
        err = HAPPlatformKeyValueStoreGet(
                accessoryKeyValueStore,
                kAppKeyValueStoreDomain_AdaptiveLight,
                characteristicKey,
                &kvsCharID,
                sizeof kvsCharID,
                &numBytes,
                &found);

        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        if (found && kvsCharID == characteristicID) {
            *keyOffset = tempKeyOffset;
            return kHAPError_None;
        }
    }

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: Transition for %llu not found in persistent memory.",
            __func__,
            (unsigned long long) characteristicID);
    return kHAPError_InvalidData;
}

HAP_RESULT_USE_CHECK
static HAPError RemoveCharacteristicTransitionFromPersistentMemory(uint64_t characteristicID) {
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPError err = kHAPError_None;
    bool processedWithErrors = false;
    uint8_t keyOffset;

    // Find key offset for characteristic in HAPPlatformKeyValueStore
    err = FindCharacteristicKeyOffsetFromPersistentMemory(characteristicID, &keyOffset);
    if (err) {
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: Nothing removed, transition %llu not found in persistent memory.",
                __func__,
                (unsigned long long) characteristicID);
        return kHAPError_None;
    }

    // Remove the Characteristic ID of the transition.
    err = HAPPlatformKeyValueStoreRemove(
            accessoryKeyValueStore,
            kAppKeyValueStoreDomain_AdaptiveLight,
            kAppKeyValueStoreKey_characteristicID + keyOffset);
    if (err) {
        processedWithErrors = true;
        HAPLogError(
                &kHAPLog_Default,
                "%s: Unable to remove characteristicID data for transition %llu from persistent memory.",
                __func__,
                (unsigned long long) characteristicID);
    }

    // Remove the Service ID of the transition.
    err = HAPPlatformKeyValueStoreRemove(
            accessoryKeyValueStore, kAppKeyValueStoreDomain_AdaptiveLight, kAppKeyValueStoreKey_ServiceID + keyOffset);
    if (err) {
        processedWithErrors = true;
        HAPLogError(
                &kHAPLog_Default,
                "%s: Unable to remove serviceID data for transition %llu from persistent memory.",
                __func__,
                (unsigned long long) characteristicID);
    }

    // Remove the Request Time (in ms) of the transition.
    err = HAPPlatformKeyValueStoreRemove(
            accessoryKeyValueStore,
            kAppKeyValueStoreDomain_AdaptiveLight,
            kAppKeyValueStoreKey_RequestTime + keyOffset);
    if (err) {
        processedWithErrors = true;
        HAPLogError(
                &kHAPLog_Default,
                "%s: Unable to remove requestTime data transition %llu from persistent memory.",
                __func__,
                (unsigned long long) characteristicID);
    }

    // Remove the Threshold value of the transition.
    err = HAPPlatformKeyValueStoreRemove(
            accessoryKeyValueStore, kAppKeyValueStoreDomain_AdaptiveLight, kAppKeyValueStoreKey_Threshold + keyOffset);
    if (err) {
        // Only linear transitions have threshholds, so this may not be an error.
        HAPLogError(
                &kHAPLog_Default,
                "%s: Unable to remove threshhold data for transition %llu from persistent memory.",
                __func__,
                (unsigned long long) characteristicID);
    }

    // Remove all transition points of the transition.
    uint8_t transitionPointsKeyStart = kAppKeyValueStoreKey_TransitionData_Start + keyOffset;
    uint8_t transitionPointsKeyEnd = kAppKeyValueStoreKey_TransitionData_End + keyOffset;
    uint8_t characteristicKey = kAppKeyValueStoreKey_characteristicID + keyOffset;
    for (uint8_t key = transitionPointsKeyStart; key < transitionPointsKeyEnd; key++) {
        err = HAPPlatformKeyValueStoreRemove(accessoryKeyValueStore, key, characteristicKey);
        if (err) {
            processedWithErrors = true;
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Unable to remove transition data chunk for transition %llu from persistent memory.",
                    __func__,
                    (unsigned long long) characteristicID);
        }
    }

    if (processedWithErrors) {
        HAPLogInfo(
                &kHAPLog_Default,
                "%s: Removed transition %llu from persistent memory. Some data was unable to be removed. Please "
                "check logs for more information.",
                __func__,
                (unsigned long long) characteristicID);
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError InitializeAdaptiveLightParameters(
        HAPPlatformKeyValueStoreRef keyValueStore,
        AdaptiveLightTransitionStorage* adaptiveLightStorage,
        AdaptiveLightSupportedTransition* supportedTransitions,
        size_t numSupportedTransitions,
        AdaptiveLightCallbacks* callbacks) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(adaptiveLightStorage);
    HAPPrecondition(supportedTransitions);
    HAPPrecondition(callbacks);
    HAPPrecondition(callbacks->handleCharacteristicValueUpdate);
    HAPPrecondition(callbacks->handleTransitionExpiry);
    HAPPrecondition(callbacks->handleCharacteristicValueRequest);

    if (numSupportedTransitions > kLightbulb_MaxSupportedTransitions) {
        return kHAPError_OutOfResources;
    }

    accessoryKeyValueStore = keyValueStore;
    adaptiveLight = adaptiveLightStorage;
    adaptiveLight->numSupportedTransitions = numSupportedTransitions;

    for (size_t i = 0; i < numSupportedTransitions; i++) {
        adaptiveLight->supportedTransitions[i].characteristicID = supportedTransitions[i].characteristicID;
        adaptiveLight->supportedTransitions[i].transitionTypes = supportedTransitions[i].transitionTypes;
    }

    // App callbacks
    adaptiveLight->callbacks.handleCharacteristicValueUpdate = callbacks->handleCharacteristicValueUpdate;
    adaptiveLight->callbacks.handleTransitionExpiry = callbacks->handleTransitionExpiry;
    adaptiveLight->callbacks.handleCharacteristicValueRequest = callbacks->handleCharacteristicValueRequest;

    // Fetch transition data from persistent memory, if any
    LoadTransitionsFromPersistentMemory();

    HAPError err = UpdateTransitionTimer();
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s Failed to update transition timer", __func__);
        return err;
    }

#if (CLOCK_PERSISTS_TIME == 0)
    err = HAPPlatformTimerRegister(
            &(adaptiveLight->schedule.persistClockTimer),
            HAPPlatformClockGetCurrent() + kPersistClockTimeout,
            HandlePersistTime,
            NULL);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to register persist clock timer");
        return err;
    }
#endif

    return kHAPError_None;
}

static void CalculateTransitionValue(AdaptiveLightTransition* t) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(t);
    AdaptiveLightTransitionPoint* activePoint = t->points[t->activePointID];

    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();

    if (nowNlTime > activePoint->endTime) {
        nowNlTime = activePoint->endTime;
    }

    // If point is supposed to executed immediately, the start and end time
    // must be set to the current time (nowNLTime)
    if (activePoint->executeImmediately) {
        activePoint->startTime = nowNlTime;
        activePoint->endTime = nowNlTime;
    }

    if (t->transitionType == kAdaptiveLightTransitionType_Linear) {
        int32_t newVal = activePoint->target - (activePoint->endTime - nowNlTime) * activePoint->rate;
        if (t->startCondition == kHAPCharacteristicValue_TransitionControl_StartCondition_None ||
            (t->startCondition == kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends &&
             newVal > t->threshold) ||
            (t->startCondition == kHAPCharacteristicValue_TransitionControl_StartCondition_Descends &&
             newVal < t->threshold)) {
            t->currentValue = newVal;
            // Once threshold is met, reset Start Condition to 'SetAlways'.
            t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        }
    } else if (t->transitionType == kAdaptiveLightTransitionType_LinearDerived) {
        AdaptiveLightTransition* sourceTransition = GetTransition(t->sourceCharacteristicID);
        int32_t sourceVal = 0;
        if (sourceTransition) {
            sourceVal = sourceTransition->currentValue;
        } else {
            adaptiveLight->callbacks.handleCharacteristicValueRequest(t->sourceCharacteristicID, &sourceVal);
        }
        if (sourceVal < t->lowerValue) {
            sourceVal = t->lowerValue;
        } else if (sourceVal > t->upperValue) {
            sourceVal = t->upperValue;
        }

        float s2 = activePoint->scale;
        float c2 = activePoint->offset;
        float t2 = activePoint->completionDuration;

        if (t->activePointID > 0 && activePoint->completionDuration) {
            float s1 = 0;
            float c1 = 0;
            float b = sourceVal;
            AdaptiveLightTransitionPoint* prevPoint = t->points[t->activePointID - 1];
            HAPAssert(prevPoint);
            s1 = prevPoint->scale;
            c1 = prevPoint->offset;
            float m = (s2 * b + c2 - s1 * b - c1) / (t2);
            float t_current = (nowNlTime - activePoint->startTime);
            t->currentValue = round(m * t_current + (s1 * b + c1));
        } else {
            t->currentValue = round((sourceVal * activePoint->scale) + activePoint->offset);
        }
    }

    // Point has already been executed
    activePoint->executeImmediately = false;
}

static void CalculateAndUpdateTransitionValue(AdaptiveLightTransition* transition) {
    HAPPrecondition(transition);
    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    int32_t newVal = 0;
    int32_t oldVal = transition->lastNotifiedCharacteristicValue;
    int32_t delta = 0;
    bool sendNotification = false;

    CalculateTransitionValue(transition);
    newVal = transition->currentValue;
    delta = newVal - oldVal;
    delta *= ((delta < 0) ? -1 : 1);

    if (delta >= transition->valueChangeThresholdForNotification && transition->nextNotificationAt <= nowNlTime) {
        transition->nextNotificationAt = nowNlTime + transition->timeIntervalThresholdForNotification;
        sendNotification = true;
        transition->lastNotifiedCharacteristicValue = transition->currentValue;
    }
    adaptiveLight->callbacks.handleCharacteristicValueUpdate(
            transition->characteristicID, transition->currentValue, sendNotification);
}

/**
 * Executes expected behavior when all the points in a transition have been iterated over.
 * If no change behavior - calculate and notify the registered callback of the final value.
 * If loop  behavior - starts transition over at the beginning.
 *
 * @param expiredTransition         The transition which has expired.
 *
 * @return kHAPError_None            When successful.
 * @return kHAPError_InvalidData     On error.
 */
static HAPError ProcessTransitionExpiry(AdaptiveLightTransition* expiredTransition) {
    HAPPrecondition(expiredTransition);
    HAPError err = kHAPError_None;
    // The transition is completed. Handle this based on transition's End Behavior
    if (expiredTransition->endBehavior == kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange) {
        uint64_t charID = expiredTransition->characteristicID;
        // Calculate and notify final value at transition expiry
        expiredTransition->activePointID--;
        CalculateTransitionValue(expiredTransition);
        adaptiveLight->callbacks.handleCharacteristicValueUpdate(charID, expiredTransition->currentValue, true);
        RemoveTransition(charID);
        adaptiveLight->callbacks.handleTransitionExpiry();
    } else if (expiredTransition->endBehavior == kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop) {
        // Reset active point index to 0
        expiredTransition->activePointID = 0;
        // Update start and end times for each point
        err = UpdateTransitionPointTimes(expiredTransition);
    }
    return err;
}

void UpdateDerivedTransitionValue(uint64_t sourceID) {
    HAPPrecondition(adaptiveLight);

    AdaptiveLightTransition* transition = NULL;

    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        if (adaptiveLight->transitions[i].transitionType == kAdaptiveLightTransitionType_LinearDerived &&
            adaptiveLight->transitions[i].sourceCharacteristicID == sourceID &&
            adaptiveLight->transitions[i].numPoints) {
            transition = &(adaptiveLight->transitions[i]);
        }
    }
    if (!transition) {
        // No derived transition found
        return;
    }

    AdaptiveLightTransitionPoint* activePoint = transition->points[transition->activePointID];

    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    if (transition->activePointID == 0 && nowNlTime < activePoint->startTime) {
        // Skip updating derived characteristic as its transition is not active yet
        return;
    }

    CalculateTransitionValue(transition);
    adaptiveLight->callbacks.handleCharacteristicValueUpdate(
            transition->characteristicID, transition->currentValue, true);
}

/**
 * Determine if active point is valid to executed immediately
 *
 * Note: Only active point able to execute immediately is the first point.
 *
 * A point that is flagged to execute immediately is in the valid time range if:
 * 1. There is more than 1 point in the transition
 * 2. The endTime of the second point has not yet been reached
 *
 * @param currentTransition   The transition to evaluate
 * @param nowNlTime           The current time
 *
 * @return bool    True if in the valid time range, false if it was not
 */
static bool
        ExecuteImmediatelyActivePointInValidTimeRange(AdaptiveLightTransition* currentTransition, HAPTime nowNlTime) {
    HAPAssert(currentTransition);
    HAPAssert(currentTransition->activePointID == 0);

    size_t secondPointID = 1;
    if (currentTransition->numPoints > 1 && currentTransition->points[secondPointID]->endTime > nowNlTime) {
        return true;
    } else {
        return false;
    }
}

HAP_RESULT_USE_CHECK
HAPError UpdateTransitionTimer(void) {
    HAPPrecondition(adaptiveLight);

    HAPError err = kHAPError_None;

    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    HAPTime nextTransitionUpdateAt = UINT64_MAX;

    if (adaptiveLight->schedule.timer) {
        HAPPlatformTimerDeregister(adaptiveLight->schedule.timer);
        adaptiveLight->schedule.timer = 0;
    }

    if (adaptiveLight->numTransitions == 0) {
        return err;
    }

    // Find active point for each transition
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        AdaptiveLightTransition* currentTransition = &(adaptiveLight->transitions[i]);
        if (currentTransition->numPoints == 0) {
            // If there are no points, there is no active point for the transition.
            continue;
        }

        // Retrieve the transition's currently selected active point.
        AdaptiveLightTransitionPoint* activePoint = currentTransition->points[currentTransition->activePointID];
        if (activePoint == NULL) {
            // If the selected active point is NULL, the data is in an invalid state.
            HAPLogError(
                    &kHAPLog_Default,
                    "%s: Unable to process transition expiry due to invalid data. Unable to find active point "
                    "for transition.",
                    __func__);
            HAPAssert(activePoint);
            return kHAPError_InvalidData;
        }
        while (nowNlTime > activePoint->endTime) {
            // The correct point has been selected if it needs to execute immediately,
            // and if it is in the valid timerange
            if (activePoint->executeImmediately &&
                ExecuteImmediatelyActivePointInValidTimeRange(currentTransition, nowNlTime)) {
                break;
            }
            currentTransition->activePointID++;
            activePoint = currentTransition->points[currentTransition->activePointID];
            if (currentTransition->activePointID >= currentTransition->numPoints) {
                bool isLoop =
                        (currentTransition->endBehavior == kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop);
                err = ProcessTransitionExpiry(currentTransition);
                if (err) {
                    // Prevents infinite while loop if the endTime is not updated in ProcessTransitionExpiry()
                    HAPAssert(err == kHAPError_InvalidData);
                    HAPLogError(
                            &kHAPLog_Default,
                            "%s: Unable to process transition expiry due to invalid data. Unable to find active point "
                            "for transition.",
                            __func__);
                    return err;
                }
                if (isLoop) {
                    // The active point ID is reset to 0
                    activePoint = currentTransition->points[currentTransition->activePointID];
                    HAPAssert(activePoint);
                    continue;
                } else {
                    break;
                }
            }
        } // while

    } // for

    // Calculate the current value for each transition
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        AdaptiveLightTransition* currentTransition = &(adaptiveLight->transitions[i]);
        if (currentTransition->numPoints == 0) {
            continue;
        }
        AdaptiveLightTransitionPoint* activePoint = currentTransition->points[currentTransition->activePointID];
        if (activePoint->executeImmediately ||
            (currentTransition->nextTransitionUpdateAt <= nowNlTime && activePoint->startTime <= nowNlTime &&
             nowNlTime <= activePoint->endTime)) {
            // The value should be updated using this active point if:
            // 1. The point is flagged that it should execute immediately
            // OR
            // 2a. It is not past the time for the next transition update (to the next point)
            //      AND
            // 2b. The present time is equal to or between the start/end time of the active point
            currentTransition->nextTransitionUpdateAt = nowNlTime + currentTransition->updateInterval;
            CalculateAndUpdateTransitionValue(currentTransition);
        } else if (nowNlTime < activePoint->startTime) {
            currentTransition->nextTransitionUpdateAt = activePoint->startTime;
        } else if (activePoint->endTime < nowNlTime) {
            HAPAssert(activePoint->endTime >= nowNlTime);
            continue;
        }
        nextTransitionUpdateAt = HAPMin(nextTransitionUpdateAt, currentTransition->nextTransitionUpdateAt);
    }

    // If there are transitions present, set a timer to indicate when the next transition should occur.
    if (adaptiveLight->numTransitions) {
        // If the time for the next transition hasn't be set, don't start a timer.
        if (nextTransitionUpdateAt == UINT64_MAX) {
            HAPAssert(nextTransitionUpdateAt != UINT64_MAX);
            return kHAPError_InvalidData;
        }

        // Make sure we do not accidentally schedule a timer in the past.
        if (nextTransitionUpdateAt < nowNlTime) {
            HAPAssert(nextTransitionUpdateAt >= nowNlTime);
            return kHAPError_InvalidData;
        }
        // Register a timer relative to system time.
        err = HAPPlatformTimerRegister(
                &(adaptiveLight->schedule.timer),
                NL_TO_SYSTEM_TIME(nextTransitionUpdateAt),
                HandleTransitionTimerExpired,
                NULL);
    }

    return err;
}

static void HandleTransitionTimerExpired(HAPPlatformTimerRef timer HAP_UNUSED, void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(adaptiveLight);
    HAPError err = kHAPError_None;
    if (adaptiveLight->schedule.timer) {
        adaptiveLight->schedule.timer = 0;
    }

    err = UpdateTransitionTimer();
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s Failed to update transition timer", __func__);
    }
}

HAP_RESULT_USE_CHECK
HAPError EnumerateSupportedTransitions(
        HAPSequenceTLVDataSource* dataSource_ HAP_UNUSED,
        HAPCharacteristicValueCallback_SupportedTransitions callback,
        void* _Nullable context) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(callback);

    HAPError err = kHAPError_None;
    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < adaptiveLight->numSupportedTransitions; i++) {
        HAPCharacteristicValue_SupportedTransition transition;
        HAPRawBufferZero(&transition, sizeof transition);
        if (!IsCharacteristicSupportedByService(
                    adaptiveLight->supportedTransitions[i].characteristicID, adaptiveLight->context.service)) {
            continue;
        }
        CONVERT_DATA(transition.HAPInstanceID, adaptiveLight->supportedTransitions[i].characteristicID);
        transition.types = adaptiveLight->supportedTransitions[i].transitionTypes;

        callback(context, &transition, &shouldContinue);
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError EnumerateActiveContext(
        HAPSequenceTLVDataSource* dataSource_ HAP_UNUSED,
        HAPCharacteristicValueCallback_TransitionState_ActiveContext callback,
        void* _Nullable context) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(callback);

    HAPError err = kHAPError_None;
    bool shouldContinue = true;
    HAPTime nowNlTime = GET_ADAPTIVE_LIGHT_TIME();
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext activeContext;
        HAPRawBufferZero(&activeContext, sizeof activeContext);
        AdaptiveLightTransition* transition = &(adaptiveLight->transitions[i]);
        if (transition->serviceID != adaptiveLight->context.service->iid) {
            continue;
        }
        CONVERT_DATA(activeContext.HAPInstanceID, transition->characteristicID);
        activeContext.context.numBytes = transition->controllerContext.numBytes;
        activeContext.context.bytes = transition->controllerContext.bytes;
        uint64_t timeElapsed = nowNlTime - transition->requestTime;
        CONVERT_DATA(activeContext.timeElapsedSinceStart, timeElapsed);

        callback(context, &activeContext, &shouldContinue);
    }
    return err;
}

static bool IsLinearTransition(const AdaptiveLightTransition* transition) {
    HAPPrecondition(transition);

    if (transition->transitionType == kAdaptiveLightTransitionType_Linear) {
        return true;
    }
    return false;
}

static bool IsLinearDerivedTransition(const AdaptiveLightTransition* transition) {
    HAPPrecondition(transition);

    if (transition->transitionType == kAdaptiveLightTransitionType_LinearDerived) {
        return true;
    }
    return false;
}

HAP_RESULT_USE_CHECK
static HAPError ValidateTransition(const AdaptiveLightTransition* transition, const HAPService* _Nullable service) {
    HAPPrecondition(transition);

    if (service != NULL && !IsCharacteristicSupportedByService(transition->characteristicID, service)) {
        HAPLogError(
                &kHAPLog_Default,
                "%s this characteristic iid= 0x%llx is not part of the requested service",
                __func__,
                (unsigned long long) transition->characteristicID);
        return kHAPError_InvalidData;
    }
    if (!IsSupported(transition->characteristicID)) {
        HAPLogError(&kHAPLog_Default, "%s this characteristic doesn't support transitions", __func__);
        return kHAPError_InvalidData;
    }
    if (IsLinearTransition(transition) &&
        !IsTransitionTypeSupported(transition->characteristicID, kAdaptiveLightTransitionType_Linear)) {
        HAPLogError(&kHAPLog_Default, "%s this characteristic doesn't support linear transitions", __func__);
        return kHAPError_InvalidData;
    }

    if (IsLinearDerivedTransition(transition) &&
        !IsTransitionTypeSupported(transition->characteristicID, kAdaptiveLightTransitionType_LinearDerived)) {
        HAPLogError(&kHAPLog_Default, "%s this characteristic doesn't support linear derived transitions", __func__);
        return kHAPError_InvalidData;
    }

    if (service != NULL && IsLinearTransition(transition) &&
        !ValidateLinearTransitionTargetValues(transition, service)) {
        HAPLogError(&kHAPLog_Default, "%s: Incorrect target values", __func__);
        return kHAPError_InvalidData;
    }

    if (service != NULL && IsLinearDerivedTransition(transition) &&
        !IsInt32CharacteristicValueRangeValid(
                transition->sourceCharacteristicID, service, transition->lowerValue, transition->upperValue)) {
        HAPLogError(&kHAPLog_Default, "%s: Incorrect source bounds", __func__);
        return kHAPError_InvalidData;
    }

    // If Linear Transition, check Start Condition to ensure it is set to a valid enum value.
    if (IsLinearTransition(transition) &&
        ((transition->startCondition != kHAPCharacteristicValue_TransitionControl_StartCondition_None) &&
         (transition->startCondition != kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends) &&
         (transition->startCondition != kHAPCharacteristicValue_TransitionControl_StartCondition_Descends))) {
        HAPLogError(
                &kHAPLog_Default,
                "%s this characteristic has an invalid Start Condition enum value of %u.",
                __func__,
                (uint8_t)(transition->startCondition));
        return kHAPError_InvalidData;
    }

    // Check End Behavior to ensure it is set to a valid enum value.
    if (transition->endBehavior != kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange &&
        transition->endBehavior != kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop) {
        HAPLogError(
                &kHAPLog_Default,
                "%s this characteristic has an invalid End Behavior enum value of %u.",
                __func__,
                (uint8_t)(transition->endBehavior));
        return kHAPError_InvalidData;
    }

    const AdaptiveLightTransition* currentTransition = GetTransition(transition->characteristicID);

    // Calculate available points in persistent storage
    size_t availablePoints = kLightbulb_MaxSupportedTransitionPoints - adaptiveLight->numTransitionPoints;
    // If a transition for given characteristic already exists, those points would be released
    if (currentTransition) {
        availablePoints += currentTransition->numPoints;
    }

    if (transition->numPoints > availablePoints) {
        HAPLogError(&kHAPLog_Default, "%s: Insufficient storage for Transition points", __func__);
        HAPLogError(
                &kHAPLog_Default,
                "%s: Insufficient storage for Transition points available = %zu required = %zu",
                __func__,
                availablePoints,
                transition->numPoints);
        return kHAPError_OutOfResources;
    }
    uint64_t prevStartTime = 0;
    uint64_t prevEndTime = 0;
    for (size_t i = 0; i < transition->numPoints; i++) {
        if (transition->points[i]) {
            // Validate start and end time
            if (prevStartTime > transition->points[i]->startTime) {
                HAPLogError(&kHAPLog_Default, "%s: startTime for Point#%zu greater than prev startTime", __func__, i);
                return kHAPError_InvalidData;
            } else if (prevEndTime > transition->points[i]->startTime) {
                HAPLogError(&kHAPLog_Default, "%s: startTime for Point#%zu greater than prev endTime", __func__, i);
                return kHAPError_InvalidData;
            } else if (transition->points[i]->startTime > transition->points[i]->endTime) {
                HAPLogError(&kHAPLog_Default, "%s: startTime for Point#%zu greater than endTime", __func__, i);
                return kHAPError_InvalidData;
            } else if (prevEndTime > transition->points[i]->endTime) {
                HAPLogError(&kHAPLog_Default, "%s: endTime for Point#%zu greater than previous endTime", __func__, i);
                return kHAPError_InvalidData;
            }
            prevStartTime = transition->points[i]->startTime;
            prevEndTime = transition->points[i]->endTime;

            // Validate target completion duration.
            // @see HomeKit Accessory Protocol Specification R17
            // Section 14.3.3.2 Transitions Procedure Reference Flow
            if (transition->points[i]->completionDuration < kLightbulb_MinTargetCompletionDuration) {
                if (i == 0 && transition->points[i]->completionDuration == 0) {
                    // Note: The first Transition Point can have a target completion duration of 0 ms.
                    continue;
                }
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Target Completion Duration for Point#%zu is %llu ms. Must be greater than %llu ms.",
                        __func__,
                        i,
                        (unsigned long long) transition->points[i]->completionDuration,
                        (unsigned long long) kLightbulb_MinTargetCompletionDuration);
                return kHAPError_InvalidData;
            }
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError SaveTransition(const AdaptiveLightTransition* ref) {
    HAPPrecondition(ref);

    HAPError err = kHAPError_None;
    AdaptiveLightTransition* transition = GetTransition(ref->characteristicID);
    if (transition) {
        ReleaseTransition(transition);
    }

    // If proposed transition has zero points, return immediately.
    if (ref->numPoints == 0) {
        HAPLogInfo(&kHAPLog_Default, "Erasing transition for id %llu", (unsigned long long) ref->characteristicID);
        return err;
    }

    transition = AllocTransition();
    if (!transition) {
        HAPLogError(&kHAPLog_Default, "%s no space left for new transition", __func__);
        return kHAPError_OutOfResources;
    }

    transition->characteristicID = ref->characteristicID;
    transition->serviceID = ref->serviceID;
    transition->transitionType = ref->transitionType;
    transition->sourceCharacteristicID = ref->sourceCharacteristicID;
    transition->lowerValue = ref->lowerValue;
    transition->upperValue = ref->upperValue;
    transition->lowerValueBufferLen = ref->lowerValueBufferLen;
    transition->upperValueBufferLen = ref->upperValueBufferLen;
    transition->numPoints = ref->numPoints;
    transition->threshold = ref->threshold;
    transition->currentValue = ref->currentValue;
    transition->startCondition = ref->startCondition;
    transition->endBehavior = ref->endBehavior;
    transition->requestTime = ref->requestTime;
    transition->updateInterval = ref->updateInterval;
    transition->timeIntervalThresholdForNotification = ref->timeIntervalThresholdForNotification;
    transition->valueChangeThresholdForNotification = ref->valueChangeThresholdForNotification;
    transition->lastNotifiedCharacteristicValue = ref->currentValue;
    transition->nextTransitionUpdateAt = 0;
    transition->nextNotificationAt = 0;
    transition->controllerContext.numBytes = ref->controllerContext.numBytes;
    transition->activePointID = 0;
    HAPRawBufferCopyBytes(
            transition->controllerContext.bytes, ref->controllerContext.bytes, ref->controllerContext.numBytes);

    for (size_t i = 0; i < ref->numPoints; i++) {
        AdaptiveLightTransitionPoint* pt = AllocTransitionPoint();
        if (!pt) {
            HAPLogError(&kHAPLog_Default, "%s no space left for new transition point", __func__);
            return kHAPError_OutOfResources;
        }
        HAPRawBufferCopyBytes(pt, ref->points[i], sizeof(AdaptiveLightTransitionPoint));
        transition->points[i] = pt;
    }

    PrintTransition(transition);
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError ParseLinearTransition(HAPTLVReader* requestReader, AdaptiveLightTransition* transition) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(requestReader);
    HAPPrecondition(transition);
    bool isStartCondSet = false;

    HAPError err = kHAPError_None;
    for (;;) {
        bool found;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        if (!found) {
            break;
        }
        if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_LinearTransition_Points) {
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            HAPCharacteristicValue_TransitionControl_LinearTransition_Point linearPoint;
            err = HAPTLVReaderDecode(
                    &subReader, &kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points, &linearPoint);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }

            AdaptiveLightTransitionPoint* point = NULL;
            point = AllocTempTransitionPoint();
            if (!point) {
                HAPLogError(&kHAPLog_Default, "%s no space left for new transition point in temp storage", __func__);
                err = kHAPError_OutOfResources;
                return err;
            }

            point->target = ConvertToInt32(&(linearPoint.targetValue));
            // This values is saved so we can expand the data back to the size of the original incoming buffer.
            point->targetBufferLen = linearPoint.targetValue.numBytes;
            if (point->targetBufferLen > sizeof(int32_t)) {
                HAPLogError(&kHAPLog_Default, "%s: Target Buffer data must be 4 bytes or less in length.", __func__);
                return kHAPError_InvalidData;
            }

            if (linearPoint.completionDurationIsSet) {
                point->completionDuration = ConvertToUInt64(&(linearPoint.completionDuration));
            }
            if (linearPoint.startDelayDurationIsSet) {
                point->startDelayDuration = ConvertToUInt64(&(linearPoint.startDelayDuration));
            }
            point->startTime = 0;
            point->endTime = 0;

            // Add this point to the transition
            transition->points[transition->numPoints] = point;
            transition->numPoints += 1;

        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_LinearTransition_StartCondition) {
            if (isStartCondSet) {
                HAPLogError(&kHAPLog_Default, "%s: Start condition already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isStartCondSet = true;
            if (tlv.value.numBytes != sizeof(uint8_t)) {
                HAPLogError(
                        &kHAPLog_Default, "%s: StartCondition has invalid length %zu.", __func__, tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            transition->startCondition = HAPReadUInt8(tlv.value.bytes);
        } else if (tlv.value.numBytes == 0) {
            HAPLogDebug(&kHAPLog_Default, "%s: Zero-Length separator detected. Skipping.", __func__);
        } else {
            HAPLogDebug(&kHAPLog_Default, "%s: Unknown TLV type = 0x%02X; ignoring", __func__, tlv.type);
        }
    }
    if (!isStartCondSet) {
        HAPLogError(&kHAPLog_Default, "%s: Start Condition not set", __func__);
        err = kHAPError_InvalidData;
    }
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError ParseLinearDerivedTransition(HAPTLVReader* requestReader, AdaptiveLightTransition* transition) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(requestReader);
    HAPPrecondition(transition);
    bool isSourceIDSet = false;
    bool isSourceRangeSet = false;

    HAPError err = kHAPError_None;
    for (;;) {
        bool found;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        if (!found) {
            break;
        }
        if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_Points) {
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point derivedPoint;
            err = HAPTLVReaderDecode(
                    &subReader,
                    &kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points,
                    &derivedPoint);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }

            AdaptiveLightTransitionPoint* point = NULL;
            point = AllocTempTransitionPoint();
            if (!point) {
                HAPLogError(&kHAPLog_Default, "%s no space left for new transition point in temp storage", __func__);
                err = kHAPError_OutOfResources;
                return err;
            }

            point->scale = HAPFloatFromBitPattern(derivedPoint.scale);
            point->offset = HAPFloatFromBitPattern(derivedPoint.offset);

            if (derivedPoint.completionDurationIsSet) {
                point->completionDuration = ConvertToUInt64(&(derivedPoint.completionDuration));
            }
            if (derivedPoint.startDelayDurationIsSet) {
                point->startDelayDuration = ConvertToUInt64(&(derivedPoint.startDelayDuration));
            }

            point->startTime = 0;
            point->endTime = 0;

            // Add this point to the transition
            transition->points[transition->numPoints] = point;
            transition->numPoints += 1;

        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceID) {
            if (isSourceIDSet) {
                HAPLogError(&kHAPLog_Default, "%s: Source ID already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isSourceIDSet = true;
            if (tlv.value.numBytes > sizeof transition->sourceCharacteristicID) {
                HAPLogError(&kHAPLog_Default, "%s: Source ID has invalid length %zu.", __func__, tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPCharacteristicValue_VariableLengthInteger id;
            id.numBytes = tlv.value.numBytes;
            id.bytes = tlv.value.bytes;
            transition->sourceCharacteristicID = ConvertToUInt64(&id);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceRange) {
            if (isSourceRangeSet) {
                HAPLogError(&kHAPLog_Default, "%s: Source Range already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isSourceRangeSet = true;
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            HAPCharacteristicValue_TransitionControl_SourceValueRange range;
            err = HAPTLVReaderDecode(
                    &subReader, &kHAPCharacteristicTLVFormat_TransitionControl_SourceValueRange, &range);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            transition->lowerValue = ConvertToInt32(&(range.lower));
            transition->upperValue = ConvertToInt32(&(range.upper));
            // These values are saved so we can expand the data back to the size of the original incoming buffer.
            transition->lowerValueBufferLen = range.lower.numBytes;
            transition->upperValueBufferLen = range.upper.numBytes;
            if (range.upper.numBytes > sizeof(int32_t) || range.lower.numBytes > sizeof(int32_t)) {
                HAPLogError(&kHAPLog_Default, "%s: Source Range data must be 4 bytes or less in length.", __func__);
                return kHAPError_InvalidData;
            }
        } else if (tlv.value.numBytes == 0) {
            HAPLogDebug(&kHAPLog_Default, "%s: Zero-Length separator detected. Skipping.", __func__);
        } else {
            HAPLogDebug(&kHAPLog_Default, "%s: Unknown TLV type = 0x%02X; ignoring", __func__, tlv.type);
        }
    }
    if (!isSourceIDSet) {
        HAPLogError(&kHAPLog_Default, "%s: Source ID not set", __func__);
        err = kHAPError_InvalidData;
    }
    if (!isSourceRangeSet) {
        HAPLogError(&kHAPLog_Default, "%s: Source range not set", __func__);
        err = kHAPError_InvalidData;
    }

    return err;
}

HAP_RESULT_USE_CHECK
static HAPError ParseTransition(HAPTLVReader* requestReader, AdaptiveLightTransition* transition) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(requestReader);
    bool isIDSet = false;
    bool isEndBehaviorSet = false;
    bool isTransitionSet = false;
    bool isUpdateIntervalSet = false;
    bool isNotifyValueChangeThresholdSet = false;
    bool isNotifyTimeIntervalThresholdSet = false;
    HAPError err = kHAPError_None;

    if (!transition) {
        err = kHAPError_InvalidData;
        return err;
    }

    for (;;) {
        bool found;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        if (!found) {
            break;
        }
        if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_HAPID) {
            if (isIDSet) {
                HAPLogError(&kHAPLog_Default, "%s: HAP ID already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isIDSet = true;
            if (tlv.value.numBytes > sizeof transition->characteristicID) {
                HAPLogError(
                        &kHAPLog_Default, "%s: HAP Instance ID has invalid length %zu.", __func__, tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPCharacteristicValue_VariableLengthInteger id;
            id.numBytes = tlv.value.numBytes;
            id.bytes = tlv.value.bytes;
            transition->characteristicID = ConvertToUInt64(&id);
            int32_t currentValue = 0;
            adaptiveLight->callbacks.handleCharacteristicValueRequest(transition->characteristicID, &currentValue);
            transition->currentValue = currentValue;
            // If we are restoring from persistent memory, use threshold value from restore context
            if (adaptiveLight->context.restoreContext) {
                transition->threshold = adaptiveLight->context.restoreContext->threshold;
            } else {
                transition->threshold = currentValue;
            }
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_ControllerContext) {
            if (tlv.value.numBytes > kLightbulb_MaxControllerContextSize) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Controller Context has invalid length %zu.",
                        __func__,
                        tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            transition->controllerContext.numBytes = tlv.value.numBytes;
            HAPRawBufferCopyBytes(transition->controllerContext.bytes, tlv.value.bytes, tlv.value.numBytes);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_EndBehavior) {
            if (isEndBehaviorSet) {
                HAPLogError(&kHAPLog_Default, "%s: EndBehavior already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isEndBehaviorSet = true;
            if (tlv.value.numBytes != sizeof(uint8_t)) {
                HAPLogError(&kHAPLog_Default, "%s: EndBehavior has invalid length %zu.", __func__, tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            transition->endBehavior = HAPReadUInt8(tlv.value.bytes);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_Linear) {
            if (isTransitionSet) {
                HAPLogError(&kHAPLog_Default, "%s: Transition already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isTransitionSet = true;
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            err = ParseLinearTransition(&subReader, transition);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to parse linear transition points", __func__);
                return err;
            }
            transition->transitionType = kAdaptiveLightTransitionType_Linear;
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_LinearDerived) {
            if (isTransitionSet) {
                HAPLogError(&kHAPLog_Default, "%s: Transition already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isTransitionSet = true;
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            err = ParseLinearDerivedTransition(&subReader, transition);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to parse linear derived transition points", __func__);
                return err;
            }
            transition->transitionType = kAdaptiveLightTransitionType_LinearDerived;
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_ValueUpdateTimeInterval) {
            if (isUpdateIntervalSet) {
                HAPLogError(&kHAPLog_Default, "%s: Update Interval already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isUpdateIntervalSet = true;
            if (tlv.value.numBytes > sizeof transition->updateInterval) {
                HAPLogError(
                        &kHAPLog_Default, "%s: Update Interval has invalid length %zu.", __func__, tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPCharacteristicValue_VariableLengthInteger updateInterval;
            updateInterval.numBytes = tlv.value.numBytes;
            updateInterval.bytes = tlv.value.bytes;
            transition->updateInterval = ConvertToUInt64(&updateInterval);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyValueChangeThreshold) {
            if (isNotifyValueChangeThresholdSet) {
                HAPLogError(&kHAPLog_Default, "%s: Notify Value Change Threshold already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isNotifyValueChangeThresholdSet = true;
            if (tlv.value.numBytes > sizeof transition->valueChangeThresholdForNotification) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Notify Value Change Threshold has invalid length %zu.",
                        __func__,
                        tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPCharacteristicValue_VariableLengthInteger valueThreshold;
            valueThreshold.numBytes = tlv.value.numBytes;
            valueThreshold.bytes = tlv.value.bytes;
            transition->valueChangeThresholdForNotification = ConvertToInt32(&valueThreshold);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyTimeIntervalThreshold) {
            if (isNotifyTimeIntervalThresholdSet) {
                HAPLogError(&kHAPLog_Default, "%s: Notify Time Interval Threshold already set", __func__);
                err = kHAPError_InvalidData;
                return err;
            }
            isNotifyTimeIntervalThresholdSet = true;
            if (tlv.value.numBytes > sizeof transition->timeIntervalThresholdForNotification) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Notify Time Interval Threshold has invalid length %zu.",
                        __func__,
                        tlv.value.numBytes);
                return kHAPError_InvalidData;
            }
            HAPCharacteristicValue_VariableLengthInteger intervalThreshold;
            intervalThreshold.numBytes = tlv.value.numBytes;
            intervalThreshold.bytes = tlv.value.bytes;
            transition->timeIntervalThresholdForNotification = ConvertToUInt64(&intervalThreshold);
        }
    }
    if (!isIDSet) {
        HAPLogError(&kHAPLog_Default, "%s: Source ID not set", __func__);
        err = kHAPError_InvalidData;
        return err;
    }
    if (!isUpdateIntervalSet) {
        // Set the default value
        transition->updateInterval = kLightbulb_TransitionUpdateInterval;
    }
    if (!isNotifyValueChangeThresholdSet) {
        // Set the default value
        transition->valueChangeThresholdForNotification = 0;
    }
    if (!isNotifyTimeIntervalThresholdSet) {
        // Set the default value
        transition->timeIntervalThresholdForNotification = transition->updateInterval;
    }
    // Once complete transition is read, calibrate start/end times for each transition point
    err = UpdateTransitionPointTimes(transition);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Failed to update transition point times", __func__);
        return err;
    }
    // Update linear transition rate as current value would be available by now
    UpdateLinearTransitionRate(transition);
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError ParseTransitions(HAPTLVReader* requestReader, const HAPService* service) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(requestReader);
    HAPPrecondition(service);

    HAPError err = kHAPError_None;
    for (;;) {
        bool found;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        if (!found) {
            break;
        }
        if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Start_Transitions) {

            if (tlv.value.numBytes > kLightbulb_MaxTransitionTlvSize) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s: Unsupported transition length %zu. Max supported length is %zu",
                        __func__,
                        tlv.value.numBytes,
                        kLightbulb_MaxTransitionTlvSize);
                return kHAPError_OutOfResources;
            }

            uint8_t tempTransitionTLV[kLightbulb_MaxTransitionTlvSize];
            HAPRawBufferCopyBytes(tempTransitionTLV, tlv.value.bytes, tlv.value.numBytes);

            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            AdaptiveLightTransition* transition = AllocTempTransition();
            if (!transition) {
                HAPLogError(&kHAPLog_Default, "%s: No temp space to allocate transition", __func__);
                return kHAPError_OutOfResources;
            }
            transition->requestTime = GET_ADAPTIVE_LIGHT_TIME();
            transition->activePointID = 0;
            transition->serviceID = service->iid;
            err = ParseTransition(&subReader, transition);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to parse transition", __func__);
                ReleaseTransition(transition);
                return err;
            }
            err = ValidateTransition(transition, service);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to validate transition", __func__);
                ReleaseTransition(transition);
                return err;
            }
            err = SaveTransition(transition);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to accept transition", __func__);
                ReleaseTransition(transition);
                return err;
            }
            err = RemoveCharacteristicTransitionFromPersistentMemory(transition->characteristicID);
            if (err) {
                HAPLogError(
                        &kHAPLog_Default,
                        "All data may not have been erased from persistent memory for transition for id %llu",
                        (unsigned long long) transition->characteristicID);
            }
            SaveTransitionsToPersistentMemory(tempTransitionTLV, tlv.value.numBytes, transition->characteristicID);
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s: Failed to write transition data to persistent memory", __func__);
            }
            ReleaseTransition(transition);
        } else if (tlv.value.numBytes == 0) {
            HAPLogDebug(&kHAPLog_Default, "%s: Zero-Length separator detected. Skipping.", __func__);
        } else {
            HAPLogDebug(&kHAPLog_Default, "%s: Unknown TLV type = 0x%02X; ignoring", __func__, tlv.type);
        }
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleSupportedTransitionConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(adaptiveLight);
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedTransitionConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_LightBulb));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err = kHAPError_None;
    adaptiveLight->context.service = request->service;

    HAPCharacteristicValue_SupportedTransitionConfigurations value;
    HAPRawBufferZero(&value, sizeof value);

    value.enumerate = EnumerateSupportedTransitions;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_SupportedTransitionConfigurations, &value);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
    }
    adaptiveLight->context.service = NULL;
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionState(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(server);
    HAPPrecondition(request);

    HAPError err = kHAPError_None;
    adaptiveLight->context.service = request->service;

    HAPCharacteristicValue_TransitionState value;
    HAPAssert(adaptiveLight->context.pendingResponseType == kTransitionControlResponseType_TransitionState);

    value.enumerate = EnumerateActiveContext;
    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_TransitionState, &value);
    adaptiveLight->context.service = NULL;
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionTransitionPoint(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(server);
    HAPPrecondition(request);

    HAPError err = kHAPError_None;

    uint64_t queriedHAPInstanceID = adaptiveLight->context.fetchID;
    const AdaptiveLightTransition* _Nullable t = GetTransition(queriedHAPInstanceID);
    HAPAssert(t);
    for (size_t i = 0; i < t->numPoints; i++) {
        const AdaptiveLightTransitionPoint* pt = t->points[i];
        uint8_t transitionTlvType = 0;
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }
        if (t->transitionType == kAdaptiveLightTransitionType_Linear) {
            HAPCharacteristicValue_TransitionControl_LinearTransition_Point point;
            HAPRawBufferZero(&point, sizeof point);

            uint8_t targetByteArray[sizeof(int32_t)];
            uint8_t targetByteArrayLen = 0;

            // Range Lower bound.
            targetByteArrayLen = ConvertInt32ToByteArray(
                    pt->target, pt->targetBufferLen, (uint8_t*) &targetByteArray, sizeof targetByteArray);
            point.targetValue.numBytes = targetByteArrayLen;
            point.targetValue.bytes = &(targetByteArray);

            CONVERT_DATA(point.completionDuration, pt->completionDuration);
            point.completionDurationIsSet = true;

            CONVERT_DATA(point.startDelayDuration, pt->startDelayDuration);
            point.startDelayDurationIsSet = true;

            err = HAPTLVWriterEncode(
                    &subWriter, &kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points, &point);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            transitionTlvType = kHAPCharacteristicTLVType_TransitionControl_LinearTransition_Points;
        } else {
            HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point point;
            HAPRawBufferZero(&point, sizeof point);
            point.scale = HAPFloatGetBitPattern(pt->scale);
            point.offset = HAPFloatGetBitPattern(pt->offset);

            CONVERT_DATA(point.completionDuration, pt->completionDuration);
            point.completionDurationIsSet = true;

            CONVERT_DATA(point.startDelayDuration, pt->startDelayDuration);
            point.startDelayDurationIsSet = true;

            err = HAPTLVWriterEncode(
                    &subWriter, &kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points, &point);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            transitionTlvType = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_Points;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = transitionTlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        // Append Separator
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_Separator,
                                  .value = { .bytes = NULL, .numBytes = 0 } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionTransition(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(server);
    HAPPrecondition(request);

    uint64_t queriedHAPInstanceID = adaptiveLight->context.fetchID;
    const AdaptiveLightTransition* _Nullable t = GetTransition(queriedHAPInstanceID);
    HAPAssert(t);
    HAPError err = kHAPError_None;
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }
        // Write Transition Point
        err = GetTransitionTransitionPoint(&subWriter, server, request);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        uint8_t transitionTlvType = 0;
        if (t->transitionType == kAdaptiveLightTransitionType_Linear) {
            // Write Start Condition
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_TransitionControl_LinearTransition_StartCondition,
                            .value = { .bytes = &(t->startCondition), .numBytes = sizeof t->startCondition } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
            transitionTlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_Linear;
        } else {
            // Write Source Characteristic ID
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceID,
                            .value = { .bytes = &(t->sourceCharacteristicID),
                                       .numBytes = sizeof t->sourceCharacteristicID } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }

            // Write Source Range
            {
                HAPTLVWriter subWriter2;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter2, bytes, maxBytes);
                }
                HAPCharacteristicValue_TransitionControl_SourceValueRange range;

                // Convert the numerical value of the range (upper and lower bound) to the original buffer provided
                // by the controller. This characteristic value is of type Data, and so it is expected to be returned
                // in it's original format.
                uint8_t lowerRangeByteArray[sizeof(int32_t)];
                uint8_t upperRangeByteArray[sizeof(int32_t)];
                uint8_t byteArrayLen = 0;

                // Range Lower bound.
                byteArrayLen = ConvertInt32ToByteArray(
                        t->lowerValue,
                        t->lowerValueBufferLen,
                        (uint8_t*) &lowerRangeByteArray,
                        sizeof lowerRangeByteArray);
                range.lower.numBytes = byteArrayLen;
                range.lower.bytes = &(lowerRangeByteArray);

                // Range Upper bound.
                byteArrayLen = ConvertInt32ToByteArray(
                        t->upperValue,
                        t->upperValueBufferLen,
                        (uint8_t*) &upperRangeByteArray,
                        sizeof upperRangeByteArray);
                range.upper.numBytes = byteArrayLen;
                range.upper.bytes = &(upperRangeByteArray);

                err = HAPTLVWriterEncode(
                        &subWriter2, &kHAPCharacteristicTLVFormat_TransitionControl_SourceValueRange, &range);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter2, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceRange,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
            transitionTlvType = kHAPCharacteristicTLVType_TransitionControl_Transition_LinearDerived;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = transitionTlvType, .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionControlResponseFetch(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(server);
    HAPPrecondition(request);

    HAPError err = kHAPError_None;
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        uint64_t queriedHAPInstanceID = adaptiveLight->context.fetchID;
        // Verify that HAP Instance ID supports transitions.
        // See HomeKit Accessory Protocol Specification R17
        // Section 11.147 Characteristic Value Transition Control
        if (!IsSupported(queriedHAPInstanceID)) {
            HAPLogError(
                    &kHAPLog_Default,
                    "%s this characteristic (id = %llu) doesn't support transitions",
                    __func__,
                    (unsigned long long) queriedHAPInstanceID);
            return kHAPError_Unknown;
        }

        const AdaptiveLightTransition* _Nullable t = GetTransition(queriedHAPInstanceID);

        // Return an empty TLV when there is no configured transition for a characteristic
        // that supports transitions.
        // See HomeKit Accessory Protocol Specification R17
        // Section 11.147 Characteristic Value Transition Control
        if (!t) {
            HAPLogInfo(
                    &kHAPLog_Default,
                    "%s: Sending empty TLV: No transition for id = %llu",
                    __func__,
                    (unsigned long long) queriedHAPInstanceID);
            return kHAPError_None;
        }

        // Write HAP Instance ID
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicTLVType_TransitionControl_Transition_HAPID,
                        .value = { .bytes = &queriedHAPInstanceID, .numBytes = sizeof queriedHAPInstanceID } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Write Controller Context
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicTLVType_TransitionControl_Transition_ControllerContext,
                        .value = { .bytes = t->controllerContext.bytes, .numBytes = t->controllerContext.numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Write End Behavior
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_TransitionControl_Transition_EndBehavior,
                                  .value = { .bytes = &(t->endBehavior), .numBytes = sizeof t->endBehavior } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Write Transition Points
        err = GetTransitionTransition(&subWriter, server, request);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Write Value Update Time Interval
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicTLVType_TransitionControl_Transition_ValueUpdateTimeInterval,
                        .value = { .bytes = &(t->updateInterval), .numBytes = sizeof t->updateInterval } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Write Notify Value Change Threshold
        // As per spec this value must be greater than 0. A value of 0 indicates this optional
        // parameter was not set.
        if (t->valueChangeThresholdForNotification > 0) {
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyValueChangeThreshold,
                            .value = { .bytes = &(t->timeIntervalThresholdForNotification),
                                       .numBytes = sizeof t->timeIntervalThresholdForNotification } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Write Notify Time Interval Threshold
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyTimeIntervalThreshold,
                        .value = { .bytes = &(t->timeIntervalThresholdForNotification),
                                   .numBytes = sizeof t->timeIntervalThresholdForNotification } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_TransitionControlResponse_Transition,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionControlResponseStart(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(server);
    HAPPrecondition(request);

    HAPError err = kHAPError_None;
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }
        err = GetTransitionState(&subWriter, server, request);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicTLVType_TransitionControlResponse_TransitionState,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError GetTransitionControlResponse(
        HAPTLVWriter* responseWriter,
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request) {
    HAPPrecondition(responseWriter);
    HAPError err = kHAPError_None;

    if (adaptiveLight->context.pendingResponseType == kTransitionControlResponseType_Transition) {
        err = GetTransitionControlResponseFetch(responseWriter, server, request);
        adaptiveLight->context.pendingResponseType = kTransitionControlResponseType_TransitionState;
    } else {
        err = GetTransitionControlResponseStart(responseWriter, server, request);
    }
    return err;
}

HAP_RESULT_USE_CHECK
static HAPError RestoreTransition(HAPTLVReader* requestReader) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(requestReader);
    HAPPrecondition(adaptiveLight->context.restoreContext);

    HAPError err = kHAPError_None;

    AdaptiveLightTransition* transition = AllocTempTransition();
    if (!transition) {
        HAPLogError(&kHAPLog_Default, "%s: No temp space to allocate transition", __func__);
        return kHAPError_OutOfResources;
    }
    transition->requestTime = adaptiveLight->context.restoreContext->requestTime;
    transition->serviceID = adaptiveLight->context.restoreContext->serviceID;
    err = ParseTransition(requestReader, transition);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Failed to parse transition", __func__);
        ReleaseTransition(transition);
        return err;
    }
    err = ValidateTransition(transition, NULL);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Failed to validate transition", __func__);
        ReleaseTransition(transition);
        return err;
    }
    err = SaveTransition(transition);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Failed to accept transition", __func__);
        ReleaseTransition(transition);
        return err;
    }
    ReleaseTransition(transition);

    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleTransitionControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(adaptiveLight);

    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_TransitionControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_LightBulb));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_TransitionControlResponse response;
    HAPRawBufferZero(&response, sizeof response);

    if (adaptiveLight->context.session) {
        err = GetTransitionControlResponse(responseWriter, server, request);
        adaptiveLight->context.session = NULL;
    } else {
        err = GetTransitionState(responseWriter, server, request);
    }
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleTransitionControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(adaptiveLight);
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_TransitionControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_LightBulb));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(requestReader);

    HAPError err = kHAPError_None;

    uint32_t prevTransitionCount = adaptiveLight->numTransitions;

    for (;;) {
        bool found;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLogError(&kHAPLog_Default, "%s: Parsing error.", __func__);
            return err;
        }
        if (!found) {
            break;
        }

        if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Fetch) {
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            HAPCharacteristicValue_TransitionControl_Fetch fetch;
            err = HAPTLVReaderDecode(&subReader, &kHAPCharacteristicTLVFormat_TransitionControl_Fetch, &fetch);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            uint64_t requestedFetchID = ConvertToUInt64(&(fetch.HAPInstanceID));
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.147 Characteristic Value Transition Control
            if (!IsCharacteristicSupportedByService(requestedFetchID, request->service)) {
                HAPLogError(
                        &kHAPLog_Default,
                        "%s this characteristic (iid = %llu) is not part of the requested service",
                        __func__,
                        (unsigned long long) requestedFetchID);
                return kHAPError_Unknown;
            }
            adaptiveLight->context.session = request->session;
            adaptiveLight->context.fetchID = requestedFetchID;
            adaptiveLight->context.pendingResponseType = kTransitionControlResponseType_Transition;
            HAPLogInfo(&kHAPLog_Default, "fetchID = %llu", (unsigned long long) adaptiveLight->context.fetchID);
        } else if (tlv.type == kHAPCharacteristicTLVType_TransitionControl_Start) {
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            AdaptiveLightTransitionStorage tempTransitionsStorage;
            HAPRawBufferZero(&tempTransitionsStorage, sizeof tempTransitionsStorage);
            tempTransitions = &tempTransitionsStorage;
            err = ParseTransitions(&subReader, request->service);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData || err == kHAPError_OutOfResources);
                return err;
            }
            adaptiveLight->context.session = request->session;
            adaptiveLight->context.pendingResponseType = kTransitionControlResponseType_TransitionState;
            err = UpdateTransitionTimer();
            if (err) {
                HAPLogError(&kHAPLog_Default, "%s Failed to update transition timer", __func__);
            }
            tempTransitions = NULL;
            HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        }
    }

    // Controller will be notified when the 'Characteristic Value Transition Count' changes.
    if (prevTransitionCount != adaptiveLight->numTransitions) {
        adaptiveLight->callbacks.handleTransitionExpiry();
    }
    return err;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleTransitionCountRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(adaptiveLight);

    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_TransitionCount));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_LightBulb));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);

    *value = (uint8_t) adaptiveLight->numTransitions;
    HAPLogInfo(&kHAPLog_Default, "%s: %u", __func__, *value);
    return kHAPError_None;
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif
