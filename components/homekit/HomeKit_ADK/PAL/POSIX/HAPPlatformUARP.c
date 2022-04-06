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

#include "HAP+API.h"
#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)

#include <stdarg.h>

#include "HAPBase.h"
#include "HAPLog.h"
#include "HAPPlatformUARP.h"

// All UARP allocations should use static buffers, the fallback dynamic memory allocation
// should never be used. Calls to these methods likely indicate a memory leak in UARP
/*
 ================================================================================
 ================================================================================
 */

void* uarpZalloc(size_t length HAP_UNUSED) {
    HAPFatalError();
    return NULL;
}

/*
 ================================================================================
 ================================================================================
 */

void uarpFree(void* pBuffer HAP_UNUSED) {
    HAPFatalError();
}

/*
 ================================================================================
 ================================================================================
 */

uint32_t uarpHtonl(uint32_t val32) {
    return htonl(val32);
}

/*
 ================================================================================
 ================================================================================
 */

uint32_t uarpNtohl(uint32_t val32) {
    return ntohl(val32);
}

/*
  ================================================================================
  ================================================================================
  */

uint16_t uarpHtons(uint16_t val16) {
    return htons(val16);
}

/*
 ================================================================================
 ================================================================================
 */

uint16_t uarpNtohs(uint16_t val16) {
    return ntohs(val16);
}

/* MARK: Logging */

#if !(UARP_DISABLE_LOGS) || !(UARP_DISABLE_REQUIRE_LOGS)
#define kUarpEmbeddedLoggingSubsystem  "com.apple.uarp.embedded"
#define kUarpLoggingCategoryAccessory  "protocolaccessory"
#define kUarpLoggingCategoryController "protocolcontroller"
#define kUarpLoggingCategoryPlatform   "platform"
#define kUarpLoggingCategoryProduct    "product"
#define kUarpLoggingCategoryMemory     "memory"
#define kUarpLoggingCategoryAssert     "assert"
#define kUarpLoggingCategoryUnknown    "unknown"

static const HAPLogObject accessoryObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                              .category = kUarpLoggingCategoryAccessory };
static const HAPLogObject controllerObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                               .category = kUarpLoggingCategoryController };
static const HAPLogObject platformObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                             .category = kUarpLoggingCategoryPlatform };
static const HAPLogObject productObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                            .category = kUarpLoggingCategoryProduct };
static const HAPLogObject memoryObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                           .category = kUarpLoggingCategoryMemory };
static const HAPLogObject assertObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                           .category = kUarpLoggingCategoryAssert };
static const HAPLogObject unknownObject = { .subsystem = kUarpEmbeddedLoggingSubsystem,
                                            .category = kUarpLoggingCategoryUnknown };

#define kUarpMaxLogBuffer 512

static char gUarpLogBuffer[kUarpMaxLogBuffer];

const HAPLogObject* getLogObjectByCategory(UARPLoggingCategory category) {
    // Using accessory category as default for logging unknown categories
    const HAPLogObject* logObject = &unknownObject;

    switch (category) {
        case kUARPLoggingCategoryAccessory:
            logObject = &accessoryObject;
            break;
        case kUARPLoggingCategoryController:
            logObject = &controllerObject;
            break;
        case kUARPLoggingCategoryPlatform:
            logObject = &platformObject;
            break;
        case kUARPLoggingCategoryProduct:
            logObject = &productObject;
            break;
        case kUARPLoggingCategoryMemory:
            logObject = &memoryObject;
            break;
        case kUARPLoggingCategoryAssert:
            logObject = &assertObject;
            break;
        case kUARPLoggingCategoryMax:
            break;
    }

    return logObject;
}
#endif // !(UARP_DISABLE_LOGS) || !(UARP_DISABLE_REQUIRE_LOGS)

/*
 ================================================================================
 ================================================================================
 */

#if !(UARP_DISABLE_REQUIRE_LOGS)
void uarpLogFault(UARPLoggingCategory category, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    HAPError err = HAPStringWithFormatAndArguments(gUarpLogBuffer, kUarpMaxLogBuffer, msg, args);
    HAPAssert(!err);
    va_end(args);

    // UARP require macros do not guarantee a fault condition.
    HAPLog(getLogObjectByCategory(category), "%s", gUarpLogBuffer);
}
#endif // !(UARP_DISABLE_REQUIRE_LOGS)

/*
 ================================================================================
 ================================================================================
 */

#if !(UARP_DISABLE_LOGS)
void uarpLogError(UARPLoggingCategory category, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    HAPError err = HAPStringWithFormatAndArguments(gUarpLogBuffer, kUarpMaxLogBuffer, msg, args);
    HAPAssert(!err);
    va_end(args);

    HAPLogError(getLogObjectByCategory(category), "%s", gUarpLogBuffer);
}

/*
 ================================================================================
 ================================================================================
 */

void uarpLogDebug(UARPLoggingCategory category, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    HAPError err = HAPStringWithFormatAndArguments(gUarpLogBuffer, kUarpMaxLogBuffer, msg, args);
    HAPAssert(!err);
    va_end(args);

    HAPLogDebug(getLogObjectByCategory(category), "%s", gUarpLogBuffer);
}

/*
 ================================================================================
 ================================================================================
 */

void uarpLogInfo(UARPLoggingCategory category, const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    HAPError err = HAPStringWithFormatAndArguments(gUarpLogBuffer, kUarpMaxLogBuffer, msg, args);
    HAPAssert(!err);
    va_end(args);

    HAPLogInfo(getLogObjectByCategory(category), "%s", gUarpLogBuffer);
}
#endif // !(UARP_DISABLE_LOGS)

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
