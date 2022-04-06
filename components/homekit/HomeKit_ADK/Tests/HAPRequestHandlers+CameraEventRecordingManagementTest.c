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

#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"

#include "HAPPlatform+Init.h"
#include "HAPPlatformTCPStreamManager+Init.h"

#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.c"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.c"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#define kApp_NumStreams ((size_t) 1)

/**
 * H.264 video codec parameters.
 */
static const HAPH264VideoCodecParameters h264VideoCodecParameters = {
    .profile = kHAPH264VideoCodecProfile_Main,
    .level = kHAPH264VideoCodecProfileLevel_3_1 | kHAPH264VideoCodecProfileLevel_4,
    .packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved,
};

/**
 * Available video parameters
 */
static const HAPVideoAttributes videoAttributes[] = { [0] = { .width = 1920, .height = 1080, .maxFrameRate = 30 },
                                                      [1] = { .width = 1280, .height = 720, .maxFrameRate = 30 },
                                                      [2] = { .width = 640, .height = 360, .maxFrameRate = 30 },
                                                      [3] = { .width = 480, .height = 270, .maxFrameRate = 30 },
                                                      [4] = { .width = 320, .height = 180, .maxFrameRate = 30 },
                                                      [5] = { .width = 1280, .height = 960, .maxFrameRate = 30 },
                                                      [6] = { .width = 1024, .height = 768, .maxFrameRate = 30 },
                                                      [7] = { .width = 640, .height = 480, .maxFrameRate = 30 },
                                                      [8] = { .width = 480, .height = 360, .maxFrameRate = 30 },
                                                      [9] = { .width = 320, .height = 240, .maxFrameRate = 30 } };

/**
 * Full screen video parameters
 */
static const HAPVideoAttributes* fullVideoAttributes[] = { &videoAttributes[0],
                                                           &videoAttributes[1],
                                                           &videoAttributes[2],
                                                           &videoAttributes[3],
                                                           &videoAttributes[4],
                                                           &videoAttributes[5],
                                                           &videoAttributes[6],
                                                           &videoAttributes[7],
                                                           &videoAttributes[8],
                                                           &videoAttributes[9],
                                                           NULL };

/**
 * H.264 video codec configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264FullVideoCodecConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecParameters,
    .attributes = fullVideoAttributes
};

/**
 * Opus audio codec parameters.
 */
static const HAPAudioCodecParameters varAudioCodecParameters = {
    .numberOfChannels = 1,
    .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
    .sampleRate = kHAPAudioCodecSampleRate_16KHz | kHAPAudioCodecSampleRate_24KHz
};

/**
 * Opus audio configuration.
 */
static const HAPCameraSupportedAudioCodecConfiguration opusAudioCodecConfiguration = {
    .codecType = kHAPAudioCodecType_Opus,
    .codecParameters = &varAudioCodecParameters
};

/**
 * First video stream:
 *  - Video: H264, Level 3.1 and 4, all resolutions
 *  - Audio: Opus, 16kHz and 24kHz sample rate
 */
static const HAPCameraStreamSupportedConfigurations supportedCameraStreamConfigurations0 = {
    .videoStream = { .configurations =
                             (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                                     &h264FullVideoCodecConfiguration,
                                     NULL } },
    .audioStream = { .configurations =
                             (const HAPCameraSupportedAudioCodecConfiguration* const[]) { &opusAudioCodecConfiguration,
                                                                                          NULL },
                     .comfortNoise = { .supported = false } },
    .rtp = { .srtpCryptoSuites = kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 |
                                 kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80 | kHAPSRTPCryptoSuite_Disabled }
};

/**
 * Fragmented MP4 container parameters.
 */
static const HAPFragmentedMP4MediaContainerParameters fragmentedMP4ContainerParameters = {
    .fragmentDuration = 2000 // ms
};

/**
 * Fragmented MP4 container configuration.
 */
static const HAPCameraSupportedMediaContainerConfiguration fragmentedMP4ContainerConfiguration = {
    .containerType = kHAPMediaContainerType_FragmentedMP4,
    .containerParameters = &fragmentedMP4ContainerParameters
};

/**
 * H.264 video codec parameters.
 */
static const HAPH264VideoCodecParameters h264VideoCodecRecordingParameters = {
    .profile = kHAPH264VideoCodecProfile_Main,
    .level = kHAPH264VideoCodecProfileLevel_3_1 | kHAPH264VideoCodecProfileLevel_4,
    .packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved
};

/**
 * H.264 video codec configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264VideoCodecRecordingConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecRecordingParameters,
    .attributes = fullVideoAttributes
};

/**
 * AAC-LC audio codec recording parameters.
 */
static const HAPAudioCodecParameters varAudioCodecRecordingParameters = {
    .numberOfChannels = 1,
    .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
    .sampleRate = kHAPAudioCodecSampleRate_16KHz | kHAPAudioCodecSampleRate_24KHz
};

/**
 * AAC-LC audio recording configuration.
 */
static const HAPCameraSupportedAudioCodecConfiguration aacAudioCodecRecordingConfiguration = {
    .codecType = kHAPAudioCodecType_AAC_LC,
    .codecParameters = &varAudioCodecRecordingParameters
};

/**
 * Recording configuration.
 */
static const HAPCameraRecordingSupportedConfiguration supportedCameraRecordingConfiguration = {
    .recording = { .prebufferDuration = 4 * HAPSecond,
                   .eventTriggerTypes = kHAPCameraEventTriggerTypes_Motion | kHAPCameraEventTriggerTypes_Doorbell,
                   .containerConfigurations =
                           (const HAPCameraSupportedMediaContainerConfiguration* const[]) {
                                   &fragmentedMP4ContainerConfiguration,
                                   NULL } },
    .video = { .configurations =
                       (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                               &h264VideoCodecRecordingConfiguration,
                               NULL } },
    .audio = { .configurations =
                       (const HAPCameraSupportedAudioCodecConfiguration* const[]) {
                               &aacAudioCodecRecordingConfiguration,
                               NULL } }
};

static void HandleGetSupportedRecordingConfiguration(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPCameraAccessoryGetSupportedRecordingConfigurationCompletionHandler completionHandler,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(service);
    HAPPrecondition(accessory);
    HAPPrecondition(completionHandler);

    completionHandler(server, service, accessory, &supportedCameraRecordingConfiguration);
}

static const HAPAccessory accessory = {
    .aid = 1,
    .category = kHAPAccessoryCategory_Other,
    .name = "Acme Test",
    .productData = "03d8a775e3644573",
    .manufacturer = "Acme",
    .model = "Test1,1",
    .serialNumber = "099DB48E9E28",
    .firmwareVersion = "1",
    .hardwareVersion = "1",
    .services = (const HAPService* const[]) { &accessoryInformationService,
                                              &hapProtocolInformationService,
                                              &pairingService,
                                              &cameraEventRecordingManagementService,
                                              &cameraRTPStreamManagement0Service,
                                              NULL },
    .dataStream.delegate = { .callbacks = NULL, .context = NULL },
    .cameraStreamConfigurations =
            (const HAPCameraStreamSupportedConfigurations* const[]) { &supportedCameraStreamConfigurations0, NULL },
    .callbacks = { .identify = IdentifyAccessoryHelper,
                   .camera = { .getSupportedRecordingConfiguration = HandleGetSupportedRecordingConfiguration } }
};

//----------------------------------------------------------------------------------------------------------------------

static void ReadSelectedCameraRecordingConfiguration(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPCameraRecordingSupportedConfiguration* cameraRecordingConfiguration,
        bool* found,
        HAPCameraRecordingConfiguration* selectedConfig,
        void* _Nullable context) {
    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading Selected Camera Recording Configuration.");
    HAPRawBufferZero(selectedConfig, sizeof *selectedConfig);

    uint8_t bytes[1024];
    HAPTLVWriter responseWriter;
    HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
    const HAPTLV8CharacteristicReadRequest request = {
        .transportType = kHAPTransportType_IP,
        .session = session,
        .characteristic = &cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic,
        .service = &cameraEventRecordingManagementService,
        .accessory = &accessory
    };
    err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, context);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        *found = false;
        return;
    }
    *found = true;
    void* responseBytes;
    size_t numResponseBytes;
    HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
    HAPTLVReader responseReader;
    HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

    HAPTLV selectedCameraRecordingParametersTLV, selectedVideoParametersTLV, selectedAudioParametersTLV;
    selectedCameraRecordingParametersTLV.type = 1;
    selectedVideoParametersTLV.type = 2;
    selectedAudioParametersTLV.type = 3;
    err = HAPTLVReaderGetAll(
            &responseReader,
            (HAPTLV* const[]) { &selectedCameraRecordingParametersTLV,
                                &selectedVideoParametersTLV,
                                &selectedAudioParametersTLV,
                                NULL });
    HAPAssert(!err);

    // Selected Camera Recording Parameters.
    {
        HAPAssert(selectedCameraRecordingParametersTLV.value.bytes);
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedCameraRecordingParametersTLV.value.bytes,
                selectedCameraRecordingParametersTLV.value.numBytes);
        HAPTLV prebufferDurationTLV, eventTriggerTypeTLV, selectedMediaContainerConfigurationTLV;
        prebufferDurationTLV.type = 1;
        eventTriggerTypeTLV.type = 2;
        selectedMediaContainerConfigurationTLV.type = 3;
        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) {
                        &prebufferDurationTLV, &eventTriggerTypeTLV, &selectedMediaContainerConfigurationTLV, NULL });
        HAPAssert(!err);

        // Prebuffer Duration.
        HAPAssert(prebufferDurationTLV.value.bytes);
        HAPAssert(prebufferDurationTLV.value.numBytes == sizeof(uint32_t));
        uint32_t rawPrebufferDuration = HAPReadLittleUInt32(prebufferDurationTLV.value.bytes);
        selectedConfig->recording.prebufferDuration = rawPrebufferDuration * HAPMillisecond;

        // Event Trigger Type.
        HAPAssert(eventTriggerTypeTLV.value.bytes);
        HAPAssert(eventTriggerTypeTLV.value.numBytes == sizeof(uint64_t));
        uint64_t rawEventTriggerType = HAPReadLittleUInt64(eventTriggerTypeTLV.value.bytes);
        if (rawEventTriggerType & 1U << 0U) {
            selectedConfig->recording.eventTriggerTypes |= kHAPCameraEventTriggerTypes_Motion;
        }
        if (rawEventTriggerType & 1U << 1U) {
            selectedConfig->recording.eventTriggerTypes |= kHAPCameraEventTriggerTypes_Doorbell;
        }
        if (rawEventTriggerType & ~((1ULL << 0) | (1ULL << 1))) {
            HAPLogCharacteristicError(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "Unexpected Event Trigger Type: 0x%016llX.",
                    (unsigned long long) rawEventTriggerType);
            HAPFatalError();
        }

        // Selected Media Container Configuration.
        HAPAssert(selectedMediaContainerConfigurationTLV.value.bytes);
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
        HAPAssert(!err);

        // Media Container Type / Media Container Parameters.
        HAPAssert(mediaContainerTypeTLV.value.bytes);
        HAPAssert(mediaContainerTypeTLV.value.numBytes == sizeof(uint8_t));
        uint8_t rawMediaContainerType = HAPReadUInt8(mediaContainerTypeTLV.value.bytes);
        HAPAssert(mediaContainerParametersTLV.value.bytes);
        HAPTLVReader sub3Reader;
        HAPTLVReaderCreate(
                &sub3Reader,
                (void*) (uintptr_t) mediaContainerParametersTLV.value.bytes,
                mediaContainerParametersTLV.value.numBytes);
        switch (rawMediaContainerType) {
            case 0: {
                // Fragmented MP4.
                selectedConfig->recording.containerConfiguration.containerType = kHAPMediaContainerType_FragmentedMP4;

                // Media Container Parameters.
                HAPTLV fragmentDurationTLV;
                fragmentDurationTLV.type = 1;
                err = HAPTLVReaderGetAll(&sub3Reader, (HAPTLV* const[]) { &fragmentDurationTLV, NULL });
                HAPAssert(!err);

                // Fragment Duration.
                HAPAssert(fragmentDurationTLV.value.bytes);
                HAPAssert(fragmentDurationTLV.value.numBytes == sizeof(uint32_t));
                uint32_t rawFragmentDuration = HAPReadLittleUInt32(fragmentDurationTLV.value.bytes);
                selectedConfig->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration =
                        rawFragmentDuration;
                break;
            }
            default: {
                HAPLogCharacteristicError(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "Unexpected Media Container Type: %u.",
                        rawMediaContainerType);
            }
                HAPFatalError();
        }
    }

    // Selected Video Parameters.
    {
        HAPAssert(selectedVideoParametersTLV.value.bytes);
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedVideoParametersTLV.value.bytes,
                selectedVideoParametersTLV.value.numBytes);
        HAPTLV selectedVideoCodecTypeTLV, selectedVideoCodecParametersTLV, selectedVideoAttributesTLV;
        selectedVideoCodecTypeTLV.type = 1;
        selectedVideoCodecParametersTLV.type = 2;
        selectedVideoAttributesTLV.type = 3;
        err = HAPTLVReaderGetAll(
                &subReader,
                (HAPTLV* const[]) { &selectedVideoCodecTypeTLV,
                                    &selectedVideoCodecParametersTLV,
                                    &selectedVideoAttributesTLV,
                                    NULL });
        HAPAssert(!err);

        // Selected Video Codec Type / Selected Video Codec Parameters.
        {
            HAPAssert(selectedVideoCodecTypeTLV.value.bytes);
            HAPAssert(selectedVideoCodecTypeTLV.value.numBytes == sizeof(uint8_t));
            uint8_t rawVideoCodecType = HAPReadUInt8(selectedVideoCodecTypeTLV.value.bytes);
            HAPAssert(selectedVideoCodecParametersTLV.value.bytes);
            HAPTLVReader sub2Reader;
            HAPTLVReaderCreate(
                    &sub2Reader,
                    (void*) (uintptr_t) selectedVideoCodecParametersTLV.value.bytes,
                    selectedVideoCodecParametersTLV.value.numBytes);
            switch (rawVideoCodecType) {
                case 0: {
                    // H.264.
                    selectedConfig->video.codecType = kHAPVideoCodecType_H264;
                    HAPH264VideoCodecParameters* codecParameters = &selectedConfig->video.codecParameters.h264;

                    // Selected Video Codec Parameters.
                    HAPTLV profileIDTLV, levelTLV, bitRateTLV, iFrameIntervalTLV;
                    profileIDTLV.type = 1;
                    levelTLV.type = 2;
                    bitRateTLV.type = 3;
                    iFrameIntervalTLV.type = 4;
                    err = HAPTLVReaderGetAll(
                            &sub2Reader,
                            (HAPTLV* const[]) { &profileIDTLV, &levelTLV, &bitRateTLV, &iFrameIntervalTLV, NULL });
                    HAPAssert(!err);

                    // ProfileID.
                    HAPAssert(profileIDTLV.value.bytes);
                    HAPAssert(profileIDTLV.value.numBytes == sizeof(uint8_t));
                    uint8_t rawProfileID = HAPReadUInt8(profileIDTLV.value.bytes);
                    switch (rawProfileID) {
                        case 0: {
                            codecParameters->profile = kHAPH264VideoCodecProfile_ConstrainedBaseline;
                            break;
                        }
                        case 1: {
                            codecParameters->profile = kHAPH264VideoCodecProfile_Main;
                            break;
                        }
                        case 2: {
                            codecParameters->profile = kHAPH264VideoCodecProfile_High;
                            break;
                        }
                        default: {
                            HAPLogCharacteristicError(
                                    &kHAPLog_Default,
                                    request.characteristic,
                                    request.service,
                                    request.accessory,
                                    "Unexpected H.264 ProfileID: %u.",
                                    rawProfileID);
                        }
                            HAPFatalError();
                    }

                    // Level.
                    HAPAssert(levelTLV.value.bytes);
                    HAPAssert(levelTLV.value.numBytes == sizeof(uint8_t));
                    uint8_t rawLevel = HAPReadUInt8(levelTLV.value.bytes);
                    switch (rawLevel) {
                        case 0: {
                            codecParameters->level = kHAPH264VideoCodecProfileLevel_3_1;
                            break;
                        }
                        case 1: {
                            codecParameters->level = kHAPH264VideoCodecProfileLevel_3_2;
                            break;
                        }
                        case 2: {
                            codecParameters->level = kHAPH264VideoCodecProfileLevel_4;
                            break;
                        }
                        default: {
                            HAPLogCharacteristicError(
                                    &kHAPLog_Default,
                                    request.characteristic,
                                    request.service,
                                    request.accessory,
                                    "Unexpected H.264 Level: %u.",
                                    rawLevel);
                        }
                            HAPFatalError();
                    }

                    // Packetization mode.
                    codecParameters->packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved;

                    // Bit Rate.
                    HAPAssert(bitRateTLV.value.bytes);
                    HAPAssert(bitRateTLV.value.numBytes == sizeof(uint32_t));
                    uint32_t rawBitRate = HAPReadLittleUInt32(bitRateTLV.value.bytes);
                    codecParameters->bitRate = rawBitRate;

                    // I-Frame Interval.
                    HAPAssert(iFrameIntervalTLV.value.bytes);
                    HAPAssert(iFrameIntervalTLV.value.numBytes == sizeof(uint32_t));
                    uint32_t rawIFrameInterval = HAPReadLittleUInt32(iFrameIntervalTLV.value.bytes);
                    codecParameters->iFrameInterval = rawIFrameInterval;
                    break;
                }
                default: {
                    HAPLogCharacteristicError(
                            &kHAPLog_Default,
                            request.characteristic,
                            request.service,
                            request.accessory,
                            "Unexpected Video Codec Type: %u.",
                            rawVideoCodecType);
                }
                    HAPFatalError();
            }
        }

        // Selected Video Attributes.
        {
            HAPVideoAttributes* attributes = &selectedConfig->video.attributes;

            HAPAssert(selectedVideoAttributesTLV.value.bytes);
            HAPTLVReader sub2Reader;
            HAPTLVReaderCreate(
                    &sub2Reader,
                    (void*) (uintptr_t) selectedVideoAttributesTLV.value.bytes,
                    selectedVideoAttributesTLV.value.numBytes);
            HAPTLV imageWidthTLV, imageHeightTLV, frameRateTLV;
            imageWidthTLV.type = 1;
            imageHeightTLV.type = 2;
            frameRateTLV.type = 3;
            err = HAPTLVReaderGetAll(
                    &sub2Reader, (HAPTLV* const[]) { &imageWidthTLV, &imageHeightTLV, &frameRateTLV, NULL });
            HAPAssert(!err);

            // Image Width.
            HAPAssert(imageWidthTLV.value.bytes);
            HAPAssert(imageWidthTLV.value.numBytes == sizeof(uint16_t));
            uint16_t rawImageWidth = HAPReadLittleUInt16(imageWidthTLV.value.bytes);
            attributes->width = rawImageWidth;

            // Image Height.
            HAPAssert(imageHeightTLV.value.bytes);
            HAPAssert(imageHeightTLV.value.numBytes == sizeof(uint16_t));
            uint16_t rawImageHeight = HAPReadLittleUInt16(imageHeightTLV.value.bytes);
            attributes->height = rawImageHeight;

            // Frame rate.
            HAPAssert(frameRateTLV.value.bytes);
            HAPAssert(frameRateTLV.value.numBytes == sizeof(uint8_t));
            uint8_t rawFrameRate = HAPReadUInt8(frameRateTLV.value.bytes);
            attributes->maxFrameRate = rawFrameRate;
        }
    }

    // Selected Audio Parameters.
    {
        HAPAssert(selectedAudioParametersTLV.value.bytes);
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader,
                (void*) (uintptr_t) selectedAudioParametersTLV.value.bytes,
                selectedAudioParametersTLV.value.numBytes);
        HAPTLV selectedAudioCodecTypeTLV, selectedAudioCodecParametersTLV;
        selectedAudioCodecTypeTLV.type = 1;
        selectedAudioCodecParametersTLV.type = 2;
        err = HAPTLVReaderGetAll(
                &subReader, (HAPTLV* const[]) { &selectedAudioCodecTypeTLV, &selectedAudioCodecParametersTLV, NULL });
        HAPAssert(!err);

        // Selected Audio Codec Type.
        HAPAssert(selectedAudioCodecTypeTLV.value.bytes);
        HAPAssert(selectedAudioCodecTypeTLV.value.numBytes == sizeof(uint8_t));
        uint8_t rawAudioCodecType = HAPReadUInt8(selectedAudioCodecTypeTLV.value.bytes);
        switch (rawAudioCodecType) {
            case 0: {
                selectedConfig->audio.codecType = kHAPAudioCodecType_AAC_LC;
                break;
            }
            default: {
                HAPLogCharacteristicError(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "Unexpected Audio Codec Type: %u.",
                        rawAudioCodecType);
            }
                HAPFatalError();
        }

        // Selected Audio Codec Parameters.
        HAPAudioCodecParameters* codecParameters = &selectedConfig->audio.codecParameters;

        HAPAssert(selectedAudioCodecParametersTLV.value.bytes);
        HAPTLVReader sub2Reader;
        HAPTLVReaderCreate(
                &sub2Reader,
                (void*) (uintptr_t) selectedAudioCodecParametersTLV.value.bytes,
                selectedAudioCodecParametersTLV.value.numBytes);
        HAPTLV audioChannelsTLV, bitRateModeTLV, sampleRateTLV, bitRateTLV;
        audioChannelsTLV.type = 1;
        bitRateModeTLV.type = 2;
        sampleRateTLV.type = 3;
        bitRateTLV.type = 4;
        err = HAPTLVReaderGetAll(
                &sub2Reader,
                (HAPTLV* const[]) { &audioChannelsTLV, &bitRateModeTLV, &sampleRateTLV, &bitRateTLV, NULL });
        HAPAssert(!err);

        // Audio Channels.
        HAPAssert(audioChannelsTLV.value.bytes);
        HAPAssert(audioChannelsTLV.value.numBytes == sizeof(uint8_t));
        uint8_t rawAudioChannels = HAPReadUInt8(audioChannelsTLV.value.bytes);
        codecParameters->numberOfChannels = rawAudioChannels;

        // Bit-rate Mode.
        HAPAssert(bitRateModeTLV.value.bytes);
        HAPAssert(bitRateModeTLV.value.numBytes == sizeof(uint8_t));
        uint8_t rawBitRateMode = HAPReadUInt8(bitRateModeTLV.value.bytes);
        switch (rawBitRateMode) {
            case 0: {
                codecParameters->bitRateMode = kHAPAudioCodecBitRateControlMode_Variable;
                break;
            }
            case 1: {
                codecParameters->bitRateMode = kHAPAudioCodecBitRateControlMode_Constant;
                break;
            }
            default: {
                HAPLogCharacteristicError(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "Unexpected Bit-rate Mode: %u.",
                        rawAudioCodecType);
            }
                HAPFatalError();
        }

        // Sample Rate.
        HAPAssert(sampleRateTLV.value.bytes);
        HAPAssert(sampleRateTLV.value.numBytes == sizeof(uint8_t));
        uint8_t rawSampleRate = HAPReadUInt8(sampleRateTLV.value.bytes);
        switch (rawSampleRate) {
            case 0: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_8KHz;
                break;
            }
            case 1: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_16KHz;
                break;
            }
            case 2: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_24KHz;
                break;
            }
            case 3: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_32KHz;
                break;
            }
            case 4: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_44_1KHz;
                break;
            }
            case 5: {
                codecParameters->sampleRate = kHAPAudioCodecSampleRate_48KHz;
                break;
            }
            default: {
                HAPLogCharacteristicError(
                        &kHAPLog_Default,
                        request.characteristic,
                        request.service,
                        request.accessory,
                        "Unexpected Sample Rate: %u.",
                        rawAudioCodecType);
            }
                HAPFatalError();
        }

        // Bit Rate.
        HAPAssert(bitRateTLV.value.bytes);
        HAPAssert(bitRateTLV.value.numBytes == sizeof(uint32_t));
        uint32_t rawBitRate = HAPReadLittleUInt32(bitRateTLV.value.bytes);
        codecParameters->bitRate = rawBitRate;
    }
    {
        char logBytes[1024];
        HAPStringBuilder stringBuilder;
        HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
        HAPStringBuilderAppend(&stringBuilder, "Selected Camera Recording Configuration:");
        {
            HAPStringBuilderAppend(&stringBuilder, "\n- Selected Camera Recording Parameters:");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n  - Prebuffer Duration: %llums",
                    (unsigned long long) (selectedConfig->recording.prebufferDuration / HAPMillisecond));
            HAPStringBuilderAppend(&stringBuilder, "\n  - Event Trigger Type:");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n    - Motion: %s",
                    (selectedConfig->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Motion) ? "Yes" : "No");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n    - Doorbell: %s",
                    (selectedConfig->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Doorbell) ? "Yes" :
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
            }
        }
        {
            HAPStringBuilderAppend(&stringBuilder, "\n- Selected Video Parameters:");
            HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Video Codec Type: ");
            switch (selectedConfig->video.codecType) {
                case kHAPVideoCodecType_H264: {
                    const HAPH264VideoCodecParameters* codecParameters = &selectedConfig->video.codecParameters.h264;
                    HAPStringBuilderAppend(&stringBuilder, "H.264");
                    HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Video Codec Parameters:");
                    HAPStringBuilderAppend(&stringBuilder, "\n    - ProfileID: ");
                    switch (codecParameters->profile) {
                        case kHAPH264VideoCodecProfile_ConstrainedBaseline: {
                            HAPStringBuilderAppend(&stringBuilder, "Constrained Baseline");
                            break;
                        }
                        case kHAPH264VideoCodecProfile_Main: {
                            HAPStringBuilderAppend(&stringBuilder, "Main");
                            break;
                        }
                        case kHAPH264VideoCodecProfile_High: {
                            HAPStringBuilderAppend(&stringBuilder, "High");
                            break;
                        }
                    }
                    HAPStringBuilderAppend(&stringBuilder, "\n    - Level: ");
                    switch (codecParameters->level) {
                        case kHAPH264VideoCodecProfileLevel_3_1: {
                            HAPStringBuilderAppend(&stringBuilder, "3.1");
                            break;
                        }
                        case kHAPH264VideoCodecProfileLevel_3_2: {
                            HAPStringBuilderAppend(&stringBuilder, "3.2");
                            break;
                        }
                        case kHAPH264VideoCodecProfileLevel_4: {
                            HAPStringBuilderAppend(&stringBuilder, "4");
                            break;
                        }
                    }
                    HAPStringBuilderAppend(
                            &stringBuilder, "\n    - Bit Rate: %lu kbit/s", (unsigned long) codecParameters->bitRate);
                    HAPStringBuilderAppend(
                            &stringBuilder,
                            "\n    - I-Frame Interval: %lums",
                            (unsigned long) codecParameters->iFrameInterval);
                    break;
                }
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
        {
            HAPStringBuilderAppend(&stringBuilder, "\n- Selected Audio Parameters:");
            HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Audio Codec Type: ");
            switch (selectedConfig->audio.codecType) {
                case kHAPAudioCodecType_AAC_LC: {
                    HAPStringBuilderAppend(&stringBuilder, "AAC-LC");
                }
                    goto logSelectedAudioCodecParameters;
                case kHAPAudioCodecType_PCMU:
                case kHAPAudioCodecType_PCMA:
                case kHAPAudioCodecType_AAC_ELD:
                case kHAPAudioCodecType_Opus:
                case kHAPAudioCodecType_MSBC:
                case kHAPAudioCodecType_AMR:
                case kHAPAudioCodecType_AMR_WB: {
                }
                    HAPFatalError();
                logSelectedAudioCodecParameters : {
                    const HAPAudioCodecParameters* codecParameters = &selectedConfig->audio.codecParameters;
                    HAPStringBuilderAppend(&stringBuilder, "\n  - Selected Audio Codec Parameters:");
                    HAPStringBuilderAppend(
                            &stringBuilder, "\n    - Audio Channels: %u", codecParameters->numberOfChannels);
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
            }
        }
        HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
        HAPLogCharacteristicInfo(
                &kHAPLog_Default,
                request.characteristic,
                request.service,
                request.accessory,
                "%s",
                HAPStringBuilderGetString(&stringBuilder));
    }

    // Validate that reported configuration is supported.
    {
        // Selected Camera Recording Parameters.
        {
            // Prebuffer Duration.
            HAPAssert(
                    selectedConfig->recording.prebufferDuration <=
                    cameraRecordingConfiguration->recording.prebufferDuration);

            // Event Trigger Type.
            HAPAssert(
                    !(selectedConfig->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Motion) ||
                    (cameraRecordingConfiguration->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Motion));
            HAPAssert(
                    !(selectedConfig->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Doorbell) ||
                    (cameraRecordingConfiguration->recording.eventTriggerTypes & kHAPCameraEventTriggerTypes_Doorbell));

            // Selected Media Container Configuration.
            bool isValid = false;
            if (cameraRecordingConfiguration->recording.containerConfigurations) {
                for (size_t i = 0; cameraRecordingConfiguration->recording.containerConfigurations[i]; i++) {
                    const HAPCameraSupportedMediaContainerConfiguration* containerConfiguration =
                            cameraRecordingConfiguration->recording.containerConfigurations[i];

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

                    isValid = true;
                    break;
                }
            }
            HAPAssert(isValid);
        }

        // Selected Video Parameters.
        {
            bool isValid = false;
            if (cameraRecordingConfiguration->video.configurations) {
                for (size_t i = 0; cameraRecordingConfiguration->video.configurations[i]; i++) {
                    const HAPCameraSupportedVideoCodecConfiguration* configuration =
                            cameraRecordingConfiguration->video.configurations[i];

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

                    isValid = true;
                    break;
                }
            }
            HAPAssert(isValid);
        }

        // Selected Audio Parameters.
        {
            bool isValid = false;
            if (cameraRecordingConfiguration->audio.configurations) {
                for (size_t i = 0; cameraRecordingConfiguration->audio.configurations[i]; i++) {
                    const HAPCameraSupportedAudioCodecConfiguration* configuration =
                            cameraRecordingConfiguration->audio.configurations[i];

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
                    }

                    isValid = true;
                    break;
                }
            }
            HAPAssert(isValid);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

int main() {
    HAPError err;
    HAPPlatformCreate();

    // Prepare key-value store.
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage.
    HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
    IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
    HAPIPReadContext ipReadContexts[kAttributeCount];
    HAPIPWriteContext ipWriteContexts[kAttributeCount];
    uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    HAPIPAccessoryServerStorage ipAccessoryServerStorage = { .sessions = ipSessions,
                                                             .numSessions = HAPArrayCount(ipSessions),
                                                             .readContexts = ipReadContexts,
                                                             .numReadContexts = HAPArrayCount(ipReadContexts),
                                                             .writeContexts = ipWriteContexts,
                                                             .numWriteContexts = HAPArrayCount(ipWriteContexts),
                                                             .scratchBuffer = { .bytes = ipScratchBuffer,
                                                                                .numBytes = sizeof ipScratchBuffer } };

    // Prepare Camera.
    static HAPCameraStreamingSession cameraStreamingSessions[kApp_NumStreams];
    static HAPCameraStreamingSessionSetup cameraStreamingSessionSetups[kHAPIPSessionStorage_DefaultNumElements];
    static HAPCameraStreamingSessionStorage cameraStreamingSessionStorage = {
        .sessions = cameraStreamingSessions,
        .numSessions = HAPArrayCount(cameraStreamingSessions),
        .setups = cameraStreamingSessionSetups,
        .numSetups = HAPArrayCount(cameraStreamingSessionSetups)
    };

    // Initialize accessory server.
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);
    HAPAccessoryServer accessoryServer;
    HAPAccessoryServerCreate(
            &accessoryServer,
            &(const HAPAccessoryServerOptions) {
                    .maxPairings = kHAPPairingStorage_MinElements,
                    .ip = { .transport = &kHAPAccessoryServerTransport_IP,
                            .accessoryServerStorage = &ipAccessoryServerStorage,
                            .camera = { .streamingSessionStorage = &cameraStreamingSessionStorage } } },
            &platform,
            &(const HAPAccessoryServerCallbacks) { .handleUpdatedState =
                                                           HAPAccessoryServerHelperHandleUpdatedAccessoryServerState },
            NULL);

    // Start accessory server.
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Running);

    // Discover IP accessory server.
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    HAPAssert(!err);
    HAPAssert(!serverInfo.statusFlags.isNotPaired);

    // Create fake security session.
    HAPSession session;
    TestCreateFakeSecuritySession(&session, &accessoryServer, controllerPairingID);

    // Enable Camera Event Recordings.
    HAPLog(&kHAPLog_Default, "Enabling Camera Event Recordings.");
    {
        const HAPUInt8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &session,
            .characteristic = &cameraEventRecordingManagementActiveCharacteristic,
            .service = &cameraEventRecordingManagementService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        err = request.characteristic->callbacks.handleWrite(&accessoryServer, &request, 1, NULL);
        HAPAssert(!err);
    }
    HAPLog(&kHAPLog_Default, "Checking that Camera Event Recordings are enabled.");
    {
        const HAPUInt8CharacteristicReadRequest request = { .transportType = kHAPTransportType_IP,
                                                            .session = &session,
                                                            .characteristic =
                                                                    &cameraEventRecordingManagementActiveCharacteristic,
                                                            .service = &cameraEventRecordingManagementService,
                                                            .accessory = &accessory };
        uint8_t value;
        err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &value, NULL);
        HAPAssert(!err);
        HAPAssert(value == 1);
    }

    // Get supported configurations.
    HAPLog(&kHAPLog_Default, "Reading Supported Camera Recording Configuration.");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &session,
            .characteristic = &cameraEventRecordingManagementSupportedCameraRecordingConfigurationCharacteristic,
            .service = &cameraEventRecordingManagementService,
            .accessory = &accessory
        };
        err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &responseWriter, NULL);
        HAPAssert(!err);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        bool foundPrebufferDuration = false;
        HAPTime prebufferDuration = 0;
        bool foundEventTriggerType = false;
        HAPCameraEventTriggerTypes eventTriggerTypes = 0;
        size_t numMediaContainerConfigurations = 0;
        struct {
            HAPMediaContainerType containerType;
            union {
                HAPFragmentedMP4MediaContainerParameters fragmentedMP4;
            } containerParameters;
        } mediaContainerConfigurations[20];
        HAPRawBufferZero(mediaContainerConfigurations, sizeof mediaContainerConfigurations);
        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            HAPAssert(!err);
            if (!valid) {
                break;
            }

            switch (tlv.type) {
                case 1: {
                    // Prebuffer Duration.
                    HAPAssert(!foundPrebufferDuration);
                    foundPrebufferDuration = true;
                    HAPAssert(tlv.value.numBytes == sizeof(uint32_t));
                    uint32_t rawPrebufferDuration = HAPReadLittleUInt32(tlv.value.bytes);
                    prebufferDuration = rawPrebufferDuration * HAPMillisecond;
                    HAPAssert(prebufferDuration >= 4 * HAPSecond);
                    break;
                }
                case 2: {
                    // Event Trigger Type.
                    HAPAssert(!foundEventTriggerType);
                    foundEventTriggerType = true;
                    HAPAssert(tlv.value.numBytes == sizeof(uint64_t));
                    uint64_t rawEventTriggerType = HAPReadLittleUInt64(tlv.value.bytes);
                    if (rawEventTriggerType & 1U << 0U) {
                        eventTriggerTypes |= kHAPCameraEventTriggerTypes_Motion;
                    }
                    if (rawEventTriggerType & 1U << 1U) {
                        eventTriggerTypes |= kHAPCameraEventTriggerTypes_Doorbell;
                    }
                    if (rawEventTriggerType & ~((1ULL << 0) | (1ULL << 1))) {
                        HAPLogCharacteristicError(
                                &kHAPLog_Default,
                                request.characteristic,
                                request.service,
                                request.accessory,
                                "Unexpected Event Trigger Type: 0x%016llX.",
                                (unsigned long long) rawEventTriggerType);
                        HAPFatalError();
                    }
                    break;
                }
                case 3: {
                    // Media Container Configuration.
                    HAPAssert(numMediaContainerConfigurations < HAPArrayCount(mediaContainerConfigurations));
                    HAPTLVReader subReader;
                    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
                    HAPTLV mediaContainerTypeTLV, mediaContainerParametersTLV;
                    mediaContainerTypeTLV.type = 1;
                    mediaContainerParametersTLV.type = 2;
                    err = HAPTLVReaderGetAll(
                            &subReader,
                            (HAPTLV* const[]) { &mediaContainerTypeTLV, &mediaContainerParametersTLV, NULL });
                    HAPAssert(!err);

                    // Media Container Type / Media Container Parameters.
                    HAPAssert(mediaContainerTypeTLV.value.bytes);
                    HAPAssert(mediaContainerTypeTLV.value.numBytes == sizeof(uint8_t));
                    uint8_t rawMediaContainerType = HAPReadUInt8(mediaContainerTypeTLV.value.bytes);
                    HAPAssert(mediaContainerParametersTLV.value.bytes);
                    HAPTLVReader sub2Reader;
                    HAPTLVReaderCreate(
                            &sub2Reader,
                            (void*) (uintptr_t) mediaContainerParametersTLV.value.bytes,
                            mediaContainerParametersTLV.value.numBytes);
                    switch (rawMediaContainerType) {
                        case 0: {
                            // Fragmented MP4.
                            mediaContainerConfigurations[numMediaContainerConfigurations].containerType =
                                    kHAPMediaContainerType_FragmentedMP4;

                            // Media Container Parameters.
                            HAPTLV fragmentDurationTLV;
                            fragmentDurationTLV.type = 1;
                            err = HAPTLVReaderGetAll(&sub2Reader, (HAPTLV* const[]) { &fragmentDurationTLV, NULL });
                            HAPAssert(!err);

                            // Fragment Duration.
                            HAPAssert(fragmentDurationTLV.value.bytes);
                            HAPAssert(fragmentDurationTLV.value.numBytes == sizeof(uint32_t));
                            uint32_t rawFragmentDuration = HAPReadLittleUInt32(fragmentDurationTLV.value.bytes);
                            mediaContainerConfigurations[numMediaContainerConfigurations]
                                    .containerParameters.fragmentedMP4.fragmentDuration = rawFragmentDuration;
                            break;
                        }
                        default: {
                            HAPLogCharacteristicError(
                                    &kHAPLog_Default,
                                    request.characteristic,
                                    request.service,
                                    request.accessory,
                                    "Unexpected Media Container Type: %u.",
                                    rawMediaContainerType);
                        }
                            HAPFatalError();
                    }

                    numMediaContainerConfigurations++;
                    break;
                }
                default: {
                    HAPLogCharacteristic(
                            &kHAPLog_Default,
                            request.characteristic,
                            request.service,
                            request.accessory,
                            "Unexpected TLV type: 0x%02X. Treating as separator.",
                            tlv.type);
                    break;
                }
            }
        }
        HAPAssert(foundPrebufferDuration);
        HAPAssert(foundEventTriggerType);
        {
            char logBytes[8 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Supported Camera Recording Configuration:");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n- Prebuffer Duration: %llums.",
                    (unsigned long long) (prebufferDuration / HAPMillisecond));
            HAPStringBuilderAppend(&stringBuilder, "\n- Event Trigger Type:");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n  - Motion: %s",
                    (eventTriggerTypes & kHAPCameraEventTriggerTypes_Motion) ? "Yes" : "No");
            HAPStringBuilderAppend(
                    &stringBuilder,
                    "\n  - Doorbell: %s",
                    (eventTriggerTypes & kHAPCameraEventTriggerTypes_Doorbell) ? "Yes" : "No");
            for (size_t i = 0; i < numMediaContainerConfigurations; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- Media Container Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Media Container Type: ");
                switch (mediaContainerConfigurations[i].containerType) {
                    case kHAPMediaContainerType_FragmentedMP4: {
                        HAPStringBuilderAppend(&stringBuilder, "Fragmented MP4");
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Media Container Parameters:");
                        HAPStringBuilderAppend(
                                &stringBuilder,
                                "\n    - Fragment Duration: %lums",
                                (unsigned long) mediaContainerConfigurations[i]
                                        .containerParameters.fragmentedMP4.fragmentDuration);
                        break;
                    }
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }

        // Validate that reported configuration matches values specified in the HAPAccessory structure.
        HAPAssert(prebufferDuration == supportedCameraRecordingConfiguration.recording.prebufferDuration);
        HAPAssert(eventTriggerTypes == supportedCameraRecordingConfiguration.recording.eventTriggerTypes);
        bool configurationWasSpecified[HAPArrayCount(mediaContainerConfigurations)] = { 0 };
        size_t i = 0;
        if (supportedCameraRecordingConfiguration.recording.containerConfigurations) {
            for (i = 0; supportedCameraRecordingConfiguration.recording.containerConfigurations[i]; i++) {
                const HAPCameraSupportedMediaContainerConfiguration* containerConfiguration =
                        supportedCameraRecordingConfiguration.recording.containerConfigurations[i];
                bool configurationWasReported = false;
                for (size_t j = 0; j < numMediaContainerConfigurations; j++) {
                    if (configurationWasSpecified[j]) {
                        continue;
                    }
                    if (mediaContainerConfigurations[j].containerType != containerConfiguration->containerType) {
                        continue;
                    }
                    switch (mediaContainerConfigurations[j].containerType) {
                        case kHAPMediaContainerType_FragmentedMP4: {
                            const HAPFragmentedMP4MediaContainerParameters* containerParameters =
                                    containerConfiguration->containerParameters;
                            if (mediaContainerConfigurations[j].containerParameters.fragmentedMP4.fragmentDuration !=
                                containerParameters->fragmentDuration) {
                                continue;
                            }
                            break;
                        }
                    }
                    configurationWasSpecified[j] = true;
                    configurationWasReported = true;
                    break;
                }
                HAPAssert(configurationWasReported);
            }
        }
        HAPAssert(i == numMediaContainerConfigurations);
        for (i = 0; i < numMediaContainerConfigurations; i++) {
            HAPAssert(configurationWasSpecified[i]);
        }
    }
    HAPLog(&kHAPLog_Default, "Reading Supported Video Recording Configuration.");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &session,
            .characteristic = &cameraEventRecordingManagementSupportedVideoRecordingConfigurationCharacteristic,
            .service = &cameraEventRecordingManagementService,
            .accessory = &accessory
        };
        err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &responseWriter, NULL);
        HAPAssert(!err);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        size_t numVideoCodecConfigurations = 0;
        struct {
            HAPVideoCodecType codecType;
            union {
                HAPH264VideoCodecParameters h264;
            } codecParameters;
            size_t numAttributes;
            HAPVideoAttributes attributes[50];
        } videoCodecConfigurations[20];
        HAPRawBufferZero(videoCodecConfigurations, sizeof videoCodecConfigurations);
        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            HAPAssert(!err);
            if (!valid) {
                break;
            }

            switch (tlv.type) {
                case 1: {
                    // Video Codec Configuration.
                    HAPAssert(numVideoCodecConfigurations < HAPArrayCount(videoCodecConfigurations));
                    HAPTLVReader subReader;
                    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);

                    bool foundVideoCodecType = false;
                    HAPTLV videoCodecParametersTLV;
                    videoCodecParametersTLV.type = 2;
                    videoCodecParametersTLV.value.bytes = NULL;
                    for (;;) {
                        err = HAPTLVReaderGetNext(&subReader, &valid, &tlv);
                        HAPAssert(!err);
                        if (!valid) {
                            break;
                        }

                        switch (tlv.type) {
                            case 1: {
                                // Video Codec Type.
                                HAPAssert(!foundVideoCodecType);
                                foundVideoCodecType = true;
                                HAPAssert(tlv.value.bytes);
                                HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                uint8_t rawVideoCodecType = HAPReadUInt8(tlv.value.bytes);
                                switch (rawVideoCodecType) {
                                    case 0: {
                                        // H.264.
                                        videoCodecConfigurations[numVideoCodecConfigurations].codecType =
                                                kHAPVideoCodecType_H264;
                                        break;
                                    }
                                    default: {
                                        HAPLogCharacteristicError(
                                                &kHAPLog_Default,
                                                request.characteristic,
                                                request.service,
                                                request.accessory,
                                                "Unexpected Video Codec Type: %u.",
                                                rawVideoCodecType);
                                    }
                                        HAPFatalError();
                                }
                                if (videoCodecParametersTLV.value.bytes) {
                                    goto parseVideoCodecParameters;
                                }
                                break;
                            }
                            case 2: {
                                // Video Codec Parameters.
                                HAPAssert(!videoCodecParametersTLV.value.bytes);
                                videoCodecParametersTLV = tlv;
                                if (foundVideoCodecType) {
                                    goto parseVideoCodecParameters;
                                }
                                break;
                            }
                            parseVideoCodecParameters : {
                                HAPAssert(foundVideoCodecType);
                                HAPAssert(videoCodecParametersTLV.value.bytes);
                                HAPTLVReader sub2Reader;
                                HAPTLVReaderCreate(
                                        &sub2Reader,
                                        (void*) (uintptr_t) videoCodecParametersTLV.value.bytes,
                                        videoCodecParametersTLV.value.numBytes);

                                switch (videoCodecConfigurations[numVideoCodecConfigurations].codecType) {
                                    case kHAPVideoCodecType_H264: {
                                        HAPH264VideoCodecParameters* codecParameters =
                                                &videoCodecConfigurations[numVideoCodecConfigurations]
                                                         .codecParameters.h264;

                                        bool foundProfileID = false;
                                        bool foundLevel = false;
                                        bool foundBitRate = false;
                                        bool foundIFrameInterval = false;
                                        for (;;) {
                                            err = HAPTLVReaderGetNext(&sub2Reader, &valid, &tlv);
                                            HAPAssert(!err);
                                            if (!valid) {
                                                break;
                                            }

                                            switch (tlv.type) {
                                                case 1: {
                                                    foundProfileID = true;
                                                    HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                                    uint8_t rawProfileID = HAPReadUInt8(tlv.value.bytes);
                                                    switch (rawProfileID) {
                                                        case 0: {
                                                            HAPAssert(
                                                                    !(codecParameters->profile &
                                                                      kHAPH264VideoCodecProfile_ConstrainedBaseline));
                                                            codecParameters->profile |=
                                                                    kHAPH264VideoCodecProfile_ConstrainedBaseline;
                                                            break;
                                                        }
                                                        case 1: {
                                                            HAPAssert(
                                                                    !(codecParameters->profile &
                                                                      kHAPH264VideoCodecProfile_Main));
                                                            codecParameters->profile |= kHAPH264VideoCodecProfile_Main;
                                                            break;
                                                        }
                                                        case 2: {
                                                            HAPAssert(
                                                                    !(codecParameters->profile &
                                                                      kHAPH264VideoCodecProfile_High));
                                                            codecParameters->profile |= kHAPH264VideoCodecProfile_High;
                                                            break;
                                                        }
                                                        default: {
                                                            HAPLogCharacteristicError(
                                                                    &kHAPLog_Default,
                                                                    request.characteristic,
                                                                    request.service,
                                                                    request.accessory,
                                                                    "Unexpected H.264 ProfileID: %u.",
                                                                    rawProfileID);
                                                        }
                                                            HAPFatalError();
                                                    }
                                                    break;
                                                }
                                                case 2: {
                                                    foundLevel = true;
                                                    HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                                    uint8_t rawLevel = HAPReadUInt8(tlv.value.bytes);
                                                    switch (rawLevel) {
                                                        case 0: {
                                                            HAPAssert(
                                                                    !(codecParameters->level &
                                                                      kHAPH264VideoCodecProfileLevel_3_1));
                                                            codecParameters->level |=
                                                                    kHAPH264VideoCodecProfileLevel_3_1;
                                                            break;
                                                        }
                                                        case 1: {
                                                            HAPAssert(
                                                                    !(codecParameters->level &
                                                                      kHAPH264VideoCodecProfileLevel_3_2));
                                                            codecParameters->level |=
                                                                    kHAPH264VideoCodecProfileLevel_3_2;
                                                            break;
                                                        }
                                                        case 2: {
                                                            HAPAssert(
                                                                    !(codecParameters->level &
                                                                      kHAPH264VideoCodecProfileLevel_4));
                                                            codecParameters->level |= kHAPH264VideoCodecProfileLevel_4;
                                                            break;
                                                        }
                                                        default: {
                                                            HAPLogCharacteristicError(
                                                                    &kHAPLog_Default,
                                                                    request.characteristic,
                                                                    request.service,
                                                                    request.accessory,
                                                                    "Unexpected H.264 Level: %u.",
                                                                    rawLevel);
                                                        }
                                                            HAPFatalError();
                                                    }
                                                    break;
                                                }
                                                case 3: {
                                                    HAPAssert(!foundBitRate);
                                                    foundBitRate = true;
                                                    HAPAssert(tlv.value.numBytes == sizeof(uint32_t));
                                                    uint32_t rawBitRate = HAPReadLittleUInt32(tlv.value.bytes);
                                                    codecParameters->bitRate = rawBitRate;
                                                    break;
                                                }
                                                case 4: {
                                                    HAPAssert(!foundIFrameInterval);
                                                    foundIFrameInterval = true;
                                                    HAPAssert(tlv.value.numBytes == sizeof(uint32_t));
                                                    uint32_t rawIFrameInterval = HAPReadLittleUInt32(tlv.value.bytes);
                                                    codecParameters->iFrameInterval = rawIFrameInterval;
                                                    break;
                                                }
                                                default: {
                                                    HAPLogCharacteristic(
                                                            &kHAPLog_Default,
                                                            request.characteristic,
                                                            request.service,
                                                            request.accessory,
                                                            "Unexpected H.264 TLV type: 0x%02X. Treating as separator.",
                                                            tlv.type);
                                                    break;
                                                }
                                            }
                                        }
                                        codecParameters->packetizationMode |=
                                                kHAPH264VideoCodecPacketizationMode_NonInterleaved;
                                        HAPAssert(foundProfileID);
                                        HAPAssert(foundLevel);
                                        break;
                                    }
                                }
                                break;
                            }
                            case 3: {
                                // Video Attributes.
                                HAPAssert(
                                        videoCodecConfigurations[numVideoCodecConfigurations].numAttributes <
                                        HAPArrayCount(
                                                videoCodecConfigurations[numVideoCodecConfigurations].attributes));
                                HAPVideoAttributes* attributes =
                                        &videoCodecConfigurations[numVideoCodecConfigurations].attributes
                                                 [videoCodecConfigurations[numVideoCodecConfigurations].numAttributes];

                                HAPTLVReader sub2Reader;
                                HAPTLVReaderCreate(
                                        &sub2Reader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
                                HAPTLV imageWidthTLV, imageHeightTLV, frameRateTLV;
                                imageWidthTLV.type = 1;
                                imageHeightTLV.type = 2;
                                frameRateTLV.type = 3;
                                err = HAPTLVReaderGetAll(
                                        &sub2Reader,
                                        (HAPTLV* const[]) { &imageWidthTLV, &imageHeightTLV, &frameRateTLV, NULL });
                                HAPAssert(!err);

                                // Image Width.
                                HAPAssert(imageWidthTLV.value.bytes);
                                HAPAssert(imageWidthTLV.value.numBytes == sizeof(uint16_t));
                                uint16_t rawImageWidth = HAPReadLittleUInt16(imageWidthTLV.value.bytes);
                                attributes->width = rawImageWidth;

                                // Image Height.
                                HAPAssert(imageHeightTLV.value.bytes);
                                HAPAssert(imageHeightTLV.value.numBytes == sizeof(uint16_t));
                                uint16_t rawImageHeight = HAPReadLittleUInt16(imageHeightTLV.value.bytes);
                                attributes->height = rawImageHeight;

                                // Frame rate.
                                HAPAssert(frameRateTLV.value.bytes);
                                HAPAssert(frameRateTLV.value.numBytes == sizeof(uint8_t));
                                uint8_t rawFrameRate = HAPReadUInt8(frameRateTLV.value.bytes);
                                attributes->maxFrameRate = rawFrameRate;

                                videoCodecConfigurations[numVideoCodecConfigurations].numAttributes++;
                                break;
                            }
                            default: {
                                HAPLogCharacteristic(
                                        &kHAPLog_Default,
                                        request.characteristic,
                                        request.service,
                                        request.accessory,
                                        "Unexpected Configuration TLV type: 0x%02X. Treating as separator.",
                                        tlv.type);
                                break;
                            }
                        }
                    }
                    HAPAssert(foundVideoCodecType);
                    HAPAssert(videoCodecParametersTLV.value.bytes);
                    HAPAssert(videoCodecConfigurations[numVideoCodecConfigurations].numAttributes);

                    numVideoCodecConfigurations++;
                    break;
                }
                default: {
                    HAPLogCharacteristic(
                            &kHAPLog_Default,
                            request.characteristic,
                            request.service,
                            request.accessory,
                            "Unexpected TLV type: 0x%02X. Treating as separator.",
                            tlv.type);
                    break;
                }
            }
        }
        {
            char logBytes[8 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Supported Video Recording Configuration:");
            for (size_t i = 0; i < numVideoCodecConfigurations; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- Video Codec Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Video Codec Type: ");
                switch (videoCodecConfigurations[i].codecType) {
                    case kHAPVideoCodecType_H264: {
                        HAPH264VideoCodecParameters* codecParameters =
                                &videoCodecConfigurations[i].codecParameters.h264;
                        HAPStringBuilderAppend(&stringBuilder, "H.264");
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Video Codec Parameters:");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - ProfileID: [");
                        {
                            bool needsSeparator = false;
                            if (codecParameters->profile & kHAPH264VideoCodecProfile_ConstrainedBaseline) {
                                HAPStringBuilderAppend(&stringBuilder, "Constrained Baseline");
                                needsSeparator = true;
                            }
                            if (codecParameters->profile & kHAPH264VideoCodecProfile_Main) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "Main");
                                needsSeparator = true;
                            }
                            if (codecParameters->profile & kHAPH264VideoCodecProfile_High) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "High");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Level: [");
                        {
                            bool needsSeparator = false;
                            if (codecParameters->level & kHAPH264VideoCodecProfileLevel_3_1) {
                                HAPStringBuilderAppend(&stringBuilder, "3.1");
                                needsSeparator = true;
                            }
                            if (codecParameters->level & kHAPH264VideoCodecProfileLevel_3_2) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "3.2");
                                needsSeparator = true;
                            }
                            if (codecParameters->level & kHAPH264VideoCodecProfileLevel_4) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "4");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        break;
                    }
                }
                HAPStringBuilderAppend(&stringBuilder, "\n  - Video Attributes:");
                for (size_t j = 0; j < videoCodecConfigurations[i].numAttributes; j++) {
                    HAPVideoAttributes* attributes = &videoCodecConfigurations[i].attributes[j];
                    HAPStringBuilderAppend(
                            &stringBuilder,
                            "\n    - %4u x %4u @ %u fps",
                            attributes->width,
                            attributes->height,
                            attributes->maxFrameRate);
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }

        // Validate that reported configuration matches values specified in the HAPAccessory structure.
        bool configurationWasSpecified[HAPArrayCount(videoCodecConfigurations)] = { 0 };
        size_t i = 0;
        if (supportedCameraRecordingConfiguration.video.configurations) {
            for (i = 0; supportedCameraRecordingConfiguration.video.configurations[i]; i++) {
                const HAPCameraSupportedVideoCodecConfiguration* configuration =
                        supportedCameraRecordingConfiguration.video.configurations[i];
                bool configurationWasReported = false;
                for (size_t j = 0; j < numVideoCodecConfigurations; j++) {
                    if (configurationWasSpecified[j]) {
                        continue;
                    }
                    if (videoCodecConfigurations[j].codecType != configuration->codecType) {
                        continue;
                    }
                    switch (configuration[j].codecType) {
                        case kHAPVideoCodecType_H264: {
                            const HAPH264VideoCodecParameters* codecParameters = configuration->codecParameters;
                            if (videoCodecConfigurations[j].codecParameters.h264.profile != codecParameters->profile) {
                                continue;
                            }
                            if (videoCodecConfigurations[j].codecParameters.h264.level != codecParameters->level) {
                                continue;
                            }
                            if (videoCodecConfigurations[j].codecParameters.h264.packetizationMode !=
                                codecParameters->packetizationMode) {
                                continue;
                            }
                            break;
                        }
                    }
                    configurationWasSpecified[j] = true;
                    configurationWasReported = true;
                    break;
                }
                HAPAssert(configurationWasReported);
            }
        }
        HAPAssert(i == numVideoCodecConfigurations);
        for (i = 0; i < numVideoCodecConfigurations; i++) {
            HAPAssert(configurationWasSpecified[i]);
        }
    }
    HAPLog(&kHAPLog_Default, "Reading Supported Audio Recording Configuration.");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &session,
            .characteristic = &cameraEventRecordingManagementSupportedAudioRecordingConfigurationCharacteristic,
            .service = &cameraEventRecordingManagementService,
            .accessory = &accessory
        };
        err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &responseWriter, NULL);
        HAPAssert(!err);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        size_t numAudioCodecConfigurations = 0;
        struct {
            HAPAudioCodecType codecType;
            HAPAudioCodecParameters audioCodecParameters;
        } audioCodecConfigurations[20];
        HAPRawBufferZero(audioCodecConfigurations, sizeof audioCodecConfigurations);
        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            HAPAssert(!err);
            if (!valid) {
                break;
            }

            switch (tlv.type) {
                case 1: {
                    // Audio Codec Configuration.
                    HAPAssert(numAudioCodecConfigurations < HAPArrayCount(audioCodecConfigurations));
                    HAPTLVReader subReader;
                    HAPTLVReaderCreate(&subReader, (void*) (uintptr_t) tlv.value.bytes, tlv.value.numBytes);
                    HAPTLV codecTypeTLV, audioCodecParametersTLV;
                    codecTypeTLV.type = 1;
                    audioCodecParametersTLV.type = 2;
                    err = HAPTLVReaderGetAll(
                            &subReader, (HAPTLV* const[]) { &codecTypeTLV, &audioCodecParametersTLV, NULL });
                    HAPAssert(!err);

                    // Codec Type.
                    HAPAssert(codecTypeTLV.value.bytes);
                    HAPAssert(codecTypeTLV.value.numBytes == sizeof(uint8_t));
                    uint8_t rawCodecType = HAPReadUInt8(codecTypeTLV.value.bytes);
                    switch (rawCodecType) {
                        case 0: {
                            audioCodecConfigurations[numAudioCodecConfigurations].codecType = kHAPAudioCodecType_AAC_LC;
                            break;
                        }
                        default: {
                            HAPLogCharacteristicError(
                                    &kHAPLog_Default,
                                    request.characteristic,
                                    request.service,
                                    request.accessory,
                                    "Unexpected Codec Type: %u.",
                                    rawCodecType);
                        }
                            HAPFatalError();
                    }

                    // Audio Codec Parameters.
                    HAPAudioCodecParameters* codecParameters =
                            &audioCodecConfigurations[numAudioCodecConfigurations].audioCodecParameters;

                    HAPAssert(audioCodecParametersTLV.value.bytes);
                    HAPTLVReader sub2Reader;
                    HAPTLVReaderCreate(
                            &sub2Reader,
                            (void*) (uintptr_t) audioCodecParametersTLV.value.bytes,
                            audioCodecParametersTLV.value.numBytes);

                    bool foundAudioChannels = false;
                    bool foundBitRateMode = false;
                    bool foundSampleRate = false;
                    bool foundBitRate = false;
                    for (;;) {
                        err = HAPTLVReaderGetNext(&sub2Reader, &valid, &tlv);
                        HAPAssert(!err);
                        if (!valid) {
                            break;
                        }

                        switch (tlv.type) {
                            case 1: {
                                HAPAssert(!foundAudioChannels);
                                foundAudioChannels = true;
                                HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                uint8_t rawAudioChannels = HAPReadUInt8(tlv.value.bytes);
                                codecParameters->numberOfChannels = rawAudioChannels;
                                break;
                            }
                            case 2: {
                                foundBitRateMode = true;
                                HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                uint8_t rawBitRateMode = HAPReadUInt8(tlv.value.bytes);
                                switch (rawBitRateMode) {
                                    case 0: {
                                        HAPAssert(
                                                !(codecParameters->bitRateMode &
                                                  kHAPAudioCodecBitRateControlMode_Variable));
                                        codecParameters->bitRateMode |= kHAPAudioCodecBitRateControlMode_Variable;
                                        break;
                                    }
                                    case 1: {
                                        HAPAssert(
                                                !(codecParameters->bitRateMode &
                                                  kHAPAudioCodecBitRateControlMode_Constant));
                                        codecParameters->bitRateMode |= kHAPAudioCodecBitRateControlMode_Constant;
                                        break;
                                    }
                                    default: {
                                        HAPLogCharacteristicError(
                                                &kHAPLog_Default,
                                                request.characteristic,
                                                request.service,
                                                request.accessory,
                                                "Unexpected Bit-rate Mode: %u.",
                                                rawCodecType);
                                    }
                                        HAPFatalError();
                                }
                                break;
                            }
                            case 3: {
                                foundSampleRate = true;
                                HAPAssert(tlv.value.numBytes == sizeof(uint8_t));
                                uint8_t rawSampleRate = HAPReadUInt8(tlv.value.bytes);
                                switch (rawSampleRate) {
                                    case 0: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_8KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_8KHz;
                                        break;
                                    }
                                    case 1: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_16KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_16KHz;
                                        break;
                                    }
                                    case 2: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_24KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_24KHz;
                                        break;
                                    }
                                    case 3: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_32KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_32KHz;
                                        break;
                                    }
                                    case 4: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_44_1KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_44_1KHz;
                                        break;
                                    }
                                    case 5: {
                                        HAPAssert(!(codecParameters->sampleRate & kHAPAudioCodecSampleRate_48KHz));
                                        codecParameters->sampleRate |= kHAPAudioCodecSampleRate_48KHz;
                                        break;
                                    }
                                    default: {
                                        HAPLogCharacteristicError(
                                                &kHAPLog_Default,
                                                request.characteristic,
                                                request.service,
                                                request.accessory,
                                                "Unexpected Sample Rate: %u.",
                                                rawCodecType);
                                    }
                                        HAPFatalError();
                                }
                                break;
                            }
                            case 4: {
                                HAPAssert(!foundBitRate);
                                foundBitRate = true;
                                HAPAssert(tlv.value.numBytes == sizeof(uint32_t));
                                uint32_t rawBitRate = HAPReadLittleUInt32(tlv.value.bytes);
                                codecParameters->bitRate = rawBitRate;
                                break;
                            }
                        }
                    }
                    if (!foundAudioChannels) {
                        codecParameters->numberOfChannels = 1;
                    }
                    HAPAssert(foundBitRateMode);
                    HAPAssert(foundSampleRate);

                    numAudioCodecConfigurations++;
                    break;
                }
                default: {
                    HAPLogCharacteristic(
                            &kHAPLog_Default,
                            request.characteristic,
                            request.service,
                            request.accessory,
                            "Unexpected TLV type: 0x%02X. Treating as separator.",
                            tlv.type);
                    break;
                }
            }
        }
        {
            char logBytes[8 * 1024];
            HAPStringBuilder stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
            HAPStringBuilderAppend(&stringBuilder, "Supported Audio Recording Configuration:");
            for (size_t i = 0; i < numAudioCodecConfigurations; i++) {
                HAPStringBuilderAppend(&stringBuilder, "\n- Audio Codec Configuration (%zu):", i);
                HAPStringBuilderAppend(&stringBuilder, "\n  - Codec Type: ");
                switch (audioCodecConfigurations[i].codecType) {
                    case kHAPAudioCodecType_AAC_LC: {
                        HAPStringBuilderAppend(&stringBuilder, "AAC-LC");
                    }
                        goto logAudioCodecParameters;
                    case kHAPAudioCodecType_PCMU:
                    case kHAPAudioCodecType_PCMA:
                    case kHAPAudioCodecType_AAC_ELD:
                    case kHAPAudioCodecType_Opus:
                    case kHAPAudioCodecType_MSBC:
                    case kHAPAudioCodecType_AMR:
                    case kHAPAudioCodecType_AMR_WB: {
                    }
                        HAPFatalError();
                    logAudioCodecParameters : {
                        HAPAudioCodecParameters* codecParameters = &audioCodecConfigurations[i].audioCodecParameters;
                        HAPStringBuilderAppend(&stringBuilder, "\n  - Audio Codec Parameters:");
                        HAPStringBuilderAppend(
                                &stringBuilder, "\n    - Audio Channels: %u", codecParameters->numberOfChannels);
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Bit-Rate Mode: [");
                        {
                            bool needsSeparator = false;
                            if (codecParameters->bitRateMode & kHAPAudioCodecBitRateControlMode_Variable) {
                                HAPStringBuilderAppend(&stringBuilder, "Variable bit-rate");
                                needsSeparator = true;
                            }
                            if (codecParameters->bitRateMode & kHAPAudioCodecBitRateControlMode_Constant) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "Constant bit-rate");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(&stringBuilder, "\n    - Sample Rate: [");
                        {
                            bool needsSeparator = false;
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_8KHz) {
                                HAPStringBuilderAppend(&stringBuilder, "8KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_16KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "16KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_24KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "24KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_32KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "32KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_44_1KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "44.1KHz");
                                needsSeparator = true;
                            }
                            if (codecParameters->sampleRate & kHAPAudioCodecSampleRate_48KHz) {
                                if (needsSeparator) {
                                    HAPStringBuilderAppend(&stringBuilder, ", ");
                                }
                                HAPStringBuilderAppend(&stringBuilder, "48KHz");
                            }
                        }
                        HAPStringBuilderAppend(&stringBuilder, "]");
                        HAPStringBuilderAppend(
                                &stringBuilder,
                                "\n    - Bit Rate: %lu kbit/s",
                                (unsigned long) codecParameters->bitRate);
                        break;
                    }
                }
            }
            HAPAssert(!HAPStringBuilderDidOverflow(&stringBuilder));
            HAPLogCharacteristicInfo(
                    &kHAPLog_Default,
                    request.characteristic,
                    request.service,
                    request.accessory,
                    "%s",
                    HAPStringBuilderGetString(&stringBuilder));
        }

        // Validate that reported configuration matches values specified in the HAPAccessory structure.
        bool configurationWasSpecified[HAPArrayCount(audioCodecConfigurations)] = { 0 };
        size_t i = 0;
        if (supportedCameraRecordingConfiguration.audio.configurations) {
            for (i = 0; supportedCameraRecordingConfiguration.audio.configurations[i]; i++) {
                const HAPCameraSupportedAudioCodecConfiguration* configuration =
                        supportedCameraRecordingConfiguration.audio.configurations[i];
                bool configurationWasReported = false;
                for (size_t j = 0; j < numAudioCodecConfigurations; j++) {
                    if (configurationWasSpecified[j]) {
                        continue;
                    }
                    if (configuration[j].codecType != configuration->codecType) {
                        continue;
                    }
                    const HAPAudioCodecParameters* codecParameters = configuration->codecParameters;
                    if (audioCodecConfigurations[j].audioCodecParameters.numberOfChannels !=
                        codecParameters->numberOfChannels) {
                        continue;
                    }
                    if (audioCodecConfigurations[j].audioCodecParameters.bitRateMode != codecParameters->bitRateMode) {
                        continue;
                    }
                    if (audioCodecConfigurations[j].audioCodecParameters.sampleRate != codecParameters->sampleRate) {
                        continue;
                    }
                    configurationWasSpecified[j] = true;
                    configurationWasReported = true;
                    break;
                }
                HAPAssert(configurationWasReported);
            }
        }
        HAPAssert(i == numAudioCodecConfigurations);
        for (i = 0; i < numAudioCodecConfigurations; i++) {
            HAPAssert(configurationWasSpecified[i]);
        }
    }

    // Verify that initially no recording configuration is selected.
    HAPLog(&kHAPLog_Default, "Verifying that Selected Camera Recording Configuration is not available.");
    bool found;
    HAPCameraRecordingConfiguration configuration;
    ReadSelectedCameraRecordingConfiguration(
            &accessoryServer, &session, &supportedCameraRecordingConfiguration, &found, &configuration, NULL);
    HAPAssert(!found);

    // Set a new recording configuration.
    HAPLog(&kHAPLog_Default, "Writing Selected Camera Recording Configuration.");
    const HAPCameraRecordingConfiguration newConfiguration = {
        .recording = {
            .prebufferDuration = 3 * HAPSecond,
            .eventTriggerTypes =
                kHAPCameraEventTriggerTypes_Motion,
            .containerConfiguration = {
                .containerType = kHAPMediaContainerType_FragmentedMP4,
                .containerParameters.fragmentedMP4 = {
                    .fragmentDuration = 1500 // ms
                }
            }
        },
        .video = {
            .codecType = kHAPVideoCodecType_H264,
            .codecParameters.h264 = {
                .profile = kHAPH264VideoCodecProfile_Main,
                .level = kHAPH264VideoCodecProfileLevel_3_1,
                .packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved,
                .bitRate = 500,        // kbit/s
                .iFrameInterval = 5000 // ms
            },
            .attributes = {
                .width = 1280,
                .height = 720,
                .maxFrameRate = 30
            }
        },
        .audio = {
            .codecType = kHAPAudioCodecType_AAC_LC,
            .codecParameters = {
                .numberOfChannels = 1,
                .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
                .sampleRate = kHAPAudioCodecSampleRate_24KHz,
                .bitRate = 28
            }
        }
    };
    {
        uint8_t requestBytes[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

        // Selected Camera Recording Parameters.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Prebuffer Duration.
            uint8_t prebufferDurationBytes[] = { HAPExpandLittleUInt32(3 * HAPSecond / HAPMillisecond) };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = 1,
                            .value = { .bytes = prebufferDurationBytes, .numBytes = sizeof prebufferDurationBytes } });
            HAPAssert(!err);

            // Event Trigger Type.
            uint8_t eventTriggerTypeBytes[] = { HAPExpandLittleUInt64((1ULL << 0) | (0ULL << 1)) };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) {
                            .type = 2,
                            .value = { .bytes = eventTriggerTypeBytes, .numBytes = sizeof eventTriggerTypeBytes } });
            HAPAssert(!err);

            // Selected Media Container Configuration.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Media Container Type.
                uint8_t mediaContainerTypeBytes[] = { 0 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 1,
                                          .value = { .bytes = mediaContainerTypeBytes,
                                                     .numBytes = sizeof mediaContainerTypeBytes } });
                HAPAssert(!err);

                // Media Container Parameters.
                {
                    HAPTLVWriter sub3Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                    }

                    // Fragment Duration.
                    uint8_t fragmentDurationBytes[] = { HAPExpandLittleUInt32(1500) };
                    err = HAPTLVWriterAppend(
                            &sub3Writer,
                            &(const HAPTLV) { .type = 1,
                                              .value = { .bytes = fragmentDurationBytes,
                                                         .numBytes = sizeof fragmentDurationBytes } });
                    HAPAssert(!err);

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                    HAPAssert(!err);
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter, &(const HAPTLV) { .type = 3, .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter, &(const HAPTLV) { .type = 1, .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }
        // Selected Video Parameters.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Selected Video Codec Type.
            uint8_t selectedVideoCodecTypeBytes[] = { 0 };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) { .type = 1,
                                      .value = { .bytes = selectedVideoCodecTypeBytes,
                                                 .numBytes = sizeof selectedVideoCodecTypeBytes } });
            HAPAssert(!err);

            // Selected Video Codec Parameters.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // ProfileID.
                uint8_t profileIDBytes[] = { 1 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 1,
                                          .value = { .bytes = profileIDBytes, .numBytes = sizeof profileIDBytes } });
                HAPAssert(!err);

                // Level.
                uint8_t levelBytes[] = { 0 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 2, .value = { .bytes = levelBytes, .numBytes = sizeof levelBytes } });
                HAPAssert(!err);

                // Bit Rate.
                uint8_t bitRateBytes[] = { HAPExpandLittleUInt32(500) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 3,
                                          .value = { .bytes = bitRateBytes, .numBytes = sizeof bitRateBytes } });
                HAPAssert(!err);

                // I-Frame Interval.
                uint8_t iFrameIntervalBytes[] = { HAPExpandLittleUInt32(5000) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = 4,
                                .value = { .bytes = iFrameIntervalBytes, .numBytes = sizeof iFrameIntervalBytes } });
                HAPAssert(!err);

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter, &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Selected Video Attributes.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Image Width.
                uint8_t imageWidthBytes[] = { HAPExpandLittleUInt16(1280) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 1,
                                          .value = { .bytes = imageWidthBytes, .numBytes = sizeof imageWidthBytes } });
                HAPAssert(!err);

                // Image Height.
                uint8_t imageHeightBytes[] = { HAPExpandLittleUInt16(720) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = 2,
                                .value = { .bytes = imageHeightBytes, .numBytes = sizeof imageHeightBytes } });
                HAPAssert(!err);

                // Frame rate.
                uint8_t frameRateBytes[] = { 30 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 3,
                                          .value = { .bytes = frameRateBytes, .numBytes = sizeof frameRateBytes } });
                HAPAssert(!err);

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter, &(const HAPTLV) { .type = 3, .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter, &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        // Selected Audio Parameters.
        {
            HAPTLVWriter subWriter;
            {
                void* bytes;
                size_t maxBytes;
                HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
            }

            // Selected Audio Codec Type.
            uint8_t selectedAudioCodecTypeBytes[] = { 0 };
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) { .type = 1,
                                      .value = { .bytes = selectedAudioCodecTypeBytes,
                                                 .numBytes = sizeof selectedAudioCodecTypeBytes } });
            HAPAssert(!err);

            // Selected Audio Codec Parameters.
            {
                HAPTLVWriter sub2Writer;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                }

                // Audio Channels.
                uint8_t audioChannelsBytes[] = { 1 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = 1,
                                .value = { .bytes = audioChannelsBytes, .numBytes = sizeof audioChannelsBytes } });
                HAPAssert(!err);

                // Bit-rate Mode.
                uint8_t bitRateModeBytes[] = { 0 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) {
                                .type = 2,
                                .value = { .bytes = bitRateModeBytes, .numBytes = sizeof bitRateModeBytes } });
                HAPAssert(!err);

                // Sample Rate.
                uint8_t sampleRateBytes[] = { 2 };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 3,
                                          .value = { .bytes = sampleRateBytes, .numBytes = sizeof sampleRateBytes } });
                HAPAssert(!err);

                // Bit Rate.
                uint8_t bitRateBytes[] = { HAPExpandLittleUInt32(28) };
                err = HAPTLVWriterAppend(
                        &sub2Writer,
                        &(const HAPTLV) { .type = 4,
                                          .value = { .bytes = bitRateBytes, .numBytes = sizeof bitRateBytes } });
                HAPAssert(!err);

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &subWriter, &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                HAPAssert(!err);
            }

            // Finalize.
            void* bytes;
            size_t numBytes;
            HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
            err = HAPTLVWriterAppend(
                    &requestWriter, &(const HAPTLV) { .type = 3, .value = { .bytes = bytes, .numBytes = numBytes } });
            HAPAssert(!err);
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, bytes, numBytes);
        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &session,
            .characteristic = &cameraEventRecordingManagementSelectedCameraRecordingConfigurationCharacteristic,
            .service = &cameraEventRecordingManagementService,
            .accessory = &accessory,
            .remote = false,
            .authorizationData = { .bytes = NULL, .numBytes = 0 }
        };
        err = request.characteristic->callbacks.handleWrite(&accessoryServer, &request, &requestReader, NULL);
        HAPAssert(!err);
    }
    HAPLog(&kHAPLog_Default, "Verifying updated Selected Camera Recording Configuration.");
    ReadSelectedCameraRecordingConfiguration(
            &accessoryServer, &session, &supportedCameraRecordingConfiguration, &found, &configuration, NULL);
    HAPAssert(found);

    HAPLogBuffer(&kHAPLog_Default, &configuration, sizeof configuration, "config");
    HAPLogBuffer(&kHAPLog_Default, &newConfiguration, sizeof newConfiguration, "newConfig");

    HAPAssert(HAPRawBufferAreEqual(&configuration, &newConfiguration, sizeof configuration));

    // Stop accessory server.
    HAPLog(&kHAPLog_Default, "Stopping accessory server.");
    HAPAccessoryServerForceStop(&accessoryServer);
    HAPPlatformClockAdvance(0);
    HAPPlatformClockAdvance(0);
    HAPAssert(HAPAccessoryServerGetState(&accessoryServer) == kHAPAccessoryServerState_Idle);
}

#else

int main() {
    HAPLog(&kHAPLog_Default, "This test is not enabled. Please enable the Camera feature to run this test.");
    return 0;
}

#endif
