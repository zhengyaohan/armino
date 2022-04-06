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

#ifndef HAP_DATA_STREAM_DISPATCHER_H
#define HAP_DATA_STREAM_DISPATCHER_H

#include "HAPDataStreamDispatcher+DataStreamProtocols.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * Base HomeKit Data Stream protocol handler.
 */
typedef struct {
    /** HomeKit Data Stream protocol handler base. */
    const HAPDataStreamProtocolBase* base;
} HAPBaseDataStreamProtocol;
HAP_STATIC_ASSERT(HAP_OFFSETOF(HAPBaseDataStreamProtocol, base) == 0, HAPBaseDataStreamProtocol_base);
HAP_NONNULL_SUPPORT(HAPBaseDataStreamProtocol)

/**
 * HomeKit Data Stream dispatcher.
 */
typedef struct _HAPDataStreamDispatcher {
    /** Accessory server. */
    HAPAccessoryServer* server;

    /** HomeKit Data Stream dispatcher storage. */
    const HAPDataStreamDispatcherStorage* storage;

    /** Timer to enforce transaction timeouts. */
    HAPPlatformTimerRef transactionTimer;

    /** Reference time for TTL values of HAPDataStreamSendRequestTransactions. */
    HAPTime referenceTime;
} HAPDataStreamDispatcher;

/**
 * HomeKit Data Stream transaction context type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamTransactionType) { /** HomeKit Data Stream send event transaction context. */
                                                        kHAPDataStreamTransactionType_SendEvent = 1,

                                                        /** HomeKit Data Stream send request transaction context. */
                                                        kHAPDataStreamTransactionType_SendRequest,

                                                        /** HomeKit Data Stream receive request transaction context. */
                                                        kHAPDataStreamTransactionType_ReceiveRequest
} HAP_ENUM_END(uint8_t, HAPDataStreamTransactionType);

/**
 * HomeKit Data Stream transaction context.
 *
 * - /!\ This must stay in sync across all transaction contexts.
 */
typedef struct {
    /** Next transaction in the linked list. */
    HAPDataStreamTransactionRef* _Nullable nextTransaction;

    struct {
        // 8-bit bitfield.
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
        uint8_t type : 2;                        /**< Transaction context type. See HAPDataStreamTransactionType. */
        uint8_t messageIsMutable : 1;            /**< Whether or not the message buffer is mutable. */
        HAP_DIAGNOSTIC_POP
    } _;

    /**@cond */
    uint8_t unusedPadding[3];
    /**@endcond */

    /** Length of message buffer. */
    uint32_t numMessageBytes;

    /** Message buffer. */
    union {
        const void* _Nullable bytes;  /**< Immutable reference to the message buffer. */
        void* _Nullable mutableBytes; /**< Mutable reference to the message buffer. */
    } message;

    /** Message completion handler. */
    HAPDataStreamSendEventCompletionHandler _Nullable messageCompletionHandler;

    /** HomeKit Data Stream protocol handler. */
    HAPDataStreamProtocol* dataStreamProtocol;

    /** Topic of the message. */
    const char* topic;
} HAPDataStreamBaseTransaction;

/**
 * HomeKit Data Stream send event transaction context state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamSendEventTransactionState) {
    /** Waiting for the event to start sending. */
    kHAPDataStreamSendEventTransactionState_WaitingForSend,

    /** Sending the event. */
    kHAPDataStreamSendEventTransactionState_SendingEvent
} HAP_ENUM_END(uint8_t, HAPDataStreamSendEventTransactionState);

/**
 * HomeKit Data Stream send event transaction context.
 */
typedef struct _HAPDataStreamSendEventTransaction {
    /** Next transaction in the linked list. */
    HAPDataStreamSendEventTransaction* _Nullable nextTransaction;

    struct {
        // 8-bit bitfield.
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
        uint8_t type : 2;                        /**< Must be kHAPDataStreamTransactionType_SendEvent. */
        uint8_t messageIsMutable : 1;            /**< Whether or not the message buffer is mutable. */
        HAP_DIAGNOSTIC_POP
    } _;

    /** Transaction state. */
    HAPDataStreamSendEventTransactionState state;

    /** Time when event must have been sent. Relative to dispatcher->referenceTime. */
    uint16_t ttl;

    /** Length of message buffer. */
    uint32_t numMessageBytes;

    /** Message buffer. */
    union {
        const void* _Nullable bytes;  /**< Immutable reference to the message buffer. */
        void* _Nullable mutableBytes; /**< Mutable reference to the message buffer. */
    } message;

    /** Message completion handler. */
    HAPDataStreamSendEventCompletionHandler _Nullable messageCompletionHandler;

    /** HomeKit Data Stream protocol handler. */
    HAPDataStreamProtocol* dataStreamProtocol;

    /** Topic of the message. */
    const char* topic;
} HAPDataStreamSendEventTransaction;
HAP_STATIC_ASSERT(
        sizeof(HAPDataStreamSendEventTransaction) >= sizeof(HAPDataStreamSendEventTransaction),
        HAPDataStreamSendEventTransaction);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, nextTransaction) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, nextTransaction),
        HAPDataStreamSendEventTransaction_nextTransaction);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, _) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, _),
        HAPDataStreamSendEventTransaction__);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, numMessageBytes) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, numMessageBytes),
        HAPDataStreamSendEventTransaction_numMessageBytes);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, message) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, message),
        HAPDataStreamSendEventTransaction_message);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, messageCompletionHandler) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, messageCompletionHandler),
        HAPDataStreamSendEventTransaction_messageCompletionHandler);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, dataStreamProtocol) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, dataStreamProtocol),
        HAPDataStreamSendEventTransaction_dataStreamProtocol);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendEventTransaction, topic) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, topic),
        HAPDataStreamSendEventTransaction_topic);

/**
 * HomeKit Data Stream send request transaction context state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamSendRequestTransactionState) {
    /** Waiting for the request to start sending. */
    kHAPDataStreamSendRequestTransactionState_WaitingForSend,

    /** Waiting for the request to finish sending. */
    kHAPDataStreamSendRequestTransactionState_SendingRequest,

    /** Waiting for the response. */
    kHAPDataStreamSendRequestTransactionState_WaitingForResponse,

    /** Received full response Header. Waiting for HomeKit Data Stream protocol handler to accept Message. */
    kHAPDataStreamSendRequestTransactionState_ResponseAvailable,

    /** Receiving the response. */
    kHAPDataStreamSendRequestTransactionState_ReceivingResponse
} HAP_ENUM_END(uint8_t, HAPDataStreamSendRequestTransactionState);

/**
 * HomeKit Data Stream send request transaction context.
 */
typedef struct _HAPDataStreamSendRequestTransaction {
    /** Next transaction in the linked list. */
    HAPDataStreamSendRequestTransaction* _Nullable nextTransaction;

    struct {
        // 8-bit bitfield.
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
        uint8_t type : 2;                        /**< Must be kHAPDataStreamTransactionType_SendRequest. */
        uint8_t messageIsMutable : 1;            /**< Whether or not the message buffer is mutable. */
        HAP_DIAGNOSTIC_POP
    } _;

    /** Transaction state. */
    HAPDataStreamSendRequestTransactionState state;

    /** Time when response must have been received. Relative to dispatcher->referenceTime. */
    uint16_t ttl;

    /** Length of message buffer. */
    uint32_t numMessageBytes;

    /** Message buffer. */
    union {
        const void* _Nullable bytes;  /**< Immutable reference to the message buffer. */
        void* _Nullable mutableBytes; /**< Mutable reference to the message buffer. */
    } message;

    /** Message completion handler. */
    HAPDataStreamSendRequestMessageCompletionHandler _Nullable messageCompletionHandler;

    /** HomeKit Data Stream protocol handler. */
    HAPDataStreamProtocol* dataStreamProtocol;

    /** Topic of the message. */
    const char* topic;

    /** A unique identifier to use to match to the response. */
    int64_t requestID;

    /** HomeKit Data Stream transaction context callbacks. */
    const HAPDataStreamSendRequestTransactionCallbacks* callbacks;
} HAPDataStreamSendRequestTransaction;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, nextTransaction) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, nextTransaction),
        HAPDataStreamSendRequestTransaction_nextTransaction);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, _) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, _),
        HAPDataStreamSendRequestTransaction__);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, numMessageBytes) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, numMessageBytes),
        HAPDataStreamSendRequestTransaction_numMessageBytes);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, message) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, message),
        HAPDataStreamSendRequestTransaction_message);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, messageCompletionHandler) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, messageCompletionHandler),
        HAPDataStreamSendRequestTransaction_messageCompletionHandler);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, dataStreamProtocol) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, dataStreamProtocol),
        HAPDataStreamSendRequestTransaction_dataStreamProtocol);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamSendRequestTransaction, topic) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, topic),
        HAPDataStreamSendRequestTransaction_topic);

/**
 * HomeKit Data Stream receive request transaction context state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamReceiveRequestTransactionState) {
    /** Receiving the request. */
    kHAPDataStreamReceiveRequestTransactionState_ReceivingRequest,

    /** Waiting for the response. */
    kHAPDataStreamReceiveRequestTransactionState_WaitingForResponse,

    /** Waiting for the response to start sending. */
    kHAPDataStreamReceiveRequestTransactionState_WaitingForSend,

    /** Sending the response. */
    kHAPDataStreamReceiveRequestTransactionState_SendingResponse
} HAP_ENUM_END(uint8_t, HAPDataStreamReceiveRequestTransactionState);

/**
 * HomeKit Data Stream receive request transaction context.
 */
typedef struct _HAPDataStreamReceiveRequestTransaction {
    /** Next transaction in the linked list. */
    HAPDataStreamSendRequestTransaction* _Nullable nextTransaction;

    struct {
        // 8-bit bitfield.
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
        uint8_t type : 2;                        /**< Must be kHAPDataStreamTransactionType_ReceiveRequest. */
        uint8_t messageIsMutable : 1;            /**< Whether or not the message buffer is mutable. */
        HAP_DIAGNOSTIC_POP
    } _;

    /** Transaction state. */
    HAPDataStreamReceiveRequestTransactionState state;

    /** Response status. */
    HAPDataStreamResponseStatus status;

    /**@cond */
    uint8_t unusedPadding[1];
    /**@endcond */

    /** Length of message buffer. */
    uint32_t numMessageBytes;

    /** Message buffer. */
    union {
        const void* _Nullable bytes;  /**< Immutable reference to the message buffer. */
        void* _Nullable mutableBytes; /**< Mutable reference to the message buffer. */
    } message;

    /** Message completion handler. */
    HAPDataStreamReceiveRequestMessageCompletionHandler _Nullable messageCompletionHandler;

    /** HomeKit Data Stream protocol handler. */
    HAPDataStreamProtocol* dataStreamProtocol;

    /** Topic of the message. */
    const char* topic;

    /** A unique identifier to use to match to the response. */
    int64_t requestID;

    /** HomeKit Data Stream receive request transaction context callbacks. */
    const HAPDataStreamReceiveRequestTransactionCallbacks* callbacks;
} HAPDataStreamReceiveRequestTransaction;
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, nextTransaction) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, nextTransaction),
        HAPDataStreamReceiveRequestTransaction_nextTransaction);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, _) == HAP_OFFSETOF(HAPDataStreamBaseTransaction, _),
        HAPDataStreamReceiveRequestTransaction__);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, numMessageBytes) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, numMessageBytes),
        HAPDataStreamReceiveRequestTransaction_numMessageBytes);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, message) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, message),
        HAPDataStreamReceiveRequestTransaction_message);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, messageCompletionHandler) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, messageCompletionHandler),
        HAPDataStreamReceiveRequestTransaction_messageCompletionHandler);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, dataStreamProtocol) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, dataStreamProtocol),
        HAPDataStreamReceiveRequestTransaction_dataStreamProtocol);
HAP_STATIC_ASSERT(
        HAP_OFFSETOF(HAPDataStreamReceiveRequestTransaction, topic) ==
                HAP_OFFSETOF(HAPDataStreamBaseTransaction, topic),
        HAPDataStreamReceiveRequestTransaction_topic);

/**
 * HomeKit Data Stream descriptor receive state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamDescriptorReceiveState) {
    /** Not receiving. */
    kHAPDataStreamDescriptorReceiveState_Idle,

    /** Receiving HeaderLen or Header. */
    kHAPDataStreamDescriptorReceiveState_ReceivingHeader,

    /** Received full event Header. Waiting for HomeKit Data Stream protocol handler to accept Message. */
    kHAPDataStreamDescriptorReceiveState_EventAvailable,

    /** Received full request Header. Waiting for HomeKit Data Stream protocol handler to accept Message. */
    kHAPDataStreamDescriptorReceiveState_RequestAvailable,

    /** Received full response Header. Waiting for HomeKit Data Stream protocol handler to accept Message. */
    kHAPDataStreamDescriptorReceiveState_ResponseAvailable,

    /** Receiving Message. */
    kHAPDataStreamDescriptorReceiveState_ReceivingMessage,

    /** Skipping request Message. */
    kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage,

    /** Waiting for the response to start sending (only used when a request is skipped). */
    kHAPDataStreamDescriptorReceiveState_WaitingForResponse,

    /** Waiting for the response to finish sending (only used when a request is skipped). */
    kHAPDataStreamDescriptorReceiveState_SendingResponse
} HAP_ENUM_END(uint8_t, HAPDataStreamDescriptorReceiveState);

/**
 * HomeKit Data Stream descriptor send state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataStreamDescriptorSendState) {
    /** Not sending. */
    kHAPDataStreamDescriptorSendState_Idle,

    /** Sending event. */
    kHAPDataStreamDescriptorSendState_SendingEvent,

    /** Sending request. */
    kHAPDataStreamDescriptorSendState_SendingRequest,

    /** Sending response. */
    kHAPDataStreamDescriptorSendState_SendingResponse,

    /** Sending response for skipped and replied to request. */
    kHAPDataStreamDescriptorSendState_SendingStatusOnlyResponse,
} HAP_ENUM_END(uint8_t, HAPDataStreamDescriptorSendState);

/**
 * HomeKit Data Stream descriptor.
 */
typedef struct _HAPDataStreamDescriptor {
    union {
        /** Information about incoming header. Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingHeader. */
        struct {
            uint8_t numBytes;         /**< Length of incoming header dictionary. */
            uint8_t bytes[UINT8_MAX]; /**< Buffer to store incoming header. */
        } header;

        /**
         * Information about received event.
         *
         * - Valid in state kHAPDataStreamDescriptorReceiveState_EventAvailable.
         * - Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingMessage if an event is received.
         */
        struct {
            /**
             * HomeKit Data Stream protocol handler that is handling the event.
             */
            HAPDataStreamProtocol* _Nullable dataStreamProtocol;

            /**
             * Completion handler of the HomeKit Data Stream event message operation.
             *
             * - Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingMessage.
             */
            HAPDataStreamReceiveEventMessageCompletionHandler completionHandler;

            /**
             * Buffer to receive short event messages. Completion handler processes the message synchronously.
             *
             * - Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingMessage.
             */
            uint8_t messageBytes[kHAPDataStreamDispatcher_NumShortMessageBytes];
        } event;

        /**
         * Information about received request.
         *
         * - Valid in state kHAPDataStreamDescriptorReceiveState_RequestAvailable.
         * - Valid in state kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage.
         * - Valid in state kHAPDataStreamDescriptorReceiveState_WaitingForResponse.
         * - Valid in state kHAPDataStreamDescriptorReceiveState_SendingResponse.
         */
        struct {
            /**
             * A unique identifier to use to match to the response.
             */
            int64_t requestID;

            /**
             * Completion handler of a HomeKit Data Stream skip and reply to request transaction.
             *
             * - Valid in state kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage.
             * - Valid in state kHAPDataStreamDescriptorReceiveState_WaitingForResponse.
             * - Valid in state kHAPDataStreamDescriptorReceiveState_SendingResponse.
             *
             * - Non-NULL if received request is skipped and replied to.
             */
            HAPDataStreamSkipAndReplyToRequestCompletionHandler _Nullable skipAndReplyToRequestCompletionHandler;

            /**
             * Response status code.
             *
             * - Valid in state kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage.
             * - Valid in state kHAPDataStreamDescriptorReceiveState_WaitingForResponse.
             * - Valid in state kHAPDataStreamDescriptorReceiveState_SendingResponse.
             */
            HAPDataStreamResponseStatus status;

            /**
             * Combined NULL-terminated protocol name and NULL-terminated topic.
             *
             * - We need to be able to form a response that fits within the header (response is longer than request).
             *   Protocol name and topic do not have explicit maximum lengths.
             *   However, the container has an explicit maximum total length of UINT8_MAX bytes.
             *   Also, protocol name and topic should always fit regardless of request ID.
             *
             *   Optimal OPACK encoding for a complete response Header:            Overhead:
             *   Dictionary with 4 entries: 0xE4                                   1 byte
             *                        "id": 0x42 'i' 'd'                           3 bytes
             *                  Request ID: Up to 9 bytes depending on encoding    9 bytes (worst case)
             *                    "status": 0x46 's' 't' 'a' 't' 'u' 's'           7 bytes
             *        Response status code: 1 byte (up to 39)                      1 byte
             *                  "response": 0x48 'r' 'e' 's' 'p' 'o' 'n' 's' 'e'   9 bytes
             *                       Topic: 1+ bytes + topic                       1+ bytes
             *                  "protocol": 0x48 'p' 'r' 'o' 't' 'o' 'c' 'o' 'l'   9 bytes
             *               Protocol name: 1+ bytes + protocol name               1+ bytes
             *                                                             Total: 41+ bytes
             *
             * - Optimal OPACK string encoding:
             *       Up to 32 bytes length: 1 byte + string
             *        Longer than 32 bytes: 0x6F + string + '\0'
             *
             *   => Combined length of protocol name + topic must not exceed:
             *      If both strings <= 32 bytes length: Maximum = UINT8_MAX - 41
             *         If one string > 32 bytes length: Maximum = UINT8_MAX - 42
             *       If both strings > 32 bytes length: Maximum = UINT8_MAX - 43
             *
             * - We need an additional 2 bytes for the NULL-terminators.
             *   Hence, maximum size of this array is UINT8_MAX - 41 + 2.
             */
            char nameAndTopic[UINT8_MAX - 41 + 2];
        } request;

        /**
         * Buffer to receive short messages. Completion handler processes the message synchronously.
         *
         * - Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingMessage if a short request is received.
         * - Valid in state kHAPDataStreamDescriptorReceiveState_ReceivingMessage if a short response is received.
         */
        uint8_t messageBytes[kHAPDataStreamDispatcher_NumShortMessageBytes];
    } _;

    /**
     * First HomeKit Data Stream transaction context.
     *
     * - A HomeKit Data Stream may have multiple transactions ongoing at a time.
     *   All transactions form a linked list.
     */
    HAPDataStreamTransactionRef* _Nullable firstTransaction;

    /** HomeKit Data Stream that this HomeKit Data Stream descriptor is linked to. */
    HAPDataStreamRef* _Nullable dataStream;

    /** Temporary buffer used while sending messages. */
    uint8_t outBytes[4]; // Size chosen to fit all potential messages and to fill padding within the struct.

    /** HomeKit Data Stream Protocol version of the connected peer. */
    double peerHdsVersion;

    /**
     * Context used for control messages.
     *
     * - Valid in state kHAPDataStreamDescriptorReceiveState_RequestAvailable.
     * - Valid in state kHAPDataStreamDescriptorReceiveState_SkippingRequestMessage.
     * - Valid in state kHAPDataStreamDescriptorReceiveState_WaitingForResponse.
     * - Valid in state kHAPDataStreamDescriptorReceiveState_SendingResponse.
     */
    struct {
        /** Receive request transaction context. */
        HAPDataStreamReceiveRequestTransaction rcvReqTransaction;

        /** Send request transaction context. */
        HAPDataStreamSendRequestTransaction sndReqTransaction;

        /** Buffer to store message payload. */
        uint8_t messageBytes[30];
    } controlContext;

    // 32-bit bitfield.
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
    uint32_t numDataBytes : 20;              /**< Remaining length of incoming data. */
    uint32_t hasPendingPacket : 1;           /**< Whether or not a packet is waiting to be received. */
    uint32_t receiveState : 4;               /**< Receive state. See HAPDataStreamDescriptorReceiveState. */
    uint32_t sendState : 3;                  /**< Send state. See HAPDataStreamDescriptorSendState. */
    uint32_t controlState : 2;               /**< "control" protocol state. See HAPControlDataStreamProtocolState. */
    HAP_DIAGNOSTIC_POP
} HAPDataStreamDescriptor;

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
