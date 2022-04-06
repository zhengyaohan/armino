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

#ifndef HAP_DATA_STREAM_PROTOCOLS_DATA_SEND_H
#define HAP_DATA_STREAM_PROTOCOLS_DATA_SEND_H

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
 * "dataSend" HomeKit Data Stream protocol name.
 */
#define kHAPDataSendDataStreamProtocol_Name "dataSend"

/**
 * "dataSend" stream type.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.1 Start
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolType) {
    /** Siri. */
    kHAPDataSendDataStreamProtocolType_Audio_Siri = 1,

    /** IP Camera recording. */
            kHAPDataSendDataStreamProtocolType_IPCamera_Recording = 2,
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
    /** Diagnostics snapshot. */
            kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot = 3,
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
    /** Accessory metrics. */
            kHAPDataSendDataStreamProtocolType_Accessory_Metrics = 4,
#endif
}
HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolType);

/**
 * "dataSend" reject reason.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.4 Close
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolRejectReason) {
    /** Not allowed to send this transfer (e.g., this stream type is not permitted). */
    kHAPDataSendDataStreamProtocolRejectReason_NotAllowed = 1,

    /** Transfer cannot be accepted right now (but normally it could have). */
    kHAPDataSendDataStreamProtocolRejectReason_Busy = 2,

    /** This stream type is not supported. */
    kHAPDataSendDataStreamProtocolRejectReason_Unsupported = 4,

    /** Some other protocol error occurred and the stream has failed. */
    kHAPDataSendDataStreamProtocolRejectReason_UnexpectedFailure = 5,

    /** Accessory not configured to perform the request. */
    kHAPDataSendDataStreamProtocolRejectReason_InvalidConfiguration = 9
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolRejectReason);

/**
 * "dataSend" cancellation reason.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.4 Close
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolCancellationReason) {
    /** Normal Close (before endOfStream). */
    kHAPDataSendDataStreamProtocolCancellationReason_Normal = 0,

    /** Not allowed to send this transfer (e.g., transfer was disabled). */
    kHAPDataSendDataStreamProtocolCancellationReason_NotAllowed = 1,

    /** The transfer will not be finished. */
    kHAPDataSendDataStreamProtocolCancellationReason_Canceled = 3,

    /** Some other protocol error occurred and the stream has failed. */
    kHAPDataSendDataStreamProtocolCancellationReason_UnexpectedFailure = 5,

    /** Failed to parse the data. */
    kHAPDataSendDataStreamProtocolCancellationReason_BadData = 7,

    /** Protocol error. */
    kHAPDataSendDataStreamProtocolCancellationReason_ProtocolError = 8,

    /** Accessory not configured to perform the request. */
    kHAPDataSendDataStreamProtocolCancellationReason_InvalidConfiguration = 9
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolCancellationReason);

/**
 * "dataSend" close reason.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 13.2.1.4 Close
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolCloseReason) {
    /** Normal Close (endOfStream has been acknowledged). */
    kHAPDataSendDataStreamProtocolCloseReason_Normal = 0,

    /** Not allowed to send this transfer (e.g., this stream type is not permitted). */
    kHAPDataSendDataStreamProtocolCloseReason_NotAllowed = 1,

    /** Transfer cannot be accepted right now (but normally it could have). */
    kHAPDataSendDataStreamProtocolCloseReason_Busy = 2,

    /** The transfer will not be finished (or Normal Close before endOfStream). */
    kHAPDataSendDataStreamProtocolCloseReason_Canceled = 3,

    /** This stream type is not supported. */
    kHAPDataSendDataStreamProtocolCloseReason_Unsupported = 4,

    /** Some other protocol error occurred and the stream has failed (or unknown close reason received). */
    kHAPDataSendDataStreamProtocolCloseReason_UnexpectedFailure = 5,

    /** Session could not be started. */
    kHAPDataSendDataStreamProtocolCloseReason_Timeout = 6,

    /** Failed to parse the data. */
    kHAPDataSendDataStreamProtocolCloseReason_BadData = 7,

    /** Protocol error. */
    kHAPDataSendDataStreamProtocolCloseReason_ProtocolError = 8,

    /** Accessory not configured to perform the request. */
    kHAPDataSendDataStreamProtocolCloseReason_InvalidConfiguration = 9
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolCloseReason);

/**
 * "dataSend" HomeKit Data Stream protocol handler context.
 */
struct _HAPDataSendDataStreamProtocolStream;
typedef struct {
    /**
     * First "dataSend" stream.
     *
     * - A HomeKit Data Stream may have multiple dataSend streams open at a time.
     *   All dataSend streams form a linked list.
     */
    struct _HAPDataSendDataStreamProtocolStream* _Nullable firstDataSendStream;
} HAPDataSendDataStreamProtocolContext;

/**
 * "dataSend" HomeKit Data Stream protocol handler listener.
 */
struct _HAPDataSendDataStreamProtocolListener;
typedef struct _HAPDataSendDataStreamProtocolListener HAPDataSendDataStreamProtocolListener;

/**
 * "dataSend" HomeKit Data Stream protocol handler base.
 */
extern const HAPDataStreamProtocolBase kHAPDataSendDataStreamProtocol_Base;

typedef struct HAPDataSendDataStreamProtocol HAPDataSendDataStreamProtocol;

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic")
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wgnu-empty-struct")
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wextern-c-compat")
/**
 * "dataSend" HomeKit Data Stream protocol open metadata.
 */
typedef struct {
    /**
     * Type of metadata.
     */
    HAPDataSendDataStreamProtocolType type;

    /** Type-specific metadata. */
    union {
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        /** Diagnostics. */
        union {
            /**
             * Snapshot.
             *
             * This metadata is provided by the controller when opening a data
             * stream to the accessory when making diagnostic snapshot request.
             */
            struct {

                /**
                 * The max diagnostics file size in bytes for the current HDS session.
                 */
                uint64_t maxLogSize;

                /**
                 * Specifies the diagnostics snapshot type.
                 */
                HAPDiagnosticsSnapshotType snapshotType;
            } snapshot;
        } diagnostics;
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
    } _;
} HAPDataSendDataStreamProtocolOpenMetadata;
HAP_DIAGNOSTIC_POP

/**
 * "dataSend" HomeKit Data Stream protocol callbacks to handle Stream Available.
 */
typedef struct {

    /**
     * Type of callback handler. Must match "dataSend" stream type that was specified when it was opened.
     */
    HAPDataSendDataStreamProtocolType type;

    /**
     * The callback used to handle incoming "dataSend" streams.
     *
     * - The "dataSend" stream must be accepted in a timely manner to avoid stalling the HomeKit Data Stream.
     *   Use HAPDataSendDataStreamProtocolAccept when ready to accept the "dataSend" stream.
     *
     * Example flow (accept):
     * - handleStreamAvailable callback.
     * - HAPDataSendDataStreamProtocolAccept.
     * - handleOpen callback on "dataSend" stream.
     * - HAPDataSendDataStreamProtocolSendData => completion handler.
     * - HAPDataSendDataStreamProtocolSendData => completion handler.
     * - HAPDataSendDataStreamProtocolSendData => completion handler.
     * - ...
     * - HAPDataSendDataStreamProtocolSendData (endOfStream: true) => completion handler,
     *   or HAPDataSendDataStreamProtocolCancel.
     * - handleClose callback on "dataSend" stream.
     *
     * Example flow (reject):
     * - handleStreamAvailable callback.
     * - HAPDataSendDataStreamProtocolReject.
     */
    void (*_Nullable handleStreamAvailable)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataSendDataStreamProtocol* dataStreamProtocol,
            const HAPServiceRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataSendDataStreamProtocolType type,
            HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
            void* _Nullable inDataSendStreamCallbacks,
            void* _Nullable context);

} HAPDataSendStreamProtocolStreamAvailableCallbacks;

/**
 * "dataSend" HomeKit Data Stream protocol handler.
 */
struct HAPDataSendDataStreamProtocol {
    /**
     * HomeKit Data Stream protocol handler base. Must refer to kHAPDataSendDataStreamProtocol_Base.
     */
    const HAPDataStreamProtocolBase* base;

    /**
     * "dataSend" HomeKit Data Stream protocol handler storage. Must be zero initialized.
     */
    struct {
        /**
         * Number of concurrent HomeKit Data Streams that the HomeKit Data Stream dispatcher supports
         * where this HomeKit Data Stream protocol handler is registered.
         */
        size_t numDataStreams;

        /**
         * Memory for HomeKit Data Stream protocol handler contexts. Must be zero-initialized.
         *
         * - One instance must be provided for each concurrently supported HomeKit Data Stream.
         */
        HAPDataSendDataStreamProtocolContext* protocolContexts;

        /**
         * Memory for HomeKit Data Stream protocol handler listeners. Must be zero-initialized.
         *
         * - One instance must be provided for each concurrently supported HomeKit Data Stream
         *   if "dataSend" streams opened by the controller are supported.
         *
         * - If "dataSend" streams are only opened by the accessory this can be set to NULL.
         */
        HAPDataSendDataStreamProtocolListener* _Nullable listeners;
    } storage;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle accepted HomeKit Data Streams.
         *
         * - To open a "dataSend" stream use HAPDataSendDataStreamProtocolOpen.
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
                HAPDataSendDataStreamProtocol* dataStreamProtocol,
                const HAPServiceRequest* request,
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
                HAPDataSendDataStreamProtocol* dataStreamProtocol,
                const HAPServiceRequest* request,
                HAPDataStreamHandle dataStream,
                void* _Nullable context);

        /**
         * Number of different Stream Available callbacks based on protocol type.
         */
        uint32_t numStreamAvailableCallbacks;

        /**
         * Stream Available callbacks.
         */
        const HAPDataSendStreamProtocolStreamAvailableCallbacks* streamAvailableCallbacks;
    } callbacks;
};
HAP_STATIC_ASSERT(HAP_OFFSETOF(HAPDataSendDataStreamProtocol, base) == 0, HAPDataSendDataStreamProtocol_base);

struct _HAPDataSendDataStreamProtocolStreamCallbacks;

/**
 * "dataSend" stream identifier.
 */
typedef int64_t HAPDataSendDataStreamProtocolStreamID;

/**
 * "dataSend" stream.
 */
struct _HAPDataSendDataStreamProtocolStream;
typedef struct _HAPDataSendDataStreamProtocolStream HAPDataSendDataStreamProtocolStream;
HAP_NONNULL_SUPPORT(HAPDataSendDataStreamProtocolStream)

/**
 * "dataSend" stream callbacks.
 */
typedef struct _HAPDataSendDataStreamProtocolStreamCallbacks {
    /**
     * The callback used when a "dataSend" stream has been closed.
     *
     * - /!\ WARNING: The "dataSend" stream must no longer be used
     *   after this callback returns.
     *
     * @param      server               Accessory server.
     * @param      dispatcher           HomeKit Data Stream dispatcher.
     * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      dataSendStream       "dataSend" stream.
     * @param      error                kHAPError_None           If successful.
     *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
     *                                                           No further operations may be started.
     *                                  kHAPError_InvalidData    If an unexpected message has been received.
     *                                  kHAPError_OutOfResources If out of resources to receive message.
     * @param      closeReason          Reason why "dataSend" stream was closed.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleClose)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataSendDataStreamProtocol* dataStreamProtocol,
            const HAPDataStreamRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataSendDataStreamProtocolStream* dataSendStream,
            HAPError error,
            HAPDataSendDataStreamProtocolCloseReason closeReason,
            void* _Nullable context);

    /**
     * The callback used to report that a "dataSend" stream has been opened.
     *
     * - Use HAPDataSendDataStreamProtocolSendData to send packets.
     *
     * @param      server               Accessory server.
     * @param      dispatcher           HomeKit Data Stream dispatcher.
     * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      dataSendStream       "dataSend" stream.
     * @param      callbacks            Callbacks supplied when opening the "dataSend" stream.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleOpen)(
            HAPAccessoryServer* server,
            HAPDataStreamDispatcher* dispatcher,
            HAPDataSendDataStreamProtocol* dataStreamProtocol,
            const HAPDataStreamRequest* request,
            HAPDataStreamHandle dataStream,
            HAPDataSendDataStreamProtocolStream* dataSendStream,
            void* _Nullable context);
} HAPDataSendDataStreamProtocolStreamCallbacks;

/**
 * Opens a "dataSend" stream.
 *
 * Example flow:
 * - HAPDataSendDataStreamProtocolOpenStream.
 * - handleOpen callback on "dataSend" stream.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - ...
 * - HAPDataSendDataStreamProtocolSendData (endOfStream: true) => completion handler,
 *   or HAPDataSendDataStreamProtocolCancel.
 * - handleClose callback on "dataSend" stream.
 *
 * If the open request is rejected the handleClose callback on "dataSend" stream is called.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param[out] dataSendStream       "dataSend" stream.
 * @param      type                 "dataSend" stream type.
 * @param      callbacks            "dataSend" stream callbacks. Must remain valid.
 */
void HAPDataSendDataStreamProtocolOpen(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPDataSendDataStreamProtocolType type,
        const HAPDataSendDataStreamProtocolStreamCallbacks* callbacks);

/**
 * Accepts an incoming "dataSend" stream.
 *
 * - This operation is asynchronous. Once the stream has been opened the handleOpen callback on the
 *   "dataSend" stream will be called.
 *
 * @param      server                           Accessory server.
 * @param      dispatcher                       HomeKit Data Stream dispatcher.
 * @param      dataStream                       HomeKit Data Stream.
 * @param[out] dataSendStream                   "dataSend" stream.
 * @param      callbacks                        "dataSend" stream callbacks. Must remain valid.
 */
void HAPDataSendDataStreamProtocolAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        const HAPDataSendDataStreamProtocolStreamCallbacks* callbacks);

/**
 * Rejects an incoming "dataSend" stream.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      rejectReason         Reason why "dataSend" stream was rejected.
 */
void HAPDataSendDataStreamProtocolReject(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolRejectReason rejectReason);

/**
 * Cancels a "dataSend" stream.
 *
 * - This operation is asynchronous. Once the stream has been canceled the handleClose callback on the
 *   "dataSend" stream will be called.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataSendStream       "dataSend" stream.
 */
void HAPDataSendDataStreamProtocolCancel(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream);

/**
 * Cancels a "dataSend" stream with a specific cancellation reason.
 *
 * - This operation is asynchronous. Once the stream has been canceled the handleClose callback on the
 *   "dataSend" stream will be called.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataSendStream       "dataSend" stream.
 * @param      cancellationReason   Reason why "dataSend" stream is canceled.
 */
void HAPDataSendDataStreamProtocolCancelWithReason(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPDataSendDataStreamProtocolCancellationReason cancellationReason);

/**
 * IP Camera recording: Data type streamed by the accessory.
 */
HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolIPCameraRecordingDataType) {
    /** Corresponds to the MP4 moov atom box. */
    kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaInitialization = 1,

    /** Corresponds to one or more MP4 <moof + mdat> boxes. */
    kHAPDataSendDataStreamProtocolIPCameraRecordingDataType_MediaFragment
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolIPCameraRecordingDataType);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
/**
 * Diagnostics metadata key value pairs for urlParameters.
 */
typedef struct {
    /** Key string. */
    char* key;

    /** Value string. */
    char* value;
} HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs;
#endif

/**
 * "dataSend" HomeKit Data Stream protocol handler packet.
 */
typedef struct {
    /**
     * Packet data.
     */
    struct {
        void* bytes;     /**< Buffer. */
        size_t numBytes; /**< Length of buffer. */
    } data;

    /**
     * Meta data for the packet.
     */
    struct {
        /**
         * Type of metadata. Must match "dataSend" stream type that was specified when it was opened.
         */
        HAPDataSendDataStreamProtocolType type;

        /** Type-specific metadata. */
        union {
            /** Audio. */
            union {
                /**
                 * Siri.
                 *
                 * - Used when "dataSend" stream was opened with kHAPDataSendDataStreamProtocolType_Audio_Siri.
                 */
                struct {
                    /**
                     * The RMS value is set for each Opus frame. The following formula computes the RMS value:
                     * RMS = sqrt((s_0^2 + s_1^2... + s_N-1^2) / number of samples) with the s_i in the range -1 ... +1
                     */
                    float rms;

                    /**
                     * Integer that starts at 0 and counts up in each frame. Frames must still be delivered in order
                     * without gaps.
                     */
                    int64_t sequenceNumber;
                } siri;

            } audio;

            /** IP Camera. */
            union {
                /**
                 * Recording.
                 *
                 * - Used when "dataSend" stream was opened with kHAPDataSendDataStreamProtocolType_IPCamera_Recording.
                 */
                struct {
                    /** Specifies the data type streamed by the accessory. */
                    HAPDataSendDataStreamProtocolIPCameraRecordingDataType dataType;

                    /**
                     * Uniquely identifies the payload data that has been segmented into chunks in the HDS session.
                     * The dataSequenceNumber for the first fragment number must be 1 and should increment by 1
                     * with each subsequent fragment.
                     */
                    int64_t dataSequenceNumber;

                    /**
                     * Indicates if this is the last chunk of data, content of the data chunk identified by
                     * the dataChunkSequenceNumber key value.
                     */
                    bool isLastDataChunk;

                    /**
                     * Specifies the data chunk for the data with identifier dataSequenceNumber.
                     * The dataChunkSequenceNumber for the first chunk must be 1 and should increment by 1
                     * with each subsequent data chunk.
                     */
                    int64_t dataChunkSequenceNumber;

                    /**
                     * Optional. 0 if not used.
                     * Specifies the total size in bytes of the transferred fragment.
                     * If possible, dataTotalSize should be sent whenever the dataChunkSequenceNumber is 1.
                     * If the dataTotalSize is not known when the dataChunkSequenceNumber is 1, it may be sent
                     * as part of a later data event. The dataTotalSize key should only be included a maximum of
                     * once per dataSequenceNumber.
                     */
                    int64_t dataTotalSize;
                } recording;
            } ipCamera;
#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
            /** Diagnostics. */
            union {
                /**
                 * Snapshot.
                 *
                 * - Used when "dataSend" stream was opened with
                 * kHAPDataSendDataStreamProtocolType_Diagnostics_Snapshot.
                 */
                struct {
                    /**
                     * Uniquely identifies the payload data that has been segmented into chunks in the HDS session.
                     * The dataSequenceNumber for the first fragment number must be 1 and should increment by 1
                     * with each subsequent fragment.
                     */
                    int64_t dataSequenceNumber;

                    /** Number of urlParameterPairs key-value pairs. */
                    int32_t numUrlParameterPairs;

                    /** Key value pairs corresponding to query string parameters for the urlParameterPairs. */
                    HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs* urlParameterPairs;
                } snapshot;
            } diagnostics;
#endif
#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)
            /** Metrics. */
            union {
                /**
                 * Accessory Metrics.
                 *
                 * - Used when "dataSend" stream was opened with
                 * kHAPDataSendDataStreamProtocolType_Accessory_Metrics.
                 */
                struct {
                    /**
                     * Uniquely identifies the metric that has been captured on the accessory.
                     */
                    int64_t metricSequenceNumber;
                } accessoryMetrics;
            } metrics;
#endif
        } _;
    } metadata;
} HAPDataSendDataStreamProtocolPacket;

/**
 * Completion handler of a "dataSend" stream send data operation.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataSendStream       "dataSend" stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      scratchBytes         Temporary buffer provided when starting the operation.
 * @param      numScratchBytes      Length of temporary buffer.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler)(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPDataStreamRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        HAPError error,
        void* scratchBytes,
        size_t numScratchBytes,
        void* _Nullable context);

/**
 * Sends data over a "dataSend" stream.
 *
 * - References to data buffers in the packets to send are treated as immutable references.
 *
 * - This function may only be called again once the previous completion handler is invoked.
 *
 * Example flow:
 * - handleOpen callback on "dataSend" stream.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - HAPDataSendDataStreamProtocolSendData => completion handler.
 * - ...
 * - HAPDataSendDataStreamProtocolSendData (endOfStream: true) => completion handler,
 *   or HAPDataSendDataStreamProtocolCancel.
 * - handleClose callback on "dataSend" stream.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataSendStream       "dataSend" stream.
 * @param      scratchBytes         Scratch buffer. Must remain valid.
 * @param      numScratchBytes      Length of scratch buffer.
 * @param      packets              Packets to send.
 * @param      numPackets           Number of packets to send.
 * @param      endOfStream          Indicates the final frame.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the scratch buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDataSendDataStreamProtocolSendData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolStream* dataSendStream,
        void* scratchBytes,
        size_t numScratchBytes,
        HAPDataSendDataStreamProtocolPacket* _Nullable packets,
        size_t numPackets,
        bool endOfStream,
        HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler completionHandler);

HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolListenerState) {
    /** Idle. */
    kHAPDataSendDataStreamProtocolListenerState_Idle,

    /** Receiving dataSend.open request. */
    kHAPDataSendDataStreamProtocolListenerState_ReceivingOpenRequest,

    /** Waiting for "dataSend" stream to be accepted. */
    kHAPDataSendDataStreamProtocolListenerState_WaitingForAccept,

    /** Sending dataSend.open response. */
    kHAPDataSendDataStreamProtocolListenerState_SendingOpenResponse
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolListenerState);

/**
 * "dataSend" HomeKit Data Stream protocol handler listener.
 */
typedef struct _HAPDataSendDataStreamProtocolListener {
    /** HomeKit Data Stream receive request transaction context. */
    HAPDataStreamReceiveRequestTransaction transaction;

    /** "dataSend" stream identifier. */
    HAPDataSendDataStreamProtocolStreamID streamID;

    /** Temporary buffer used to send messages. */
    uint8_t outBytes[13]; // Size chosen to fit all potential messages and to fill padding within the struct.

    /** "dataSend" stream type. */
    HAPDataSendDataStreamProtocolType type;

    /** State of ongoing operation. */
    HAPDataSendDataStreamProtocolListenerState state;

    bool hasPendingOpen : 1; /**< Whether a dataSend.open request is waiting to be received. */
} HAPDataSendDataStreamProtocolListener;

HAP_ENUM_BEGIN(uint8_t, HAPDataSendDataStreamProtocolStreamState) {
    /** Idle. */
    kHAPDataSendDataStreamProtocolStreamState_Idle,

    /** Opening "dataSend" stream. */
    kHAPDataSendDataStreamProtocolStreamState_Opening,

    /** Accepting "dataSend" stream. */
    kHAPDataSendDataStreamProtocolStreamState_Accepting,

    /** Sending data packets. */
    kHAPDataSendDataStreamProtocolStreamState_SendingData,

    /** Closing. */
    kHAPDataSendDataStreamProtocolStreamState_Closing
} HAP_ENUM_END(uint8_t, HAPDataSendDataStreamProtocolStreamState);

typedef struct _HAPDataSendDataStreamProtocolStream {
    /** Next "dataSend" stream. */
    struct _HAPDataSendDataStreamProtocolStream* _Nullable nextDataSendStream;

    /** "dataSend" stream callbacks. */
    const struct _HAPDataSendDataStreamProtocolStreamCallbacks* callbacks;

    /** "dataSend" stream identifier. */
    HAPDataSendDataStreamProtocolStreamID streamID;

    /** Details about ongoing operation. */
    union {
        /** Open state. */
        struct {
            /** HomeKit Data Stream send request transaction context. */
            HAPDataStreamSendRequestTransaction transaction;

            /** Time when open operation has been started. */
            HAPTime startTime;
        } open;

        /** SendData state. */
        struct {
            /** HomeKit Data Stream send event transaction context. */
            HAPDataStreamSendEventTransaction transaction;

            /** Completion handler of the SendData operation. */
            HAPDataSendDataStreamProtocolStreamSendDataCompletionHandler completionHandler;

            /** Temporary buffer provided when starting the operation. */
            void* scratchBytes;

            /** Length of temporary buffer. */
            size_t numScratchBytes;
        } sendData;

        /** Close state. */
        struct {
            /** HomeKit Data Stream send event transaction context. */
            HAPDataStreamSendEventTransaction transaction;
        } close;
    } _;

    /** Temporary buffer used to send messages. */
    uint8_t outBytes[43]; // Size chosen to fit all potential messages and to fill padding within the struct.

    /** "dataSend" stream type. */
    HAPDataSendDataStreamProtocolType type;

    /** State of ongoing operation. */
    HAPDataSendDataStreamProtocolStreamState state;

    /** Reason given by the controller why the stream has been closed. */
    HAPDataSendDataStreamProtocolCloseReason closeReason;

    /** Reason given by the user why the stream has been canceled. Only valid when wasCanceled is set. */
    HAPDataSendDataStreamProtocolCancellationReason cancellationReason;

    bool isOpen : 1;        /**< Whether the dataSend stream has been successfully opened. */
    bool isCancellable : 1; /**< Whether the dataSend stream can be cancelled */
    bool wasClosed : 1;     /**< Whether the dataSend stream has been closed by the controller. */
    bool wasCanceled : 1;   /**< Whether the dataSend stream has been canceled. */
    bool hasTimedOut : 1;   /**< Whether opening the dataSend stream has timed out. */
    bool endOfStream : 1;   /**< Whether the final frame has been indicated. */
    bool ackReceived : 1;   /**< Whether the final frame has been acknowledged. */
} HAPDataSendDataStreamProtocolStream;

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
