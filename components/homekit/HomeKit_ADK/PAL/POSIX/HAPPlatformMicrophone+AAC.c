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

#include "HAP.h"

#include "HAPPlatform.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA) && HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE)

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wunused-function")
HAP_DIAGNOSTIC_IGNORED_GCC("-Wunused-function")
#include <fdk-aac/aacenc_lib.h> // libfdk-aac
HAP_DIAGNOSTIC_POP
#include <math.h> // sqrtf

#include "HAPPlatformMicrophone+Init.h"
#include "HAPPlatformMicrophone.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Microphone" };

/**
 * AAC samples per packet.
 */
#define kAACLCSamplesPerPacket  (1024)
#define kAACELDSamplesPerPacket (480)

/**
 * Maximum AAC packet size.
 */
#define kMaxAACPacket (2048)

/**
 * AAC audio input polling thread.
 */
static void* _Nullable AudioAACInputThread(void* ptr) {
    HAPPrecondition(ptr);
    HAPPlatformMicrophoneStreamRef microphoneStream = ptr;
    snd_pcm_t* pcmHandle = microphoneStream->pcmHandle;
    HAPPrecondition(pcmHandle);

    int16_t pcm[kAACLCSamplesPerPacket];
    uint8_t aac[kMaxAACPacket];
    HANDLE_AACENCODER aacEncoder = (HANDLE_AACENCODER) microphoneStream->encoder;
    INT samplesPerPacket = aacEncoder_GetParam(aacEncoder, AACENC_GRANULE_LENGTH);
    INT_PCM* inBufList[] = { pcm };
    INT inBufIds[] = { IN_AUDIO_DATA };
    INT inBufSize[] = { sizeof pcm };
    INT inBufElSize[] = { sizeof(INT_PCM) };
    AACENC_BufDesc inBufDesc = { .numBufs = 1,
                                 .bufs = (void**) &inBufList,
                                 .bufferIdentifiers = inBufIds,
                                 .bufSizes = inBufSize,
                                 .bufElSizes = inBufElSize };
    UCHAR* outBufList[] = { aac };
    INT outBufIds[] = { OUT_BITSTREAM_DATA };
    INT outBufSize[] = { sizeof aac };
    INT outBufElSize[] = { sizeof(UCHAR) };
    AACENC_BufDesc outBufDesc = { .numBufs = 1,
                                  .bufs = (void**) &outBufList,
                                  .bufferIdentifiers = outBufIds,
                                  .bufSizes = outBufSize,
                                  .bufElSizes = outBufElSize };
    AACENC_InArgs inArgs = { .numInSamples = samplesPerPacket, .numAncBytes = 0 };
    AACENC_OutArgs outArgs;
    AACENC_ERROR err;
    uint32_t sampleRate = microphoneStream->sampleRate;
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
        if (num != samplesPerPacket) {
            // Partial packet at stream end.
            break;
        }
        packetCount++;

        // Apply volume and get RMS.
        int volume = microphone->muted ? 0 : (int) (microphone->volume * 167773 >> 8); // * 2^16 / 100
        float rms = 0.0F;
        for (int i = 0; i < num; i++) {
            int32_t sample = pcm[i];
            sample = sample * volume >> 16;
            pcm[i] = (int16_t) sample;
            rms += (float) (sample * sample);
        }
        rms /= (float) num;  // Mean(x^2).
        rms = sqrtf(rms);    // Sqrt(mean(x^2)).
        rms *= 1 / 32768.0F; // Normalize.

        // Get ns timestamp.
        HAPTimeNS ts = ((HAPTimeNS) samplesPerPacket * 1000000000ll) * packetCount / sampleRate;

        // AAC encoder.
        int length;
        HAPAssert(num == samplesPerPacket);
        err = aacEncEncode(aacEncoder, &inBufDesc, &outBufDesc, &inArgs, &outArgs);
        HAPAssert(err == AACENC_OK);
        HAPAssert(outArgs.numInSamples == samplesPerPacket);
        length = outArgs.numOutBytes;

        if (length > 0) {
            // Use callback.
            callback(microphoneStream->context, microphone, microphoneStream, aac, (size_t) length, ts, rms);
        }
    }

    // Close AAC encoder.
    err = aacEncClose(&aacEncoder);
    HAPAssert(err == AACENC_OK);
    microphoneStream->encoder = NULL;

    return NULL;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACLCStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    AACENC_ERROR er;

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

    // Open ALSA input.
    snd_pcm_t* pcmHandle;
    int err;
    err = snd_pcm_open(&pcmHandle, microphone->audioInputDevice, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        HAPLogError(&logObject, "Microphone input open failed: %s", snd_strerror(err));
        return kHAPError_Unknown;
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

    // Configure ALSA parameters.
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
    err = snd_pcm_hw_params_set_rate(pcmHandle, hwparams, rate, 0);
    if (err != 0) {
        HAPLogError(&logObject, "Sample rate %u not supported by microphone input", rate);
        return kHAPError_Unknown;
    }
    err = snd_pcm_hw_params(pcmHandle, hwparams);
    HAPAssert(err == 0);
    snd_pcm_hw_params_free(hwparams);

    // Initialize AAC encoder.
    HANDLE_AACENCODER aacEncoder;
    er = aacEncOpen(&aacEncoder, /* AAC, SBR */ 0x03, /* channels */ 1);
    if (er != AACENC_OK) {
        HAPLogError(&logObject, "AAC encoder creation failed: %d", err);
        return kHAPError_Unknown;
    }

    UINT vbr = 0;
    if (bitRateMode == kHAPAudioCodecBitRateControlMode_Variable) {
        if (bitrate <= 48000)
            vbr = 1;
        else if (bitrate <= 56000)
            vbr = 2;
        else if (bitrate <= 64000)
            vbr = 3;
        else if (bitrate <= 80000)
            vbr = 4;
        else
            vbr = 5;
    }

    er = aacEncoder_SetParam(aacEncoder, AACENC_AOT, AOT_AAC_LC);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_TRANSMUX, TT_MP4_RAW); // Raw output.
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_CHANNELMODE, MODE_1); // mono
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_GRANULE_LENGTH, kAACLCSamplesPerPacket);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_SAMPLERATE, rate);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_BITRATEMODE, vbr);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_BITRATE, bitrate);
    HAPAssert(er == AACENC_OK);
    // er = aacEncoder_SetParam(aacEncoder, AACENC_PEAK_BITRATE, bitrate); // ???
    // HAPAssert(er == AACENC_OK);

    er = aacEncEncode(aacEncoder, NULL, NULL, NULL, NULL);
    if (er != AACENC_OK) {
        HAPLogError(&logObject, "aacEncEncode(NULL) error: 0x%04X", (int) er);
    }
    HAPAssert(er == AACENC_OK);

    AACENC_InfoStruct encInfo;
    er = aacEncInfo(aacEncoder, &encInfo);
    HAPAssert(er == AACENC_OK);
    HAPAssert(encInfo.frameLength == kAACLCSamplesPerPacket);

    (*microphoneStream)->pcmHandle = pcmHandle;
    (*microphoneStream)->encoder = aacEncoder;
    (*microphoneStream)->sampleRate = rate;
    (*microphoneStream)->context = context;
    (*microphoneStream)->callback = callback;

    // Start Thread.
    err = pthread_create(&(*microphoneStream)->thread, NULL, AudioAACInputThread, *microphoneStream);
    HAPAssert(err == 0);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACELDStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context) {
    HAPPrecondition(microphone);
    HAPPrecondition(microphone->audioInputDevice);
    HAPPrecondition(microphoneStream);
    HAPPrecondition(callback);

    AACENC_ERROR er;

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

    // Open ALSA input.
    snd_pcm_t* pcmHandle;
    int err;
    err = snd_pcm_open(&pcmHandle, microphone->audioInputDevice, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        HAPLogError(&logObject, "Microphone input open failed: %s", snd_strerror(err));
        return kHAPError_Unknown;
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

    // Configure ALSA parameters.
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
    err = snd_pcm_hw_params_set_rate(pcmHandle, hwparams, rate, 0);
    if (err != 0) {
        HAPLogError(&logObject, "Sample rate %u not supported by microphone input", rate);
        return kHAPError_Unknown;
    }
    err = snd_pcm_hw_params(pcmHandle, hwparams);
    HAPAssert(err == 0);
    snd_pcm_hw_params_free(hwparams);

    // Initialize AAC encoder.
    HANDLE_AACENCODER aacEncoder;
    er = aacEncOpen(&aacEncoder, /* AAC */ 0x03, /* channels */ 1);
    if (er != AACENC_OK) {
        HAPLogError(&logObject, "AAC encoder creation failed: %d", err);
        return kHAPError_Unknown;
    }

    UINT vbr = 0;
    if (bitRateMode == kHAPAudioCodecBitRateControlMode_Variable) {
        if (bitrate <= 56000)
            vbr = 1;
        else if (bitrate <= 64000)
            vbr = 2;
        else if (bitrate <= 72000)
            vbr = 3;
        else if (bitrate <= 88000)
            vbr = 4;
        else
            vbr = 5;
    }

    er = aacEncoder_SetParam(aacEncoder, AACENC_AOT, AOT_ER_AAC_ELD);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_TRANSMUX, TT_MP4_RAW); // Raw output.
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_CHANNELMODE, MODE_1); // mono
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_GRANULE_LENGTH, kAACELDSamplesPerPacket);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_SAMPLERATE, rate);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_BITRATEMODE, vbr);
    HAPAssert(er == AACENC_OK);
    er = aacEncoder_SetParam(aacEncoder, AACENC_BITRATE, bitrate);
    HAPAssert(er == AACENC_OK);
    // er = aacEncoder_SetParam(aacEncoder, AACENC_PEAK_BITRATE, bitrate); // ???
    // HAPAssert(er == AACENC_OK);
    // er = aacEncoder_SetParam(aacEncoder, AACENC_SBR_MODE, 0); // No SBR. // ???
    // HAPAssert(er == AACENC_OK);
    // er = aacEncoder_SetParam(aacEncoder, AACENC_SBR_RATIO, 2); // ???
    // HAPAssert(er == AACENC_OK);

    er = aacEncEncode(aacEncoder, NULL, NULL, NULL, NULL);
    if (er != AACENC_OK) {
        HAPLogError(&logObject, "aacEncEncode(NULL) error: 0x%04X", (int) er);
    }
    HAPAssert(er == AACENC_OK);

    AACENC_InfoStruct encInfo;
    er = aacEncInfo(aacEncoder, &encInfo);
    HAPAssert(er == AACENC_OK);
    HAPAssert(encInfo.frameLength == kAACELDSamplesPerPacket);

    (*microphoneStream)->pcmHandle = pcmHandle;
    (*microphoneStream)->encoder = aacEncoder;
    (*microphoneStream)->sampleRate = rate;
    (*microphoneStream)->context = context;
    (*microphoneStream)->callback = callback;

    // Start Thread.
    err = pthread_create(&(*microphoneStream)->thread, NULL, AudioAACInputThread, *microphoneStream);
    HAPAssert(err == 0);

    return kHAPError_None;
}

#endif
