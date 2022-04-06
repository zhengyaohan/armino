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

#include "HAP+API.h"
#include "HAP.h"
#include "HAPAccessory.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationIdentifyWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context) {
    if (!value) {
        HAPLog(&logObject, "Received invalid identify request.");
        return kHAPError_InvalidData;
    }

    if (!request->accessory->callbacks.identify) {
        HAPLogAccessory(&logObject, request->accessory, "No identify callback registered. Ignoring identify request.");
        return kHAPError_None;
    }
    return request->accessory->callbacks.identify(
            server,
            &(const HAPAccessoryIdentifyRequest) { .transportType = request->transportType,
                                                   .session = request->session,
                                                   .accessory = request->accessory,
                                                   .remote = request->remote },
            context);
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationManufacturerRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = request->accessory->manufacturer;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes >= 1);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationModelRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = request->accessory->model;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationNameRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = request->accessory->name;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationSerialNumberRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = request->accessory->serialNumber;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes >= 1);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationFirmwareRevisionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* stringToCopy = request->accessory->firmwareVersion;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes >= 1);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationHardwareRevisionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPAssert(request->accessory->hardwareVersion);
    const char* stringToCopy = request->accessory->hardwareVersion;
    size_t numBytes = HAPStringGetNumBytes(stringToCopy);
    HAPAssert(numBytes >= 1);
    HAPAssert(numBytes <= 64);
    if (numBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(value, stringToCopy, numBytes);
    value[numBytes] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationProductDataRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes,
        size_t maxValueBytes,
        size_t* numValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPAccessoryProductData productData;
    HAPAccessoryGetProductData(request->accessory, &productData);
    if (sizeof productData.bytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                sizeof productData.bytes,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    HAPRawBufferCopyBytes(valueBytes, productData.bytes, sizeof productData.bytes);
    *numValueBytes = sizeof productData.bytes;
    return kHAPError_None;
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationHardwareFinishRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_HardwareFinish));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AccessoryInformation));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPCharacteristicValue_HardwareFinish hardwareFinish;
    HAPRawBufferZero(&hardwareFinish, sizeof hardwareFinish);

    hardwareFinish.rgbColorValue = request->accessory->hardwareFinish;

    err = HAPTLVWriterEncode(responseWriter, &kHAPCharacteristicTLVFormat_HardwareFinish, &hardwareFinish);
    if (err) {
        HAPAssert(
                err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_OutOfResources ||
                err == kHAPError_Busy);
        return err;
    }

    return kHAPError_None;
}
#endif

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryInformationADKVersionRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context HAP_UNUSED) {
    const char* versionToCopy = HAPGetVersion();
    size_t numVersionBytes = HAPStringGetNumBytes(versionToCopy);
    HAPAssert(numVersionBytes >= 1);
    HAPAssert(numVersionBytes <= 64);
    const char* buildToCopy = HAPGetBuild();
    size_t numBuildBytes = HAPStringGetNumBytes(buildToCopy);
    HAPAssert(numBuildBytes >= 1);
    HAPAssert(numBuildBytes <= 64);
    if (numVersionBytes + 1 + numBuildBytes >= maxValueBytes) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "Not enough space (needed: %zu, available: %zu).",
                numVersionBytes + 1 + numBuildBytes + 1,
                maxValueBytes);
        return kHAPError_OutOfResources;
    }
    size_t position = 0;
    HAPRawBufferCopyBytes(&value[position], versionToCopy, numVersionBytes);
    position += numVersionBytes;
    value[position] = ';';
    position++;
    HAPRawBufferCopyBytes(&value[position], buildToCopy, numBuildBytes);
    position += numBuildBytes;
    HAPAssert(position < maxValueBytes);
    value[position] = '\0';
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryRuntimeInformationPingRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes HAP_UNUSED,
        size_t maxValueBytes HAP_UNUSED,
        size_t* numValueBytes,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);

    *numValueBytes = 0;
    HAPLogInfo(&kHAPLog_Default, "%s Controller sent Ping", __func__);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleAccessoryRuntimeInformationSleepIntervalRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    HAPAccessoryReachabilityConfiguration* reachabilityConfig = request->accessory->reachabilityConfiguration;
    if (reachabilityConfig) {
        *value = reachabilityConfig->sleepIntervalInMs;
    } else {
        *value = 0;
    }
    HAPLogInfo(&kHAPLog_Default, "%s Controller read Sleep Interval as %u", __func__, (unsigned int) *value);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleAccessoryRuntimeInformationHeartBeatRead(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPUInt32CharacteristicReadRequest* request HAP_UNUSED,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->accessory);
    HAPPrecondition(value);

    *value = server->heartBeat.value;
    HAPLogInfo(&kHAPLog_Default, "%s Controller read Heart Beat as %u", __func__, (unsigned int) *value);
    return kHAPError_None;
}
