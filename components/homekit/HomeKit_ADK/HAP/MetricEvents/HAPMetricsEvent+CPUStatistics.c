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

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)

#include "HAPLogSubsystem.h"
#include "HAPMetricsEvent+CPUStatistics.h"
#include "HAPMetricsEvent.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "HAPMetrics" };

/**
 * Initialize a single metric event.
 *
 * @param  event        Metrics event.
 */
void HAPMetricsEventInitializeCPUStatistics(HAPMetricsEvent* event) {
    HAPPrecondition(event);

    MetricsCPUStatistics* cpuStats = (MetricsCPUStatistics*) event;
    HAPRawBufferZero(cpuStats, sizeof(*cpuStats));

    // Set the field properties
    // timestamp
    cpuStats->timestamp.property.fieldID = kHAPMetricsFieldIDCPUStatisticsTimestamp;
    // metricID
    cpuStats->metricsID.value = kHAPMetricsIDCPUStats;
    cpuStats->metricsID.property.flags = kHAPMetricsFieldMandatory | kHAPMetricsFieldValueIsSet;
    cpuStats->metricsID.property.fieldID = kHAPMetricsFieldIDCPUStatisticsMetricsID;
    // numberOfCpus
    cpuStats->numberOfCPUs.property.fieldID = kHAPMetricsFieldIDCPUStatisticsNumberOfCPUs;
    // user
    cpuStats->user.property.fieldID = kHAPMetricsFieldIDCPUStatisticsUser;
    // system
    cpuStats->system.property.fieldID = kHAPMetricsFieldIDCPUStatisticsSystem;
    // ioWait
    cpuStats->ioWait.property.fieldID = kHAPMetricsFieldIDCPUStatisticsIOWait;
    // idle
    cpuStats->idle.property.fieldID = kHAPMetricsFieldIDCPUStatisticsIdle;
    // temperature
    cpuStats->temp.property.fieldID = kHAPMetricsFieldIDCPUStatisticsTemp;

    return;
}

/**
 * Set values for members of the metric event.
 * This function validates the inputs before they are set.
 *
 * @param  event        Metrics event.
 * @param  fieldID      Metric field to be set.
 * @param  value        Value to be set.
 * @param  numBytes     Size of the value.
 */
void HAPMetricsEventSetFieldValueCPUStatistics(
        HAPMetricsEvent* event,
        const HAPMetricsFieldID fieldID,
        void* value,
        const size_t numBytes) {
    HAPPrecondition(event);
    HAPPrecondition(value);

    MetricsCPUStatistics* cpuStats = (MetricsCPUStatistics*) event;
    if (fieldID == kHAPMetricsFieldIDCPUStatisticsTimestamp && numBytes == sizeof(cpuStats->timestamp.value)) {
        cpuStats->timestamp.value = *((uint64_t*) value);
        cpuStats->timestamp.property.flags = cpuStats->timestamp.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsMetricsID && numBytes == sizeof(cpuStats->metricsID.value)) {
        cpuStats->metricsID.value = *((uint32_t*) value);
        cpuStats->metricsID.property.flags = cpuStats->metricsID.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (
            fieldID == kHAPMetricsFieldIDCPUStatisticsNumberOfCPUs &&
            numBytes == sizeof(cpuStats->numberOfCPUs.value)) {
        cpuStats->numberOfCPUs.value = *((uint32_t*) value);
        cpuStats->numberOfCPUs.property.flags = cpuStats->numberOfCPUs.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsUser && numBytes == sizeof(cpuStats->user.value)) {
        cpuStats->user.value = *((uint8_t*) value);
        cpuStats->user.property.flags = cpuStats->user.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsSystem && numBytes == sizeof(cpuStats->system.value)) {
        cpuStats->system.value = *((uint8_t*) value);
        cpuStats->system.property.flags = cpuStats->system.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsIOWait && numBytes == sizeof(cpuStats->ioWait.value)) {
        cpuStats->ioWait.value = *((uint8_t*) value);
        cpuStats->ioWait.property.flags = cpuStats->ioWait.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsIdle && numBytes == sizeof(cpuStats->idle.value)) {
        cpuStats->idle.value = *((uint8_t*) value);
        cpuStats->idle.property.flags = cpuStats->idle.property.flags | kHAPMetricsFieldValueIsSet;
    } else if (fieldID == kHAPMetricsFieldIDCPUStatisticsTemp && numBytes == sizeof(cpuStats->temp.value)) {
        cpuStats->temp.value = *((float*) value);
        cpuStats->temp.property.flags = cpuStats->temp.property.flags | kHAPMetricsFieldValueIsSet;
    } else {
        HAPLogError(&logObject, "Invalid data");
        HAPFatalError();
    }

    return;
}

/**
 * This function validates the metric event format
 */
static bool IsMetricEventFormatValid(MetricsCPUStatistics* cpuStats) {
    HAPPrecondition(cpuStats);

    // Check if all mandatory parameters are present
    if ((cpuStats->metricsID.property.flags & kHAPMetricsFieldMandatory) &&
        !(cpuStats->metricsID.property.flags & kHAPMetricsFieldValueIsSet)) {
        return false;
    }
    return true;
}

/**
 * This function encodes the metric event to TLV
 *
 * @param  event                     Metrics event.
 * @param  scratchBuffer             Scratch buffer.
 * @param  scratchBufferSize         Size of scratch buffer.
 * @param  encodedTLVBuffer          Encoded TLV buffer.
 * @param  encodedTLVBufferSize      Size of encoded TLV buffer.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMetricsEventEncodeTLVCPUStatistics(
        HAPMetricsEvent* event,
        void* scratchBuffer,
        size_t scratchBufferSize,
        void* _Nonnull* _Nonnull encodedTLVBuffer,
        size_t* encodedTLVBufferSize) {
    HAPPrecondition(event);
    HAPPrecondition(scratchBuffer);
    HAPPrecondition(scratchBufferSize > 0);
    HAPPrecondition(encodedTLVBuffer);

    HAPError err = kHAPError_None;
    HAPTLVWriter tlvWriter;
    HAPTLVWriterCreate(&tlvWriter, scratchBuffer, scratchBufferSize);

    MetricsCPUStatistics* cpuStats = (MetricsCPUStatistics*) event;
    if (IsMetricEventFormatValid(cpuStats) == false) {
        return kHAPError_InvalidData;
    }

    if (cpuStats->timestamp.property.flags & kHAPMetricsFieldValueIsSet) {
        size_t minBytes = HAPGetVariableIntEncodingLength(cpuStats->timestamp.value);
        err = HAPTLVWriterAppend(
                &tlvWriter,
                &(const HAPTLV) { .type = kHAPMetricsFieldIDCPUStatisticsTimestamp,
                                  .value = { .bytes = &cpuStats->timestamp.value, .numBytes = minBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (cpuStats->metricsID.property.flags & kHAPMetricsFieldValueIsSet) {
        size_t minBytes = HAPGetVariableIntEncodingLength(cpuStats->metricsID.value);
        err = HAPTLVWriterAppend(
                &tlvWriter,
                &(const HAPTLV) { .type = kHAPMetricsFieldIDCPUStatisticsMetricsID,
                                  .value = { .bytes = &cpuStats->metricsID.value, .numBytes = minBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (cpuStats->numberOfCPUs.property.flags & kHAPMetricsFieldValueIsSet) {
        size_t minBytes = HAPGetVariableIntEncodingLength(cpuStats->numberOfCPUs.value);
        err = HAPTLVWriterAppend(
                &tlvWriter,
                &(const HAPTLV) { .type = kHAPMetricsFieldIDCPUStatisticsNumberOfCPUs,
                                  .value = { .bytes = &cpuStats->numberOfCPUs.value, .numBytes = minBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (cpuStats->user.property.flags & kHAPMetricsFieldValueIsSet) {
        err = HAPTLVWriterAppend(
                &tlvWriter,
                &(const HAPTLV) {
                        .type = kHAPMetricsFieldIDCPUStatisticsUser,
                        .value = { .bytes = &cpuStats->user.value, .numBytes = sizeof cpuStats->user.value } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }
    if (cpuStats->system.property.flags & kHAPMetricsFieldValueIsSet) {
        err = HAPTLVWriterAppend(
                &tlvWriter,
                &(const HAPTLV) {
                        .type = kHAPMetricsFieldIDCPUStatisticsSystem,
                        .value = { .bytes = &cpuStats->system.value, .numBytes = sizeof cpuStats->system.value } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    HAPTLVWriterGetBuffer(&tlvWriter, encodedTLVBuffer, encodedTLVBufferSize);
    HAPAssert(*encodedTLVBufferSize <= scratchBufferSize);

    return kHAPError_None;
}

#endif
