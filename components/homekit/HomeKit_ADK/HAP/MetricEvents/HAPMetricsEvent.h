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

#ifndef HAP_METRICS_EVENT_H
#define HAP_METRICS_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESSORY_METRICS)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Denotes metrics field member flags
 */
HAP_OPTIONS_BEGIN(uint8_t, MetricsFieldFlags) {
    /** Set if field is mandatory. */
    kHAPMetricsFieldMandatory = 1U << 0U,

    /** Set if field value is set. */
    kHAPMetricsFieldValueIsSet = 1U << 1U,
} HAP_OPTIONS_END(uint8_t, MetricsFieldFlags);

#define kHAPMetricsMaxByteArraySize ((size_t) 1024)

/**
 * Metrics Field ID
 */
typedef uint8_t HAPMetricsFieldID;

typedef struct {
    MetricsFieldFlags flags;
    HAPMetricsFieldID fieldID;
} MetricsFieldProperty;

typedef struct {
    MetricsFieldProperty property;
    uint8_t value;
} MetricsFieldTypeUint8;

typedef struct {
    MetricsFieldProperty property;
    uint16_t value;
} MetricsFieldTypeUint16;

typedef struct {
    MetricsFieldProperty property;
    uint32_t value;
} MetricsFieldTypeUint32;

typedef struct {
    MetricsFieldProperty property;
    uint64_t value;
} MetricsFieldTypeUint64;

typedef struct {
    MetricsFieldProperty property;
    float value;
} MetricsFieldTypeFloat;

typedef struct {
    MetricsFieldProperty property;
    double value;
} MetricsFieldTypeDouble;

typedef struct {
    MetricsFieldProperty property;
    uint8_t value[kHAPMetricsMaxByteArraySize];
    size_t numBytes;
} MetricsFieldTypeByteArray;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Metric ID
 */
typedef uint32_t HAPMetricsID;

#define kHAPMetricsIDBase     ((uint32_t) 0)
#define kHAPMetricsIDCPUStats ((uint32_t) 1) // CPU statistics
//----------------------------------------------------------------------------------------------------------------------

typedef void HAPMetricsEvent;

/**
 * Initialize a single metric event.
 *
 * @param  event        Metrics event.
 * @param  id           Metrics ID.
 */
void HAPMetricsEventInitialize(HAPMetricsEvent* event, const HAPMetricsID id);

/**
 * Set values for members of the metric event.
 * This function validates the inputs before they are set.
 *
 * @param  event        Metrics event.
 * @param  id           Metrics ID.
 * @param  fieldID      Metric field to be set.
 * @param  value        Value to be set.
 * @param  numBytes     Size of the value.
 */
void HAPMetricsEventSetFieldValue(
        HAPMetricsEvent* event,
        const HAPMetricsID id,
        const HAPMetricsFieldID fieldID,
        void* value,
        const size_t numBytes);

/**
 * This function encodes the metric event to TLV.
 * Metric events should be in TLV format before they are
 * sent to a controller over HDS.
 *
 * @param  event                     Metrics event.
 * @param  id                        Metrics ID.
 * @param  scratchBuffer             Scratch buffer.
 * @param  scratchBufferSize         Size of scratch buffer.
 * @param  encodedTLVBuffer          Encoded TLV buffer.
 * @param  encodedTLVBufferSize      Size of encoded TLV buffer.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMetricsEventEncodeTLV(
        HAPMetricsEvent* event,
        const HAPMetricsID id,
        void* scratchBuffer,
        size_t scratchBufferSize,
        void* _Nonnull* _Nonnull encodedTLVBuffer,
        size_t* encodedTLVBufferSize);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
