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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE)

#include <math.h> // sqrtf
#include <opus/opus.h>
#include <string.h>

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

/**
 * Opus audio input polling thread.
 */
static void* _Nullable AudioOpusInputThread(void* ptr) {
    HAPPrecondition(ptr);
    HAPPlatformMicrophoneStreamRef microphoneStream = ptr;
    snd_pcm_t* pcmHandle = microphoneStream->pcmHandle;
    HAPPrecondition(pcmHandle);

    opus_int16 pcm[kMaxPacketSize];
    uint8_t opus[kMaxOpusPacket];
    OpusEncoder* opusEncoder = (OpusEncoder*) microphoneStream->encoder;
    HAPPrecondition(opusEncoder);
    uint32_t samplesPerPacket = microphoneStream->packetTime * (kOpusRawSampleRate / 1000);
    HAPAssert(samplesPerPacket <= kMaxPacketSize);
    HAPTimeNS packetTimeNS = (HAPTimeNS) microphoneStream->packetTime * 1000000ll;
    uint32_t packetCount = 0;

    for (;;) {
        // Read next PCM packet from ALSA.
        snd_pcm_sframes_t num;
        num = snd_pcm_readi(pcmHandle, pcm, samplesPerPacket);
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        HAPPlatformMicrophoneDataCallback callback = microphoneStream->callback;
        HAPPlatformMicrophone* microphone = microphoneStream->microphone;
        if (callback == NULL || microphone == NULL) {
            break; // End of stream.
        }
        if (num < 0) {
            HAPLogError(&logObject, "Microphone input read failed: %s", snd_strerror(num));
            snd_pcm_prepare(pcmHandle); // Recover from error.
            continue;
        }
        if ((uint32_t) num != samplesPerPacket) {
            // Partial packet at stream end.
            break;
        }
        packetCount++;

        if (!microphoneStream->isSuspended) {
            // Apply volume and get RMS.
            int volume = microphone->muted ? 0 : (int) (microphone->volume * 167773 >> 8); // * 2^16 / 100
            float rms = 0.0F;
            for (int i = 0; i < num; i++) {
                int32_t sample = pcm[i];
                sample = sample * volume >> 16;
                pcm[i] = (opus_int16) sample;
                rms += (float) (sample * sample);
            }
            rms /= (float) num;  // Mean(x^2).
            rms = sqrtf(rms);    // Sqrt(mean(x^2)).
            rms *= 1 / 32768.0F; // Normalize.

            // Get ns timestamp.
            HAPTimeNS ts = packetTimeNS * packetCount;

            // Opus encoder.
            opus_int32 length;
            length = opus_encode(opusEncoder, pcm, (int) num, opus, (opus_int32) sizeof opus);
            HAPAssert(length > 0);

            // Use callback.
            callback(microphoneStream->context, microphone, microphoneStream, opus, length, ts, rms);
        }
    }

    // Close Opus encoder.
    opus_encoder_destroy(opusEncoder);
    microphoneStream->encoder = NULL;

    return NULL;
}

void HAPPlatformMicrophoneCreate(HAPPlatformMicrophoneRef microphone, const HAPPlatformMicrophoneOptions* options) {
    HAPPrecondition(microphone);
    HAPPrecondition(options);
    HAPPrecondition(options->audioInputDevice);
    HAPPrecondition(options->microphoneStreams);

    for (size_t i = 0; i < options->numMicrophoneStreams; i++) {
        HAPPlatformMicrophoneStreamRef microphoneStream = &options->microphoneStreams[i];
        HAPRawBufferZero(microphoneStream, sizeof *microphoneStream);
    }

    HAPRawBufferZero(microphone, sizeof *microphone);
    microphone->audioInputDevice = strdup(options->audioInputDevice);
    microphone->microphoneStreams = options->microphoneStreams;
    microphone->numMicrophoneStreams = options->numMicrophoneStreams;
}

void HAPPlatformMicrophoneRelease(HAPPlatformMicrophoneRef microphone) {
    HAPPrecondition(microphone);

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
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        uint32_t packetTime,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    HAPLog(&logObject, "ALSA version: %s, Opus version: %s", snd_asoundlib_version(), opus_get_version_string());

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
                "[%p] No additional concurrent microphone streams may be started.",
                (const void*) microphone);
        return kHAPError_OutOfResources;
    }

    // Open ALSA input.
    snd_pcm_t* pcmHandle;
    int err;
    err = snd_pcm_open(&pcmHandle, microphone->audioInputDevice, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        HAPLogError(&logObject, "Microphone input open failed: %s", snd_strerror(err));
        return kHAPError_Unknown;
    }

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
    err = snd_pcm_hw_params(pcmHandle, hwparams);
    HAPAssert(err == 0);
    snd_pcm_hw_params_free(hwparams);

    opus_int32 bandwidth = OPUS_BANDWIDTH_WIDEBAND;
    switch (sampleRate) {
        case kHAPAudioCodecSampleRate_8KHz:
            bandwidth = OPUS_BANDWIDTH_NARROWBAND;
            break;
        case kHAPAudioCodecSampleRate_16KHz:
            bandwidth = OPUS_BANDWIDTH_WIDEBAND;
            break;
        case kHAPAudioCodecSampleRate_24KHz:
            bandwidth = OPUS_BANDWIDTH_SUPERWIDEBAND;
            break;
    }

    // Initialize Opus encoder.
    OpusEncoder* opusEncoder;
    opusEncoder = opus_encoder_create(kOpusRawSampleRate, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        HAPLogError(&logObject, "Opus encoder creation failed: %d", err);
        return kHAPError_Unknown;
    }
    err = opus_encoder_ctl(opusEncoder, OPUS_SET_VBR(bitRateMode == kHAPAudioCodecBitRateControlMode_Variable));
    HAPAssert(err == OPUS_OK);
    err = opus_encoder_ctl(opusEncoder, OPUS_SET_BITRATE(bitrate));
    HAPAssert(err == OPUS_OK);
    err = opus_encoder_ctl(opusEncoder, OPUS_SET_MAX_BANDWIDTH(bandwidth));
    HAPAssert(err == OPUS_OK);
    err = opus_encoder_ctl(opusEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    HAPAssert(err == OPUS_OK);

    (*microphoneStream)->pcmHandle = pcmHandle;
    (*microphoneStream)->encoder = opusEncoder;
    (*microphoneStream)->packetTime = packetTime;
    (*microphoneStream)->context = context;
    (*microphoneStream)->callback = callback;

    // Start Thread.
    err = pthread_create(&(*microphoneStream)->thread, NULL, AudioOpusInputThread, *microphoneStream);
    HAPAssert(err == 0);

    return kHAPError_None;
}

void HAPPlatformMicrophoneStopStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);

    if (microphoneStream->microphone) {
        // Prevent from delivering more data and trigger ALSA close.
        microphoneStream->isSuspended = true;
        microphoneStream->callback = NULL;
        microphoneStream->microphone = NULL;

        int e = pthread_join(microphoneStream->thread, NULL);
        if (e) {
            HAPLogError(&logObject, "`pthread_join` failed to join audio stream thread (%d).", e);
        }

        // Close ALSA input.
        if (microphoneStream && microphoneStream->pcmHandle) {
            int err = snd_pcm_close(microphoneStream->pcmHandle);
            HAPAssert(err == 0);
            microphoneStream->pcmHandle = NULL;
        }
    }
}

void HAPPlatformMicrophoneSuspendStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(microphoneStream->microphone == microphone);

    microphoneStream->isSuspended = true;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void HAPPlatformMicrophoneResumeStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(microphoneStream->microphone == microphone);

    microphoneStream->isSuspended = false;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

void HAPPlatformMicrophoneSetVolume(HAPPlatformMicrophoneRef microphone, unsigned int volume) {
    HAPPrecondition(microphone);
    HAPPrecondition(volume <= 100);

    microphone->volume = volume;
}

void HAPPlatformMicrophoneSetMuted(HAPPlatformMicrophoneRef microphone, bool muted) {
    HAPPrecondition(microphone);

    microphone->muted = muted;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformMicrophoneIsStreamRunning(
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const char* _Nullable* _Nonnull stateString) {
    HAPPrecondition(microphoneStream);
    HAPPrecondition(stateString);

    *stateString = "Unknown";

    if (microphoneStream->pcmHandle) {
        snd_pcm_state_t state = snd_pcm_state(microphoneStream->pcmHandle);
        switch (state) {
            case SND_PCM_STATE_OPEN: {
                *stateString = "Open";
                break;
            }
            case SND_PCM_STATE_SETUP: {
                *stateString = "Setup";
                break;
            }
            case SND_PCM_STATE_PREPARED: {
                *stateString = "Prepared";
                break;
            }
            case SND_PCM_STATE_RUNNING: {
                *stateString = "Running";
                break;
            }
            case SND_PCM_STATE_XRUN: {
                *stateString = "Overrun";
                break;
            }
            case SND_PCM_STATE_DRAINING: {
                *stateString = "Draining";
                break;
            }
            case SND_PCM_STATE_PAUSED: {
                *stateString = "Paused";
                break;
            }
            case SND_PCM_STATE_SUSPENDED: {
                *stateString = "Suspended";
                break;
            }
            case SND_PCM_STATE_DISCONNECTED: {
                *stateString = "Disconnected";
                break;
            }
            default: {
                // Note that SND_PCM_STATE_PRIVATE1 was dropped on purpose because not all versions of ALSA library
                // defines the enumeration and the state isn't expected anyways.
                break;
            }
        }
        if (state == SND_PCM_STATE_RUNNING) {
            return true;
        }
    }
    return false;
}

#endif
