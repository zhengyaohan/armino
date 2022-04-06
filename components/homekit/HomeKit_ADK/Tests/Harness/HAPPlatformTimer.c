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

#include "HAPPlatform.h"
#include "HAPPlatformClockHelper.h"
#include "HAPPlatformTimerHelper.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Timer" };

#define kTimerStorage_MaxTimers ((size_t) 40)

typedef struct {
    /**
     * ID. 0 if timer has never been used.
     */
    HAPPlatformTimerRef id;

    /**
     * Deadline after which the timer expires.
     */
    HAPTime deadline;

    /**
     * Callback. NULL if timer inactive.
     */
    HAPPlatformTimerCallback _Nullable callback;

    /**
     * The context parameter given to the HAPPlatformTimerRegister function.
     */
    void* _Nullable context;
} HAPPlatformTimer;

static HAPPlatformTimer timers[kTimerStorage_MaxTimers];
static size_t numActiveTimers;
static size_t numExpiredTimers;

/** queued timer events to emulate race condition */
static HAPPlatformTimer queuedTimerEvents[kTimerStorage_MaxTimers];
static size_t numQueuedTimerEvents = 0;

static bool shouldQueueTimerEvents = false;
static bool isDequeuingTimerEvents = false;

void HAPPlatformTimerProcessExpiredTimers(void) {
    // Reentrancy note - Callbacks may lead to reentrant add / remove timer invocations.
    // Do not call any functions that may lead to reentrancy!
    //
    // The idea is that timers 0 ..< numExpiredTimers are managed here.
    // add / remove must only move timers numExpiredTimers ..< numActiveTimers.
    // Timers added through reentrancy are allocated after the expired timers.
    // Timers removed through reentrancy have their callback set to NULL.

    // Get current time, and, by checking, make sure that it is updated.
    HAPTime now = HAPPlatformClockGetCurrent();

    // Find number of expired timers.
    for (numExpiredTimers = 0; numExpiredTimers < numActiveTimers; numExpiredTimers++) {
        if (timers[numExpiredTimers].deadline > now) {
            break;
        }
    }

    // Invoke callbacks.
    for (size_t i = 0; i < numExpiredTimers; i++) {
        if (timers[i].callback) {
            if (shouldQueueTimerEvents) {
                // shouldQueueTimerEvents must not be set in the middle of dequeuing timer events.
                // That is, a timer callback function must not call HAPPlatformTimerEmulateQueuedTimerEvents().
                // If the following assertion fails, check if any timer event handler is callling
                // HAPPlatformTimerEmulateQueuedTimerEvents().
                HAPAssert(!isDequeuingTimerEvents);

                HAPLogDebug(&logObject, "Timer event queued for expired timer: %lu", (unsigned long) timers[i].id);
                HAPAssert(numQueuedTimerEvents < HAPArrayCount(queuedTimerEvents));
                queuedTimerEvents[numQueuedTimerEvents++] = timers[i];
                timers[i].callback = NULL;
            } else {
                HAPLogDebug(&logObject, "Expired timer: %lu", (unsigned long) timers[i].id);
                timers[i].callback(timers[i].id, timers[i].context);
                timers[i].callback = NULL;
            }
        }
    }

    // Free memory.
    HAPAssert(numExpiredTimers <= numActiveTimers);
    while (numExpiredTimers) {
        HAPPlatformTimerRef id = timers[0].id;
        HAPRawBufferCopyBytes(&timers[0], &timers[1], (numActiveTimers - 1) * sizeof timers[0]);
        numActiveTimers--;
        numExpiredTimers--;
        timers[numActiveTimers].id = id;
    }
    HAPAssert(!numExpiredTimers);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTimerRegister(
        HAPPlatformTimerRef* timer,
        HAPTime deadline,
        HAPPlatformTimerCallback callback,
        void* _Nullable context) {
    HAPPrecondition(timer);
    HAPPrecondition(callback);

    // Do not call any functions that may lead to reentrancy!

    if (numActiveTimers == sizeof timers / sizeof timers[0]) {
        HAPLog(&logObject, "Cannot allocate more timers.");
        return kHAPError_OutOfResources;
    }

    // Find timer slot.
    size_t i;
    for (i = numExpiredTimers; i < numActiveTimers; i++) {
        if (timers[i].deadline > deadline) {
            // Search condition must be '>' and not '>=' to ensure that timers fire in ascending order of their
            // deadlines and that timers registered with the same deadline fire in order of registration.
            break;
        }
    }

    // Move timers.
    HAPPlatformTimerRef id = timers[numActiveTimers].id;
    HAPRawBufferCopyBytes(&timers[i + 1], &timers[i], (numActiveTimers - i) * sizeof timers[0]);
    timers[i].id = id;
    numActiveTimers++;

    // Prepare timer.
    static HAPPlatformTimerRef peakNumTimers;
    if (!timers[i].id) {
        timers[i].id = ++peakNumTimers;
        HAPAssert(timers[i].id <= sizeof timers / sizeof timers[0]);
        HAPLogInfo(
                &logObject,
                "New maximum of concurrent timers: %u (%u%%).",
                (unsigned int) peakNumTimers,
                (unsigned int) (100 * peakNumTimers / (sizeof timers / sizeof timers[0])));
    }

    // Store client data.
    timers[i].deadline = deadline;
    timers[i].callback = callback;
    timers[i].context = context;

    // Store timer ID.
    *timer = timers[i].id;

    HAPLogDebug(
            &logObject,
            "Added timer: %lu (deadline %8llu.%03llu).",
            (unsigned long) timers[i].id,
            (unsigned long long) (timers[i].deadline / HAPSecond),
            (unsigned long long) (timers[i].deadline % HAPSecond));
    return kHAPError_None;
}

void HAPPlatformTimerDeregister(HAPPlatformTimerRef timer) {
    HAPPrecondition(timer);

    // Do not call any functions that may lead to reentrancy!

    HAPLogDebug(&logObject, "Removed timer: %lu", (unsigned long) timer);

    // Find timer.
    for (size_t i = 0; i < numActiveTimers; i++) {
        if (timers[i].id == timer) {
            HAPAssert(timers[i].callback);
            timers[i].callback = NULL;

            if (i >= numExpiredTimers) {
                // Move remaining timers.
                HAPRawBufferCopyBytes(&timers[i], &timers[i + 1], (numActiveTimers - i - 1) * sizeof timers[i]);
                numActiveTimers--;
                timers[numActiveTimers].id = timer;
            }

            return;
        }
    }

    // Find timer from queued expired timer
    for (size_t i = 0; i < numQueuedTimerEvents; i++) {
        if (queuedTimerEvents[i].id == timer) {
            // Timer found.
            // Note that the purpose of the queuedTimerEvents is to emulate that already queued timer cannot be aborted.
            // Hence, no action is required.
            return;
        }
    }

    // Timer not found.
    HAPLogError(&logObject, "Timer not found: %lu.", (unsigned long) timer);
    HAPFatalError();
}

void HAPPlatformTimerDeregisterAll(void) {
    HAPLogDebug(&logObject, "%s", __func__);

    numActiveTimers = numExpiredTimers;
}

void HAPPlatformTimerEmulateClockAdvances(HAPTime advance) {
    // Note that clock advance emulation doesn't work when called from within a timer callback
    // because HAPPlatformClockAdvance() itself doesn't work in such a case.
    // If the following assertion fails, the test must be rewritten not to call this function from within a timer
    // callback.
    HAPAssert(!numExpiredTimers);

    HAPTime now = HAPPlatformClockGetCurrent();
    HAPTime advancedDeadline = now + advance;
    for (;;) {
        HAPTime increment = 0;
        bool timerShouldExpire = false;
        for (size_t i = 0; i < numActiveTimers; i++) {
            if (timers[i].deadline <= advancedDeadline) {
                if (timers[i].deadline > now) {
                    increment = timers[i].deadline - now;
                }
                timerShouldExpire = true;
                break;
            }
        }
        if (timerShouldExpire) {
            // Clock should advance only up till next timer expires.
            // The following call will expire the next timer(s).
            HAPPlatformClockAdvance(increment);
            HAPTime newNow = HAPPlatformClockGetCurrent();
            // We are assuming that clock advance would advance ticks at least by increment and no less.
            HAPAssert(newNow >= now + increment);
            now = newNow;
        } else {
            // No timers pending to expire between now and the advanced time.
            // Just adjust clock tick.
            HAPPlatformClockAdvance(advancedDeadline - now);
            break;
        }
    }
}

void HAPPlatformTimerEmulateQueuedTimerEvents(bool shouldQueue) {
    if (!shouldQueue && shouldQueueTimerEvents) {
        // Dequeue queued timer events if any
        isDequeuingTimerEvents = true;
        shouldQueueTimerEvents = false;
        for (size_t i = 0; i < numQueuedTimerEvents; i++) {
            HAPAssert(queuedTimerEvents[i].callback);
            HAPLogDebug(&logObject, "Executed queued timer: %lu", (unsigned long) queuedTimerEvents[i].id);
            queuedTimerEvents[i].callback(queuedTimerEvents[i].id, queuedTimerEvents[i].context);
        }
        numQueuedTimerEvents = 0;
        isDequeuingTimerEvents = false;
    }
    shouldQueueTimerEvents = shouldQueue;
}
