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

#include "app_timer.h"

#include "HAPPlatform.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Clock" };

APP_TIMER_DEF(periodicTimer);
APP_TIMER_DEF(periodicTimer2);

static void HandlePeriodicTimerExpired(void* context HAP_UNUSED) {
    // We have to update the internal state at least once per RTC1 counter rollover period
    // to keeping track of time accurately. Fetching the current time frequently enough ensures this.
    HAPTime time = HAPPlatformClockGetCurrent();
    (void) time;
}

APP_TIMER_DEF(delayTimer);

static void HandleDelayTimerExpired(void* context HAP_UNUSED) {
    uint32_t e = app_timer_start(periodicTimer2, APP_TIMER_TICKS(/* MS: */ 5 * 60 * 1000), /* p_context: */ NULL);
    if (e) {
        HAPLogError(&logObject, "app_timer_start failed: %lu.", (unsigned long) e);
        HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPTime HAPPlatformClockGetCurrent(void) {
    static uint32_t ticks;
    static bool initialized;

    // Initialize.
    if (!initialized) {
        if (!APP_TIMER_KEEPS_RTC_ACTIVE) {
            HAPLogError(&logObject, "APP_TIMER_KEEPS_RTC_ACTIVE must be set to 1 in sdk_config.h.");
            HAPFatalError();
        }

        // app_timer resets the RTC1 counter when no timers are scheduled even when APP_TIMER_KEEPS_RTC_ACTIVE is set.
        // To work around this, we start a periodic timer that remains active at all time.
        // Because this does not fully resolve the issue, two periodic timers are started to make sure that
        // one of them is always running.
        uint32_t e = app_timer_create(&periodicTimer, APP_TIMER_MODE_REPEATED, HandlePeriodicTimerExpired);
        if (e) {
            HAPLogError(&logObject, "app_timer_create failed: %lu.", (unsigned long) e);
            HAPFatalError();
        }
        e = app_timer_start(periodicTimer, APP_TIMER_TICKS(/* MS: */ 300000), /* p_context: */ NULL);
        if (e) {
            HAPLogError(&logObject, "app_timer_start failed: %lu.", (unsigned long) e);
            HAPFatalError();
        }

        // Because the workaround above does not fully resolve the issue, a second periodic timer is started
        // after some time to make sure that one of them is always running.
        e = app_timer_create(&periodicTimer2, APP_TIMER_MODE_REPEATED, HandlePeriodicTimerExpired);
        if (e) {
            HAPLogError(&logObject, "app_timer_create failed: %lu.", (unsigned long) e);
            HAPFatalError();
        }
        e = app_timer_create(&delayTimer, APP_TIMER_MODE_SINGLE_SHOT, HandleDelayTimerExpired);
        if (e) {
            HAPLogError(&logObject, "app_timer_create failed: %lu.", (unsigned long) e);
            HAPFatalError();
        }
        e = app_timer_start(delayTimer, APP_TIMER_TICKS(/* MS: */ 150000), /* p_context: */ NULL);
        if (e) {
            HAPLogError(&logObject, "app_timer_start failed: %lu.", (unsigned long) e);
            HAPFatalError();
        }

        ticks = app_timer_cnt_get();
        initialized = true;
    }

    // Compute elapsed ticks.
    uint32_t actualTicks = app_timer_cnt_get();
    uint32_t elapsedTicks = app_timer_cnt_diff_compute(actualTicks, ticks);
    ticks = actualTicks;

    // Update time.
    static HAPTime now;
    now += ROUNDED_DIV((uint64_t) elapsedTicks * 1000 * (APP_TIMER_CONFIG_RTC_FREQUENCY + 1), APP_TIMER_CLOCK_FREQ);

    // Check for overflow.
    if (now & (1ULL << 63)) {
        HAPLog(&logObject, "Time overflowed (capped at 2^63 - 1).");
        HAPFatalError();
    }
    return now;
}
