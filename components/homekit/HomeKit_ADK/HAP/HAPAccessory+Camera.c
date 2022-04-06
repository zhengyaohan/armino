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

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPService.h"
#include "HAPVideo.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "Accessory" };

HAP_RESULT_USE_CHECK
HAPPlatformCameraRef HAPCameraAccessoryGetCamera(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    return server->platform.ip.camera;
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleGetSupportedRecordingConfigurationComplete(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);

    HAPCameraAccessoryGetSupportedRecordingConfigurationCallback _Nullable callback =
            server->ip.camera.getSupportedRecordingConfigurationDelegate.callback;
    void* _Nullable context = server->ip.camera.getSupportedRecordingConfigurationDelegate.context;

    if (!callback) {
        HAPLogError(
                &logObject,
                "completionHandler is only callable during a %s callback.",
                "getSupportedRecordingConfiguration");
        HAPFatalError();
    }

    HAPRawBufferZero(
            &server->ip.camera.getSupportedRecordingConfigurationDelegate,
            sizeof server->ip.camera.getSupportedRecordingConfigurationDelegate);

    callback(context, server, service, accessory, supportedConfig);
}

void HAPCameraAccessoryGetSupportedRecordingConfiguration(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPCameraAccessoryGetSupportedRecordingConfigurationCallback callback,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!server->ip.camera.getSupportedRecordingConfigurationDelegate.callback);
    HAPPrecondition(!server->ip.camera.getSupportedRecordingConfigurationDelegate.context);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(accessory->callbacks.camera.getSupportedRecordingConfiguration);
    HAPPrecondition(callback);

    server->ip.camera.getSupportedRecordingConfigurationDelegate.callback = callback;
    server->ip.camera.getSupportedRecordingConfigurationDelegate.context = context;

    accessory->callbacks.camera.getSupportedRecordingConfiguration(
            server, service, accessory, HandleGetSupportedRecordingConfigurationComplete, server->context);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
static bool HAPMediaContainerTypeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPMediaContainerType));
    switch ((HAPMediaContainerType) value) {
        case kHAPMediaContainerType_FragmentedMP4: {
            return true;
        }
        default:
            return false;
    }
}
/**
 * Indicates whether a video codec type is supported for streaming.
 *
 * @param      videoCodecType       Video codec type.
 *
 * @return true                     If the video codec type is supported for streaming.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPVideoCodecTypeIsSupportedForStreaming(HAPVideoCodecType videoCodecType) {
    switch (videoCodecType) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPVideoCodecType_H264: {
            return true;
        }
        default: {
            HAPLogError(&logObject, "%s: Unknown video codec type: %u.", __func__, videoCodecType);
            return false;
        }
    }
}

HAP_RESULT_USE_CHECK
bool HAPVideoCodecTypeIsSupportedForRecording(HAPVideoCodecType videoCodecType) {
    switch (videoCodecType) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPVideoCodecType_H264: {
            return true;
        }
        default: {
            HAPLogError(&logObject, "%s: Unknown video codec type: %u.", __func__, videoCodecType);
            return false;
        }
    }
}

HAP_RESULT_USE_CHECK
bool HAPAudioCodecTypeIsSupportedForRecording(HAPAudioCodecType audioCodecType) {
    switch (audioCodecType) {
        case kHAPAudioCodecType_AAC_ELD:
        case kHAPAudioCodecType_AAC_LC: {
            return true;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_Opus:
        case kHAPAudioCodecType_MSBC:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            return false;
        }
        default: {
            HAPLogError(&logObject, "%s: Unknown audio codec type: %u.", __func__, audioCodecType);
            return false;
        }
    }
}

/**
 * Indicates whether an audio codec type is supported for streaming.
 *
 * @param      audioCodecType       Audio codec type.
 *
 * @return true                     If the audio codec type is supported for streaming.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAudioCodecTypeIsSupportedForStreaming(HAPAudioCodecType audioCodecType) {
    switch (audioCodecType) {
        case kHAPAudioCodecType_AAC_ELD:
        case kHAPAudioCodecType_Opus:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            return true;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC: {
            HAPLogError(&logObject, "Audio codec not supported for streaming: %u.", audioCodecType);
            return false;
        }
        default: {
            HAPLogError(&logObject, "%s: Unknown audio codec type: %u.", __func__, audioCodecType);
            return false;
        }
    }
}

HAP_RESULT_USE_CHECK
static bool HAPAudioCodecBitRateControlModeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPAudioCodecBitRateControlMode));
    switch ((HAPAudioCodecBitRateControlMode) value) {
        case kHAPAudioCodecBitRateControlMode_Variable:
        case kHAPAudioCodecBitRateControlMode_Constant: {
            return true;
        }
        default:
            return false;
    }
}

HAP_RESULT_USE_CHECK
bool HAPAudioCodecSampleRateIsSupportedForRecording(HAPAudioCodecSampleRate audioSampleRate) {
    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
        case kHAPAudioCodecSampleRate_16KHz:
        case kHAPAudioCodecSampleRate_24KHz:
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz: {
            return true;
        }
        default: {
            HAPLogError(&logObject, "Unknown audio codec sample rate: %u.", audioSampleRate);
            return false;
        }
    }
}

HAP_RESULT_USE_CHECK
static bool IsRecordingConfigurationSupported(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig,
        const HAPCameraRecordingConfiguration* selectedConfig) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);
    HAPPrecondition(selectedConfig);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, accessory);

    // Validate Selected Camera Recording Configuration.
    {
        if (!HAPMediaContainerTypeIsValid(selectedConfig->recording.containerConfiguration.containerType)) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Invalid Media Container Type: %u.",
                    (const void*) camera,
                    selectedConfig->recording.containerConfiguration.containerType);
            return false;
        }
        if (!HAPVideoCodecTypeIsSupportedForRecording(selectedConfig->video.codecType)) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Video Codec Type not supported for recording: %u.",
                    (const void*) camera,
                    selectedConfig->video.codecType);
            return false;
        }
        switch (selectedConfig->video.codecType) {
            case kHAPVideoCodecType_H264: {
                const HAPH264VideoCodecParameters* selectedCodecParameters =
                        &selectedConfig->video.codecParameters.h264;
                const HAPVideoAttributes* selectedAttributes = &selectedConfig->video.attributes;

                if (!HAPVideoH264ProfileIDIsValid(selectedCodecParameters->profile)) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "[%p] Invalid H.264 ProfileID: 0x%X.",
                            (const void*) camera,
                            selectedCodecParameters->profile);
                    return false;
                }
                if (!HAPVideoH264ProfileLevelIsValid(selectedCodecParameters->level)) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "[%p] Invalid H.264 Level: 0x%X.",
                            (const void*) camera,
                            selectedCodecParameters->level);
                    return false;
                }
                if (!HAPVideoH264PacketizationModeIsValid(selectedCodecParameters->packetizationMode)) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "[%p] Invalid H.264 Packetization mode: 0x%X.",
                            (const void*) camera,
                            selectedCodecParameters->packetizationMode);
                    return false;
                }

                uint32_t pixelLimit = HAPVideoGetH264ProfileLevelPixelLimit(selectedCodecParameters->level);
                if ((uint32_t) selectedAttributes->width * (uint32_t) selectedAttributes->height > pixelLimit) {
                    HAPLogInfo(
                            &logObject,
                            "Warning: [%p] Invalid Video Attributes %4u x %4u @ %u fps for H.264 Level: %u.",
                            (const void*) camera,
                            selectedAttributes->width,
                            selectedAttributes->height,
                            selectedAttributes->maxFrameRate,
                            selectedCodecParameters->level);
                    // ADK supports the 1536x1536 resolution, which requires H.264 profile level 5. However, the HAP R17
                    // specification does not provide a way for a controller to specify profile level 5.
                    // Therefore, we leave it up to the accessory to decide how it wants to handle mismatched
                    // profile level and resolutions, in order to enable the controller requesting 1536x1536 resolution.
                    // Note: Enable the "return false;" line below if an accessory wants resolutions to conform
                    // to the controller specified profile level. Enabling this line will disable resolutions that
                    // require profile level 5 (or any other profile level not specified in HAP R17).
                    // @see  HomeKit Accessory Protocol Specification R17
                    //       Table 10‐4: Video Codec Streaming Configuration TLV Types
                    //       Table 10‐42: Video Codec Recording Parameters H.264 TLV Types
                    // return false;
                }
                break;
            }
        }
        if (!HAPAudioCodecTypeIsSupportedForRecording(selectedConfig->audio.codecType)) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Audio Codec Type not supported for recording: %u.",
                    (const void*) camera,
                    selectedConfig->audio.codecType);
            return false;
        }
        switch (selectedConfig->audio.codecType) {
            case kHAPAudioCodecType_AAC_ELD:
            case kHAPAudioCodecType_AAC_LC: {
                const HAPAudioCodecParameters* selectedCodecParameters = &selectedConfig->audio.codecParameters;
                if (!HAPAudioCodecBitRateControlModeIsValid(selectedCodecParameters->bitRateMode)) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "[%p] Invalid Audio Bit-rate Mode: %u.",
                            (const void*) camera,
                            selectedCodecParameters->bitRateMode);
                    return false;
                }
                if (!HAPAudioCodecSampleRateIsSupportedForRecording(selectedCodecParameters->sampleRate)) {
                    HAPLogServiceError(
                            &logObject,
                            service,
                            accessory,
                            "[%p] Audio Sample Rate not supported for recording: %u.",
                            (const void*) camera,
                            selectedCodecParameters->sampleRate);
                    return false;
                }
                break;
            }
            case kHAPAudioCodecType_PCMU:
            case kHAPAudioCodecType_PCMA:
            case kHAPAudioCodecType_Opus:
            case kHAPAudioCodecType_MSBC:
            case kHAPAudioCodecType_AMR:
            case kHAPAudioCodecType_AMR_WB: {
            }
                HAPFatalError();
        }
    }

    // Selected Camera Recording Parameters.
    {
        // Prebuffer Duration.
        HAPAssert(
                supportedConfig->recording.prebufferDuration == 0 ||
                supportedConfig->recording.prebufferDuration >=
                        kHAPCameraRecordingSupportedConfiguration_MinPrebufferDuration);
        HAPAssert(
                supportedConfig->recording.prebufferDuration <=
                kHAPCameraRecordingSupportedConfiguration_MaxPrebufferDuration);
        if (selectedConfig->recording.prebufferDuration > supportedConfig->recording.prebufferDuration) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Prebuffer Duration must be in range %llums ... %llums.",
                    (const void*) camera,
                    (unsigned long long) 0,
                    (unsigned long long) supportedConfig->recording.prebufferDuration);
            return false;
        }

        // Event Trigger Type.
        HAPCameraEventTriggerTypes unsupportedEventTriggerTypes = ~supportedConfig->recording.eventTriggerTypes;
        if (selectedConfig->recording.eventTriggerTypes & unsupportedEventTriggerTypes) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Event Trigger Types 0x%llX are not supported.",
                    (const void*) camera,
                    (unsigned long long) unsupportedEventTriggerTypes);
            return false;
        }

        // Selected Media Container Configuration.
        bool found = false;
        if (supportedConfig->recording.containerConfigurations) {
            for (size_t i = 0; supportedConfig->recording.containerConfigurations[i]; i++) {
                const HAPCameraSupportedMediaContainerConfiguration* containerConfiguration =
                        supportedConfig->recording.containerConfigurations[i];

                // Media Container Type.
                if (selectedConfig->recording.containerConfiguration.containerType !=
                    containerConfiguration->containerType) {
                    continue;
                }

                // Media Container Parameters.
                switch (containerConfiguration->containerType) {
                    case kHAPMediaContainerType_FragmentedMP4: {
                        const HAPFragmentedMP4MediaContainerParameters* containerParameters =
                                containerConfiguration->containerParameters;
                        const HAPFragmentedMP4MediaContainerParameters* selectedContainerParameters =
                                &selectedConfig->recording.containerConfiguration.containerParameters.fragmentedMP4;

                        // Fragment Duration.
                        if (selectedContainerParameters->fragmentDuration > containerParameters->fragmentDuration) {
                            continue;
                        }
                        break;
                    }
                }

                found = true;
                break;
            }
        }
        if (!found) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Media Container Configuration is not supported.",
                    (const void*) camera);
            return false;
        }
    }

    // Selected Video Parameters.
    {
        bool found = false;
        if (supportedConfig->video.configurations) {
            for (size_t i = 0; supportedConfig->video.configurations[i]; i++) {
                const HAPCameraSupportedVideoCodecConfiguration* configuration =
                        supportedConfig->video.configurations[i];

                // Selected Video Codec Type.
                if (selectedConfig->video.codecType != configuration->codecType) {
                    continue;
                }

                // Selected Video Codec Parameters.
                switch (configuration->codecType) {
                    case kHAPVideoCodecType_H264: {
                        const HAPH264VideoCodecParameters* codecParameters = configuration->codecParameters;
                        const HAPH264VideoCodecParameters* selectedCodecParameters =
                                &selectedConfig->video.codecParameters.h264;

                        // ProfileID.
                        if (!(codecParameters->profile & selectedCodecParameters->profile)) {
                            continue;
                        }

                        // Level.
                        if (!(codecParameters->level & selectedCodecParameters->level)) {
                            continue;
                        }

                        // Packetization mode.
                        if (!(codecParameters->packetizationMode & selectedCodecParameters->packetizationMode)) {
                            continue;
                        }

                        // Bit Rate.
                        if (codecParameters->bitRate && selectedCodecParameters->bitRate > codecParameters->bitRate) {
                            continue;
                        }

                        // I-Frame Interval.
                        if (codecParameters->iFrameInterval &&
                            selectedCodecParameters->iFrameInterval > codecParameters->iFrameInterval) {
                            continue;
                        }
                        break;
                    }
                }

                // Selected Video Attributes.
                bool foundAttributes = false;
                if (configuration->attributes) {
                    for (size_t j = 0; configuration->attributes[j]; j++) {
                        const HAPVideoAttributes* attributes = configuration->attributes[j];
                        const HAPVideoAttributes* selectedAttributes = &selectedConfig->video.attributes;

                        // Image Width.
                        if (selectedAttributes->width != attributes->width) {
                            continue;
                        }

                        // Image Height.
                        if (selectedAttributes->height != attributes->height) {
                            continue;
                        }

                        // Frame rate.
                        if (selectedAttributes->maxFrameRate != attributes->maxFrameRate) {
                            continue;
                        }

                        foundAttributes = true;
                        break;
                    }
                }
                if (!foundAttributes) {
                    continue;
                }

                found = true;
                break;
            }
        }
        if (!found) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Video Codec Configuration is not supported.",
                    (const void*) camera);
            return false;
        }
    }

    // Selected Audio Parameters.
    {
        bool found = false;
        if (supportedConfig->audio.configurations) {
            for (size_t i = 0; supportedConfig->audio.configurations[i]; i++) {
                const HAPCameraSupportedAudioCodecConfiguration* configuration =
                        supportedConfig->audio.configurations[i];

                // Selected Audio Codec Type.
                if (selectedConfig->audio.codecType != configuration->codecType) {
                    continue;
                }

                // Selected Audio Codec Parameters.
                {
                    const HAPAudioCodecParameters* codecParameters = configuration->codecParameters;
                    const HAPAudioCodecParameters* selectedCodecParameters = &selectedConfig->audio.codecParameters;

                    // Audio Channels.
                    if (selectedCodecParameters->numberOfChannels != codecParameters->numberOfChannels) {
                        continue;
                    }

                    // Bit-rate Mode.
                    if (!(codecParameters->bitRateMode & selectedCodecParameters->bitRateMode)) {
                        continue;
                    }

                    // Sample Rate.
                    if (!(codecParameters->sampleRate & selectedCodecParameters->sampleRate)) {
                        continue;
                    }

                    // Bit Rate.
                    if (codecParameters->bitRate && selectedCodecParameters->bitRate > codecParameters->bitRate) {
                        continue;
                    }
                }

                found = true;
                break;
            }
        }
        if (!found) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Audio Codec Configuration is not supported.",
                    (const void*) camera);
            return false;
        }
    }

    return true;
}

typedef struct {
    const HAPCameraRecordingConfiguration* selectedConfig;
    bool isSupported;
} IsRecordingConfigurationSupportedContext;

static void IsRecordingConfigurationSupportedCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(context);
    IsRecordingConfigurationSupportedContext* arguments = context;
    HAPPrecondition(arguments->selectedConfig);
    const HAPCameraRecordingConfiguration* selectedConfig = arguments->selectedConfig;

    arguments->isSupported =
            IsRecordingConfigurationSupported(server, service, accessory, supportedConfig, selectedConfig);
}

HAP_RESULT_USE_CHECK
bool HAPCameraAccessoryIsRecordingConfigurationSupported(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingConfiguration* selectedConfig) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(selectedConfig);

    IsRecordingConfigurationSupportedContext requestContext;
    HAPRawBufferZero(&requestContext, sizeof requestContext);
    requestContext.selectedConfig = selectedConfig;
    requestContext.isSupported = false;
    HAPCameraAccessoryGetSupportedRecordingConfiguration(
            server, service, accessory, IsRecordingConfigurationSupportedCallback, &requestContext);
    return requestContext.isSupported;
}

//----------------------------------------------------------------------------------------------------------------------

typedef struct {
    HAPCameraSupportedRecordingConfigurationChange changes;
} SupportedRecordingConfigurationChangeContext;

static void SupportedRecordingConfigurationChangeCallback(
        void* _Nullable context,
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig) {
    HAPPrecondition(context);
    SupportedRecordingConfigurationChangeContext* arguments = context;
    HAPCameraSupportedRecordingConfigurationChange changes = arguments->changes;
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(supportedConfig);
    HAPPlatformCameraRef camera = HAPAccessoryGetCamera(server, accessory);

    HAPError err;

    // Get selected recording configuration.
    HAPCameraRecordingConfiguration selectedConfig;
    bool found;
    err = HAPPlatformCameraGetRecordingConfiguration(camera, &found, &selectedConfig);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogServiceError(
                &logObject,
                service,
                accessory,
                "[%p] Failed to get Selected Camera Recording Configuration: %u.",
                (const void*) camera,
                err);
        HAPFatalError();
    }

    // If recording configuration is no longer supported, invalidate it.
    if (found && !IsRecordingConfigurationSupported(server, service, accessory, supportedConfig, &selectedConfig)) {
        HAPLogService(
                &logObject,
                service,
                accessory,
                "[%p] Selected Camera Recording Configuration is no longer supported. Invalidating.",
                (const void*) camera);

        // Invalidate recording configuration.
        err = HAPPlatformCameraInvalidateRecordingConfiguration(camera);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "HAPPlatformCameraInvalidateRecordingConfiguration failed: %u.", err);
            HAPFatalError();
        }

        // Report to controller.
        const HAPCharacteristic* _Nullable characteristic =
                HAPServiceGetCharacteristic(service, &kHAPCharacteristicType_SelectedCameraRecordingConfiguration);
        if (!characteristic) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Not notifying as %s characteristic not available.",
                    (const void*) camera,
                    kHAPCharacteristicDebugDescription_SelectedCameraRecordingConfiguration);
        } else {
            HAPAccessoryServerRaiseEvent(server, HAPNonnullVoid(characteristic), service, accessory);
        }
    }

    // Report to controller.
    if (changes & (uint8_t) kHAPCameraSupportedRecordingConfigurationChange_Camera) {
        const HAPCharacteristic* _Nullable characteristic =
                HAPServiceGetCharacteristic(service, &kHAPCharacteristicType_SupportedCameraRecordingConfiguration);
        if (!characteristic) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Not notifying as %s characteristic not available.",
                    (const void*) camera,
                    kHAPCharacteristicDebugDescription_SupportedCameraRecordingConfiguration);
        } else {
            HAPLogCharacteristicDebug(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "[%p] Reporting configuration change.",
                    (const void*) camera);
            HAPAccessoryServerRaiseEvent(server, HAPNonnullVoid(characteristic), service, accessory);
        }
    }
    if (changes & (uint8_t) kHAPCameraSupportedRecordingConfigurationChange_Video) {
        const HAPCharacteristic* _Nullable characteristic =
                HAPServiceGetCharacteristic(service, &kHAPCharacteristicType_SupportedVideoRecordingConfiguration);
        if (!characteristic) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Not notifying as %s characteristic not available.",
                    (const void*) camera,
                    kHAPCharacteristicDebugDescription_SupportedVideoRecordingConfiguration);
        } else {
            HAPLogCharacteristicDebug(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "[%p] Reporting configuration change.",
                    (const void*) camera);
            HAPAccessoryServerRaiseEvent(server, HAPNonnullVoid(characteristic), service, accessory);
        }
    }
    if (changes & (uint8_t) kHAPCameraSupportedRecordingConfigurationChange_Audio) {
        const HAPCharacteristic* _Nullable characteristic =
                HAPServiceGetCharacteristic(service, &kHAPCharacteristicType_SupportedAudioRecordingConfiguration);
        if (!characteristic) {
            HAPLogServiceError(
                    &logObject,
                    service,
                    accessory,
                    "[%p] Not notifying as %s characteristic not available.",
                    (const void*) camera,
                    kHAPCharacteristicDebugDescription_SupportedAudioRecordingConfiguration);
        } else {
            HAPLogCharacteristicDebug(
                    &logObject,
                    characteristic,
                    service,
                    accessory,
                    "[%p] Reporting configuration change.",
                    (const void*) camera);
            HAPAccessoryServerRaiseEvent(server, HAPNonnullVoid(characteristic), service, accessory);
        }
    }
}

void HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPCameraSupportedRecordingConfigurationChange changes) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);

    SupportedRecordingConfigurationChangeContext requestContext;
    HAPRawBufferZero(&requestContext, sizeof requestContext);
    requestContext.changes = changes;
    HAPCameraAccessoryGetSupportedRecordingConfiguration(
            server, service, accessory, SupportedRecordingConfigurationChangeCallback, &requestContext);
}

#endif
