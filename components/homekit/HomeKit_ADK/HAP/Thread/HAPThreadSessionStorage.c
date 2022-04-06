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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include "HAPAccessoryServer+Info.h"
#include "HAPAccessoryServer+Internal.h"
#include "HAPBitSet.h"
#include "HAPLogSubsystem.h"
#include "HAPSession.h"
#include "HAPThreadAccessoryServer.h"
#include "HAPThreadSessionStorage.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "SessionStorage" };

HAP_RESULT_USE_CHECK
static bool HAPThreadSessionStorage_NotificationBitSet_IsValid(HAPThreadSessionStorage_NotificationBitSet value) {
    switch (value) {
        case kHAPSessionStorage_NotificationBitSet_Status:
        case kHAPSessionStorage_NotificationBitSet_Pending: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPThreadSessionStorage_DataBuffer_IsValid(HAPThreadSessionStorage_DataBuffer value) {
    switch (value) {
        case kHAPSessionStorage_DataBuffer_TimedWrite:
        case kHAPSessionStorage_DataBuffer_Notification: {
            return true;
        }
        default:
            return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPThreadSessionStorage* GetSessionStorage(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    return &server->sessionStorage;
}

HAP_RESULT_USE_CHECK
static HAPThreadSessionStorageState* GetSessionStorageState(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    return &session->storageState;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPError SetNumBytesWithOptionalErase(HAPAccessoryServer* server, size_t numBytes, bool erase) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);

    // While it is tempting to use dynamic memory allocation at this point it would bring a number of disadvantages:
    // - It is no longer possible to easily audit how much memory is being used.
    // - The system might fail at unexpected times due to other unrelated components needing more memory.
    // - The minimum requirements for HomeKit (e.g., supporting X concurrent sessions) cannot be guaranteed.
    // - Especially on embedded systems with low memory capacity, the heap is prone to fragment over time.
    // - Some platforms might not come with a heap or have a complex heap configuration (e.g., block configuration).

    // Check hard requirement.
    if (sessionStorage->maxBytes < numBytes) {
        HAPLogError(
                &logObject,
                "Session storage buffer is not large enough (%zu / %zu bytes).",
                sessionStorage->maxBytes,
                numBytes);
        return kHAPError_OutOfResources;
    }
    if (numBytes > sessionStorage->peakNumBytes) {
        HAPLogInfo(
                &logObject,
                "New peak session storage buffer size: %zu / %zu bytes (%zu%%).",
                numBytes,
                sessionStorage->maxBytes,
                (size_t)(100U * numBytes / sessionStorage->maxBytes));
        sessionStorage->peakNumBytes = numBytes;
    }

    // Check soft requirement.
    // Potential buffer size assumes that all sessions could show the behaviour of the session
    // which consumes most resources, at the same time. If this buffer size is supported,
    // the accessory server is guaranteed to never reject any request (barring changes to the request behaviour).
    size_t numSessions = HAPAccessoryServerGetNumSessions(server);
    size_t potentialNumBytes = sessionStorage->dataBuffer.startPosition;
    for (size_t i = 0; i < HAPArrayCount(sessionStorage->dataBuffer.peakNumBytes); i++) {
        potentialNumBytes += numSessions * sessionStorage->dataBuffer.peakNumBytes[i];
    }
    if (sessionStorage->maxBytes < potentialNumBytes) {
        HAPLogInfo(
                &logObject,
                "Session storage buffer is large enough for now, "
                "but concurrent requests might be rejected (%zu / %zu bytes).",
                sessionStorage->maxBytes,
                potentialNumBytes);
    }
    if (potentialNumBytes > sessionStorage->peakPotentialNumBytes) {
        HAPLogInfo(
                &logObject,
                "New peak potential session storage buffer size: %zu / %zu bytes (%zu%%).",
                potentialNumBytes,
                sessionStorage->maxBytes,
                (size_t)(100U * potentialNumBytes / sessionStorage->maxBytes));
        sessionStorage->peakPotentialNumBytes = potentialNumBytes;
    }

    // Update memory for new length.
    uint8_t* bytes = sessionStorage->bytes;
    if (erase) {
        if (numBytes > sessionStorage->numBytes) {
            HAPRawBufferZero(&bytes[sessionStorage->numBytes], numBytes - sessionStorage->numBytes);
        } else if (numBytes < sessionStorage->numBytes) {
            HAPRawBufferZero(&bytes[numBytes], sessionStorage->numBytes - numBytes);
        } else {
            // Do nothing.
        }
    }
    sessionStorage->numBytes = numBytes;

    return kHAPError_None;
}

static HAPError SetNumBytes(HAPAccessoryServer* server, size_t numBytes) {
    return SetNumBytesWithOptionalErase(server, numBytes, true);
}

HAP_RESULT_USE_CHECK
static bool IsInBuffer(HAPAccessoryServer* server, const void* bytes_) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    uint8_t* bufferBytes = sessionStorage->bytes;
    HAPPrecondition(bytes_);
    const uint8_t* bytes = bytes_;

    return bytes >= &bufferBytes[0] && bytes < &bufferBytes[sessionStorage->numBytes];
}

//----------------------------------------------------------------------------------------------------------------------

void HAPThreadSessionStorageCreate(HAPAccessoryServer* server, void* bytes, size_t numBytes) {
    HAPPrecondition(server);
    HAPPrecondition(bytes);

    // Initialize buffer.
    HAPRawBufferZero(&server->sessionStorage, sizeof server->sessionStorage);
    server->sessionStorage.bytes = bytes;
    server->sessionStorage.maxBytes = numBytes;
}

void HAPThreadSessionStorageHandleAccessoryServerWillStart(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);

    HAPError err;

    // Clear buffer.
    err = SetNumBytes(server, 0);
    HAPAssert(!err);

    // Compute offsets.
    size_t numNotifyingCharacteristics = HAPAccessoryServerGetNumCharacteristicsSupportingEventNotification(server);
    size_t numBytesPerBitSet = (numNotifyingCharacteristics + CHAR_BIT - 1) / CHAR_BIT;
    size_t numSessions = HAPAccessoryServerGetNumSessions(server);
    size_t numNotificationBytes = numSessions * kHAPSessionStorage_NumNotificationBitSets * numBytesPerBitSet;
    sessionStorage->notifications.numBytesPerBitSet = numBytesPerBitSet;
    sessionStorage->dataBuffer.startPosition = numNotificationBytes;

    // Allocate memory for event notification related bit sets.
    err = SetNumBytes(server, numNotificationBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPFatalError();
    }

    // Initialize session buffer states.
    for (size_t sessIndex = 0; sessIndex < numSessions; sessIndex++) {
        HAPSession* sess = HAPAccessoryServerGetSessionWithIndex(server, sessIndex);
        HAPThreadSessionStorageState* sessState = GetSessionStorageState(server, sess);
        HAPRawBufferZero(sessState, sizeof *sessState);
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
bool HAPThreadSessionStorageIsEmpty(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPThreadSessionStorageState* sessionState = GetSessionStorageState(server, session);

    for (size_t i = 0; i < kHAPSessionStorage_NumNotificationBitSets; i++) {
        uint8_t* bitSet;
        size_t numBytes;
        HAPThreadSessionStorageGetNotificationBitSet(
                server, session, (HAPThreadSessionStorage_NotificationBitSet) i, &bitSet, &numBytes);
        if (!HAPBitSetIsEmpty(bitSet, numBytes)) {
            return false;
        }
    }
    for (size_t i = 0; i < kHAPSessionStorage_NumDataBuffers; i++) {
        if (sessionState->dataBuffer.numBytes[i]) {
            return false;
        }
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPThreadSessionStorageGetNotificationBitSet(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_NotificationBitSet bitSetType,
        uint8_t* _Nonnull* _Nonnull bitSet,
        size_t* numBytes) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    uint8_t* bufferBytes = sessionStorage->bytes;
    HAPPrecondition(session);
    size_t sessionIndex = HAPAccessoryServerGetSessionIndex(server, session);
    HAPPrecondition(HAPThreadSessionStorage_NotificationBitSet_IsValid(bitSetType));
    HAPPrecondition(bitSet);
    HAPPrecondition(numBytes);

    size_t numBytesPerBitSet = sessionStorage->notifications.numBytesPerBitSet;
    *bitSet = &bufferBytes[(sessionIndex * kHAPSessionStorage_NumNotificationBitSets + bitSetType) * numBytesPerBitSet];
    *numBytes = numBytesPerBitSet;
    HAPAssert(IsInBuffer(server, *bitSet));
    HAPAssert(!*numBytes || IsInBuffer(server, &(*bitSet)[*numBytes - 1]));
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static size_t GetDataBufferPosition(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    HAPPrecondition(session);
    size_t sessionIndex = HAPAccessoryServerGetSessionIndex(server, session);
    HAPThreadSessionStorageState* sessionState = GetSessionStorageState(server, session);
    HAPPrecondition(HAPThreadSessionStorage_DataBuffer_IsValid(dataBufferType));

    size_t position = sessionStorage->dataBuffer.startPosition;
    for (size_t sessIndex = 0; sessIndex < sessionIndex; sessIndex++) {
        HAPSession* sess = HAPAccessoryServerGetSessionWithIndex(server, sessIndex);
        HAPThreadSessionStorageState* sessState = GetSessionStorageState(server, sess);

        for (size_t i = 0; i < HAPArrayCount(sessState->dataBuffer.numBytes); i++) {
            position += sessState->dataBuffer.numBytes[i];
        }
    }
    for (size_t i = 0; i < dataBufferType; i++) {
        position += sessionState->dataBuffer.numBytes[i];
    }
    return position;
}

void HAPThreadSessionStorageGetData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* _Nonnull* _Nonnull bytes,
        size_t* numBytes) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    uint8_t* bufferBytes = sessionStorage->bytes;
    HAPPrecondition(session);
    HAPThreadSessionStorageState* sessionState = GetSessionStorageState(server, session);
    HAPPrecondition(HAPThreadSessionStorage_DataBuffer_IsValid(dataBufferType));
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    *numBytes = sessionState->dataBuffer.numBytes[dataBufferType];
    if (*numBytes) {
        *bytes = &bufferBytes[GetDataBufferPosition(server, session, dataBufferType)];
        HAPAssert(IsInBuffer(server, *bytes));
        HAPAssert(IsInBuffer(server, &((const uint8_t*) *bytes)[*numBytes - 1]));
    } else {
        static uint8_t emptyBytes[1] = { 0 };
        *bytes = emptyBytes;
    }
}

HAP_RESULT_USE_CHECK
static HAPError SetDataBuffer(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        size_t minBytes,
        HAPThreadSessionStorageDataSource dataSource,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    HAPPrecondition(!sessionStorage->isEditing);
    uint8_t* bufferBytes = sessionStorage->bytes;
    HAPPrecondition(session);
    size_t sessionIndex = HAPAccessoryServerGetSessionIndex(server, session);
    HAPThreadSessionStorageState* sessionState = GetSessionStorageState(server, session);
    HAPPrecondition(HAPThreadSessionStorage_DataBuffer_IsValid(dataBufferType));
    HAPPrecondition(dataSource);

    HAPError err;

    if (sessionState->dataBuffer.numBytes[dataBufferType]) {
        HAPLogError(&logObject, "Data buffer %zu/%d is already in use.", sessionIndex, dataBufferType);
        return kHAPError_InvalidState;
    }

    // Check to see that there will be enough memory.
    if (minBytes) {
        sessionStorage->dataBuffer.peakNumBytes[dataBufferType] =
                HAPMax(minBytes, sessionStorage->dataBuffer.peakNumBytes[dataBufferType]);
        if (sessionStorage->numBytes + minBytes > sessionStorage->maxBytes) {
            return kHAPError_OutOfResources;
        }
    }

    sessionStorage->isEditing = true;

    // Gather information.
    size_t position = GetDataBufferPosition(server, session, dataBufferType);
    size_t maxBytes = sessionStorage->maxBytes - sessionStorage->numBytes;
    size_t numBytesAfter = sessionStorage->numBytes - position;

    HAPAssert(sessionState->dataBuffer.numBytes[dataBufferType] == 0);
    // Move all following data buffers away to allocate a scratch buffer with correct start address.
    HAPRawBufferCopyBytes(&bufferBytes[position + maxBytes], &bufferBytes[position], numBytesAfter);
    HAPRawBufferZero(&bufferBytes[position], maxBytes);

    // Fill data buffer.
    size_t numBytes;
    err = dataSource(context, server, session, dataBufferType, &bufferBytes[position], maxBytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "Session storage buffer is not large enough (%zu bytes).", sessionStorage->maxBytes);
        numBytes = 0;
    }
    HAPAssert(numBytes <= maxBytes);

    // Resize scratch buffer to final size.
    HAPRawBufferCopyBytes(&bufferBytes[position + numBytes], &bufferBytes[position + maxBytes], numBytesAfter);
    HAPRawBufferZero(&bufferBytes[position + numBytes + numBytesAfter], maxBytes - numBytes);
    sessionState->dataBuffer.numBytes[dataBufferType] = numBytes;

    // Finalize memory allocation.
    if (!err) {
        sessionStorage->dataBuffer.peakNumBytes[dataBufferType] =
                HAPMax(numBytes, sessionStorage->dataBuffer.peakNumBytes[dataBufferType]);
        err = SetNumBytesWithOptionalErase(server, sessionStorage->numBytes + numBytes, false);
        HAPAssert(!err);
    }

    sessionStorage->isEditing = false;
    return err;
}

typedef struct {
    const void* bytes;
    size_t numBytes;
} StaticDataSourceContext;

HAP_RESULT_USE_CHECK
static HAPError StaticDataSource(
        void* _Nullable context_,
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(context_);
    StaticDataSourceContext* context = context_;
    HAPPrecondition(context->bytes);
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(HAPThreadSessionStorage_DataBuffer_IsValid(dataBufferType));
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    // SetDataBuffer pre-allocates the memory and fails early, so we can assert having enough memory here.
    HAPAssert(maxBytes >= context->numBytes);

    HAPRawBufferCopyBytes(bytes, context->bytes, context->numBytes);
    *numBytes = context->numBytes;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPThreadSessionStorageSetData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        const void* bytes,
        size_t numBytes) {
    StaticDataSourceContext dataSourceContext;
    HAPRawBufferZero(&dataSourceContext, sizeof dataSourceContext);
    dataSourceContext.bytes = bytes;
    dataSourceContext.numBytes = numBytes;
    return SetDataBuffer(
            server, session, dataBufferType, /* minBytes: */ numBytes, StaticDataSource, &dataSourceContext);
}

HAP_RESULT_USE_CHECK
HAPError HAPThreadSessionStorageSetDynamicData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType,
        HAPThreadSessionStorageDataSource dataSource,
        void* _Nullable context) {
    return SetDataBuffer(server, session, dataBufferType, /* minBytes: */ 0, dataSource, context);
}

void HAPThreadSessionStorageClearData(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPThreadSessionStorage_DataBuffer dataBufferType) {
    HAPPrecondition(server);
    HAPThreadSessionStorage* sessionStorage = GetSessionStorage(server);
    HAPPrecondition(!sessionStorage->isEditing);
    uint8_t* bufferBytes = sessionStorage->bytes;
    HAPPrecondition(session);
    HAPThreadSessionStorageState* sessionState = GetSessionStorageState(server, session);
    HAPPrecondition(HAPThreadSessionStorage_DataBuffer_IsValid(dataBufferType));

    HAPError err;

    if (!sessionState->dataBuffer.numBytes[dataBufferType]) {
        return;
    }

    // Gather information.
    size_t position = GetDataBufferPosition(server, session, dataBufferType);
    size_t numBytes = sessionState->dataBuffer.numBytes[dataBufferType];
    size_t numBytesAfter = sessionStorage->numBytes - numBytes - position;

    // Move following data buffers into cleared data buffer.
    HAPRawBufferCopyBytes(&bufferBytes[position], &bufferBytes[position + numBytes], numBytesAfter);
    HAPRawBufferZero(&bufferBytes[position + numBytesAfter], numBytes);
    sessionState->dataBuffer.numBytes[dataBufferType] = 0;

    // Finalize memory allocation.
    err = SetNumBytes(server, sessionStorage->numBytes - numBytes);
    HAPAssert(!err);
}

#endif
