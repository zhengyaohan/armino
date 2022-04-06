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

#if (HAP_TESTING == 1)

#include <dispatch/dispatch.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "HAP.h"

#include "AppUserInput.h"

/**
 * Application user input event callback function
 */
static HAPPlatformRunLoopCallback userInputEventCallback;

/**
 * Converts signal to user input identifier
 */
static AppUserInputIdentifier SignalToUserInputId(int signum) {
    switch (signum) {
        case SIGUSR1:
            return kAppUserInputIdentifier_1;
        case SIGUSR2:
            return kAppUserInputIdentifier_2;
        case SIGTERM:
            return kAppUserInputIdentifier_3;
        case SIGQUIT:
            return kAppUserInputIdentifier_4;
    }
    HAPLogInfo(&kHAPLog_Default, "Signal %d is ignored", signum);
    return kAppUserInputIdentifier_Invalid;
}

/**
 * Signal handler
 */
static void HandleSignal(int signum) {
    AppUserInputEvent userInputEvent = { .id = SignalToUserInputId(signum) };
    if (userInputEvent.id == kAppUserInputIdentifier_Invalid) {
        // Unsupported signal
        return;
    }

    HAPError err;
    err = HAPPlatformRunLoopScheduleCallback(userInputEventCallback, &userInputEvent, sizeof userInputEvent);
    if (err) {
        HAPLogError(&kHAPLog_Default, "User input event callback failed");
        HAPFatalError();
    }
}

/** default sigaction for segmentation fault */
static struct sigaction defaultSegvAction;

/** Segmentation fault handler */
static void HandleSegvSignal(int signum) {
    if (signum != SIGSEGV) {
        // This should not happen
        HAPFatalError();
    }

    // reinstall default handler in case the subsequent call causes SIGSEGV again.
    if (sigaction(SIGSEGV, &defaultSegvAction, NULL)) {
        goto error;
    }

    // HAP platform abort will attempt to print backtrace.
    // However, if heap is corrupt or memory protection has changed due to the fault,
    // it may cause SIGSEGV again.
    fprintf(stderr, "Segmentation Fault\n");
    HAPPlatformAbort();

error:
    fprintf(stderr, "Segmentation Fault\n");
    exit(1);
}

/**
 * Initialize Signal Handlers.
 */
static void InitializeSignalHandlers() {

    dispatch_source_t sigUsr1Source =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGUSR1, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigUsr1Source, ^{
        HandleSignal(SIGUSR1);
    });
    dispatch_resume(sigUsr1Source);

    dispatch_source_t sigUsr2Source =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGUSR2, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigUsr2Source, ^{
        HandleSignal(SIGUSR2);
    });
    dispatch_resume(sigUsr2Source);

    dispatch_source_t sigTermSource =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGTERM, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigTermSource, ^{
        HandleSignal(SIGTERM);
    });
    dispatch_resume(sigTermSource);

    dispatch_source_t sigQuitSource =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGQUIT, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigQuitSource, ^{
        HandleSignal(SIGQUIT);
    });
    dispatch_resume(sigQuitSource);

    dispatch_source_t sigSegvSource =
            dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGSEGV, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(sigSegvSource, ^{
        HandleSegvSignal(SIGSEGV);
    });
    dispatch_resume(sigSegvSource);

    // reset handlers for above signals with sigaction
    struct sigaction act;
    HAPRawBufferZero(&act, sizeof act);
    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGUSR1, &act, /* oact: */ NULL)) {
        HAPLogError(&kHAPLog_Default, "SignalHandler initialization failed");
        HAPFatalError();
    }
    if (sigaction(SIGUSR2, &act, /* oact: */ NULL)) {
        HAPLogError(&kHAPLog_Default, "SignalHandler initialization failed");
        HAPFatalError();
    }
    if (sigaction(SIGTERM, &act, /* oact: */ NULL)) {
        HAPLogError(&kHAPLog_Default, "SignalHandler initialization failed");
        HAPFatalError();
    }
    if (sigaction(SIGQUIT, &act, /* oact: */ NULL)) {
        HAPLogError(&kHAPLog_Default, "SignalHandler initialization failed");
        HAPFatalError();
    }

    if (sigaction(SIGSEGV, &act, /* oact: */ NULL)) {
        HAPLogError(&kHAPLog_Default, "SignalHandler initialization failed");
        HAPFatalError();
    }
}

void AppUserInputInitPlatform(HAPPlatformRunLoopCallback callback) {
    userInputEventCallback = callback;

    // Buttons are emulated with signals
    InitializeSignalHandlers();
}

#endif // HAP_TESTING
