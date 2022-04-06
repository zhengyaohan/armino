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

#ifndef HAP_METRICS_H
#define HAP_METRICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"
#include "HAPMetricsEvent.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)

#define kHAPMetrics_MaxNumMetrics   100 // Maximum number of unique metric types
#define kHAPMetrics_NumScratchBytes (3 * 1024)
#define kHAPMetrics_MaxEventBytes   (1 * 64)
#ifndef HAP_METRICS_MAX_STORED_EVENTS
#define HAP_METRICS_MAX_STORED_EVENTS (50)
#endif
#define kHAPMetrics_MaxStoredEvents HAP_METRICS_MAX_STORED_EVENTS
#define kHAPMetrics_SendInterval    ((HAPTime) 60 * HAPSecond)

/** Supported metrics status */
HAP_ENUM_BEGIN(uint8_t, HAPMetricsSupportedMetricsStatus) {
    kHAPMetricsSupportedMetricsStatus_Disabled = 0x00,
    kHAPMetricsSupportedMetricsStatus_Enabled = 0x01
} HAP_ENUM_END(uint8_t, HAPMetricsSupportedMetricsStatus);

/** Supported metrics configuration */
typedef struct {
    HAPMetricsID metricID;
    HAPMetricsSupportedMetricsStatus status;
} HAPMetricsConfig;

/** Supported metrics */
typedef struct {
    int numMetrics;
    HAPMetricsConfig config[kHAPMetrics_MaxNumMetrics];
} HAPMetricsSupportedMetrics;

/**
 * The metrics context associated with a data stream.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPDataStreamDispatcher* dispatcher;
    HAPDataStreamHandle dataStream;
    HAPDataSendDataStreamProtocolStream* dataSendStream;
    bool inProgress;
} HAPMetricsDataStreamContext;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
/**
 * The metrics event buffers.
 */
typedef struct {
    uint8_t buffer[kHAPMetrics_MaxEventBytes];
    bool isUsed;
} HAPMetricsEventBuffers;
#endif

typedef void (*HAPMetricsBufferStateCallback)(bool isBufferFull);
typedef void (*HAPMetricsSaveAccessoryStateCallback)(void);

/**
 * Metrics context structure
 */
typedef struct {
    /** Metrics Data Stream context. */
    HAPMetricsDataStreamContext dataStreamContext;

    /** Metrics Data Send Stream. */
    HAPDataSendDataStreamProtocolStream dataSendStream;

    /** Metrics Data Send scratch bytes. */
    uint8_t dataSendScratchBytes[kHAPMetrics_NumScratchBytes * kHAPMetrics_MaxNumMetrics];

    /** Supported metrics configuration */
    HAPMetricsSupportedMetrics* supportedMetricsStorage;

    /** Metrics Active value */
    uint8_t* metricsActiveValueStorage;

    /** Save Accessory State callback function */
    HAPMetricsSaveAccessoryStateCallback saveAccessoryStateCallback;

    /** Metrics Buffer Full State callback function */
    HAPMetricsBufferStateCallback bufferFullStateCallback;

    /** Accessory Server */
    HAPAccessoryServer* server;

    /** Accessory instance */
    HAPAccessory* accessory;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 0)
    /** Metrics event buffers */
    HAPMetricsEventBuffers* eventBuffers;

    /** Number of event buffers */
    uint8_t numEventBuffers;
#endif

    /** Number of dropped events because of a full buffer */
    uint32_t numDroppedEventsBufferFull;
} HAPMetricsContext;

/**
 * Initialize accessory metrics.
 *
 * @param  metricsContext           Metrics context.
 */
void HAPMetricsInitialize(HAPMetricsContext* metricsContext);

/**
 * Deinitialize accessory metrics.
 *
 * @param  metricsContext           Metrics context.
 */
void HAPMetricsDeinitialize(HAPMetricsContext* metricsContext);

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
void HAPMetricsHandleDataSendStreamAvailable(
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
 * Submit metric event to the metric queue waiting
 * to be sent to a controller.
 *
 * @param  metricID                 Metric ID.
 * @param  metricEventTLVBytes      TLV encoded metric event.
 * @param  numBytes                 Size of the TLV encoded metric event.
 */
void HAPMetricsSubmitMetricEvent(HAPMetricsID metricID, void* metricEventTLVBytes, size_t numBytes);

/**
 * Deletes all previously queued metric events of a specific metric ID
 *
 * @param  metricID                 Metric ID.
 */
void HAPMetricsDeleteMetricsEvents(HAPMetricsID metricID);

/**
 * Deletes all previously queued metric events
 */
void HAPMetricsDeleteAllMetricEvents(void);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
