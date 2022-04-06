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

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPDataStream+HAP.h"
#include "HAPDataStream+Internal.h"
#include "HAPDataStream.h"
#include "HAPDataStreamRef.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPService.h"
#include "HAPServiceTypes.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStream" };

#define GetTransportState(server) (&((server)->dataStream.transportState.hap))

static void UpdateSetupTimerState(HAPAccessoryServer* server);

/**
 * Returns the HomeKit Data Stream for which setup has been prepared.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamHAP* _Nullable DataStreamSetupGetDataStream(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        if (dataStream->state == HAPDataStreamHAPState_SetupBegun) {
            HAPAssert(dataStream->session);
            if (dataStream->session->transportType == kHAPTransportType_BLE) {
                HAPAssert(!HAPRawBufferIsZero(dataStream->sessionSecret, sizeof dataStream->sessionSecret));
            }
            return dataStream;
        }
    }

    return NULL;
}

/**
 * Returns the HomeKit Data Stream for a given session identifier.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamHAP* _Nullable DataStreamGetDataStream(
        HAPAccessoryServer* server,
        HAPDataStreamHAPSessionIdentifier sessionIdentifier) {
    HAPPrecondition(server);

    if (sessionIdentifier == kHAPDataStreamHAPSessionIdentifierNone) {
        return NULL;
    }

    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        if (dataStream->session && dataStream->sessionIdentifier == sessionIdentifier) {
            return dataStream;
        }
    }

    return NULL;
}

/**
 * Returns the HomeKit Data Stream with a pending read for a session.
 */
HAP_RESULT_USE_CHECK
static HAPDataStreamHAP* _Nullable DataStreamGetDataStreamToReadFromSession(
        HAPAccessoryServer* server,
        HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        if (dataStream->session != session) {
            continue;
        }

        if ((session->transportType == kHAPTransportType_BLE) &&
            !HAPRawBufferAreEqual(dataStream->sessionSecret, session->hap.cv_KEY, sizeof dataStream->sessionSecret)) {
            continue;
        }

        switch (dataStream->state) {
            default:
            case HAPDataStreamHAPState_Uninitialized:
            case HAPDataStreamHAPState_SetupBegun:
            case HAPDataStreamHAPState_WaitingForFirstFrame:
            case HAPDataStreamHAPState_Active_Idle:
            case HAPDataStreamHAPState_Active_RequestedToSend:
            case HAPDataStreamHAPState_Active_HandlingWrite:
                break;

            case HAPDataStreamHAPState_Active_WrittenPendingRead:
                // Found it!
                return dataStream;
        }
    }

    return NULL;
}

/** Check whether clients should even know of this stream yet (bug check) */
HAP_RESULT_USE_CHECK
static bool IsSetupComplete(HAPDataStreamHAP const* dataStream) {
    HAPPrecondition(dataStream);
    switch (dataStream->state) {
        default:
        case HAPDataStreamHAPState_Uninitialized:
        case HAPDataStreamHAPState_SetupBegun:
        case HAPDataStreamHAPState_WaitingForFirstFrame:
            return false;

        case HAPDataStreamHAPState_Active_Idle:
        case HAPDataStreamHAPState_Active_HandlingWrite:
        case HAPDataStreamHAPState_Active_WrittenPendingRead:
        case HAPDataStreamHAPState_Active_RequestedToSend:
            return true;
    }
}

unsigned HAPDataStreamGetMaxControllerTransportMTU(HAPAccessoryServer* server HAP_UNUSED) {
    // For now, this is just hard-coded but could be made configurable.
    return kHAPDataStream_HAPTransport_DefaultMaxControllerTransportMTU;
}

/** Reset a Data Stream state. Clean up resources if necessary (currently none). */
static void ResetDataStreamState(HAPDataStreamHAP* dataStream) {
    HAPPrecondition(dataStream);
    HAPRawBufferZero(dataStream, sizeof *dataStream);
}

static bool DataStreamNeedsToSend(HAPDataStreamHAP const* dataStream) {
    // Delegate invoking "Send" sets the tx->state to something other than Idle.
    return dataStream->accessoryToController.tx.state != kHAPDataStreamTransmissionState_Idle;
}

static bool DataStreamNeedsSendInterrupt(HAPDataStreamHAP const* dataStream) {
    // Being in Idle state with something to send requires an Interrupt notification.
    return dataStream->state == HAPDataStreamHAPState_Active_Idle && DataStreamNeedsToSend(dataStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Prepares invalidation of a HomeKit Data Stream.
 */
static void SetPendingDestroy(HAPAccessoryServer* server, HAPDataStreamHAP* dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);

    // Set this flag, which will get handled at a good stopping point (eg, when events are flushed).
    dataStream->pendingDestroy = true;
}

static void NotifyDelegateOfCompletion(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPError error,
        HAPDataStreamHAPUnidirectionalState* state,
        bool isComplete) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPPrecondition(state);

    HAPDataStreamTransmission* const tx = &state->tx;

    HAPDataStreamDataCompletionHandler completionHandler = tx->completionHandler;
    void* dataBytes = tx->data.mutableBytes;
    size_t numDataBytes = (size_t) tx->numDataBytes;

    if (isComplete) {
        // Reset transmission for cases when callback reenters.
        HAPRawBufferZero(tx, sizeof *tx);
    } else {
        // Prepare transmission for next chunk for cases when callback reenters.
        tx->completionHandler = NULL;
        tx->dataIsMutable = false;
        tx->data.mutableBytes = NULL;
        tx->numDataBytes = 0;
    }

    // Inform delegate.
    HAPDataStreamRequest request;
    HAPDataStreamGetRequestContext(server, dataStream_, &request);
    HAPAssert(completionHandler);
    completionHandler(server, &request, dataStream_, error, dataBytes, numDataBytes, isComplete, server->context);
}

/**
 * Receive bytes into a buffer incrementally.
 *
 * Some data structures must be completed before they are usable. This allows partial data to be received over
 * multiple buffer transactions (using 'numProgressBytes' to keep track of the progress).
 *
 * @param      tx                   The transmission state to use for progress tracking.
 * @param      buffer               Where to write the data (if NULL, the data is "skipped").
 * @param      totalBytesNeeded     The desired total amount needed to be complete.
 * @param      sourceBuffer         The buffer to read from (will be updated to reflect progress).
 * @param      sourceBufferSize     The available size of the buffer to read from (will be updated to reflect progress).
 *
 * @return     true if 'totalBytesNeeded' has been met.
 */
HAP_RESULT_USE_CHECK
static bool BuildBytesIntoBuffer(
        HAPDataStreamTransmission* tx,
        uint8_t* _Nullable buffer,
        size_t totalBytesNeeded,
        const uint8_t* _Nonnull* _Nonnull sourceBuffer,
        size_t* sourceBufferSize) {
    HAPPrecondition(tx);
    HAPPrecondition(sourceBuffer);
    HAPPrecondition(*sourceBuffer);
    HAPPrecondition(sourceBufferSize);

    if (tx->numProgressBytes < totalBytesNeeded) {
        uint8_t* const cursor = buffer + tx->numProgressBytes;
        size_t needed = totalBytesNeeded - (size_t) tx->numProgressBytes;
        HAPAssert(needed);

        /* Receive data. */
        size_t consumed = HAPMin(needed, *sourceBufferSize);

        /* Consume the bytes */
        if (buffer) {
            HAPRawBufferCopyBytes(cursor, *sourceBuffer, consumed);
        }

        *sourceBuffer += consumed;
        *sourceBufferSize -= consumed;

        tx->numProgressBytes += consumed;
    }

    if (tx->numProgressBytes == totalBytesNeeded) {
        tx->numProgressBytes = 0;
        return true;
    }

    return false;
}

/**
 * Send bytes into a buffer incrementally.
 *
 * Some data structures must be completed before they are usable. This allows partial data to be received over
 * multiple buffer transactions (using 'numProgressBytes' to keep track of the progress).
 *
 * @param      tx                   The transmission state to use for progress tracking.
 * @param      buffer               Where to write the data.
 * @param      totalBytesNeeded     The desired total amount needed to be complete.
 * @param      sourceBuffer         The buffer to read from (will be updated to reflect progress).
 * @param      sourceBufferSize     The available size of the buffer to read from (will be updated to reflect progress).
 *
 * @return     true if 'totalBytesNeeded' has been met.
 */
HAP_RESULT_USE_CHECK
static bool ConsumeBytesFromBuffer(
        HAPDataStreamTransmission* tx,
        uint8_t* _Nonnull* _Nonnull destBuffer,
        size_t* destBufferSize,
        const uint8_t* sourceBuffer,
        size_t sourceBufferSize) {
    HAPPrecondition(tx);
    HAPPrecondition(destBuffer);
    HAPPrecondition(destBufferSize);
    HAPPrecondition(sourceBuffer);

    if (tx->numProgressBytes < sourceBufferSize) {
        const uint8_t* const cursor = sourceBuffer + tx->numProgressBytes;
        size_t needed = sourceBufferSize - (size_t) tx->numProgressBytes;
        HAPAssert(needed);

        /* Receive data. */
        size_t consumed = HAPMin(needed, *destBufferSize);

        /* Consume the bytes */
        HAPRawBufferCopyBytes(*destBuffer, cursor, consumed);

        *destBuffer += consumed;
        *destBufferSize -= consumed;

        tx->numProgressBytes += consumed;
    }

    if (tx->numProgressBytes == sourceBufferSize) {
        tx->numProgressBytes = 0;
        return true;
    }

    return false;
}

/**
 * The first frame must be sent in one chunk. Check that the frame mostly looks right,
 * and then pass it off to the actual receive state machine.
 */
HAP_RESULT_USE_CHECK
static bool HandleTransportWriteForFirstFrame(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* dataStream = &dataStream_->hap;
    HAPPrecondition(!dataStream->pendingDestroy);

    HAPDataStreamHAPUnidirectionalState* const state = &dataStream->controllerToAccessory;
    HAPDataStreamTransmission* const tx = &state->tx;

    HAPAssert(dataStream->state == HAPDataStreamHAPState_WaitingForFirstFrame);

    uint8_t const* buffer = state->pendingData.buffer;
    size_t const originalFrameSize = state->pendingData.size;

    // The first frame always comes in one write, so we can validate it directly.

    // Sanity check.
    if (originalFrameSize < HAP_DATASTREAM_FRAME_HEADER_LENGTH) {
        HAPLog(&logObject,
               "[%p] Received initial frame but header too short (%lu bytes).",
               (const void*) dataStream,
               (unsigned long) originalFrameSize);
        return false;
    }

    // Handle Length.
    uint32_t totalDataBytes = HAPReadBigUInt24(&buffer[1]);
    if (totalDataBytes > sizeof tx->_.initialFrame.bytes) {
        HAPLog(&logObject,
               "[%p] Received initial frame with unexpected Length: %lu.",
               (const void*) dataStream,
               (unsigned long) totalDataBytes);
        return false;
    }

    if (originalFrameSize != HAP_DATASTREAM_FRAME_HEADER_LENGTH + totalDataBytes) {
        HAPLog(&logObject,
               "[%p] Received initial frame but has invalid total size (payload %lu bytes; total frame %lu bytes).",
               (const void*) dataStream,
               (unsigned long) totalDataBytes,
               (unsigned long) originalFrameSize);
        return false;
    }

    // First frame sanity check looks good!

    HAPLogInfo(&logObject, "[%p] Received initial frame.", (const void*) dataStream_);

    // Mark setup complete.
    dataStream->setupTTL = 0;
    // Client is about to get first callback, so move to a state where it may start using the Data Stream.
    dataStream->state = HAPDataStreamHAPState_Active_Idle;

    // Allow delegate to plug in own completion handler.
    HAPAssert(!tx->completionHandler);
    tx->dataIsMutable = false;
    tx->data.mutableBytes = NULL;
    tx->numDataBytes = 0;

    // Inform delegate.
    HAPDataStreamRequest request;
    HAPDataStreamHAPGetRequestContext(server, dataStream_, &request);
    HAPAssert(request.accessory->dataStream.delegate.callbacks);
    HAPAssert(request.accessory->dataStream.delegate.callbacks->handleAccept);
    request.accessory->dataStream.delegate.callbacks->handleAccept(server, &request, dataStream_, server->context);
    if (dataStream->pendingDestroy) {
        // Client may have rejected this.
        return false;
    }

    // Set up receiver so it can get the 'data' callback.
    tx->state = kHAPDataStreamTransmissionState_FrameHeader;

    return true;
}

/**
 * One-pass operation on the Controller Write (incoming message) operation. Will get invoked multiple times.
 *
 * @return false if the operation hit a stop-condition (eg, no more data).
 * @return true if the operation hit a continue-condition (eg, called delegate, may have more to do).
 */
HAP_RESULT_USE_CHECK
static bool DoTransportWrite(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(!dataStream->pendingDestroy);

    HAPDataStreamHAPUnidirectionalState* const state = &dataStream->controllerToAccessory;
    HAPDataStreamTransmission* const tx = &state->tx;

    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.2 Frame Format
    switch ((HAPDataStreamTransmissionState) tx->state) {
        default:
        case kHAPDataStreamTransmissionState_Idle:
        case kHAPDataStreamTransmissionState_DataEncrypted:
        case kHAPDataStreamTransmissionState_AuthTag: {
            HAPFatalError();
        }
        case kHAPDataStreamTransmissionState_PrepareFrame: {
            // No preparation actually needed. Just advance to next state.
            HAPLogDebug(&logObject, "[%p] HAP Transport Write: beginning next frame.", (const void*) dataStream_);
            tx->state = kHAPDataStreamTransmissionState_FrameHeader;
            HAP_FALLTHROUGH;
        }
        case kHAPDataStreamTransmissionState_FrameHeader: {
            // Try to consume the frame header (4 bytes) from the send buffer.
            bool completed = BuildBytesIntoBuffer(
                    tx,
                    &tx->scratchBytes[0],
                    HAP_DATASTREAM_FRAME_HEADER_LENGTH,
                    &state->pendingData.buffer,
                    &state->pendingData.size);
            if (!completed) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Write: frame header not yet ready; waiting for next write.",
                        (const void*) dataStream_);
                return false;
            }

            // Check the frame type.
            HAPDataStreamFrameType const frameType = tx->scratchBytes[0];
            bool acceptedFrameType = false;
            switch (frameType) {
                case kHAPDataStreamFrameType_UnencryptedMessage:
                    // Only supported frame type.
                    acceptedFrameType = true;
                    break;
                case kHAPDataStreamFrameType_EncryptedMessage:
                    // Not accepted.
                    break;
            }
            if (!acceptedFrameType) {
                HAPLog(&logObject,
                       "[%p] HAP Transport Write: Received packet with unexpected frame type: 0x%02X (%d bytes).",
                       (const void*) dataStream_,
                       frameType,
                       tx->totalDataBytes);
                SetPendingDestroy(server, dataStream);
                break;
            }

            // Handle Length.
            uint32_t totalDataBytes = HAPReadBigUInt24(&tx->scratchBytes[1]);
            if (totalDataBytes > kHAPDataStream_MaxPayloadBytes) {
                HAPLog(&logObject,
                       "[%p] HAP Transport Write: Received frame with unsupported Length: %lu.",
                       (const void*) dataStream,
                       (unsigned long) totalDataBytes);
                SetPendingDestroy(server, dataStream);
                return false;
            }
            tx->totalDataBytes = totalDataBytes;

            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_DataReadyToAccept;
            HAP_FALLTHROUGH;
        }
        case kHAPDataStreamTransmissionState_DataReadyToAccept: {
            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_Data;

            // Inform delegate.
            HAPLogInfo(
                    &logObject,
                    "[%p] HAP Transport Write: Ready to accept data (%zu bytes).",
                    (const void*) dataStream,
                    (size_t) tx->totalDataBytes);
            HAPDataStreamRequest request;
            HAPDataStreamHAPGetRequestContext(server, dataStream_, &request);
            HAPAssert(request.accessory->dataStream.delegate.callbacks);
            HAPAssert(request.accessory->dataStream.delegate.callbacks->handleData);
            request.accessory->dataStream.delegate.callbacks->handleData(
                    server, &request, dataStream_, (size_t) tx->totalDataBytes, server->context);
            if (dataStream->pendingDestroy) {
                return false;
            }
            HAP_FALLTHROUGH;
        }
        case kHAPDataStreamTransmissionState_Data: {
            // Completion handler must always be called at least once, even if frame is empty.
            // Otherwise there is no way to inform the delegate about decryption issues.
            if (!tx->completionHandler) {
                HAPLogDebug(
                        &logObject, "[%p] HAP Transport Write: no data from delegate yet.", (const void*) dataStream_);
                return false;
            }

            HAPAssert(tx->dataIsMutable == (tx->data.mutableBytes != NULL));
            HAPAssert(tx->numDataBytes <= tx->totalDataBytes);

            bool completed = BuildBytesIntoBuffer(
                    tx,
                    tx->data.mutableBytes,
                    (size_t) tx->numDataBytes,
                    &state->pendingData.buffer,
                    &state->pendingData.size);
            if (!completed) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Write: partial write in progress (%u of %u bytes; %u bytes total "
                        "remaining).",
                        (const void*) dataStream_,
                        (unsigned) tx->numProgressBytes,
                        (unsigned) tx->numDataBytes,
                        (unsigned) tx->totalDataBytes);
                return false;
            }
            tx->totalDataBytes -= tx->numDataBytes;

            // If there is more data incoming, inform delegate that current chunk is done.
            // Data from initial frame is always read in one chunk.
            if (tx->totalDataBytes) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Write: partial write completed (%u bytes; %u bytes remaining).",
                        (const void*) dataStream_,
                        (unsigned) tx->numDataBytes,
                        (unsigned) tx->totalDataBytes);
                NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, false);
                return true; // Re-enter and do more reads.
            }

            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_Idle;

            HAPLogDebug(&logObject, "[%p] HAP Transport Write: frame completed.", (const void*) dataStream_);
            NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, true);

            // Move on to the next frame if possible.
            HAPAssert(tx->state == kHAPDataStreamTransmissionState_Idle);
            tx->state = kHAPDataStreamTransmissionState_PrepareFrame;
            return true;
        }
    }
    HAPFatalError();
    return false;
}

/**
 * One-pass operation on the Controller Read (outgoing message) operation. Will get invoked multiple times.
 *
 * @return false if the operation hit a stop-condition (eg, no more data).
 * @return true if the operation hit a continue-condition (eg, called delegate, may have more to do).
 */
HAP_RESULT_USE_CHECK
static bool DoTransportRead(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(!dataStream->pendingDestroy);

    HAPDataStreamHAPUnidirectionalState* const state = &dataStream->accessoryToController;
    HAPDataStreamTransmission* const tx = &state->tx;

    switch ((HAPDataStreamTransmissionState) tx->state) {
        default:
        case kHAPDataStreamTransmissionState_Idle:
        case kHAPDataStreamTransmissionState_DataReadyToAccept:
        case kHAPDataStreamTransmissionState_DataEncrypted:
        case kHAPDataStreamTransmissionState_AuthTag: {
            HAPFatalError();
        }
        case kHAPDataStreamTransmissionState_PrepareFrame: {
            HAPLogDebug(
                    &logObject,
                    "[%p] HAP Transport Read: beginning next frame. (%u bytes)",
                    (const void*) dataStream_,
                    (unsigned) tx->totalDataBytes);

            // Type. The only supported one.
            tx->scratchBytes[0] = kHAPDataStreamFrameType_UnencryptedMessage;

            // Length has been set to tx->totalDataBytes in HAPDataStreamPrepareData.
            HAPAssert(tx->totalDataBytes <= kHAPDataStream_MaxPayloadBytes);
            HAPWriteBigUInt24(&tx->scratchBytes[1], (size_t) tx->totalDataBytes);

            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_FrameHeader;
            HAP_FALLTHROUGH;
        }
        case kHAPDataStreamTransmissionState_FrameHeader: {
            // Try to write the frame header (4 bytes) into the outgoing buffer.
            bool completed = ConsumeBytesFromBuffer(
                    tx,
                    &state->pendingData.mutableBuffer,
                    &state->pendingData.size,
                    tx->scratchBytes,
                    HAP_DATASTREAM_FRAME_HEADER_LENGTH);
            if (!completed) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Read: frame header not yet ready; waiting for next read.",
                        (const void*) dataStream_);
                return false;
            }

            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_Data;

            HAPAssert(!tx->dataIsMutable);
            HAPAssert(!tx->data.mutableBytes);
            HAPAssert(!tx->numDataBytes);

            // Signal that the Prepare has been completed and the delegate can start pushing data.
            HAPLogDebug(
                    &logObject,
                    "[%p] HAP Transport Read: ready to receive bytes from delegate.",
                    (const void*) dataStream_);
            NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, false);
            if (dataStream->pendingDestroy) {
                return false;
            }

            HAP_FALLTHROUGH;
        }
        case kHAPDataStreamTransmissionState_Data: {
            // HAPDataStreamSendData / HAPDataStreamSendMutableData must always be called, even if frame is empty.
            // This is for consistency with the receive flow.
            if (!tx->completionHandler) {
                // Delegate hasn't given any data yet.
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Read: no data submitted by delegate yet.",
                        (const void*) dataStream_);
                return false;
            }
            if (!state->pendingData.size) {
                // No space to write yet.
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Read: data submitted by delegate but buffer full; waiting for next read",
                        (const void*) dataStream_);
                return false;
            }

            HAPAssert(tx->numDataBytes <= tx->totalDataBytes);

            bool completed = ConsumeBytesFromBuffer(
                    tx,
                    &state->pendingData.mutableBuffer,
                    &state->pendingData.size,
                    tx->data.mutableBytes,
                    tx->numDataBytes);
            if (!completed) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Read: partial read in progress (%u of %u bytes; %u bytes total remaining).",
                        (const void*) dataStream_,
                        (unsigned) tx->numProgressBytes,
                        (unsigned) tx->numDataBytes,
                        (unsigned) tx->totalDataBytes);
                return false;
            }

            HAPAssert(tx->numDataBytes <= tx->totalDataBytes);
            tx->totalDataBytes -= tx->numDataBytes;

            // If there is more data outgoing, inform delegate that current chunk is done.
            if (tx->totalDataBytes) {
                HAPLogDebug(
                        &logObject,
                        "[%p] HAP Transport Read: partial read completed (%u bytes; %u bytes remaining).",
                        (const void*) dataStream_,
                        (unsigned) tx->numDataBytes,
                        (unsigned) tx->totalDataBytes);
                NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, false);
                return true; // Re-enter and do more reads.
            }

            // Advance state.
            tx->state = kHAPDataStreamTransmissionState_Idle;

            HAPLogDebug(&logObject, "[%p] HAP Transport Read: frame completed.", (const void*) dataStream_);
            NotifyDelegateOfCompletion(server, dataStream_, kHAPError_None, state, true);
            return true;
        }
    }
    HAPFatalError();
    return false;
}

/**
 * Run the Transport Write (incoming message) run loop to consume the incoming message buffer.
 */
static void RunTransportWriteRunLoop(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(!dataStream->pendingDestroy);

    HAPDataStreamTransmission* const tx = &dataStream->controllerToAccessory.tx;

    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.2 Frame Format
    while (tx->state != kHAPDataStreamTransmissionState_Idle) {
        bool keepGoing = DoTransportWrite(server, dataStream_);
        if (!keepGoing) {
            break;
        }
        if (dataStream->pendingDestroy) {
            break;
        }
    }
}

/**
 * Run the Transport Read (outgoing message) run loop to fill up the outgoing message buffer.
 */
static void RunTransportReadRunLoop(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(!dataStream->pendingDestroy);

    HAPDataStreamTransmission* const tx = &dataStream->accessoryToController.tx;

    // See HomeKit Accessory Protocol Specification R17
    // Section 9.1.2 Frame Format
    while (tx->state != kHAPDataStreamTransmissionState_Idle) {
        bool keepGoing = DoTransportRead(server, dataStream_);
        if (!keepGoing) {
            break;
        }
        if (dataStream->pendingDestroy) {
            break;
        }
    }
}

/**
 * Handle HAP Transport Write (Controller -> Accessory messaging) and delegate based on the Data Stream state.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleTransportWrite(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        const uint8_t* _Nullable buffer,
        size_t size) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    switch (dataStream->state) {
        default:
        case HAPDataStreamHAPState_Uninitialized:
            // We shouldn't even have gotten here.
            HAPLog(&logObject, "[%p] Got HAP Transport Write but is an unitialized DataStream.", (void*) dataStream_);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_SetupBegun:
            // We shouldn't receive data before setup completes.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Write but was still being set up. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_Active_HandlingWrite: {
            // We shouldn't receive a write while handling a write.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Write while handling a write. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;
        }

        case HAPDataStreamHAPState_Active_WrittenPendingRead: {
            // Two writes without a read in between? yikes.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Write but was expecting a read. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;
        }

        case HAPDataStreamHAPState_WaitingForFirstFrame: {
            // First frame!
            HAPDataStreamHAPUnidirectionalState* const state = &dataStream->controllerToAccessory;

            // Sanity check the buffer to see if it contains the exactly one-expected message (per spec).
            state->pendingData.buffer = buffer;
            state->pendingData.size = size;
            bool success = HandleTransportWriteForFirstFrame(server, dataStream_);
            if (!success) {
                // Nope. Didn't work.
                SetPendingDestroy(server, dataStream);
                break;
            }

            // Sanity check passed, so trigger the receive code (to trigger delegate callbacks).
            RunTransportWriteRunLoop(server, dataStream_);

            // Underflow? Client must handle all bytes (there's no way to store them for later)!
            HAPAssert(!state->pendingData.size);

            UpdateSetupTimerState(server);

            // The Write-Response if followed by an immediate Read, so set up to expect that next.
            dataStream->state = HAPDataStreamHAPState_Active_WrittenPendingRead;
            break;
        }

        case HAPDataStreamHAPState_Active_Idle:
        case HAPDataStreamHAPState_Active_RequestedToSend: {
            // Incoming messaage.
            HAPDataStreamHAPUnidirectionalState* const state = &dataStream->controllerToAccessory;

            // Overflow? Make sure we don't have anything still pending.
            HAPAssert(!state->pendingData.size);

            // If a write occurred without a data paylod, skip the run loop which consumes the incoming message buffer.
            if (buffer && (size > 0)) {
                state->pendingData.buffer = buffer;
                state->pendingData.size = size;

                // Indicate we are actively handling the write portion of a Write-Response.
                dataStream->state = HAPDataStreamHAPState_Active_HandlingWrite;

                // Consume the message.
                RunTransportWriteRunLoop(server, dataStream_);

                // Underflow? Client delegates must handle all incoming data; there's no buffers to store them.
                HAPAssert(!state->pendingData.size);
            }

            // The Write-Response if followed by an immediate Read, so set up to expect that next.
            dataStream->state = HAPDataStreamHAPState_Active_WrittenPendingRead;
            break;
        }
    }

    return kHAPError_None;
}

/**
 * Handle HAP Transport Read (Accessory -> Controller messaging) and delegate based on the Data Stream state.
 */
HAP_RESULT_USE_CHECK
static HAPError HandleTransportRead(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        uint8_t* payload,
        size_t payloadCapacity,
        size_t* payloadSize,
        bool* requestToSend) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPPrecondition(payload);
    HAPPrecondition(payloadSize);
    HAPPrecondition(requestToSend);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    switch (dataStream->state) {
        default:
        case HAPDataStreamHAPState_Uninitialized:
            // We shouldn't even have gotten here.
            HAPLog(&logObject, "[%p] Got HAP Transport Read but is an unitialized DataStream.", (void*) dataStream_);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_SetupBegun:
            // We shouldn't receive data before setup completes.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Read but was still being set up. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_WaitingForFirstFrame:
            // Should not receive a 'read' when we're waiting for a 'write'.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Read without any write (waiting for first frame too). Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_Active_Idle:
        case HAPDataStreamHAPState_Active_RequestedToSend:
            // A read without a write? Yikes.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Read without any write. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;

        case HAPDataStreamHAPState_Active_HandlingWrite: {
            // We shouldn't receive a read while still handling a write.
            HAPLog(&logObject,
                   "[%p] Got HAP Transport Read while handling a write. Closing DataStream.",
                   (void*) dataStream_);
            SetPendingDestroy(server, dataStream);
            return kHAPError_InvalidState;
        }

        case HAPDataStreamHAPState_Active_WrittenPendingRead: {
            // Outgoing message.
            HAPDataStreamHAPUnidirectionalState* const state = &dataStream->accessoryToController;

            state->pendingData.mutableBuffer = payload;
            state->pendingData.size = payloadCapacity;
            RunTransportReadRunLoop(server, dataStream_);

            // Report back how much of the buffer was filled up.
            *payloadSize = (payloadCapacity - state->pendingData.size);

            state->pendingData.mutableBuffer = NULL;
            state->pendingData.size = 0;

            // If the send-state is anything but Idle, then there are still bytes to push out.
            if (DataStreamNeedsToSend(dataStream)) {
                // Controller will continue to pull.
                *requestToSend = true;
                dataStream->state = HAPDataStreamHAPState_Active_RequestedToSend;
            } else {
                // The Write-Response fully complete. Go back to idle waiting for the next Write.
                // (A further send will require an Interrupt).
                *requestToSend = false;
                dataStream->state = HAPDataStreamHAPState_Active_Idle;
            }
            break;
        }
    }

    return kHAPError_None;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void HandleSetupTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;
    HAPPrecondition(timer == GetTransportState(server)->setupTimer);

    HAPLogDebug(&logObject, "HomeKit Data Stream setup timer expired.");
    GetTransportState(server)->setupTimer = 0;

    UpdateSetupTimerState(server);
}

static void UpdateSetupTimerState(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPDataStreamHAPTransportState* const transportState = GetTransportState(server);

    HAPError err;

    // Abort ongoing timer.
    if (transportState->setupTimer) {
        HAPPlatformTimerDeregister(transportState->setupTimer);
        transportState->setupTimer = 0;
    }

    // Advance time.
    HAPTime now = HAPPlatformClockGetCurrent();
    HAPTime delta = now - transportState->referenceTime;
    transportState->referenceTime = now;
    if (delta >= 1 * HAPMillisecond) {
        HAPLogDebug(
                &logObject,
                "Advancing HomeKit Data Stream reference time by %llu.%03llus.",
                (unsigned long long) (delta / HAPSecond),
                (unsigned long long) (delta % HAPSecond));
    }

    // Since the delegate is only informed about HomeKit Data Streams that went through matchmaking successfully,
    // it is not necessary to worry about calling delegate functions in here.

    // Enumerate unmatched setup requests.
    HAPTime minTTL = HAPTime_Max;
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        switch (dataStream->state) {
            default:
            case HAPDataStreamHAPState_Uninitialized:
            case HAPDataStreamHAPState_Active_Idle:
            case HAPDataStreamHAPState_Active_WrittenPendingRead:
            case HAPDataStreamHAPState_Active_RequestedToSend:
                // Not in setup.
                continue;

            case HAPDataStreamHAPState_SetupBegun:
            case HAPDataStreamHAPState_WaitingForFirstFrame:
                break;
        }

        // Check if setup time expired; if so, kill the stream.
        if (dataStream->setupTTL <= delta) {
            HAPLog(&logObject, "[%p] Aborting pending HomeKit Data Stream setup request.", (const void*) dataStream_);
            ResetDataStreamState(dataStream);
            continue;
        }

        // Update remaining time.
        HAPAssert(delta <= UINT16_MAX);
        dataStream->setupTTL -= (uint16_t) delta;
        minTTL = HAPMin(dataStream->setupTTL, minTTL);
    }

    // Schedule timer.
    if (minTTL != HAPTime_Max) {
        // NOLINTNEXTLINE(bugprone-branch-clone)
        HAPAssert(minTTL <= kHAPDataStream_SetupTimeout);
        err = HAPPlatformTimerRegister(&transportState->setupTimer, now + minTTL, HandleSetupTimerExpired, server);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLogError(&logObject, "Not enough timers available (HomeKit Data Stream setup timer).");
            HAPFatalError();
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPSetupBegin(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session,
        const HAPDataStreamControllerSetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(HAPSessionIsSecured(session) && !HAPSessionIsTransient(session));
    HAPPrecondition(setupParams);

    // An encrypted HDS requires a key-salt from the controller.
    if (setupParams->controllerKeySalt) {
        HAPLogError(&logObject, "Setup failed: Controller Key Salt present but not used for HAP DataStream.");
        return kHAPError_InvalidData;
    }

    // // Update reference time for timeouts.
    UpdateSetupTimerState(server);

    // Only one HomeKit Data Stream may be set up at a time.
    HAPPrecondition(!DataStreamSetupGetDataStream(server));

    // Find memory to register setup request.
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        if (dataStream->state != HAPDataStreamHAPState_Uninitialized) {
            continue;
        }

        HAPAssert(HAPRawBufferIsZero(&dataStream->accessoryToController, sizeof dataStream->accessoryToController));
        HAPAssert(HAPRawBufferIsZero(&dataStream->controllerToAccessory, sizeof dataStream->controllerToAccessory));
        HAPAssert(!dataStream->session);
        HAPAssert(!dataStream->serviceTypeIndex);
        HAPAssert(!dataStream->setupTTL);
        if (session->transportType == kHAPTransportType_BLE) {
            HAPAssert(HAPRawBufferIsZero(dataStream->sessionSecret, sizeof dataStream->sessionSecret));
        }

        // Register TTL.
        dataStream->setupTTL = kHAPDataStream_SetupTimeout;

        // Register setup request.
        dataStream->session = session;
        dataStream->pairingID = session->hap.pairingID;
        dataStream->serviceTypeIndex = HAPAccessoryServerGetServiceTypeIndex(server, service, accessory);
        dataStream->state = HAPDataStreamHAPState_SetupBegun;
        if (session->transportType == kHAPTransportType_BLE) {
            HAPRawBufferCopyBytes(dataStream->sessionSecret, session->hap.cv_KEY, sizeof dataStream->sessionSecret);
        }

        // Start enforcing TTL and ensure TCP stream listener is open.
        HAPLogInfo(&logObject, "[%p] HomeKit Data Stream setup request allocated.", (const void*) dataStream_);
        UpdateSetupTimerState(server);
        return kHAPError_None;
    }

    HAPLogError(
            &logObject,
            "No space to allocate HomeKit Data Stream setup request (%zu max). Adjust HAPAccessoryServerOptions.",
            server->dataStream.numDataStreams);
    return kHAPError_OutOfResources;
}

static HAPDataStreamHAPSessionIdentifier PickNextSessionIdentifier(HAPAccessoryServer* server) {
    // Do a linear search to find the first unused one. This relies on wrapping around to zero.
    HAPDataStreamHAPSessionIdentifier nextSessionIdentifier = GetTransportState(server)->lastSessionIdentifier;
    HAPDataStreamHAPSessionIdentifier sessionIdentifier = nextSessionIdentifier;
    do {
        ++sessionIdentifier;

        // Never pick the None sentinel.
        if (sessionIdentifier == kHAPDataStreamHAPSessionIdentifierNone) {
            continue;
        }

        // Found one!
        if (!DataStreamGetDataStream(server, sessionIdentifier)) {
            GetTransportState(server)->lastSessionIdentifier = sessionIdentifier;
            return sessionIdentifier;
        }
    } while (sessionIdentifier != nextSessionIdentifier);

    // We should never get through all of them (as we have a limited number of streams and we never have 255 of them).
    HAPFatalError();
}

static void
        FillInAccessorySetupParamsForHAP(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams) {
    setupParams->sessionIdentifier = PickNextSessionIdentifier(server);
    HAPAssert(setupParams->sessionIdentifier != kHAPDataStreamHAPSessionIdentifierNone);
}

HAP_RESULT_USE_CHECK
HAPError HAPDataStreamHAPSetupComplete(HAPAccessoryServer* server, HAPDataStreamAccessorySetupParams* setupParams) {
    HAPPrecondition(server);
    HAPPrecondition(setupParams);

    // Reset them all to zero just in case.
    HAPRawBufferZero(setupParams, sizeof *setupParams);

    HAPDataStreamHAP* const dataStream = DataStreamSetupGetDataStream(server);
    if (!dataStream) {
        HAPLog(&logObject, "HomeKit Data Stream setup has not been prepared or has timed out.");
        return kHAPError_InvalidState;
    }
    HAPAssert(dataStream->state == HAPDataStreamHAPState_SetupBegun);

    FillInAccessorySetupParamsForHAP(server, setupParams);

    HAPLogInfo(
            &logObject,
            "[%p] HomeKit Data Stream setup request registered (sessionIdentifier=0x%x).",
            (const void*) dataStream,
            setupParams->sessionIdentifier);

    // Complete setup.
    dataStream->state = HAPDataStreamHAPState_WaitingForFirstFrame;
    dataStream->sessionIdentifier = setupParams->sessionIdentifier;

    return kHAPError_None;
}

void HAPDataStreamHAPSetupCancel(HAPAccessoryServer* server) {
    HAPPrecondition(server);

    HAPDataStreamRef* const dataStream = (HAPDataStreamRef*) DataStreamSetupGetDataStream(server);
    if (!dataStream) {
        return;
    }

    HAPLog(&logObject,
           "[%p] Canceling pending HomeKit Data Stream setup request (setup not completed).",
           (const void*) dataStream);

    HAPDataStreamHAPInvalidate(server, dataStream);
}

HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamHAPGetReceiveTransmission(HAPDataStreamRef* dataStream_) {
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(IsSetupComplete(dataStream));
    HAPPrecondition(!dataStream->pendingDestroy);

    return &dataStream->controllerToAccessory.tx;
}

HAP_RESULT_USE_CHECK
HAPDataStreamTransmission* HAPDataStreamHAPGetSendTransmission(HAPDataStreamRef* dataStream_) {
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(IsSetupComplete(dataStream));
    HAPPrecondition(!dataStream->pendingDestroy);

    return &dataStream->accessoryToController.tx;
}

void HAPDataStreamHAPGetRequestContext(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream_,
        HAPDataStreamRequest* request) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;
    HAPPrecondition(request);

    HAPRawBufferZero(request, sizeof *request);

    request->transportType = kHAPTransportType_IP;
    request->session = HAPNonnull(dataStream->session);
    HAPAccessoryServerGetServiceFromServiceTypeIndex(
            server,
            &kHAPServiceType_DataStreamTransportManagement,
            dataStream->serviceTypeIndex,
            &request->service,
            &request->accessory);
}

void HAPDataStreamHAPDoReceive(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    // Nothing to do. The interest to receive data has been registered and will be handled
    // the next time the controller writes.
}

static void RaiseInterruptNotification(HAPAccessoryServer* server, HAPDataStreamHAP* dataStream) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream);

    GetTransportState(server)->interruptSequenceNumber++;

    // Get the service and accessory handles.
    const HAPService* service;
    const HAPAccessory* accessory;
    HAPAccessoryServerGetServiceFromServiceTypeIndex(
            server, &kHAPServiceType_DataStreamTransportManagement, dataStream->serviceTypeIndex, &service, &accessory);

    const HAPCharacteristic* _Nullable characteristic =
            HAPServiceGetCharacteristic(service, &kHAPCharacteristicType_DataStreamHAPTransportInterrupt);
    if (!characteristic) {
        HAPLogServiceError(
                &logObject,
                service,
                accessory,
                "[%p] Not notifying as %s characteristic not available.",
                (const void*) dataStream,
                kHAPCharacteristicDebugDescription_DataStreamHAPTransportInterrupt);
    } else {
        HAPLogCharacteristicDebug(
                &logObject,
                characteristic,
                service,
                accessory,
                "[%p] Reporting HAP transport interrupt change.",
                (const void*) dataStream);
        HAPAccessoryServerRaiseEvent(server, HAPNonnullVoid(characteristic), service, accessory);
    }
}

void HAPDataStreamHAPDoSend(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    // If this send causes a need to trigger a send-interrupt, signal that here.
    if (DataStreamNeedsSendInterrupt(dataStream)) {
        RaiseInterruptNotification(server, dataStream);
    } else {
        HAPLogDebug(
                &logObject,
                "No need to raise interrupt (state==%u and tx->state==%u)",
                dataStream->state,
                dataStream->accessoryToController.tx.state);
    }

    // Otherwise, nothing to do, as the next read-write pattern will pull this data next.
}

HAPError HAPDataStreamHAPTransportHandleWrite(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPDataStreamHAPSessionIdentifier sessionIdentifier,
        const uint8_t* _Nullable payload,
        size_t payloadSize,
        bool forceClose) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(payload || payloadSize == 0);

    HAPError err;

    if (HAPDataStreamGetActiveTransportForSession(server, session) != kHAPDataStreamTransport_HAP) {
        HAPLogError(&logObject, "HAP Transport Write: DataStream is not active on HAP.");
        return kHAPError_InvalidState;
    }

    HAPDataStreamHAP* const dataStream = DataStreamGetDataStream(server, sessionIdentifier);
    if (!dataStream) {
        HAPLog(&logObject, "HAP Transport Write: no stream with sessionIdentifier==%u", sessionIdentifier);
        return kHAPError_InvalidState;
    }
    if (dataStream->session != session) {
        HAPLog(&logObject,
               "[%p] HAP Transport Write: receive on sessionIdentifier==%u from wrong HAPSession",
               (void*) dataStream,
               sessionIdentifier);
        return kHAPError_InvalidState;
    } else if (
            (session->transportType == kHAPTransportType_BLE) &&
            !HAPRawBufferAreEqual(dataStream->sessionSecret, session->hap.cv_KEY, sizeof dataStream->sessionSecret)) {
        HAPLog(&logObject,
               "[%p] HAP Transport Write: receive on sessionIdentifier==%u from wrong HAPSession",
               (void*) dataStream,
               sessionIdentifier);
        return kHAPError_InvalidState;
    }

    HAPAssert(!dataStream->pendingClose);
    if (dataStream->pendingClose) {
        return kHAPError_InvalidState;
    } else if (forceClose) {
        // Set up the data stream for invalidation in the read handler.
        // This allows an empty response to be generated to the close request per the spec.
        dataStream->pendingClose = true;
        dataStream->state = HAPDataStreamHAPState_Active_WrittenPendingRead;
        return kHAPError_None;
    }

    HAPAssert(!dataStream->isHandlingEvent);
    dataStream->isHandlingEvent = true;

    HAPDataStreamRef* const dataStream_ = (HAPDataStreamRef*) dataStream;
    err = HandleTransportWrite(server, dataStream_, payload, payloadSize);

    HAPAssert(dataStream->isHandlingEvent);
    dataStream->isHandlingEvent = false;

    // Invalidation gets queued while handling events. Catch up here if needed.
    if (dataStream->pendingDestroy) {
        HAPDataStreamInvalidate(server, dataStream_);
    }

    return err;
}

HAPError HAPDataStreamHAPTransportHandleRead(
        HAPAccessoryServer* server,
        HAPSession* session,
        uint8_t* payload,
        size_t payloadCapacity,
        size_t* payloadSize,
        bool* requestToSend,
        bool* sendEmptyResponse) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(payload);
    HAPPrecondition(payloadSize);
    HAPPrecondition(requestToSend);
    HAPPrecondition(sendEmptyResponse);

    HAPError err = kHAPError_None;

    if (HAPDataStreamGetActiveTransportForSession(server, session) != kHAPDataStreamTransport_HAP) {
        HAPLogError(&logObject, "HAP Transport Receive: DataStream is not active on HAP.");
        return kHAPError_InvalidState;
    }

    HAPDataStreamHAP* const dataStream = DataStreamGetDataStreamToReadFromSession(server, session);
    if (!dataStream) {
        HAPLog(&logObject, "HAP Transport Receive: no stream is expecting a read on session %p", (void*) session);
        return kHAPError_InvalidState;
    }

    HAPDataStreamRef* const dataStream_ = (HAPDataStreamRef*) dataStream;

    if (dataStream->pendingClose) {
        // Generate empty response to the close request and mark the data stream for invalidation.
        *sendEmptyResponse = true;
        SetPendingDestroy(server, dataStream);
    } else {
        HAPAssert(!dataStream->isHandlingEvent);
        dataStream->isHandlingEvent = true;

        err = HandleTransportRead(server, dataStream_, payload, payloadCapacity, payloadSize, requestToSend);
        *sendEmptyResponse = false;

        HAPAssert(dataStream->isHandlingEvent);
        dataStream->isHandlingEvent = false;
    }

    // Invalidation gets queued while handling events. Catch up here if needed.
    if (dataStream->pendingDestroy) {
        HAPDataStreamInvalidate(server, dataStream_);
    }

    return err;
}

HAPError HAPDataStreamHAPTransportGetInterruptState(
        HAPAccessoryServer* server,
        uint8_t* requestToSendSessionIdentifiers,
        size_t requestToSendSessionIdentifiersCapacity,
        size_t* requestToSendSessionIdentifiersSize,
        HAPDataStreamInterruptSequenceNumber* interruptSequenceNumber) {
    HAPPrecondition(server);
    HAPPrecondition(requestToSendSessionIdentifiers);
    HAPPrecondition(requestToSendSessionIdentifiersSize);

    size_t written = 0;
    for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
        HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
        HAPDataStreamHAP* const dataStream = &dataStream_->hap;

        // A Data Stream has an interrupt if it's in the idle state but also has data to send.
        if (!DataStreamNeedsSendInterrupt(dataStream)) {
            continue;
        }

        if (written >= requestToSendSessionIdentifiersCapacity) {
            HAPLogError(
                    &logObject,
                    "[%p] Unable to write all request to send identifiers (capacity=%zu).",
                    (const void*) dataStream_,
                    requestToSendSessionIdentifiersCapacity);
            return kHAPError_OutOfResources;
        }

        requestToSendSessionIdentifiers[written++] = dataStream->sessionIdentifier;
    }

    *requestToSendSessionIdentifiersSize = written;
    *interruptSequenceNumber = GetTransportState(server)->interruptSequenceNumber;
    return kHAPError_None;
}

void HAPDataStreamHAPInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);
    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    SetPendingDestroy(server, dataStream);

    // If stream events are being handled, HomeKit Data Stream state will still be accessed.
    // Therefore, resources may not yet be released. HAPDataStreamInvalidate is called again after events are handled.
    if (dataStream->isHandlingEvent) {
        HAPLogDebug(
                &logObject, "[%p] Stream events are being handled. Delaying invalidation.", (const void*) dataStream_);
        return;
    }

    switch (dataStream->state) {
        default:
        case HAPDataStreamHAPState_Uninitialized:
            // We shouldn't even have gotten here.
            HAPFatalError();
            break;

        case HAPDataStreamHAPState_SetupBegun:
        case HAPDataStreamHAPState_WaitingForFirstFrame: {
            // Delegate has never seen this stream.
            HAPLogInfo(&logObject, "[%p] Invalidating HAP stream.", (const void*) dataStream_);

            // Invalidate HomeKit Data Stream.
            ResetDataStreamState(dataStream);

            // Maybe update timers.
            UpdateSetupTimerState(server);
            break;
        }

        case HAPDataStreamHAPState_Active_Idle:
        case HAPDataStreamHAPState_Active_WrittenPendingRead:
        case HAPDataStreamHAPState_Active_RequestedToSend: {
            HAPLogInfo(&logObject, "[%p] Invalidating HomeKit Data Stream.", (const void*) dataStream_);

            // Abort transmissions.
            dataStream->isHandlingEvent = true;
            if (dataStream->accessoryToController.tx.completionHandler) {
                HAPDataStreamHAPUnidirectionalState* const state = &dataStream->accessoryToController;
                NotifyDelegateOfCompletion(server, dataStream_, kHAPError_InvalidState, state, true);
            }
            if (dataStream->controllerToAccessory.tx.completionHandler) {
                HAPDataStreamHAPUnidirectionalState* const state = &dataStream->controllerToAccessory;
                NotifyDelegateOfCompletion(server, dataStream_, kHAPError_InvalidState, state, true);
            }
            dataStream->isHandlingEvent = false;

            // Save arguments for callback invocation.
            HAPDataStreamRequest request;
            HAPDataStreamGetRequestContext(server, dataStream_, &request);

            // Invalidate HomeKit Data Stream.
            ResetDataStreamState(dataStream);

            // Inform delegate.
            HAPAssert(request.accessory->dataStream.delegate.callbacks);
            HAPAssert(request.accessory->dataStream.delegate.callbacks->handleInvalidate);
            request.accessory->dataStream.delegate.callbacks->handleInvalidate(
                    server, &request, dataStream_, server->context);
            break;
        }
    }
}

void HAPDataStreamHAPInvalidateAllForHAPPairingID(
        HAPAccessoryServer* server,
        int pairingID,
        HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    if (dataStream->state == HAPDataStreamHAPState_Uninitialized) {
        return;
    }

    if (dataStream->pairingID != pairingID) {
        return;
    }

    HAPPrecondition(!dataStream->isHandlingEvent);

    HAPDataStreamInvalidate(server, dataStream_);
}

void HAPDataStreamHAPInvalidateAllForHAPSession(
        HAPAccessoryServer* server,
        HAPSession* _Nullable session,
        HAPDataStreamRef* dataStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dataStream_);

    HAPDataStreamHAP* const dataStream = &dataStream_->hap;

    if (!dataStream->session) {
        return;
    }

    if (session != NULL && dataStream->session != session) {
        return;
    }

    HAPPrecondition(!dataStream->isHandlingEvent);

    HAPDataStreamInvalidate(server, dataStream_);
}

void HAPDataStreamHAPPrepareSessionResume(
        HAPAccessoryServer* server,
        uint8_t* resumedSessionSecret,
        HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(resumedSessionSecret);
    HAPPrecondition(session);

    if (server->dataStream.numDataStreams == 0) {
        return;
    }

    if (session->transportType == kHAPTransportType_BLE) {
        HAPAssert(session->state.pairVerify.state == 2);

        for (size_t i = 0; i < server->dataStream.numDataStreams; i++) {
            HAPDataStreamRef* const dataStream_ = &server->dataStream.dataStreams[i];
            HAPDataStreamHAP* const dataStream = &dataStream_->hap;

            if (HAPRawBufferAreEqual(
                        dataStream->sessionSecret, resumedSessionSecret, sizeof(dataStream->sessionSecret))) {
                // Update the shared secret of the data stream for the resumed session.
                HAPRawBufferCopyBytes(
                        dataStream->sessionSecret, session->state.pairVerify.cv_KEY, sizeof(dataStream->sessionSecret));
            }
        }
    } else {
        HAPLogError(&logObject, "Session resume attempted for invalid transport type.");
        HAPFatalError();
    }
}

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif
