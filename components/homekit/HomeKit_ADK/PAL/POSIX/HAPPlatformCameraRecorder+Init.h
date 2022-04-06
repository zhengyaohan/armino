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

#ifndef HAP_PLATFORM_CAMERA_RECORDING_INIT_H
#define HAP_PLATFORM_CAMERA_RECORDING_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
#include "HAPPlatformCamera+Init.h"

#include <pthread.h> // pthread_mutex_t

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define kHAPPlatformCameraRecorder_MaxFragmentDuration (4000) // ms
#define kHAPPlatformCameraRecorder_MaxRunDuration      (500)  // ms
#define kHAPPlatformCameraRecorder_MaxRuns \
    (((kHAPPlatformCameraRecorder_MaxFragmentDuration) / (kHAPPlatformCameraRecorder_MaxRunDuration) + 2) * 2)

#define kHAPPlatformCameraRecorder_NumChunkBytes        (262144) // 256K
#define kHAPPlatformCameraRecorder_NumExtraScratchBytes (256)
#define kHAPPlatformCameraRecorder_NumScratchBytes \
    (kHAPPlatformCameraRecorder_NumChunkBytes + kHAPPlatformCameraRecorder_NumExtraScratchBytes)

/**
 * Camera recording buffer.
 */
typedef struct {
    uint8_t* bytes;  /**< Buffer. */
    size_t numBytes; /**< Length of buffer. */
} HAPPlatformCameraRecordingBuffer;

/**
 * Camera recorder provider initialization options.
 */
typedef struct {
    /** Memory used to store the current camera recording. */
    HAPPlatformCameraRecordingBuffer recordingBuffer;
} HAPPlatformCameraRecorderOptions;

/**
 * Camera recorder states.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformCameraRecordingState) { /** Recording is disabled (not active). */
                                                           kHAPPlatformCameraRecordingState_Disabled,

                                                           /** Recording is monitoring (pre trigger). */
                                                           kHAPPlatformCameraRecordingState_Monitoring,

                                                           /** Recording is recording (post trigger). */
                                                           kHAPPlatformCameraRecordingState_Recording,

                                                           /** Recording is paused (out of memory). */
                                                           kHAPPlatformCameraRecordingState_Paused
} HAP_ENUM_END(uint8_t, HAPPlatformCameraRecordingState);

/**
 * Camera recorder queue.
 */
typedef struct {
    uint8_t* bytes;        /**< Memory used to store the queue. */
    size_t numBytes;       /**< Size of the queue. */
    size_t headIndex;      /**< Head index of the queue. */
    size_t tailIndex;      /**< Tail index of the queue. */
    size_t tempIndex;      /**< Temporary tail index of the queue. */
    size_t remainingBytes; /**< Remaining bytes in current frame. */
    pthread_mutex_t mutex; /**< Access synchronization */
} HAPPlatformCameraRecorderQueue;

/**
 * The recording context associated with a data stream.
 */
typedef struct {
    HAPAccessoryServer* server;
    HAPDataStreamDispatcher* dispatcher;
    HAPDataStreamHandle dataStream;
    HAPDataSendDataStreamProtocolStream* dataSendStream;
    bool readyToSend;
} HAPPlatformCameraRecorderDataStreamContext;

/**
 * Camera recorder provider.
 */
struct HAPPlatformCameraRecorder {
    // Do not access the instance fields directly.
    /**@cond */
    /** Associated camera object. */
    HAPPlatformCameraRef camera;

    /** Video stream. */
    HAPPlatformCameraInputStreamRef cameraInputStream;

    /** Microphone stream. */
    HAPPlatformMicrophoneStreamRef microphoneStream;

    /** Last audio timestamp for audio-video synchronization [ns] */
    HAPTimeNS audioStreamTime;

    /** Video time correction for audio-video synchronization [ns] */
    int64_t videoTimeCorrection;

    /** Speaker stream. */
    HAPPlatformSpeakerStreamRef speakerStream;

    /** Memory used to store the camera recording. */
    HAPPlatformCameraRecordingBuffer recordingBuffer;

    /** Video synchronization queue. */
    HAPPlatformCameraRecorderQueue videoSynchQueue;

    /** Audio synchronization queue. */
    HAPPlatformCameraRecorderQueue audioSynchQueue;

    /** Main recording queue with alternating video and audio runs. */
    HAPPlatformCameraRecorderQueue mainQueue;

    /** The index of the start of the current fragment in the main queue. */
    size_t fragmentStartIndex;

    /** Recording state. */
    HAPPlatformCameraRecordingState recordingState;

    /** End time of the current fragment [ns]. */
    HAPTimeNS fragmentEnd;

    /** Bool indicating key frame is requested */
    bool keyFrameRequested;

    /** Start time of the last video run [ns]. */
    HAPTimeNS videoRunStart;

    /** The run currently added to the main queue is a video run. */
    bool runIsVideo;

    /** Margin for extra key frame request [ns]. */
    HAPTimeNS extraKeyFrameMargin;

    /** Time span from first to last frame of a fragment [ns]. */
    HAPTimeNS fragmentDuration;

    /** Time span from first to last frame of a run [ns]. */
    HAPTimeNS runDuration;

    /** Margin for prebuffer region [ns]. */
    HAPTimeNS prebufferMargin;

    /** SPS bytes for current recording. */
    uint8_t sps[64];

    /** PPS bytes for current recording. */
    uint8_t pps[64];

    /** MP4 configuration for current recording. */
    HAPFragmentedMP4RecordingConfiguration mp4Configuration;

    /** The configuration changed while the recorder is streaming. */
    bool configurationChanged;

    /** The video timestamp frequency. */
    uint32_t videoFrequency;

    /** The audio timestamp frequency. */
    uint32_t audioFrequency;

    /** The size of the current fragment. */
    size_t fragmentSize;

    /** Start time of first transmitted fragment. */
    HAPTimeNS firstFragmentTime;

    /** The sequence number of the current fragment. */
    uint32_t fragmentNumber;

    /** The sequence number of the current chunk. */
    uint32_t chunkNumber;

    /** Recorder Data Stream context. */
    HAPPlatformCameraRecorderDataStreamContext dataStreamContext;

    /** Recorder Data Send Stream. */
    HAPDataSendDataStreamProtocolStream dataSendStream;

    /** Recorder Data Send scratch bytes. */
    uint8_t dataSendScratchBytes[kHAPPlatformCameraRecorder_NumScratchBytes];

#ifdef DARWIN
    /** Indicates that the end of the provided video file has been reached.  */
    bool videoEOF;
#endif
};

/**
 * Initializes the IP Camera recorder.
 *
 * @param[out] camera               Pointer to an allocated but uninitialized HAPPlatformCamera structure.
 * @param[out] recorder             Pointer to an allocated but uninitialized HAPPlatformCameraRecorder structure.
 * @param      cameraOptions        Camera initialization options.
 * @param      recorderOptions      Recorder initialization options.
 */
void HAPPlatformCameraRecorderCreate(
        HAPPlatformCameraRef camera,
        HAPPlatformCameraRecorderRef recorder,
        const HAPPlatformCameraOptions* cameraOptions,
        const HAPPlatformCameraRecorderOptions* recorderOptions);

/**
 * Returns whether the recording is currently enabled.
 *
 * @param      camera               IP Camera provider.
 *
 * @return true                     If recording is enabled.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformCameraIsRecordingEnabled(HAPPlatformCameraRef camera);

/**
 * Returns the current recording resolution if the recorder is running.
 *
 * @param      camera               IP Camera provider.
 *
 * @return true                     The current image height or 0 if the recorder is disabled.
 */
HAP_RESULT_USE_CHECK
uint32_t HAPPlatformCameraGetRecordingResolution(HAPPlatformCameraRef camera);

/**
 * Releases resources associated with an initialized camera recorder.
 *
 * @param      camera               IP Camera provider.
 */
void HAPPlatformCameraRecorderRelease(HAPPlatformCameraRef camera);

/**
 * Starts or restarts the recording with a new selected recording configuration.
 *
 * @param      camera               IP Camera provider.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraConfigureRecording(HAPPlatformCamera* camera);

/**
 * Stops recording.
 *
 * @param      camera               IP Camera provider.
 */
void HAPPlatformCameraDisableRecording(HAPPlatformCamera* camera);

/**
 * Triggers a recording.
 *
 * @param      camera               IP Camera provider.
 */
void HAPPlatformCameraTriggerRecording(HAPPlatformCameraRef camera);

/**
 * Data stream available handler.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStreamProtocol   HomeKit Data Stream protocol handler.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      metadata             HomeKit Data Send open metadata.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
void HAPPlatformCameraRecorderHandleDataSendStreamAvailable(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataSendDataStreamProtocol* dataStreamProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        HAPDataSendDataStreamProtocolType type,
        HAPDataSendDataStreamProtocolOpenMetadata* _Nullable metadata HAP_UNUSED,
        void* _Nullable inDataSendStreamCallbacks,
        void* _Nullable context HAP_UNUSED);

/**
 * Starts the recorder and sets it to Monitoring mode.
 *
 * @param      recorder             Recorder.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraStartRecording(HAPPlatformCameraRecorder* _Nullable recorder);

/**
 * Stops the recorder and sets it to Disabled.
 *
 * @param      recorder             Recorder.
 */
void HAPPlatformCameraStopRecording(HAPPlatformCameraRecorder* _Nullable recorder);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
