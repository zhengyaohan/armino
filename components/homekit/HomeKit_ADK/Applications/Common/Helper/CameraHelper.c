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

#ifndef NRF52840_XXAA

#include "HAPPlatformFeatures.h"

#include "ApplicationFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "AudioDevice.h"
#include "CameraHelper.h"

void CameraInputInitialize(
        HAPPlatformCameraInput* cameraInput,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        bool disableCamera HAP_UNUSED) {
    HAPAssert(cameraInput);
    HAPAssert(hapAccessoryServerOptions);

    static HAPPlatformCameraInputStream cameraInputStreams[kNumStreams];
    static HAPPlatformCameraSnapshot cameraSnapshots[kNumSnapshots];

    HAPPlatformCameraInputOptions cameraInputOptions;
    cameraInputOptions.cameraInputStreams = cameraInputStreams,
    cameraInputOptions.numCameraInputStreams = HAPArrayCount(cameraInputStreams),
    cameraInputOptions.cameraSnapshots = cameraSnapshots,
    cameraInputOptions.numCameraSnapshots = HAPArrayCount(cameraSnapshots);
#if (HAP_TESTING == 1)
    cameraInputOptions.mediaPath = hapAccessoryServerOptions->ip.camera.mediaSourcePath;
#ifdef DARWIN
    cameraInputOptions.disable = disableCamera;
#endif
#endif

#if (HAVE_PORTRAIT_MODE == 1)
    cameraInputOptions.isPortraitMode = true;
#else
    cameraInputOptions.isPortraitMode = false;
#endif

    HAPPlatformCameraInputCreate(cameraInput, &cameraInputOptions);
}

void MicrophoneInitialize(
        HAPPlatformMicrophone* microphone,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        bool disableMicrophone HAP_UNUSED) {
    HAPAssert(microphone);
    HAPAssert(hapAccessoryServerOptions);

    static HAPPlatformMicrophoneStream microphoneStreams[kNumStreams];
    HAPPlatformMicrophoneOptions micOptions;
    micOptions.audioInputDevice = kDefaultAudioInputDevice;
    micOptions.microphoneStreams = microphoneStreams;
    micOptions.numMicrophoneStreams = HAPArrayCount(microphoneStreams);
#if (HAP_TESTING == 1)
    micOptions.mediaPath = hapAccessoryServerOptions->ip.camera.mediaSourcePath;
#ifdef DARWIN
    micOptions.disable = disableMicrophone;
#endif
#endif
    HAPPlatformMicrophoneCreate(microphone, &micOptions);
}

void SpeakerInitialize(HAPPlatformSpeaker* speaker) {

    HAPAssert(speaker);
    static HAPPlatformSpeakerStream speakerStreams[kNumStreams];
    const char* outputDevice = kDefaultAudioPlaybackDevice;

    HAPPlatformSpeakerCreate(
            speaker,
            &(const HAPPlatformSpeakerOptions) {
                    .audioOutputDevice = outputDevice,
                    .speakerStreams = speakerStreams,
                    .numSpeakerStreams = HAPArrayCount(speakerStreams),
            });
}

HAPCameraStreamingSessionStorage* AllocateStreamingSessionStorage(void) {

    static HAPCameraStreamingSession cameraStreamingSessions[kNumStreams];
    static HAPCameraStreamingSessionSetup cameraStreamingSessionSetups
            [kHAPIPSessionStorage_DefaultNumElements * kDB_NumCameraRTPStreamManagementServices];
    static HAPCameraStreamingSessionStorage cameraStreamingSessionStorage = {
        .sessions = cameraStreamingSessions,
        .numSessions = HAPArrayCount(cameraStreamingSessions),
        .setups = cameraStreamingSessionSetups,
        .numSetups = HAPArrayCount(cameraStreamingSessionSetups)
    };

    return &cameraStreamingSessionStorage;
}

HAPCameraStreamingSessionStorage* AllocateBridgeStreamingSessionStorage(void) {

    static HAPCameraStreamingSession cameraStreamingSessions[kAppState_NumCameras * kNumStreams];
    static HAPCameraStreamingSessionSetup cameraStreamingSessionSetups
            [kHAPIPSessionStorage_DefaultNumElements * kDB_NumCameraRTPStreamManagementServices * kAppState_NumCameras];
    static HAPCameraStreamingSessionStorage cameraStreamingSessionStorage = {
        .sessions = cameraStreamingSessions,
        .numSessions = HAPArrayCount(cameraStreamingSessions),
        .setups = cameraStreamingSessionSetups,
        .numSetups = HAPArrayCount(cameraStreamingSessionSetups)
    };

    return &cameraStreamingSessionStorage;
}

void CameraInitialize(
        PlatformCamera* platform,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        bool disableMedia) {

    HAPAssert(platform);
    HAPAssert(hapAccessoryServerOptions);
    HAPAssert(hapPlatform);

    CameraInputInitialize(&(platform->cameraInput), hapAccessoryServerOptions, disableMedia);
    MicrophoneInitialize(&(platform->microphone), hapAccessoryServerOptions, disableMedia);
    SpeakerInitialize(&(platform->speaker));

    // IP Camera provider.
    static HAPPlatformCameraStreamingSession streamingSessions[kNumStreams];
    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage = {
        .sessions = streamingSessions, .numSessions = HAPArrayCount(streamingSessions)
    };

    HAPPlatformCameraCreate(
            &(platform->camera),
            &(const HAPPlatformCameraOptions) { .streamingSessionStorage = &platformIPCameraStreamingSessionStorage,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) });

    hapAccessoryServerOptions->ip.camera.streamingSessionStorage = AllocateStreamingSessionStorage();
    hapPlatform->ip.camera = &(platform->camera);
}

void CameraRecorderInitialize(
        PlatformCameraRecorder* platform,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        bool inDisableMedia HAP_UNUSED) {
#ifdef DARWIN
    bool disableMedia = inDisableMedia;
#else
    bool disableMedia = false;
#endif

    HAPAssert(platform);
    HAPAssert(hapAccessoryServerOptions);
    HAPAssert(hapPlatform);

    CameraInputInitialize(&(platform->cameraInput), hapAccessoryServerOptions, disableMedia);
    MicrophoneInitialize(&(platform->microphone), hapAccessoryServerOptions, disableMedia);
    SpeakerInitialize(&(platform->speaker));

    // IP Camera provider.
    static HAPPlatformCameraStreamingSession streamingSessions[kNumStreams];
    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage = {
        .sessions = streamingSessions, .numSessions = HAPArrayCount(streamingSessions)
    };

    static uint8_t platformCameraRecordingStorage[kCameraRecorderStorageSize];
    HAPPlatformCameraRecorderCreate(
            &(platform->camera),
            &(platform->recorder),
            &(const HAPPlatformCameraOptions) { .identifier = 1,
                                                .keyValueStore = hapPlatform->keyValueStore,
                                                .streamingSessionStorage = &platformIPCameraStreamingSessionStorage,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) },
            &(const HAPPlatformCameraRecorderOptions) {
                    .recordingBuffer = { .bytes = platformCameraRecordingStorage,
                                         .numBytes = HAPArrayCount(platformCameraRecordingStorage) } });

    hapAccessoryServerOptions->ip.camera.streamingSessionStorage = AllocateStreamingSessionStorage();
    hapPlatform->ip.camera = &(platform->camera);
}

void BridgeCameraInitialize(
        PlatformBridgeCamera* platform,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform) {

    HAPAssert(platform);
    HAPAssert(hapAccessoryServerOptions);
    HAPAssert(hapPlatform);

    CameraInputInitialize(&(platform->cameraInput), hapAccessoryServerOptions, false);
    MicrophoneInitialize(&(platform->microphone), hapAccessoryServerOptions, false);
    SpeakerInitialize(&(platform->speaker));

    // Prepare camera array.
    for (size_t i = 0; i < HAPArrayCount(platform->cameraStorage); i++) {
        platform->cameras[i] = &(platform->cameraStorage[i]);
    }

    // IP Camera provider.
    static HAPPlatformCameraStreamingSession streamingSessions[kAppState_NumCameras * kNumStreams];

    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage1 = {
        .sessions = &streamingSessions[0 * kNumStreams], .numSessions = kNumStreams
    };
    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage2 = {
        .sessions = &streamingSessions[1 * kNumStreams], .numSessions = kNumStreams
    };

    HAPPlatformCameraCreate(
            platform->cameras[0],
            &(const HAPPlatformCameraOptions) { .identifier = 1,
                                                .streamingSessionStorage = &platformIPCameraStreamingSessionStorage1,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) });
    HAPPlatformCameraCreate(
            platform->cameras[1],
            &(const HAPPlatformCameraOptions) { .identifier = 2,
                                                .streamingSessionStorage = &platformIPCameraStreamingSessionStorage2,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) });

    hapAccessoryServerOptions->ip.camera.streamingSessionStorage = AllocateBridgeStreamingSessionStorage();
}

void BridgeCameraRecorderInitialize(
        PlatformBridgeCameraRecorder* platform,
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform) {

    HAPAssert(platform);
    HAPAssert(hapAccessoryServerOptions);
    HAPAssert(hapPlatform);

    CameraInputInitialize(&(platform->cameraInput), hapAccessoryServerOptions, false);
    MicrophoneInitialize(&(platform->microphone), hapAccessoryServerOptions, false);
    SpeakerInitialize(&(platform->speaker));

    // Prepare camera array.
    for (size_t i = 0; i < HAPArrayCount(platform->cameraStorage); i++) {
        platform->cameras[i] = &(platform->cameraStorage[i]);
    }

    // Prepare recorder array.
    for (size_t i = 0; i < HAPArrayCount(platform->recorderStorage); i++) {
        platform->recorders[i] = &(platform->recorderStorage[i]);
    }

    // IP Camera provider.
    static HAPPlatformCameraStreamingSession streamingSessions[kAppState_NumCameras * kNumStreams];

    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage1 = {
        .sessions = &streamingSessions[0 * kNumStreams], .numSessions = kNumStreams
    };
    static HAPPlatformCameraStreamingSessionStorage platformIPCameraStreamingSessionStorage2 = {
        .sessions = &streamingSessions[1 * kNumStreams], .numSessions = kNumStreams
    };

    static uint8_t platformCameraRecordingStorage1[kCameraRecorderStorageSize];
    static uint8_t platformCameraRecordingStorage2[kCameraRecorderStorageSize];

    HAPPlatformCameraRecorderCreate(
            platform->cameras[0],
            platform->recorders[0],
            &(const HAPPlatformCameraOptions) { .identifier = 1,
                                                .keyValueStore = hapPlatform->keyValueStore,
                                                .streamingSessionStorage = &platformIPCameraStreamingSessionStorage1,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) },
            &(const HAPPlatformCameraRecorderOptions) {
                    .recordingBuffer = { .bytes = platformCameraRecordingStorage1,
                                         .numBytes = HAPArrayCount(platformCameraRecordingStorage1) } });
    HAPPlatformCameraRecorderCreate(
            platform->cameras[1],
            platform->recorders[1],
            &(const HAPPlatformCameraOptions) { .identifier = 2,
                                                .keyValueStore = hapPlatform->keyValueStore,
                                                .streamingSessionStorage = &platformIPCameraStreamingSessionStorage2,
                                                .cameraInput = &(platform->cameraInput),
                                                .microphone = &(platform->microphone),
                                                .speaker = &(platform->speaker) },
            &(const HAPPlatformCameraRecorderOptions) {
                    .recordingBuffer = { .bytes = platformCameraRecordingStorage2,
                                         .numBytes = HAPArrayCount(platformCameraRecordingStorage2) } });

    hapAccessoryServerOptions->ip.camera.streamingSessionStorage = AllocateBridgeStreamingSessionStorage();
}

void CameraDeinitialize(PlatformCamera* platform) {

    HAPAssert(platform);

    HAPPlatformCameraRelease(&(platform->camera));
    HAPPlatformSpeakerRelease(&(platform->speaker));
    HAPPlatformMicrophoneRelease(&(platform->microphone));
    HAPPlatformCameraInputRelease(&(platform->cameraInput));
}

void CameraRecorderDeinitialize(PlatformCameraRecorder* platform) {

    HAPAssert(platform);

    HAPPlatformCameraRecorderRelease(&(platform->camera));
    HAPPlatformSpeakerRelease(&(platform->speaker));
    HAPPlatformMicrophoneRelease(&(platform->microphone));
    HAPPlatformCameraInputRelease(&(platform->cameraInput));
}

void BridgeCameraDeinitialize(PlatformBridgeCamera* platform) {

    HAPAssert(platform);

    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        HAPPlatformCameraRelease(platform->cameras[i]);
    }
    HAPPlatformSpeakerRelease(&(platform->speaker));
    HAPPlatformMicrophoneRelease(&(platform->microphone));
    HAPPlatformCameraInputRelease(&(platform->cameraInput));
}

void BridgeCameraRecorderDeinitialize(PlatformBridgeCameraRecorder* platform) {

    HAPAssert(platform);

    for (size_t i = 0; i < kAppState_NumCameras; i++) {
        HAPPlatformCameraRecorderRelease(platform->cameras[i]);
    }
    HAPPlatformSpeakerRelease(&(platform->speaker));
    HAPPlatformMicrophoneRelease(&(platform->microphone));
    HAPPlatformCameraInputRelease(&(platform->cameraInput));
}

void CameraEndStreaming(HAPPlatformCameraRef _Nonnull camera) {
    HAPPrecondition(camera);

    for (size_t streamIndex = 0; streamIndex < kNumStreams; streamIndex++) {
        HAPCameraStreamingStatus status = HAPPlatformCameraGetStreamStatus(camera, streamIndex);
        if (status == kHAPCameraStreamingStatus_InUse) {
            HAPPlatformCameraEndStreamingSession(camera, streamIndex);
            HAPError err =
                    HAPPlatformCameraTrySetStreamStatus(camera, streamIndex, kHAPCameraStreamingStatus_Available);
            HAPAssert(err == kHAPError_None);
        }
    }
}

#endif

#endif
