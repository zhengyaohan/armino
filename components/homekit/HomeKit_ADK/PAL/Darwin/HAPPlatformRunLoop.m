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

#import <Foundation/Foundation.h>

#include "HAPPlatform+Init.h"
#include "HAPPlatformFileHandle.h"
#include "HAPPlatformRunLoop+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "RunLoop" };

/**
 * Internal file handle type, representing the registration of a platform-specific file descriptor.
 */
typedef struct HAPPlatformFileHandle HAPPlatformFileHandle;

/**
 * Internal file handle representation.
 */
struct HAPPlatformFileHandle {
    /** File descriptor. */
    CFFileDescriptorRef fileDescriptor;

    /** Set of file handle events on which the callback shall be invoked. */
    HAPPlatformFileHandleEvent interests;

    /** Function to call when one or more events occur on the given file descriptor. */
    HAPPlatformFileHandleCallback callback;

    /** The context parameter given to the HAPPlatformFileHandleRegister function. */
    void* _Nullable context;
};

HAPPlatformFileHandle* registeredFileHandle = NULL;
CFRunLoopSourceRef runLoopSource;
CFRunLoopRef runLoop = NULL;

void HAPPlatformRunLoopCreate(const HAPPlatformRunLoopOptions* options) {
    HAPPrecondition(options);
    HAPPrecondition(options->keyValueStore);
}

void HAPPlatformRunLoopRelease(void) {
}

static void inputDetectedCallback(
        CFFileDescriptorRef fdref,
        CFOptionFlags callBackTypes HAP_UNUSED,
        void* info HAP_UNUSED) {
    registeredFileHandle->interests.isReadyForReading = true;
    registeredFileHandle->callback(
            (HAPPlatformFileHandleRef) registeredFileHandle,
            registeredFileHandle->interests,
            registeredFileHandle->context);
    registeredFileHandle->interests.isReadyForReading = false;
    CFFileDescriptorEnableCallBacks(fdref, kCFFileDescriptorReadCallBack);
}

void HAPPlatformRunLoopRun(void) {
    runLoop = CFRunLoopGetCurrent();
    CFRunLoopRun();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformRunLoopScheduleCallback(
        HAPPlatformRunLoopCallback callback,
        void* _Nullable context,
        size_t contextSize) {
    if (context) {
        void* msg = malloc(contextSize);
        HAPRawBufferCopyBytes(msg, context, contextSize);
        dispatch_async(dispatch_get_main_queue(), ^{
            callback(msg, contextSize);
            free(msg);
        });
    } else {
        HAPAssert(contextSize == 0);
        dispatch_async(dispatch_get_main_queue(), ^{
            callback(NULL, 0);
        });
    }
    return kHAPError_None;
}

void HAPPlatformRunLoopStop(void) {
    if (runLoop) {
        CFRunLoopStop(runLoop);
        runLoop = nil;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformFileHandleRegister(
        HAPPlatformFileHandleRef* fileHandle_,
        int fileDescriptor,
        HAPPlatformFileHandleEvent interests,
        HAPPlatformFileHandleCallback callback,
        void* _Nullable context) {
    HAPPrecondition(registeredFileHandle == NULL);

    HAPPlatformFileHandle* fileHandle = calloc(1, sizeof(HAPPlatformFileHandle));
    if (!fileHandle) {
        HAPLog(&logObject, "Cannot allocate file handle.");
        *fileHandle_ = 0;
        return kHAPError_OutOfResources;
    }

    fileHandle->fileDescriptor =
            CFFileDescriptorCreate(kCFAllocatorDefault, fileDescriptor, true, inputDetectedCallback, NULL);
    fileHandle->interests = interests;
    fileHandle->callback = callback;
    fileHandle->context = context;
    registeredFileHandle = fileHandle;
    *fileHandle_ = (HAPPlatformFileHandleRef) fileHandle;
    CFFileDescriptorEnableCallBacks(fileHandle->fileDescriptor, kCFFileDescriptorReadCallBack);
    runLoopSource = CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, registeredFileHandle->fileDescriptor, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopDefaultMode);
    return kHAPError_None;
}

void HAPPlatformFileHandleDeregister(HAPPlatformFileHandleRef fileHandle_ HAP_UNUSED) {
    HAPPrecondition(registeredFileHandle);

    CFFileDescriptorDisableCallBacks(registeredFileHandle->fileDescriptor, kCFFileDescriptorReadCallBack);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopDefaultMode);
    registeredFileHandle->interests.isReadyForReading = false;
    registeredFileHandle->interests.isReadyForWriting = false;
    registeredFileHandle->interests.hasErrorConditionPending = false;
    HAPPlatformFreeSafe(registeredFileHandle);
}
