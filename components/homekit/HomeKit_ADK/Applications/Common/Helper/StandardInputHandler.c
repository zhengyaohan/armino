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

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "HAP+API.h"

#include "StandardInputHandler.h"

#if (HAP_TESTING == 1)

/**
 * User Interface State.
 */
/**@{*/
typedef struct {
    StandardInputHandleCallback inputCallback;
    HAPPlatformFileHandleRef stdinFileHandle;
    int stdinFileDescriptor;
} UserInputState;

static UserInputState userInputState;

typedef struct {
    struct termios values;
    bool isDefined;
} TerminalAttributes;
static TerminalAttributes terminalAttributes;

static void InputHandleCallback(
        HAPPlatformFileHandleRef fileHandle,
        HAPPlatformFileHandleEvent fileHandleEvents,
        void* _Nullable context) {
    HAPAssert(fileHandle);
    HAPAssert(fileHandleEvents.isReadyForReading);
    HAPAssert(context);

    int* fileDescriptor = (int*) context;

    for (;;) {
        char singleChar;
        ssize_t n;
        do {
            n = read(*fileDescriptor, &singleChar, sizeof singleChar);
        } while (n == -1 && errno == EINTR);
        if (n == 0) {
            return;
        }
        if (n == -1 && errno == EAGAIN) {
            return;
        }
        if (singleChar == '\n') {
            return;
        }

        // Call Application Stdin Handler Callback
        if (userInputState.inputCallback != NULL) {
            userInputState.inputCallback(singleChar);
        }
    }
}

void StdinPipeCreate(StandardInputHandleCallback inputHandleCallback) {
    HAPError err;

    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

#if !defined(DARWIN)
    int e;

    // Setup terminal
    struct termios t;
    do {
        e = tcgetattr(STDIN_FILENO, &t);
        terminalAttributes.values = t;
        terminalAttributes.isDefined = true;
    } while (e == -1 && errno == EINTR);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&kHAPLog_Default, "%s: tcgetattr failed: %d.", __func__, _errno);
        terminalAttributes.isDefined = false;
    }
    e = cfsetispeed(&t, B115200);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&kHAPLog_Default, "%s: cfsetispeed failed: %d.", __func__, _errno);
    }
    e = cfsetospeed(&t, B115200);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&kHAPLog_Default, "%s: cfsetospeed failed: %d.", __func__, _errno);
    }

    // Make the terminal raw.
    t.c_iflag &= (tcflag_t) ~(BRKINT | INPCK | ISTRIP);
    t.c_lflag &= (tcflag_t) ~(ECHO | ICANON);
    t.c_cflag |= (CS8);

    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;

    struct termios t_expected = t;
    do {
        e = tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    } while (e == -1 && errno == EINTR);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&kHAPLog_Default, "%s: tcsetattr failed: %d.", __func__, _errno);
    }

    do {
        e = tcgetattr(STDIN_FILENO, &t);
    } while (e == -1 && errno == EINTR);
    if (e) {
        int _errno = errno;
        HAPAssert(e == -1);
        HAPLogError(&kHAPLog_Default, "%s: tcgetattr failed: %d.", __func__, _errno);
    }

    if (!HAPRawBufferAreEqual(&t, &t_expected, sizeof(struct termios))) {
        HAPLogError(&kHAPLog_Default, "%s: tcsetattr did not apply all requested changes.", __func__);
    }

    e = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    if (e == -1) {
        HAPLogError(
                &kHAPLog_Default,
                "%s: System call 'fcntl' to set STDIN file descriptor 0 flags to 'non-blocking' failed.",
                __func__);
        HAPFatalError();
    }
#endif

    userInputState.stdinFileDescriptor = STDIN_FILENO;
    userInputState.inputCallback = inputHandleCallback;

    err = HAPPlatformFileHandleRegister(
            &userInputState.stdinFileHandle,
            userInputState.stdinFileDescriptor,
            (HAPPlatformFileHandleEvent) {
                    .isReadyForReading = true, .isReadyForWriting = false, .hasErrorConditionPending = false },
            InputHandleCallback,
            &userInputState.stdinFileDescriptor);
    if (err) {
        HAPLogError(&kHAPLog_Default, "%s: Unable to register Filehandle.", __func__);
        HAPFatalError();
    }
}

void StdinPipeRelease(void) {
    HAPLogDebug(&kHAPLog_Default, "%s", __func__);

    if (terminalAttributes.isDefined) {
        int e;
        do {
            e = tcsetattr(STDIN_FILENO, TCSANOW, &terminalAttributes.values);
        } while (e == -1 && errno == EINTR);
        if (e) {
            int _errno = errno;
            HAPAssert(e == -1);
            HAPLogError(&kHAPLog_Default, "%s: tcsetattr failed: %d.", __func__, _errno);
        }
    }
    HAPPlatformFileHandleDeregister(userInputState.stdinFileHandle);
}

#endif // HAP_TESTING
