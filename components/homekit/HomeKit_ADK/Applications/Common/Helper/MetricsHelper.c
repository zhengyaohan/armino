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

#include "ApplicationFeatures.h"

#include "HAP.h"

#if (HAVE_ACCESSORY_METRICS == 1)

#include "AppLogSubsystem.h"
#include "MetricsHelper.h"
#include "MetricsServiceDB.h"

static HAPMetricsContext* activeContext = NULL;
static bool metricsBufferFullState = false;

static const HAPLogObject logObject = { .subsystem = kApp_LogSubsystem, .category = "Metrics" };

static HAPMetricsSupportedMetrics
        defaultSupportedMetrics = { .numMetrics = 1,
                                    .config = {
                                            { .metricID = kHAPMetricsIDCPUStats,
                                              .status = kHAPMetricsSupportedMetricsStatus_Disabled }, // Disabled by
                                                                                                      // default, used
                                                                                                      // for testing
                                    } };

/**
 * TLV type to use to separate TLV items of same type.
 */
#define kHAPSupportedMetrics_TLVType_Separator ((uint8_t) 0x00)

/**
 * Initialize accessory metrics
 *
 * @param  metricsContext           Metrics context.
 */
void MetricsInitialize(HAPMetricsContext* metricsContext) {
    HAPPrecondition(metricsContext);
    HAPPrecondition(metricsContext->supportedMetricsStorage);
    HAPPrecondition(metricsContext->metricsActiveValueStorage);
    HAPPrecondition(metricsContext->saveAccessoryStateCallback);
    HAPPrecondition(metricsContext->server);
    HAPPrecondition(metricsContext->accessory);

    HAPLogDebug(&logObject, "%s", __func__);
    activeContext = metricsContext;

    HAPMetricsInitialize(metricsContext);
}

/**
 * Deinitialize accessory metrics
 *
 * @param  metricsContext           Metrics context.
 */
void MetricsDeinitialize(HAPMetricsContext* metricsContext) {
    HAPPrecondition(metricsContext);
    HAPLogDebug(&logObject, "%s", __func__);
    activeContext = NULL;

    HAPMetricsDeinitialize(metricsContext);
}

/**
 * Handle read request on the 'Active' characteristic of the Metrics service
 */
HAP_RESULT_USE_CHECK
HAPError HandleMetricsActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);
    HAPPrecondition(activeContext);

    *value = *activeContext->metricsActiveValueStorage;
    HAPLogInfo(&logObject, "%s: Value: %d", __func__, (int) (*value));

    return kHAPError_None;
}

/**
 * Handle write request on the 'Active' characteristic of the Metrics service
 */
HAP_RESULT_USE_CHECK
HAPError HandleMetricsActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(activeContext);
    HAPPrecondition(activeContext->saveAccessoryStateCallback);

    HAPLogDebug(&logObject, "%s", __func__);

    HAPCharacteristicValue_Active active = (HAPCharacteristicValue_Active) value;
    if (*activeContext->metricsActiveValueStorage != active) {
        *activeContext->metricsActiveValueStorage = active;
        activeContext->saveAccessoryStateCallback();
    }
    return kHAPError_None;
}

/**
 * Handle read request on the 'Supported Metrics' characteristic of the Metrics service
 */
HAP_RESULT_USE_CHECK
HAPError HandleMetricsSupportedMetricsRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(responseWriter);
    HAPPrecondition(activeContext);

    HAPLogDebug(&logObject, "%s", __func__);

    HAPError err;
    for (int i = 0; i < activeContext->supportedMetricsStorage->numMetrics; i++) {
        // Append separator if necessary.
        if (i) {
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPSupportedMetrics_TLVType_Separator,
                                      .value = { .bytes = NULL, .numBytes = 0 } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Supported Metrics Configuration
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Metric ID
            {
                size_t metricIDMinBytes =
                        HAPGetVariableIntEncodingLength(activeContext->supportedMetricsStorage->config[i].metricID);
                uint8_t metricIDBytes[] = { HAPExpandLittleUInt32(
                        activeContext->supportedMetricsStorage->config[i].metricID) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedMetrics_MetricID,
                                          .value = { .bytes = metricIDBytes, .numBytes = metricIDMinBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
            // Status
            {
                uint8_t statusBytes[] = { (uint8_t)(activeContext->supportedMetricsStorage->config[i].status) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedMetrics_Status,
                                          .value = { .bytes = statusBytes, .numBytes = sizeof statusBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    responseWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedMetrics_Configuration,
                                      .value = { .bytes = bytes, .numBytes = numBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }
    }

    return kHAPError_None;
}

/**
 * Handle write request on the 'Supported Metrics' characteristic of the Metrics service
 */
HAP_RESULT_USE_CHECK
HAPError HandleMetricsSupportedMetricsWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(requestReader);
    HAPPrecondition(activeContext);

    HAPLogDebug(&logObject, "%s", __func__);

    for (;;) {
        bool found;
        HAPError err;
        HAPTLV tlv;
        err = HAPTLVReaderGetNext(requestReader, &found, &tlv);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            HAPLogError(&logObject, "%s: Parsing error.", __func__);
            return err;
        }
        if (!found) {
            break;
        }

        if (tlv.type == kHAPCharacteristicValue_SupportedMetrics_Configuration) {
            HAPTLVReader subReader;
            HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
            HAPMetricsConfig* config = NULL;
            bool found;
            HAPTLV supportedMetricsTLV;

            err = HAPTLVReaderGetNext(&subReader, &found, &supportedMetricsTLV);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (!found) {
                HAPLogError(&logObject, "metric ID not present");
                return kHAPError_InvalidData;
            }

            if (supportedMetricsTLV.type == kHAPCharacteristicValue_SupportedMetrics_MetricID) {
                // Handle Metric ID
                HAPMetricsID metricID = kHAPMetricsIDBase;
                if (supportedMetricsTLV.value.numBytes == sizeof(uint8_t)) {
                    metricID = (HAPMetricsID) HAPReadUIntMax8(supportedMetricsTLV.value.bytes, sizeof(uint8_t));
                } else if (supportedMetricsTLV.value.numBytes == sizeof(uint16_t)) {
                    metricID = (HAPMetricsID) HAPReadLittleUInt16(supportedMetricsTLV.value.bytes);
                } else if (supportedMetricsTLV.value.numBytes == sizeof(uint32_t)) {
                    metricID = (HAPMetricsID) HAPReadLittleUInt32(supportedMetricsTLV.value.bytes);
                } else {
                    HAPLogError(
                            &logObject,
                            "%s: Metric ID has invalid length %zu.",
                            __func__,
                            supportedMetricsTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }

                bool foundMetric = false;
                for (int i = 0; i < activeContext->supportedMetricsStorage->numMetrics; i++) {
                    if (metricID == activeContext->supportedMetricsStorage->config[i].metricID) {
                        config = &activeContext->supportedMetricsStorage->config[i];
                        foundMetric = true;
                    }
                }
                if (foundMetric == false) {
                    HAPLogError(&logObject, "%s: Metric ID %u not supported.", __func__, metricID);
                    return kHAPError_InvalidData;
                } else {
                    HAPLogInfo(&logObject, "Found metric ID %u", metricID);
                }

            } else {
                HAPLogError(&logObject, "Expected metric ID");
                return kHAPError_InvalidData;
            }

            err = HAPTLVReaderGetNext(&subReader, &found, &supportedMetricsTLV);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }
            if (!found) {
                HAPLogError(&logObject, "Metric status not present");
                return kHAPError_InvalidData;
            }

            if (supportedMetricsTLV.type == kHAPCharacteristicValue_SupportedMetrics_Status) {
                // Handle status
                HAPPrecondition(config);
                if (supportedMetricsTLV.value.numBytes != sizeof(HAPMetricsSupportedMetricsStatus)) {
                    HAPLogError(
                            &logObject,
                            "%s: Metric status has invalid length %zu.",
                            __func__,
                            supportedMetricsTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                HAPLogInfo(&logObject, "Status: %u", (unsigned int) HAPReadUInt8(supportedMetricsTLV.value.bytes));

                switch (HAPReadUInt8(supportedMetricsTLV.value.bytes)) {
                    case kHAPMetricsSupportedMetricsStatus_Enabled: {
                        config->status = kHAPMetricsSupportedMetricsStatus_Enabled;
                        break;
                    }
                    case kHAPMetricsSupportedMetricsStatus_Disabled: {
                        if (config->status == kHAPMetricsSupportedMetricsStatus_Enabled) {
                            HAPMetricsDeleteMetricsEvents(config->metricID);
                            HAPLogDebug(
                                    &logObject,
                                    "Deleted previously stored metric events for metric ID %u.",
                                    config->metricID);
                        }
                        config->status = kHAPMetricsSupportedMetricsStatus_Disabled;
                        break;
                    }
                    default: {
                        HAPLogError(&logObject, "%s: Unknown status value.", __func__);
                        return kHAPError_InvalidData;
                    }
                }
            } else {
                HAPLogError(&logObject, "Expected metric status");
                return kHAPError_InvalidData;
            }
        } else if (tlv.type == kHAPSupportedMetrics_TLVType_Separator) {
            continue;
        } else {
            HAPLogError(&logObject, "Unknown TLV type: %u", (unsigned int) tlv.type);
            return kHAPError_InvalidData;
        }
    }
    activeContext->saveAccessoryStateCallback();

    return kHAPError_None;
}

/**
 * Handle read request on the 'Metrics Buffer Full State' characteristic of the Metrics service
 */
HAP_RESULT_USE_CHECK
HAPError HandleMetricsBufferFullStateRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->service);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);
    HAPPrecondition(activeContext);

    *value = metricsBufferFullState;
    HAPLogInfo(&logObject, "%s: IsMetricsBufferFull: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

/**
 * Callback function used by HAP to inform app the state of the metrics buffer
 */
void MetricsBufferFullStateCallback(bool isBufferFull) {
    HAPLogInfo(&logObject, "%s: IsMetricsBufferFull: %s", __func__, isBufferFull ? "true" : "false");

    if (metricsBufferFullState != isBufferFull) {
        metricsBufferFullState = isBufferFull;
        HAPAccessoryServerRaiseEvent(
                activeContext->server,
                &metricsBufferFullStateCharacteristic,
                &metricsService,
                activeContext->accessory);
    }
}

/**
 * Handle changes to accessory pairing state
 */
void MetricsHandlePairingStateChange(
        HAPPairingStateChange state,
        uint8_t* metricsActiveValue,
        HAPMetricsSupportedMetrics* supportedMetrics,
        SaveAccessoryStateFunc saveAccessoryStateFunc) {
    HAPPrecondition(metricsActiveValue);
    HAPPrecondition(supportedMetrics);
    HAPPrecondition(saveAccessoryStateFunc);
    switch (state) {
        case kHAPPairingStateChange_Paired:
            break;
        case kHAPPairingStateChange_Unpaired: {
            *metricsActiveValue = kHAPCharacteristicValue_Active_Inactive;
            MetricsGetDefaultSupportedMetricsConfiguration(supportedMetrics);
            HAPMetricsDeleteAllMetricEvents();
            // Save accessory state in order to save the updated setting into persistent memory.
            saveAccessoryStateFunc();
            break;
        }
    }
}

/**
 * Get the supported metrics default configuration
 */
void MetricsGetDefaultSupportedMetricsConfiguration(HAPMetricsSupportedMetrics* supportedMetrics) {
    HAPPrecondition(supportedMetrics);
    HAPLogInfo(&logObject, "%s: Resetting to default supported metrics configuration", __func__);
    *supportedMetrics = defaultSupportedMetrics;
}

#endif // HAVE_ACCESSORY_METRICS
