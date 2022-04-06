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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "HAPPlatformSystemCommand.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SystemCommand(Mock)" };

HAP_RESULT_USE_CHECK
HAPError HAPPlatformSystemCommandRun(
        char* _Nullable const command[_Nonnull],
        char* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(command);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    return HAPPlatformSystemCommandRunWithEnvironment(command, /* environment: */ NULL, bytes, maxBytes, numBytes);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformSystemCommandRunWithEnvironment(
        char* _Nullable const command[_Nonnull],
        char* _Nullable const environment[_Nullable],
        char* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(command);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    int e;
    int pipeFDs[2];

    e = pipe(pipeFDs);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&logObject, "%s: pipe failed: %d.", __func__, _errno);
        HAPFatalError();
    }

    pid_t commandPID = fork();
    if (commandPID < 0) {
        int _errno = errno;
        HAPAssert(commandPID == -1);
        HAPLogError(&logObject, "%s: fork failed: %d.", __func__, _errno);
        HAPFatalError();
    }

    if (commandPID == 0) {
        // Forked process.

        // Remove signal handlers as they are inherited when we fork.
        struct sigaction act;
        HAPRawBufferZero(&act, sizeof act);
        act.sa_handler = SIG_DFL;
        sigemptyset(&act.sa_mask);
        if (sigaction(SIGUSR1, &act, /* oact: */ NULL)) {
            int _errno = errno;
            HAPLogError(&logObject, "%s: sigaction USR1 failed: %d.", __func__, _errno);
            HAPFatalError();
        }
        if (sigaction(SIGUSR2, &act, /* oact: */ NULL)) {
            int _errno = errno;
            HAPLogError(&logObject, "%s: sigaction USR2 failed: %d.", __func__, _errno);
            HAPFatalError();
        }
        if (sigaction(SIGTERM, &act, /* oact: */ NULL)) {
            int _errno = errno;
            HAPLogError(&logObject, "%s: sigaction TERM failed: %d.", __func__, _errno);
            HAPFatalError();
        }
        if (sigaction(SIGQUIT, &act, /* oact: */ NULL)) {
            int _errno = errno;
            HAPLogError(&logObject, "%s: sigaction QUIT failed: %d.", __func__, _errno);
            HAPFatalError();
        }

        (void) close(pipeFDs[0]);

        do {
            e = dup2(pipeFDs[1], STDOUT_FILENO);
        } while (e == -1 && errno == EINTR);
        if (e == -1) {
            int _errno = errno;
            (void) close(pipeFDs[1]);
            HAPLogError(&logObject, "%s: dup2 STDOUT failed: %d.", __func__, _errno);
            HAPFatalError();
        }

        (void) close(pipeFDs[1]);

        e = execve(command[0], command, environment);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            HAPLogError(&logObject, "%s: execve failed: %d.", __func__, _errno);
            _exit(1);
        }
        // Unreachable
        HAPFatalError();
    }

    // Main process.
    (void) close(pipeFDs[1]);

    bool bufferTooSmall = false;
    bool successfulRead = true;

    size_t o = 0;
    while (o < maxBytes) {
        size_t c = maxBytes - o;
        ssize_t n;
        do {
            n = read(pipeFDs[0], &((uint8_t*) bytes)[o], c);
        } while (n == -1 && errno == EINTR);
        if (n <= -1) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "%s: read failed: %d.", __func__, _errno);
            successfulRead = false;
            break;
        }

        HAPAssert((size_t) n <= c);
        o += (size_t) n;

        if (o == maxBytes) {
            // Try to read one additional byte to check if there is more data.
            char tempBuffer;
            do {
                n = read(pipeFDs[0], &tempBuffer, 1);
            } while (n == -1 && errno == EINTR);
            if (n <= -1) {
                int _errno = errno;
                HAPAssert(n == -1);
                HAPLogError(&logObject, "%s: read failed: %d.", __func__, _errno);
                successfulRead = false;
                break;
            }
            if (n == 1) {
                bufferTooSmall = true;
                break;
            }
            HAPAssert(n == 0);
        }

        if (n == 0) {
            break;
        }
    }
    *numBytes = o;

    (void) close(pipeFDs[0]);

    int status;
    pid_t pid;
    do {
        pid = waitpid(commandPID, &status, 0);
    } while (pid == -1 && errno == EINTR);
    int _errno = errno;

    if (pid < 0 && _errno != ECHILD) {
        HAPLogError(&logObject, "%s: waitpid failed: %d.", __func__, _errno);
        HAPFatalError();
    }

    // Check if read has been successful.
    if (!successfulRead) {
        return kHAPError_Unknown;
    }

    // Check exit status.
    HAPAssert(pid == commandPID || (pid == -1 && _errno == ECHILD));
    if (!WIFEXITED(status)) {
        HAPLogError(&logObject, "%s: Process did not exit: Status %d.", __func__, WIFEXITED(status));
        return kHAPError_Unknown;
    }

    int exitStatus = WEXITSTATUS(status);
    if (exitStatus != 0) {
        HAPLogInfo(&logObject, "%s: process exited with status %d.", __func__, exitStatus);
        return kHAPError_Unknown;
    }

    // Check buffer flag.
    if (bufferTooSmall) {
        HAPLogInfo(&logObject, "%s: buffer too small to store result.", __func__);
        return kHAPError_OutOfResources;
    }

    return kHAPError_None;
}
