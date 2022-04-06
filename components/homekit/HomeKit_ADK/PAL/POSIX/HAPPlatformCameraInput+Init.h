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

#ifndef HAP_PLATFORM_CAMERA_INPUT_INIT_H
#define HAP_PLATFORM_CAMERA_INPUT_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include "HAPPlatformCameraInput.h"

#ifdef RPI
// MMAL Dependency.
// It is not possible to import MMAL without causing warnings. For this reason, they are switched off locally:
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wreserved-id-macro")
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wcast-qual")
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wpedantic")
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic")
HAP_DIAGNOSTIC_IGNORED_GCC("-Wvariadic-macros")
#include "interface/mmal/mmal_component.h"
#include "interface/mmal/mmal_pool.h"
HAP_DIAGNOSTIC_POP
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**@file
 * Broadcomm / MMAL camera input.
 *
 * This platform module makes use of the MMAL library to handle camera video streams.
 *
 * This implementation can be used as a baseline to support IP Cameras. However, it needs to be adjusted to different
 * platforms and/or technologies.
 */

/**
 * Camera input stream state.
 */
struct HAPPlatformCameraInputStream {
    // Do not access the instance fields directly.
    /**@cond */
    /** Camera input if active. NULL if not active. */
    HAPPlatformCameraInputRef _Nullable cameraInput;
#ifdef RPI
    MMAL_COMPONENT_T *resizer, *encoder;
    MMAL_POOL_T* pool; /**< Encoder output port pool. */
#endif
    uint64_t startTime; /**< Time reference for timestamps. */
    int8_t balance;     /**< Frame rate resampling balance. */
    uint8_t framerate;  /**< Resampler frame rate. */

    /** Callback to be called for each block. */
    HAPPlatformCameraInputDataCallback callback;
    void* _Nullable context;
    /**@endcond */
};

/**
 * Camera snapshot state.
 */
struct HAPPlatformCameraSnapshot {
    /**@cond */
    /** Camera if active. NULL if not active. */
    HAPPlatformCameraInputRef _Nullable cameraInput;

    /** Camera snapshot image reader. */
    HAPPlatformCameraSnapshotReader reader;

#ifdef RPI
    MMAL_COMPONENT_T *resizer, *encoder;
    MMAL_BUFFER_HEADER_T* volatile _Nullable buffer;
    MMAL_POOL_T* pool;
#endif
    size_t size;   /**< Snapshot size. */
    size_t offset; /**< Reader offset. */
    /**@endcond */
};

/**
 * Camera input initialization options.
 */
typedef struct {
    /** Storage for camera streams. Must remain valid. */
    HAPPlatformCameraInputStream* cameraInputStreams;

    /** Number of concurrent camera streams. */
    size_t numCameraInputStreams;

    /** Storage for camera snapshots. Must remain valid. */
    HAPPlatformCameraSnapshot* cameraSnapshots;

    /** Number of concurrent camera snapshots. */
    size_t numCameraSnapshots;

    /** Configured mode for camera. */
    bool isPortraitMode;

#if (HAP_TESTING == 1)
    /** Video file path. */
    const char* _Nullable mediaPath;
#endif
} HAPPlatformCameraInputOptions;

/**
 * Camera input.
 */
struct HAPPlatformCameraInput {
    // Do not access the instance fields directly.
    /**@cond */
    HAPPlatformCameraInputStream* cameraInputStreams;
    size_t numCameraInputStreams;

    HAPPlatformCameraSnapshot* cameraSnapshots;
    size_t numCameraSnapshots;

#ifdef RPI
    MMAL_COMPONENT_T* cameraHW;
    MMAL_COMPONENT_T* splitter;
    uint32_t width;
    uint32_t height;
#endif
    /**@endcond */
};

/**
 * Initializes a camera input.
 *
 * @param[out] cameraInput          Pointer to an allocated but uninitialized HAPPlatformCameraInput structure.
 * @param      options              Initialization options.
 */
void HAPPlatformCameraInputCreate(HAPPlatformCameraInputRef cameraInput, const HAPPlatformCameraInputOptions* options);

/**
 * De-initializes a camera input.
 *
 * @param      cameraInput          Initialized camera input.
 */
void HAPPlatformCameraInputRelease(HAPPlatformCameraInputRef cameraInput);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
