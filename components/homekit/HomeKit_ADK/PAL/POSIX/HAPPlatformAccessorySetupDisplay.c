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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "HAPPlatform.h"
#include "HAPPlatformAccessorySetupDisplay+Init.h"
#include "HAPPlatformSystemCommand.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "AccessorySetupDisplay" };

static void DisplayQRCode(HAPPlatformAccessorySetupDisplayRef setupDisplay) {
    HAPPrecondition(setupDisplay);
    HAPPrecondition(setupDisplay->setupPayloadIsSet);
    HAPPrecondition(setupDisplay->setupCodeIsSet);

    HAPError err;

    HAPLogInfo(
            &logObject,
            "%s: Launching 'qrencode' to display QR code with setup code: %s.",
            __func__,
            setupDisplay->setupCode.stringValue);

    HAPSetupPayload setupPayload;
    HAPRawBufferCopyBytes(&setupPayload, &setupDisplay->setupPayload, sizeof setupPayload);

    char* const cmd[] = { "/usr/bin/env", "qrencode", "-t", "ANSIUTF8", "-m", "2", setupPayload.stringValue, NULL };

    char bytes[6800];
    size_t numBytes;
    extern char** environ; // declared by unistd.h

    err = HAPPlatformSystemCommandRunWithEnvironment(cmd, environ, bytes, sizeof bytes - 1, &numBytes);
    bytes[numBytes] = '\0';

    if (err == kHAPError_OutOfResources) {
        HAPLogError(&logObject, "%s: Displaying QR code failed: Buffer too small.", __func__);
        return;
    } else if (err) {
        printf("%s\n", bytes);
        HAPLogError(&logObject, "%s: Displaying QR code failed: 'qrencode' not installed.", __func__);
        return;
    }
    printf("\n%s\n", bytes);
}

void HAPPlatformAccessorySetupDisplayCreate(
        HAPPlatformAccessorySetupDisplayRef setupDisplay,
        const HAPPlatformAccessorySetupDisplayOptions* options) {
    HAPPrecondition(setupDisplay);
    HAPPrecondition(options);
    HAPPrecondition(options->server);

    HAPRawBufferZero(setupDisplay, sizeof *setupDisplay);
    setupDisplay->server = options->server;
}

void HAPPlatformAccessorySetupDisplayUpdateSetupPayload(
        HAPPlatformAccessorySetupDisplayRef setupDisplay,
        const HAPSetupPayload* _Nullable setupPayload,
        const HAPSetupCode* _Nullable setupCode) {
    HAPPrecondition(setupDisplay);

    if (setupCode) {
        HAPLogInfo(&logObject, "##### Setup code for display: %s", setupCode->stringValue);
        HAPRawBufferCopyBytes(&setupDisplay->setupCode, HAPNonnull(setupCode), sizeof setupDisplay->setupCode);
        setupDisplay->setupCodeIsSet = true;
    } else {
        HAPLogInfo(&logObject, "##### Setup code for display invalidated.");
        HAPRawBufferZero(&setupDisplay->setupCode, sizeof setupDisplay->setupCode);
        setupDisplay->setupCodeIsSet = false;
    }
    if (setupPayload) {
        HAPLogInfo(&logObject, "##### Setup payload for QR code display: %s", setupPayload->stringValue);
        HAPRawBufferCopyBytes(&setupDisplay->setupPayload, HAPNonnull(setupPayload), sizeof setupDisplay->setupPayload);
        setupDisplay->setupPayloadIsSet = true;
    } else {
        HAPLogInfo(&logObject, "##### Setup payload for display invalidated.");
        HAPRawBufferZero(&setupDisplay->setupPayload, sizeof setupDisplay->setupPayload);
        setupDisplay->setupPayloadIsSet = false;
    }

    if (setupDisplay->setupPayloadIsSet) {
        DisplayQRCode(setupDisplay);
    }
}

void HAPPlatformAccessorySetupDisplayHandleStartPairing(HAPPlatformAccessorySetupDisplayRef setupDisplay) {
    HAPPrecondition(setupDisplay);
    HAPPrecondition(setupDisplay->setupCodeIsSet);

    HAPLogInfo(
            &logObject, "##### Pairing attempt has started with setup code: %s.", setupDisplay->setupCode.stringValue);

    if (setupDisplay->setupPayloadIsSet) {
        DisplayQRCode(setupDisplay);
    }
}

void HAPPlatformAccessorySetupDisplayHandleStopPairing(HAPPlatformAccessorySetupDisplayRef setupDisplay) {
    HAPPrecondition(setupDisplay);
    HAPPrecondition(setupDisplay->server);

    HAPLogInfo(&logObject, "##### Pairing attempt has completed or has been canceled.");
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif
