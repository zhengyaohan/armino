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

#include "HAPPlatformCamera+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Camera(Mock)" };

void HAPPlatformCameraCreate(HAPPlatformCameraRef camera, const HAPPlatformCameraOptions* options) {
    HAPPrecondition(camera);
    HAPPrecondition(options);

    HAPRawBufferZero(camera, sizeof *camera);

    if (options->recording.isSupported) {
        camera->recording.isSupported = options->recording.isSupported;
    }

    camera->streaming.isActive = true;
    camera->recording.isActive = true;
    camera->recording.areEventSnapshotsActive = true;
    camera->recording.arePeriodicSnapshotsActive = true;
    camera->recording.isHomeKitActive = true;
    camera->recording.isAudioEnabled = true;
}

void HAPPlatformCameraSetDelegate(HAPPlatformCameraRef camera, const HAPPlatformCameraDelegate* _Nullable delegate) {
    HAPPrecondition(camera);

    if (delegate) {
        camera->delegate = *delegate;
    } else {
        HAPRawBufferZero(&camera->delegate, sizeof camera->delegate);
    }
}

HAP_RESULT_USE_CHECK
HAPCameraStreamingStatus HAPPlatformCameraGetStreamStatus(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraTrySetStreamStatus(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        HAPCameraStreamingStatus status) {
    HAPPrecondition(camera);

    (void) streamIndex;
    (void) status;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetStreamingSessionEndpoint(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraIPAddress* controllerAddress,
        HAPPlatformCameraIPAddress* accessoryAddress,
        HAPNetworkPort* videoPort,
        HAPNetworkPort* audioPort) {
    HAPPrecondition(camera);
    HAPPrecondition(controllerAddress);
    HAPPrecondition(accessoryAddress);
    HAPPrecondition(videoPort);
    HAPPrecondition(audioPort);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraStartStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraEndpointParameters* controllerEndpoint,
        const HAPPlatformCameraEndpointParameters* accessoryEndpoint,
        const HAPPlatformCameraStartStreamingSessionConfiguration* streamConfiguration) {
    HAPPrecondition(camera);
    HAPPrecondition(controllerEndpoint);
    HAPPrecondition(accessoryEndpoint);
    HAPPrecondition(streamConfiguration);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSuspendStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraResumeStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraReconfigureStreamingSession(
        HAPPlatformCameraRef camera,
        size_t streamIndex,
        const HAPPlatformCameraReconfigureStreamingSessionConfiguration* streamConfiguration) {
    HAPPrecondition(camera);
    HAPPrecondition(streamConfiguration);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

void HAPPlatformCameraEndStreamingSession(HAPPlatformCameraRef camera, size_t streamIndex) {
    HAPPrecondition(camera);

    (void) streamIndex;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAPError HAPPlatformCameraTakeSnapshot(
        HAPPlatformCameraRef camera,
        uint16_t imageWidth,
        uint16_t imageHeight,
        HAPPlatformCameraSnapshotReader* _Nonnull* _Nonnull snapshotReader) {
    HAPPrecondition(camera);
    HAPPrecondition(snapshotReader);

    (void) imageWidth;
    (void) imageHeight;
    HAPLogError(&logObject, "[NYI] %s.", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsStreamingActive(HAPPlatformCameraRef camera, size_t streamIndex, bool* isStreamingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(!streamIndex);
    HAPPrecondition(isStreamingActive);

    *isStreamingActive = camera->streaming.isActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetStreamingActive(HAPPlatformCameraRef camera, size_t streamIndex, bool streamingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(!streamIndex);

    camera->streaming.isActive = streamingActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingActive(HAPPlatformCameraRef camera, bool* isRecordingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recording.isSupported);
    HAPPrecondition(isRecordingActive);

    *isRecordingActive = camera->recording.isActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingActive(HAPPlatformCameraRef camera, bool recordingActive) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recording.isSupported);

    camera->recording.isActive = recordingActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraGetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        bool* found,
        HAPCameraRecordingConfiguration* _Nullable configuration) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recording.isSupported);
    HAPPrecondition(found);
    HAPPrecondition(configuration);
    *found = false;

    if (!camera->recording.isConfigured) {
        return kHAPError_None;
    }

    *found = true;
    if (configuration) {
        *configuration = camera->recording.configuration;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingConfiguration(
        HAPPlatformCameraRef camera,
        const HAPCameraRecordingConfiguration* configuration) {
    HAPPrecondition(camera);
    HAPPrecondition(camera->recording.isSupported);
    HAPPrecondition(configuration);

    camera->recording.configuration = *configuration;
    camera->recording.isConfigured = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInvalidateRecordingConfiguration(HAPPlatformCameraRef camera) {
    HAPPrecondition(camera);

    camera->recording.isConfigured = false;
    HAPRawBufferZero(&camera->recording.configuration, sizeof camera->recording.configuration);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraAreEventSnapshotsActive(HAPPlatformCameraRef camera, bool* areSnapshotsActive) {
    HAPPrecondition(camera);
    HAPPrecondition(areSnapshotsActive);

    *areSnapshotsActive = camera->recording.areEventSnapshotsActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetEventSnapshotsActive(HAPPlatformCameraRef camera, bool areSnapshotsActive) {
    HAPPrecondition(camera);

    camera->recording.areEventSnapshotsActive = areSnapshotsActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraArePeriodicSnapshotsActive(HAPPlatformCameraRef camera, bool* areSnapshotsActive) {
    HAPPrecondition(camera);
    HAPPrecondition(areSnapshotsActive);

    *areSnapshotsActive = camera->recording.arePeriodicSnapshotsActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetPeriodicSnapshotsActive(HAPPlatformCameraRef camera, bool areSnapshotsActive) {
    HAPPrecondition(camera);

    camera->recording.arePeriodicSnapshotsActive = areSnapshotsActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsHomeKitCameraActive(HAPPlatformCameraRef camera, bool* isHomeKitActive) {
    HAPPrecondition(camera);
    HAPPrecondition(isHomeKitActive);

    *isHomeKitActive = camera->recording.isHomeKitActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetHomeKitCameraActive(HAPPlatformCameraRef camera, bool isHomeKitActive) {
    HAPPrecondition(camera);

    camera->recording.isHomeKitActive = isHomeKitActive;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsThirdPartyCameraActive(HAPPlatformCameraRef camera, bool* isThirdPartyActive) {
    HAPPrecondition(camera);
    HAPPrecondition(isThirdPartyActive);

    *isThirdPartyActive = true;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsOperatingModeIndicatorEnabled(HAPPlatformCameraRef camera, bool* isIndicatorEnabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isIndicatorEnabled);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetOperatingModeIndicatorEnabled(
        HAPPlatformCameraRef camera,
        bool indicatorEnabled HAP_UNUSED) {
    HAPPrecondition(camera);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsRecordingAudioEnabled(HAPPlatformCameraRef camera, bool* isAudioEnabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isAudioEnabled);

    *isAudioEnabled = camera->recording.isAudioEnabled;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraSetRecordingAudioEnabled(HAPPlatformCameraRef camera, bool audioEnabled) {
    HAPPrecondition(camera);

    camera->recording.isAudioEnabled = audioEnabled;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraIsManuallyDisabled(HAPPlatformCameraRef camera, bool* isManuallyDisabled) {
    HAPPrecondition(camera);
    HAPPrecondition(isManuallyDisabled);

    *isManuallyDisabled = false;
    return kHAPError_None;
}
