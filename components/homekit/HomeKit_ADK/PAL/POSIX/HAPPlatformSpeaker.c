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

#include <opus/opus.h>

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

    // Open ALSA output.
    snd_pcm_t* pcmHandle;
    int err;
    err = snd_pcm_open(&pcmHandle, speaker->audioOutputDevice, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        HAPLogError(&logObject, "Speaker output open failed: %s", snd_strerror(err));
        return kHAPError_Unknown;
    }

    // Buffer parameters in samples.
    snd_pcm_uframes_t packetSize = kOpusRawSampleRate / 1000 * packetTime;
    snd_pcm_uframes_t periodeSize = packetSize;
    snd_pcm_uframes_t bufferSize = kAlsaBufferSize;

    // Configure ALSA hardware parameters.
    snd_pcm_hw_params_t* hwparams;
    snd_pcm_hw_params_malloc(&hwparams);
    err = snd_pcm_hw_params_any(pcmHandle, hwparams);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_access(pcmHandle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_format(pcmHandle, hwparams, SND_PCM_FORMAT_S16);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_channels(pcmHandle, hwparams, 1);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_rate(pcmHandle, hwparams, kOpusRawSampleRate, 0);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_buffer_size_near(pcmHandle, hwparams, &bufferSize);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params_set_period_size_near(pcmHandle, hwparams, &periodeSize, NULL);
    HAPAssert(err == 0);
    err = snd_pcm_hw_params(pcmHandle, hwparams);
    HAPAssert(err == 0);
    snd_pcm_hw_params_free(hwparams);

    // Initialize Opus decoder.
    OpusDecoder* opusDecoder;
    opusDecoder = opus_decoder_create(kOpusRawSampleRate, 1, &err);
    if (err != OPUS_OK) {
        HAPLogError(&logObject, "Opus decoder creation failed: %d", err);
        return kHAPError_Unknown;
    }

    (*speakerStream)->pcmHandle = pcmHandle;
    (*speakerStream)->decoder = opusDecoder;
    (*speakerStream)->packetTime = packetTime;
    (*speakerStream)->quaterBufferSize = bufferSize / 4;
    (*speakerStream)->lastTimestamp = UINT64_MAX;

    return kHAPError_None;
}

/**
 * Start or restart the ALSA buffer.
 *
 * @param      pcmHandle            ALSA PCM handle.
 * @param      startThreshold       Start threshold to use [samples].
 */
static void PrepareAlsaBuffer(snd_pcm_t* pcmHandle, snd_pcm_uframes_t startThreshold) {
    HAPPrecondition(pcmHandle);
    int err;

    // Configure start threshold in ALSA software parameters.
    snd_pcm_sw_params_t* swparams;
    snd_pcm_sw_params_malloc(&swparams);
    err = snd_pcm_sw_params_current(pcmHandle, swparams);
    HAPAssert(err == 0);
    err = snd_pcm_sw_params_set_start_threshold(pcmHandle, swparams, startThreshold);
    HAPAssert(err == 0);
    err = snd_pcm_sw_params(pcmHandle, swparams);
    HAPAssert(err == 0);
    snd_pcm_sw_params_free(swparams);

    err = snd_pcm_prepare(pcmHandle);
    HAPAssert(err == 0);
}

void HAPPlatformSpeakerStopStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);

    if (speakerStream->decoder) {
        opus_decoder_destroy((OpusDecoder*) speakerStream->decoder);
        speakerStream->decoder = NULL;
    }
    if (speakerStream->pcmHandle) {
        int err = snd_pcm_close(speakerStream->pcmHandle);
        HAPAssert(err == 0);
        speakerStream->pcmHandle = NULL;
    }

    speakerStream->speaker = NULL;
}

void HAPPlatformSpeakerSuspendStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
    HAPPrecondition(speakerStream->speaker == speaker);

    speakerStream->isSuspended = true;
}

void HAPPlatformSpeakerResumeStream(HAPPlatformSpeakerRef speaker, HAPPlatformSpeakerStreamRef speakerStream) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
    HAPPrecondition(speakerStream->decoder);
    HAPPrecondition(speakerStream->speaker == speaker);

    // Restart Opus decoder
    int err = opus_decoder_init((OpusDecoder*) speakerStream->decoder, kOpusRawSampleRate, 1);
    HAPAssert(err == OPUS_OK);

    speakerStream->lastTimestamp = UINT64_MAX;
    speakerStream->isSuspended = false;
}

/**
 * Decode an Opus packet and forward to ALSA.
 *
 * @param      speakerStream        Speaker stream reference.
 * @param      opusDecoder          Opus decoder state.
 * @param      pcmHandle            ALSA PCM handle.
 * @param      volume               Volume scaling value.
 * @param      bytes                Opus data.
 * @param      numBytes             Length of Opus data.
 */
static void DecodeOpusData(
        HAPPlatformSpeakerStreamRef speakerStream,
        OpusDecoder* opusDecoder,
        snd_pcm_t* pcmHandle,
        int32_t volume,
        const void* bytes,
        size_t numBytes) {
    HAPPrecondition(speakerStream);
    HAPPrecondition(opusDecoder);
    HAPPrecondition(pcmHandle);

    opus_int16 pcm[kMaxPcmSize];

    // Decode Opus packet.
    int frameSize = kOpusRawSampleRate / 1000 * speakerStream->packetTime;
    HAPAssert(frameSize < kMaxPcmSize);
    int num = opus_decode(opusDecoder, bytes, (opus_int32) numBytes, pcm, frameSize, 0);
    if (num < 0) {
        HAPLogError(&logObject, "Speaker Opus decode failed: %s", opus_strerror(num));
        return;
    }

    // Apply volume.
    for (int i = 0; i < num; i++) {
        int32_t sample = pcm[i];
        sample = sample * volume >> 16;
        pcm[i] = (int16_t) sample;
    }

    // Apply clock skew compensation if needed.
    snd_pcm_sframes_t free = snd_pcm_avail(pcmHandle);
    if (free < 0) {
        // Buffer error handled below.
    } else if ((snd_pcm_uframes_t) free < speakerStream->quaterBufferSize) {
        // Drop last sample.
        num--;
    } else if ((snd_pcm_uframes_t) free > speakerStream->quaterBufferSize * 3) {
        // Double last sample.
        pcm[num] = pcm[num - 1];
        num++;
    }

    // Write speaker output.
    num = snd_pcm_writei(pcmHandle, pcm, num);
    if (num < 0 && num != -EINTR) {
        HAPAssert(num == -EPIPE);
        // Recover from buffer underrun.
        // Use a smaller threshold to avoid buffer overflow due to late arriving packets.
        PrepareAlsaBuffer(pcmHandle, speakerStream->quaterBufferSize);
        HAPLogDebug(&logObject, "Recovered from speaker Alsa buffer underrun.");
    }
}

void HAPPlatformSpeakerPushData(
        HAPPlatformSpeakerRef speaker,
        HAPPlatformSpeakerStreamRef speakerStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime) {
    HAPPrecondition(speaker);
    HAPPrecondition(speakerStream);
    HAPPrecondition(bytes);

    OpusDecoder* opusDecoder = (OpusDecoder*) speakerStream->decoder;
    snd_pcm_t* pcmHandle = speakerStream->pcmHandle;

    if (speakerStream->isSuspended || !opusDecoder || !pcmHandle) {
        // Ignore data.
        return;
    }

    int32_t volume = speaker->muted ? 0 : (int) (speaker->volume * 167773 >> 8); // * 2^16 / 100

    if (speakerStream->lastTimestamp == UINT64_MAX) {
        // Start stream.
        PrepareAlsaBuffer(pcmHandle, speakerStream->quaterBufferSize * 2);
    } else {
        HAPTimeNS timeDiff = sampleTime - speakerStream->lastTimestamp;
        if (timeDiff > speakerStream->packetTime * 3500000) {
            // Restart stream after long gap.
            int err = snd_pcm_drop(pcmHandle);
            HAPAssert(err == 0);
            PrepareAlsaBuffer(pcmHandle, speakerStream->quaterBufferSize * 2);
            // Restart Opus decoder
            err = opus_decoder_init(opusDecoder, kOpusRawSampleRate, 1);
            HAPAssert(err == OPUS_OK);
        } else if (timeDiff > speakerStream->packetTime * 1500000) {
            // Generate dummy data for one or two missing packets.
            DecodeOpusData(speakerStream, opusDecoder, pcmHandle, volume, NULL, numBytes);
            if (timeDiff > speakerStream->packetTime * 2500000) {
                DecodeOpusData(speakerStream, opusDecoder, pcmHandle, volume, NULL, numBytes);
            }
        }
    }
    speakerStream->lastTimestamp = sampleTime;

    DecodeOpusData(speakerStream, opusDecoder, pcmHandle, volume, bytes, numBytes);
}

void HAPPlatformSpeakerSetVolume(HAPPlatformSpeakerRef speaker, unsigned int volume) {
    HAPPrecondition(speaker);
    HAPPrecondition(volume <= 100);

    speaker->volume = volume;
}

void HAPPlatformSpeakerSetMuted(HAPPlatformSpeakerRef speaker, bool muted) {
    HAPPrecondition(speaker);

    speaker->muted = muted;
}

#endif
