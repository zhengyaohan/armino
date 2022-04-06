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

#include "HAPPlatformCamera+Configuration.h"
#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformCameraRecorder+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#define kMinVideoBitrate (100) // kbit/s
#define kMinAudioBitrate (1)   // kbit/s

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

//----------------------------------------------------------------------------------------------------------------------

#define kCamera_BaseSize                  ((size_t) 9)
#define kCamera_RecorderSize              ((size_t) 11)
#define kCamera_SelectedConfigurationSize ((size_t) 42)

#define kCamera_FullRecorderSize (kCamera_RecorderSize + kCamera_SelectedConfigurationSize)
//----------------------------------------------------------------------------------------------------------------------

// See HAPPlatformKeyValueStore+SDKDomains.h for storage format.
static const struct {
    size_t version;
    size_t cameraIdentifier;
} kCamera_BaseOffsets = {
    .version = 0,
    .cameraIdentifier = 1,
};
HAP_STATIC_ASSERT(kCamera_BaseSize == 9, kCamera_BaseSize);

typedef struct {
    HAPPlatformCameraRef camera;
    HAPPlatformKeyValueStoreDomain* domain;
    HAPPlatformKeyValueStoreKey* key;
    void* bytes;
    size_t maxBytes;
    size_t* numBytes;
    bool found;
    HAPError err;
} GetConfigurationContext;

HAP_RESULT_USE_CHECK
static HAPError GetConfigurationCallback(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue) {
    HAPPrecondition(context);
    GetConfigurationContext* arguments = context;
    HAPPrecondition(arguments->camera);
    HAPPrecondition(arguments->domain);
    HAPPrecondition(arguments->key);
    HAPPrecondition(arguments->bytes);
    HAPPrecondition(arguments->numBytes);
    HAPPrecondition(!arguments->found);
    HAPPrecondition(!arguments->err);
    HAPPlatformCameraRef camera = arguments->camera;
    uint8_t* bytes = arguments->bytes;
    HAPPrecondition(keyValueStore);
    HAPPrecondition(shouldContinue);
    HAPPrecondition(*shouldContinue);

    HAPError err;

    *arguments->domain = domain;
    *arguments->key = key;

    bool found;
    err = HAPPlatformKeyValueStoreGet(
            keyValueStore,
            *arguments->domain,
            *arguments->key,
            arguments->bytes,
            arguments->maxBytes,
            arguments->numBytes,
            &found);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(
                &logObject, "Failed to load IP Camera configuration %02X.%02X.", *arguments->domain, *arguments->key);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }
    HAPAssert(found);
    HAPAssert(*arguments->numBytes <= arguments->maxBytes);

    if (*arguments->numBytes < kCamera_BaseSize) {
        HAPLogError(
                &logObject,
                "Invalid IP Camera configuration %02X.%02X size %zu.",
                *arguments->domain,
                *arguments->key,
                *arguments->numBytes);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }

    // Check version.
    uint8_t version = HAPReadUInt8(&bytes[kCamera_BaseOffsets.version]);
    if (version) {
        HAPLogError(
                &logObject,
                "Unsupported IP Camera configuration %02X.%02X version %u.",
                *arguments->domain,
                *arguments->key,
                version);
        arguments->err = err;
        *shouldContinue = false;
        return kHAPError_None;
    }

    // Check camera identifier.
    uint64_t cameraIdentifier = HAPReadLittleUInt64(&bytes[kCamera_BaseOffsets.cameraIdentifier]);
    if (cameraIdentifier != camera->identifier) {
        return kHAPError_None;
    }
    arguments->found = true;

    *shouldContinue = false;
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

// See HAPPlatformKeyValueStore+SDKDomains.h for storage format.
static const struct {
    size_t version;
    size_t cameraIdentifier;
    size_t isRecordingActive;
    size_t audioEnabled;
    struct {
        size_t prebufferDuration;
        size_t eventTriggerTypes;
        size_t containerType;
        struct {
            size_t fragmentDuration;
        } fragmentedMP4;
    } recording;
    struct {
        size_t codecType;
        struct {
            struct {
                size_t profile;
                size_t level;
                size_t packetizationMode;
                size_t bitRate;
                size_t iFrameInterval;
            } h264;
        } codecParameters;
        struct {
            size_t width;
            size_t height;
            size_t maxFrameRate;
        } attributes;
    } video;
    struct {
        size_t codecType;
        struct {
            size_t numberOfChannels;
            size_t bitRateMode;
            size_t sampleRate;
            size_t bitRate;
        } codecParameters;
    } audio;
} kCamera_RecorderConfigurationOffsets = {
    .version = 0,
    .cameraIdentifier = 1,
    .isRecordingActive = 9,
    .audioEnabled = 10,
    .recording = { .prebufferDuration = 11,
                   .eventTriggerTypes = 15,
                   .containerType = 23,
                   .fragmentedMP4 = { .fragmentDuration = 24 } },
    .video = { .codecType = 28,
               .codecParameters = { .h264 = { .profile = 29,
                                              .level = 30,
                                              .packetizationMode = 31,
                                              .bitRate = 32,
                                              .iFrameInterval = 36 } },
               .attributes = { .width = 40, .height = 42, .maxFrameRate = 44 } },
    .audio = { .codecType = 45,
               .codecParameters = { .numberOfChannels = 46, .bitRateMode = 47, .sampleRate = 48, .bitRate = 49 } },
};
HAP_STATIC_ASSERT(kCamera_FullRecorderSize == 53, kCamera_FullRecorderSize);

HAP_RESULT_USE_CHECK
static HAPError GetRecorderConfiguration(
        HAPPlatformCameraRef camera,
        HAPPlatformKeyValueStoreDomain* domain,
        HAPPlatformKeyValueStoreKey* key,
        void* bytes_,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(camera);
    HAPPrecondition(domain);
    HAPPrecondition(key);
    HAPPrecondition(bytes_);
    uint8_t* bytes = bytes_;
    HAPPrecondition(maxBytes > kCamera_RecorderSize);
    HAPPrecondition(numBytes);

    HAPError err;

    GetConfigurationContext enumerateContext;
    HAPRawBufferZero(&enumerateContext, sizeof enumerateContext);
    enumerateContext.camera = camera;
    enumerateContext.domain = domain;
    enumerateContext.key = key;
    enumerateContext.bytes = bytes_;
    enumerateContext.maxBytes = maxBytes;
    enumerateContext.numBytes = numBytes;
    if (camera->keyValueStore) {
        err = HAPPlatformKeyValueStoreEnumerate(
                HAPNonnull(camera->keyValueStore),
                kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration,
                GetConfigurationCallback,
                &enumerateContext);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate recorder configuration.");
            return err;
        }
        err = enumerateContext.err;
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate recorder configuration.");
            return err;
        }
    }

    // Initialize new configuration if not found.
    if (!enumerateContext.found) {
        // Find unused key.
        *domain = kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration;
        *key = 0;
        bool foundFreeKey = true;
        if (camera->keyValueStore) {
            do {
                bool found;
                err = HAPPlatformKeyValueStoreGet(
                        HAPNonnull(camera->keyValueStore), *domain, *key, NULL, 0, NULL, &found);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLogError(&logObject, "Failed to load recorder configuration %02X.%02X.", *domain, *key);
                    return err;
                }
                foundFreeKey = !found;
            } while (!foundFreeKey && (*key)++ != (HAPPlatformKeyValueStoreKey) -1);
        }
        if (!foundFreeKey) {
            HAPLogError(&logObject, "Not enough memory to store paired recorder configuration.");
            return kHAPError_Unknown;
        }

        HAPRawBufferZero(bytes_, maxBytes);
        bytes[kCamera_RecorderConfigurationOffsets.version] = 0;
        HAPWriteLittleUInt64(&bytes[kCamera_RecorderConfigurationOffsets.cameraIdentifier], camera->identifier);

        // See HomeKit Accessory Protocol Specification R17
        // Section 10.45 Camera Event Recording Management
        // Default: Inactive.
        bytes[kCamera_RecorderConfigurationOffsets.isRecordingActive] = 0;
        bytes[kCamera_RecorderConfigurationOffsets.audioEnabled] = 0;

        *numBytes = kCamera_RecorderSize;
    }

    if (*numBytes != kCamera_RecorderSize && *numBytes != kCamera_FullRecorderSize) {
        HAPLogError(&logObject, "Invalid recorder configuration %02X.%02X size %zu.", *domain, *key, *numBytes);
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingActive(HAPPlatformCameraRef camera, bool* isRecordingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(isRecordingActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isRecordingActive = bytes[kCamera_RecorderConfigurationOffsets.isRecordingActive];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformCameraIsRecordingEnabled(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    HAPError err;

    bool isRecordingActive;
    err = HAPPlatformCameraIsRecordingActive(camera, &isRecordingActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsRecordingActive failed.");
        return false;
    }

    bool isRecordingConfigured;
    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "GetRecorderConfiguration failed.");
        return false;
    }
    isRecordingConfigured = numBytes != kCamera_RecorderSize;

    bool isHomeKitCameraActive;
    err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsHomeKitCameraActive failed.");
        return false;
    }

    // If camera is turned off with a physical button, it will override both HK and third party operating modes.
    // See HomeKit Accessory Protocol Specification R17
    // Section 11.136 Manually Disabled
    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return false;
    }

    return isRecordingActive & isRecordingConfigured & isHomeKitCameraActive & !isManuallyDisabled;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingActive(HAPPlatformCameraRef camera, bool recordingActive) {
    HAPPrecondition(camera);

    HAPError err;

    if (!camera->keyValueStore && recordingActive) {
        HAPLogError(
                &logObject,
                "No key-value store supplied in Camera initialization options. Recording feature cannot be enabled.");
        return kHAPError_Unknown;
    }

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_RecorderConfigurationOffsets.isRecordingActive] == recordingActive) {
        return kHAPError_None;
    }

    bytes[kCamera_RecorderConfigurationOffsets.isRecordingActive] = recordingActive;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, true);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        bool* found,
        HAPCameraRecordingConfiguration* _Nullable configuration) {
    HAPPrecondition(camera);
    HAPPrecondition(found);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *found = numBytes != kCamera_RecorderSize;
    if (!*found) {
        HAPLog(&logObject, "No recording configuration selected.");
        return kHAPError_None;
    }

    if (!configuration) {
        return kHAPError_None;
    }

    HAPRawBufferZero(HAPNonnull(configuration), sizeof *configuration);

    // Recording.
    configuration->recording.prebufferDuration =
            HAPReadLittleUInt32(&bytes[kCamera_RecorderConfigurationOffsets.recording.prebufferDuration]) *
            HAPMillisecond;
    configuration->recording.eventTriggerTypes =
            (uint8_t) HAPReadLittleUInt64(&bytes[kCamera_RecorderConfigurationOffsets.recording.eventTriggerTypes]);
    configuration->recording.containerConfiguration.containerType =
            HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.recording.containerType]);
    switch (configuration->recording.containerConfiguration.containerType) {
        case kHAPMediaContainerType_FragmentedMP4: {
            configuration->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration =
                    HAPReadLittleUInt32(
                            &bytes[kCamera_RecorderConfigurationOffsets.recording.fragmentedMP4.fragmentDuration]);
            break;
        }
    }

    // Video.
    configuration->video.codecType = HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.video.codecType]);
    switch (configuration->video.codecType) {
        case kHAPVideoCodecType_H264: {
            configuration->video.codecParameters.h264.profile = (uint8_t)(
                    1U << (HAPReadUInt8(
                                   &bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.profile]) -
                           1));
            configuration->video.codecParameters.h264.level = (uint8_t)(
                    1U << (HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.level]) -
                           1));
            configuration->video.codecParameters.h264.packetizationMode = (uint8_t)(
                    1U << (HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264
                                                       .packetizationMode]) -
                           1));
            configuration->video.codecParameters.h264.bitRate = HAPReadLittleUInt32(
                    &bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.bitRate]);
            configuration->video.codecParameters.h264.iFrameInterval = HAPReadLittleUInt32(
                    &bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.iFrameInterval]);
            break;
        }
    }
    configuration->video.attributes.width =
            HAPReadLittleUInt16(&bytes[kCamera_RecorderConfigurationOffsets.video.attributes.width]);
    configuration->video.attributes.height =
            HAPReadLittleUInt16(&bytes[kCamera_RecorderConfigurationOffsets.video.attributes.height]);
    configuration->video.attributes.maxFrameRate =
            HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.video.attributes.maxFrameRate]);

    // Audio.
    configuration->audio.codecType = HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.audio.codecType]);
    configuration->audio.codecParameters.numberOfChannels =
            HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.numberOfChannels]);
    configuration->audio.codecParameters.bitRateMode = (uint8_t)(
            1U << (HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.bitRateMode]) - 1));
    configuration->audio.codecParameters.sampleRate = (uint8_t)(
            1U << (HAPReadUInt8(&bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.sampleRate]) - 1));
    configuration->audio.codecParameters.bitRate =
            HAPReadLittleUInt32(&bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.bitRate]);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static uint8_t BitIndex(uint8_t value) {
    uint8_t bitIndex = 0;
    for (uint8_t v = value >> 1; v; v >>= 1) {
        bitIndex++;
    }
    HAPAssert(1U << bitIndex == value);
    return bitIndex;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        const HAPCameraRecordingConfiguration* configuration) {
    HAPPrecondition(camera);
    HAPPrecondition(configuration);

    HAPError err;

    if (configuration->video.codecParameters.h264.bitRate < kMinVideoBitrate) {
        HAPLogError(
                &logObject,
                "Unsupported video bitrate: %ukb/s",
                (unsigned int) configuration->video.codecParameters.h264.bitRate);
        return kHAPError_InvalidData;
    }
    if (configuration->audio.codecParameters.bitRate < kMinAudioBitrate) {
        HAPLogError(
                &logObject,
                "Unsupported audio bitrate: %ukb/s",
                (unsigned int) configuration->audio.codecParameters.bitRate);
        return kHAPError_InvalidData;
    }
    if (configuration->video.codecParameters.h264.iFrameInterval * configuration->video.attributes.maxFrameRate <
        1000) {
        // iFrameInterval < 1 / maxFrameRate;
        HAPLogError(
                &logObject,
                "Unsupported i-frame interval: %lums",
                (unsigned long) configuration->video.codecParameters.h264.iFrameInterval);
        return kHAPError_InvalidData;
    }
    if (configuration->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration *
                configuration->video.attributes.maxFrameRate <
        1000) {
        // fragmentDuration < 1 / maxFrameRate;
        HAPLogError(
                &logObject,
                "Unsupported fragment duration: %lums",
                (unsigned long) configuration->recording.containerConfiguration.containerParameters.fragmentedMP4
                        .fragmentDuration);
        return kHAPError_InvalidData;
    }
    if (configuration->recording.prebufferDuration != 0 &&
        configuration->recording.prebufferDuration <
                configuration->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration) {
        HAPLogError(
                &logObject,
                "Unsupported prebuffer duration: %lums",
                (unsigned long) configuration->recording.prebufferDuration);
        return kHAPError_InvalidData;
    }

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }
    HAPRawBufferZero(&bytes[kCamera_RecorderSize], kCamera_SelectedConfigurationSize);
    numBytes = kCamera_FullRecorderSize;

    // Recording.
    HAPWriteLittleUInt32(
            &bytes[kCamera_RecorderConfigurationOffsets.recording.prebufferDuration],
            configuration->recording.prebufferDuration / HAPMillisecond);
    HAPWriteLittleUInt64(
            &bytes[kCamera_RecorderConfigurationOffsets.recording.eventTriggerTypes],
            (uint64_t) configuration->recording.eventTriggerTypes);
    bytes[kCamera_RecorderConfigurationOffsets.recording.containerType] =
            configuration->recording.containerConfiguration.containerType;
    switch (configuration->recording.containerConfiguration.containerType) {
        case kHAPMediaContainerType_FragmentedMP4: {
            HAPWriteLittleUInt32(
                    &bytes[kCamera_RecorderConfigurationOffsets.recording.fragmentedMP4.fragmentDuration],
                    configuration->recording.containerConfiguration.containerParameters.fragmentedMP4.fragmentDuration);
            break;
        }
    }

    // Video.
    bytes[kCamera_RecorderConfigurationOffsets.video.codecType] = configuration->video.codecType;
    switch (configuration->video.codecType) {
        case kHAPVideoCodecType_H264: {
            bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.profile] =
                    (uint8_t)(1 + BitIndex(configuration->video.codecParameters.h264.profile));
            bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.level] =
                    (uint8_t)(1 + BitIndex(configuration->video.codecParameters.h264.level));
            bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.packetizationMode] =
                    (uint8_t)(1 + BitIndex(configuration->video.codecParameters.h264.packetizationMode));
            HAPWriteLittleUInt32(
                    &bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.bitRate],
                    configuration->video.codecParameters.h264.bitRate);
            HAPWriteLittleUInt32(
                    &bytes[kCamera_RecorderConfigurationOffsets.video.codecParameters.h264.iFrameInterval],
                    configuration->video.codecParameters.h264.iFrameInterval);
            break;
        }
    }
    HAPWriteLittleUInt16(
            &bytes[kCamera_RecorderConfigurationOffsets.video.attributes.width], configuration->video.attributes.width);
    HAPWriteLittleUInt16(
            &bytes[kCamera_RecorderConfigurationOffsets.video.attributes.height],
            configuration->video.attributes.height);
    bytes[kCamera_RecorderConfigurationOffsets.video.attributes.maxFrameRate] =
            configuration->video.attributes.maxFrameRate;

    // Audio.
    bytes[kCamera_RecorderConfigurationOffsets.audio.codecType] = configuration->audio.codecType;
    bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.numberOfChannels] =
            configuration->audio.codecParameters.numberOfChannels;
    bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.bitRateMode] =
            (uint8_t)(1 + BitIndex(configuration->audio.codecParameters.bitRateMode));
    bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.sampleRate] =
            (uint8_t)(1 + BitIndex(configuration->audio.codecParameters.sampleRate));
    HAPWriteLittleUInt32(
            &bytes[kCamera_RecorderConfigurationOffsets.audio.codecParameters.bitRate],
            configuration->audio.codecParameters.bitRate);

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }

    if (HAPPlatformCameraIsRecordingEnabled(camera)) {
        err = HAPPlatformCameraConfigureRecording(camera);
        if (err) {
            HAPLogError(&logObject, "Configuring camera recording stream failed.");
        }
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInvalidateRecordingConfiguration(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (numBytes == kCamera_RecorderSize) {
        return kHAPError_None;
    }

    numBytes = kCamera_RecorderSize;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingAudioEnabled(HAPPlatformCameraRef camera, bool* isAudioEnabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isAudioEnabled);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isAudioEnabled = bytes[kCamera_RecorderConfigurationOffsets.audioEnabled];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingAudioEnabled(HAPPlatformCameraRef camera, bool audioEnabled) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_FullRecorderSize + 1];
    size_t numBytes;
    err = GetRecorderConfiguration(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_RecorderConfigurationOffsets.audioEnabled] == audioEnabled) {
        return kHAPError_None;
    }

    bytes[kCamera_RecorderConfigurationOffsets.audioEnabled] = audioEnabled;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save IP Camera configuration.");
        return err;
    }

    if (HAPPlatformCameraIsRecordingEnabled(camera)) {
        err = HAPPlatformCameraConfigureRecording(camera);
        if (err) {
            HAPLogError(&logObject, "Configuring camera recording stream failed.");
        }
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void HAPPlatformCameraReloadRecorderConfiguration(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    // Assert correct cleanup of streaming sessions.
    for (size_t streamIndex = 0; streamIndex < camera->streamingSessionStorage.numSessions; streamIndex++) {
        HAPAssert(camera->streamingSessionStorage.sessions[streamIndex].status != kHAPCameraStreamingStatus_InUse);
    }

    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, true);
    }
}

#endif
