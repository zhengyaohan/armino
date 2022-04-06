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

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if (HAP_TESTING == 1)

#include "DebugCommandFileMonitor.h"

#include "DebugCommand.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static bool isMonitoring = false;

// End of file wait timeout in milliseconds.
// This timeout value determines maximum command latency.
#define kEOFTimeout 200

// File open retry parameters
#define kFileOpenRetriesMax     10
#define kFileOpenRetryDelayUsec 10000 // 10ms

#if __has_feature(nullability)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif
static pthread_t monitoringThread;
#if __has_feature(nullability)
#pragma clang diagnostic pop
#endif

static pthread_cond_t commandDoneCondition;
static pthread_mutex_t commandDoneMutex;

static void HandleCommandFileReady(void* _Nullable context HAP_UNUSED, size_t contextSize HAP_UNUSED) {
    ProcessCommandsFromFile();
    if (pthread_mutex_lock(&commandDoneMutex)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_lock failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
    if (pthread_cond_signal(&commandDoneCondition)) {
        HAPLogError(&kHAPLog_Default, "%s: poll failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
    if (pthread_mutex_unlock(&commandDoneMutex)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_unlock failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
}

static void* _Nullable MonitorThread(void* _Nullable arg HAP_UNUSED) {
    int openRetries = kFileOpenRetriesMax;
    uint32_t retryDelay = kFileOpenRetryDelayUsec;
    FILE* file = NULL;
    while (openRetries-- > 0) {
        char debugCommandFileName[PATH_MAX];
        GetCommandLineFilePath((char*) &debugCommandFileName, sizeof(debugCommandFileName));
        file = fopen(debugCommandFileName, "r");
        if (file) {
            HAPLogDebug(&kHAPLog_Default, "%s: opened file `%s`", __func__, debugCommandFileName);
            break;
        }
        if (openRetries == 0) {
            HAPLogError(&kHAPLog_Default, "%s: Unable to open command file `%s`", __func__, debugCommandFileName);
            HAPFatalError();
        } else {
            HAPLogDebug(
                    &kHAPLog_Default,
                    "%s: Unable to open command file `%s`, retrying...",
                    __func__,
                    debugCommandFileName);
            usleep(retryDelay);
            // wait progressively longer.
            retryDelay += kFileOpenRetryDelayUsec;
        }
    }
    if (pthread_cond_init(&commandDoneCondition, NULL)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_cond_init failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
    if (pthread_mutex_init(&commandDoneMutex, NULL)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_init failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }

    while (isMonitoring) {
        int ret = fflush(file);
        if (ret) {
            HAPLogError(&kHAPLog_Default, "%s: fflush failed: %s", __func__, strerror(errno));
            HAPFatalError();
        }
        ret = fseek(file, -1, SEEK_END);
        if (ret) {
            int error = errno;
            if (error == EINVAL) {
                // The command file is empty.
                struct timespec rqt;
                rqt.tv_sec = kEOFTimeout / 1000;
                rqt.tv_nsec = (kEOFTimeout % 1000) * 1000000;
                nanosleep(&rqt, NULL);
                continue;
            }
            HAPLogError(&kHAPLog_Default, "%s: fseek failed: %s", __func__, strerror(error));
            HAPFatalError();
        }
        long pos = ftell(file);
        if (pos > 0 && fgetc(file) == '\n') {
            if (pthread_mutex_lock(&commandDoneMutex)) {
                HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_lock failed: %s", __func__, strerror(errno));
                HAPFatalError();
            }
            HAPError err = HAPPlatformRunLoopScheduleCallback(HandleCommandFileReady, NULL, 0);
            HAPAssert(!err);
            if (pthread_cond_wait(&commandDoneCondition, &commandDoneMutex)) {
                HAPLogError(&kHAPLog_Default, "%s: pthread_cond_wait failed: %s", __func__, strerror(errno));
                HAPFatalError();
            }
            if (pthread_mutex_unlock(&commandDoneMutex)) {
                HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_unlock failed: %s", __func__, strerror(errno));
                HAPFatalError();
            }
        } else {
            // Command line is incomplete.
            struct timespec rqt;
            rqt.tv_sec = kEOFTimeout / 1000;
            rqt.tv_nsec = (kEOFTimeout % 1000) * 1000000;
            nanosleep(&rqt, NULL);
        }
    }

    if (pthread_mutex_destroy(&commandDoneMutex)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_mutex_destroy failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
    if (pthread_cond_destroy(&commandDoneCondition)) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_cond_destroy failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
    fclose(file);
    return NULL;
}

void DebugCommandFileMonitorStart(void) {
    isMonitoring = true;
    int ret = pthread_create(&monitoringThread, NULL, MonitorThread, NULL);
    if (ret) {
        HAPLogError(&kHAPLog_Default, "%s: pthread_create failed: %s", __func__, strerror(errno));
        HAPFatalError();
    }
}

void DebugCommandFileMonitorStop(void) {
    if (isMonitoring) {
        isMonitoring = false;
        int ret = pthread_join(monitoringThread, NULL);
        if (ret) {
            HAPLogError(&kHAPLog_Default, "%s: pthread_join failed: %s", __func__, strerror(errno));
            HAPFatalError();
        }
    }
}

#endif // (HAP_TESTING == 1)
