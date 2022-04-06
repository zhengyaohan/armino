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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#include "HAPPlatform+Init.h"

#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/TemplateDB.c"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define kIID_TestLightBulbBrightness ((uint64_t) 0x00A0)
#define kIID_TestLightBulbColorTemp  ((uint64_t) 0x00A1)

#define kMaxTestSamples ((size_t) 100)

static AdaptiveLightTransitionStorage NLUTAdaptiveLightStorage;

static int32_t actualBrightnessValue = 60;
static int32_t actualColorTempValue = 50;

static int32_t expectedBrightnessValue[kMaxTestSamples];
static int32_t expectedColorTempValue[kMaxTestSamples];
static HAPTime expectedEventTime[kMaxTestSamples];

/**
 * Update Interval for Transitions.
 */
#define kTest_TransitionUpdateInterval ((HAPTime) 1 * HAPSecond)

static void updateExpectedTimeToAbsolute() {
    HAPTime currentTime = HAPPlatformClockGetCurrent();
    for (size_t i = 0; i < kMaxTestSamples; i++) {
        if (i && expectedEventTime[i] == 0) {
            return;
        }
        expectedEventTime[i] += currentTime;
    }
}

static void checkBrightnessVal(size_t id) {
    if (id < kMaxTestSamples && expectedEventTime[id] == HAPPlatformClockGetCurrent()) {
        if (actualBrightnessValue != expectedBrightnessValue[id]) {
            HAPLogError(
                    &kHAPLog_Default,
                    "Actual vs Expected = %li vs %li",
                    (long) actualBrightnessValue,
                    (long) expectedBrightnessValue[id]);
            HAPAssert(false);
        }
    } else {
        HAPLogInfo(&kHAPLog_Default, "WARNING: Value not checked");
    }
}

static bool AddLinearTransitionPoint(
        AdaptiveLightTransition* t,
        int32_t targetVal,
        HAPTime startDelay,
        HAPTime completionDelay) {
    HAPPrecondition(t);
    HAPPrecondition(t->numPoints < kLightbulb_MaxSupportedTransitionPoints);
    int32_t prevEndVal = 0;
    HAPTime prevEndTime = 0;
    if (t->numPoints) {
        prevEndVal = t->points[t->numPoints - 1]->target;
        prevEndTime = t->points[t->numPoints - 1]->endTime;
    } else {
        prevEndVal = t->currentValue;
        prevEndTime = HAPPlatformClockGetCurrent();
    }
    AdaptiveLightTransitionPoint* pt = NULL;
    pt = AllocTransitionPoint();
    HAPAssert(pt);
    if (pt) {
        pt->target = targetVal;
        pt->startDelayDuration = startDelay;
        pt->completionDuration = completionDelay;
        pt->startTime = pt->startDelayDuration + prevEndTime;
        pt->endTime = pt->startTime + pt->completionDuration;
        if (pt->completionDuration) {
            pt->rate = (float) (pt->target - prevEndVal) / (float) pt->completionDuration;
        } else {
            pt->rate = 0.0f;
        }
        pt->isUsed = true;
    } else {
        return false;
    }
    // Add to transition points list
    t->points[t->numPoints++] = pt;
    return true;
}

static bool AddDerivedTransitionPoint(
        AdaptiveLightTransition* t,
        float scale,
        float offset,
        HAPTime startDelay,
        HAPTime completionDelay) {
    HAPPrecondition(t);
    HAPPrecondition(t->numPoints < kLightbulb_MaxSupportedTransitionPoints);
    HAPTime prevEndTime = 0;
    if (t->numPoints) {
        prevEndTime = t->points[t->numPoints - 1]->endTime;
    } else {
        prevEndTime = HAPPlatformClockGetCurrent();
    }
    AdaptiveLightTransitionPoint* pt = NULL;
    pt = AllocTransitionPoint();
    HAPAssert(pt);
    if (pt) {
        pt->scale = scale;
        pt->offset = offset;
        pt->startDelayDuration = startDelay;
        pt->completionDuration = completionDelay;
        pt->startTime = pt->startDelayDuration + prevEndTime;
        pt->endTime = pt->startTime + pt->completionDuration;
        pt->isUsed = true;
    } else {
        return false;
    }
    // Add to transition points list
    t->points[t->numPoints++] = pt;
    return true;
}

static void NLUTCharacteristicValueUpdate(uint64_t characteristicID, int32_t value, bool sendNotification) {
    if (characteristicID == kIID_TestLightBulbBrightness) {
        actualBrightnessValue = value;
        HAPLogInfo(&kHAPLog_Default, "Brightness changed to %d", (int) value);
    }
    if (characteristicID == kIID_TestLightBulbColorTemp) {
        actualColorTempValue = value;
        HAPLogInfo(&kHAPLog_Default, "Color Temp changed to %d", (int) value);
    }
}

static void NLUTCharacteristicValueRequest(uint64_t characteristicID, int32_t* value HAP_UNUSED) {
    if (characteristicID == kIID_TestLightBulbBrightness) {
    }
    if (characteristicID == kIID_TestLightBulbColorTemp) {
    }
}

static void NLUTTransitionExpiry() {
    HAPLogInfo(&kHAPLog_Default, "Number of active transitions changed.");
}

void resetState() {
    HAPRawBufferZero(&expectedEventTime, sizeof expectedEventTime);
    HAPRawBufferZero(&expectedBrightnessValue, sizeof expectedBrightnessValue);
    HAPRawBufferZero(&expectedColorTempValue, sizeof expectedColorTempValue);
    actualBrightnessValue = 60;
    actualColorTempValue = 50;
    HAPPlatformClockAdvance(100 * HAPSecond);
}

static void NLUTInitializeAdaptiveLight() {
    HAPError err;
    err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kAppKeyValueStoreDomain_AdaptiveLight);
    if (err) {
        HAPLogInfo(&kHAPLog_Default, "Unable to purge key value store.");
    }
    HAPRawBufferZero(&NLUTAdaptiveLightStorage, sizeof NLUTAdaptiveLightStorage);
    AdaptiveLightSupportedTransition supportedTransitions[] = {
        { kIID_TestLightBulbBrightness,
          kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_Linear },
        { kIID_TestLightBulbColorTemp,
          kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_LinearDerived }
    };
    AdaptiveLightCallbacks callbacks = {
        .handleCharacteristicValueUpdate = NLUTCharacteristicValueUpdate,
        .handleTransitionExpiry = NLUTTransitionExpiry,
        .handleCharacteristicValueRequest = NLUTCharacteristicValueRequest,
    };

    err = InitializeAdaptiveLightParameters(
            platform.keyValueStore,
            &NLUTAdaptiveLightStorage,
            supportedTransitions,
            HAPArrayCount(supportedTransitions),
            &callbacks);
    if (err) {
        HAPLogError(&kHAPLog_Default, "Failed to initialize Adaptive Light parameters");
    }
    resetState();
}

int Test1() {
    HAPLog(&kHAPLog_Default, "%s: Testing Transition Allocations", __func__);
    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t[kLightbulb_MaxSupportedTransitions];
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        t[i] = AllocTransition();
        HAPAssert(t[i]);
    }
    AdaptiveLightTransition* transition = AllocTransition();
    HAPAssert(!transition);

    // release transitions
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        ReleaseTransition(t[i]);
    }

    // try again
    transition = AllocTransition();
    HAPAssert(transition);

    resetState();
    return 0;
}

int Test2() {
    HAPLog(&kHAPLog_Default, "Testing Transition Point Allocations");
    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransitionPoint* pt[kLightbulb_MaxSupportedTransitionPoints];
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitionPoints; i++) {
        pt[i] = AllocTransitionPoint();
        HAPAssert(pt[i]);
    }
    AdaptiveLightTransitionPoint* point = AllocTransitionPoint();
    HAPAssert(!point);

    resetState();
    return 0;
}

int Test3() {
    HAPLog(&kHAPLog_Default, "%s: Testing Transition Allocations with points", __func__);
    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t[kLightbulb_MaxSupportedTransitions];
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        t[i] = AllocTransition();
        HAPAssert(t[i]);
        for (size_t j = 0; j < kLightbulb_MaxSupportedTransitionPoints / kLightbulb_MaxSupportedTransitions; j++) {
            t[i]->points[j] = AllocTransitionPoint();
            HAPAssert(t[i]->points[j]);
            t[i]->numPoints++;
        }
    }

    // release transitions
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        ReleaseTransition(t[i]);
    }

    // try allocting transition and points again
    for (size_t i = 0; i < kLightbulb_MaxSupportedTransitions; i++) {
        t[i] = AllocTransition();
        HAPAssert(t[i]);
        for (size_t j = 0; j < kLightbulb_MaxSupportedTransitionPoints / kLightbulb_MaxSupportedTransitions; j++) {
            t[i]->points[j] = AllocTransitionPoint();
            HAPAssert(t[i]->points[j]);
            t[i]->numPoints++;
        }
    }

    resetState();
    return 0;
}

int Test4() {
    HAPLog(&kHAPLog_Default, "%s: Testing Linear Transition with No Change as End Behavior", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 100, 0, 4000));
    }

    int32_t expectedValue[] = { 70, 80, 90, 100, 100, 100 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test5() {
    HAPLog(&kHAPLog_Default, "%s: Testing Linear Transition with Loop", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 100, 0, 4000));
    }

    int32_t expectedValue[] = { 70, 80, 90, 100, 70, 80, 90, 100, 70, 80 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test6() {
    HAPLog(&kHAPLog_Default, "%s: Linear Transition, Single Point, Start Delay, Loop", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 100, 2000, 0));
    }

    int32_t expectedValue[] = { 60, 100, 100 };
    HAPTime expectedTime[] = { 1000, 2000, 3000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test7() {
    HAPLog(&kHAPLog_Default, "%s: Linear Transition, Two Points, No Loop", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 50, 0, 1000));

        HAPAssert(AddLinearTransitionPoint(t, 90, 0, 4000));
    }

    int32_t expectedValue[] = { 50, 60, 70, 80, 90, 90, 90, 90, 90 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test8() {
    HAPLog(&kHAPLog_Default, "%s: Linear Transition, Two Points, Loop", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 50, 0, 1000));
        HAPAssert(AddLinearTransitionPoint(t, 90, 0, 4000));
    }

    int32_t expectedValue[] = { 50, 60, 70, 80, 90, 50, 60, 70, 80, 90, 50 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test9() {
    HAPLog(&kHAPLog_Default, "%s: Linear Transition, Two Points, Loop, Lower bound", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = 80;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 50, 0, 1000));
        HAPAssert(AddLinearTransitionPoint(t, 90, 0, 4000));
    }

    int32_t expectedValue[] = { 60, 60, 60, 60, 90, 50, 60, 70, 80, 90, 50 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test10() {
    HAPLog(&kHAPLog_Default, "%s: Linear Transition, Three Points, Loop, Upper bound", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = 40;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_Descends;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 50, 0, 1000));
        HAPAssert(AddLinearTransitionPoint(t, 10, 0, 4000));
        HAPAssert(AddLinearTransitionPoint(t, 90, 0, 8000));
    }
    int32_t expectedValue[] = { 60, 60, 30, 20, 10, 20, 30, 40, 50, 60, 70, 80, 90, 50,
                                40, 30, 20, 10, 20, 30, 40, 50, 60, 70, 80, 90, 50 };
    HAPTime expectedTime[] = { 1000,  2000,  3000,  4000,  5000,  6000,  7000,  8000,  9000,
                               10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000,
                               19000, 20000, 21000, 22000, 23000, 24000, 25000, 26000, 27000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test11() {
    HAPLog(&kHAPLog_Default, "%s: Linear Derived Transition, Two Points, Loop, Lower bound", __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t1;
    AdaptiveLightTransition* t2;
    // Create Linear Transition with a point
    {
        t1 = AllocTransition();
        HAPAssert(t1);

        t1->characteristicID = kIID_TestLightBulbBrightness;
        t1->transitionType = kAdaptiveLightTransitionType_Linear;
        t1->controllerContext.numBytes = 0;
        t1->threshold = 80;
        t1->currentValue = actualBrightnessValue;
        t1->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends;
        t1->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t1->requestTime = HAPPlatformClockGetCurrent();
        t1->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t1, 50, 0, 1000));
        HAPAssert(AddLinearTransitionPoint(t1, 90, 0, 4000));
    }

    // Create Linear Derived Transition
    {
        t2 = AllocTransition();
        HAPAssert(t2);

        t2->characteristicID = kIID_TestLightBulbColorTemp;
        t2->transitionType = kAdaptiveLightTransitionType_LinearDerived;
        t2->controllerContext.numBytes = 0;
        t2->currentValue = actualColorTempValue;
        t2->sourceCharacteristicID = kIID_TestLightBulbBrightness;
        t2->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop;
        t2->requestTime = HAPPlatformClockGetCurrent();
        t2->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddDerivedTransitionPoint(t2, 12.12, 2787.88, 0, 1000));
        HAPAssert(AddDerivedTransitionPoint(t2, 10.10, 2989.90, 0, 4000));
    }

    int32_t expectedBrightnessVal[] = { 60, 60, 60, 60, 90, 50, 60, 70, 80, 90, 50 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 11000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedBrightnessVal));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedBrightnessVal, sizeof(expectedBrightnessVal));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int Test12() {
    HAPLog(&kHAPLog_Default,
           "%s: Testing Linear Transition with completionDuration smaller than update interval",
           __func__);
    HAPError err = kHAPError_None;

    NLUTInitializeAdaptiveLight();

    AdaptiveLightTransition* t;
    // Create Linear Transition with a point
    {
        t = AllocTransition();
        HAPAssert(t);

        t->characteristicID = kIID_TestLightBulbBrightness;
        t->transitionType = kAdaptiveLightTransitionType_Linear;
        t->controllerContext.numBytes = 0;
        t->threshold = actualBrightnessValue;
        t->currentValue = actualBrightnessValue;
        t->startCondition = kHAPCharacteristicValue_TransitionControl_StartCondition_None;
        t->endBehavior = kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange;
        t->requestTime = HAPPlatformClockGetCurrent();
        t->updateInterval = kTest_TransitionUpdateInterval;

        HAPAssert(AddLinearTransitionPoint(t, 100, 0, 500));
    }

    int32_t expectedValue[] = { 100, 100, 100, 100 };
    HAPTime expectedTime[] = { 1000, 2000, 3000, 4000 };
    HAPAssert(HAPArrayCount(expectedTime) == HAPArrayCount(expectedValue));
    HAPRawBufferCopyBytes(expectedBrightnessValue, expectedValue, sizeof(expectedValue));
    HAPRawBufferCopyBytes(expectedEventTime, expectedTime, sizeof(expectedTime));
    updateExpectedTimeToAbsolute();

    err = UpdateTransitionTimer();
    HAPAssert(!err);
    for (size_t tm = 0; tm < HAPArrayCount(expectedTime); tm++) {
        HAPPlatformClockAdvance(1 * HAPSecond);
        checkBrightnessVal(tm);
    }

    resetState();
    return 0;
}

int main() {
    HAPPlatformCreate();
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();
    Test10();
    Test11();
    Test12();
    return 0;
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif
