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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "HAPAccessory+Camera.h"
#include "HAPCharacteristic.h"
#include "HAPCharacteristicTypes.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"
#include "HAPVideo.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * TLV type to use to separate TLV items of same type.
 */
#define kHAPCameraEventRecordingManagement_TLVType_Separator ((uint8_t) 0x00)

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Active));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(value);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isRecordingActive;
    err = HAPPlatformCameraIsRecordingActive(camera, &isRecordingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsRecordingActive failed: %u.", err);
        return err;
    }
    if (isRecordingActive) {
        *value = kHAPCharacteristicValue_Active_Active;
    } else {
        *value = kHAPCharacteristicValue_Active_Inactive;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_ActiveIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_Active));
    switch ((HAPCharacteristicValue_Active) value) {
        case kHAPCharacteristicValue_Active_Inactive:
        case kHAPCharacteristicValue_Active_Active: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_Active));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_ActiveIsValid(value_));
    HAPCharacteristicValue_Active value = (HAPCharacteristicValue_Active) value_;

    HAPError err;

    bool wasRecordingActive;
    err = HAPPlatformCameraIsRecordingActive(camera, &wasRecordingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsRecordingActive failed: %u.", err);
        return err;
    }
    bool isRecordingActive = value == kHAPCharacteristicValue_Active_Active;

    if (isRecordingActive != wasRecordingActive) {
        HAPLogInfo(
                &logObject,
                "[%p] %s Camera Event Recordings.",
                (const void*) camera,
                isRecordingActive ? "Enabling" : "Disabling");
        err = HAPPlatformCameraSetRecordingActive(camera, isRecordingActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetRecordingActive failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_MediaContainerType SerializeMediaContainerType(HAPMediaContainerType value) {
    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPMediaContainerType_FragmentedMP4: {
            return kHAPCharacteristicValue_MediaContainerType_FragmentedMP4;
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPError SerializeMediaContainerConfiguration(
        HAPMediaContainerType containerType,
        const HAPMediaContainerParameters* containerParameters_,
        HAPTLVWriter* responseWriter,
        uint8_t containerTypeTLVType,
        uint8_t containerParametersTLVType) {
    HAPPrecondition(responseWriter);

    HAPError err;

    // Media Container Type.
    uint8_t mediaContainerTypeBytes[] = { SerializeMediaContainerType(containerType) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = containerTypeTLVType,
                    .value = { .bytes = mediaContainerTypeBytes, .numBytes = sizeof mediaContainerTypeBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Media Container Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        switch (containerType) { // NOLINT(hicpp-multiway-paths-covered)
            case kHAPMediaContainerType_FragmentedMP4: {
                HAPPrecondition(containerParameters_);
                const HAPFragmentedMP4MediaContainerParameters* containerParameters = containerParameters_;

                // Fragment Duration.
                uint8_t fragmentDurationBytes[] = { HAPExpandLittleUInt32(containerParameters->fragmentDuration) };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_FragmentedMP4MediaContainerParameter_FragmentDuration,
                                .value = { .bytes = fragmentDurationBytes,
                                           .numBytes = sizeof fragmentDurationBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
                break;
            }
            default:
                HAPFatalError();
        }

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = containerParametersTLVType,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

typedef struct {
    const HAPTLV8CharacteristicReadRequest* request;
    HAPTLVWriter* responseWriter;
    HAPError err;
} SupportedCameraRecordingConfigurationContext;

static void SupportedCameraRecordingConfigurationCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(context);
    SupportedCameraRecordingConfigurationContext* arguments = context;
    HAPPrecondition(arguments->request);
    const HAPTLV8CharacteristicReadRequest* request = arguments->request;
    HAPPrecondition(arguments->responseWriter);
    HAPTLVWriter* responseWriter = arguments->responseWriter;
    HAPPrecondition(!arguments->err);
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

// Log Supported Camera Recording Configuration.
#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    {
        char logBytes[2048];
        HAPStringBuilder stringBuilder;
        HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
        HAPStringBuilderAppend(&stringBuilder, "Supported Camera Recording Configuration:");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n- Prebuffer Duration: %llums",
                // NOLINTNEXTLINE(google-readability-casting)
                (unsigned long long) (supportedConfig->recording.prebufferDuration / HAPMillisecond));
        HAPStringBuilderAppend(&stringBuilder, "\n- Event Trigger Type:");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n  - Motion: %s",
                (supportedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Motion) ? "Yes" :
                                                                                                                "No");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n  - Doorbell: %s",
                (supportedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Doorbell) ?
                        "Yes" :
                        "No");
        if (supportedConfig->recording.containerConfigurations) {
            for (size_t i = 0; supportedConfig->recording.containerConfigurations[i]; i++) {
                const HAPCameraSupportedMediaContainerConfiguration* containerConfiguration =
                        supportedConfig->recording.containerConfigurations[i];
                HAPStringBuilderAppend(&stringBuilder, "\n- Media Container Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Media Container Type: ");
                switch (containerConfiguration->containerType) {
                    case kHAPMediaContainerType_FragmentedMP4: {
                        const HAPFragmentedMP4MediaContainerParameters* containerParameters =
                                containerConfiguration->containerParameters;
                        HAPStringBuilderAppend(&stringBuilder, "Fragmented MP4");
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Media Container Parameters:");
                        HAPStringBuilderAppend(
                                &stringBuilder,
                                "\n    - Fragment Duration: %lums",
                                (unsigned long) containerParameters->fragmentDuration);
                        break;
                    }
                }
            }
        }
        if (HAPStringBuilderDidOverflow(&stringBuilder)) {
            HAPLogCharacteristicError(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Logs were truncated.",
                    (const void*) camera);
        }
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] %s",
                (const void*) camera,
                HAPStringBuilderGetString(&stringBuilder));
    }
#endif

    // Prebuffer Duration.
    if ((supportedConfig->recording.prebufferDuration != 0 &&
         supportedConfig->recording.prebufferDuration <
                 kHAPCameraRecordingSupportedConfiguration_MinPrebufferDuration) ||
        supportedConfig->recording.prebufferDuration > kHAPCameraRecordingSupportedConfiguration_MaxPrebufferDuration) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Prebuffer Duration must be in range %llums ... %llums.",
                (const void*) camera,
                (unsigned long long) (kHAPCameraRecordingSupportedConfiguration_MinPrebufferDuration / HAPMillisecond),
                (unsigned long long) (kHAPCameraRecordingSupportedConfiguration_MaxPrebufferDuration / HAPMillisecond));
        HAPFatalError();
    }
    uint8_t prebufferDurationBytes[] = { HAPExpandLittleUInt32(
            (uint32_t)(supportedConfig->recording.prebufferDuration / HAPMillisecond)) };
    arguments->err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_PrebufferDuration,
                    .value = { .bytes = prebufferDurationBytes, .numBytes = sizeof prebufferDurationBytes } });
    if (arguments->err) {
        HAPAssert(arguments->err == kHAPError_OutOfResources);
        return;
    }

    // Event Trigger Type.
    HAPCameraEventTriggerTypes allEventTriggerTypes =
            kHAPCameraEventTriggerTypes_Motion + kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell;
    if (supportedConfig->recording.eventTriggerTypes & (HAPCameraEventTriggerTypes) ~allEventTriggerTypes) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Unknown event trigger types specified : 0x%X.",
                (const void*) camera,
                supportedConfig->recording.eventTriggerTypes & (HAPCameraEventTriggerTypes) ~allEventTriggerTypes);
        HAPFatalError();
    }
    uint64_t rawEventTriggerType = 0;
    HAPAssert(sizeof(HAPCameraEventTriggerTypes) == sizeof(uint8_t));
    if (supportedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Motion) {
        rawEventTriggerType |= (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Motion;
    }
    if (supportedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Doorbell) {
        rawEventTriggerType |= (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell;
    }
    uint8_t eventTriggerTypeBytes[] = { HAPExpandLittleUInt64(rawEventTriggerType) };
    arguments->err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_EventTriggerType,
                              .value = { .bytes = eventTriggerTypeBytes, .numBytes = sizeof eventTriggerTypeBytes } });
    if (arguments->err) {
        HAPAssert(arguments->err == kHAPError_OutOfResources);
        return;
    }

    // Enumerate Media Container Configurations.
    if (supportedConfig->recording.containerConfigurations) {
        for (size_t i = 0; supportedConfig->recording.containerConfigurations[i]; i++) {
            const HAPCameraSupportedMediaContainerConfiguration* containerConfiguration =
                    supportedConfig->recording.containerConfigurations[i];

            // Append separator if necessary.
            if (i) {
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }

            // Media Container Configuration.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                arguments->err = SerializeMediaContainerConfiguration(
                        containerConfiguration->containerType,
                        containerConfiguration->containerParameters,
                        &subWriter,
                        kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration_Type,
                        kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration_Parameters);
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }

                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementSupportedCameraRecordingConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType,
            &kHAPCharacteristicType_SupportedCameraRecordingConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(request->accessory->callbacks.camera.getSupportedRecordingConfiguration);
    HAPPrecondition(responseWriter);

    SupportedCameraRecordingConfigurationContext requestContext;
    HAPRawBufferZero(&requestContext, sizeof requestContext);
    requestContext.request = request;
    requestContext.responseWriter = responseWriter;
    requestContext.err = kHAPError_None;
    HAPCameraAccessoryGetSupportedRecordingConfiguration(
            server,
            request->service,
            request->accessory,
            SupportedCameraRecordingConfigurationCallback,
            &requestContext);
    if (requestContext.err) {
        HAPAssert(requestContext.err == kHAPError_OutOfResources);
        return requestContext.err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_CameraRecording_VideoCodecType SerializeVideoCodecType(HAPVideoCodecType value) {
    HAPPrecondition(HAPVideoCodecTypeIsSupportedForRecording(value));

    switch (value) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPVideoCodecType_H264:
            return kHAPCharacteristicValue_CameraRecording_VideoCodecType_H264;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPError SerializeSupportedH264VideoCodecParameters(
        const HAPH264VideoCodecParameters* codecParameters,
        HAPTLVWriter* responseWriter,
        bool appendRateAndIFrame) {
    HAPPrecondition(codecParameters);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Enumerate ProfileIDs.
    {
        bool needsSeparator = false;
        HAPH264VideoCodecProfile profiles = codecParameters->profile;
        for (size_t i = 0; profiles; i++) {
            HAPH264VideoCodecProfile profile = (HAPH264VideoCodecProfile)(1U << i);
            if (profiles & profile) {
                profiles &= (HAPH264VideoCodecProfile) ~profile;

                // Append separator if necessary.
                if (needsSeparator) {
                    err = HAPTLVWriterAppend(
                            responseWriter,
                            &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                              .value = { .bytes = NULL, .numBytes = 0 } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    needsSeparator = true;
                }

                // ProfileID.
                uint8_t profileIDBytes[] = { HAPVideoSerializeH264ProfileID(profile) };
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Profile,
                                .value = { .bytes = profileIDBytes, .numBytes = sizeof profileIDBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    // Enumerate Levels.
    {
        bool needsSeparator = false;
        HAPH264VideoCodecProfileLevel levels = codecParameters->level;
        for (size_t i = 0; levels; i++) {
            HAPH264VideoCodecProfileLevel level = (HAPH264VideoCodecProfileLevel)(1U << i);
            if (levels & level) {
                levels &= (HAPH264VideoCodecProfileLevel) ~level;

                // Append separator if necessary.
                if (needsSeparator) {
                    err = HAPTLVWriterAppend(
                            responseWriter,
                            &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                              .value = { .bytes = NULL, .numBytes = 0 } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    needsSeparator = true;
                }

                // Level.
                uint8_t levelBytes[] = { HAPVideoSerializeH264ProfileLevel(level) };
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Level,
                                          .value = { .bytes = levelBytes, .numBytes = sizeof levelBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    if (appendRateAndIFrame) {
        // Bit Rate.
        uint8_t bitRateBytes[] = { HAPExpandLittleUInt32(codecParameters->bitRate) };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_BitRate,
                                  .value = { .bytes = bitRateBytes, .numBytes = sizeof bitRateBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // I-Frame Interval.
        uint8_t iFrameIntervalBytes[] = { HAPExpandLittleUInt32(codecParameters->iFrameInterval) };
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_IFrameInterval,
                        .value = { .bytes = iFrameIntervalBytes, .numBytes = sizeof iFrameIntervalBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError SerializeVideoAttributes(const HAPVideoAttributes* attributes, HAPTLVWriter* responseWriter) {
    HAPPrecondition(attributes);
    HAPPrecondition(responseWriter);

    HAPError err;

    // Image Width.
    uint8_t imageWidthBytes[] = { HAPExpandLittleUInt16(attributes->width) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_VideoCodecAttribute_ImageWidth,
                              .value = { .bytes = imageWidthBytes, .numBytes = sizeof imageWidthBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Image Height.
    uint8_t imageHeightBytes[] = { HAPExpandLittleUInt16(attributes->height) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_VideoCodecAttribute_ImageHeight,
                              .value = { .bytes = imageHeightBytes, .numBytes = sizeof imageHeightBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Frame rate.
    uint8_t frameRateBytes[] = { attributes->maxFrameRate };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_VideoCodecAttribute_FrameRate,
                              .value = { .bytes = frameRateBytes, .numBytes = sizeof frameRateBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError SerializeVideoCodecRecordingConfiguration(
        HAPVideoCodecType codecType,
        const HAPVideoCodecParameters* codecParameters_,
        const HAPVideoAttributes* _Nullable const* _Nullable attributesList,
        HAPTLVWriter* responseWriter,
        uint8_t codecTypeTLVType,
        uint8_t codecParametersTLVType,
        uint8_t attributesTLVType,
        bool appendRateAndIFrame) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(HAPVideoCodecTypeIsSupportedForRecording(codecType));

    HAPError err;

    // Video Codec Type.
    uint8_t videoCodecTypeBytes[] = { SerializeVideoCodecType(codecType) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = codecTypeTLVType,
                              .value = { .bytes = videoCodecTypeBytes, .numBytes = sizeof videoCodecTypeBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Video Codec Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        switch (codecType) { // NOLINT(hicpp-multiway-paths-covered)
            case kHAPVideoCodecType_H264: {
                HAPPrecondition(codecParameters_);
                const HAPH264VideoCodecParameters* codecParameters = codecParameters_;

                err = SerializeSupportedH264VideoCodecParameters(codecParameters, &subWriter, appendRateAndIFrame);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
                break;
            }
            default:
                HAPFatalError();
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = codecParametersTLVType, .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Enumerate Video Attributes.
    if (attributesList) {
        for (size_t i = 0; attributesList[i]; i++) {
            const HAPVideoAttributes* attributes = attributesList[i];

            // Append separator if necessary.
            if (i) {
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Video Attributes.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                err = SerializeVideoAttributes(attributes, &subWriter);
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = attributesTLVType,
                                          .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
    }

    return kHAPError_None;
}

typedef struct {
    const HAPTLV8CharacteristicReadRequest* request;
    HAPTLVWriter* responseWriter;
    HAPError err;
} SupportedVideoRecordingConfigurationContext;

static void SupportedVideoRecordingConfigurationCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(context);
    SupportedCameraRecordingConfigurationContext* arguments = context;
    HAPPrecondition(arguments->request);
    HAPPrecondition(arguments->responseWriter);
    HAPTLVWriter* responseWriter = arguments->responseWriter;
    HAPPrecondition(!arguments->err);
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);

#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    {
        const HAPTLV8CharacteristicReadRequest* request = arguments->request;
        HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

        char logBytes[2048];
        HAPStringBuilder stringBuilder;
        HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
        HAPStringBuilderAppend(&stringBuilder, "Supported Video Recording Configuration:");
        if (supportedConfig->video.configurations) {
            for (size_t i = 0; supportedConfig->video.configurations[i]; i++) {
                const HAPCameraSupportedVideoCodecConfiguration* configuration =
                        supportedConfig->video.configurations[i];
                HAPPrecondition(HAPVideoCodecTypeIsSupportedForRecording(configuration->codecType));
                HAPStringBuilderAppend(&stringBuilder, "\n- Video Codec Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Video Codec Type: ");
                switch (configuration->codecType) {
                    case kHAPVideoCodecType_H264: {
                        const HAPH264VideoCodecParameters* codecParameters = configuration->codecParameters;
                        HAPStringBuilderAppend(
                                &stringBuilder, "%s", HAPVideoGetVideoCodecTypeDescription(configuration->codecType));
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Video Codec Parameters:");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - ProfileID: [");
                        HAPVideoStringBuilderAppendH264ProfileIDs(&stringBuilder, codecParameters->profile);
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Level: [");
                        HAPVideoStringBuilderAppendH264ProfileLevels(&stringBuilder, codecParameters->level);
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Packetization mode: [");
                        HAPVideoStringBuilderAppendH264PacketizationModes(
                                &stringBuilder, codecParameters->packetizationMode);
                        HAPStringBuilderAppend(&stringBuilder, "] (not sent to controller)");
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                HAPStringBuilderAppend(&stringBuilder, "\n  - Video Attributes:");
                if (configuration->attributes) {
                    for (size_t j = 0; configuration->attributes[j]; j++) {
                        const HAPVideoAttributes* attributes = configuration->attributes[j];
                        HAPStringBuilderAppend(
                                &stringBuilder,
                                "\n    - %4u x %4u @ %u fps",
                                attributes->width,
                                attributes->height,
                                attributes->maxFrameRate);
                    }
                }
            }
        }
        if (HAPStringBuilderDidOverflow(&stringBuilder)) {
            HAPLogCharacteristicError(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Logs were truncated.",
                    (const void*) camera);
        }
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] %s",
                (const void*) camera,
                HAPStringBuilderGetString(&stringBuilder));
    }
#endif

    // Enumerate Video Codec Configurations.
    if (supportedConfig->video.configurations) {
        for (size_t i = 0; supportedConfig->video.configurations[i]; i++) {
            const HAPCameraSupportedVideoCodecConfiguration* configuration = supportedConfig->video.configurations[i];

            // Append separator if necessary.
            if (i) {
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }

            // Video Codec Configuration.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                arguments->err = SerializeVideoCodecRecordingConfiguration(
                        configuration->codecType,
                        configuration->codecParameters,
                        configuration->attributes,
                        &subWriter,
                        kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_CodecType,
                        kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_CodecParameters,
                        kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_Attributes,
                        /* appendRateAndIFrame: */ false);
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }

                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementSupportedVideoRecordingConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedVideoRecordingConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(request->accessory->callbacks.camera.getSupportedRecordingConfiguration);
    HAPPrecondition(responseWriter);

    SupportedVideoRecordingConfigurationContext requestContext;
    HAPRawBufferZero(&requestContext, sizeof requestContext);
    requestContext.request = request;
    requestContext.responseWriter = responseWriter;
    requestContext.err = kHAPError_None;
    HAPCameraAccessoryGetSupportedRecordingConfiguration(
            server,
            request->service,
            request->accessory,
            SupportedVideoRecordingConfigurationCallback,
            &requestContext);
    if (requestContext.err) {
        HAPAssert(requestContext.err == kHAPError_OutOfResources);
        return requestContext.err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecRecordingType SerializeAudioCodecType(HAPAudioCodecType value) {
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForRecording(value));

    switch (value) {
        case kHAPAudioCodecType_AAC_ELD: {
            return kHAPCharacteristicValue_AudioCodecRecordingType_AAC_ELD;
        }
        case kHAPAudioCodecType_AAC_LC: {
            return kHAPCharacteristicValue_AudioCodecRecordingType_AAC_LC;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_Opus:
        case kHAPAudioCodecType_MSBC:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecBitRateControlMode
        SerializeAudioCodecBitRateControlMode(HAPAudioCodecBitRateControlMode value) {
    switch (value) {
        case kHAPAudioCodecBitRateControlMode_Variable:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable;
        case kHAPAudioCodecBitRateControlMode_Constant:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecRecordingSampleRate
        SerializeAudioCodecSampleRate(HAPAudioCodecSampleRate value) {
    HAPPrecondition(HAPAudioCodecSampleRateIsSupportedForRecording(value));

    switch (value) {
        case kHAPAudioCodecSampleRate_8KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_8KHz;
        case kHAPAudioCodecSampleRate_16KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_16KHz;
        case kHAPAudioCodecSampleRate_24KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_24KHz;
        case kHAPAudioCodecSampleRate_32KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_32KHz;
        case kHAPAudioCodecSampleRate_44_1KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_44_1KHz;
        case kHAPAudioCodecSampleRate_48KHz:
            return kHAPCharacteristicValue_AudioCodecRecordingSampleRate_48KHz;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
static HAPError SerializeAudioCodecConfiguration(
        HAPAudioCodecType codecType,
        const HAPAudioCodecParameters* codecParameters_,
        HAPTLVWriter* responseWriter,
        uint8_t codecTypeTLVType,
        uint8_t codecParametersTLVType,
        bool appendRate) {
    HAPPrecondition(responseWriter);
    HAPPrecondition(HAPAudioCodecTypeIsSupportedForRecording(codecType));

    HAPError err;

    // Audio Codec Type.
    uint8_t audioCodecTypeBytes[] = { SerializeAudioCodecType(codecType) };
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = codecTypeTLVType,
                              .value = { .bytes = audioCodecTypeBytes, .numBytes = sizeof audioCodecTypeBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Audio Codec Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        HAPPrecondition(codecParameters_);
        const HAPAudioCodecParameters* codecParameters = codecParameters_;

        // Audio Channels.
        uint8_t audioChannelsBytes[] = { codecParameters->numberOfChannels };
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_AudioCodecRecordingParameter_AudioChannels,
                                  .value = { .bytes = audioChannelsBytes, .numBytes = sizeof audioChannelsBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Enumerate Bit-rate Modes.
        {
            bool needsSeparator = false;
            HAPAudioCodecBitRateControlMode bitRateModes = codecParameters->bitRateMode;
            for (size_t i = 0; bitRateModes; i++) {
                HAPAudioCodecBitRateControlMode bitRateMode = (HAPAudioCodecBitRateControlMode)(1U << i);
                if (bitRateModes & bitRateMode) {
                    bitRateModes &= (HAPAudioCodecBitRateControlMode) ~bitRateMode;

                    // Append separator if necessary.
                    if (needsSeparator) {
                        err = HAPTLVWriterAppend(
                                &subWriter,
                                &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                                  .value = { .bytes = NULL, .numBytes = 0 } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    } else {
                        needsSeparator = true;
                    }

                    // Bit-rate Mode.
                    uint8_t bitRateModeBytes[] = { SerializeAudioCodecBitRateControlMode(bitRateMode) };
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRateMode,
                                    .value = { .bytes = bitRateModeBytes, .numBytes = sizeof bitRateModeBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                }
            }
        }

        // Enumerate Sample Rates.
        {
            bool needsSeparator = false;
            HAPAudioCodecSampleRate sampleRates = codecParameters->sampleRate;
            for (size_t i = 0; sampleRates; i++) {
                HAPAudioCodecSampleRate sampleRate = (HAPAudioCodecSampleRate)(1U << i);
                if (sampleRates & sampleRate) {
                    HAPPrecondition(HAPAudioCodecSampleRateIsSupportedForRecording(sampleRate));
                    sampleRates &= (HAPAudioCodecSampleRate) ~sampleRate;

                    // Append separator if necessary.
                    if (needsSeparator) {
                        err = HAPTLVWriterAppend(
                                &subWriter,
                                &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                                  .value = { .bytes = NULL, .numBytes = 0 } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    } else {
                        needsSeparator = true;
                    }

                    // Sample Rate.
                    uint8_t sampleRateBytes[] = { SerializeAudioCodecSampleRate(sampleRate) };
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) {
                                    .type = kHAPCharacteristicValue_AudioCodecRecordingParameter_SampleRate,
                                    .value = { .bytes = sampleRateBytes, .numBytes = sizeof sampleRateBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                }
            }
        }

        // Bit Rate.
        if (appendRate) {
            uint8_t bitRateBytes[] = { HAPExpandLittleUInt32(codecParameters->bitRate) };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRate,
                                      .value = { .bytes = bitRateBytes, .numBytes = sizeof bitRateBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = codecParametersTLVType, .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

typedef struct {
    const HAPTLV8CharacteristicReadRequest* request;
    HAPTLVWriter* responseWriter;
    HAPError err;
} SupportedAudioRecordingConfigurationContext;

static void SupportedAudioRecordingConfigurationCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(context);
    SupportedCameraRecordingConfigurationContext* arguments = context;
    HAPPrecondition(arguments->request);
    HAPPrecondition(arguments->responseWriter);
    HAPTLVWriter* responseWriter = arguments->responseWriter;
    HAPPrecondition(!arguments->err);
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);

#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    {
        const HAPTLV8CharacteristicReadRequest* request = arguments->request;
        HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

        char logBytes[2048];
        HAPStringBuilder stringBuilder;
        HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
        HAPStringBuilderAppend(&stringBuilder, "Supported Audio Recording Configuration:");
        if (supportedConfig->audio.configurations) {
            for (size_t i = 0; supportedConfig->audio.configurations[i]; i++) {
                const HAPCameraSupportedAudioCodecConfiguration* configuration =
                        supportedConfig->audio.configurations[i];
                HAPPrecondition(HAPAudioCodecTypeIsSupportedForRecording(configuration->codecType));
                HAPStringBuilderAppend(&stringBuilder, "\n- Audio Codec Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Codec Type: ");
                switch (configuration->codecType) {
                    case kHAPAudioCodecType_AAC_LC: {
                        HAPStringBuilderAppend(&stringBuilder, "AAC-LC");
                    }
                        goto logAudioCodecParameters;
                    case kHAPAudioCodecType_AAC_ELD: {
                        HAPStringBuilderAppend(&stringBuilder, "AAC-ELD");
                    }
                        goto logAudioCodecParameters;
                    case kHAPAudioCodecType_PCMU:
                    case kHAPAudioCodecType_PCMA:
                    case kHAPAudioCodecType_Opus:
                    case kHAPAudioCodecType_MSBC:
                    case kHAPAudioCodecType_AMR:
                    case kHAPAudioCodecType_AMR_WB: {
                    }
                        HAPFatalError();
                    logAudioCodecParameters : {
                        const HAPAudioCodecParameters* codecParameters = configuration->codecParameters;
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Audio Codec Parameters:");
                        HAPStringBuilderAppend(
                                &stringBuilder, "\n    - Audio Channels: %u", codecParameters->numberOfChannels);
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Bit-Rate Mode: [");
                        {
                            bool needsSeparator = false;
                            if (codecParameters->bitRateMode & (uint8_t) kHAPAudioCodecBitRateControlMode_Variable) {
                                HAPStringBuilderAppend(&stringBuilder, "Variable bit-rate");
                                needsSeparator = true;
                            }
                            if (codecParameters->bitRateMode & (uint8_t) kHAPAudioCodecBitRateControlMode_Constant) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "Constant bit-rate");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Sample Rate: [");
                        {
                            HAPAudioCodecSampleRate allSupportedValues =
                                    kHAPAudioCodecSampleRate_8KHz + kHAPAudioCodecSampleRate_16KHz +
                                    kHAPAudioCodecSampleRate_24KHz + kHAPAudioCodecSampleRate_32KHz +
                                    kHAPAudioCodecSampleRate_44_1KHz + kHAPAudioCodecSampleRate_48KHz;
                            HAPPrecondition(
                                    !(codecParameters->sampleRate & (HAPAudioCodecSampleRate) ~allSupportedValues));
                            bool needsSeparator = false;
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_8KHz) {
                                HAPStringBuilderAppend(&stringBuilder, "8KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_16KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "16KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_24KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "24KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_32KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "32KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_44_1KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "44.1KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & (uint8_t) kHAPAudioCodecSampleRate_48KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "48KHz");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        break;
                    }
                    default:
                        HAPFatalError();
                }
            }
        }
        if (HAPStringBuilderDidOverflow(&stringBuilder)) {
            HAPLogCharacteristicError(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Logs were truncated.",
                    (const void*) camera);
        }
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] %s",
                (const void*) camera,
                HAPStringBuilderGetString(&stringBuilder));
    }
#endif

    // Enumerate Audio Codec Configurations.
    if (supportedConfig->audio.configurations) {
        for (size_t i = 0; supportedConfig->audio.configurations[i]; i++) {
            const HAPCameraSupportedAudioCodecConfiguration* configuration = supportedConfig->audio.configurations[i];

            // Append separator if necessary.
            if (i) {
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) { .type = kHAPCameraEventRecordingManagement_TLVType_Separator,
                                          .value = { .bytes = NULL, .numBytes = 0 } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }

            // Audio Codec Configuration.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                arguments->err = SerializeAudioCodecConfiguration(
                        configuration->codecType,
                        configuration->codecParameters,
                        &subWriter,
                        kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration_CodecType,
                        kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration_CodecParameters,
                        /* appendRate: */ false);
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }

                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                arguments->err = HAPTLVWriterAppend(
                        responseWriter,
                        &(const HAPTLV) {
                                .type = kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration,
                                .value = { .bytes = bytes, .numBytes = numBytes } });
                if (arguments->err) {
                    HAPAssert(arguments->err == kHAPError_OutOfResources);
                    return;
                }
            }
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementSupportedAudioRecordingConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedAudioRecordingConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(request->accessory->callbacks.camera.getSupportedRecordingConfiguration);
    HAPPrecondition(responseWriter);

    SupportedAudioRecordingConfigurationContext requestContext;
    HAPRawBufferZero(&requestContext, sizeof requestContext);
    requestContext.request = request;
    requestContext.responseWriter = responseWriter;
    requestContext.err = kHAPError_None;
    HAPCameraAccessoryGetSupportedRecordingConfiguration(
            server,
            request->service,
            request->accessory,
            SupportedAudioRecordingConfigurationCallback,
            &requestContext);
    if (requestContext.err) {
        HAPAssert(requestContext.err == kHAPError_OutOfResources);
        return requestContext.err;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool AreRecordingConfigurationsEqual(
        const HAPCameraRecordingConfiguration* config1,
        const HAPCameraRecordingConfiguration* config2) {
    HAPPrecondition(config1);
    HAPPrecondition(config2);

    // Selected Recording Parameters.
    if (config1->recording.prebufferDuration != config2->recording.prebufferDuration) {
        return false;
    }
    if (config1->recording.eventTriggerTypes != config2->recording.eventTriggerTypes) {
        return false;
    }
    if (config1->recording.containerConfiguration.containerType !=
        config2->recording.containerConfiguration.containerType) {
        return false;
    }
    switch (config1->recording.containerConfiguration.containerType) {
        case kHAPMediaContainerType_FragmentedMP4: {
            if (config1->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration !=
                config2->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration) {
                return false;
            }
            break;
        }
        default:
            HAPFatalError();
    }

    // Selected Video Parameters.
    if (config1->video.codecType != config2->video.codecType) {
        return false;
    }
    switch (config1->video.codecType) {
        case kHAPVideoCodecType_H264: {
            if (config1->video.codecParameters.h264.profile != config2->video.codecParameters.h264.profile) {
                return false;
            }
            if (config1->video.codecParameters.h264.level != config2->video.codecParameters.h264.level) {
                return false;
            }
            if (config1->video.codecParameters.h264.packetizationMode !=
                config2->video.codecParameters.h264.packetizationMode) {
                return false;
            }
            if (config1->video.codecParameters.h264.bitRate != config2->video.codecParameters.h264.bitRate) {
                return false;
            }
            if (config1->video.codecParameters.h264.iFrameInterval !=
                config2->video.codecParameters.h264.iFrameInterval) {
                return false;
            }
            break;
        }
        default:
            HAPFatalError();
    }
    if (config1->video.attributes.width != config2->video.attributes.width) {
        return false;
    }
    if (config1->video.attributes.height != config2->video.attributes.height) {
        return false;
    }
    if (config1->video.attributes.maxFrameRate != config2->video.attributes.maxFrameRate) {
        return false;
    }

    // Selected Audio Parameters.
    if (config1->audio.codecType != config2->audio.codecType) {
        return false;
    }
    if (config1->audio.codecParameters.numberOfChannels != config2->audio.codecParameters.numberOfChannels) {
        return false;
    }
    if (config1->audio.codecParameters.sampleRate != config2->audio.codecParameters.sampleRate) {
        return false;
    }
    if (config1->audio.codecParameters.bitRateMode != config2->audio.codecParameters.bitRateMode) {
        return false;
    }
    if (config1->audio.codecParameters.bitRate != config2->audio.codecParameters.bitRate) {
        return false;
    }

    return true;
}

#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
static void LogSelectedRecordingConfiguration(
        const HAPAccessory* accessory,
        const HAPService* service HAP_UNUSED,
        const HAPCharacteristic* characteristic,
        HAPPlatformCameraRef camera,
        const HAPCameraRecordingConfiguration* selectedConfig) {
    HAPPrecondition(selectedConfig);

    char logBytes[2048];
    HAPStringBuilder stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);

    HAPStringBuilderAppend(&stringBuilder, "Selected Camera Recording Configuration:");

    // Selected Camera Recording Parameters.
    {
        HAPStringBuilderAppend(&stringBuilder, "\n- Selected Camera Recording Parameters:");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n  - Prebuffer Duration: %llums",
                // NOLINTNEXTLINE(google-readability-casting)
                (unsigned long long) (selectedConfig->recording.prebufferDuration / HAPMillisecond));
        HAPStringBuilderAppend(&stringBuilder, "\n  - Event Trigger Type:");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n    - Motion: %s",
                (selectedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Motion) ? "Yes" :
                                                                                                               "No");
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n    - Doorbell: %s",
                (selectedConfig->recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Doorbell) ? "Yes" :
                                                                                                                 "No");
        HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Media Container Configuration:");
        HAPStringBuilderAppend(&stringBuilder, "\n    - Media Container Type: ");
        switch (selectedConfig->recording.containerConfiguration.containerType) {
            case kHAPMediaContainerType_FragmentedMP4: {
                const HAPFragmentedMP4MediaContainerParameters* containerParameters =
                        &selectedConfig->recording.containerConfiguration.containerParameters.fragmentedMP4;
                HAPStringBuilderAppend(&stringBuilder, "Fragmented MP4");
                HAPStringBuilderAppend(&stringBuilder, "\n    - Media Container Parameters:");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n      - Fragment Duration: %lums",
                        (unsigned long) containerParameters->fragmentDuration);
                break;
            }
            default:
                HAPFatalError();
        }
    }

    // Selected Video Parameters.
    {
        HAPPrecondition(HAPVideoCodecTypeIsSupportedForRecording(selectedConfig->video.codecType));
        HAPStringBuilderAppend(&stringBuilder, "\n- Selected Video Parameters:");
        HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Video Codec Type: ");
        switch (selectedConfig->video.codecType) {
            case kHAPVideoCodecType_H264: {
                const HAPH264VideoCodecParameters* codecParameters = &selectedConfig->video.codecParameters.h264;
                HAPStringBuilderAppend(
                        &stringBuilder, "%s", HAPVideoGetVideoCodecTypeDescription(selectedConfig->video.codecType));
                HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Video Codec Parameters:");
                HAPStringBuilderAppend(&stringBuilder, "\n    - ProfileID: ");
                HAPStringBuilderAppend(
                        &stringBuilder, "%s", HAPVideoGetH264ProfileIDDescription(codecParameters->profile));
                HAPStringBuilderAppend(&stringBuilder, "\n    - Level: ");
                HAPStringBuilderAppend(
                        &stringBuilder, "%s", HAPVideoGetH264ProfileLevelDescription(codecParameters->level));
                HAPStringBuilderAppend(&stringBuilder, "\n    - Packetization mode: ");
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "%s",
                        HAPVideoGetH264PacketizationModeDescription(codecParameters->packetizationMode));

                HAPStringBuilderAppend(
                        &stringBuilder, "\n    - Bit Rate: %lu kbit/s", (unsigned long) codecParameters->bitRate);
                HAPStringBuilderAppend(
                        &stringBuilder,
                        "\n    - I-Frame Interval: %lums",
                        (unsigned long) codecParameters->iFrameInterval);
                break;
            }
            default:
                HAPFatalError();
        }
        HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Video Attributes:");
        const HAPVideoAttributes* attributes = &selectedConfig->video.attributes;
        HAPStringBuilderAppend(
                &stringBuilder,
                "\n    - %4u x %4u @ %u fps",
                attributes->width,
                attributes->height,
                attributes->maxFrameRate);
    }

    // Selected Audio Parameters.
    {
        HAPPrecondition(HAPAudioCodecTypeIsSupportedForRecording(selectedConfig->audio.codecType));
        HAPStringBuilderAppend(&stringBuilder, "\n- Selected Audio Parameters:");
        HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Audio Codec Type: ");
        switch (selectedConfig->audio.codecType) {
            case kHAPAudioCodecType_AAC_LC: {
                HAPStringBuilderAppend(&stringBuilder, "AAC-LC");
            }
                goto logAudioCodecParameters;
            case kHAPAudioCodecType_AAC_ELD: {
                HAPStringBuilderAppend(&stringBuilder, "AAC-ELD");
            }
                goto logAudioCodecParameters;
            case kHAPAudioCodecType_PCMU:
            case kHAPAudioCodecType_PCMA:
            case kHAPAudioCodecType_Opus:
            case kHAPAudioCodecType_MSBC:
            case kHAPAudioCodecType_AMR:
            case kHAPAudioCodecType_AMR_WB: {
            }
                HAPFatalError();
            logAudioCodecParameters : {
                const HAPAudioCodecParameters* codecParameters = &selectedConfig->audio.codecParameters;
                HAPPrecondition(HAPAudioCodecSampleRateIsSupportedForRecording(codecParameters->sampleRate));
                HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Audio Codec Parameters:");
                HAPStringBuilderAppend(&stringBuilder, "\n    - Audio Channels: %u", codecParameters->numberOfChannels);
                HAPStringBuilderAppend(&stringBuilder, "\n    - Bit-Rate Mode: ");
                switch (codecParameters->bitRateMode) {
                    case kHAPAudioCodecBitRateControlMode_Variable: {
                        HAPStringBuilderAppend(&stringBuilder, "Variable bit-rate");
                        break;
                    }
                    case kHAPAudioCodecBitRateControlMode_Constant: {
                        HAPStringBuilderAppend(&stringBuilder, "Constant bit-rate");
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                HAPStringBuilderAppend(&stringBuilder, "\n    - Sample Rate: ");
                switch (codecParameters->sampleRate) {
                    case kHAPAudioCodecSampleRate_8KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "8KHz");
                        break;
                    }
                    case kHAPAudioCodecSampleRate_16KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "16KHz");
                        break;
                    }
                    case kHAPAudioCodecSampleRate_24KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "24KHz");
                        break;
                    }
                    case kHAPAudioCodecSampleRate_32KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "32KHz");
                        break;
                    }
                    case kHAPAudioCodecSampleRate_44_1KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "44.1KHz");
                        break;
                    }
                    case kHAPAudioCodecSampleRate_48KHz: {
                        HAPStringBuilderAppend(&stringBuilder, "48KHz");
                        break;
                    }
                    default:
                        HAPFatalError();
                }
                HAPStringBuilderAppend(
                        &stringBuilder, "\n    - Bit Rate: %lu kbit/s", (unsigned long) codecParameters->bitRate);
                break;
            }
            default:
                HAPFatalError();
        }
    }
    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogCharacteristicError(
                &logObject, characteristic, service, accessory, "[%p] Logs were truncated.", (const void*) camera);
    }
    HAPLogCharacteristicInfo(
            &logObject,
            characteristic,
            service,
            accessory,
            "[%p] %s",
            (const void*) camera,
            HAPStringBuilderGetString(&stringBuilder));
}
#endif

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedCameraRecordingConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(request->accessory->callbacks.camera.getSupportedRecordingConfiguration);
    HAPPrecondition(responseWriter);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    // Get Selected Camera Recording Configuration.
    HAPCameraRecordingConfiguration selectedConfig;
    bool found;
    err = HAPPlatformCameraGetRecordingConfiguration(camera, &found, &selectedConfig);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Failed to get Selected Camera Recording Configuration: %u.",
                (const void*) camera,
                err);
        return err;
    }
    if (!found) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Selected Camera Recording Configuration not found.",
                (const void*) camera);
        return kHAPError_Unknown;
    }
    if (!HAPCameraAccessoryIsRecordingConfigurationSupported(
                server, request->service, request->accessory, &selectedConfig)) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Selected Camera Recording Configuration is not supported. "
                "Use HAPCameraAccessoryHandleSupportedRecordingConfigurationChange to indicate change of support.",
                (const void*) camera);
        return kHAPError_Unknown;
    }

// Log Selected Camera Recording Configuration.
#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    LogSelectedRecordingConfiguration(
            request->accessory, request->service, request->characteristic, camera, &selectedConfig);
#endif

    // Selected Camera Recording Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        // Prebuffer Duration.
        uint8_t prebufferDurationBytes[] = { HAPExpandLittleUInt32(
                (uint32_t)(selectedConfig.recording.prebufferDuration / HAPMillisecond)) };
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_PrebufferDuration,
                        .value = { .bytes = prebufferDurationBytes, .numBytes = sizeof prebufferDurationBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Event Trigger Type.
        uint64_t rawEventTriggerType = 0;
        HAPAssert(sizeof(HAPCameraEventTriggerTypes) == sizeof(uint8_t));
        if (selectedConfig.recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Motion) {
            rawEventTriggerType |= (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Motion;
        }
        if (selectedConfig.recording.eventTriggerTypes & (uint8_t) kHAPCameraEventTriggerTypes_Doorbell) {
            rawEventTriggerType |= (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell;
        }
        uint8_t eventTriggerTypeBytes[] = { HAPExpandLittleUInt64(rawEventTriggerType) };
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_EventTriggerType,
                        .value = { .bytes = eventTriggerTypeBytes, .numBytes = sizeof eventTriggerTypeBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Selected Media Container Configuration.
        {
            HAPTLVWriter sub2Writer;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
            }

            err = SerializeMediaContainerConfiguration(
                    selectedConfig.recording.containerConfiguration.containerType,
                    &selectedConfig.recording.containerConfiguration.containerParameters,
                    &sub2Writer,
                    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration_Type,
                    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration_Parameters);
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }

            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_ContainerConfiguration,
                            .value = { .bytes = bytes, .numBytes = numBytes } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Selected Video Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        const HAPVideoAttributes* attributesList[] = { &selectedConfig.video.attributes, NULL };
        err = SerializeVideoCodecRecordingConfiguration(
                selectedConfig.video.codecType,
                &selectedConfig.video.codecParameters,
                attributesList,
                &subWriter,
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecType,
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecParameters,
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_Attributes,
                /* appendRateAndIFrame: */ true);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Selected Audio Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        err = SerializeAudioCodecConfiguration(
                selectedConfig.audio.codecType,
                &selectedConfig.audio.codecParameters,
                &subWriter,
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecType,
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecParameters,
                /* appendRate: */ true);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Reads a UInt16 value from a buffer containing its corresponding little-endian representation
 * with optional 0-padding removed.
 *
 * @param      bytes_               Buffer to read from.
 * @param      numBytes             Length of buffer. Must be at most 2 bytes.
 *
 * @return Value that has been read.
 */
HAP_RESULT_USE_CHECK
static uint16_t HAPReadLittleUIntMax16(const void* bytes_, size_t numBytes) {
    HAPPrecondition(bytes_);
    const uint8_t* bytes = bytes_;
    HAPPrecondition(numBytes <= sizeof(uint16_t));

    uint16_t value = 0;
    for (size_t i = 0; i < numBytes; i++) {
        value |= (uint16_t)((uint16_t) bytes[i] << (i * 8));
    }
    return value;
}

/**
 * Reads a UInt32 value from a buffer containing its corresponding little-endian representation
 * with optional 0-padding removed.
 *
 * @param      bytes_               Buffer to read from.
 * @param      numBytes             Length of buffer. Must be at most 4 bytes.
 *
 * @return Value that has been read.
 */
HAP_RESULT_USE_CHECK
static uint32_t HAPReadLittleUIntMax32(const void* bytes_, size_t numBytes) {
    HAPPrecondition(bytes_);
    const uint8_t* bytes = bytes_;
    HAPPrecondition(numBytes <= sizeof(uint32_t));

    uint32_t value = 0;
    for (size_t i = 0; i < numBytes; i++) {
        value |= (uint32_t)((uint32_t) bytes[i] << (i * 8)); // NOLINT(google-readability-casting)
    }
    return value;
}

/**
 * Reads a UInt64 value from a buffer containing its corresponding little-endian representation
 * with optional 0-padding removed.
 *
 * @param      bytes_               Buffer to read from.
 * @param      numBytes             Length of buffer. Must be at most 8 bytes.
 *
 * @return Value that has been read.
 */
HAP_RESULT_USE_CHECK
static uint64_t HAPReadLittleUIntMax64(const void* bytes_, size_t numBytes) {
    HAPPrecondition(bytes_);
    const uint8_t* bytes = bytes_;
    HAPPrecondition(numBytes <= sizeof(uint64_t));

    uint64_t value = 0;
    for (size_t i = 0; i < numBytes; i++) {
        value |= (uint64_t)((uint64_t) bytes[i] << (i * 8)); // NOLINT(google-readability-casting)
    }
    return value;
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_MediaContainerTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_MediaContainerType));
    switch ((HAPCharacteristicValue_MediaContainerType) value) {
        case kHAPCharacteristicValue_MediaContainerType_FragmentedMP4: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_CameraRecording_VideoCodecTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_CameraRecording_VideoCodecType));
    switch ((HAPCharacteristicValue_CameraRecording_VideoCodecType) value) {
        case kHAPCharacteristicValue_CameraRecording_VideoCodecType_H264: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_AudioCodecRecordingTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_AudioCodecRecordingType));
    switch ((HAPCharacteristicValue_AudioCodecRecordingType) value) {
        case kHAPCharacteristicValue_AudioCodecRecordingType_AAC_LC:
        case kHAPCharacteristicValue_AudioCodecRecordingType_AAC_ELD: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_AudioCodecBitRateControlModeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_AudioCodecBitRateControlMode));
    switch ((HAPCharacteristicValue_AudioCodecBitRateControlMode) value) {
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable:
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_AudioCodecRecordingSampleRateIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_AudioCodecRecordingSampleRate));
    switch ((HAPCharacteristicValue_AudioCodecRecordingSampleRate) value) {
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_8KHz:
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_16KHz:
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_24KHz:
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_32KHz:
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_44_1KHz:
        case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_48KHz: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
// NOLINTNEXTLINE(readability-function-size)
HAPError HAPHandleCameraEventRecordingManagementSelectedCameraRecordingConfigurationWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedCameraRecordingConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPrecondition(requestReader);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    // Prepare Selected Camera Recording Configuration.
    HAPCameraRecordingConfiguration selectedConfig;
    HAPRawBufferZero(&selectedConfig, sizeof selectedConfig);

    HAPTLV selectedCameraRecordingParametersTLV, selectedVideoParametersTLV, selectedAudioParametersTLV;
    selectedCameraRecordingParametersTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording;
    selectedVideoParametersTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video;
    selectedAudioParametersTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio;
    err = HAPTLVReaderGetAll(
            requestReader,
            (HAPTLV* const[]) { &selectedCameraRecordingParametersTLV,
                                &selectedVideoParametersTLV,
                                &selectedAudioParametersTLV,
                                NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Camera Recording Parameters.
    {
        if (!selectedCameraRecordingParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Camera Recording Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedCameraRecordingParametersTLV.value.bytes,
                selectedCameraRecordingParametersTLV.value.numBytes);
        HAPTLV prebufferDurationTLV, eventTriggerTypeTLV, selectedMediaContainerConfigurationTLV;
        prebufferDurationTLV.type =
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_PrebufferDuration;
        eventTriggerTypeTLV.type =
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_EventTriggerType;
        selectedMediaContainerConfigurationTLV.type =
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_ContainerConfiguration;
        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) {
                        &prebufferDurationTLV, &eventTriggerTypeTLV, &selectedMediaContainerConfigurationTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Prebuffer Duration.
        if (!prebufferDurationTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Prebuffer Duration missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (prebufferDurationTLV.value.numBytes > sizeof(uint32_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Prebuffer Duration has invalid length (%zu).",
                    (const void*) camera,
                    prebufferDurationTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint32_t rawPrebufferDuration =
                HAPReadLittleUIntMax32(prebufferDurationTLV.value.bytes, prebufferDurationTLV.value.numBytes);
        selectedConfig.recording.prebufferDuration = rawPrebufferDuration * HAPMillisecond;

        // Event Trigger Type.
        if (!eventTriggerTypeTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Event Trigger Type missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (eventTriggerTypeTLV.value.numBytes > sizeof(uint64_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Event Trigger Type has invalid length (%zu).",
                    (const void*) camera,
                    eventTriggerTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint64_t rawEventTriggerType =
                HAPReadLittleUIntMax64(eventTriggerTypeTLV.value.bytes, eventTriggerTypeTLV.value.numBytes);
        if (rawEventTriggerType & (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Motion) {
            rawEventTriggerType &= ~(uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Motion;
            selectedConfig.recording.eventTriggerTypes |=
                    (HAPCameraEventTriggerTypes) kHAPCameraEventTriggerTypes_Motion;
        }
        if (rawEventTriggerType & (uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell) {
            rawEventTriggerType &= ~(uint64_t) kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell;
            selectedConfig.recording.eventTriggerTypes |=
                    (HAPCameraEventTriggerTypes) kHAPCameraEventTriggerTypes_Doorbell;
        }
        if (rawEventTriggerType) {
            HAPLogCharacteristicError(
                    &kHAPLog_Default,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "Unexpected Event Trigger Type: 0x%016llX.",
                    (unsigned long long) rawEventTriggerType);
            return kHAPError_InvalidData;
        }

        // Selected Media Container Configuration.
        if (!selectedMediaContainerConfigurationTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Media Container Configuration missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader sub2Reader;
        HAPTLVReaderCreate(
                &sub2Reader,
                (void*) (uintptr_t) selectedMediaContainerConfigurationTLV.value.bytes,
                selectedMediaContainerConfigurationTLV.value.numBytes);
        HAPTLV mediaContainerTypeTLV, mediaContainerParametersTLV;
        mediaContainerTypeTLV.type = 1;
        mediaContainerParametersTLV.type = 2;
        err = HAPTLVReaderGetAll(
                &sub2Reader, (HAPTLV* const[]) { &mediaContainerTypeTLV, &mediaContainerParametersTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Media Container Type / Media Container Parameters.
        if (!mediaContainerTypeTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Media Container Type missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (mediaContainerTypeTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Media Container Type has invalid length (%zu).",
                    (const void*) camera,
                    mediaContainerTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawMediaContainerType =
                HAPReadUIntMax8(mediaContainerTypeTLV.value.bytes, mediaContainerTypeTLV.value.numBytes);
        if (!HAPCharacteristicValue_MediaContainerTypeIsValid(rawMediaContainerType)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Media Container Type invalid: %u.",
                    (const void*) camera,
                    rawMediaContainerType);
            return kHAPError_InvalidData;
        }
        if (!mediaContainerParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Media Container Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader sub3Reader;
        HAPTLVReaderCreate(
                &sub3Reader,
                (void*) (uintptr_t) mediaContainerParametersTLV.value.bytes,
                mediaContainerParametersTLV.value.numBytes);
        switch ((HAPCharacteristicValue_MediaContainerType) rawMediaContainerType) {
            case kHAPCharacteristicValue_MediaContainerType_FragmentedMP4: {
                // Fragmented MP4.
                selectedConfig.recording.containerConfiguration.containerType = kHAPMediaContainerType_FragmentedMP4;

                // Media Container Parameters.
                HAPTLV fragmentDurationTLV;
                fragmentDurationTLV.type = 1;
                err = HAPTLVReaderGetAll(&sub3Reader, (HAPTLV* const[]) { &fragmentDurationTLV, NULL });
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // Fragment Duration.
                if (!fragmentDurationTLV.value.bytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] Fragment Duration missing.",
                            (const void*) camera);
                    return kHAPError_InvalidData;
                }
                if (fragmentDurationTLV.value.numBytes > sizeof(uint32_t)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] Fragment Duration has invalid length (%zu).",
                            (const void*) camera,
                            fragmentDurationTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint32_t rawFragmentDuration =
                        HAPReadLittleUIntMax32(fragmentDurationTLV.value.bytes, fragmentDurationTLV.value.numBytes);
                selectedConfig.recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration =
                        rawFragmentDuration;
                break;
            }
            default:
                HAPFatalError();
        }
    }

    // Selected Video Parameters.
    {
        if (!selectedVideoParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Video Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedVideoParametersTLV.value.bytes,
                selectedVideoParametersTLV.value.numBytes);
        HAPTLV selectedVideoCodecTypeTLV, selectedVideoCodecParametersTLV, selectedVideoAttributesTLV;
        selectedVideoCodecTypeTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecType;
        selectedVideoCodecParametersTLV.type =
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecParameters;
        selectedVideoAttributesTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_Attributes;
        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) { &selectedVideoCodecTypeTLV,
                                    &selectedVideoCodecParametersTLV,
                                    &selectedVideoAttributesTLV,
                                    NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Selected Video Codec Type / Selected Video Codec Parameters.
        if (!selectedVideoCodecTypeTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Video Codec Type missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (selectedVideoCodecTypeTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Video Codec Type has invalid length (%zu).",
                    (const void*) camera,
                    selectedVideoCodecTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawVideoCodecType =
                HAPReadUIntMax8(selectedVideoCodecTypeTLV.value.bytes, selectedVideoCodecTypeTLV.value.numBytes);
        if (!HAPCharacteristicValue_CameraRecording_VideoCodecTypeIsValid(rawVideoCodecType)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Video Codec Type invalid: %u.",
                    (const void*) camera,
                    rawVideoCodecType);
            return kHAPError_InvalidData;
        }
        if (!selectedVideoCodecParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Video Codec Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader sub2Reader;
        HAPTLVReaderCreate(
                &sub2Reader,
                (void*) (uintptr_t) selectedVideoCodecParametersTLV.value.bytes,
                selectedVideoCodecParametersTLV.value.numBytes);
        switch ((HAPCharacteristicValue_CameraRecording_VideoCodecType) rawVideoCodecType) {
            case kHAPCharacteristicValue_CameraRecording_VideoCodecType_H264: {
                // H.264.
                selectedConfig.video.codecType = kHAPVideoCodecType_H264;
                HAPH264VideoCodecParameters* codecParameters = &selectedConfig.video.codecParameters.h264;

                // Selected Video Codec Parameters.
                HAPTLV profileIDTLV, levelTLV, bitRateTLV, iFrameIntervalTLV;
                profileIDTLV.type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Profile;
                levelTLV.type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Level;
                bitRateTLV.type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_BitRate;
                iFrameIntervalTLV.type = kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_IFrameInterval;
                err = HAPTLVReaderGetAll(
                        &sub2Reader,
                        (HAPTLV* const[]) { &profileIDTLV, &levelTLV, &bitRateTLV, &iFrameIntervalTLV, NULL });
                if (err) {
                    HAPAssert(err == kHAPError_InvalidData);
                    return err;
                }

                // ProfileID.
                if (!profileIDTLV.value.bytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 ProfileID missing.",
                            (const void*) camera);
                    return kHAPError_InvalidData;
                }
                if (profileIDTLV.value.numBytes > sizeof(uint8_t)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 ProfileID has invalid length (%zu).",
                            (const void*) camera,
                            profileIDTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint8_t rawProfileID = HAPReadUIntMax8(profileIDTLV.value.bytes, profileIDTLV.value.numBytes);
                if (!HAPVideo_HAPCharacteristicValue_H264VideoCodecProfileIsValid(rawProfileID)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 ProfileID invalid: %u.",
                            (const void*) camera,
                            rawProfileID);
                    return kHAPError_InvalidData;
                }
                err = HAPVideoTryParseH264ProfileID(&codecParameters->profile, rawProfileID);
                if (err) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] Unable to parse H.264 ProfileID: %u.",
                            (const void*) camera,
                            rawProfileID);
                    return kHAPError_InvalidData;
                }

                // Level.
                if (!levelTLV.value.bytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 Level missing.",
                            (const void*) camera);
                    return kHAPError_InvalidData;
                }
                if (levelTLV.value.numBytes > sizeof(uint8_t)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 Level has invalid length (%zu).",
                            (const void*) camera,
                            levelTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint8_t rawLevel = HAPReadUIntMax8(levelTLV.value.bytes, levelTLV.value.numBytes);
                if (!HAPVideo_HAPCharacteristicValue_H264VideoCodecProfileLevelIsValid(rawLevel)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 Level invalid: %u.",
                            (const void*) camera,
                            rawLevel);
                    return kHAPError_InvalidData;
                }
                err = HAPVideoTryParseH264ProfileLevel(&codecParameters->level, rawLevel);
                if (err) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] Unable to parse H.264 Level: %u.",
                            (const void*) camera,
                            rawLevel);
                    return kHAPError_InvalidData;
                }

                // Packetization Mode.
                codecParameters->packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved;

                // Bit Rate.
                if (!bitRateTLV.value.bytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 Bit Rate missing.",
                            (const void*) camera);
                    return kHAPError_InvalidData;
                }
                if (bitRateTLV.value.numBytes > sizeof(uint32_t)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 Bit Rate has invalid length (%zu).",
                            (const void*) camera,
                            bitRateTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint32_t rawBitRate = HAPReadLittleUIntMax32(bitRateTLV.value.bytes, bitRateTLV.value.numBytes);
                codecParameters->bitRate = rawBitRate;

                // I-Frame Interval.
                if (!iFrameIntervalTLV.value.bytes) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 I-Frame Interval missing.",
                            (const void*) camera);
                    return kHAPError_InvalidData;
                }
                if (iFrameIntervalTLV.value.numBytes > sizeof(uint32_t)) {
                    HAPLogCharacteristic(
                            &logObject,
                            request->characteristic,
                            request->service,
                            request->accessory,
                            "[%p] H.264 I-Frame Interval has invalid length (%zu).",
                            (const void*) camera,
                            iFrameIntervalTLV.value.numBytes);
                    return kHAPError_InvalidData;
                }
                uint32_t rawIFrameInterval =
                        HAPReadLittleUIntMax32(iFrameIntervalTLV.value.bytes, iFrameIntervalTLV.value.numBytes);
                codecParameters->iFrameInterval = rawIFrameInterval;
                break;
            }
            default:
                HAPFatalError();
        }

        // Selected Video Attributes.
        {
            HAPVideoAttributes* attributes = &selectedConfig.video.attributes;

            if (!selectedVideoAttributesTLV.value.bytes) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Selected Video Attributes missing.",
                        (const void*) camera);
                return kHAPError_InvalidData;
            }
            HAPTLVReader sub3Reader;
            HAPTLVReaderCreate(
                    &sub3Reader,
                    (void*) (uintptr_t) selectedVideoAttributesTLV.value.bytes,
                    selectedVideoAttributesTLV.value.numBytes);
            HAPTLV imageWidthTLV, imageHeightTLV, frameRateTLV;
            imageWidthTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_ImageWidth;
            imageHeightTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_ImageHeight;
            frameRateTLV.type = kHAPCharacteristicValue_VideoCodecAttribute_FrameRate;
            err = HAPTLVReaderGetAll(
                    &sub3Reader, (HAPTLV* const[]) { &imageWidthTLV, &imageHeightTLV, &frameRateTLV, NULL });
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                return err;
            }

            // Image Width.
            if (!imageWidthTLV.value.bytes) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Image Width missing.",
                        (const void*) camera);
                return kHAPError_InvalidData;
            }
            if (imageWidthTLV.value.numBytes > sizeof(uint16_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Image Width has invalid length (%zu).",
                        (const void*) camera,
                        imageWidthTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            uint16_t rawImageWidth = HAPReadLittleUIntMax16(imageWidthTLV.value.bytes, imageWidthTLV.value.numBytes);
            attributes->width = rawImageWidth;

            // Image Height.
            if (!imageHeightTLV.value.bytes) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Image Height missing.",
                        (const void*) camera);
                return kHAPError_InvalidData;
            }
            if (imageHeightTLV.value.numBytes > sizeof(uint16_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Image Height has invalid length (%zu).",
                        (const void*) camera,
                        imageHeightTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            uint16_t rawImageHeight = HAPReadLittleUIntMax16(imageHeightTLV.value.bytes, imageHeightTLV.value.numBytes);
            attributes->height = rawImageHeight;

            // Frame rate.
            if (!frameRateTLV.value.bytes) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Frame rate missing.",
                        (const void*) camera);
                return kHAPError_InvalidData;
            }
            if (frameRateTLV.value.numBytes > sizeof(uint8_t)) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Frame rate has invalid length (%zu).",
                        (const void*) camera,
                        frameRateTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            uint8_t rawFrameRate = HAPReadUIntMax8(frameRateTLV.value.bytes, frameRateTLV.value.numBytes);
            if (rawFrameRate == 0) {
                HAPLogCharacteristic(
                        &logObject,
                        request->characteristic,
                        request->service,
                        request->accessory,
                        "[%p] Frame rate is zero.",
                        (const void*) camera);
                return kHAPError_InvalidData;
            }
            attributes->maxFrameRate = rawFrameRate;
        }
    }

    // Selected Audio Parameters.
    {
        if (!selectedAudioParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Audio Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedAudioParametersTLV.value.bytes,
                selectedAudioParametersTLV.value.numBytes);
        HAPTLV selectedAudioCodecTypeTLV, selectedAudioCodecParametersTLV;
        selectedAudioCodecTypeTLV.type = kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecType;
        selectedAudioCodecParametersTLV.type =
                kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecParameters;
        err = HAPTLVReaderGetAll(
                &subReader, (HAPTLV* const[]) { &selectedAudioCodecTypeTLV, &selectedAudioCodecParametersTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Selected Audio Codec Type.
        if (!selectedAudioCodecTypeTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Audio Codec Type missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (selectedAudioCodecTypeTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Audio Codec Type has invalid length (%zu).",
                    (const void*) camera,
                    selectedAudioCodecTypeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawAudioCodecType =
                HAPReadUIntMax8(selectedAudioCodecTypeTLV.value.bytes, selectedAudioCodecTypeTLV.value.numBytes);
        if (!HAPCharacteristicValue_AudioCodecRecordingTypeIsValid(rawAudioCodecType)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Audio Codec Type invalid: %u.",
                    (const void*) camera,
                    rawAudioCodecType);
            return kHAPError_InvalidData;
        }
        switch ((HAPCharacteristicValue_AudioCodecRecordingType) rawAudioCodecType) {
            case kHAPCharacteristicValue_AudioCodecRecordingType_AAC_LC: {
                selectedConfig.audio.codecType = kHAPAudioCodecType_AAC_LC;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingType_AAC_ELD: {
                selectedConfig.audio.codecType = kHAPAudioCodecType_AAC_ELD;
                break;
            }
            default:
                HAPFatalError();
        }

        // Selected Audio Codec Parameters.
        HAPAudioCodecParameters* codecParameters = &selectedConfig.audio.codecParameters;

        if (!selectedAudioCodecParametersTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Selected Audio Codec Parameters missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        HAPTLVReader sub2Reader;
        HAPTLVReaderCreate(
                &sub2Reader,
                (void*) (uintptr_t) selectedAudioCodecParametersTLV.value.bytes,
                selectedAudioCodecParametersTLV.value.numBytes);
        HAPTLV audioChannelsTLV, bitRateModeTLV, sampleRateTLV, bitRateTLV;
        audioChannelsTLV.type = kHAPCharacteristicValue_AudioCodecRecordingParameter_AudioChannels;
        bitRateModeTLV.type = kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRateMode;
        sampleRateTLV.type = kHAPCharacteristicValue_AudioCodecRecordingParameter_SampleRate;
        bitRateTLV.type = kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRate;
        err = HAPTLVReaderGetAll(
                &sub2Reader,
                (HAPTLV* const[]) { &audioChannelsTLV, &bitRateModeTLV, &sampleRateTLV, &bitRateTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Audio Channels.
        if (!audioChannelsTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Audio Channels missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (audioChannelsTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Audio Channels has invalid length (%zu).",
                    (const void*) camera,
                    audioChannelsTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawAudioChannels = HAPReadUIntMax8(audioChannelsTLV.value.bytes, audioChannelsTLV.value.numBytes);
        codecParameters->numberOfChannels = rawAudioChannels;

        // Bit-rate Mode.
        if (!bitRateModeTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Bit-rate Mode missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (bitRateModeTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Bit-rate Mode has invalid length (%zu).",
                    (const void*) camera,
                    bitRateModeTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawBitRateMode = HAPReadUIntMax8(bitRateModeTLV.value.bytes, bitRateModeTLV.value.numBytes);
        if (!HAPCharacteristicValue_AudioCodecBitRateControlModeIsValid(rawBitRateMode)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Bit-rate Mode invalid: %u.",
                    (const void*) camera,
                    rawBitRateMode);
            return kHAPError_InvalidData;
        }
        switch ((HAPCharacteristicValue_AudioCodecBitRateControlMode) rawBitRateMode) {
            case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable: {
                codecParameters->bitRateMode = kHAPAudioCodecBitRateControlMode_Variable;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant: {
                codecParameters->bitRateMode = kHAPAudioCodecBitRateControlMode_Constant;
                break;
            }
            default:
                HAPFatalError();
        }

        // Sample Rate.
        if (!sampleRateTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Sample Rate missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (sampleRateTLV.value.numBytes > sizeof(uint8_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Sample Rate has invalid length (%zu).",
                    (const void*) camera,
                    sampleRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint8_t rawSampleRate = HAPReadUIntMax8(sampleRateTLV.value.bytes, sampleRateTLV.value.numBytes);
        if (!HAPCharacteristicValue_AudioCodecRecordingSampleRateIsValid(rawSampleRate)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Sample Rate invalid: %u.",
                    (const void*) camera,
                    rawSampleRate);
            return kHAPError_InvalidData;
        }
        switch ((HAPCharacteristicValue_AudioCodecRecordingSampleRate) rawSampleRate) {
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_8KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_8KHz;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_16KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_16KHz;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_24KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_24KHz;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_32KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_32KHz;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_44_1KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_44_1KHz;
                break;
            }
            case kHAPCharacteristicValue_AudioCodecRecordingSampleRate_48KHz: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_48KHz;
                break;
            }
            default:
                HAPFatalError();
        }

        // Bit Rate.
        if (!bitRateTLV.value.bytes) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Bit Rate missing.",
                    (const void*) camera);
            return kHAPError_InvalidData;
        }
        if (bitRateTLV.value.numBytes > sizeof(uint32_t)) {
            HAPLogCharacteristic(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Bit Rate has invalid length (%zu).",
                    (const void*) camera,
                    bitRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        uint32_t rawBitRate = HAPReadLittleUIntMax32(bitRateTLV.value.bytes, bitRateTLV.value.numBytes);
        codecParameters->bitRate = rawBitRate;
    }

// Log Selected Camera Recording Configuration.
#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    LogSelectedRecordingConfiguration(
            request->accessory, request->service, request->characteristic, camera, &selectedConfig);
#endif

    // Verify that Selected Camera Recording Configuration is supported.
    if (!HAPCameraAccessoryIsRecordingConfigurationSupported(
                server, request->service, request->accessory, &selectedConfig)) {
        HAPLogCharacteristic(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "[%p] Selected Camera Recording Configuration is not supported.",
                (const void*) camera);
        return kHAPError_InvalidData;
    }

    HAPCameraRecordingConfiguration currentSelectedConfig;
    bool found;
    err = HAPPlatformCameraGetRecordingConfiguration(camera, &found, &currentSelectedConfig);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraGetRecordingConfiguration failed: %u.", err);
        return err;
    }
    if (!found || !AreRecordingConfigurationsEqual(&selectedConfig, &currentSelectedConfig)) {

        // Set Selected Camera Recording Configuration.
        err = HAPPlatformCameraSetRecordingConfiguration(camera, &selectedConfig);
        if (err) {
            HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidData);
            HAPLogCharacteristicError(
                    &logObject,
                    request->characteristic,
                    request->service,
                    request->accessory,
                    "[%p] Failed to set Selected Camera Recording Configuration: %u.",
                    (const void*) camera,
                    err);
            return err;
        }

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementRecordingAudioActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_RecordingAudioActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isAudioEnabled;
    err = HAPPlatformCameraIsRecordingAudioEnabled(camera, &isAudioEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsRecordingAudioEnabled failed: %u.", err);
        return err;
    }

    if (isAudioEnabled) {
        *value = kHAPCharacteristicValue_RecordingAudioActive_EnableAudio;
    } else {
        *value = kHAPCharacteristicValue_RecordingAudioActive_DisableAudio;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_RecordingAudioActiveIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_RecordingAudioActive));
    switch ((HAPCharacteristicValue_Active) value) {
        case kHAPCharacteristicValue_RecordingAudioActive_DisableAudio:
        case kHAPCharacteristicValue_RecordingAudioActive_EnableAudio: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraEventRecordingManagementRecordingAudioActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_RecordingAudioActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraEventRecordingManagement));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_RecordingAudioActiveIsValid(value_));
    HAPCharacteristicValue_RecordingAudioActive value = (HAPCharacteristicValue_RecordingAudioActive) value_;

    HAPError err;

    bool wasAudioEnabled;
    err = HAPPlatformCameraIsRecordingAudioEnabled(camera, &wasAudioEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsRecordingAudioEnabled failed: %u.", err);
        return err;
    }
    bool isAudioEnabled = value == kHAPCharacteristicValue_RecordingAudioActive_EnableAudio;

    if (isAudioEnabled != wasAudioEnabled) {
        HAPLogInfo(&logObject, "[%p] %s Audio Active.", (const void*) camera, isAudioEnabled ? "Enable" : "Disable");
        err = HAPPlatformCameraSetRecordingAudioEnabled(camera, isAudioEnabled);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetRecordingAudioEnabled failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeEventSnapshotsActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_EventSnapshotsActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isSnapshotsActive;
    err = HAPPlatformCameraAreEventSnapshotsActive(camera, &isSnapshotsActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraAreEventSnapshotsActive failed: %u.", err);
        return err;
    }

    if (isSnapshotsActive) {
        *value = kHAPCharacteristicValue_EventSnapshotsActive_EnableSnapshots;
    } else {
        *value = kHAPCharacteristicValue_EventSnapshotsActive_DisableSnapshots;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeEventSnapshotsActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_EventSnapshotsActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_ActiveIsValid(value_));
    HAPCharacteristicValue_EventSnapshotsActive value = (HAPCharacteristicValue_EventSnapshotsActive) value_;

    HAPError err;

    bool wasSnapshotsActive;
    err = HAPPlatformCameraAreEventSnapshotsActive(camera, &wasSnapshotsActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraAreEventSnapshotsActive failed: %u.", err);
        return err;
    }
    bool isSnapshotsActive = value == kHAPCharacteristicValue_EventSnapshotsActive_EnableSnapshots;

    if (isSnapshotsActive != wasSnapshotsActive) {
        HAPLogInfo(
                &logObject,
                "[%p] %s Event Snapshots Active.",
                (const void*) camera,
                isSnapshotsActive ? "Set" : "Reset");
        err = HAPPlatformCameraSetEventSnapshotsActive(camera, isSnapshotsActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetEventSnapshotsActive failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModePeriodicSnapshotsActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_PeriodicSnapshotsActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isSnapshotsActive;
    err = HAPPlatformCameraArePeriodicSnapshotsActive(camera, &isSnapshotsActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraArePeriodicSnapshotsActive failed: %u.", err);
        return err;
    }

    if (isSnapshotsActive) {
        *value = kHAPCharacteristicValue_PeriodicSnapshotsActive_EnableSnapshots;
    } else {
        *value = kHAPCharacteristicValue_PeriodicSnapshotsActive_DisableSnapshots;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModePeriodicSnapshotsActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_PeriodicSnapshotsActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_ActiveIsValid(value_));
    HAPCharacteristicValue_PeriodicSnapshotsActive value = (HAPCharacteristicValue_PeriodicSnapshotsActive) value_;

    HAPError err;

    bool wasSnapshotsActive;
    err = HAPPlatformCameraArePeriodicSnapshotsActive(camera, &wasSnapshotsActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraArePeriodicSnapshotsActive failed: %u.", err);
        return err;
    }
    bool isSnapshotsActive = value == kHAPCharacteristicValue_PeriodicSnapshotsActive_EnableSnapshots;

    if (isSnapshotsActive != wasSnapshotsActive) {
        HAPLogInfo(
                &logObject,
                "[%p] %s Periodic Snapshots Active.",
                (const void*) camera,
                isSnapshotsActive ? "Set" : "Reset");
        err = HAPPlatformCameraSetPeriodicSnapshotsActive(camera, isSnapshotsActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetPeriodicSnapshotsActive failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeHomeKitCameraActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_HomeKitCameraActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isHomeKitCameraActive;
    err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsHomeKitCameraActive failed: %u.", err);
        return err;
    }

    if (isHomeKitCameraActive) {
        *value = kHAPCharacteristicValue_HomeKitCameraActive_On;
    } else {
        *value = kHAPCharacteristicValue_HomeKitCameraActive_Off;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeHomeKitCameraActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_HomeKitCameraActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_ActiveIsValid(value_));
    HAPCharacteristicValue_HomeKitCameraActive value = (HAPCharacteristicValue_HomeKitCameraActive) value_;

    HAPError err;

    bool wasHomeKitCameraActive;
    err = HAPPlatformCameraIsHomeKitCameraActive(camera, &wasHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsHomeKitCameraActive failed: %u.", err);
        return err;
    }
    bool isHomeKitCameraActive = value == kHAPCharacteristicValue_HomeKitCameraActive_On;

    if (isHomeKitCameraActive != wasHomeKitCameraActive) {
        HAPLogInfo(
                &logObject,
                "[%p] %s HomeKit Camera Active.",
                (const void*) camera,
                isHomeKitCameraActive ? "Set" : "Reset");

        err = HAPPlatformCameraSetHomeKitCameraActive(camera, isHomeKitCameraActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetHomeKitCameraActive failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeThirdPartyCameraActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_ThirdPartyCameraActive));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isThirdPartyCameraActive;
    err = HAPPlatformCameraIsThirdPartyCameraActive(camera, &isThirdPartyCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsThirdPartyCameraActive failed: %u.", err);
        return err;
    }

    if (isThirdPartyCameraActive) {
        *value = kHAPCharacteristicValue_ThirdPartyCameraActive_On;
    } else {
        *value = kHAPCharacteristicValue_ThirdPartyCameraActive_Off;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeCameraOperatingModeIndicatorRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_CameraOperatingModeIndicator));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isIndicatorEnabled;
    err = HAPPlatformCameraIsOperatingModeIndicatorEnabled(camera, &isIndicatorEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsOperatingModeIndicatorEnabled failed: %u.", err);
        return err;
    }

    if (isIndicatorEnabled) {
        *value = kHAPCharacteristicValue_CameraOperatingModeIndicator_Enable;
    } else {
        *value = kHAPCharacteristicValue_CameraOperatingModeIndicator_Disable;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static bool HAPCharacteristicValue_CameraOperatingModeIndicatorIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_CameraOperatingModeIndicator));
    switch ((HAPCharacteristicValue_Active) value) {
        case kHAPCharacteristicValue_CameraOperatingModeIndicator_Disable:
        case kHAPCharacteristicValue_CameraOperatingModeIndicator_Enable: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeCameraOperatingModeIndicatorWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value_,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_CameraOperatingModeIndicator));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);
    HAPPrecondition(HAPCharacteristicValue_CameraOperatingModeIndicatorIsValid(value_));
    HAPCharacteristicValue_CameraOperatingModeIndicator value =
            (HAPCharacteristicValue_CameraOperatingModeIndicator) value_;

    HAPError err;

    bool wasIndicatorEnabled;
    err = HAPPlatformCameraIsOperatingModeIndicatorEnabled(camera, &wasIndicatorEnabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsOperatingModeIndicatorEnabled failed: %u.", err);
        return err;
    }
    bool isIndicatorEnabled = value == kHAPCharacteristicValue_CameraOperatingModeIndicator_Enable;

    if (isIndicatorEnabled != wasIndicatorEnabled) {
        HAPLogInfo(
                &logObject,
                "[%p] %s Operating Mode Indicator.",
                (const void*) camera,
                isIndicatorEnabled ? "Enable" : "Disable");
        err = HAPPlatformCameraSetOperatingModeIndicatorEnabled(camera, isIndicatorEnabled);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraSetOperatingModeIndicatorEnabled failed: %u.", err);
            return err;
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPHandleCameraOperatingModeManuallyDisabledRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_ManuallyDisabled));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_CameraOperatingMode));
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, request->accessory);

    HAPError err;

    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return err;
    }

    if (isManuallyDisabled) {
        *value = kHAPCharacteristicValue_ManuallyDisabled_ManuallyDisabled;
    } else {
        *value = kHAPCharacteristicValue_ManuallyDisabled_ManuallyEnabled;
    }
    return kHAPError_None;
}

#endif
