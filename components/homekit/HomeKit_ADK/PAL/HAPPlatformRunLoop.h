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

#ifndef HAP_PLATFORM_RUN_LOOP_H
#define HAP_PLATFORM_RUN_LOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * The ADK runs in one loop, here called the run loop, and is responsible for managing and dispatching I/O events from
 * all attached event sources, and it also drives the scheduler of timer events. In addition, `HAPPlatformRunLoop`
 * provides a way for scheduling callbacks that will be called from the run loop. It must be safe to call this function
 * from execution contexts other than the main loop, e.g., from another thread, from a signal handler, or an interrupt
 * handler. It is the only synchronization mechanism of the ADK. The remaining ADK functionality, with the exception of
 * the camera protocol stack *HAP-RTP*, executes in a single-threaded way from the run loop.
 *
 * Callbacks are blocking, meaning it is the accessory logic’s responsibility to immediately process them without
 * waiting. This is possible because HAP is essentially a protocol for the remote reading and writing of variables
 * (characteristics), which should not involve lengthy processing on the accessory. I/O in the PAL is generally
 * non-blocking.
 *
 * **Expected behavior:**
 *
 * Function `HAPPlatformRunLoopScheduleCallback` must be called by the accessory logic in order to schedule a callback
 * on the run loop execution context from any other execution context. To minimize resource consumption, it should not
 * be called from code that already executes on the run loop execution context.
 *
 * A HomeKit controller may send a write request for multiple characteristics, e.g., requesting to turn on a light bulb,
 * at *50%* intensity, with blue color. When such a request has been received, all handlers for these characteristics
 * must be called in an uninterrupted sequence, i.e., if there are pending scheduled callbacks or timer callbacks they
 * must be delayed accordingly.
 */

/**
 * Runs the loop which processes data from all attached input sources.
 *
 * - This function may only be called while the run loop is stopped.
 */
void HAPPlatformRunLoopRun(void);

/**
 * Schedules a request to exit the run loop.
 *
 * - This function must be called from the same execution context (e.g., thread) as the run loop,
 *   i.e., it should be scheduled on the run loop.
 *
 * - When this function returns, the run loop is not yet fully stopped.
 *   Wait for HAPPlatformRunLoopRun to return.
 *
 * - The execution of pending scheduled events may be delayed until the run loop is started again.
 */
void HAPPlatformRunLoopStop(void);

/**
 * Callback that is invoked from the run loop.
 *
 * @param      context              Client context.
 * @param      contextSize          Size of the context.
 */
typedef void (*HAPPlatformRunLoopCallback)(void* _Nullable context, size_t contextSize);

/**
 * Schedule a callback that will be called from the run loop.
 *
 * - It is safe to call this function from execution contexts (e.g., threads) other than the run loop,
 *   e.g., from another thread or from a signal handler.
 *
 * @param      callback             Function to call on the run loop.
 * @param      context              Address of context memory block which is copied and then passed to the callback.
 *                                  It is OK to deallocate the memory block after this function call because the memory
 *                                  block is copied.
 * @param      contextSize          Size of context memory block that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If there are not enough resources to schedule the callback.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformRunLoopScheduleCallback(
        HAPPlatformRunLoopCallback callback,
        void* _Nullable context,
        size_t contextSize);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
