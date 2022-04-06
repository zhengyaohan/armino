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

#include "HAPLogSubsystem.h"

#include "HAPDataStream.h"
#include "HAPDataStreamDispatcher.h"
#include "HAPOPACK.h"
#include "HAPSession.h"

#include "HAPDataStreamProtocols+Stream.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStreamProtocols" };

/**
 * "data" key for "stream" event messages.
 */
#define kHAPStreamDataStreamProtocol_DataKey "data"

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPStreamApplicationProtocol* GetStreamApplicationProtocolForProtocolContext(
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext) {
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(streamAppProtocolContext);
    HAPPrecondition(dataStreamProtocol->applicationProtocols);

    HAPStreamApplicationProtocol* streamAppProtocol;

    for (size_t i = 0; dataStreamProtocol->applicationProtocols[i]; i++) {
        streamAppProtocol = dataStreamProtocol->applicationProtocols[i];
        if (&streamAppProtocol->storage.streamAppProtocolContexts[dataStream] == streamAppProtocolContext) {
            return streamAppProtocol;
        }
    }
    HAPLogError(
            &logObject,
            "[%p.%u] No stream application protocol found for context.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
static HAPStreamDataStreamApplicationProtocolContext* GetStreamApplicationProtocolContextForSendEventTransaction(
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction) {
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(transaction);
    HAPPrecondition(dataStreamProtocol->applicationProtocols);

    HAPStreamApplicationProtocol* streamAppProtocol;
    HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext;

    for (size_t i = 0; dataStreamProtocol->applicationProtocols[i]; i++) {
        streamAppProtocol = dataStreamProtocol->applicationProtocols[i];
        streamAppProtocolContext = &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
        HAPStreamDataStreamApplicationProtocolSendContext* sendContext = streamAppProtocolContext->firstSendContext;
        while (sendContext != NULL) {
            if (&sendContext->transaction == transaction) {
                return streamAppProtocolContext;
            }
            sendContext = sendContext->nextSendContext;
        }
    }
    HAPLogError(
            &logObject,
            "[%p.%u] No stream application protocol context found for send event transaction.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

static void RequestAppProtocolBuffer(
        HAPStreamApplicationProtocol* streamAppProtocol,
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(streamAppProtocolContext);

    if (streamAppProtocolContext->receive.buffer) {
        // Buffer requested while already holding a buffer. This shouldn't happen!
        HAPLogError(&logObject, "Invalid event buffer request.");
        HAPFatalError();
    }

    // Find the first available buffer.
    size_t bufferIdx;
    uint8_t* eventBuffer;
    for (bufferIdx = 0, eventBuffer = streamAppProtocol->storage.rxBuffers;
         bufferIdx < streamAppProtocol->storage.numRxBuffers;
         bufferIdx++, eventBuffer += streamAppProtocol->storage.rxBufferSize) {
        if (!(streamAppProtocol->storage._.rxBufferIsAllocated & (1 << bufferIdx))) {
            // Found one. Mark it as allocated and link to the app protocol context.
            streamAppProtocol->storage._.rxBufferIsAllocated |= (1 << bufferIdx);
            streamAppProtocolContext->receive.buffer = eventBuffer;
            break;
        }
    }
}

static void ReturnAppProtocolBuffer(
        HAPStreamApplicationProtocol* streamAppProtocol,
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(streamAppProtocolContext);

    if (streamAppProtocolContext->receive.buffer == NULL) {
        // Return attempted without holding a buffer. This shouldn't happen!
        HAPLogError(&logObject, "Invalid event buffer return.");
        HAPFatalError();
    }

    // Find the buffer being returned.
    int bufferIdx = 0;
    uint8_t* eventBuffer = streamAppProtocol->storage.rxBuffers;
    HAPStreamApplicationProtocolRxBufferTracker allocatedBuffers = streamAppProtocol->storage._.rxBufferIsAllocated;
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && (eventBuffer == streamAppProtocolContext->receive.buffer)) {
            // Found it. Mark it as free and remove link from app protocol context.
            streamAppProtocol->storage._.rxBufferIsAllocated &= ~(1 << bufferIdx);
            streamAppProtocolContext->receive.buffer = NULL;
            return;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        eventBuffer += streamAppProtocol->storage.rxBufferSize;
    }
    HAPLogError(&logObject, "Unable to return event buffer (%p)", streamAppProtocolContext->receive.buffer);
    HAPAssert(false);
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(dataStreamProtocol->applicationProtocols);

    // Check whether the HAP session opening this data stream corresponds to an admin controller.
    // This check requires accessing the KVS so the admin setting is cached on data stream open.
    bool controllerIsAdmin = HAPSessionControllerIsAdmin(request->session);

    for (size_t i = 0; dataStreamProtocol->applicationProtocols[i]; i++) {
        // Initialize stream application protocol context
        HAPStreamApplicationProtocol* streamAppProtocol = dataStreamProtocol->applicationProtocols[i];
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext =
                &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
        HAPRawBufferZero(streamAppProtocolContext, sizeof *streamAppProtocolContext);
        streamAppProtocolContext->dataStreamContext.isAdmin = controllerIsAdmin;

        // Inform stream application protocol
        if ((streamAppProtocol->config.requiresAdminPermissions == true) && (controllerIsAdmin == false)) {
            HAPLogInfo(
                    &logObject,
                    "%s: Ignoring DataStream (%u) accept from non-admin controller for %s stream protocol.",
                    __func__,
                    dataStream,
                    streamAppProtocol->name);
        } else if (streamAppProtocol->callbacks.handleAccept) {
            HAPLogInfo(
                    &logObject,
                    "%s: Informing %s stream application protocol of DataStream (%u) accept.",
                    __func__,
                    streamAppProtocol->name,
                    dataStream);

            streamAppProtocol->callbacks.handleAccept(
                    server, dispatcher, streamAppProtocol, request, dataStream, context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(dataStreamProtocol->applicationProtocols);

    for (size_t i = 0; dataStreamProtocol->applicationProtocols[i]; i++) {
        // Invalidate stream application protocol context
        HAPStreamApplicationProtocol* streamAppProtocol = dataStreamProtocol->applicationProtocols[i];
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext =
                &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
        bool requiresAdminPermissions = streamAppProtocol->config.requiresAdminPermissions;
        bool controllerIsAdmin = streamAppProtocolContext->dataStreamContext.isAdmin;
        if (streamAppProtocolContext->receive.buffer) {
            ReturnAppProtocolBuffer(streamAppProtocol, streamAppProtocolContext);
        }
        HAPRawBufferZero(streamAppProtocolContext, sizeof *streamAppProtocolContext);

        // Inform stream application protocol
        if ((requiresAdminPermissions == true) && (controllerIsAdmin == false)) {
            HAPLogInfo(
                    &logObject,
                    "%s: Ignoring DataStream (%u) invalidate from non-admin controller for %s stream protocol.",
                    __func__,
                    dataStream,
                    streamAppProtocol->name);
        } else if (streamAppProtocol->callbacks.handleInvalidate) {
            HAPLogInfo(
                    &logObject,
                    "%s: Informing %s stream application protocol of DataStream (%u) invalidate.",
                    __func__,
                    streamAppProtocol->name,
                    dataStream);

            streamAppProtocol->callbacks.handleInvalidate(
                    server, dispatcher, streamAppProtocol, request, dataStream, context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void CleanupHandleEvent(
        HAPStreamApplicationProtocol* streamAppProtocol,
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(streamAppProtocolContext);

    if (streamAppProtocolContext->receive.buffer) {
        ReturnAppProtocolBuffer(streamAppProtocol, streamAppProtocolContext);
    }
    streamAppProtocolContext->receive.eventAvail = false;
}

static void HandleEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);

    HAPStreamApplicationProtocol* streamAppProtocol;
    HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext;

    for (size_t i = 0; (streamAppProtocol = dataStreamProtocol->applicationProtocols[i]); i++) {
        streamAppProtocolContext = &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
        if (streamAppProtocolContext->receive.eventAvail) {
            break;
        }
    }

    if (!streamAppProtocol) {
        HAPLogError(
                &logObject,
                "[%p.%u] No stream application protocol context found.",
                (const void*) dispatcher,
                dataStream);
        HAPFatalError();
    }

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
        return;
    }
    if (!messageBytes) {
        HAPLog(&logObject,
               "[%p.%u] Skipped %s event.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name);
        CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
        return;
    }

    const char* topic = streamAppProtocol->name;

    // Parse message.
    HAPError err;
    HAPOPACKReader messageReader;
    HAPOPACKReaderCreate(&messageReader, messageBytes, numMessageBytes);

    HAPOPACKStringDictionaryElement dataElement;
    dataElement.key = kHAPStreamDataStreamProtocol_DataKey;

    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &dataElement, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLogError(
                &logObject,
                "[%p.%u] Received malformed %s.%s event.",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
        return;
    }

    // Parse data.
    if (!dataElement.value.exists) {
        HAPLogError(
                &logObject,
                "[%p.%u] Received %s.%s event does not contain 'data' key.",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
        return;
    }
    void* dataBytes;
    size_t numDataBytes;
    err = HAPOPACKReaderGetNextData(&dataElement.value.reader, &dataBytes, &numDataBytes);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLogError(
                &logObject,
                "[%p.%u] Received %s.%s event with malformed 'data'.",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
        return;
    }

    if (streamAppProtocol->callbacks.handleData) {
        streamAppProtocol->callbacks.handleData(
                server, dispatcher, streamAppProtocol, request, dataStream, context, dataBytes, numDataBytes);
    }
    CleanupHandleEvent(streamAppProtocol, streamAppProtocolContext);
}

static void HandleSkipEventComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(!messageBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Skipped event.", (const void*) dispatcher, dataStream);
}

static void HandleEventAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        const char* topic,
        size_t numMessageBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(topic);
    HAPPrecondition(dataStreamProtocol->applicationProtocols);

    HAPStreamApplicationProtocol* streamAppProtocol;
    for (size_t i = 0; (streamAppProtocol = dataStreamProtocol->applicationProtocols[i]); i++) {
        HAPAssert(streamAppProtocol->name);
        if (HAPStringAreEqual(topic, streamAppProtocol->name)) {
            break;
        }
    }

    if (streamAppProtocol) {
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext =
                &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
        HAPDataStreamDescriptor* dataStreamDescriptor HAP_UNUSED =
                &dispatcher->storage->dataStreamDescriptors[dataStream];

        if ((streamAppProtocol->config.requiresAdminPermissions == true) &&
            (streamAppProtocolContext->dataStreamContext.isAdmin == false)) {
            HAPLogInfo(
                    &logObject,
                    "[%p.%u] Skipping %s.%s event message from non-admin controller.",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);
            HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, HandleSkipEventComplete);
        } else if (numMessageBytes <= sizeof(dataStreamDescriptor->_.event.messageBytes)) {
            // Default to receive event into the DataStream short buffer.
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] Receiving %s.%s event (short).",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);
            streamAppProtocolContext->receive.eventAvail = true;
            HAPDataStreamDispatcherReceiveShortEvent(server, dispatcher, dataStream, HandleEvent);
        } else if (
                (streamAppProtocol->storage.rxBuffers == NULL) ||
                (streamAppProtocol->storage.rxBufferSize < numMessageBytes)) {
            // Message cannot be received into application protocol buffer.
            HAPLogError(
                    &logObject,
                    "[%p.%u] Skipping %s.%s event message (too long - %zu bytes).",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic,
                    numMessageBytes);
            HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, HandleSkipEventComplete);
        } else {
            // Allocate application protocol buffer.
            RequestAppProtocolBuffer(streamAppProtocol, streamAppProtocolContext);
            if (streamAppProtocolContext->receive.buffer) {
                HAPLogDebug(
                        &logObject,
                        "[%p.%u] Receiving %s.%s event.",
                        (const void*) dispatcher,
                        dataStream,
                        dataStreamProtocol->base->name,
                        topic);
                streamAppProtocolContext->receive.eventAvail = true;
                HAPDataStreamDispatcherReceiveEvent(
                        server,
                        dispatcher,
                        dataStream,
                        streamAppProtocolContext->receive.buffer,
                        numMessageBytes,
                        HandleEvent);
            } else {
                HAPLogError(
                        &logObject,
                        "[%p.%u] Skipping %s.%s event message (no buffer).",
                        (const void*) dispatcher,
                        dataStream,
                        dataStreamProtocol->base->name,
                        topic);
                HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, HandleSkipEventComplete);
            }
        }
    } else {
        HAPLog(&logObject,
               "[%p.%u] Skipping unexpected %s.%s event (topic not supported).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, HandleSkipEventComplete);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleRejectRequestComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
}

static void HandleRequestAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        const char* topic,
        size_t numMessageBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(topic);

    // Stream protocol does not support request messages
    HAPLog(&logObject,
           "[%p.%u] Rejecting unexpected %s.%s request.",
           (const void*) dispatcher,
           dataStream,
           dataStreamProtocol->base->name,
           topic);
    HAPDataStreamDispatcherSkipAndReplyToRequest(
            server, dispatcher, dataStream, kHAPDataStreamResponseStatus_HeaderError, HandleRejectRequestComplete);
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleSendDataEventComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPStreamDataStreamProtocol* dataStreamProtocol = (HAPStreamDataStreamProtocol*) dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPStreamDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(transaction);
    HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext =
            GetStreamApplicationProtocolContextForSendEventTransaction(
                    dispatcher, dataStreamProtocol, dataStream, transaction);
    HAPStreamApplicationProtocol* streamAppProtocol = GetStreamApplicationProtocolForProtocolContext(
            dispatcher, dataStreamProtocol, dataStream, streamAppProtocolContext);
    HAPPrecondition(streamAppProtocol->name);
    const char* topic = streamAppProtocol->name;

    if (error) {
        HAPLogError(
                &logObject,
                "[%p.%u] %s.%s event not sent (%d).",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic,
                error);
        HAPAssert(error == kHAPError_InvalidState);
    } else {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Sent %s.%s event.",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
    }

    // Find the completed send context.
    HAPStreamDataStreamApplicationProtocolSendContext* sendContext = streamAppProtocolContext->firstSendContext;
    while ((sendContext != NULL) && (&sendContext->transaction != transaction)) {
        sendContext = sendContext->nextSendContext;
    }

    if (sendContext == NULL) {
        HAPLogError(
                &logObject,
                "[%p.%u] No stream application protocol send context found for send event transaction.",
                (const void*) dispatcher,
                dataStream);
        HAPFatalError();
    }

    // Save off send context needed to invoke the delegate handler.
    HAPAssert(messageBytes == sendContext->scratchBytes);
    HAPAssert(numMessageBytes <= sendContext->numScratchBytes);
    HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler completionHandler = sendContext->completionHandler;
    void* scratchBytes = sendContext->scratchBytes;
    size_t numScratchBytes = sendContext->numScratchBytes;

    // Remove the send context from the stream's list.
    sendContext->inUse = false;
    if (streamAppProtocolContext->firstSendContext == sendContext) {
        streamAppProtocolContext->firstSendContext = sendContext->nextSendContext;
    } else {
        HAPStreamDataStreamApplicationProtocolSendContext* prevSendContext = streamAppProtocolContext->firstSendContext;
        while (prevSendContext) {
            if (prevSendContext->nextSendContext == sendContext) {
                prevSendContext->nextSendContext = sendContext->nextSendContext;
                break;
            }
            prevSendContext = prevSendContext->nextSendContext;
        }
    }
    sendContext->nextSendContext = NULL;
    sendContext->inUse = false;
    HAPLogDebug(&logObject, "%p removed send context %p.", (void*) streamAppProtocolContext, (void*) sendContext);

    // Inform delegate.
    completionHandler(
            server, dispatcher, streamAppProtocol, request, dataStream, error, scratchBytes, numScratchBytes, context);
}

HAP_RESULT_USE_CHECK
HAPError HAPStreamDataStreamProtocolSendData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        HAPDataStreamHandle dataStream,
        void* scratchBytes,
        size_t numScratchBytes,
        void* dataBytes,
        size_t numDataBytes,
        HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(streamAppProtocol);
    HAPStreamDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPStreamDataStreamProtocol_Name);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->numDataStreams);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(dataBytes);
    HAPPrecondition(completionHandler);
    HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContext =
            &streamAppProtocol->storage.streamAppProtocolContexts[dataStream];
    HAPError err;

    // Check the number of outstanding sends for this stream. If it has reached the max allowed, return busy to the
    // caller.
    size_t numSendContexts = 0;
    HAPStreamDataStreamApplicationProtocolSendContext* sendContext = streamAppProtocolContext->firstSendContext;
    while (sendContext != NULL) {
        numSendContexts++;
        sendContext = sendContext->nextSendContext;
    }
    if (numSendContexts >= streamAppProtocol->config.maxSendContextsPerDataStream) {
        HAPLogError(
                &logObject,
                "[%p.%u] [%s] %s: Max stream application protocol send contexts (%zu) already in use.",
                (const void*) dispatcher,
                dataStream,
                streamAppProtocol->name,
                __func__,
                streamAppProtocol->config.maxSendContextsPerDataStream);
        return kHAPError_Busy;
    }

    // Serialize message into scratch buffer.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, scratchBytes, numScratchBytes);

    err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, kHAPStreamDataStreamProtocol_DataKey);
    }
    if (!err) {
        err = HAPOPACKWriterAppendData(&messageWriter, dataBytes, numDataBytes);
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%s] %s: Scratch buffer too small.",
                (const void*) dispatcher,
                dataStream,
                streamAppProtocol->name,
                __func__);
        return err;
    }

    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);

    // Find a free send context to use.
    sendContext = NULL;
    for (size_t i = 0; i < streamAppProtocol->storage.numSendContexts; i++) {
        if (!streamAppProtocol->storage.streamAppProtocolSendContexts[i].inUse) {
            sendContext = &streamAppProtocol->storage.streamAppProtocolSendContexts[i];
            break;
        }
    }

    if (sendContext == NULL) {
        HAPLogError(
                &logObject,
                "[%p.%u] [%s] %s: No free stream application protocol send contexts.",
                (const void*) dispatcher,
                dataStream,
                streamAppProtocol->name,
                __func__);
        return kHAPError_OutOfResources;
    } else {
        // Insert the send context into the stream's list.
        if (streamAppProtocolContext->firstSendContext == NULL) {
            streamAppProtocolContext->firstSendContext = sendContext;
        } else {
            HAPStreamDataStreamApplicationProtocolSendContext* lastContext = streamAppProtocolContext->firstSendContext;
            while (lastContext->nextSendContext != NULL) {
                lastContext = lastContext->nextSendContext;
            }
            lastContext->nextSendContext = sendContext;
        }
        sendContext->inUse = true;
        HAPLogDebug(&logObject, "%p inserted send context %p.", (void*) streamAppProtocolContext, (void*) sendContext);
    }

    HAPLogDebug(
            &logObject,
            "[%p.%u] Sending %s.%s event (%zu bytes).",
            (const void*) dispatcher,
            dataStream,
            dataStreamProtocol->base->name,
            streamAppProtocol->name,
            numBytes);

    sendContext->completionHandler = completionHandler;
    sendContext->scratchBytes = scratchBytes;
    sendContext->numScratchBytes = numScratchBytes;
    HAPDataStreamDispatcherSendMutableEvent(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            &sendContext->transaction,
            streamAppProtocol->name,
            bytes,
            numBytes,
            HandleSendDataEventComplete);

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

const HAPDataStreamProtocolBase kHAPStreamDataStreamProtocol_Base = {
    .name = kHAPStreamDataStreamProtocol_Name,
    .callbacks = { .handleAccept = HandleAccept,
                   .handleInvalidate = HandleInvalidate,
                   .handleEventAvailable = HandleEventAvailable,
                   .handleRequestAvailable = HandleRequestAvailable }
};

#endif
