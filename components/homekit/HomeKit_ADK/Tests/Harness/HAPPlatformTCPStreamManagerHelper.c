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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include <stdlib.h>

#include "HAPPlatformTCPStreamManager+Init.h"
#include "HAPPlatformTCPStreamManagerHelper.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "TCPStreamManager" };

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

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamManagerConnectToListener(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef* tcpStream) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(HAPPlatformTCPStreamManagerIsListenerOpen(tcpStreamManager));
    HAPPrecondition(tcpStream);

    for (size_t i = 0; i < tcpStreamManager->numTCPStreams; i++) {
        HAPPlatformTCPStream* stream = &tcpStreamManager->tcpStreams[i];
        if (stream->isActive) {
            continue;
        }

        // Open connection.
        HAPLogInfo(&logObject, "Opened connection: %p.", (const void*) stream);
        stream->isActive = true;
        stream->rx.maxBytes = tcpStreamManager->numBufferBytes;
        stream->rx.bytes = calloc(1, stream->rx.maxBytes);
        HAPAssert(stream->rx.bytes);
        stream->tx.maxBytes = tcpStreamManager->numBufferBytes;
        stream->tx.bytes = calloc(1, stream->tx.maxBytes);
        HAPAssert(stream->tx.bytes);
        *tcpStream = (HAPPlatformTCPStreamRef) stream;

        // Inform delegate.
        HAPPrecondition(tcpStreamManager->callback);
        tcpStreamManager->callback(tcpStreamManager, tcpStreamManager->context);

        return kHAPError_None;
    }

    HAPLogError(&logObject, "TCP stream manager cannot accept more connections.");
    return kHAPError_OutOfResources;
}

void HAPPlatformTCPStreamManagerClientClose(
        HAPPlatformTCPStreamManagerRef tcpStreamManager,
        HAPPlatformTCPStreamRef tcpStream_) {
    HAPPrecondition(tcpStreamManager);
    HAPPrecondition(tcpStream_);
    HAPPlatformTCPStream* tcpStream = (HAPPlatformTCPStream*) tcpStream_;
    HAPAssert(tcpStream->isActive);

    HAPAssert(!tcpStream->rx.isClientClosed);
    tcpStream->rx.isClientClosed = true;
    InvokeCallback(tcpStreamManager, tcpStream_);

    // Release resources when both sides closed.
    if (tcpStream->rx.isClosed && tcpStream->rx.isClientClosed) {
        HAPLogDebug(&logObject, "[%p] Closing (Client closed).", (const void*) tcpStream);
        HAPAssert(tcpStream->tx.isClosed);
        Invalidate(tcpStreamManager, tcpStream_);
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamClientRead(
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

    *numBytes = 0;
    size_t n;
    InvokeCallback(tcpStreamManager, tcpStream_);
    do {
        n = HAPMin(tcpStream->tx.numBytes, maxBytes - *numBytes);
        HAPRawBufferCopyBytes(&((uint8_t*) bytes)[*numBytes], HAPNonnullVoid(tcpStream->tx.bytes), n);
        HAPRawBufferCopyBytes(
                HAPNonnullVoid(tcpStream->tx.bytes), &((uint8_t*) tcpStream->tx.bytes)[n], tcpStream->tx.numBytes - n);
        tcpStream->tx.numBytes -= n;
        *numBytes += n;
        InvokeCallback(tcpStreamManager, tcpStream_);
    } while (*numBytes < maxBytes && n && !tcpStream->tx.isClosed);
    HAPAssert(*numBytes <= maxBytes);

    if (!*numBytes && !tcpStream->tx.isClosed) {
        return kHAPError_Busy;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTCPStreamClientWrite(
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

    *numBytes = 0;
    size_t n;
    InvokeCallback(tcpStreamManager, tcpStream_);
    do {
        n = HAPMin(tcpStream->rx.maxBytes - tcpStream->rx.numBytes, maxBytes - *numBytes);
        HAPRawBufferCopyBytes(
                &((uint8_t*) tcpStream->rx.bytes)[tcpStream->rx.numBytes], &((const uint8_t*) bytes)[*numBytes], n);
        tcpStream->rx.numBytes += n;
        *numBytes += n;
        InvokeCallback(tcpStreamManager, tcpStream_);
    } while (*numBytes < maxBytes && n && !tcpStream->rx.isClosed);
    HAPAssert(*numBytes <= maxBytes);

    if (!*numBytes && !tcpStream->rx.isClosed) {
        return kHAPError_Busy;
    }
    return kHAPError_None;
}
