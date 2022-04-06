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

#include "HAP.h"

#include "ApplicationFeatures.h"

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1)
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "HAPPlatformDiagnostics.h"

#define PRINT_LOG(fmt, ...) \
    do { \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        HAPPlatformDiagnosticsWriteToCrashLog(fmt, ##__VA_ARGS__); \
    } while (0)
#define CLEAR_LOG() HAPPlatformDiagnosticsClearCrashLog()
#else
#define CLEAR_LOG()
#define PRINT_LOG(fmt, ...) \
    do { \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
    } while (0)
#endif // HAVE_DIAGNOSTICS_SERVICE == 1

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

    // The following will attempt to print backtrace.
    // However, if heap is corrupt or memory protection has changed due to the fault,
    // it may cause SIGSEGV again.
    pid_t pid = getpid();
    char appPath[256];
    char commandString[sizeof(appPath) + 64];
    int ret = readlink("/proc/self/exe", appPath, sizeof appPath - 1);
    if (ret == -1) {
        // Couldn't read. No backtrace.
        goto error;
    }
    CLEAR_LOG();
    struct timeval now;
    ret = gettimeofday(&now, NULL);
    if (!ret) {
        struct tm g;
        struct tm* gmt = localtime_r(&now.tv_sec, &g);
        int microsecondTail = now.tv_usec % 1000;
        int microsecondHead = now.tv_usec / 1000;

        if (gmt) {
            PRINT_LOG(
                    "%04d-%02d-%02d'T'%02d:%02d:%02d.%03d%03d Segmentation fault\n",
                    1900 + gmt->tm_year,
                    1 + gmt->tm_mon,
                    gmt->tm_mday,
                    gmt->tm_hour,
                    gmt->tm_min,
                    gmt->tm_sec,
                    microsecondHead,
                    microsecondTail);
        }
    }
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    HAPError err = HAPStringWithFormat(
            commandString,
            sizeof commandString,
            "gdb --nh --batch --pid=%d --symbols=\"%s\" -ex \"thread apply all bt full\" "
            "| tee -a " kHAPPlatformDiagnosticsCrashLogFile,
            (int) pid,
            appPath);
#else  // HAVE_DIAGNOSTICS_SERVICE
    HAPError err = HAPStringWithFormat(
            commandString,
            sizeof commandString,
            "gdb --nh --batch --pid=%d --symbols=\"%s\" -ex \"thread apply all bt full\"",
            (int) pid,
            appPath);
#endif // HAVE_DIAGNOSTICS_SERVICE == 1
    if (err) {
        // Couldn't build command. No backtrace.
        goto error;
    }
    ret = system(commandString);
    if (ret) {
        // Couldn't run gdb. No backtrace.
        goto error;
    }
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    // Ensure logs are flushed to disk
    HAPPlatformDiagnosticsFlushLog();
#endif
    exit(1);
error:
    fprintf(stderr, "Segmentation Fault\n");
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
    // Ensure logs are flushed to disk
    HAPPlatformDiagnosticsFlushLog();
#endif
    exit(1);
}

/**
 * Initialize Signal Handlers.
 */
static void InitializeSignalHandlers() {
    struct sigaction act;
    HAPRawBufferZero(&act, sizeof act);
    act.sa_handler = HandleSignal;
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
    // Overwrite segmentation fault handler to print backtrace
    act.sa_handler = HandleSegvSignal;
    if (sigaction(SIGSEGV, &act, &defaultSegvAction)) {
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
HAP_DIAGNOSTIC_POP
