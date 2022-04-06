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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.

#include "HAPPlatformBLE+BlueZ.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_PLATFORM_BLE)

#include "HAPLogSubsystem.h"
#include "HAPPlatform.h"
#include "HAPPlatformSystemCommand.h"
#include "bluez_version.h"

#include <string.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "BLE" };

#define BLUETOOTH_DAEMON_PATH "/usr/local/libexec/bluetooth/bluetoothd"

static char versionString[32];

// Check whether patched bluetooth daemon is running
static HAPError CheckDaemonStatus(void) {
    char* const statusCmds[] = {
        "/bin/systemctl",
        "status",
        "bluetooth",
        NULL,
    };
    char buffer[2048];
    size_t numStatusBytes;
    HAPError err = HAPPlatformSystemCommandRun(statusCmds, buffer, sizeof buffer, &numStatusBytes);
    if (err || numStatusBytes >= sizeof buffer) {
        HAPLogError(&logObject, "%s: could not retrieve bluetooth daemon status", __func__);
        return kHAPError_Unknown;
    }
    if (!strstr(buffer, BLUETOOTH_DAEMON_PATH)) {
        HAPLogError(
                &logObject, "%s: patched bluetooth daemon is not found. Please re-install bluetooth daemon.", __func__);
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

const char* _Nullable HAPPlatformBLEGetBlueZVersionString(void) {
    if (versionString[0]) {
        return versionString;
    }

    // Check that the patched version of BlueZ is running
    HAPError err = CheckDaemonStatus();
    if (err) {
        return NULL;
    }

    // Read the version string
    size_t numVersionBytes;
    char* const cmds[] = {
        BLUETOOTH_DAEMON_PATH,
        "--version",
        NULL,
    };
    err = HAPPlatformSystemCommandRun(cmds, versionString, sizeof versionString, &numVersionBytes);
    if (err || numVersionBytes >= sizeof versionString) {
        HAPLogError(&logObject, "%s: could not read BlueZ version.", __func__);
        return NULL;
    } else {
        if (numVersionBytes > 0 && versionString[numVersionBytes - 1] == '\n') {
            versionString[--numVersionBytes] = 0;
        } else {
            versionString[numVersionBytes] = 0;
        }
    }
    return versionString;
}

void HAPPlatformBLECheckBlueZVersion(void) {
    const char* version = HAPPlatformBLEGetBlueZVersionString();
    if (!version || !HAPStringAreEqual(version, BLUEZ_VERSION_STRING)) {
        HAPLogError(
                &logObject,
                "%s: version \"%s\" mismatches expected version \"%s\"",
                __func__,
                version,
                BLUEZ_VERSION_STRING);
        HAPFatalError();
    } else {
        HAPLogInfo(&logObject, "%s: %s", __func__, version);
    }
}

#endif
