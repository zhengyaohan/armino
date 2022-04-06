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

#include "HAPPlatformMicrophone.h"

#include <errno.h>

#include "HAP.h"
#include "HAPPlatformMicrophone+Init.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE)

#import <CoreGraphics/CoreGraphics.h>
#import <CoreServices/CoreServices.h>
#import <Foundation/Foundation.h>
#import <Network/Network.h>
#include <pthread.h>

const char MAGIC[] = { 0X72, 0X61, 0x77, 0x76 };
#define AUDIO_DMP_NAME "PAL/Darwin/MediaFiles/audiodmp.buf"

static uint8_t* gb_audioBuf = NULL;
static long gl_offsetAudioBuf = 0;
static long gl_maxSize = 0;

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Microphone" };

/**
 * Opus raw sample rate [Hz].
 *
 * Independent of negotiated audio sample rate.
 */
#define kOpusRawSampleRate (24000)

/**
 * Maximum Opus packet time [ms].
 */
#define kMaxPacketTime (60)

/**
 * Maximum raw packet size [samples].
 */
#define kMaxPacketSize (kOpusRawSampleRate / 1000 * kMaxPacketTime)

/**
 * Maximum Opus packet size [bytes].
 */
#define kMaxOpusPacket (512)

void loadAudio(void) {
    FILE* file = fopen(AUDIO_DMP_NAME, "r");

    if (!file) {
        int _errno = errno;
        HAPLogError(&logObject, "Failed to open (%d) raw audio dmp file %s", _errno, AUDIO_DMP_NAME);
        return;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    gb_audioBuf = malloc(sizeof(uint8_t) * size);

    if (!gb_audioBuf) {
        HAPLogError(&logObject, "Failed allocate memory for audio buffer of %ld bytes", size);
        return;
    }

    size_t readIn = fread(gb_audioBuf, sizeof(uint8_t), size, file);
    gl_offsetAudioBuf = 0;
    gl_maxSize = readIn;

    HAPLogInfo(&logObject, "read in %zd bytes of %ld bytes of audio file", readIn, size);
    fclose(file);
}

static bool testReset(int size) {
    if ((gl_offsetAudioBuf + size) >= gl_maxSize) {
        HAPLogInfo(&logObject, "%s  ", __func__);
        gl_offsetAudioBuf = 0;
        return true;
    }

    return false;
}

void microphoneReplayAudio(uint8_t* inBuf, int inLen, int* outLen) {
    if (testReset(sizeof(MAGIC))) {
        microphoneReplayAudio(inBuf, inLen, outLen);
        return;
    }

    HAPLogInfo(&logObject, "%s validating magic", __func__);
    if (memcmp(gb_audioBuf + gl_offsetAudioBuf, MAGIC, sizeof(MAGIC)) != 0) {
        HAPLogError(&logObject, "%s failed to find audio MAGIC", __func__);
        HAPFatalError();
    }

    gl_offsetAudioBuf += sizeof(MAGIC);

    HAPLogInfo(&logObject, "%s checking time", __func__);
    if (testReset(sizeof(HAPTimeNS))) {
        microphoneReplayAudio(inBuf, inLen, outLen);
        return;
    }

    gl_offsetAudioBuf += sizeof(HAPTimeNS);

    if (testReset(sizeof(int))) {
        microphoneReplayAudio(inBuf, inLen, outLen);
        return;
    }

    // casting abuse
    int len = *(int*) (gb_audioBuf + gl_offsetAudioBuf);
    gl_offsetAudioBuf += sizeof(len);

    HAPLogInfo(&logObject, "%s length of audio buffer %d", __func__, len);

    if (testReset(len)) {
        microphoneReplayAudio(inBuf, inLen, outLen);
        return;
    }
    uint8_t* buf = gb_audioBuf + gl_offsetAudioBuf;

    gl_offsetAudioBuf += len;

    HAPPrecondition(len <= inLen);
    HAPPrecondition(outLen);
    HAPPrecondition(buf);

    HAPRawBufferCopyBytes(inBuf, buf, len);
    *outLen = len;
}

void HAPPlatformMicrophoneCreate(HAPPlatformMicrophoneRef microphone, const HAPPlatformMicrophoneOptions* options) {
    HAPPrecondition(microphone);
    HAPPrecondition(options);
    HAPPrecondition(options->audioInputDevice);
    HAPPrecondition(options->microphoneStreams);

    HAPLogInfo(&logObject, "%s", __func__);

    for (size_t i = 0; i < options->numMicrophoneStreams; i++) {
        HAPPlatformMicrophoneStreamRef microphoneStream = &options->microphoneStreams[i];
        HAPRawBufferZero(microphoneStream, sizeof *microphoneStream);
    }

    HAPRawBufferZero(microphone, sizeof *microphone);
    microphone->audioInputDevice = strdup(options->audioInputDevice);
    microphone->microphoneStreams = options->microphoneStreams;
    microphone->numMicrophoneStreams = options->numMicrophoneStreams;

#if (HAP_TESTING == 1)
    if (options->disable) {
        microphone->mediaPath = NULL;
    } else {
        microphone->mediaPath = options->mediaPath;
        if (options->mediaPath == NULL) {
            microphone->mediaPath = kMediaFilePath;
        }
    }
#endif
}

void HAPPlatformMicrophoneRelease(HAPPlatformMicrophoneRef microphone) {
    HAPPrecondition(microphone);

    HAPLogInfo(&logObject, "%s", __func__);

    for (size_t i = 0; i < microphone->numMicrophoneStreams; i++) {
        HAPPlatformMicrophoneStreamRef microphoneStream = &microphone->microphoneStreams[i];
        if (microphoneStream->microphone) {
            HAPLogError(
                    &logObject,
                    "Microphone stream %p not stopped before releasing microphone.",
                    (const void*) microphoneStream);
            HAPFatalError();
        }
        HAPRawBufferZero(microphoneStream, sizeof *microphoneStream);
    }
    if (microphone->audioInputDevice) {
        free(microphone->audioInputDevice);
    }

    HAPRawBufferZero(microphone, sizeof *microphone);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartOpusStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate HAP_UNUSED,
        HAPAudioCodecBitRateControlMode bitRateMode HAP_UNUSED,
        uint32_t bitrate HAP_UNUSED,
        uint32_t packetTime HAP_UNUSED,
        HAPPlatformMicrophoneDataCallback callback HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);

    HAPLog(&logObject, "%s", __func__);

    // Find free microphone stream
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
                "[%p] No additional concurrent microphone streams may be started.",
                (const void*) microphone);
        return kHAPError_OutOfResources;
    }

    loadAudio();
    return kHAPError_None;
}

void HAPPlatformMicrophoneStopStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);

    HAPLog(&logObject, "%s", __func__);

    if (microphoneStream->microphone) {
        // Prevent from delivering more data
        microphoneStream->isSuspended = true;
        microphoneStream->callback = NULL;
        microphoneStream->microphone = NULL;

        // Stop the audio source, invalidates timers, etc.
        HAPPlatformMicrophoneStopAudioSource(microphoneStream);
    }
}

void HAPPlatformMicrophoneSuspendStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(microphoneStream->microphone == microphone);

    HAPLog(&logObject, "%s", __func__);
}

void HAPPlatformMicrophoneResumeStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(microphoneStream->microphone == microphone);

    HAPLog(&logObject, "%s", __func__);
}

void HAPPlatformMicrophoneSetVolume(HAPPlatformMicrophoneRef microphone, unsigned int volume) {
    HAPPrecondition(microphone);
    HAPPrecondition(volume <= 100);

    HAPLog(&logObject, "%s", __func__);
    microphone->volume = volume;
}

void HAPPlatformMicrophoneSetMuted(HAPPlatformMicrophoneRef microphone, bool muted) {
    HAPPrecondition(microphone);

    HAPLog(&logObject, "%s", __func__);
    microphone->muted = muted;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformMicrophoneIsStreamRunning(
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const char* _Nullable* _Nonnull stateString) {
    HAPPrecondition(microphoneStream);
    HAPPrecondition(stateString);

    *stateString = "Unknown";

    return (microphoneStream->microphone != NULL);
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE)
