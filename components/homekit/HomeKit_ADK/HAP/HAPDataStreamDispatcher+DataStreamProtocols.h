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

#ifndef HAP_DATA_STREAM_DISPATCHER_DATA_STREAM_PROTOCOLS_H
#define HAP_DATA_STREAM_DISPATCHER_DATA_STREAM_PROTOCOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPDataStreamProtocols.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * @defgroup HAPDataStreamProtocol APIs for use by HomeKit Data Stream protocol handlers.
 * @{
 */

/**
 * Finds a HomeKit Data Stream protocol handler for a given protocol name.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      protocolName         Name of the protocol.
 *
 * @return HomeKit Data Stream protocol handler for the protocol with the given name, if supported. NULL, otherwise.
 */
HAP_RESULT_USE_CHECK
HAPDataStreamProtocol* _Nullable HAPDataStreamDispatcherFindHandlerForProtocolName(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        const char* protocolName);

/**
 * HomeKit Data Stream protocol handler base.
 *
 * - A pointer to an initialized instance of this structure must be the first element of a concrete
 *   HAPDataStreamProtocol. Additional data after the HAPDataStreamProtocolBase is considered opaque by the HomeKit Data
 *   Stream dispatcher.
 *
 * **Example**

   @code{.c}

   typedef const HAPDataStreamProtocolBase kHAPFooDataStreamProtocol_Base;
   typedef struct {
       \/\*\*
        \* HomeKit Data Stream protocol handler base. Must refer to kHAPFooDataStreamProtocol_Base.
        \*\/
       const HAPDataStreamProtocolBase *base;
   } HAPFooDataStreamProtocol;
   HAP_STATIC_ASSERT(HAP_OFFSETOF(HAPFooDataStreamProtocol, base) == 0, HAPFooDataStreamProtocol_base);

   @endcode
 */
struct HAPDataStreamProtocolBase {
    /**
     * The name of the protocol.
     */
    const char* name;

    /**
     * HomeKit Data Stream dispatcher callbacks.
     *
     * - These callbacks will be invoked by the HomeKit Data Stream dispatcher to exchange messages specific to the
     *   protocol. A HomeKit Data Stream protocol is identified by its name. Within the context of a HomeKit Data Stream
     *   dispatcher registered protocol handlers must support protocols with different names.
     */
    struct {
        /**
         * The callback used to handle accepted HomeKit Data Streams.
         *
         * - If multiple protocols are registered the order in which the various handleAccept callbacks are invoked
         *   is undefined. Do not assume that other related protocols were already informed.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleAccept)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPDataStreamProtocol* dataStreamProtocol,
                const HAPDataStreamRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context);

        /**
         * The callback used when a HomeKit Data Stream is invalidated.
         *
         * - /!\ WARNING: The HomeKit Data Stream must no longer be used after this callback returns.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleInvalidate)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPDataStreamProtocol* dataStreamProtocol,
                const HAPDataStreamRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context);

        /**
         * The callback used to handle incoming event messages.
         *
         * - The message must be received or skipped in a timely manner to avoid stalling the HomeKit Data Stream.
         *   Use HAPDataStreamDispatcherReceiveEvent when ready to receive the message.
         *
         * Example flow:
         * - handleEvent callback.
         * - HAPDataStreamDispatcherReceiveEvent / HAPDataStreamDispatcherSkipEvent => completion handler.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      topic                The topic of the incoming message.
         * @param      numMessageBytes      The length of the incoming message.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*handleEventAvailable)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPDataStreamProtocol* dataStreamProtocol,
                const HAPDataStreamRequest* request,
                HAPDataStreamHandle dataStream,
                const char* topic,
                size_t numMessageBytes,
                void* _Nullable context);

        /**
         * The callback used to handle incoming request messages.
         *
         * - The message must be accepted or ignored in a timely manner to avoid stalling the HomeKit Data Stream.
         *   Use HAPDataStreamDispatcherReceiveRequest when ready to accept the message.
         *
         * - No new requests may be received until the previous request has been fully received or ignored.
         *   However, after a request has been received, additional concurrent requests may arrive before
         *   the previous request has been responded to.
         *
         * - /!\ The request metadata may only be used as guidelines but must not modify critical state.
         *   Encryption is only validated once the request has been fully received!
         *
         * Example flow:
         * - handleRequestAvailable callback.
         * - HAPDataStreamDispatcherReceiveRequest / HAPDataStreamDispatcherSkipRequest => completion handler.
         * - HAPDataStreamDispatcherSendMutableResponse => completion handler.
         * - handleComplete callback on HomeKit Data Stream receive request transaction context.
         *
         * If the HomeKit Data Stream is invalidated all pending completion handlers will be invoked with an error.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      topic                The topic of the incoming message.
         * @param      numMessageBytes      The length of the incoming message.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*handleRequestAvailable)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPDataStreamProtocol* dataStreamProtocol,
                const HAPDataStreamRequest* request,
                HAPDataStreamHandle dataStream,
                const char* topic,
                size_t numMessageBytes,
                void* _Nullable context);
    } callbacks;
};

/**
 * Length of the shared buffer that may be used to receive short messages.
 */
#define kHAPDataStreamDispatcher_NumShortMessageBytes ((size_t) 200)

/**
 * HomeKit Data Stream response status codes.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.3.4 Response Messages
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamResponseStatus) {
    /** Success. */
    kHAPDataStreamResponseStatus_Success = 0x00,

    /** Out Of Memory; Resource exhaustion, out of memory. */
    kHAPDataStreamResponseStatus_OutOfMemory = 0x01,

    /** Timeout. */
    kHAPDataStreamResponseStatus_Timeout = 0x02,

    /** Header error; Error in header dictionary. */
    kHAPDataStreamResponseStatus_HeaderError = 0x03,

    /** Payload error; Error in payload dictionary. */
    kHAPDataStreamResponseStatus_PayloadError = 0x04,

    /** Missing protocol; Protocol does not exist or is not active. */
    kHAPDataStreamResponseStatus_MissingProtocol = 0x05,

    /**
     * Protocol-specific error. If this value is returned in the response header,
     * the accessory may specify a protocol-specific error in the response payload.
     */
    kHAPDataStreamResponseStatus_ProtocolSpecificError = 0x06
} HAP_ENUM_END(uint8_t, HAPDataStreamResponseStatus);

/**
 * HomeKit Data Stream transaction context.
 */
typedef void HAPDataStreamTransactionRef;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Completion handler of a HomeKit Data Stream receive event message operation.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      messageBytes         Message buffer provided when starting the HomeKit Data Stream operation.
 * @param      numMessageBytes      Length of message buffer.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamReceiveEventMessageCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        void* _Nullable context);

/**
 * Receives an incoming HomeKit Data Stream event message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      messageBytes         Message buffer to fill with incoming message. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler);

/**
 * Skips an incoming HomeKit Data Stream event message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSkipEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler);

/**
 * Receives an incoming HomeKit Data Stream event message into a shared buffer.
 *
 * - /!\ The completion handler must process the message synchronously as the shared buffer will be reused.
 *
 * - /!\ If the message is longer than kHAPDataStreamDispatcher_NumShortMessageBytes it will be skipped
 *   and the completion handler will receive a NULL value for messageBytes.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveShortEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveEventMessageCompletionHandler completionHandler);

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit Data Stream send event transaction context.
 */
struct _HAPDataStreamSendEventTransaction;
typedef struct _HAPDataStreamSendEventTransaction HAPDataStreamSendEventTransaction;
HAP_NONNULL_SUPPORT(HAPDataStreamSendEventTransaction)

/**
 * Completion handler of a HomeKit Data Stream send event transaction.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      messageBytes         Message buffer provided when starting the HomeKit Data Stream operation.
 * @param      numMessageBytes      Length of message buffer.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamSendEventCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        void* _Nullable context);

/**
 * Sends a HomeKit Data Stream event message.
 *
 * - This function is less efficient than the HAPDataStreamDispatcherSendMutableEvent function.
 *   If possible, use the latter.
 *
 * Example flow:
 * - HAPDataStreamDispatcherSendEvent => completion handler.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid.
 * @param      messageBytes         Message buffer. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        const char* topic,
        const void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendEventCompletionHandler completionHandler);

/**
 * Sends a HomeKit Data Stream event message.
 *
 * Example flow:
 * - HAPDataStreamDispatcherSendMutableEvent => completion handler.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid.
 * @param      messageBytes         Message buffer. Will be modified. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendMutableEvent(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendEventTransaction* transaction,
        const char* topic,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendEventCompletionHandler completionHandler);

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit Data Stream receive request transaction context.
 */
struct _HAPDataStreamReceiveRequestTransaction;
typedef struct _HAPDataStreamReceiveRequestTransaction HAPDataStreamReceiveRequestTransaction;
HAP_NONNULL_SUPPORT(HAPDataStreamReceiveRequestTransaction)

/**
 * HomeKit Data Stream receive request transaction context callbacks.
 */
typedef struct {
    /**
     * The callback used when a HomeKit Data Stream receive request transaction is invalidated before completion.
     *
     * - /!\ WARNING: The transaction must no longer be used after this callback returns.
     *
     * @param      server               Accessory server.
     * @param      dispatcher           HomeKit Data Stream dispatcher.
     * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      transaction          HomeKit Data Stream transaction context.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleInvalidate)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataStreamProtocol* dataStreamProtocol,
            const HAPDataStreamRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataStreamReceiveRequestTransaction* transaction,
            void* _Nullable context);
} HAPDataStreamReceiveRequestTransactionCallbacks;

/**
 * Completion handler of a HomeKit Data Stream receive request transaction's message operation.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      messageBytes         Message buffer provided when starting the HomeKit Data Stream operation.
 * @param      numMessageBytes      Length of message buffer.
 * @param      isComplete           An indication that the HomeKit Data Stream transaction has successfully completed.
 *                                  No further callbacks will be invoked on its context (including handleInvalidate).
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamReceiveRequestMessageCompletionHandler)(
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
        void* _Nullable context);

/**
 * Receives an incoming HomeKit Data Stream request message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      callbacks            HomeKit Data Stream transaction context callbacks. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid. The topic parameter from the
 *                                  handleRequestAvailable callback cannot be used as it won't stay valid.
 * @param      messageBytes         Message buffer to fill with incoming message. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler);

/**
 * Skips an incoming HomeKit Data Stream request message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      callbacks            HomeKit Data Stream transaction context callbacks. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid. The topic parameter from the
 *                                  handleRequestAvailable callback cannot be used as it won't stay valid.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSkipRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler);

/**
 * Receives an incoming HomeKit Data Stream request message into a shared buffer.
 *
 * - /!\ The completion handler must process the message synchronously as the shared buffer will be reused.
 *
 * - /!\ If the message is longer than kHAPDataStreamDispatcher_NumShortMessageBytes it will be skipped
 *   and the completion handler will receive a NULL value for messageBytes.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      callbacks            HomeKit Data Stream transaction context callbacks. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid. The topic parameter from the
 *                                  handleRequestAvailable callback cannot be used as it won't stay valid.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveShortRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks,
        const char* topic,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler);

/**
 * Sends a HomeKit Data Stream response message.
 *
 * - This function is less efficient than the HAPDataStreamDispatcherSendMutableResponse function.
 *   If possible, use the latter.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      status               Status code to send in the response message.
 * @param      messageBytes         Message buffer. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPDataStreamResponseStatus status,
        const void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler);

/**
 * Sends a mutable HomeKit Data Stream response message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      status               Status code to send in the response message.
 * @param      messageBytes         Message buffer. Will be modified. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendMutableResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamReceiveRequestTransaction* transaction,
        HAPDataStreamResponseStatus status,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamReceiveRequestMessageCompletionHandler completionHandler);

/**
 * Completion handler of a HomeKit Data Stream skip and reply to request transaction.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamSkipAndReplyToRequestCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* _Nullable context);

/**
 * Skips an incoming HomeKit Data Stream request message and sends an empty response with a given status code.
 *
 * - /!\ This function should only be used if no HomeKit Data Stream receive request transaction context is available.
 *   Until the completion handler is called, no other requests may be received, even by other protocols!
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      status               Status code to send in the response message.
 * @param      completionHandler    Completion handler to call when the transaction is complete.
 */
void HAPDataStreamDispatcherSkipAndReplyToRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamResponseStatus status,
        HAPDataStreamSkipAndReplyToRequestCompletionHandler completionHandler);

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit Data Stream send request transaction context.
 */
struct _HAPDataStreamSendRequestTransaction;
typedef struct _HAPDataStreamSendRequestTransaction HAPDataStreamSendRequestTransaction;
HAP_NONNULL_SUPPORT(HAPDataStreamSendRequestTransaction)

/**
 * HomeKit Data Stream send request transaction context callbacks.
 */
typedef struct {
    /**
     * The callback used when a HomeKit Data Stream send request transaction is invalidated before completion.
     *
     * - /!\ WARNING: The transaction must no longer be used after this callback returns.
     *
     * @param      server               Accessory server.
     * @param      dispatcher           HomeKit Data Stream dispatcher.
     * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      transaction          HomeKit Data Stream transaction context.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleInvalidate)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataStreamProtocol* dataStreamProtocol,
            const HAPDataStreamRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataStreamSendRequestTransaction* transaction,
            void* _Nullable context);

    /**
     * The callback used to handle an incoming response message.
     *
     * - The message must be received in a timely manner to avoid stalling the HomeKit Data Stream.
     *   Use HAPDataStreamDispatcherReceiveResponse when ready to receive the message.
     *
     * @param      server               Accessory server.
     * @param      dispatcher           HomeKit Data Stream dispatcher.
     * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      transaction          HomeKit Data Stream transaction context.
     * @param      topic                The topic of the incoming message.
     * @param      status               HomeKit Data Stream response status code.
     * @param      numMessageBytes      The length of the incoming message.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleResponseAvailable)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataStreamProtocol* dataStreamProtocol,
            const HAPDataStreamRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataStreamSendRequestTransaction* transaction,
            const char* topic,
            HAPDataStreamResponseStatus status,
            size_t numMessageBytes,
            void* _Nullable context);
} HAPDataStreamSendRequestTransactionCallbacks;

/**
 * Completion handler of a HomeKit Data Stream send request transaction's message operation.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      messageBytes         Message buffer provided when starting the HomeKit Data Stream operation.
 * @param      numMessageBytes      Length of message buffer.
 * @param      isComplete           An indication that the HomeKit Data Stream transaction has successfully completed.
 *                                  No further callbacks will be invoked on its context (including handleInvalidate).
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamSendRequestMessageCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* _Nullable dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPError error,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        bool isComplete,
        void* _Nullable context);

/**
 * Starts a HomeKit Data Stream request transaction.
 *
 * - This function is less efficient than the HAPDataStreamDispatcherSendMutableRequest function.
 *   If possible, use the latter.
 *
 * Example flow:
 * - HAPDataStreamDispatcherSendMutableRequest => completion handler.
 * - handleResponse callback on HomeKit Data Stream send request transaction context.
 * - HAPDataStreamDispatcherReceiveResponse / HAPDataStreamDispatcherSkipResponse => completion handler (isComplete).
 *
 * - If the transaction invalidates before completion the handleInvalidate transaction context callback is called.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      callbacks            HomeKit Data Stream transaction context callbacks. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid.
 * @param      messageBytes         Message buffer. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        const HAPDataStreamSendRequestTransactionCallbacks* callbacks,
        const char* topic,
        const void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler);

/**
 * Starts a HomeKit Data Stream request transaction.
 *
 * Example flow:
 * - HAPDataStreamDispatcherSendMutableRequest => completion handler.
 * - handleResponse callback on HomeKit Data Stream send request transaction context.
 * - HAPDataStreamDispatcherReceiveResponse / HAPDataStreamDispatcherSkipResponse => completion handler (isComplete).
 *
 * - If the transaction invalidates before completion the handleInvalidate transaction context callback is called.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      callbacks            HomeKit Data Stream transaction context callbacks. Must remain valid.
 * @param      topic                Topic of the message. Must remain valid.
 * @param      messageBytes         Message buffer. Will be modified. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSendMutableRequest(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamProtocol* dataStreamProtocol,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        const HAPDataStreamSendRequestTransactionCallbacks* callbacks,
        const char* topic,
        void* _Nullable messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler);

/**
 * Receives an incoming HomeKit Data Stream response message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      messageBytes         Message buffer to fill with incoming message. Must remain valid.
 * @param      numMessageBytes      Length of message buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        void* messageBytes,
        size_t numMessageBytes,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler);

/**
 * Skips an incoming HomeKit Data Stream response message.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherSkipResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler);

/**
 * Receives an incoming HomeKit Data Stream response message into a shared buffer.
 *
 * - /!\ The completion handler must process the message synchronously as the shared buffer will be reused.
 *
 * - /!\ If the message is longer than kHAPDataStreamDispatcher_NumShortMessageBytes it will be skipped
 *   and the completion handler will receive a NULL value for messageBytes.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          HomeKit Data Stream transaction context. Must remain valid.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 */
void HAPDataStreamDispatcherReceiveShortResponse(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamSendRequestTransaction* transaction,
        HAPDataStreamSendRequestMessageCompletionHandler completionHandler);

/**
 * Sets the data stream minimum priority.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      priority             Minimum priority of the stream.
 */
void HAPDataStreamDispatcherSetMinimumPriority(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPStreamPriority priority);

/**
 * Clear a pending transaction if any, with no trailing actions.
 * This function is designed to be used by protocol implementations to clear
 * pending transactions when the data stream is being invalidated.
 * Hence, recursive action should not occur from this function call.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      transaction          transaction to delete.<br>
 *                                  If the transaction is not pending, this function doesn't do anything.
 */
void HAPDataStreamDispatcherClearTransaction(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataStreamTransactionRef* transaction);

/**@}*/

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
