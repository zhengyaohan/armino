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

#include "GatewayPing.h"

// VENDOR-TODO: Implement the functions in platform specific way.

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "AppLogSubsystem.h"
#include "HAPPlatformSystemCommand.h"

static const HAPLogObject logObject = { .subsystem = kApp_LogSubsystem, .category = "GatewayPing" };

/** ping period in seconds */
#define kPingPeriod (20)

/** other stats period in multiples of @ref kPingPeriod */
#define kStatsPeriod (15)

/** maximum number of bytes for ping destination string */
#define kMaxPingDestinationBytes 128

/** interface over which to send ping */
#ifdef __MACH__
#define kPingInterface "en0"
#else
#define kPingInterface "wlan0"
#endif

/** alternative interface over which to send ping */
#ifndef __MACH__
#define kAltPingInterface "eth0"
#endif

/** Invalid gateway address */
#define kNullAddress "0.0.0.0"

/** ping destination address buffer */
static char pingAddressBuffer[kMaxPingDestinationBytes];

/** consecutive ping failure count */
static int consecutivePingFailureCount;

/** number of ping failures to reset gateway address */
#define kNumPingFailuresToResetGatewayAddress (3)

/** ping command path */
#ifdef __MACH__
#define kPingCommandPath "/sbin/ping"
#else
#define kPingCommandPath "/bin/ping"
#endif

/** ping command interface option */
#ifdef __MACH__
#define kPingInterfaceOption "-b"
#else
#define kPingInterfaceOption "-I"
#endif

/** netstat command path */
#ifdef __MACH__
#define kNetstatCommandPath "/usr/sbin/netstat"
#else
#define kNetstatCommandPath "/bin/netstat"
#endif

/** maximum number of netstat response bytes */
#ifdef __MACH__
#define kMaxNumNetstatResponseBytes 8192
#else
#define kMaxNumNetstatResponseBytes 1024
#endif

/** ping command */
static char* _Nullable pingCommand[] = {
    kPingCommandPath, "-c", "1", kPingInterfaceOption, NULL, pingAddressBuffer, NULL,
};

/** index to the interface string in the pingCommand array */
#define kPingCommandInterfaceIndex 4

/** ifconfig command path */
#define kIfConfigCommandPath "/sbin/ifconfig"

/** ifconfig command */
static char* _Nullable ifconfigCommand[] = {
    kIfConfigCommandPath,
    kPingInterface,
    NULL,
};

/** WiFi stats command */
#ifdef __MACH__
#define WIFI_STATS_COMMAND_IS_AVAILABLE 0
#else // __MACH__
static char* _Nullable wifiStatsCommand[] = {
    "/sbin/iwconfig",
    NULL,
};
#define WIFI_STATS_COMMAND_IS_AVAILABLE 1
#endif // __MACH__

#define kSecondInUSecs 1000000

#ifdef __MACH__
static pthread_t _Nullable pingThread;
#else
static pthread_t pingThread;
#endif

static bool pingThreadActive;
static bool suppressPing;

static void PingGateway(void) {
    HAPPrecondition(pingAddressBuffer[0]);
    char buffer[1024];
    size_t numBytes;
    HAPError err = HAPPlatformSystemCommandRun(pingCommand, buffer, sizeof buffer - 1, &numBytes);
    if (err) {
        HAPLogError(
                &logObject,
                "%s: ping failed (%s %s).",
                __func__,
                pingAddressBuffer,
                pingCommand[kPingCommandInterfaceIndex]);
        if (++consecutivePingFailureCount >= kNumPingFailuresToResetGatewayAddress) {
            pingAddressBuffer[0] = 0;
        }
    } else {
        HAPPrecondition(numBytes < sizeof buffer);
        buffer[numBytes] = 0;
        if (buffer[numBytes - 1] == '\n') {
            buffer[numBytes - 1] = 0;
        }
        HAPLogInfo(&logObject, "%s", buffer);
        consecutivePingFailureCount = 0;
    }
}

static void LogWiFiStats(void) {
    char buffer[2048];
    size_t numBytes;
    HAPError err = HAPPlatformSystemCommandRun(ifconfigCommand, buffer, sizeof buffer - 1, &numBytes);
    if (err) {
        HAPLogError(&logObject, "%s: ifconfig failed.", __func__);
    } else {
        HAPPrecondition(numBytes < sizeof buffer);
        buffer[numBytes] = 0;
        if (buffer[numBytes - 1] == '\n') {
            buffer[numBytes - 1] = 0;
        }
        HAPLogInfo(&logObject, "ifconfig:\n%s", buffer);
    }

#if (WIFI_STATS_COMMAND_IS_AVAILABLE == 1)
    err = HAPPlatformSystemCommandRun(wifiStatsCommand, buffer, sizeof buffer - 1, &numBytes);
    if (err) {
        HAPLogError(&logObject, "%s: WiFi stats failed.", __func__);
    } else {
        HAPPrecondition(numBytes < sizeof buffer);
        buffer[numBytes] = 0;
        if (buffer[numBytes - 1] == '\n') {
            buffer[numBytes - 1] = 0;
        }
        HAPLogInfo(&logObject, "wifi:\n%s", buffer);
    }
#endif // (WIFI_STATS_COMMAND_IS_AVAILABLE == 1)
}

static void InitializePingDestination(void) {
    pingAddressBuffer[0] = 0;
    char buffer[kMaxNumNetstatResponseBytes];
    size_t numBytes;
    static char* netstatCommand[] = {
        kNetstatCommandPath, "-nr", "-f", "inet", NULL,
    };
    HAPError err = HAPPlatformSystemCommandRun(netstatCommand, buffer, sizeof buffer - 1, &numBytes);
    if (err) {
        HAPLogError(&logObject, "%s: Failed to run netstat command: %d", __func__, (int) err);
        return;
    }
    HAPPrecondition(numBytes < sizeof buffer);
    buffer[numBytes] = 0;
    char* lineStart = buffer;
#ifdef kAltPingInterface
    pingCommand[kPingCommandInterfaceIndex] = kAltPingInterface;
#else
    pingCommand[kPingCommandInterfaceIndex] = kPingInterface;
#endif
    for (;;) {
        char* eol = strchr(lineStart, '\n');
        if (eol == NULL) {
            break;
        }
        *eol = 0;
        char* eolSpace = eol - 1;
        while (eolSpace != lineStart && *eolSpace == ' ') {
            *eolSpace = 0;
            eolSpace--;
        }
        bool isPrimaryInterface = false;
        if (HAPStringAreEqual(kPingInterface, eolSpace + 2 - sizeof(kPingInterface))) {
            isPrimaryInterface = true;
#ifdef kAltPingInterface
        } else if (!HAPStringAreEqual(kAltPingInterface, eolSpace + 2 - sizeof(kAltPingInterface))) {
#else
        } else {
#endif
            lineStart = eol + 1;
            continue;
        }
        char* space = strchr(lineStart, ' ');
        if (space == NULL) {
            lineStart = eol + 1;
            continue;
        }
        while (*space == ' ' && *space != 0) {
            space++;
        }
        if (*space == 0) {
            lineStart = eol + 1;
            continue;
        }
        char* address = space;
        space = strchr(address, ' ');
        if (space == NULL) {
            lineStart = eol + 1;
            continue;
        }
        *space = 0;
        if (HAPStringAreEqual(address, kNullAddress)) {
            lineStart = eol + 1;
            continue;
        }
        size_t numAddressBytes = HAPStringGetNumBytes(address) + 1;
        if (numAddressBytes > sizeof pingAddressBuffer) {
            HAPLogError(&logObject, "%s: Gateway address too long: %s", __func__, address);
            pingAddressBuffer[0] = 0;
            return;
        }
        HAPRawBufferCopyBytes(pingAddressBuffer, address, numAddressBytes);
        if (isPrimaryInterface) {
            pingCommand[kPingCommandInterfaceIndex] = kPingInterface;
            break;
        }
        lineStart = eol + 1;
    }
    if (pingAddressBuffer[0]) {
        consecutivePingFailureCount = 0;
        HAPLogDebug(
                &logObject,
                "%s: Ping destination is set to \"%s\" (%s).",
                __func__,
                pingAddressBuffer,
                pingCommand[kPingCommandInterfaceIndex]);
        PingGateway();
    } else {
        HAPLogDebug(&logObject, "%s: No ping destination found", __func__);
    }
}

static void* _Nullable PingThread(void* _Nullable context HAP_UNUSED) {
    int statsCountUp = 0;
    for (;;) {
        if (suppressPing) {
            usleep(kSecondInUSecs);
            pingAddressBuffer[0] = 0;
        } else {
            if (pingAddressBuffer[0] == 0) {
                InitializePingDestination();
            } else {
                PingGateway();
            }
            usleep(kPingPeriod * kSecondInUSecs);
        }
        if (statsCountUp == 0) {
            LogWiFiStats();
        }
        statsCountUp = (statsCountUp + 1) % kStatsPeriod;
    }
    return NULL;
}

void GatewayPingStart(void) {
    HAPLogInfo(&logObject, "%s", __func__);
    suppressPing = false;
    if (!pingThreadActive) {
        pingThreadActive = true;
        int ret = pthread_create(&pingThread, NULL, PingThread, NULL);
        if (ret != 0) {
            HAPLogError(&logObject, "%s: failed to launch ping thread: %s", __func__, strerror(ret));
        }
    }
}

void GatewayPingStop(void) {
    HAPLogInfo(&logObject, "%s", __func__);
    suppressPing = true;
}
