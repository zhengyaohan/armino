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

#include "HAPPlatformCameraOperatingMode.h"
#include "HAPPlatformCamera+Init.h"
#include "HAPPlatformCameraRecorder+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera" };

//----------------------------------------------------------------------------------------------------------------------

#define kCamera_BaseSize                ((size_t) 9)
#define kCamera_OperatingModeSize       ((size_t) 27)
#define kCamera_PairedOperatingModeSize ((size_t) 13)

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
    HAPPlatformCameraRef _Nonnull camera;
    HAPPlatformKeyValueStoreDomain* _Nullable domain;
    HAPPlatformKeyValueStoreKey* _Nullable key;
    void* _Nullable bytes;
    size_t maxBytes;
    size_t* _Nullable numBytes;
    bool found;
    HAPError err;
} GetConfigurationContext;

HAP_RESULT_USE_CHECK
static HAPError GetConfigurationCallback(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef _Nullable keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* _Nonnull shouldContinue) {
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
    size_t streamingActive;
    size_t thirdPartyActive;
    size_t manuallyDisabled;
} kCamera_OperatingModeOffsets = {
    .version = 0,
    .cameraIdentifier = 1,
    .streamingActive = 9,
    .thirdPartyActive = 25,
    .manuallyDisabled = 26,
};
HAP_STATIC_ASSERT(kCamera_OperatingModeSize == 27, kCamera_OperatingModeSize);

HAP_RESULT_USE_CHECK
static HAPError GetOperatingMode(
        HAPPlatformCameraRef _Nonnull camera,
        HAPPlatformKeyValueStoreDomain* _Nonnull domain,
        HAPPlatformKeyValueStoreKey* _Nonnull key,
        void* _Nonnull bytes_,
        size_t maxBytes,
        size_t* _Nonnull numBytes) {
    HAPPrecondition(camera);
    HAPPrecondition(domain);
    HAPPrecondition(key);
    HAPPrecondition(bytes_);
    uint8_t* bytes = bytes_;
    HAPPrecondition(maxBytes > kCamera_OperatingModeSize);
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
                kSDKKeyValueStoreDomain_IPCameraOperatingMode,
                GetConfigurationCallback,
                &enumerateContext);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate IP Camera Operating Mode.");
            return err;
        }
        err = enumerateContext.err;
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate IP Camera Operating Mode.");
            return err;
        }
    }

    // Initialize new configuration if not found.
    if (!enumerateContext.found) {
        // Find unused key.
        *domain = kSDKKeyValueStoreDomain_IPCameraOperatingMode;
        *key = 0;
        bool foundFreeKey = true;
        if (camera->keyValueStore) {
            do {
                bool found;
                err = HAPPlatformKeyValueStoreGet(
                        HAPNonnull(camera->keyValueStore), *domain, *key, NULL, 0, NULL, &found);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLogError(&logObject, "Failed to load IP Camera Operating Mode %02X.%02X.", *domain, *key);
                    return err;
                }
                foundFreeKey = !found;
            } while (!foundFreeKey && (*key)++ != (HAPPlatformKeyValueStoreKey) -1);
        }
        if (!foundFreeKey) {
            HAPLogError(&logObject, "Not enough memory to store IP Camera Operating Mode.");
            return kHAPError_Unknown;
        }

        HAPRawBufferZero(bytes_, maxBytes);
        bytes[kCamera_OperatingModeOffsets.version] = 0;
        HAPWriteLittleUInt64(&bytes[kCamera_OperatingModeOffsets.cameraIdentifier], camera->identifier);

        // See HomeKit Accessory Protocol Specification R17
        // Section 10.26 Camera RTP Stream Management
        // Default: Active.
        for (size_t byteIndex = 0; byteIndex < 128 / CHAR_BIT; byteIndex++) {
            size_t offset = kCamera_OperatingModeOffsets.streamingActive + byteIndex;
            HAPAssert(offset < kCamera_OperatingModeSize);
            bytes[offset] = 0xFF;
        }

        // See HomeKit Accessory Protocol Specification R17
        // Section 10.46 Camera Operating Mode
        // Default: Active: On, Manually Disabled: Off.
        bytes[kCamera_OperatingModeOffsets.thirdPartyActive] = 1;
        bytes[kCamera_OperatingModeOffsets.manuallyDisabled] = 0;

        *numBytes = kCamera_OperatingModeSize;
    }

    if (*numBytes != kCamera_OperatingModeSize) {
        HAPLogError(&logObject, "Invalid IP Camera configuration %02X.%02X size %zu.", *domain, *key, *numBytes);
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

// See HAPPlatformKeyValueStore+SDKDomains.h for storage format.
static const struct {
    size_t version;
    size_t cameraIdentifier;
    size_t eventSnapshotsActive;
    size_t periodicSnapshotsActive;
    size_t homeKitActive;
    size_t indicatorEnabled;
} kCamera_PairedOperatingModeOffsets = {
    .version = 0,
    .cameraIdentifier = 1,
    .eventSnapshotsActive = 9,
    .periodicSnapshotsActive = 10,
    .homeKitActive = 11,
    .indicatorEnabled = 12,
};
HAP_STATIC_ASSERT(kCamera_PairedOperatingModeSize == 13, kCamera_PairedOperatingModeSize);

HAP_RESULT_USE_CHECK
static HAPError GetPairedOperatingMode(
        HAPPlatformCameraRef _Nonnull camera,
        HAPPlatformKeyValueStoreDomain* _Nullable domain,
        HAPPlatformKeyValueStoreKey* _Nullable key,
        void* _Nullable bytes_,
        size_t maxBytes,
        size_t* _Nullable numBytes) {
    HAPPrecondition(camera);
    HAPPrecondition(domain);
    HAPPrecondition(key);
    HAPPrecondition(bytes_);
    uint8_t* bytes = bytes_;
    HAPPrecondition(maxBytes > kCamera_PairedOperatingModeSize);
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
                kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode,
                GetConfigurationCallback,
                &enumerateContext);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate IP Camera paired Operating Mode.");
            return err;
        }
        err = enumerateContext.err;
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "Failed to enumerate IP Camera paired Operating Mode.");
            return err;
        }
    }

    // Initialize new configuration if not found.
    if (!enumerateContext.found) {
        // Find unused key.
        *domain = kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode;
        *key = 0;
        bool foundFreeKey = true;
        if (camera->keyValueStore) {
            do {
                bool found;
                err = HAPPlatformKeyValueStoreGet(
                        HAPNonnull(camera->keyValueStore), *domain, *key, NULL, 0, NULL, &found);
                if (err) {
                    HAPAssert(err == kHAPError_Unknown);
                    HAPLogError(&logObject, "Failed to load IP Camera paired Operating Mode %02X.%02X.", *domain, *key);
                    return err;
                }
                foundFreeKey = !found;
            } while (!foundFreeKey && (*key)++ != (HAPPlatformKeyValueStoreKey) -1);
        }
        if (!foundFreeKey) {
            HAPLogError(&logObject, "Not enough memory to store paired IP Camera paired Operating Mode.");
            return kHAPError_Unknown;
        }

        HAPRawBufferZero(bytes_, maxBytes);
        bytes[kCamera_PairedOperatingModeOffsets.version] = 0;
        HAPWriteLittleUInt64(&bytes[kCamera_PairedOperatingModeOffsets.cameraIdentifier], camera->identifier);

        // See HomeKit Accessory Protocol Specification R17
        // Section 10.46 Camera Operating Mode
        // Default: Active: On, Indicator Enabled: On.
        bytes[kCamera_PairedOperatingModeOffsets.eventSnapshotsActive] = 1;
        bytes[kCamera_PairedOperatingModeOffsets.periodicSnapshotsActive] = 1;
        bytes[kCamera_PairedOperatingModeOffsets.homeKitActive] = 1;
        bytes[kCamera_PairedOperatingModeOffsets.indicatorEnabled] = 1;

        *numBytes = kCamera_PairedOperatingModeSize;
    }

    if (*numBytes != kCamera_PairedOperatingModeSize) {
        HAPLogError(
                &logObject, "Invalid IP Camera paired Operating Mode %02X.%02X size %zu.", *domain, *key, *numBytes);
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsStreamingActive(
        HAPPlatformCameraRef _Nonnull camera,
        size_t streamIndex,
        bool* _Nonnull isStreamingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < 128);
    HAPPrecondition(isStreamingActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    size_t byteIndex = streamIndex / CHAR_BIT;
    size_t bitIndex = streamIndex % CHAR_BIT;
    size_t offset = kCamera_OperatingModeOffsets.streamingActive + byteIndex;
    HAPAssert(offset < kCamera_OperatingModeSize);
    uint8_t mask = (uint8_t)(1U << bitIndex);
    *isStreamingActive = (bytes[offset] & mask) != 0;

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetStreamingActive(
        HAPPlatformCamera* _Nonnull camera,
        size_t streamIndex,
        bool streamingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(streamIndex < 128);

    HAPError err;

    if (!camera->keyValueStore && !streamingActive) {
        HAPLogError(
                &logObject,
                "No key-value store supplied in Camera initialization options. Streaming feature cannot be disabled.");
        return kHAPError_Unknown;
    }

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    size_t byteIndex = streamIndex / CHAR_BIT;
    size_t bitIndex = streamIndex % CHAR_BIT;
    size_t offset = kCamera_OperatingModeOffsets.streamingActive + byteIndex;
    HAPAssert(offset < kCamera_OperatingModeSize);
    uint8_t mask = (uint8_t)(1U << bitIndex);
    bool isStreamingActive = (bytes[offset] & mask) != 0;

    if (streamingActive == isStreamingActive) {
        return kHAPError_None;
    }

    if (!streamingActive) {
        bytes[offset] &= ~mask;
    } else {
        bytes[offset] |= mask;
    }

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save IP Camera configuration.");
        return err;
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, false);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformCameraIsStreamingEnabled(HAPPlatformCameraRef _Nonnull camera) {
    HAPPrecondition(camera);

    bool isHomeKitCameraActive;
    HAPError err = HAPPlatformCameraIsHomeKitCameraActive(camera, &isHomeKitCameraActive);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return false;
    }
    if (!isHomeKitCameraActive) {
        return false;
    }

    bool isManuallyDisabled;
    err = HAPPlatformCameraIsManuallyDisabled(camera, &isManuallyDisabled);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return false;
    }
    if (isManuallyDisabled) {
        return false;
    }

    for (size_t streamIndex = 0; streamIndex < camera->streamingSessionStorage.numSessions; streamIndex++) {
        bool isStreamingActive;
        err = HAPPlatformCameraIsStreamingActive(camera, streamIndex, &isStreamingActive);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return false;
        }
        if (isStreamingActive) {
            return true;
        }
    }
    return false;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformCameraIsStreamingInUse(HAPPlatformCameraRef _Nonnull camera) {
    for (size_t streamIndex = 0; streamIndex < camera->streamingSessionStorage.numSessions; streamIndex++) {
        HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(camera, streamIndex);
        if (status == kHAPCameraStreamingStatus_InUse) {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void HAPPlatformCameraDisableRTPStreams(HAPPlatformCameraRef _Nonnull camera) {
    HAPPrecondition(camera);

    for (size_t streamIndex = 0; streamIndex < camera->streamingSessionStorage.numSessions; streamIndex++) {
        HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(camera, streamIndex);
        if (status == kHAPCameraStreamingStatus_InUse) {
            HAPPlatformCameraEndStreamingSession(camera, streamIndex);
            HAPError err =
                    HAPPlatformCameraTrySetStreamStatus(camera, streamIndex, kHAPCameraStreamingStatus_Available);
            HAPAssert(err == kHAPError_None);
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraAreEventSnapshotsActive(
        HAPPlatformCameraRef _Nonnull camera,
        bool* _Nonnull areSnapshotsActive) {
    HAPPrecondition(camera);
    HAPPrecondition(areSnapshotsActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *areSnapshotsActive = bytes[kCamera_PairedOperatingModeOffsets.eventSnapshotsActive];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetEventSnapshotsActive(HAPPlatformCameraRef _Nonnull camera, bool snapshotsActive) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_PairedOperatingModeOffsets.eventSnapshotsActive] == snapshotsActive) {
        return kHAPError_None;
    }

    bytes[kCamera_PairedOperatingModeOffsets.eventSnapshotsActive] = snapshotsActive;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraArePeriodicSnapshotsActive(
        HAPPlatformCameraRef _Nonnull camera,
        bool* _Nonnull areSnapshotsActive) {
    HAPPrecondition(camera);
    HAPPrecondition(areSnapshotsActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *areSnapshotsActive = bytes[kCamera_PairedOperatingModeOffsets.periodicSnapshotsActive];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetPeriodicSnapshotsActive(HAPPlatformCameraRef _Nonnull camera, bool snapshotsActive) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_PairedOperatingModeOffsets.periodicSnapshotsActive] == snapshotsActive) {
        return kHAPError_None;
    }

    bytes[kCamera_PairedOperatingModeOffsets.periodicSnapshotsActive] = snapshotsActive;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsHomeKitCameraActive(HAPPlatformCameraRef _Nonnull camera, bool* _Nonnull isHomeKitActive) {
    HAPPrecondition(camera);
    HAPPrecondition(isHomeKitActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isHomeKitActive = bytes[kCamera_PairedOperatingModeOffsets.homeKitActive];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetHomeKitCameraActive(HAPPlatformCameraRef _Nonnull camera, bool homeKitActive) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_PairedOperatingModeOffsets.homeKitActive] == homeKitActive) {
        return kHAPError_None;
    }

    bytes[kCamera_PairedOperatingModeOffsets.homeKitActive] = homeKitActive;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save paired IP Camera configuration.");
        return err;
    }

    if (!homeKitActive) {
        HAPPlatformCameraDisableRTPStreams(camera);
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, true);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsThirdPartyCameraActive(
        HAPPlatformCameraRef _Nonnull camera,
        bool* _Nonnull isThirdPartyActive) {
    HAPPrecondition(camera);
    HAPPrecondition(isThirdPartyActive);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isThirdPartyActive = bytes[kCamera_OperatingModeOffsets.thirdPartyActive];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetThirdPartyCameraActive(HAPPlatformCamera* _Nonnull camera, bool thirdPartyActive) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_OperatingModeOffsets.thirdPartyActive] == thirdPartyActive) {
        return kHAPError_None;
    }

    bytes[kCamera_OperatingModeOffsets.thirdPartyActive] = thirdPartyActive;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save IP Camera configuration.");
        return err;
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, false);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsOperatingModeIndicatorEnabled(
        HAPPlatformCameraRef _Nonnull camera,
        bool* _Nonnull isIndicatorEnabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isIndicatorEnabled);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isIndicatorEnabled = bytes[kCamera_PairedOperatingModeOffsets.indicatorEnabled];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError
        HAPPlatformCameraSetOperatingModeIndicatorEnabled(HAPPlatformCameraRef _Nonnull camera, bool indicatorEnabled) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_PairedOperatingModeSize + 1];
    size_t numBytes;
    err = GetPairedOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_PairedOperatingModeOffsets.indicatorEnabled] == indicatorEnabled) {
        return kHAPError_None;
    }

    bytes[kCamera_PairedOperatingModeOffsets.indicatorEnabled] = indicatorEnabled;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save IP Camera configuration.");
        return err;
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, false);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsManuallyDisabled(HAPPlatformCameraRef _Nonnull camera, bool* _Nonnull isManuallyDisabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isManuallyDisabled);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    *isManuallyDisabled = bytes[kCamera_OperatingModeOffsets.manuallyDisabled];

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetManuallyDisabled(HAPPlatformCamera* _Nonnull camera, bool manuallyDisabled) {
    HAPPrecondition(camera);

    HAPError err;

    HAPPlatformKeyValueStoreDomain domain;
    HAPPlatformKeyValueStoreKey key;
    uint8_t bytes[kCamera_OperatingModeSize + 1];
    size_t numBytes;
    err = GetOperatingMode(camera, &domain, &key, bytes, sizeof bytes, &numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    if (bytes[kCamera_OperatingModeOffsets.manuallyDisabled] == manuallyDisabled) {
        return kHAPError_None;
    }

    bytes[kCamera_OperatingModeOffsets.manuallyDisabled] = manuallyDisabled;

    err = HAPPlatformKeyValueStoreSet(HAPNonnull(camera->keyValueStore), domain, key, bytes, numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Failed to save IP Camera configuration.");
        return err;
    }

    if (manuallyDisabled) {
        HAPPlatformCameraDisableRTPStreams(camera);
    }
    if (camera->recorderDelegate.handleOperatingModeChanged) {
        camera->recorderDelegate.handleOperatingModeChanged(camera, true);
    }

    return kHAPError_None;
}

#endif
