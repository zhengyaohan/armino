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

#include "HAPCircularQueue.h"

void HAPCircularQueueCreate(HAPCircularQueue* queue, uint8_t* buffer, size_t bufferMaxSize) {
    HAPRawBufferZero(buffer, bufferMaxSize);
    queue->bytes = buffer;
    queue->totalSize = bufferMaxSize;
    queue->startIndex = 0;
    queue->endIndex = 0;
    queue->remainingSpace = bufferMaxSize;
    queue->usedSpace = 0;
    HAPPlatformMutexInit(&queue->mutex, NULL);
}

/**
 * Since this function is recursive and also inside a critical area, we use a sub-function to
 * execute the enqueue to prevent a deadlock if the data wraps around past the edge of the buffer.
 */
static HAPError executeEnqueue(HAPCircularQueue* queue, uint8_t* sourceBytes, size_t numBytes) {
    if (numBytes > queue->remainingSpace) {
        return kHAPError_OutOfResources;
    }
    size_t spaceBeforeWrap = queue->totalSize - queue->endIndex;
    if (numBytes > spaceBeforeWrap) {
        executeEnqueue(queue, sourceBytes, spaceBeforeWrap);
        executeEnqueue(queue, sourceBytes + spaceBeforeWrap, numBytes - spaceBeforeWrap);
    } else {
        HAPRawBufferCopyBytes(queue->bytes + queue->endIndex, sourceBytes, numBytes);
        queue->endIndex += numBytes;
        queue->usedSpace += numBytes;
        queue->endIndex = queue->endIndex % queue->totalSize;
        queue->remainingSpace = queue->totalSize - queue->usedSpace;
    }
    return kHAPError_None;
}

HAPError HAPCircularQueueEnqueue(HAPCircularQueue* queue, uint8_t* sourceBytes, size_t numBytes) {
    if (HAPPlatformMutexLock(&queue->mutex)) {
        HAPFatalError();
    }
    HAPError result = executeEnqueue(queue, sourceBytes, numBytes);
    if (HAPPlatformMutexUnlock(&queue->mutex)) {
        HAPFatalError();
    }
    return result;
}

/**
 * Since this function is recursive and also inside a critical area, we use a sub-function to
 * execute the enqueue to prevent a deadlock if the data wraps around past the edge of the buffer.
 * This also prevents code duplication between Dequeue and Peek functions
 */
static HAPError
        getFromQueue(HAPCircularQueue* queue, uint8_t* destinationBytes, size_t numBytes, bool peek, size_t offset) {
    if (numBytes > queue->totalSize) {
        return kHAPError_OutOfResources;
    }
    if (numBytes > queue->usedSpace) {
        return kHAPError_InvalidState;
    }

    size_t spaceBeforeWrap = queue->totalSize - queue->startIndex;
    if (numBytes > spaceBeforeWrap) {
        getFromQueue(queue, destinationBytes, spaceBeforeWrap, peek, 0);
        getFromQueue(queue, destinationBytes + spaceBeforeWrap, numBytes - spaceBeforeWrap, peek, spaceBeforeWrap);
    } else {
        size_t totalOffset = queue->startIndex;
        if (peek) {
            totalOffset += offset;
        }
        totalOffset = totalOffset % queue->totalSize;
        HAPRawBufferCopyBytes(destinationBytes, queue->bytes + totalOffset, numBytes);

        if (!peek) {
            queue->startIndex += numBytes;
            queue->startIndex = queue->startIndex % queue->totalSize;
            queue->usedSpace -= numBytes;
            queue->remainingSpace = queue->totalSize - queue->usedSpace;
        }
    }
    return kHAPError_None;
}

HAPError HAPCircularQueuePeek(HAPCircularQueue* queue, uint8_t* destinationBytes, size_t numBytes) {
    if (HAPPlatformMutexLock(&queue->mutex)) {
        HAPFatalError();
    }
    HAPError result = getFromQueue(queue, destinationBytes, numBytes, true, 0);
    if (HAPPlatformMutexUnlock(&queue->mutex)) {
        HAPFatalError();
    }
    return result;
}

HAPError HAPCircularQueueDequeue(HAPCircularQueue* queue, uint8_t* destinationBytes, size_t numBytes) {
    if (HAPPlatformMutexLock(&queue->mutex)) {
        HAPFatalError();
    }
    HAPError result = getFromQueue(queue, destinationBytes, numBytes, false, 0);
    if (HAPPlatformMutexUnlock(&queue->mutex)) {
        HAPFatalError();
    }
    return result;
}

size_t HAPCircularQueueGetRemainingSpace(HAPCircularQueue* queue) {
    return queue->remainingSpace;
}
