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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef HAP_PLATFORM_CAMERA_INIT_H
#define HAP_PLATFORM_CAMERA_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <net/if.h>
#include <pthread.h>

#include "HAP.h"

#include "HAPPlatformCameraInput.h"
#include "HAPPlatformMicrophone.h"
#include "HAPPlatformSpeaker.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Camera recorder.
 */
typedef struct HAPPlatformCameraRecorder HAPPlatformCameraRecorder;
typedef struct HAPPlatformCameraRecorder* HAPPlatformCameraRecorderRef;
HAP_NONNULL_SUPPORT(HAPPlatformCameraRecorder)

/**
 * Log object to indicate video output.
 */
extern const HAPLogObject kHAPPlatformCamera_VideoLog;

/**
 * Log object to indicate audio output.
 */
extern const HAPLogObject kHAPPlatformCamera_AudioLog;

/**
 * Log object to indicate speaker output.
 */
extern const HAPLogObject kHAPPlatformCamera_SpeakerLog;

/**@cond */
/**
 * Platform-specific state of a stream.
 */
typedef struct {
    /**
     * The structure is initialized.
     */
    bool isInitialized : 1;

    /**
     * The thread is running.
     */
    bool isActive : 1;

    /**
     * The thread is aborted.
     */
    volatile bool isAborted;

    /**
     * The stream is suspended.
     */
    volatile bool isSuspended;

    /**
     * HAP RTP Stream.
     */
    HAPRTPStream rtpStream;

    /**
     * Socket to send/receive RTP data.
     */
    int socket;

    /**
     * Thread used to poll input socket.
     */
    pthread_t thread;

    /**
     * Associated camera object.
     */
    HAPPlatformCameraRef camera;

    /**
     * IP Camera stream index.
     */
    size_t streamIndex;

    /**
     * Camera input stream state.
     */
    HAPPlatformCameraInputStreamRef cameraInputStream;

    /**
     * Microphone stream state.
     */
    HAPPlatformMicrophoneStreamRef _Nullable microphoneStream;

    /**
     * Speaker stream state.
     */
    HAPPlatformSpeakerStreamRef _Nullable speakerStream;
} HAPPlatformCameraStream;
/**@endcond */

/**
 * Platform-specific state of an IP Camera streaming session.
 */
typedef struct {
    // Do not access the instance fields directly.
    /**@cond */
    /**
     * Video stream.
     */
    HAPPlatformCameraStream video;

    /**
     * Audio stream.
     */
    HAPPlatformCameraStream audio;

    /**
     * Streaming status.
     */
    HAPCameraStreamingStatus status;
    /**@endcond */
} HAPPlatformCameraStreamingSession;

/**
 * Platform-specific state of the IP Camera streaming sessions.
 *
 * - The provided streaming sessions must remain valid while the platform is initialized.
 */
typedef struct {
    /**
     * IP Camera streaming sessions.
     *
     * - One session must be provided per Camera RTP Stream Management Service.
     */
    HAPPlatformCameraStreamingSession* sessions;

    /**
     * Number of sessions.
     */
    size_t numSessions;
} HAPPlatformCameraStreamingSessionStorage;

/**
 * Camera provider initialization options.
 */
typedef struct {
    /**
     * Camera identifier.
     */
    uint64_t identifier;

    /**
     * Key-value store.
     *
     * Only needed when camera supports disabling of streaming feature, or when it supports recording feature.
     */
    HAPPlatformKeyValueStoreRef _Nullable keyValueStore;

    /**
     * Local network interface name on which to bind the IP Camera streaming session endpoints.
     * A value of NULL will use all available network interfaces.
     */
    const char* _Nullable interfaceName;

    /**
     * Memory for platform-specific state of the IP Camera streaming sessions.
     */
    HAPPlatformCameraStreamingSessionStorage* streamingSessionStorage;

    /**
     * Camera input.
     */
    HAPPlatformCameraInputRef cameraInput;

    /**
     * Microphone.
     */
    HAPPlatformMicrophoneRef microphone;

    /**
     * Speaker.
     */
    HAPPlatformSpeakerRef speaker;
} HAPPlatformCameraOptions;

/**
 * The resolution or mode change delegate.
 */
typedef struct {
    /**
     * Invoked when the streaming resolution changes.
     *
     * @param      camera               Camera provider.
     * @param      attributes           The new video attributes or NULL if there is no current stream.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_InvalidData    If the configuration cannot be adapted.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*_Nullable handleResolutionChanged)(
            HAPPlatformCameraRef camera,
            const HAPVideoAttributes* _Nullable attributes);

    /**
     * Invoked when an Operating Mode Active status changes.
     *
     * @param      camera               Camera provider.
     * @param      configurationChanged Whether the recording configuration changed.
     */
    void (*_Nullable handleOperatingModeChanged)(HAPPlatformCameraRef camera, bool configurationChanged);
} HAPPlatformCameraRecorderDelegate;

/**
 * Camera provider.
 */
struct HAPPlatformCamera {
    // Do not access the instance fields directly.
    /**@cond */
    /**
     * Camera identifier.
     */
    uint64_t identifier;

    /**
     * Key-value store.
     */
    HAPPlatformKeyValueStoreRef _Nullable keyValueStore;

    /**
     * Local network interface name on which to bind the IP Camera streaming session endpoints.
     * An empty string will use all available network interfaces.
     */
    char interfaceName[IFNAMSIZ];

    /**
     * Streaming session storage.
     */
    HAPPlatformCameraStreamingSessionStorage streamingSessionStorage;

    /**
     * Delegate to monitor camera stream status.
     */
    HAPPlatformCameraDelegate delegate;

    /**
     * Delegate to monitor camera streaming changes.
     */
    HAPPlatformCameraRecorderDelegate recorderDelegate;

    /**
     * Camera input.
     */
    HAPPlatformCameraInputRef cameraInput;

    /**
     * Microphone.
     */
    HAPPlatformMicrophoneRef microphone;

    /**
     * Speaker.
     */
    HAPPlatformSpeakerRef speaker;

    /**
     * Associated recorder if present, NULL otherwise.
     */
    HAPPlatformCameraRecorderRef _Nullable recorder;
    /**@endcond */
};

/**
 * Specifies or clears the delegate for receiving recorder events.
 *
 * @param      camera               Camera provider.
 * @param      delegate             The delegate to receive the recorder events. NULL to clear.
 */
void HAPPlatformCameraSetRecorderDelegate(
        HAPPlatformCameraRef camera,
        const HAPPlatformCameraRecorderDelegate* _Nullable delegate);

/**
 * Initializes the IP Camera provider.
 *
 * @param[out] camera               Pointer to an allocated but uninitialized HAPPlatformCamera structure.
 * @param      options              Initialization options.
 */
void HAPPlatformCameraCreate(HAPPlatformCameraRef camera, const HAPPlatformCameraOptions* options);

/**
 * Releases resources associated with an initialized camera.
 *
 * @param      camera               Camera provider.
 */
void HAPPlatformCameraRelease(HAPPlatformCameraRef camera);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
