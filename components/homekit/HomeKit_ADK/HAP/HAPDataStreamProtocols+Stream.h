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

#ifndef HAP_DATA_STREAM_PROTOCOLS_STREAM_H
#define HAP_DATA_STREAM_PROTOCOLS_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPDataStreamDispatcher.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * "stream" HomeKit Data Stream protocol name.
 */
#define kHAPStreamDataStreamProtocol_Name "stream"

/**
 * Stream receive message overhead.
 * Accounts for the OPACK encoding of the 'data' key in the event message.
 */
#define kHAPStreamDataStreamProtocol_RxMsgHeaderBytes 16

/**
 * Stream send message overhead.
 * Accounts for the OPACK encoding of the 'data' key and the event message header.
 */
#define kHAPStreamDataStreamProtocol_TxMsgHeaderBytes 72

/**
 * Default send queue depth for "stream" application protocols.
 */
#define kHAPStreamDataStreamProtocol_DefaultMaxSendsPerStream 1

/**
 * "stream" HomeKit Data Stream protocol application protocol context.
 */
typedef struct _HAPStreamDataStreamApplicationProtocolContext HAPStreamDataStreamApplicationProtocolContext;

/**
 * "stream" HomeKit Data Stream protocol application protocol send context.
 */
typedef struct _HAPStreamDataStreamApplicationProtocolSendContext HAPStreamDataStreamApplicationProtocolSendContext;

/**
 * "stream" HomeKit Data Stream protocol handler base.
 */
extern const HAPDataStreamProtocolBase kHAPStreamDataStreamProtocol_Base;

typedef struct HAPStreamDataStreamProtocol HAPStreamDataStreamProtocol;
typedef struct HAPStreamApplicationProtocol HAPStreamApplicationProtocol;

/**
 * "stream" base HomeKit Data Stream protocol handler.
 */
struct HAPStreamDataStreamProtocol {
    /**
     * HomeKit Data Stream protocol handler base. Must refer to kHAPStreamDataStreamProtocol_Base.
     */
    const HAPDataStreamProtocolBase* base;

    /**
     * Number of concurrent HomeKit Data Streams that the HomeKit Data Stream dispatcher supports
     * where this HomeKit Data Stream protocol handler is registered.
     */
    size_t numDataStreams;

    /**
     * Array of supported "stream" application protocol handlers. NULL-terminated.
     */
    HAPStreamApplicationProtocol* _Nullable const* _Nonnull applicationProtocols;
};
HAP_STATIC_ASSERT(HAP_OFFSETOF(HAPStreamDataStreamProtocol, base) == 0, HAPStreamDataStreamProtocol_base);

/**
 * Type for bitmask tracking of receive buffers.
 */
typedef uint32_t HAPStreamApplicationProtocolRxBufferTracker;
HAP_STATIC_ASSERT(
        kApp_NumDataStreams <= (sizeof(HAPStreamApplicationProtocolRxBufferTracker) * CHAR_BIT),
        HAPStreamApplicationProtocolRxBufferTracker);

/**
 * "stream" application protocol handler.
 */
struct HAPStreamApplicationProtocol {
    /**
     * The name of the application protocol.
     */
    const char* name;

    /**
     * Application protocol context.
     */
    void* _Nullable context;

    /**
     * "stream" application protocol handler storage. Must be zero initialized.
     */
    struct {
        /**
         * Memory for "stream" application protocol handler contexts. Must be zero-initialized.
         *
         * - One instance must be provided for each concurrently supported HomeKit Data Stream.
         */
        HAPStreamDataStreamApplicationProtocolContext* streamAppProtocolContexts;

        /**
         * Memory for "stream" application protocol send contexts. Must be zero-initialized.
         *
         * - Limits the number of simultaneous sends on the "stream" application protocol across all HomeKit Data
         *   Streams.
         */
        HAPStreamDataStreamApplicationProtocolSendContext* streamAppProtocolSendContexts;

        /**
         * Number of send contexts provided for the "stream" application protocol.
         */
        size_t numSendContexts;

        /**
         * Optional buffer(s) to receive application protocol messages. receiveData handler processes the message
         * synchronously.
         *
         * - If used, buffer space of rxBufferSize * numRxBuffers must be provided.
         * - If no buffers are provided for the application protocol, messages must be limited to
         *   kHAPDataStreamDispatcher_NumShortMessageBytes.
         */
        void* _Nullable rxBuffers;

        /**
         * Size of the optional buffer(s) used to receive application protocol messages.
         *
         * - Valid if rxBuffers is not NULL.
         * - If numRxBuffers > 1, rxBufferSize should provide alignment (e.g. multiple of 8 bytes).
         */
        size_t rxBufferSize;

        /**
         * Number of optional buffers used to receive application protocol messages.
         *
         * - Valid if rxBuffers is not NULL.
         */
        size_t numRxBuffers;

        /**
         * Allocation tracking of optional application protocol receive buffers.
         */
        struct {
            HAPStreamApplicationProtocolRxBufferTracker rxBufferIsAllocated;
        } _;
    } storage;

    /**
     * "stream" application protocol handler configuration.
     */
    struct {
        /**
         * Maximum number of send contexts a single HomeKit Data Stream may utilize.
         */
        size_t maxSendContextsPerDataStream;

        /**
         * Application protocol controller access.
         *
         * - Allows application protocols to be restricted to controllers with admin permissions.
         */
        bool requiresAdminPermissions;
    } config;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to notify "stream" application protocols of accepted HomeKit Data Streams.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      streamAppProtocol    Stream application protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleAccept)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPStreamApplicationProtocol* streamAppProtocol,
                const HAPServiceRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context);

        /**
         * The callback used when data is received for the application protocol.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      streamAppProtocol    Stream application protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         * @param      dataBytes            The buffer holding the received data.
         * @param      numDataBytes         The number of bytes in the buffer.
         */
        void (*_Nullable handleData)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPStreamApplicationProtocol* streamAppProtocol,
                const HAPServiceRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context,
                void* dataBytes,
                size_t numDataBytes);

        /**
         * The callback used to notify "stream" application protocols when a HomeKit Data Stream is invalidated.
         *
         * - /!\ WARNING: The "stream" application protocol must no longer be used after this callback returns.
         *
         * @param      server               Accessory server.
         * @param      dispatcher           HomeKit Data Stream dispatcher.
         * @param      streamAppProtocol    Stream application protocol handler.
         * @param      request              Request that originated the HomeKit Data Stream.
         * @param      dataStream           HomeKit Data Stream.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleInvalidate)(
                HAPAccessoryServer* server,
                HAPDataStreamDispatcher* dispatcher,
                HAPStreamApplicationProtocol* streamAppProtocol,
                const HAPServiceRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context);
    } callbacks;
};

/**
 * Completion handler of a sent "stream" data event operation.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      streamAppProtocol    Stream application protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      scratchBytes         Temporary buffer provided when starting the operation.
 * @param      numScratchBytes      Length of temporary buffer.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context);

/**
 * "stream" HomeKit Data Stream protocol application protocol send context.
 */
typedef struct _HAPStreamDataStreamApplicationProtocolSendContext {
    /** Next send context. */
    struct _HAPStreamDataStreamApplicationProtocolSendContext* _Nullable nextSendContext;

    /** HomeKit Data Stream send event transaction context. */
    HAPDataStreamSendEventTransaction transaction;

    /** Completion handler of the send data event operation. */
    HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler completionHandler;

    /** Temporary buffer provided when starting the operation. */
    void* scratchBytes;

    /** Length of temporary buffer. */
    size_t numScratchBytes;

    /** Whether the context has been allocated to a stream. */
    bool inUse;
} HAPStreamDataStreamApplicationProtocolSendContext;

/**
 * "stream" HomeKit Data Stream protocol application protocol context.
 */
typedef struct _HAPStreamDataStreamApplicationProtocolContext {
    /** DataStream context. */
    struct {
        /** HomeKit DataStream session opened by admin controller. */
        bool isAdmin;
    } dataStreamContext;

    /** List of send contexts. */
    HAPStreamDataStreamApplicationProtocolSendContext* _Nullable firstSendContext;

    /** Receive state. */
    struct {
        /** Event message available. */
        bool eventAvail;

        /** Event receive application protocol buffer. */
        void* _Nullable buffer;
    } receive;
} HAPStreamDataStreamApplicationProtocolContext;

/**
 * Sends application protocol data over "stream" protocol.
 *
 * - Reference to data buffer is treated as immutable reference.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      streamAppProtocol    Stream application protocol handler.
 * @param      dataStream           HomeKit Data Stream.
 * @param      scratchBytes         Scratch buffer. Must remain valid.
 * @param      numScratchBytes      Length of scratch buffer.
 * @param      dataBytes            Data buffer to send.
 * @param      numDataBytes         Length of data buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 *
 * @return kHAPError_None                 If successful.
 * @return kHAPError_OutOfResources       If the scratch buffer is not large enough.
 */
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
        HAPStreamDataStreamApplicationProtocolDataSendCompletionHandler completionHandler);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
