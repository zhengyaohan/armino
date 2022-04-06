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

#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>

#include "HAP.h"
#include "stdio.h"

#include "HAPPlatformCameraInput+Init.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "CameraInput" };

#define kNALUStart4 (0x01000000)
#define kNALUStart3 (0x010000)
// NAL unit types.
// "ITU-T H.264", Table 7-1.
#define kNALUTypeMask (0x1F)
#define kNALUTypeIDR  (5)
#define kNALUTypeSEI  (6)
#define kNALUTypeSPS  (7)
#define kNALUTypePPS  (8)
// Flag set on starting key frame of a fragment.
#define kNALUTypeFragmentStartFlag (0x40)
// Flag set on first frame after an out-of-memory condition.
#define kNALUTypeRestartFrameFlag (0x80)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Finds Next NAL Prefix in given binary file
 *
 * @param      f              Bitstream file.
 * @param      offset         Offset within file to begin search from.
 *
 * @return offset where next NAL unit starts
 */
static size_t FindNALPrefix(FILE* f, size_t offset) {
    HAPPrecondition(f);
    uint32_t num = 0;
    const uint8_t* ptr = (const uint8_t*) &num;
    fseek(f, offset, SEEK_SET);
    while (fread((void*) ptr, sizeof(uint32_t), 1, f)) {
        if ((*(const uint32_t*) ptr == kNALUStart4) || (((*(const uint32_t*) ptr) & 0xffffff) == kNALUStart3)) {
            return offset;
        }
        offset++;
        fseek(f, offset, SEEK_SET);
    }
    return ftell(f);
}

/**
 * Skips the NAL start code
 *
 * @param      f              Bitstream file.
 * @param      offset         Offset within file to begin search from.
 *
 * @return offset where NAL unit starts after start code
 */
static size_t SkipNALPrefix(FILE* f, size_t offset) {
    HAPPrecondition(f);
    uint32_t num = 0;
    const uint8_t* ptr = (const uint8_t*) &num;
    fseek(f, offset, SEEK_SET);
    while (fread((void*) ptr, sizeof(uint32_t), 1, f)) {
        if (*(const uint32_t*) ptr == kNALUStart4) {
            return offset + 4;
        }
        if ((((*(const uint32_t*) ptr) & 0xffffff) == kNALUStart3)) {
            return offset + 3;
        }
        offset++;
        fseek(f, offset, SEEK_SET);
    }
    return ftell(f);
}

/**
 * Starts Video Source
 *
 * @param      width            width
 * @param      height           height
 * @param      stream           associated camera input stream
 */
static void StartVideoSource(size_t width, size_t height, HAPPlatformCameraInputStream* stream) {
    HAPPrecondition(stream);
    HAPPrecondition(stream->cameraInput);

    const char* _Nonnull mediaPath = kMediaFilePath;
#if (HAP_TESTING == 1)
    mediaPath = stream->cameraInput->mediaPath;
#endif

    if (!mediaPath) {
        HAPLogError(&logObject, "%s: Failed to start video source", __func__);
        HAPFatalError();
        return;
    }

    NSMutableString* path = [[NSMutableString alloc] initWithCString:mediaPath encoding:NSUTF8StringEncoding];
    BOOL isDir = NO;
    if (path && [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir] && isDir) {
        path = [[NSMutableString alloc] initWithFormat:@"%@/%zux%zu.h264", path, width, height];
    }

    HAPLogDebug(&logObject, "%s: video file used: %s", __func__, [path UTF8String]);

    const char* _Nullable cPath = [path UTF8String];

    if (stream->f) {
        fclose(stream->f);
        stream->f = NULL;
    }

    stream->f = fopen(cPath, "rb");
    if (!stream->f) {
        int _errno = errno;
        HAPLogError(&logObject, "%s: Failed to open(%d) video file %s ", __func__, _errno, cPath);
        HAPFatalError();
    }

    stream->offset = 0;
}

/**
 * Stops Video Source
 *
 * @param      stream           associated camera input stream
 */
static void StopVideoSource(HAPPlatformCameraInputStream* stream) {
    HAPPrecondition(stream);

    if (stream->f) {
        fclose(stream->f);
        stream->f = NULL;
    }

    stream->offset = 0;
    stream->ts = 0;

    if (stream->callbackTimer) {
        NSTimer* timer = (__bridge NSTimer*) stream->callbackTimer;
        [timer invalidate];
        timer = nil;
        stream->callbackTimer = NULL;
    }
}

/**
 * Receives Video Frames from source and pushes to client
 *
 * @param      stream           associated camera input stream
 */
static void ReceiveVideoFrames(HAPPlatformCameraInputStream* stream) {
    HAPPrecondition(stream);

    if (!stream->f || !stream->cameraInput) {
        HAPLogInfo(&logObject, "%s: missing file or camerainput", __func__);
        return;
    }

    HAPPrecondition(stream->f);

    uint32_t num = 0;
    const uint8_t* ptr = (const uint8_t*) &num;
    uint8_t* start = NULL;

    fseek(stream->f, stream->offset, SEEK_SET);
    if (fread((void*) ptr, sizeof(uint32_t), 1, stream->f) == 0) {
        // The end of the video file has been reached.
        if (stream->eofCallback != NULL) {
            // If a callback was provided for when the video file reached EOF, invoke the callback and return.
            // Do not continue processing the video in this function.
            HAPLogInfo(&logObject, "%s: reached end of file for video recording.", __func__);
            stream->eofCallback(stream->context);
            return;
        }
        // Restart processing at the beginning of the video file.
        stream->offset = 0;
        fseek(stream->f, stream->offset, SEEK_SET);
        fread((void*) ptr, sizeof(uint32_t), 1, stream->f);
    }

    stream->offset = SkipNALPrefix(stream->f, stream->offset);
    size_t nextOffset = FindNALPrefix(stream->f, stream->offset);

    size_t frameSize = nextOffset - stream->offset;
    if (frameSize == 0) {
        HAPLogError(&logObject, "%s: frameSize is zero.", __func__);
        return;
    }

    start = (uint8_t*) malloc(frameSize);
    if (start == NULL) {
        return;
    }

    fseek(stream->f, stream->offset, SEEK_SET);

    fread((void*) start, frameSize, 1, stream->f);

    stream->offset = nextOffset;

    uint32_t type = ((const uint8_t*) start)[0] & kNALUTypeMask;

    if (type != kNALUTypeSEI) {
        // Push frames to camera/camera recorder
        stream->callback(stream->context, HAPNonnull(stream->cameraInput), stream, start, frameSize, stream->ts);
    }

    bool isConfigData = false;
    if (type == kNALUTypeSPS || type == kNALUTypePPS || type == kNALUTypeSEI) {
        isConfigData = true;
    }

    // Don't update timestamps for SPS/PPS
    if (isConfigData) {
        ReceiveVideoFrames(stream);
    } else {
        if (stream->framerate) {
            // Calculate ts in nanoseconds
            uint64_t t = (1000000 / stream->framerate);
            t *= 1000;
            stream->ts += t;
        }
    }

    free(start);
}

void HAPPlatformCameraInputCreate(HAPPlatformCameraInputRef cameraInput, const HAPPlatformCameraInputOptions* options) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(options);
    HAPPrecondition(options->cameraInputStreams);
    HAPPrecondition(options->cameraSnapshots);

    for (size_t i = 0; i < options->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef cameraInputStream = &options->cameraInputStreams[i];
        HAPRawBufferZero(cameraInputStream, sizeof *cameraInputStream);
    }

    for (size_t i = 0; i < options->numCameraSnapshots; i++) {
        HAPPlatformCameraSnapshotRef cameraSnapshot = &options->cameraSnapshots[i];
        HAPRawBufferZero(cameraSnapshot, sizeof *cameraSnapshot);
    }

    HAPRawBufferZero(cameraInput, sizeof *cameraInput);
    cameraInput->cameraInputStreams = options->cameraInputStreams;
    cameraInput->numCameraInputStreams = options->numCameraInputStreams;
    cameraInput->cameraSnapshots = options->cameraSnapshots;
    cameraInput->numCameraSnapshots = options->numCameraSnapshots;

#if (HAP_TESTING == 1)
    if (options->disable) {
        cameraInput->mediaPath = NULL;
    } else {
        cameraInput->mediaPath = options->mediaPath;
        if (options->mediaPath == NULL) {
            cameraInput->mediaPath = kMediaFilePath;
        }
    }
    HAPLogInfo(&logObject, "camera input media path %s", cameraInput->mediaPath);
#endif
}

void HAPPlatformCameraInputRelease(HAPPlatformCameraInputRef cameraInput) {
    HAPPrecondition(cameraInput);

    for (size_t i = 0; i < cameraInput->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef cameraInputStream = &cameraInput->cameraInputStreams[i];
        if (cameraInputStream->cameraInput) {
            HAPLogError(
                    &logObject,
                    "Camera input stream %p not stopped before releasing camera.",
                    (const void*) cameraInputStream);
            HAPFatalError();
        }
        HAPRawBufferZero(cameraInputStream, sizeof *cameraInputStream);
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputStartStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef _Nullable* _Nonnull cameraInputStream,
        uint32_t width,
        uint32_t height,
        uint32_t framerate,
        uint32_t bitrate HAP_UNUSED,
        uint32_t keyFrameInterval HAP_UNUSED,
        HAPH264VideoCodecProfile profile HAP_UNUSED,
        HAPH264VideoCodecProfileLevel level HAP_UNUSED,
        HAPPlatformCameraInputDataCallback dataCallback,
        HAPPlatformCameraInputEOFCallback eofCallback,
        void* _Nullable context) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(dataCallback);
    HAPPrecondition(cameraInput->cameraInputStreams);
    HAPPrecondition(framerate);

    // Find free camera input stream.
    *cameraInputStream = NULL;
    for (size_t i = 0; i < cameraInput->numCameraInputStreams; i++) {
        HAPPlatformCameraInputStreamRef stream = &cameraInput->cameraInputStreams[i];
        if (!stream->cameraInput) {
            HAPRawBufferZero(stream, sizeof *stream);
            stream->cameraInput = cameraInput;
            *cameraInputStream = stream;
            break;
        }
    }
    if (!*cameraInputStream) {
        HAPLogError(
                &logObject,
                "[%p] No additional concurrent camera input streams may be started.",
                (const void*) cameraInput);
        return kHAPError_OutOfResources;
    }

    // Actual start time is set in callback.
    (*cameraInputStream)->startTime = 0;
    (*cameraInputStream)->callback = dataCallback;
    (*cameraInputStream)->eofCallback = eofCallback;
    (*cameraInputStream)->context = context;
    (*cameraInputStream)->framerate = framerate;

#if (HAP_TESTING == 1)
    if ((*cameraInputStream)->cameraInput->mediaPath != NULL) {
#endif
        StartVideoSource(width, height, (*cameraInputStream));

        NSTimer* timer = nil;

        timer = [NSTimer scheduledTimerWithTimeInterval:((NSTimeInterval) 1) / framerate
                                                repeats:YES
                                                  block:^(NSTimer* timer HAP_UNUSED) {
                                                      dispatch_async(dispatch_get_main_queue(), ^{
                                                          ReceiveVideoFrames(*cameraInputStream);
                                                      });
                                                  }];

        // Associated with the callback field, not eofCallback.
        (*cameraInputStream)->callbackTimer = (__bridge void*) timer;
#if (HAP_TESTING == 1)
    }
#endif
    return kHAPError_None;
}

void HAPPlatformCameraInputStopStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

    StopVideoSource(cameraInputStream);

    cameraInputStream->cameraInput = NULL;
}

void HAPPlatformCameraInputSuspendStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

    if (cameraInputStream->callbackTimer) {
        NSTimer* timer = (__bridge NSTimer*) cameraInputStream->callbackTimer;
        [timer invalidate];
        timer = nil;
        cameraInputStream->callbackTimer = NULL;
    }
}

void HAPPlatformCameraInputResumeStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

    NSTimer* timer = nil;

    timer = [NSTimer scheduledTimerWithTimeInterval:((NSTimeInterval) 1) / cameraInputStream->framerate
                                            repeats:YES
                                              block:^(NSTimer* timer HAP_UNUSED) {
                                                  dispatch_async(dispatch_get_main_queue(), ^{
                                                      ReceiveVideoFrames(cameraInputStream);
                                                  });
                                              }];

    cameraInputStream->callbackTimer = (__bridge void*) timer;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformCameraInputIsStreamRunning(
        HAPPlatformCameraInputStreamRef cameraInputStream,
        const char* _Nullable* _Nonnull stateString) {
    HAPPrecondition(cameraInputStream);
    HAPPrecondition(stateString);
    if (cameraInputStream->callbackTimer) {
        *stateString = "Enabled";
        return true;
    }
    *stateString = "Disabled";
    return false;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputReconfigureStream(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        uint32_t width,
        uint32_t height,
        uint32_t framerate,
        uint32_t bitrate HAP_UNUSED) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

    // Stop previous source
    StopVideoSource(cameraInputStream);

    cameraInputStream->framerate = framerate;

    // Restart source with new configuration
    StartVideoSource(width, height, cameraInputStream);

    HAPPlatformCameraInputResumeStream(cameraInput, cameraInputStream);

    return kHAPError_None;
}

void HAPPlatformCameraInputSetBitrate(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream,
        uint32_t bitrate HAP_UNUSED) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);
}

void HAPPlatformCameraInputRequestKeyFrame(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraInputStreamRef cameraInputStream) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(cameraInputStream);

    cameraInputStream->offset = 0;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputInitializeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef _Nonnull* _Nonnull snapshot) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(snapshot);
    HAPPrecondition(cameraInput->cameraSnapshots);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformCameraInputTakeSnapshot(
        HAPPlatformCameraInputRef cameraInput,
        HAPPlatformCameraSnapshotRef snapshot,
        uint16_t width HAP_UNUSED,
        uint16_t height HAP_UNUSED,
        HAPPlatformCameraSnapshotReader* _Nonnull* _Nonnull snapshotReader) {
    HAPPrecondition(cameraInput);
    HAPPrecondition(snapshot);
    HAPPrecondition(snapshotReader);

    return kHAPError_None;
}

#endif
