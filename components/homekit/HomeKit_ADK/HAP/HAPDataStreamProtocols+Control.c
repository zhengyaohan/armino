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

#include "HAPDataStream.h"
#include "HAPDataStreamDispatcher.h"
#include "HAPDataStreamProtocols+Control.h"
#include "HAPLogSubsystem.h"
#include "HAPOPACK.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStreamProtocols" };

/**
 * Payload of an empty message.
 */
static const uint8_t kHAPControlDataStream_EmptyMessage[] = { kHAPOPACKTag_Dictionary0 };

static void HandleAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);

    // Initialize state.
    dataStreamDescriptor->controlState = kHAPControlDataStreamProtocolState_WaitingForHelloRequest;
}

static void HandleInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);

    // Inform protocols.
    if (dataStreamDescriptor->controlState == kHAPControlDataStreamProtocolState_Connected) {
        for (size_t i = 0; dispatcher->storage->dataStreamProtocols[i]; i++) {
            HAPBaseDataStreamProtocol* otherDataStreamProtocol = dispatcher->storage->dataStreamProtocols[i];

            HAPLogInfo(
                    &logObject,
                    "[%p.%u] HomeKit Data Stream invalidated. Informing %s protocol.",
                    (const void*) dispatcher,
                    dataStream,
                    otherDataStreamProtocol->base->name);
            if (otherDataStreamProtocol->base->callbacks.handleInvalidate) {
                otherDataStreamProtocol->base->callbacks.handleInvalidate(
                        server, dispatcher, otherDataStreamProtocol, request, dataStream, context);
            }
        }
        HAPLogInfo(
                &logObject,
                "[%p.%u] HomeKit Data Stream invalidation complete. All protocols informed.",
                (const void*) dispatcher,
                dataStream);
    } else {
        HAPLogInfo(
                &logObject,
                "[%p.%u] HomeKit Data Stream invalidated (setup incomplete). Not informing protocols.",
                (const void*) dispatcher,
                dataStream);
    }

    // Reset state.
    dataStreamDescriptor->controlState = 0;
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
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
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
        size_t numMessageBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPPrecondition(topic);

    switch ((HAPControlDataStreamProtocolState) dataStreamDescriptor->controlState) {
        case kHAPControlDataStreamProtocolState_WaitingForHelloRequest: {
            // The first message received must be a valid "hello" request.
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.121 Setup Data Stream Transport
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s event instead of control.hello request.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            HAPDataStreamInvalidate(server, HAPNonnull(dataStreamDescriptor->dataStream));
            return;
        }
        case kHAPControlDataStreamProtocolState_SendingHelloResponse: {
            // While ignoring a request no other events or requests may be received.
            HAPAssertionFailure();
            return;
        }
        case kHAPControlDataStreamProtocolState_Connected: {
            HAPLog(&logObject,
                   "[%p.%u] Skipping unexpected %s.%s event (topic not supported).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            HAPDataStreamDispatcherSkipEvent(server, dispatcher, dataStream, HandleSkipEventComplete);
            return;
        }
        default:
            HAPFatalError();
    }
}

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
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Rejected request.", (const void*) dispatcher, dataStream);
}

static void HandleHelloRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    // Advance state.
    HAPAssert(dataStreamDescriptor->controlState == kHAPControlDataStreamProtocolState_SendingHelloResponse);
    dataStreamDescriptor->controlState = kHAPControlDataStreamProtocolState_Connected;

    // Inform protocols.
    for (size_t i = 0; dispatcher->storage->dataStreamProtocols[i]; i++) {
        HAPBaseDataStreamProtocol* otherDataStreamProtocol = dispatcher->storage->dataStreamProtocols[i];

        HAPLogInfo(
                &logObject,
                "[%p.%u] HomeKit Data Stream [%p] accepted. Informing %s protocol.",
                (const void*) dispatcher,
                dataStream,
                (const void*) HAPNonnull(dataStreamDescriptor->dataStream),
                otherDataStreamProtocol->base->name);
        if (otherDataStreamProtocol->base->callbacks.handleAccept) {
            otherDataStreamProtocol->base->callbacks.handleAccept(
                    server, dispatcher, otherDataStreamProtocol, request, dataStream, context);
        }
    }
    HAPLogInfo(
            &logObject,
            "[%p.%u] HomeKit Data Stream setup complete. All protocols may now exchange messages.",
            (const void*) dispatcher,
            dataStream);
}

static void HandleInvalidateReceiveRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction HAP_UNUSED,
        void* _Nullable context) {
    HAPLogDebug(&logObject, "%s", __func__);
    HandleInvalidate(server, dispatcher, dataStreamProtocol, request, dataStream, context);
}

static const HAPDataStreamReceiveRequestTransactionCallbacks kHAPControlDataStreamReceiveRequestTransactionCallbacks = {
    .handleInvalidate = HandleInvalidateReceiveRequest
};

static void HandleInvalidateSendRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction HAP_UNUSED,
        void* _Nullable context) {
    HAPLogDebug(&logObject, "%s", __func__);
    HandleInvalidate(server, dispatcher, dataStreamProtocol, request, dataStream, context);
}

static void HandleControlSkipHelloResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes HAP_UNUSED,
        size_t numMessageBytes HAP_UNUSED,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPPrecondition(transaction);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete == true);
    return;
}

static void HandleSendResponseAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        const char* topic,
        HAPDataStreamResponseStatus status,
        size_t numMessageBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPPrecondition(transaction);
    HAPAssert(HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_HelloTopic) == true);

    HAPLogInfo(&logObject, "Received hello ping response with status %u", (unsigned int) status);
    HAPDataStreamDispatcherSkipResponse(server, dispatcher, dataStream, transaction, HandleControlSkipHelloResponse);
    return;
}

static const HAPDataStreamSendRequestTransactionCallbacks kHAPControlDataStreamSendRequestTransactionCallbacks = {
    .handleInvalidate = HandleInvalidateSendRequest,
    .handleResponseAvailable = HandleSendResponseAvailable
};

static void HandleControlHdsVersionMessageSendComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes HAP_UNUSED,
        size_t numMessageBytes HAP_UNUSED,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPPrecondition(transaction);
    HAPAssert(isComplete == true);
    return;
}

static void HandleVersionRequestComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPError err;
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(transaction);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete == false);

    HAPOPACKReader payload;
    HAPOPACKReaderCreate(&payload, messageBytes, numMessageBytes);
    HAPOPACKStringDictionaryElement versionElement;
    versionElement.key = "version";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &payload, (HAPOPACKStringDictionaryElement* const[]) { &versionElement, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLogError(&logObject, "[%p.%u] Received malformed payload.", (const void*) dispatcher, dataStream);
        HAPDataStreamDispatcherSkipAndReplyToRequest(
                server, dispatcher, dataStream, kHAPDataStreamResponseStatus_PayloadError, HandleRejectRequestComplete);
    }
    // Parse version.
    if (!versionElement.value.exists) {
        HAPLogError(
                &logObject,
                "[%p.%u] Received payload does not contain 'version' key.",
                (const void*) dispatcher,
                dataStream);
        HAPDataStreamDispatcherSkipAndReplyToRequest(
                server, dispatcher, dataStream, kHAPDataStreamResponseStatus_PayloadError, HandleRejectRequestComplete);
        return;
    }

    err = HAPOPACKReaderGetNextFloat(&versionElement.value.reader, &(dataStreamDescriptor->peerHdsVersion));
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject, "[%p.%u] Received payload with malformed 'version'.", (const void*) dispatcher, dataStream);
        HAPDataStreamDispatcherSkipAndReplyToRequest(
                server, dispatcher, dataStream, kHAPDataStreamResponseStatus_PayloadError, HandleRejectRequestComplete);
        return;
    }
    HAPLogInfo(&logObject, "Peer HDS version: %g", dataStreamDescriptor->peerHdsVersion);

    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(
            &messageWriter,
            dataStreamDescriptor->controlContext.messageBytes,
            sizeof dataStreamDescriptor->controlContext.messageBytes);
    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "version");
    }
    if (!err) {
        err = HAPOPACKWriterAppendFloat(&messageWriter, kHAPControlDataStreamProtocol_AccessoryHdsVersion);
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "[%p.%u] Outgoing buffer too small.", (const void*) dispatcher, dataStream);
        HAPFatalError();
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);

    HAPDataStreamDispatcherSendMutableResponse(
            server,
            dispatcher,
            dataStream,
            transaction,
            kHAPDataStreamResponseStatus_Success,
            bytes,
            numBytes,
            HandleControlHdsVersionMessageSendComplete);
    return;
}

static void HandleHelloPingSendRequestComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes HAP_UNUSED,
        size_t numMessageBytes HAP_UNUSED,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(transaction);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete == false);
}

void HAPControlDataStreamProtocolSendHelloPing(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream) {
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPBaseDataStreamProtocol* _Nullable controlDataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPControlDataStreamProtocol_Name);
    HAPAssert(controlDataStreamProtocol);

    HAPAssert(dataStreamDescriptor->peerHdsVersion >= kHAPControlDataStreamProtocol_MinPeerHdsVersionForPing);
    if (dataStreamDescriptor->peerHdsVersion < kHAPControlDataStreamProtocol_MinPeerHdsVersionForPing) {
        HAPLog(&logObject,
               "Attempting to send hello ping to peer with HDS version %g",
               dataStreamDescriptor->peerHdsVersion);
    }

    HAPDataStreamDispatcherSendRequest(
            server,
            dispatcher,
            controlDataStreamProtocol,
            dataStream,
            &(dataStreamDescriptor->controlContext.sndReqTransaction),
            &kHAPControlDataStreamSendRequestTransactionCallbacks,
            kHAPControlDataStreamProtocol_HelloTopic,
            kHAPControlDataStream_EmptyMessage,
            sizeof kHAPControlDataStream_EmptyMessage,
            HandleHelloPingSendRequestComplete);
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
    HAPLogDebug(&logObject, "%s", __func__);
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataStreamDescriptor* dataStreamDescriptor = &dispatcher->storage->dataStreamDescriptors[dataStream];
    HAPPrecondition(dataStreamProtocol_);
    HAPControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name));
    HAPPrecondition(topic);
    HAPPrecondition(dataStream < dispatcher->storage->numDataStreams);

    switch ((HAPControlDataStreamProtocolState) dataStreamDescriptor->controlState) {
        case kHAPControlDataStreamProtocolState_WaitingForHelloRequest: {
            // The first message received must be a valid "hello" request.
            // See HomeKit Accessory Protocol Specification R17
            // Section 11.121 Setup Data Stream Transport
            if (!HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_HelloTopic)) {
                HAPLog(&logObject,
                       "[%p.%u] Received %s.%s request instead of control.hello request.",
                       (const void*) dispatcher,
                       dataStream,
                       dataStreamProtocol->base->name,
                       topic);
                HAPDataStreamInvalidate(server, HAPNonnull(dataStreamDescriptor->dataStream));
                return;
            }

            HAPLogInfo(
                    &logObject,
                    "[%p.%u] Received %s.%s request.",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);

            // Set the defalt peer HDS version in case the peer doesn't provide
            // its version information
            dataStreamDescriptor->peerHdsVersion = kHAPControlDataStreamProtocol_DefaultPeerHdsVersion;

            HAPLogInfo(
                    &logObject,
                    "[%p.%u] Ignoring request body and sending %s.%s response.",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);
            dataStreamDescriptor->controlState = kHAPControlDataStreamProtocolState_SendingHelloResponse;
            HAPDataStreamDispatcherSkipAndReplyToRequest(
                    server, dispatcher, dataStream, kHAPDataStreamResponseStatus_Success, HandleHelloRequest);
            return;
        }
        case kHAPControlDataStreamProtocolState_SendingHelloResponse: {
            // While ignoring a request no other events or requests may be received.
            HAPAssertionFailure();
            return;
        }
        case kHAPControlDataStreamProtocolState_Connected: {
            HAPLogInfo(
                    &logObject,
                    "[%p.%u] Received %s.%s request.",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);
            if ((HAPStringAreEqual(dataStreamProtocol->base->name, kHAPControlDataStreamProtocol_Name) == true) &&
                (HAPStringAreEqual(topic, kHAPControlDataStreamProtocol_VersionTopic) == true)) {

                HAPDataStreamDispatcherReceiveRequest(
                        server,
                        dispatcher,
                        dataStream,
                        &(dataStreamDescriptor->controlContext.rcvReqTransaction),
                        &kHAPControlDataStreamReceiveRequestTransactionCallbacks,
                        kHAPControlDataStreamProtocol_VersionTopic,
                        dataStreamDescriptor->controlContext.messageBytes,
                        numMessageBytes,
                        HandleVersionRequestComplete);
                return;
            }

            HAPLog(&logObject,
                   "[%p.%u] Rejecting unexpected %s.%s request (topic not supported).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            HAPDataStreamDispatcherSkipAndReplyToRequest(
                    server,
                    dispatcher,
                    dataStream,
                    kHAPDataStreamResponseStatus_HeaderError,
                    HandleRejectRequestComplete);
            return;
        }
        default:
            HAPFatalError();
    }
}

const HAPDataStreamProtocolBase kHAPControlDataStreamProtocol_Base = {
    .name = kHAPControlDataStreamProtocol_Name,
    .callbacks = { .handleAccept = HandleAccept,
                   .handleInvalidate = HandleInvalidate,
                   .handleEventAvailable = HandleEventAvailable,
                   .handleRequestAvailable = HandleRequestAvailable }
};

#endif
