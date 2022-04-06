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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#ifndef HAP_DIAGNOSTICS_H
#define HAP_DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"
#include "HAPCircularQueue.h"
#include "HAPPlatformDiagnosticsLog.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)
#define kHAPDiagnostics_NumScratchBytes (500 * 1024)
#define kHAPDiagnostics_NumChunkBytes   (400 * 1024) // 400KB data chunk size for IP transport
#else
#define kHAPDiagnostics_NumScratchBytes (3 * 1024)
#define kHAPDiagnostics_NumChunkBytes   (2 * 1024)
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_FILE_STORAGE)

#define kHAPDiagnostics_NumChunkBytesHDSOverHAP (1 * 1024) // 1KB chunk size for BLE/Thread
#define kHAPDiagnostics_MaxDataSizeInMB         ((size_t) 5)
#define kHAPDiagnostics_MBToBytes               (1024 * 1024)
#define kHAPDiagnostics_GBToBytes               (kHAPDiagnostics_MBToBytes * 1024)
#define kHAPDiagnostics_1KiloBytes              (1024)
#define kHAPDiagnostics_PrepareDataTimeout      (15 * 60 * HAPSecond) // 15 minute timeout

/**
 * The diagnostics context associated with a data stream.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPDataStreamDispatcher* dispatcher;
    HAPDataStreamHandle dataStream;
    HAPDataSendDataStreamProtocolStream* dataSendStream;
    bool inProgress;
    HAPTransportType transportType;
} HAPDiagnosticsDataStreamContext;

/**
 * Diagnostics data event type
 */
HAP_ENUM_BEGIN(uint8_t, DataEventType) { kDiagnosticsDataEvent_Unknown,
                                         kDiagnosticsDataEvent_Cancel,
                                         kDiagnosticsDataEvent_Ready } HAP_ENUM_END(uint8_t, DataEventType);

/**
 * Diagnostics metadata key value pairs for urlParameters.
 */
typedef struct {
    /** Number of urlParameter key-value pairs. */
    int32_t numUrlParameterPairs;

    /** Key value pairs corresponding to query string parameters for the urlParameter. */
    HAPDataSendDataStreamProtocolPacketDiagnosticsMetadataKeyValuePairs* urlParameterPairs;
} HAPDiagnosticsUrlParameters;

/**
 * Diagnostics context structure
 */
typedef struct {
    /** The upload file end of stream flag. */
    bool isEOF;

    /** The folder name. */
    char folderName[PATH_MAX];

    /** Diagnostics log buffer*/
    uint8_t* _Nullable logBuffer;

    /** Diagnostics log buffer size*/
    size_t logBufferSizeBytes;

    /** Diagnostics circular queue for log buffer*/
    HAPCircularQueue logBufferQueue;

    /** Diagnostics log context*/
    DiagnosticsLog logContext;

    /** Diagnostics Data Stream context. */
    HAPDiagnosticsDataStreamContext dataStreamContext;

    /** Diagnostics Data Send Stream. */
    HAPDataSendDataStreamProtocolStream dataSendStream;

    /** Diagnostics Data Send scratch bytes. */
    uint8_t dataSendScratchBytes[kHAPDiagnostics_NumScratchBytes];

    /** Diagnostics Data Send open metadata. */
    HAPDataSendDataStreamProtocolOpenMetadata dataSendOpenMetadata;

    /** Diagnostics Data Send sequence number. */
    int64_t dataSequenceNumber;

    /** Diagnostics URL Parameters. */
    HAPDiagnosticsUrlParameters* _Nullable urlParameters;

    /** The total number of bytes for transfer. */
    int64_t totalTransferBytes;

    /** The data event type received from PAL. */
    DataEventType eventType;

    /** The context pointer provided by HAPPlatformDiagnosticsPrepareData. */
    void* _Nullable prepareDataContext;

    /** Abort diagnostics data timer. */
    HAPPlatformTimerRef abortDiagnosticsDataTimer;

    /** Selected diagnostics mode. */
    uint32_t* _Nullable diagnosticsSelectedMode;

    /** Accessory pointer. */
    const HAPAccessory* _Nullable accessory;

    /** Accessory platform pointer. */
    const HAPPlatform* _Nullable hapPlatform;
} HAPDiagnosticsContext;

/**
 * Set up capturing of HAP logs to file
 *
 * @param  folderName               Folder where HAP logs are stored.
 * @param  logFileName              HAP log file name
 * @param  maxLogFileSizeMB         Maximum size of HAP logs
 * @param  accessory                Accessory pointer
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 * @return kHAPError_InvalidData    If configuration data is invalid.
 * @return kHAPError_Busy           If log capture was already started.
 */
HAP_RESULT_USE_CHECK
HAPError HAPDiagnosticsSetupLogCaptureToFile(
        const char* folderName,
        const char* logFileName,
        const size_t maxLogFileSizeMB,
        const HAPAccessory* accessory);

/**
 * Stop capturing HAP log messages
 *
 * @param   accessory       Accessory pointer.
 */
void HAPDiagnosticsStopLogCaptureToFile(const HAPAccessory* accessory);

/**
 * Data stream available handler.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      type                 HomeKit Data Stream protocol type.
 * @param      metadata             HomeKit Data Send open metadata.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void HAPDiagnosticsHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata,
        void* _Nullable inDataSendStreamCallbacks,
        void* _Nullable context HAP_UNUSED);

/**
 * Diagnostics callbacks.
 */
typedef struct {
    /**
     * Callback used by PAL to inform HAP that diagnostics data is ready to transfer
     *
     * This asynchronous function will return immediately and will
     * schedule the HAPPlatformDiagnosticsGetBytesToUpload operation from
     * the main thread.
     *
     * @param  dataSizeBytes          Size in bytes of the diagnostics data.
     * @param  urlParameters          URL parameters
     *
     * @return kHAPError_None         If successful.
     * @return kHAPError_Unknown      If any other error.
     */
    HAPError (*handleDiagnosticsDataReady)(
            const size_t dataSizeBytes,
            HAPDiagnosticsUrlParameters* _Nullable urlParameters);

    /**
     * Cancels an active diagnostics dataSend stream if one exists.
     * To be called by PAL if diagnostics data collection cannot be
     * completed after a call to HAPPlatformDiagnosticsPrepareData.
     *
     * This asynchronous function will return immediately and will
     * schedule the cancel operation on the main thread.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If any other error.
     */
    HAPError (*handleDiagnosticsDataCancel)(void);
} HAPDiagnosticsCallbacks;
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
