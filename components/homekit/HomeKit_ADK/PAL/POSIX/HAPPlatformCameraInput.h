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

#ifndef HAP_PLATFORM_CAMERA_INPUT_H
#define HAP_PLATFORM_CAMERA_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * \rst
 *
 * Video input abstraction. It is only needed if the default camera PAL implementation of Apple is used. For more
 * information, see :doc:`IP Camera  </ip_camera>` documentation.
 *
 * An implementation must be able to provide at least two H.264 encoded video channels, one with at least *1920 x 1080*
 * pixels and one with at least 1200 x 720 pixels. In addition it must be able to deliver a JPEG-encoded snapshot
 * picture in parallel. Dedicated hardware is usually needed to achieve the required performance.
 *
 * \endrst
 */

/**
 * Camera.
 */
typedef struct HAPPlatformCameraInput HAPPlatformCameraInput;
typedef struct HAPPlatformCameraInput* HAPPlatformCameraInputRef;
HAP_NONNULL_SUPPORT(HAPPlatformCameraInput)

/**
 * Camera stream.
 */
typedef struct HAPPlatformCameraInputStream HAPPlatformCameraInputStream;
typedef struct HAPPlatformCameraInputStream* HAPPlatformCameraInputStreamRef;
HAP_NONNULL_SUPPORT(HAPPlatformCameraInputStream)

/**
 * Camera snapshot.
 */
typedef struct HAPPlatformCameraSnapshot HAPPlatformCameraSnapshot;
typedef struct HAPPlatformCameraSnapshot* HAPPlatformCameraSnapshotRef;
HAP_NONNULL_SUPPORT(HAPPlatformCameraSnapshot)

/**
 * Invoked when new video data is available.
 *
 * The callback is called from a pipeline thread.
 *
 * @param      context              Context.
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 * @param      bytes                Video data.
 * @param      numBytes             Length of video data.
 * @param      sampleTime           Sample time of first data entry [ns].
 */
typedef void (*_Nullable HAPPlatformCameraInputDataCallback)(
        void* _Nullable context,
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime);

#ifdef DARWIN
/**
 * Invoked when the end of the video file has been reached.
 *
 * The callback is called from a pipeline thread.
 *
 * @param      context              Context.
 */
typedef void (*_Nullable HAPPlatformCameraInputEOFCallback)(void* _Nullable context);
#endif

/**
 * Starts a camera video stream.
 *
 * @param      cameraInput          Camera input.
 * @param[out] cameraInputStream    Camera input stream.
 * @param      width                Video format width.
 * @param      height               Video format height.
 * @param      framerate            Video framerate [Hz].
 * @param      bitrate              Bit-rate [bit/s].
 * @param      keyFrameInterval     Video key frame interval [ms].
 * @param      profile              Video encoding profile.
 * @param      level                Video encoding level.
 * @param      callback             Callback to be called for each block.
 * #ifdef DARWIN
 * @param      eofCallback          Callback to be called when the end of the
 * provided video file is reached. If NULL, video file will keep looping.
 * #endif
 * @param      context              Callback context.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputStartStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef _Nullable* _Nonnull cameraInputStream,
        uint32_t width,
        uint32_t height,
        uint32_t framerate,
        uint32_t bitrate,
        uint32_t keyFrameInterval,
        HAPH264VideoCodecProfile profile,
        HAPH264VideoCodecProfileLevel level,
        HAPPlatformCameraInputDataCallback callback,
#ifdef DARWIN
        HAPPlatformCameraInputEOFCallback eofCallback,
#endif
        void* _Nullable context);

/**
 * Stops the camera video stream.
 *
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 */
void HAPPlatformCameraInputStopStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream);

/**
 * Suspends the camera video stream.
 *
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 */
void HAPPlatformCameraInputSuspendStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream);

/**
 * Resumes the camera video stream.
 *
 * @param      cameraInput          Camera.
 * @param      cameraInputStream    Camera stream.
 */
void HAPPlatformCameraInputResumeStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream);

/**
 * Checks whether camera video stream driver is in a running state.
 *
 * This function should be used to check the sanity of the driver.
 *
 * @param      cameraInputStream  Camera stream
 * @param[out] stateString        Variable to store the pointer to the state description string
 *
 * @return true if the camera video stream driver is in a running state. false, otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformCameraInputIsStreamRunning(
        HAPPlatformCameraInputStreamRef cameraInputStream,
        const char* _Nullable* _Nonnull stateString);

/**
 * Reconfigures the camera video stream.
 *
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 * @param      width                Video format width.
 * @param      height               Video format height.
 * @param      framerate            Video framerate [Hz].
 * @param      bitrate              Bit-rate [bit/s].
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputReconfigureStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        uint32_t width,
        uint32_t height,
        uint32_t framerate,
        uint32_t bitrate);

/**
 * Sets the new video stream bitrate.
 *
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 * @param      bitrate              Bit-rate [bit/s].
 */
void HAPPlatformCameraInputSetBitrate(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        uint32_t bitrate);

/**
 * Requests a new key frame.
 *
 * @param      cameraInput          Camera input.
 * @param      cameraInputStream    Camera input stream.
 */
void HAPPlatformCameraInputRequestKeyFrame(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream);

/**
 * Initializes the snapshot state.
 *
 * @param      cameraInput          Camera input.
 * @param[out] snapshot             Snapshot state.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputInitializeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef _Nonnull* _Nonnull snapshot);

/**
 * Takes a camera snapshot.
 *
 * @param      cameraInput          Camera input.
 * @param      snapshot             Snapshot state.
 * @param      width                Width of the requested image.
 * @param      height               Height of the requested image.
 * @param[out] snapshotReader       Reader for the image.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputTakeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef snapshot,
        uint16_t width,
        uint16_t height,
        HAPPlatformCameraSnapshotReader* _Nonnull* _Nonnull snapshotReader);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
