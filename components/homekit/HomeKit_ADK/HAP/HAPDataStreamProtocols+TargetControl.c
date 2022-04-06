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
#include "HAPDataStreamProtocols+TargetControl.h"
#include "HAPLogSubsystem.h"
#include "HAPOPACK.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStreamProtocols" };

/**
 * Topic of a targetControl.whoami message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.5 Target Control Identifier
 */
#define kHAPTargetControlDataStreamProtocol_WhoamiTopic "whoami"

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
    HAPTargetControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPTargetControlDataStreamProtocol_Name));
    HAPPrecondition(!messageBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Skipped event.", (const void*) dispatcher, dataStream);
}

static void HandleWhoamiEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPTargetControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPTargetControlDataStreamProtocol_Name));
    const char* topic = kHAPTargetControlDataStreamProtocol_WhoamiTopic;
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    if (!messageBytes) {
        HAPLog(&logObject,
               "[%p.%u] Skipped %s.%s event.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }

    HAPError err;

    // Parse Message.
    HAPOPACKReader messageReader;
    HAPOPACKReaderCreate(&messageReader, messageBytes, numMessageBytes);
    HAPOPACKStringDictionaryElement identifierElement;
    identifierElement.key = "identifier";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &identifierElement, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received malformed %s.%s event.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }

    // Parse identifier.
    if (!identifierElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event does not contain 'identifier' key.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    int64_t rawIdentifier;
    err = HAPOPACKReaderGetNextInt(&identifierElement.value.reader, &rawIdentifier);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with malformed 'identifier'.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    if (rawIdentifier < 0 || rawIdentifier > (HAPTargetControlDataStreamProtocolTargetIdentifier) -1) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with out-of-range 'identifier' (%lld).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic,
               (long long) rawIdentifier);
        return;
    }
    HAPTargetControlDataStreamProtocolTargetIdentifier identifier =
            (HAPTargetControlDataStreamProtocolTargetIdentifier) rawIdentifier;

    // Inform delegate.
    HAPLogInfo(
            &logObject,
            "[%p.%u] Received %s.%s event (identifier: %lu).",
            (const void*) dispatcher,
            dataStream,
            dataStreamProtocol->base->name,
            topic,
            (unsigned long) identifier);
    dataStreamProtocol->callbacks.handleIdentifierUpdate(
            server, dispatcher, dataStreamProtocol, request, dataStream, identifier, context);
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
    HAPPrecondition(dataStreamProtocol_);
    HAPTargetControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPTargetControlDataStreamProtocol_Name));
    HAPPrecondition(topic);

    if (HAPStringAreEqual(topic, kHAPTargetControlDataStreamProtocol_WhoamiTopic)) {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Receiving %s.%s event (short).",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        HAPDataStreamDispatcherReceiveShortEvent(server, dispatcher, dataStream, HandleWhoamiEvent);
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
    HAPTargetControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPTargetControlDataStreamProtocol_Name));
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Rejected request.", (const void*) dispatcher, dataStream);
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
    HAPTargetControlDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPTargetControlDataStreamProtocol_Name));
    HAPPrecondition(topic);

    HAPLog(&logObject,
           "[%p.%u] Rejecting unexpected %s.%s request (topic not supported).",
           (const void*) dispatcher,
           dataStream,
           dataStreamProtocol->base->name,
           topic);
    HAPDataStreamDispatcherSkipAndReplyToRequest(
            server, dispatcher, dataStream, kHAPDataStreamResponseStatus_HeaderError, HandleRejectRequestComplete);
}

const HAPDataStreamProtocolBase kHAPTargetControlDataStreamProtocol_Base = {
    .name = kHAPTargetControlDataStreamProtocol_Name,
    .callbacks = { .handleAccept = NULL,
                   .handleInvalidate = NULL,
                   .handleEventAvailable = HandleEventAvailable,
                   .handleRequestAvailable = HandleRequestAvailable }
};

#endif
