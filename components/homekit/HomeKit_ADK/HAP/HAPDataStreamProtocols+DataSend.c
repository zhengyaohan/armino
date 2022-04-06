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
#include "HAPDataStreamProtocols+DataSend.h"
#include "HAPLogSubsystem.h"
#include "HAPOPACK.h"
#include "HAPSession.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "DataStreamProtocols" };

/**
 * Topic of a dataSend.open message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.1 Start
 */
#define kHAPDataSendDataStreamProtocol_OpenTopic "open"

/**
 * Timeout of a dataSend.open request.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.1 Start
 */
#define kHAPDataSendDataStreamProtocol_OpenTimeout ((HAPTime)(5 * HAPSecond))

/**
 * Topic of a dataSend.data message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.2 Binary Data
 */
#define kHAPDataSendDataStreamProtocol_DataTopic "data"

/**
 * Topic of a dataSend.ack message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.3 Acknowledgement
 */
#define kHAPDataSendDataStreamProtocol_AckTopic "ack"

/**
 * Topic of a dataSend.close message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.4 Close
 */
#define kHAPDataSendDataStreamProtocol_CloseTopic "close"

/**
 * Description of "dataSend" stream types.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.1 Start
 */
/**@{*/
/** Siri. */
#define kHAPDataSendDataStreamProtocolTypeDescription_Audio_Siri "audio.siri"

/** IP Camera recording. */
#define kHAPDataSendDataStreamProtocolTypeDescription_IPCamera_Recording "ipcamera.recording"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
/** Diagnostics snapshot. */
#define kHAPDataSendDataStreamProtocolTypeDescription_Diagnostics_Snapshot "diagnostics.snapshot"
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
/** Accessory metrics. */
#define kHAPDataSendDataStreamProtocolTypeDescription_Accessory_Metrics "accessory.metrics"
#endif
/**@}*/

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks whether a value represents a valid "dataSend" stream type.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataSendDataStreamProtocolTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPDataSendDataStreamProtocolType));
    switch ((HAPDataSendDataStreamProtocolType) value) {
        case kHAPDataSendDataStreamProtocolType_Audio_Siri:
        case kHAPDataSendDataStreamProtocolType_IPCamera_Recording:
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        case kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot:
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
        case kHAPDataSendDataStreamProtocolType_Accessory_Metrics:
#endif
        {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether a value represents a valid "dataSend" reject reason.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataSendDataStreamProtocolRejectReasonIsValid(int64_t value) {
    if ((uint64_t) value > (HAPDataSendDataStreamProtocolRejectReason) -1) {
        return false;
    }
    switch ((HAPDataSendDataStreamProtocolRejectReason) value) {
        case kHAPDataSendDataStreamProtocolRejectReason_NotAllowed:
        case kHAPDataSendDataStreamProtocolRejectReason_Busy:
        case kHAPDataSendDataStreamProtocolRejectReason_Unsupported:
        case kHAPDataSendDataStreamProtocolRejectReason_UnexpectedFailure:
        case kHAPDataSendDataStreamProtocolRejectReason_InvalidConfiguration: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether a value represents a valid "dataSend" close reason.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataSendDataStreamProtocolCloseReasonIsValid(int64_t value) {
    if ((uint64_t) value > (HAPDataSendDataStreamProtocolCloseReason) -1) {
        return false;
    }
    switch ((HAPDataSendDataStreamProtocolCloseReason) value) {
        case kHAPDataSendDataStreamProtocolCloseReason_Normal:
        case kHAPDataSendDataStreamProtocolCloseReason_NotAllowed:
        case kHAPDataSendDataStreamProtocolCloseReason_Busy:
        case kHAPDataSendDataStreamProtocolCloseReason_Canceled:
        case kHAPDataSendDataStreamProtocolCloseReason_Unsupported:
        case kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure:
        case kHAPDataSendDataStreamProtocolCloseReason_Timeout:
        case kHAPDataSendDataStreamProtocolCloseReason_BadData:
        case kHAPDataSendDataStreamProtocolCloseReason_ProtocolError:
        case kHAPDataSendDataStreamProtocolCloseReason_InvalidConfiguration: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Checks whether a value represents a valid IP Camera recording data type.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataSendDataStreamProtocolIPCameraRecordingDataTypeIsValid(int64_t value) {
    if ((uint64_t) value > (HAPDataSendDataStreamProtocolIPCameraRecordingDataType) -1) {
        return false;
    }
    switch ((HAPDataSendDataStreamProtocolIPCameraRecordingDataType) value) {
        case kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization:
        case kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment: {
            return true;
        }
        default:
            return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the protocol context for a HomeKit Data Stream send request transaction context.
 *
 * @param      server               Accessory server,
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol_  HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream send request transaction context.
 *
 * @return "dataSend" stream.
 */
HAP_RESULT_USE_CHECK
static HAPDataSendDataStreamProtocolStream* GetDataSendStreamForSendRequestTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];
    HAPPrecondition(transaction);

    HAPAssert(protocolContext->firstDataSendStream);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = protocolContext->firstDataSendStream;
    while (dataSendStream_) {
        HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
        if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening &&
            &dataSendStream->_.open.transaction == transaction) {
            return dataSendStream_;
        }
        dataSendStream_ = dataSendStream->nextDataSendStream;
    }
    HAPLogError(
            &kHAPLog_Default,
            "[%p.%u] No dataSend stream found for send request transaction.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

/**
 * Returns the protocol context for a HomeKit Data Stream send event transaction context.
 *
 * @param      server               Accessory server,
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol_  HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream send event transaction context.
 *
 * @return "dataSend" stream.
 */
HAP_RESULT_USE_CHECK
static HAPDataSendDataStreamProtocolStream* GetDataSendStreamForSendEventTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];
    HAPPrecondition(transaction);

    HAPAssert(protocolContext->firstDataSendStream);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = protocolContext->firstDataSendStream;
    while (dataSendStream_) {
        HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
        if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_SendingData &&
            &dataSendStream->_.sendData.transaction == transaction) {
            return dataSendStream_;
        }
        if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Closing &&
            &dataSendStream->_.close.transaction == transaction) {
            return dataSendStream_;
        }
        dataSendStream_ = dataSendStream->nextDataSendStream;
    }
    HAPLogError(
            &kHAPLog_Default,
            "[%p.%u] No dataSend stream found for send event transaction.",
            (const void*) dispatcher,
            dataStream);
    HAPFatalError();
}

/**
 * Returns the protocol context for a "dataSend" stream identifier.
 *
 * @param      dataStreamProtocol_  HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param      streamID             "dataSend" stream identifier.
 *
 * @return "dataSend" stream if found, NULL otherwise.
 */
HAP_RESULT_USE_CHECK
static HAPDataSendDataStreamProtocolStream* _Nullable GetDataSendStreamForStreamID(
        HAPDataStreamProtocol* dataStreamProtocol_,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStreamID streamID) {
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];

    HAPDataSendDataStreamProtocolStream* dataSendStream_ = protocolContext->firstDataSendStream;
    while (dataSendStream_) {
        HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
        if ((dataSendStream->isOpen || dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Accepting) &&
            dataSendStream->streamID == streamID) {
            if (dataSendStream->wasClosed) {
                return NULL;
            }
            return dataSendStream_;
        }
        dataSendStream_ = dataSendStream->nextDataSendStream;
    }
    return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Closes a "dataSend" stream.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol_  HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataSendStream_      "dataSend" stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 *                                  kHAPError_InvalidData    If an unexpected Message has been received.
 * @param      closeReason          Reason why "dataSend" stream was closed.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
static void
        Close(HAPAccessoryServer* server,
              HAPDataStreamDispatcher* dispatcher,
              HAPDataStreamProtocol* dataStreamProtocol_,
              const HAPDataStreamRequest* request,
              HAPDataStreamHandle dataStream,
              HAPDataSendDataStreamProtocolStream* dataSendStream_,
              HAPError error,
              HAPDataSendDataStreamProtocolCloseReason closeReason,
              void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;

    if (closeReason == kHAPDataSendDataStreamProtocolCloseReason_Normal &&
        (dataSendStream->wasCanceled || dataSendStream->hasTimedOut || !dataSendStream->endOfStream ||
         !dataSendStream->ackReceived)) {
        HAPAssert(dataSendStream->isOpen);
        HAPLogInfo(
                &logObject,
                "[%p.%u] dataSend stream closed with Normal 'reason' (%u) before transfer completed with endOfStream. "
                "Reporting closeReason as %u.",
                (const void*) dispatcher,
                dataStream,
                closeReason,
                kHAPDataSendDataStreamProtocolCloseReason_Canceled);
        closeReason = kHAPDataSendDataStreamProtocolCloseReason_Canceled;
    }

    if (!dataSendStream->isOpen && dataSendStream->state != kHAPDataSendDataStreamProtocolStreamState_Accepting) {
        HAPLogInfo(
                &logObject,
                "[%p.%u] [%p] Closing dataSend stream (error %u / reason %u).",
                (const void*) dispatcher,
                dataStream,
                (const void*) dataSendStream_,
                error,
                closeReason);
    } else {
        HAPLogInfo(
                &logObject,
                "[%p.%u] [%lld] Closing dataSend stream (error %u / reason %u).",
                (const void*) dispatcher,
                dataStream,
                (long long) dataSendStream->streamID,
                error,
                closeReason);
    }

    // Cancel pending operations.
    if (!error) {
        HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle);
    }
    if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_SendingData) {
        HAPLogInfo(
                &logObject,
                "[%p.%u] [%lld] Canceling %s.%s event.",
                (const void*) dispatcher,
                dataStream,
                (long long) dataSendStream->streamID,
                dataStreamProtocol->base->name,
                kHAPDataSendDataStreamProtocol_DataTopic);

        dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
        HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler completionHandler =
                dataSendStream->_.sendData.completionHandler;
        void* scratchBytes = dataSendStream->_.sendData.scratchBytes;
        size_t numScratchBytes = dataSendStream->_.sendData.numScratchBytes;

        // Clear potential transaction from dispatcher before clearing the transaction buffer
        HAPDataStreamDispatcherClearTransaction(
                server, dispatcher, dataStream, &dataSendStream->_.sendData.transaction);

        HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);
        completionHandler(
                server,
                dispatcher,
                dataStreamProtocol,
                request,
                dataStream,
                dataSendStream_,
                error,
                scratchBytes,
                numScratchBytes,
                context);

        if (dataSendStream->wasClosed && (error == kHAPError_InvalidState)) {
            // Delayed close should not be reported as an error.
            // Update error to None for callbacks->handleClose called below.
            error = kHAPError_None;
        }
    }

    // Unregister dataSend stream in linked list.
    HAPAssert(protocolContext->firstDataSendStream);
    bool ok = false;
    if (protocolContext->firstDataSendStream == dataSendStream_) {
        protocolContext->firstDataSendStream = dataSendStream->nextDataSendStream;
        dataSendStream->nextDataSendStream = NULL;
        ok = true;
    } else {
        HAPDataSendDataStreamProtocolStream* previousDataSendStream_ = protocolContext->firstDataSendStream;
        while (previousDataSendStream_) {
            HAPDataSendDataStreamProtocolStream* previousDataSendStream =
                    (HAPDataSendDataStreamProtocolStream*) previousDataSendStream_;

            if (previousDataSendStream->nextDataSendStream == dataSendStream_) {
                previousDataSendStream->nextDataSendStream = dataSendStream->nextDataSendStream;
                dataSendStream->nextDataSendStream = NULL;
                ok = true;
                break;
            }

            previousDataSendStream_ = previousDataSendStream->nextDataSendStream;
        }
    }
    HAPAssert(ok);

    // Clear potential transaction from dispatcher before clearing the transaction buffer
    if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Closing) {
        HAPDataStreamDispatcherClearTransaction(server, dispatcher, dataStream, &dataSendStream->_.close.transaction);
    } else if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening) {
        HAPDataStreamDispatcherClearTransaction(server, dispatcher, dataStream, &dataSendStream->_.open.transaction);
    }

    // Inform delegate.
    const HAPDataSendDataStreamProtocolStreamCallbacks* callbacks = dataSendStream->callbacks;
    HAPRawBufferZero(dataSendStream_, sizeof *dataSendStream_);
    callbacks->handleClose(
            server, dispatcher, dataStreamProtocol_, request, dataStream, dataSendStream_, error, closeReason, context);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Gets the close reason to send as part of the dataSend.close message.
 *
 * @param      dataSendStream_      "dataSend" stream.
 *
 * @return Close reason.
 */
HAP_RESULT_USE_CHECK
static HAPDataSendDataStreamProtocolCloseReason
        GetSendCloseReason(HAPDataSendDataStreamProtocolStream* dataSendStream_) {
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;

    if (dataSendStream->wasCanceled) {
        switch (dataSendStream->cancellationReason) {
            case kHAPDataSendDataStreamProtocolCancellationReason_Normal:
                return kHAPDataSendDataStreamProtocolCloseReason_Normal;
            case kHAPDataSendDataStreamProtocolCancellationReason_NotAllowed:
                return kHAPDataSendDataStreamProtocolCloseReason_NotAllowed;
            case kHAPDataSendDataStreamProtocolCancellationReason_Canceled:
                return kHAPDataSendDataStreamProtocolCloseReason_Canceled;
            case kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure:
                return kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure;
            case kHAPDataSendDataStreamProtocolCancellationReason_BadData:
                return kHAPDataSendDataStreamProtocolCloseReason_BadData;
            case kHAPDataSendDataStreamProtocolCancellationReason_ProtocolError:
                return kHAPDataSendDataStreamProtocolCloseReason_ProtocolError;
            case kHAPDataSendDataStreamProtocolCancellationReason_InvalidConfiguration:
                return kHAPDataSendDataStreamProtocolCloseReason_InvalidConfiguration;
            default:
                HAPFatalError();
        }
    } else if (dataSendStream->hasTimedOut) {
        return kHAPDataSendDataStreamProtocolCloseReason_Timeout;
    } else {
        return kHAPDataSendDataStreamProtocolCloseReason_Normal;
    }
}

static void HandleSendCloseEventComplete(
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_CloseTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ =
            GetDataSendStreamForSendEventTransaction(server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(messageBytes == dataSendStream->outBytes);
    HAPPrecondition(numMessageBytes <= sizeof dataSendStream->outBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    if (dataSendStream->wasClosed) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] dataSend stream has been closed while sending %s.%s event.",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidState,
              dataSendStream->closeReason,
              context);
        return;
    }

    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Sent %s.%s event.",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            topic);

    // Close dataSend stream.
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Closing);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);

    // Inform delegate.
    Close(server,
          dispatcher,
          dataStreamProtocol,
          request,
          dataStream,
          dataSendStream_,
          kHAPError_None,
          GetSendCloseReason(dataSendStream_),
          context);
}

static void SendClose(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream_) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(dataSendStream->isOpen);
    HAPPrecondition(!dataSendStream->wasClosed);

    HAPError err;

    // Send dataSend.close event.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, dataSendStream->outBytes, sizeof dataSendStream->outBytes);
    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "streamId");
    }
    if (!err) {
        err = HAPOPACKWriterAppendInt(&messageWriter, dataSendStream->streamID);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "reason");
    }
    if (!err) {
        err = HAPOPACKWriterAppendInt(&messageWriter, GetSendCloseReason(dataSendStream_));
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%lld] Outgoing buffer too small.",
                (const void*) dispatcher,
                dataStream,
                (long long) dataSendStream->streamID);
        HAPFatalError();
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Sending %s.%s event.",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            kHAPDataSendDataStreamProtocol_CloseTopic);
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Closing;
    HAPDataStreamDispatcherSendMutableEvent(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            &dataSendStream->_.close.transaction,
            kHAPDataSendDataStreamProtocol_CloseTopic,
            bytes,
            numBytes,
            HandleSendCloseEventComplete);
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);

    // Inform delegate.
    if (dataStreamProtocol->callbacks.handleAccept) {
        dataStreamProtocol->callbacks.handleAccept(
                server, dispatcher, dataStreamProtocol, request, dataStream, context);
    }
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
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];

    // Invalidate all dataSend streams.
    HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream = protocolContext->firstDataSendStream;
    while (dataSendStream) {
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream,
              kHAPError_InvalidState,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        dataSendStream = protocolContext->firstDataSendStream;
    }

    // Invalidate all listeners.
    if (dataStreamProtocol->storage.listeners) {
        HAPDataSendDataStreamProtocolListener* listener = &dataStreamProtocol->storage.listeners[dataStream];
        HAPRawBufferZero(listener, sizeof *listener);
    }

    // Inform delegate.
    if (dataStreamProtocol->callbacks.handleInvalidate) {
        dataStreamProtocol->callbacks.handleInvalidate(
                server, dispatcher, dataStreamProtocol, request, dataStream, context);
    }
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(!messageBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }

    HAPLog(&logObject, "[%p.%u] Skipped event.", (const void*) dispatcher, dataStream);
}

static void HandleAckEvent(
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_AckTopic;
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
    HAPOPACKStringDictionaryElement streamIdElement, endOfStreamElement;
    streamIdElement.key = "streamId";
    endOfStreamElement.key = "endOfStream";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &streamIdElement, &endOfStreamElement, NULL });
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

    // Parse streamId.
    if (!streamIdElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event does not contain 'streamId' key.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    HAPDataSendDataStreamProtocolStreamID streamID;
    err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamID);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with malformed 'streamId'.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream_ =
            GetDataSendStreamForStreamID(dataStreamProtocol, dataStream, streamID);
    if (!dataSendStream_) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with unexpected 'streamId' (%lld).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic,
               (long long) streamID);
        return;
    }
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;

    // Parse endOfStream.
    if (!endOfStreamElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event does not contain 'endOfStream' key.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    bool endOfStream;
    err = HAPOPACKReaderGetNextBool(&endOfStreamElement.value.reader, &endOfStream);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with malformed 'endOfStream'.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    if (!endOfStream) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with unexpected 'endOfStream' (%u).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic,
               endOfStream);
        return;
    }

    // Update state.
    if (!dataSendStream->endOfStream) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] Received %s.%s event although endOfStream has not been set.",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    if (dataSendStream->ackReceived) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] Received duplicate %s.%s event.",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    dataSendStream->ackReceived = true;

    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Received %s.%s event (endOfStream: %u).",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            topic,
            endOfStream);

    // Continue completing dataSend stream.
    if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle) {
        HAPAssert(!dataSendStream->wasCanceled);
        HAPAssert(!dataSendStream->hasTimedOut);

        // Close "dataSend" stream if endOfStream has been sent and acknowledged.
        if (dataSendStream->endOfStream && dataSendStream->ackReceived) {
            HAPLogInfo(
                    &logObject,
                    "[%p.%u] [%lld] endOfStream has been acknowledged. Closing dataSend stream.",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID);
            SendClose(server, dispatcher, dataStream, dataSendStream_);
        }
    }
}

static void HandleCloseEvent(
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_CloseTopic;
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
    HAPOPACKStringDictionaryElement streamIdElement, reasonElement;
    streamIdElement.key = "streamId";
    reasonElement.key = "reason";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &streamIdElement, &reasonElement, NULL });
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

    // Parse streamId.
    if (!streamIdElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event does not contain 'streamId' key.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    HAPDataSendDataStreamProtocolStreamID streamID;
    err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamID);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with malformed 'streamId'.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream_ =
            GetDataSendStreamForStreamID(dataStreamProtocol, dataStream, streamID);
    if (!dataSendStream_) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with unexpected 'streamId' (%lld).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic,
               (long long) streamID);
        return;
    }
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;

    // Parse reason.
    if (!reasonElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event does not contain 'reason' key.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    int64_t rawReason;
    err = HAPOPACKReaderGetNextInt(&reasonElement.value.reader, &rawReason);
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with malformed 'reason'.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        return;
    }
    HAPDataSendDataStreamProtocolCloseReason reason;
    if (!HAPDataSendDataStreamProtocolCloseReasonIsValid(rawReason)) {
        HAPLog(&logObject,
               "[%p.%u] Received %s.%s event with unknown 'reason' (%lld). Treating as %u.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic,
               (long long) rawReason,
               kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure);
        reason = kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure;
    } else {
        reason = (HAPDataSendDataStreamProtocolCloseReason) rawReason;
    }

    // Update state.
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Received %s.%s event (reason: %u).",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            topic,
            reason);
    dataSendStream->closeReason = reason;
    dataSendStream->wasClosed = true;

    if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] dataSend stream has been closed by the controller.",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID);
        Close(server,
              dispatcher,
              dataStreamProtocol,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_None,
              dataSendStream->closeReason,
              context);
    }
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(topic);

    if (HAPStringAreEqual(topic, kHAPDataSendDataStreamProtocol_AckTopic)) {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Receiving %s.%s event (short).",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        HAPDataStreamDispatcherReceiveShortEvent(server, dispatcher, dataStream, HandleAckEvent);
    } else if (HAPStringAreEqual(topic, kHAPDataSendDataStreamProtocol_CloseTopic)) {
        HAPLogDebug(
                &logObject,
                "[%p.%u] Receiving %s.%s event (short).",
                (const void*) dispatcher,
                dataStream,
                dataStreamProtocol->base->name,
                topic);
        HAPDataStreamDispatcherReceiveShortEvent(server, dispatcher, dataStream, HandleCloseEvent);
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

static void ContinueListening(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context);

static void HandleSendOpenResponseComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse);
    HAPPrecondition(transaction);
    HAPPrecondition(transaction == &listener->transaction);
    HAPPrecondition(messageBytes == listener->outBytes);
    HAPPrecondition(numMessageBytes <= sizeof listener->outBytes);

    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);

    HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream_ =
            GetDataSendStreamForStreamID(dataStreamProtocol, dataStream, listener->streamID);
    HAPAssert(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Accepting);
    HAPAssert(!dataSendStream->isOpen);
    HAPAssert(!dataSendStream->hasTimedOut);
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Sent %s.%s response.",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            topic);

    // Reset listener state.
    bool hasPendingOpen = listener->hasPendingOpen;
    HAPRawBufferZero(listener_, sizeof *listener_);
    listener->hasPendingOpen = hasPendingOpen;
    listener->state = kHAPDataSendDataStreamProtocolListenerState_Idle;

    // Start receiving next open request if one is pending.
    if (listener->hasPendingOpen) {
        ContinueListening(server, dispatcher, dataStreamProtocol, request, dataStream, context);
    }

    // Open dataSend stream.
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    dataSendStream->isOpen = true;

    // Check for cancellation.
    if (dataSendStream->wasCanceled) {
        SendClose(server, dispatcher, dataStream, dataSendStream_);
        return;
    }

    // Inform delegate.
    dataSendStream->callbacks->handleOpen(
            server, dispatcher, dataStreamProtocol, request, dataStream, HAPNonnull(dataSendStream_), context);
}

static void HandleSendOpenProtocolErrorResponseComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse);
    HAPPrecondition(transaction);
    HAPPrecondition(transaction == &listener->transaction);
    HAPPrecondition(messageBytes == listener->outBytes);
    HAPPrecondition(numMessageBytes <= sizeof listener->outBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);

    HAPLogInfo(
            &logObject,
            "[%p.%u] [%lld] Sent %s.%s protocol error response.",
            (const void*) dispatcher,
            dataStream,
            (long long) listener->streamID,
            dataStreamProtocol->base->name,
            topic);

    // Reset listener state.
    bool hasPendingOpen = listener->hasPendingOpen;
    HAPRawBufferZero(listener_, sizeof *listener_);
    listener->hasPendingOpen = hasPendingOpen;
    listener->state = kHAPDataSendDataStreamProtocolListenerState_Idle;

    // Start receiving next open request if one is pending.
    if (listener->hasPendingOpen) {
        ContinueListening(server, dispatcher, dataStreamProtocol, request, dataStream, context);
    }
}

static void HandleSendOpenErrorResponseComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse);
    HAPPrecondition(transaction);
    HAPPrecondition(transaction == &listener->transaction);
    HAPPrecondition(messageBytes == listener->outBytes);
    HAPPrecondition(numMessageBytes <= sizeof listener->outBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);

    HAPLogInfo(
            &logObject,
            "[%p.%u] Sent %s.%s error response.",
            (const void*) dispatcher,
            dataStream,
            dataStreamProtocol->base->name,
            topic);

    // Reset listener state.
    bool hasPendingOpen = listener->hasPendingOpen;
    HAPRawBufferZero(listener_, sizeof *listener_);
    listener->hasPendingOpen = hasPendingOpen;
    listener->state = kHAPDataSendDataStreamProtocolListenerState_Idle;

    // Start receiving next open request if one is pending.
    if (listener->hasPendingOpen) {
        ContinueListening(server, dispatcher, dataStreamProtocol, request, dataStream, context);
    }
}

void HAPDataSendDataStreamProtocolAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream_,
        const HAPDataSendDataStreamProtocolStreamCallbacks* callbacks) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_WaitingForAccept);
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(callbacks);
    HAPPrecondition(callbacks->handleClose);
    HAPPrecondition(callbacks->handleOpen);

    HAPError err;

    // Initialize dataSend stream.
    HAPRawBufferZero(dataSendStream_, sizeof *dataSendStream_);
    dataSendStream->callbacks = callbacks;
    dataSendStream->streamID = listener->streamID;
    dataSendStream->type = listener->type;
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Accepting;
    dataSendStream->isCancellable = true;

    // Register dataSend stream in linked list.
    if (!protocolContext->firstDataSendStream) {
        protocolContext->firstDataSendStream = dataSendStream_;
    } else {
        HAPDataSendDataStreamProtocolStream* _Nullable lastDataSendStream =
                (HAPDataSendDataStreamProtocolStream*) protocolContext->firstDataSendStream;
        while (lastDataSendStream->nextDataSendStream) {
            lastDataSendStream = (HAPDataSendDataStreamProtocolStream*) lastDataSendStream->nextDataSendStream;
        }
        lastDataSendStream->nextDataSendStream = dataSendStream_;
    }

    listener->state = kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse;

    // Send dataSend.open response.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, listener->outBytes, sizeof listener->outBytes);

    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%lld] Outgoing buffer too small.",
                (const void*) dispatcher,
                dataStream,
                (long long) listener->streamID);
        HAPFatalError();
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);
    HAPLog(&logObject,
           "[%p.%u] [%lld] Sending %s.%s response (status %u / dataSend status %u).",
           (const void*) dispatcher,
           dataStream,
           (long long) listener->streamID,
           dataStreamProtocol->base->name,
           topic,
           kHAPDataStreamResponseStatus_Success,
           0);
    HAPDataStreamDispatcherSendMutableResponse(
            server,
            dispatcher,
            dataStream,
            &listener->transaction,
            kHAPDataStreamResponseStatus_Success,
            bytes,
            numBytes,
            HandleSendOpenResponseComplete);
}

void HAPDataSendDataStreamProtocolReject(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolRejectReason rejectReason) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_WaitingForAccept);
    HAPPrecondition(HAPDataSendDataStreamProtocolRejectReasonIsValid(rejectReason));

    HAPError err;

    listener->state = kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse;

    // Send dataSend.open protocol error response.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, listener->outBytes, sizeof listener->outBytes);
    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "status");
    }
    if (!err) {
        err = HAPOPACKWriterAppendInt(&messageWriter, rejectReason);
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%lld] Outgoing buffer too small.",
                (const void*) dispatcher,
                dataStream,
                (long long) listener->streamID);
        HAPFatalError();
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);
    HAPLog(&logObject,
           "[%p.%u] [%lld] Sending %s.%s protocol error response.",
           (const void*) dispatcher,
           dataStream,
           (long long) listener->streamID,
           dataStreamProtocol->base->name,
           topic);
    HAPDataStreamDispatcherSendMutableResponse(
            server,
            dispatcher,
            dataStream,
            &listener->transaction,
            kHAPDataStreamResponseStatus_ProtocolSpecificError,
            bytes,
            numBytes,
            HandleSendOpenProtocolErrorResponseComplete);
}

static void HandleOpenRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state == kHAPDataSendDataStreamProtocolListenerState_ReceivingOpenRequest);
    HAPPrecondition(transaction);
    HAPPrecondition(transaction == &listener->transaction);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(!isComplete);

    HAPError err = kHAPError_None;

    HAPDataStreamResponseStatus responseStatus = kHAPDataStreamResponseStatus_Success;
    HAPDataSendDataStreamProtocolRejectReason rejectReason = (HAPDataSendDataStreamProtocolRejectReason) 0;
    HAPDataSendDataStreamProtocolOpenMetadata metadata HAP_UNUSED = { 0 };

    char* rawType = NULL;
    HAPDataSendDataStreamProtocolType type = (HAPDataSendDataStreamProtocolType) 0;
    HAPDataSendDataStreamProtocolStreamID streamID = 0;

    {
        if (!messageBytes) {
            HAPLog(&logObject,
                   "[%p.%u] Skipped %s.%s request.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_OutOfMemory;
            goto parsingDone;
        }

        // Parse Message.
        HAPOPACKReader messageReader;
        HAPOPACKReaderCreate(&messageReader, messageBytes, numMessageBytes);
        HAPOPACKStringDictionaryElement targetElement, typeElement, streamIdElement;
        targetElement.key = "target";
        typeElement.key = "type";
        streamIdElement.key = "streamId";
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        HAPOPACKStringDictionaryElement metadataElement;
        metadataElement.key = "metadata";
        err = HAPOPACKStringDictionaryReaderGetAll(
                &messageReader,
                (HAPOPACKStringDictionaryElement* const[]) {
                        &targetElement, &typeElement, &streamIdElement, &metadataElement, NULL });
#else
        err = HAPOPACKStringDictionaryReaderGetAll(
                &messageReader,
                (HAPOPACKStringDictionaryElement* const[]) { &targetElement, &typeElement, &streamIdElement, NULL });
#endif
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received malformed %s.%s request.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }

        // Parse target.
        if (!targetElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request does not contain 'target' key.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        char* target;
        err = HAPOPACKReaderGetNextString(&targetElement.value.reader, &target);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with malformed 'target'.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        if (!HAPStringAreEqual(target, "controller")) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with unexpected 'target' (%s).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   target);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }

        // Parse type.
        if (!typeElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request does not contain 'type' key.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        err = HAPOPACKReaderGetNextString(&typeElement.value.reader, &rawType);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with malformed 'type'.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        if (HAPStringAreEqual(rawType, kHAPDataSendDataStreamProtocolTypeDescription_Audio_Siri)) {
            type = kHAPDataSendDataStreamProtocolType_Audio_Siri;
        } else if (HAPStringAreEqual(rawType, kHAPDataSendDataStreamProtocolTypeDescription_IPCamera_Recording)) {
            type = kHAPDataSendDataStreamProtocolType_IPCamera_Recording;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        } else if (HAPStringAreEqual(rawType, kHAPDataSendDataStreamProtocolTypeDescription_Diagnostics_Snapshot)) {
            type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot;
            if (HAPSessionControllerIsAdmin(request->session) == false) {
                HAPLog(&logObject,
                       "[%p.%u] Received %s.%s request from a non-admin controller",
                       (const void*) dispatcher,
                       dataStream,
                       dataStreamProtocol->base->name,
                       topic);
                responseStatus = kHAPDataStreamResponseStatus_ProtocolSpecificError;
                rejectReason = kHAPDataSendDataStreamProtocolRejectReason_NotAllowed;
            }
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
        } else if (HAPStringAreEqual(rawType, kHAPDataSendDataStreamProtocolTypeDescription_Accessory_Metrics)) {
            type = kHAPDataSendDataStreamProtocolType_Accessory_Metrics;
            if (HAPSessionControllerIsAdmin(request->session) == false) {
                HAPLog(&logObject,
                       "[%p.%u] Received %s.%s request from a non-admin controller",
                       (const void*) dispatcher,
                       dataStream,
                       dataStreamProtocol->base->name,
                       topic);
                responseStatus = kHAPDataStreamResponseStatus_ProtocolSpecificError;
                rejectReason = kHAPDataSendDataStreamProtocolRejectReason_NotAllowed;
            }
#endif // HAP_FEATURE_ACCESSORY_METRICS
        } else {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with unexpected 'type' (%s).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   rawType);
            responseStatus = kHAPDataStreamResponseStatus_ProtocolSpecificError;
            rejectReason = kHAPDataSendDataStreamProtocolRejectReason_Unsupported;
            // Do not jump to parsingDone yet so that payload errors take precedence (if code follows this check).
        }

        // Parse streamId.
        if (!streamIdElement.value.exists) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request does not contain 'streamId' key.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamID);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with malformed 'streamId'.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic);
            responseStatus = kHAPDataStreamResponseStatus_PayloadError;
            goto parsingDone;
        }
        HAPDataSendDataStreamProtocolStream* _Nullable dataSendStream_ =
                GetDataSendStreamForStreamID(dataStreamProtocol, dataStream, streamID);
        if (dataSendStream_) {
            HAPLog(&logObject,
                   "[%p.%u] Received %s.%s request with in-use 'streamId' (%lld).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   (long long) streamID);
            responseStatus = kHAPDataStreamResponseStatus_ProtocolSpecificError;
            rejectReason = kHAPDataSendDataStreamProtocolRejectReason_Busy;
            // Do not jump to parsingDone yet so that payload errors take precedence (if code follows this check).
        }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        // Parse metadata.
        if (metadataElement.value.exists && type == kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot) {
            metadata.type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot;

            HAPOPACKStringDictionaryElement maxLogSizeElement, snapshotTypeElement;
            maxLogSizeElement.key = "maxLogSize";
            snapshotTypeElement.key = "snapshotType";

            err = HAPOPACKStringDictionaryReaderGetAll(
                    &metadataElement.value.reader,
                    (HAPOPACKStringDictionaryElement* const[]) { &maxLogSizeElement, &snapshotTypeElement, NULL });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                HAPLog(&logObject,
                       "[%p.%u] Received malformed metadata %s.%s request.",
                       (const void*) dispatcher,
                       dataStream,
                       dataStreamProtocol->base->name,
                       topic);
                responseStatus = kHAPDataStreamResponseStatus_PayloadError;
                goto parsingDone;
            }

            // Parse maxLogSize.
            if (maxLogSizeElement.value.exists) {
                int64_t maxLogSize = 0;
                err = HAPOPACKReaderGetNextInt(&maxLogSizeElement.value.reader, &maxLogSize);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    HAPLog(&logObject,
                           "[%p.%u] Received %s.%s request with malformed 'maxLogSize' for metadata.",
                           (const void*) dispatcher,
                           dataStream,
                           dataStreamProtocol->base->name,
                           topic);
                    responseStatus = kHAPDataStreamResponseStatus_PayloadError;
                    goto parsingDone;
                }
                metadata._.diagnostics.snapshot.maxLogSize = maxLogSize;
            }

            // Parse snapshotType.
            if (snapshotTypeElement.value.exists) {
                int64_t snapshotType = 0;
                err = HAPOPACKReaderGetNextInt(&snapshotTypeElement.value.reader, &snapshotType);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    HAPLog(&logObject,
                           "[%p.%u] Received %s.%s request with malformed 'snapshotType' for metadata.",
                           (const void*) dispatcher,
                           dataStream,
                           dataStreamProtocol->base->name,
                           topic);
                    responseStatus = kHAPDataStreamResponseStatus_PayloadError;
                    goto parsingDone;
                }
                metadata._.diagnostics.snapshot.snapshotType = (HAPDiagnosticsSnapshotType) snapshotType;
            } else {
                HAPLogInfo(&logObject, "Snapshot type not provided. Defaulting to manufacturer snapshot.");
                metadata._.diagnostics.snapshot.snapshotType = kHAPDiagnosticsSnapshotType_Manufacturer;
            }
        } else {
            HAPLogInfo(&logObject, "Metadata not provided by controller, setting default values.");
            metadata.type = kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot;
            metadata._.diagnostics.snapshot.maxLogSize = 0;
            metadata._.diagnostics.snapshot.snapshotType = kHAPDiagnosticsSnapshotType_Manufacturer;
        }
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
    }

parsingDone:

    if (responseStatus) {
        listener->state = kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse;

        // Send dataSend.open error response.
        HAPOPACKWriter messageWriter;
        HAPOPACKWriterCreate(&messageWriter, listener->outBytes, sizeof listener->outBytes);
        if (true) {
            err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
        }
        if (responseStatus == kHAPDataStreamResponseStatus_ProtocolSpecificError) {
            HAPAssert(HAPDataSendDataStreamProtocolRejectReasonIsValid(rejectReason));
            if (!err) {
                err = HAPOPACKWriterAppendString(&messageWriter, "status");
            }
            if (!err) {
                err = HAPOPACKWriterAppendInt(&messageWriter, rejectReason);
            }
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
        if (responseStatus != kHAPDataStreamResponseStatus_ProtocolSpecificError) {
            HAPLog(&logObject,
                   "[%p.%u] Sending %s.%s error response (status %u).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   responseStatus);
        } else {
            HAPLog(&logObject,
                   "[%p.%u] Sending %s.%s error response (status %u / dataSend status %u).",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   responseStatus,
                   rejectReason);
        }
        HAPDataStreamDispatcherSendMutableResponse(
                server,
                dispatcher,
                dataStream,
                transaction,
                responseStatus,
                bytes,
                numBytes,
                HandleSendOpenErrorResponseComplete);
        return;
    }

    // Update state.
    listener->streamID = streamID;
    listener->type = type;
    listener->state = kHAPDataSendDataStreamProtocolListenerState_WaitingForAccept;

    HAPLogInfo(
            &logObject,
            "[%p.%u] [%lld] Received %s.%s request (type: %s).",
            (const void*) dispatcher,
            dataStream,
            (long long) listener->streamID,
            dataStreamProtocol->base->name,
            topic,
            rawType);

    // Inform delegate.
    const HAPDataSendStreamProtocolStreamAvailableCallbacks* delegateCallback = NULL;

    for (uint32_t i = 0; i < dataStreamProtocol->callbacks.numStreamAvailableCallbacks; i++) {
        HAPAssert(&(dataStreamProtocol->callbacks.streamAvailableCallbacks[i]));
        if (dataStreamProtocol->callbacks.streamAvailableCallbacks[i].type == type) {
            delegateCallback = &(dataStreamProtocol->callbacks.streamAvailableCallbacks[i]);
            break;
        }
    }

    HAPAssert(delegateCallback);
    HAPAssert(delegateCallback->handleStreamAvailable);
    delegateCallback->handleStreamAvailable(
            server, dispatcher, dataStreamProtocol, request, dataStream, type, &metadata, NULL, context);
}

static void HandleReceiveOpenRequestInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->state != kHAPDataSendDataStreamProtocolListenerState_Idle);
    HAPPrecondition(transaction);
    HAPPrecondition(transaction == &listener->transaction);

    HAPLog(&logObject,
           "[%p.%u] HomeKit Data Stream invalidated while handling %s.%s request.",
           (const void*) dispatcher,
           dataStream,
           dataStreamProtocol->base->name,
           topic);
    HAPRawBufferZero(transaction, sizeof *transaction);
}

static HAPDataStreamReceiveRequestTransactionCallbacks receiveOpenRequestCallbacks = {
    .handleInvalidate = HandleReceiveOpenRequestInvalidate
};

static void ContinueListening(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataStreamProtocol->storage.listeners);
    HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
    HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
    HAPPrecondition(listener->hasPendingOpen);

    switch (listener->state) {
        case kHAPDataSendDataStreamProtocolListenerState_Idle: {
            listener->hasPendingOpen = false;
            listener->state = kHAPDataSendDataStreamProtocolListenerState_ReceivingOpenRequest;
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] Receiving %s.%s request (short).",
                    (const void*) dispatcher,
                    dataStream,
                    dataStreamProtocol->base->name,
                    topic);
            HAPDataStreamDispatcherReceiveShortRequest(
                    server,
                    dispatcher,
                    dataStream,
                    &listener->transaction,
                    &receiveOpenRequestCallbacks,
                    topic,
                    HandleOpenRequest);
            break;
        }
        case kHAPDataSendDataStreamProtocolListenerState_ReceivingOpenRequest:
        case kHAPDataSendDataStreamProtocolListenerState_WaitingForAccept:
        case kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse: {
            HAPLog(&logObject,
                   "[%p.%u] Delaying accept of %s.%s request due to outstanding %s.%s request.",
                   (const void*) dispatcher,
                   dataStream,
                   dataStreamProtocol->base->name,
                   topic,
                   dataStreamProtocol->base->name,
                   topic);
            break;
        }
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
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
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(request);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(topic);

    if (dataStreamProtocol->storage.listeners && dataStreamProtocol->callbacks.numStreamAvailableCallbacks &&
        HAPStringAreEqual(topic, kHAPDataSendDataStreamProtocol_OpenTopic)) {
        HAPDataSendDataStreamProtocolListener* listener_ = &dataStreamProtocol->storage.listeners[dataStream];
        HAPDataSendDataStreamProtocolListener* listener = (HAPDataSendDataStreamProtocolListener*) listener_;
        HAPAssert(!listener->hasPendingOpen);
        listener->hasPendingOpen = true;
        ContinueListening(server, dispatcher, dataStreamProtocol_, request, dataStream, context);
    } else {
        HAPLog(&logObject,
               "[%p.%u] Rejecting unexpected %s.%s request (topic not supported).",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        HAPDataStreamDispatcherSkipAndReplyToRequest(
                server, dispatcher, dataStream, kHAPDataStreamResponseStatus_HeaderError, HandleRejectRequestComplete);
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleSendOpenRequestInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        void* _Nullable context) {
    HAPDataSendDataStreamProtocolStream* dataSendStream_ =
            GetDataSendStreamForSendRequestTransaction(server, dispatcher, dataStreamProtocol, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;

    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);

    Close(server,
          dispatcher,
          dataStreamProtocol,
          request,
          dataStream,
          dataSendStream_,
          kHAPError_InvalidState,
          kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
          context);
}

static void HandleOpenResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = GetDataSendStreamForSendRequestTransaction(
            server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);
    if (!messageBytes) {
        HAPLog(&logObject,
               "[%p.%u] Skipped %s.%s response.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_OutOfResources,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    HAPError err;

    // Complete receiving Message.
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening);
    HAPTime now = HAPPlatformClockGetCurrent();
    bool hasTimedOut = dataSendStream->_.open.startTime + kHAPDataSendDataStreamProtocol_OpenTimeout < now;
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);

    // Parse Message.
    HAPOPACKReader messageReader;
    HAPOPACKReaderCreate(&messageReader, messageBytes, numMessageBytes);
    HAPOPACKStringDictionaryElement streamIdElement;
    streamIdElement.key = "streamId";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &streamIdElement, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] [%p] Received malformed %s.%s response.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    // Parse streamId.
    if (!streamIdElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s response does not contain 'streamId' key.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }
    int64_t streamId;
    err = HAPOPACKReaderGetNextInt(&streamIdElement.value.reader, &streamId);
    if (err) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s response with malformed 'streamId' key.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    // Open dataSend stream.
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%p] Received %s.%s response (streamId: %llu).",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            topic,
            (long long) streamId);
    dataSendStream->streamID = streamId;
    dataSendStream->isOpen = true;

    // Check for timeout.
    if (hasTimedOut) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] Received %s.%s response too late (timeout).",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID,
               dataStreamProtocol->base->name,
               topic);
        HAPAssert(!dataSendStream->hasTimedOut);
        dataSendStream->hasTimedOut = true;
    }

    // Check for timeout / cancellation.
    if (dataSendStream->wasCanceled || dataSendStream->hasTimedOut) {
        SendClose(server, dispatcher, dataStream, dataSendStream_);
        return;
    }

    // Inform delegate.
    dataSendStream->callbacks->handleOpen(
            server, dispatcher, dataStreamProtocol, request, dataStream, dataSendStream_, context);
}

static void HandleOpenProtocolErrorResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = GetDataSendStreamForSendRequestTransaction(
            server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);
    if (!messageBytes) {
        HAPLog(&logObject,
               "[%p.%u] Skipped %s.%s protocol error response.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_OutOfResources,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    HAPError err;

    // Complete receiving Message.
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);

    // Parse Message.
    HAPOPACKReader messageReader;
    HAPOPACKReaderCreate(&messageReader, messageBytes, numMessageBytes);
    HAPOPACKStringDictionaryElement statusElement;
    statusElement.key = "status";
    err = HAPOPACKStringDictionaryReaderGetAll(
            &messageReader, (HAPOPACKStringDictionaryElement* const[]) { &statusElement, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        HAPLog(&logObject,
               "[%p.%u] [%p] Received malformed %s.%s protocol error response.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    // Parse status.
    if (!statusElement.value.exists) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s protocol error response does not contain 'status' key.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }
    int64_t rawStatus;
    err = HAPOPACKReaderGetNextInt(&statusElement.value.reader, &rawStatus);
    if (err) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s protocol error response with malformed 'status' key.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }
    if (!HAPDataSendDataStreamProtocolRejectReasonIsValid(rawStatus)) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s protocol error response with unknown 'status' (%lld).",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic,
               (long long) rawStatus);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }
    HAPDataSendDataStreamProtocolRejectReason status = (HAPDataSendDataStreamProtocolRejectReason) rawStatus;

    // Close dataSend stream.
    HAPLogInfo(
            &logObject,
            "[%p.%u] [%p] Received %s.%s protocol error response (status: %u).",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            topic,
            status);
    Close(server,
          dispatcher,
          dataStreamProtocol_,
          request,
          dataStream,
          dataSendStream_,
          kHAPError_InvalidState,
          kHAPDataSendDataStreamProtocolCloseReason_ProtocolError,
          context);
}

static void HandleOpenErrorResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = GetDataSendStreamForSendRequestTransaction(
            server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(isComplete);
    if (!messageBytes) {
        HAPLog(&logObject,
               "[%p.%u] Skipped %s.%s error response.",
               (const void*) dispatcher,
               dataStream,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_OutOfResources,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    // Complete receiving Message.
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Opening);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);

    // Parse Message.
    if (numMessageBytes) {
        HAPLog(&logObject,
               "[%p.%u] [%p] Received %s.%s error response with non-empty Message.",
               (const void*) dispatcher,
               dataStream,
               (const void*) dataSendStream_,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidData,
              kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
              context);
        return;
    }

    // Close dataSend stream.
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%p] Received %s.%s error response.",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            topic);
    Close(server,
          dispatcher,
          dataStreamProtocol_,
          request,
          dataStream,
          dataSendStream_,
          kHAPError_InvalidData,
          kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure,
          context);
}

static void HandleOpenResponseAvailable(
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
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = GetDataSendStreamForSendRequestTransaction(
            server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPPrecondition(topic);
    HAPPrecondition(HAPStringAreEqual(topic, kHAPDataSendDataStreamProtocol_OpenTopic));

    // See HomeKit Accessory Protocol Specification R17
    // Section 13.2.1.1 Start

    // Select appropriate completion handler.
    HAPDataStreamSendRequestMessageCompletionHandler completionHandler;
    if (status != kHAPDataStreamResponseStatus_Success) {
        if (status == kHAPDataStreamResponseStatus_ProtocolSpecificError) {
            completionHandler = HandleOpenProtocolErrorResponse;
        } else {
            completionHandler = HandleOpenErrorResponse;
        }
    } else {
        completionHandler = HandleOpenResponse;
    }

    // Receive Message.
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%p] Receiving %s.%s 0x%02X response.",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            topic,
            status);
    HAPDataStreamDispatcherReceiveShortResponse(server, dispatcher, dataStream, transaction, completionHandler);
}

static void HandleSendOpenRequestComplete(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol_,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPPrecondition(dataStreamProtocol_);
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_OpenTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ = GetDataSendStreamForSendRequestTransaction(
            server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(messageBytes == dataSendStream->outBytes);
    HAPPrecondition(numMessageBytes <= sizeof dataSendStream->outBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    HAPAssert(!isComplete);

    HAPLogDebug(
            &logObject,
            "[%p.%u] [%p] Sent %s.%s request.",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            topic);
}

static HAPDataStreamSendRequestTransactionCallbacks sendOpenRequestCallbacks = {
    .handleInvalidate = HandleSendOpenRequestInvalidate,
    .handleResponseAvailable = HandleOpenResponseAvailable
};

void HAPDataSendDataStreamProtocolOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream_,
        HAPDataSendDataStreamProtocolType type,
        const HAPDataSendDataStreamProtocolStreamCallbacks* callbacks) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPDataSendDataStreamProtocolContext* protocolContext =
            (HAPDataSendDataStreamProtocolContext*) &dataStreamProtocol->storage.protocolContexts[dataStream];
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(HAPDataSendDataStreamProtocolTypeIsValid(type));
    HAPPrecondition(callbacks);
    HAPPrecondition(callbacks->handleClose);
    HAPPrecondition(callbacks->handleOpen);

    HAPError err;

    // Check that dataSend stream is not in use.
    HAPDataSendDataStreamProtocolStream* _Nullable otherDataSendStream_ = protocolContext->firstDataSendStream;
    while (otherDataSendStream_) {
        HAPDataSendDataStreamProtocolStream* otherDataSendStream =
                (HAPDataSendDataStreamProtocolStream*) otherDataSendStream_;
        if (otherDataSendStream_ == dataSendStream_) {
            HAPLogError(
                    &logObject,
                    "[%p.%u] [%p] dataSend stream is already in use.",
                    (const void*) dispatcher,
                    dataStream,
                    (const void*) dataSendStream_);
            HAPFatalError();
        }
        otherDataSendStream_ = otherDataSendStream->nextDataSendStream;
    }

    // Initialize dataSend stream.
    HAPRawBufferZero(dataSendStream_, sizeof *dataSendStream_);
    dataSendStream->callbacks = callbacks;
    dataSendStream->type = type;

    // Register dataSend stream in linked list.
    if (!protocolContext->firstDataSendStream) {
        protocolContext->firstDataSendStream = dataSendStream_;
    } else {
        HAPDataSendDataStreamProtocolStream* _Nullable lastDataSendStream =
                (HAPDataSendDataStreamProtocolStream*) protocolContext->firstDataSendStream;
        while (lastDataSendStream->nextDataSendStream) {
            lastDataSendStream = (HAPDataSendDataStreamProtocolStream*) lastDataSendStream->nextDataSendStream;
        }
        lastDataSendStream->nextDataSendStream = dataSendStream_;
    }

    // Send dataSend.open request.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, dataSendStream->outBytes, sizeof dataSendStream->outBytes);
    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "target");
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "controller");
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "type");
    }
    HAPAssert(type == kHAPDataSendDataStreamProtocolType_Audio_Siri);
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "audio.siri");
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%p] Outgoing buffer too small.",
                (const void*) dispatcher,
                dataStream,
                (const void*) dataSendStream_);
        HAPFatalError();
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);
    HAPLogDebug(
            &logObject,
            "[%p.%u] [%p] Sending %s.%s request.",
            (const void*) dispatcher,
            dataStream,
            (const void*) dataSendStream_,
            dataStreamProtocol->base->name,
            kHAPDataSendDataStreamProtocol_OpenTopic);
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Opening;
    dataSendStream->isCancellable = true;
    dataSendStream->_.open.startTime = HAPPlatformClockGetCurrent();
    HAPDataStreamDispatcherSendMutableRequest(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            &dataSendStream->_.open.transaction,
            &sendOpenRequestCallbacks,
            kHAPDataSendDataStreamProtocol_OpenTopic,
            bytes,
            numBytes,
            HandleSendOpenRequestComplete);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks whether a value represents a valid "dataSend" cancel reason.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool HAPDataSendDataStreamProtocolCancellationReasonIsValid(int64_t value) {
    if ((uint64_t) value > (HAPDataSendDataStreamProtocolCancellationReason) -1) {
        return false;
    }
    switch ((HAPDataSendDataStreamProtocolCancellationReason) value) {
        case kHAPDataSendDataStreamProtocolCancellationReason_Normal:
        case kHAPDataSendDataStreamProtocolCancellationReason_NotAllowed:
        case kHAPDataSendDataStreamProtocolCancellationReason_Canceled:
        case kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure:
        case kHAPDataSendDataStreamProtocolCancellationReason_BadData:
        case kHAPDataSendDataStreamProtocolCancellationReason_ProtocolError:
        case kHAPDataSendDataStreamProtocolCancellationReason_InvalidConfiguration: {
            return true;
        }
        default:
            return false;
    }
}

void HAPDataSendDataStreamProtocolCancel(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream) {
    HAPDataSendDataStreamProtocolCancelWithReason(
            server, dispatcher, dataStream, dataSendStream, kHAPDataSendDataStreamProtocolCancellationReason_Canceled);
}

void HAPDataSendDataStreamProtocolCancelWithReason(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream_,
        HAPDataSendDataStreamProtocolCancellationReason cancellationReason) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(HAPDataSendDataStreamProtocolCancellationReasonIsValid(cancellationReason));

    if (dataSendStream->wasCanceled || dataSendStream->hasTimedOut || dataSendStream->wasClosed) {
        return;
    }

    switch (dataSendStream->state) {
        case kHAPDataSendDataStreamProtocolStreamState_Idle: {
            HAPAssert(dataSendStream->isOpen);
            HAPLog(&logObject,
                   "[%p.%u] [%lld] Canceling dataSend stream.",
                   (const void*) dispatcher,
                   dataStream,
                   (long long) dataSendStream->streamID);
            dataSendStream->wasCanceled = true;
            dataSendStream->cancellationReason = cancellationReason;
            SendClose(server, dispatcher, dataStream, dataSendStream_);
            break;
        }
        case kHAPDataSendDataStreamProtocolStreamState_Opening: {
            HAPAssert(!dataSendStream->isOpen);
            HAPLog(&logObject,
                   "[%p.%u] [%p] dataSend stream will be canceled after being opened.",
                   (const void*) dispatcher,
                   dataStream,
                   (const void*) dataSendStream_);
            dataSendStream->wasCanceled = true;
            dataSendStream->cancellationReason = cancellationReason;
            break;
        }
        case kHAPDataSendDataStreamProtocolStreamState_Accepting: {
            HAPAssert(!dataSendStream->isOpen);
            HAPLog(&logObject,
                   "[%p.%u] [%lld] dataSend stream will be canceled after being opened.",
                   (const void*) dispatcher,
                   dataStream,
                   (long long) dataSendStream->streamID);
            dataSendStream->wasCanceled = true;
            dataSendStream->cancellationReason = cancellationReason;
            break;
        }
        case kHAPDataSendDataStreamProtocolStreamState_SendingData: {
            HAPAssert(dataSendStream->isOpen);
            HAPLog(&logObject,
                   "[%p.%u] [%lld] dataSend stream will be canceled after current packet is sent.",
                   (const void*) dispatcher,
                   dataStream,
                   (long long) dataSendStream->streamID);
            dataSendStream->wasCanceled = true;
            dataSendStream->cancellationReason = cancellationReason;
            break;
        }
        case kHAPDataSendDataStreamProtocolStreamState_Closing: {
            HAPAssert(dataSendStream->isOpen);
            HAPLog(&logObject,
                   "[%p.%u] [%lld] dataSend stream is already closing.",
                   (const void*) dispatcher,
                   dataStream,
                   (long long) dataSendStream->streamID);
            break;
        }
    }
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
    HAPDataSendDataStreamProtocol* dataStreamProtocol = dataStreamProtocol_;
    HAPPrecondition(request);
    HAPPrecondition(HAPStringAreEqual(dataStreamProtocol->base->name, kHAPDataSendDataStreamProtocol_Name));
    const char* topic = kHAPDataSendDataStreamProtocol_DataTopic;
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream_ =
            GetDataSendStreamForSendEventTransaction(server, dispatcher, dataStreamProtocol_, dataStream, transaction);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(messageBytes == dataSendStream->_.sendData.scratchBytes);
    HAPPrecondition(numMessageBytes <= dataSendStream->_.sendData.numScratchBytes);
    if (error) {
        HAPAssert(error == kHAPError_InvalidState);
        return;
    }
    if (dataSendStream->wasClosed) {
        HAPLog(&logObject,
               "[%p.%u] [%lld] dataSend stream has been closed while sending %s.%s event.",
               (const void*) dispatcher,
               dataStream,
               (long long) dataSendStream->streamID,
               dataStreamProtocol->base->name,
               topic);
        Close(server,
              dispatcher,
              dataStreamProtocol_,
              request,
              dataStream,
              dataSendStream_,
              kHAPError_InvalidState,
              dataSendStream->closeReason,
              context);
        return;
    }

    HAPLogDebug(
            &logObject,
            "[%p.%u] [%lld] Sent %s.%s event.",
            (const void*) dispatcher,
            dataStream,
            (long long) dataSendStream->streamID,
            dataStreamProtocol->base->name,
            topic);

    // Inform delegate.
    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_SendingData);
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_Idle;
    HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler completionHandler =
            dataSendStream->_.sendData.completionHandler;
    void* scratchBytes = dataSendStream->_.sendData.scratchBytes;
    size_t numScratchBytes = dataSendStream->_.sendData.numScratchBytes;
    HAPRawBufferZero(&dataSendStream->_, sizeof dataSendStream->_);
    completionHandler(
            server,
            dispatcher,
            dataStreamProtocol,
            request,
            dataStream,
            dataSendStream_,
            error,
            scratchBytes,
            numScratchBytes,
            context);

    // Continue completing dataSend stream.
    if (dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle) {
        // Check for timeout / cancellation.
        if (dataSendStream->wasCanceled || dataSendStream->hasTimedOut) {
            SendClose(server, dispatcher, dataStream, dataSendStream_);
            return;
        }

        // Close "dataSend" stream if endOfStream has been sent and acknowledged.
        if (dataSendStream->endOfStream && dataSendStream->ackReceived) {
            HAPLogInfo(
                    &logObject,
                    "[%p.%u] [%lld] endOfStream has been acknowledged. Closing dataSend stream.",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID);
            SendClose(server, dispatcher, dataStream, dataSendStream_);
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPDataSendDataStreamProtocolSendData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream_,
        void* scratchBytes,
        size_t numScratchBytes,
        HAPDataSendDataStreamProtocolPacket* _Nullable packets,
        size_t numPackets,
        bool endOfStream,
        HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler completionHandler) {
    HAPPrecondition(server);
    HAPPrecondition(dispatcher);
    HAPDataSendDataStreamProtocol* dataStreamProtocol =
            HAPDataStreamDispatcherFindHandlerForProtocolName(server, dispatcher, kHAPDataSendDataStreamProtocol_Name);
    HAPPrecondition(dataStreamProtocol);
    HAPPrecondition(dataStream < dataStreamProtocol->storage.numDataStreams);
    HAPPrecondition(dataSendStream_);
    HAPDataSendDataStreamProtocolStream* dataSendStream = (HAPDataSendDataStreamProtocolStream*) dataSendStream_;
    HAPPrecondition(dataSendStream->isOpen);
    HAPPrecondition(!dataSendStream->wasClosed);
    HAPPrecondition(!dataSendStream->hasTimedOut);
    HAPPrecondition(!dataSendStream->endOfStream);
    HAPPrecondition(scratchBytes);
    HAPPrecondition(!numPackets || packets);
    HAPPrecondition(completionHandler);

    HAPError err;

    int64_t startSequenceNumber = 0;
    int64_t endSequenceNumber = 0;
    bool isChunked = false;
    int64_t startChunkSequenceNumber = 0;
    int64_t endChunkSequenceNumber = 0;

    // Send dataSend.data event.
    HAPOPACKWriter messageWriter;
    HAPOPACKWriterCreate(&messageWriter, scratchBytes, numScratchBytes);
    if (true) {
        err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "streamId");
    }
    if (!err) {
        err = HAPOPACKWriterAppendInt(&messageWriter, dataSendStream->streamID);
    }
    if (!err) {
        err = HAPOPACKWriterAppendString(&messageWriter, "packets");
    }
    {
        if (!err) {
            err = HAPOPACKWriterAppendArrayBegin(&messageWriter);
        }
        for (size_t i = 0; i < numPackets; i++) {
            HAPDataSendDataStreamProtocolPacket* packet = &packets[i];
            HAPPrecondition(packet->data.bytes);
            if (!err) {
                err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
            }

            // Append metadata first to have more context when logs are truncated.
            if (!err) {
                err = HAPOPACKWriterAppendString(&messageWriter, "metadata");
            }
            {
                if (!err) {
                    err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
                }
                HAPPrecondition(packet->metadata.type == dataSendStream->type);

                switch (packet->metadata.type) {
                    case kHAPDataSendDataStreamProtocolType_Audio_Siri: {
                        if (packet->metadata._.audio.siri.rms < 0 || packet->metadata._.audio.siri.rms > 1) {
                            HAPLogError(
                                    &logObject,
                                    "[%p.%u] [%lld] %s: rms value %g must be in interval 0 ... 1 "
                                    "(calculated on samples in the range -1 ... 1).",
                                    (const void*) dispatcher,
                                    dataStream,
                                    (long long) dataSendStream->streamID,
                                    __func__,
                                    (double) packet->metadata._.audio.siri.rms);
                            HAPPreconditionFailure();
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "rms");
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendFloat(&messageWriter, (double) packet->metadata._.audio.siri.rms);
                        }

                        endSequenceNumber = packet->metadata._.audio.siri.sequenceNumber;
                        if (!i) {
                            startSequenceNumber = endSequenceNumber;
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "sequenceNumber");
                        }
                        HAPPrecondition(packet->metadata._.audio.siri.sequenceNumber >= 0);
                        if (!err) {
                            err = HAPOPACKWriterAppendInt(&messageWriter, packet->metadata._.audio.siri.sequenceNumber);
                        }
                        break;
                    }
                    case kHAPDataSendDataStreamProtocolType_IPCamera_Recording: {
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "dataType");
                        }
                        HAPPrecondition(HAPDataSendDataStreamProtocolIPCameraRecordingDataTypeIsValid(
                                packet->metadata._.ipCamera.recording.dataType));
                        switch (packet->metadata._.ipCamera.recording.dataType) {
                            case kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization: {
                                if (!err) {
                                    err = HAPOPACKWriterAppendString(&messageWriter, "mediaInitialization");
                                }
                                break;
                            }
                            case kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment: {
                                if (!err) {
                                    err = HAPOPACKWriterAppendString(&messageWriter, "mediaFragment");
                                }
                                break;
                            }
                        }

                        isChunked = true;
                        endSequenceNumber = packet->metadata._.audio.siri.sequenceNumber;
                        endChunkSequenceNumber = packet->metadata._.ipCamera.recording.dataChunkSequenceNumber;
                        if (!i) {
                            startSequenceNumber = endSequenceNumber;
                            startChunkSequenceNumber = endChunkSequenceNumber;
                        }

                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "dataSequenceNumber");
                        }
                        HAPPrecondition(packet->metadata._.ipCamera.recording.dataSequenceNumber >= 1);
                        if (!err) {
                            err = HAPOPACKWriterAppendInt(
                                    &messageWriter, packet->metadata._.ipCamera.recording.dataSequenceNumber);
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "isLastDataChunk");
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendBool(
                                    &messageWriter, packet->metadata._.ipCamera.recording.isLastDataChunk);
                        }
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "dataChunkSequenceNumber");
                        }
                        HAPPrecondition(packet->metadata._.ipCamera.recording.dataSequenceNumber >= 1);
                        if (!err) {
                            err = HAPOPACKWriterAppendInt(
                                    &messageWriter, packet->metadata._.ipCamera.recording.dataChunkSequenceNumber);
                        }
                        if (packet->metadata._.ipCamera.recording.dataTotalSize) {
                            if (!err) {
                                err = HAPOPACKWriterAppendString(&messageWriter, "dataTotalSize");
                            }
                            if (!err) {
                                err = HAPOPACKWriterAppendInt(
                                        &messageWriter, packet->metadata._.ipCamera.recording.dataTotalSize);
                            }
                        }
                        break;
                    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
                    case kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot: {
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "dataSequenceNumber");
                        }
                        HAPPrecondition(packet->metadata._.diagnostics.snapshot.dataSequenceNumber >= 1);
                        if (!err) {
                            err = HAPOPACKWriterAppendInt(
                                    &messageWriter, packet->metadata._.diagnostics.snapshot.dataSequenceNumber);
                        }
                        if (packet->metadata._.diagnostics.snapshot.dataSequenceNumber == 1 &&
                            packet->metadata._.diagnostics.snapshot.numUrlParameterPairs != 0) {
                            if (!err) {
                                err = HAPOPACKWriterAppendString(&messageWriter, "urlParameters");
                            }
                            if (!err) {
                                err = HAPOPACKWriterAppendDictionaryBegin(&messageWriter);
                            }
                            for (int32_t i = 0; i < packet->metadata._.diagnostics.snapshot.numUrlParameterPairs; i++) {
                                HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs* pair =
                                        &(packet->metadata._.diagnostics.snapshot.urlParameterPairs[i]);
                                HAPPrecondition(pair);
                                HAPPrecondition(pair->key);
                                HAPPrecondition(pair->value);
                                if (!err) {
                                    err = HAPOPACKWriterAppendString(&messageWriter, pair->key);
                                }
                                if (!err) {
                                    err = HAPOPACKWriterAppendString(&messageWriter, pair->value);
                                }
                            }
                            if (!err) {
                                err = HAPOPACKWriterAppendTerminator(&messageWriter);
                            }
                        }
                        break;
                    }
#endif // HAP_FEATURE_DIAGNOSTICS_SERVICE
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
                    case kHAPDataSendDataStreamProtocolType_Accessory_Metrics: {
                        if (!err) {
                            err = HAPOPACKWriterAppendString(&messageWriter, "metricSequenceNumber");
                        }
                        HAPPrecondition(packet->metadata._.metrics.accessoryMetrics.metricSequenceNumber >= 1);
                        if (!err) {
                            err = HAPOPACKWriterAppendInt(
                                    &messageWriter, packet->metadata._.metrics.accessoryMetrics.metricSequenceNumber);
                        }
                        break;
                    }
#endif
                }
                if (!err) {
                    err = HAPOPACKWriterAppendTerminator(&messageWriter);
                }
            }

            // Append data last so that start of packet contains more relevant data when it is logged.
            if (!err) {
                err = HAPOPACKWriterAppendString(&messageWriter, "data");
            }
            if (!err) {
                err = HAPOPACKWriterAppendData(&messageWriter, packet->data.bytes, packet->data.numBytes);
            }

            if (!err) {
                err = HAPOPACKWriterAppendTerminator(&messageWriter);
            }
        }
        if (!err) {
            err = HAPOPACKWriterAppendTerminator(&messageWriter);
        }
    }
    if (endOfStream) {
        if (!err) {
            err = HAPOPACKWriterAppendString(&messageWriter, "endOfStream");
        }
        if (!err) {
            err = HAPOPACKWriterAppendBool(&messageWriter, endOfStream);
        }
    }
    if (!err) {
        err = HAPOPACKWriterAppendTerminator(&messageWriter);
    }
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "[%p.%u] [%lld] %s: Scratch buffer too small.",
                (const void*) dispatcher,
                dataStream,
                (long long) dataSendStream->streamID,
                __func__);
        return err;
    }
    void* bytes;
    size_t numBytes;
    HAPOPACKWriterGetBuffer(&messageWriter, &bytes, &numBytes);

    if (!numPackets) {
        HAPLogDebug(
                &logObject,
                "[%p.%u] [%lld] Sending %s.%s event (%zu bytes).",
                (const void*) dispatcher,
                dataStream,
                (long long) dataSendStream->streamID,
                dataStreamProtocol->base->name,
                kHAPDataSendDataStreamProtocol_DataTopic,
                numBytes);
    } else if (
            endSequenceNumber == startSequenceNumber &&
            (!isChunked || startChunkSequenceNumber == endChunkSequenceNumber)) {
        if (isChunked) {
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] [%lld] Sending %s.%s event (seq: %lld/%lld, %zu bytes).",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID,
                    dataStreamProtocol->base->name,
                    kHAPDataSendDataStreamProtocol_DataTopic,
                    (long long) startSequenceNumber,
                    (long long) startChunkSequenceNumber,
                    numBytes);
        } else {
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] [%lld] Sending %s.%s event (seq: %lld, %zu bytes).",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID,
                    dataStreamProtocol->base->name,
                    kHAPDataSendDataStreamProtocol_DataTopic,
                    (long long) startSequenceNumber,
                    numBytes);
        }
    } else {
        if (isChunked) {
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] [%lld] Sending %s.%s event (seq: %lld/%lld - %lld/%lld, %zu bytes).",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID,
                    dataStreamProtocol->base->name,
                    kHAPDataSendDataStreamProtocol_DataTopic,
                    (long long) startSequenceNumber,
                    (long long) startChunkSequenceNumber,
                    (long long) endSequenceNumber,
                    (long long) endChunkSequenceNumber,
                    numBytes);
        } else {
            HAPLogDebug(
                    &logObject,
                    "[%p.%u] [%lld] Sending %s.%s event (seq: %lld - %lld, %zu bytes).",
                    (const void*) dispatcher,
                    dataStream,
                    (long long) dataSendStream->streamID,
                    dataStreamProtocol->base->name,
                    kHAPDataSendDataStreamProtocol_DataTopic,
                    (long long) startSequenceNumber,
                    (long long) endSequenceNumber,
                    numBytes);
        }
    }

    HAPAssert(dataSendStream->state == kHAPDataSendDataStreamProtocolStreamState_Idle);
    dataSendStream->endOfStream = endOfStream;
    dataSendStream->state = kHAPDataSendDataStreamProtocolStreamState_SendingData;
    dataSendStream->_.sendData.completionHandler = completionHandler;
    dataSendStream->_.sendData.scratchBytes = scratchBytes;
    dataSendStream->_.sendData.numScratchBytes = numScratchBytes;
    HAPDataStreamDispatcherSendMutableEvent(
            server,
            dispatcher,
            dataStreamProtocol,
            dataStream,
            &dataSendStream->_.sendData.transaction,
            kHAPDataSendDataStreamProtocol_DataTopic,
            bytes,
            numBytes,
            HandleSendDataEventComplete);
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

const HAPDataStreamProtocolBase kHAPDataSendDataStreamProtocol_Base = {
    .name = kHAPDataSendDataStreamProtocol_Name,
    .callbacks = { .handleAccept = HandleAccept,
                   .handleInvalidate = HandleInvalidate,
                   .handleEventAvailable = HandleEventAvailable,
                   .handleRequestAvailable = HandleRequestAvailable }
};

#endif
