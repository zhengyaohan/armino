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

#include "HAP.h"

#include "HAPPlatformMicrophone+Init.h"
#include "HAPPlatformMicrophone.h"

#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#include <errno.h>
#include "stdio.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE) && HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Microphone" };

/**
 * AAC samples per packet.
 */
#define kAACSamplesPerPacket (1024)

@class MicrophoneStreamContext;

@interface MicrophoneStreamContext : NSObject {
@public
    HAPPlatformMicrophoneStreamRef stream;
    NSTimer* timer;
}
@end

@implementation MicrophoneStreamContext
- (instancetype)init {
    if (self = [super init]) {
        stream = nil;
        timer = nil;
    }
    return self;
}
@end

static struct {
    NSMutableArray<MicrophoneStreamContext*>* context;
} state = {
    .context = nil,
};

HAP_UNUSED
static NSTimer* GetTimerForStream(HAPPlatformMicrophoneStreamRef _Nonnull microphone_stream) {
    for (size_t i = 0; i < state.context.count; i++) {
        if (state.context[i] && state.context[i]->stream == microphone_stream) {
            return state.context[i]->timer;
        }
    }
    return nil;
}

static void RemoveStream(HAPPlatformMicrophoneStreamRef _Nonnull microphone_stream) {
    size_t i = 0;
    bool found = false;
    for (i = 0; i < state.context.count; i++) {
        if (state.context[i] && state.context[i]->stream == microphone_stream) {
            [state.context[i]->timer invalidate];
            found = true;
            break;
        }
    }
    if (found) {
        [state.context removeObjectAtIndex:i];
    }
}

/**
 * Starts Audio Source

 * @param      stream           associated microphone input stream
 */
static void StartAudioSource(HAPPlatformMicrophoneStream* stream) {
    HAPPrecondition(stream);
    HAPPrecondition(stream->microphone);

    const char* _Nullable mediaPath = kMediaFilePath;
#if (HAP_TESTING == 1)
    mediaPath = stream->microphone->mediaPath;
#endif

    if (mediaPath != NULL) {
        char audioFilePath[PATH_MAX];
        HAPError err = HAPStringWithFormat(audioFilePath, sizeof audioFilePath, "%s/aacdump.dmp", mediaPath);
        HAPAssert(!err);
        HAPLogDebug(&logObject, "%s: audio file used: %s", __func__, audioFilePath);

        if (stream->f) {
            fclose(stream->f);
            stream->f = NULL;
        }

        stream->f = fopen(audioFilePath, "rb");
        if (!stream->f) {
            int _errno = errno;
            HAPLogInfo(
                    &logObject,
                    "%s: Failed to open(%d) aac file %s, defaulting to file %s",
                    __func__,
                    _errno,
                    audioFilePath,
                    kMediaFilePath);

            mediaPath = kMediaFilePath;
            err = HAPStringWithFormat(audioFilePath, sizeof audioFilePath, "%s/aacdump.dmp", mediaPath);
            HAPAssert(!err);
            stream->f = fopen(audioFilePath, "rb");
            if (!stream->f) {
                _errno = errno;
                HAPLogError(&logObject, "%s: Failed to open(%d) aac file %s ", __func__, _errno, audioFilePath);
                HAPFatalError();
            }
        }

        char audioMetaFilePath[PATH_MAX];
        err = HAPStringWithFormat(audioMetaFilePath, sizeof audioMetaFilePath, "%s/aacmeta.txt", mediaPath);
        HAPAssert(!err);

        if (stream->fmeta) {
            fclose(stream->fmeta);
            stream->fmeta = NULL;
        }

        stream->fmeta = fopen(audioMetaFilePath, "r");
        if (!stream->fmeta) {
            int _errno = errno;
            HAPLogError(&logObject, "%s:Failed to open(%d) aac meta file %s", __func__, _errno, audioMetaFilePath);
            HAPFatalError();
        }

        stream->offset = 0;
        stream->ts = 0;
    }
}

/**
 * Stops Audio Source

 * @param      stream           associated microphone input stream
 */
void HAPPlatformMicrophoneStopAudioSource(HAPPlatformMicrophoneStream* stream) {
    HAPPrecondition(stream);

    if (stream->f) {
        fclose(stream->f);
        stream->f = NULL;
    }

    if (stream->fmeta) {
        fclose(stream->fmeta);
        stream->fmeta = NULL;
    }

    stream->offset = 0;
    stream->ts = 0;

    RemoveStream(stream);
}

/**
 * Receives Audio Frames from source and pushes to client
 *
 * @param      stream           associated microphone input stream
 */
static void ReceiveAudioFrames(HAPPlatformMicrophoneStream* stream) {
    HAPPrecondition(stream);

    if (stream->isSuspended || !stream->microphone || !stream->callback) {
        HAPLogInfo(&logObject, "Audio stream stopped");
        HAPPlatformMicrophoneStopAudioSource(stream);
        return;
    }

    // Only verify that the stream is valid, if the stream hasn't been stopped.
    HAPPrecondition(stream->f);
    HAPPrecondition(stream->fmeta);

    uint32_t num = 0;
    const uint8_t* ptr = (const uint8_t*) &num;
    uint8_t* start = NULL;

    size_t nextOffset = 0;
    fscanf(stream->fmeta, "%zu\n", &nextOffset);

    fseek(stream->f, stream->offset, SEEK_SET);
    if (fread((void*) ptr, 1, 1, stream->f) == 0) {
        stream->offset = 0;
        fseek(stream->f, stream->offset, SEEK_SET);
        fread((void*) ptr, sizeof(uint32_t), 1, stream->f);
        fseek(stream->fmeta, 0, SEEK_SET);
        fscanf(stream->fmeta, "%zu\n", &nextOffset);
    }

    HAPAssert(nextOffset >= stream->offset);
    size_t frameSize = nextOffset - stream->offset;
    start = (uint8_t*) malloc(frameSize);
    if (start == NULL) {
        return;
    }

    fread((void*) start, frameSize, 1, stream->f);

    stream->offset = nextOffset;

    if (stream->sampleRate) {
        uint64_t t = (1000000 / stream->sampleRate);
        t *= 1000;
        stream->ts += t;
    }

    HAPPlatformMicrophone* microphone = stream->microphone;

    stream->callback(stream->context, microphone, stream, start, frameSize, stream->ts, 0);

    free(start);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACELDStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate HAP_UNUSED,
        HAPAudioCodecBitRateControlMode bitRateMode HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACLCStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    // Find free microphone stream.
    *microphoneStream = NULL;
    for (size_t i = 0; i < microphone->numMicrophoneStreams; i++) {
        HAPPlatformMicrophoneStreamRef stream = &microphone->microphoneStreams[i];
        if (!stream->microphone) {
            HAPRawBufferZero(stream, sizeof *stream);
            stream->microphone = microphone;
            *microphoneStream = stream;
            break;
        }
    }
    if (!*microphoneStream) {
        HAPLogError(
                &logObject,
                "[%p] No additional concurrent AAC microphone streams may be started.",
                (const void*) microphone);
        return kHAPError_OutOfResources;
    }

    unsigned int rate = 16000;
    switch (sampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
            rate = 8000;
            break;
        case kHAPAudioCodecSampleRate_16KHz:
            rate = 16000;
            break;
        case kHAPAudioCodecSampleRate_24KHz:
            rate = 24000;
            break;
        case kHAPAudioCodecSampleRate_32KHz:
            rate = 32000;
            break;
        case kHAPAudioCodecSampleRate_44_1KHz:
            rate = 44100;
            break;
        case kHAPAudioCodecSampleRate_48KHz:
            rate = 48000;
            break;
    }
    size_t packetsPerSecond = rate / kAACSamplesPerPacket + (rate % kAACSamplesPerPacket != 0);

    (*microphoneStream)->sampleRate = packetsPerSecond;
    (*microphoneStream)->context = context;
    (*microphoneStream)->callback = callback;

#if (HAP_TESTING == 1)
    if ((*microphoneStream)->microphone->mediaPath != NULL) {
#endif
        StartAudioSource(*microphoneStream);

        if (state.context == nil) {
            state.context = [[NSMutableArray alloc] init];
        }

        MicrophoneStreamContext* streamContext = [[MicrophoneStreamContext alloc] init];
        streamContext->stream = *microphoneStream;
        streamContext->timer = [NSTimer scheduledTimerWithTimeInterval:((NSTimeInterval) 1) / packetsPerSecond
                                                               repeats:YES
                                                                 block:^(NSTimer* timer HAP_UNUSED) {
                                                                     dispatch_async(dispatch_get_main_queue(), ^{
                                                                         ReceiveAudioFrames(*microphoneStream);
                                                                     });
                                                                 }];
        [state.context addObject:streamContext];
#if (HAP_TESTING == 1)
    }
#endif

    return kHAPError_None;
}
#endif
