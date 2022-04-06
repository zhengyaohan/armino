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

#ifndef HAP_IP_ACCESSORY_PROTOCOL_H
#define HAP_IP_ACCESSORY_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPIP+ByteBuffer.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#define kHAPIPAccessoryProtocolAID_PrimaryAccessory ((uint64_t) 1)

#define kHAPIPAccessoryProtocolIID_AccessoryInformation ((uint64_t) 1)

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

typedef struct _HAPIPReadContext {
    uint64_t aid;
    uint64_t iid;
    int32_t status;
    union {
        int32_t intValue;
        uint64_t unsignedIntValue;
        float floatValue;
        struct {
            char* _Nullable bytes;
            size_t numBytes;
        } stringValue;
    } value;
    bool ev;
    struct {
        char* _Nullable bytes;
        size_t numBytes;
    } contextData;
} HAPIPReadContext;

typedef struct {
    bool meta;
    bool perms;
    bool type;
    bool ev;
} HAPIPReadRequestParameters;

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCharacteristicReadRequests(
        char* bytes,
        size_t numBytes,
        HAPIPReadContext* readContexts,
        size_t maxReadContexts,
        size_t* numReadContexts,
        HAPIPReadRequestParameters* parameters);

HAP_RESULT_USE_CHECK
size_t HAPIPAccessoryProtocolGetNumCharacteristicReadResponseBytes(
        HAPAccessoryServer* server,
        HAPIPReadContext* readContexts,
        size_t numReadContexts,
        HAPIPReadRequestParameters* parameters);

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCharacteristicReadResponseBytes(
        HAPAccessoryServer* server,
        HAPIPReadContext* readContexts,
        size_t numReadContexts,
        HAPIPReadRequestParameters* parameters,
        HAPIPByteBuffer* buffer);

HAP_ENUM_BEGIN(uint8_t, HAPIPWriteValueType) { kHAPIPWriteValueType_None,
                                               kHAPIPWriteValueType_Int,
                                               kHAPIPWriteValueType_UInt,
                                               kHAPIPWriteValueType_Float,
                                               kHAPIPWriteValueType_String } HAP_ENUM_END(uint8_t, HAPIPWriteValueType);

HAP_ENUM_BEGIN(uint8_t, HAPIPEventNotificationState) {
    kHAPIPEventNotificationState_Undefined,
    kHAPIPEventNotificationState_Disabled,
    kHAPIPEventNotificationState_Enabled,
} HAP_ENUM_END(uint8_t, HAPIPEventNotificationState);

typedef struct _HAPIPWriteContext {
    uint64_t aid;
    uint64_t iid;
    int32_t status;
    HAPIPWriteValueType type;
    union {
        int32_t intValue;
        uint64_t unsignedIntValue;
        float floatValue;
        struct {
            char* _Nullable bytes;
            size_t numBytes;
        } stringValue;
    } value;
    struct {
        char* _Nullable bytes;
        size_t numBytes;
    } authorizationData;
    struct {
        char* _Nullable bytes;
        size_t numBytes;
    } contextData;
    bool remote;
    HAPIPEventNotificationState ev;
    bool response;
} HAPIPWriteContext;

/**
 * Parses a PUT /characteristic request.
 *
 * @param      bytes                Bytes
 * @param      numBytes             Length of @p bytes.
 * @param[out] writeContexts        Contexts to store data about the received write requests.
 * @param      maxWriteContexts     Capacity of @p writeContexts.
 * @param[out] numWriteContexts     Number of valid contexts.
 * @param[out] hasPID               True if a PID was specified. False otherwise.
 * @param[out] pid                  PID, if a PID was specified.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If request malformed.
 * @return kHAPError_OutOfResources If not enough contexts were available.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
        char* bytes,
        size_t numBytes,
        HAPIPWriteContext* writeContexts,
        size_t maxWriteContexts,
        size_t* numWriteContexts,
        bool* hasPID,
        uint64_t* pid);

HAP_RESULT_USE_CHECK
size_t HAPIPAccessoryProtocolGetNumCharacteristicWriteResponseBytes(
        HAPAccessoryServer* server,
        HAPIPWriteContext* writeContexts,
        size_t numWriteContexts);

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCharacteristicWriteResponseBytes(
        HAPAccessoryServer* server,
        HAPIPWriteContext* writeContexts,
        size_t numWriteContexts,
        HAPIPByteBuffer* buffer);

HAP_RESULT_USE_CHECK
size_t HAPIPAccessoryProtocolGetNumEventNotificationBytes(
        HAPAccessoryServer* server,
        HAPIPReadContext* readContexts,
        size_t numReadContexts);

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetEventNotificationBytes(
        HAPAccessoryServer* server,
        HAPIPReadContext* readContexts,
        size_t numReadContexts,
        HAPIPByteBuffer* buffer);

/**
 * Parses a PUT /prepare request.
 *
 * @param      bytes                Buffer
 * @param      numBytes             Length of @p bytes.
 * @param[out] ttl                  TTL.
 * @param[out] pid                  PID.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If request malformed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCharacteristicWritePreparation(
        const char* bytes,
        size_t numBytes,
        uint64_t* ttl,
        uint64_t* pid);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
HAP_ENUM_BEGIN(uint8_t, HAPIPCameraSnapshotReason) {
    kHAPIPCameraSnapshotReason_Undefined = 0,
    kHAPIPCameraSnapshotReason_PeriodicSnapshot,
    kHAPIPCameraSnapshotReason_EventSnapshot,
} HAP_ENUM_END(uint8_t, HAPIPCameraSnapshotReason);

HAP_RESULT_USE_CHECK
HAPError HAPIPAccessoryProtocolGetCameraSnapshotRequest(
        const char* bytes,
        size_t numBytes,
        uint64_t* aid,
        HAPIPCameraSnapshotReason* snapshotReason,
        uint64_t* imageWidth,
        uint64_t* imageHeight);

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
