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

#include "HAPPlatformSpeaker+Init.h"
#include "HAPPlatformSpeaker.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_SPEAKER)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Speaker" };

/**
 * Opus RAW Samplerate [Hz].
 *
 * Independent of negotiated audio sample rate.
 */
#define kOpusRawSampleRate (24000)

/**
 * Maximum Opus packet time [ms].
 */
#define kMaxPacketTime (60)

/**
 * Length of ALSA jitter buffer [ms].
 */
#define kAlsaBufferTime (300)

/**
 * Length of ALSA jitter buffer [samples].
 */
#define kAlsaBufferSize (kOpusRawSampleRate / 1000 * kAlsaBufferTime)

/**
 * Maximum raw packet size [samples].
 */
#define kMaxPacketSize (kOpusRawSampleRate / 1000 * kMaxPacketTime)

/**
 * Maximum PCM buffer size [samples].
 */
#define kMaxPcmSize (kMaxPacketSize + 4)

void HAPPlatformSpeakerCreate(HAPPlatformSpeakerRef speaker, const HAPPlatformSpeakerOptions* options) {
    HAPPrecondition(speaker);
    HAPPrecondition(options);
    HAPPrecondition(options->audioOutputDevice);
    HAPPrecondition(options->speakerStreams);

    for (size_t i = 0; i < options->numSpeakerStreams; i++) {
        HAPPlatformSpeakerStreamRef speakerStream = &options->speakerStreams[i];
        HAPRawBufferZero(speakerStream, sizeof *speakerStream);
    }

    HAPRawBufferZero(speaker, sizeof *speaker);
    speaker->audioOutputDevice = options->audioOutputDevice;
    speaker->speakerStreams = options->speakerStreams;
    speaker->numSpeakerStreams = options->numSpeakerStreams;
}

void HAPPlatformSpeakerRelease(HAPPlatformSpeakerRef speaker) {
    HAPPrecondition(speaker);

    for (size_t i = 0; i < speaker->numSpeakerStreams; i++) {
        HAPPlatformSpeakerStreamRef speakerStream = &speaker->speakerStreams[i];
        if (speakerStream->speaker) {
            HAPLogError(
                    &logObject, "Speaker stream %p not stopped before releasing speaker.", (const void*) speakerStream);
            HAPFatalError();
        }
        HAPRawBufferZero(speakerStream, sizeof *speakerStream);
    }

    HAPRawBufferZero(speaker, sizeof *speaker);

    HAPLog(&logObject, "%s", __func__);
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformSpeakerStartOpusStream(
        HAPPlatformSpeakerRef speaker,
        HAPPlatformSpeakerStreamRef _Nullable* _Nonnull speakerStream,
        HAPAudioCodecSampleRate sampleRate HAP_UNUSED,
        uint32_t packetTime) {
    HAPPrecondition(speaker);
    HAPPrecondition(speaker->audioOutputDevice);
    HAPPrecondition(speakerStream);
    HAPPrecondition(packetTime <= kMaxPacketTime);

    // Find free speaker stream.
    *speakerStream = NULL;
    for (size_t i = 0; i < speaker->numSpeakerStreams; i++) {
        HAPPlatformSpeakerStreamRef stream = &speaker->speakerStreams[i];
        if (!stream->speaker) {
            HAPRawBufferZero(stream, sizeof *stream);
            stream->speaker = speaker;
            *speakerStream = stream;
            break;
        }
    }
    if (!*speakerStream) {
        HAPLogError(&logObject, "[%p] No additional concurrent speaker streams may be started.", (const void*) speaker);
        return kHAPError_OutOfResources;
    }

    HAPLog(&logObject, "%s", __func__);
    HAPFatalError();

    return kHAPError_None;
}

void HAPPlatformSpeakerStopStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);

    HAPLog(&logObject, "%s", __func__);
    HAPFatalError();
}

void HAPPlatformSpeakerSuspendStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
    HAPPrecondition(speakerStream->speaker == speaker);

    HAPLog(&logObject, "%s", __func__);
    HAPFatalError();
}

void HAPPlatformSpeakerResumeStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
}

void HAPPlatformSpeakerPushData(
        HAPPlatformSpeakerRef speaker,
        HAPPlatformSpeakerStreamRef speakerStream,
        const void* bytes,
        size_t numBytes HAP_UNUSED,
        HAPTimeNS sampleTime HAP_UNUSED) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
    HAPPrecondition(bytes);

    HAPLog(&logObject, "%s", __func__);
    HAPFatalError();
}

void HAPPlatformSpeakerSetVolume(HAPPlatformSpeakerRef speaker, unsigned int volume) {
    HAPPrecondition(speaker);
    HAPPrecondition(volume <= 100);

    speaker->volume = volume;
    HAPLog(&logObject, "%s", __func__);
}

void HAPPlatformSpeakerSetMuted(HAPPlatformSpeakerRef speaker, bool muted) {
    HAPPrecondition(speaker);

    speaker->muted = muted;
    HAPLog(&logObject, "%s", __func__);
}

#endif
