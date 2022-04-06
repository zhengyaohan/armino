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

#include <stdlib.h>

#include "HAPDataStream+TCP.h"
#include "HAPPlatformTCPStreamManager+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "TCPStreamManager(Mock)" };

static HAPNetworkPort _port = kHAPDataStream_TCPMinimumPort;

void HAPPlatformTCPStreamManagerCreate(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        const HAPPlatformTCPStreamManagerOptions* options) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(options);
    HAPPrecondition(options->tcpStreams);

    HAPRawBufferZero(options->tcpStreams, options->numTCPStreams * sizeof *options->tcpStreams);

    HAPRawBufferZero(tcpStreamManager, sizeof *tcpStreamManager);
    tcpStreamManager->tcpStreams = options->tcpStreams;
    tcpStreamManager->numTCPStreams = options->numTCPStreams;
    tcpStreamManager->numBufferBytes =
            options->numBufferBytes ? options->numBufferBytes : kHAPPlatformTCPStreamManager_NumBufferBytes;

    tcpStreamManager->port = _port++;

    for (size_t i = 0; i < tcpStreamManager->numTCPStreams; i++) {
        HAPPlatformTCPStream* tcpStream = &tcpStreamManager->tcpStreams[i];
        tcpStream->tcpStreamManager = tcpStreamManager;
    }
}
void HAPPlatformTCPStreamManagerSetTCPUserTimeout(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPTime tcpUserTimeout HAP_UNUSED) {
    HAPPrecondition(tcpStreamManager);
}

HAP_RESULT_USE_CHECK
HAPNetworkPort HAPPlatformTCPStreamManagerGetListenerPort(HAPPlatformTCPStreamManagerRef tcpStreamManager) {
    HAPPrecondition(tcpStreamManager);

    return tcpStreamManager->port;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformTCPStreamManagerIsListenerOpen(HAPPlatformTCPStreamManagerRef tcpStreamManager) {
    HAPPrecondition(tcpStreamManager);

    return tcpStreamManager->callback != NULL;
}

void HAPPlatformTCPStreamManagerOpenListener(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamListenerCallback callback,
        void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(callback);

    HAPLogInfo(&logObject, "%s(%u).", __func__, tcpStreamManager->port);
    HAPPrecondition(!tcpStreamManager->callback);
    HAPPrecondition(!tcpStreamManager->context);
    tcpStreamManager->callback = callback;
    tcpStreamManager->context = context;
}

void HAPPlatformTCPStreamManagerCloseListener(HAPPlatformTCPStreamManagerRef tcpStreamManager) {
    HAPPrecondition(tcpStreamManager);

    HAPLogInfo(&logObject, "%s(%u).", __func__, tcpStreamManager->port);
    tcpStreamManager->callback = NULL;
    tcpStreamManager->context = NULL;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamManagerAcceptTCPStream(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef* tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    for (size_t i = 0; i < tcpStreamManager->numTCPStreams; i++) {
        HAPPlatformTCPStream* stream = &tcpStreamManager->tcpStreams[i];
        if (!stream->isActive) {
            continue;
        }
        if (stream->isConnected) {
            continue;
        }

        // Open connection.
        HAPLogInfo(&logObject, "Accepted connection: %p.", (const void*) stream);
        stream->isConnected = true;
        *tcpStream = (HAPPlatformTCPStreamRef) stream;

        return kHAPError_None;
    }

    HAPLog(&logObject, "No acceptable connections found.");
    return kHAPError_Unknown;
}

static void Invalidate(HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPPlatformTCPStreamRef tcpStream_) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;

    if (tcpStream->invokeCallbackTimer) {
        HAPPlatformTimerDeregister(tcpStream->invokeCallbackTimer);
    }
    free(tcpStream->rx.bytes);
    free(tcpStream->tx.bytes);
    HAPRawBufferZero(tcpStream, sizeof *tcpStream);
}

void HAPPlatformTCPStreamCloseOutput(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream HAP_UNUSED) {
    HAPPrecondition(tcpStreamManager);

    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

void HAPPlatformTCPStreamClose(HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPPlatformTCPStreamRef tcpStream_) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPAssert(tcpStream->isActive);

    HAPAssert(!tcpStream->rx.isClosed);
    tcpStream->rx.isClosed = true;
    tcpStream->tx.isClosed = true;

    // Release resources when both sides closed.
    if (tcpStream->rx.isClosed && tcpStream->rx.isClientClosed) {
        HAPLogDebug(&logObject, "[%p] Closing.", (const void*) tcpStream);
        HAPAssert(tcpStream->tx.isClosed);
        Invalidate(tcpStreamManager, tcpStream_);
    }
}

static void InvokeCallback(HAPPlatformTCPStreamManagerRef tcpStreamManager, HAPPlatformTCPStreamRef tcpStream_) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPAssert(tcpStream->isActive);

    if (!tcpStream->callback) {
        return;
    }

    HAPPlatformTCPStreamEvent event = { .hasBytesAvailable = tcpStream->rx.numBytes || tcpStream->rx.isClientClosed,
                                        .hasSpaceAvailable = tcpStream->tx.numBytes < tcpStream->tx.maxBytes };

    if (tcpStream->rx.isClosed || !tcpStream->interests.hasBytesAvailable) {
        event.hasBytesAvailable = false;
    }
    if (tcpStream->tx.isClosed || !tcpStream->interests.hasSpaceAvailable) {
        event.hasSpaceAvailable = false;
    }
    if (event.hasBytesAvailable || event.hasSpaceAvailable) {
        tcpStream->callback(tcpStreamManager, tcpStream_, event, tcpStream->context);
    }
}

static void InvokeCallbackExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPPlatformTCPStream* tcpStream = context;
    HAPPlatformTCPStreamRef tcpStream_ = (HAPPlatformTCPStreamRef) tcpStream;
    HAPPrecondition(timer == tcpStream->invokeCallbackTimer);
    tcpStream->invokeCallbackTimer = 0;

    HAPPlatformTCPStreamManagerRef tcpStreamManager = tcpStream->tcpStreamManager;
    InvokeCallback(tcpStreamManager, tcpStream_);
}

void HAPPlatformTCPStreamUpdateInterests(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream_,
        HAPPlatformTCPStreamEvent interests,
        HAPPlatformTCPStreamEventCallback _Nullable callback,
        void* _Nullable context) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPPrecondition(tcpStream->isActive);
    HAPPrecondition(tcpStream->isConnected);
    HAPPrecondition(!(interests.hasBytesAvailable || interests.hasSpaceAvailable) || callback != NULL);

    HAPError err;

    tcpStream->interests = interests;
    tcpStream->callback = callback;
    tcpStream->context = context;

    if (!tcpStream->invokeCallbackTimer) {
        err = HAPPlatformTimerRegister(&tcpStream->invokeCallbackTimer, 0, InvokeCallbackExpired, tcpStream);
        HAPAssert(!err);
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamRead(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream_,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPAssert(tcpStream->isActive);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    if (tcpStream->rx.isClientClosed) {
        // Peer closed
        *numBytes = 0;
        return kHAPError_None;
    }

    *numBytes = HAPMin(tcpStream->rx.numBytes, maxBytes);
    HAPRawBufferCopyBytes(bytes, HAPNonnullVoid(tcpStream->rx.bytes), *numBytes);
    HAPRawBufferCopyBytes(
            HAPNonnullVoid(tcpStream->rx.bytes),
            &((uint8_t*) tcpStream->rx.bytes)[*numBytes],
            tcpStream->rx.numBytes - *numBytes);
    tcpStream->rx.numBytes -= *numBytes;

    if (!*numBytes && !tcpStream->rx.isClosed) {
        return kHAPError_Busy;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamWrite(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream_,
        const void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPAssert(tcpStream->isActive);
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    if (tcpStream->tx.isClosed) {
        return kHAPError_Unknown;
    }

    *numBytes = HAPMin(tcpStream->tx.maxBytes - tcpStream->tx.numBytes, maxBytes);
    HAPRawBufferCopyBytes(&((uint8_t*) tcpStream->tx.bytes)[tcpStream->tx.numBytes], bytes, *numBytes);
    tcpStream->tx.numBytes += *numBytes;

    if (!*numBytes) {
        return kHAPError_Busy;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
size_t HAPPlatformTCPStreamGetNumNonAcknowledgedBytes(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream);

    return 0;
}

void HAPPlatformTCPStreamManagerRelease(HAPPlatformTCPStreamManagerRef _Nonnull tcpStreamManager) {
    if (HAPPlatformTCPStreamManagerIsListenerOpen(tcpStreamManager)) {
        HAPPlatformTCPStreamManagerCloseListener(tcpStreamManager);
    }
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStreamManager->tcpStreams);
    tcpStreamManager->tcpStreams = NULL;
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
bool HAPPlatformTCPStreamManagerIsWiFiCurrentTransport(HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED) {
    return false;
}
#endif

HAP_RESULT_USE_CHECK
HAPPlatformWiFiCapability
        HAPPlatformTCPStreamManagerGetWiFiCapability(HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED) {
    return (HAPPlatformWiFiCapability) { .supports2_4GHz = true, .supports5GHz = false };
}

void HAPPlatformTCPStreamSetPriority(
        HAPPlatformTCPStreamManagerRef tcpStreamManager HAP_UNUSED,
        HAPPlatformTCPStreamRef tcpStream HAP_UNUSED,
        HAPStreamPriority priority HAP_UNUSED) {
}

#endif
