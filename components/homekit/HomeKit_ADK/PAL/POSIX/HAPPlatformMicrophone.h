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

#ifndef HAP_PLATFORM_MICROPHONE_H
#define HAP_PLATFORM_MICROPHONE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MICROPHONE)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * Audio input abstraction. For IP cameras and video doorbells, it is only needed if the default camera PAL
 * implementation of Apple is used. This PAL module delivers sound data from a microphone by invoking
 * `HAPPlatformMicrophoneDataCallback`. Along with the Opus-encoded sound samples that you provide through the
 * arguments bytes, numBytes and sampleTime of this function, you must also provide a root mean square value in argument
 * rms. It is an average volume computed like this:
 *
 * \rst
 *
 * :math:`rms = sqrt( sum( i = [0 - (n-1)], s[i]^{2} ) / n )`
 *
 * \endrst
 *
 * Where s[i] are the individual raw audio samples and n is the number of audio samples provided in this callback.
 *
 * The resulting value, which you then pass to the rms argument, must be in the range of *0* to *1* for all
 * representable audio signals. The necessary scaling can be done by scaling the raw audio samples to the range -1 to +1
 * before the rms calculation. The scale factor is simply 1/max where max is the positive maximum of the values
 * representable in the used format. For floating point formats (F32 or F64), no scaling is needed, as they already are
 * in the range
 * *-1* to *+1*.
 *
 * Alternatively, the rms could be computed on the unscaled samples, followed by a scaling of the resulting rms. The
 * same scale factor is needed as in the first approach. These are the scale factors for some commonly used formats:
 *
 * \rst
 *
 * +-------------------+-------------------+
 * | Audio Format      | Scale Factor      |
 * +===================+===================+
 * | S8                | :math:`1 / 2^{7}` |
 * +-------------------+-------------------+
 * | S16               | :math:`1 / 2^{15}`|
 * +-------------------+-------------------+
 * | S24               | :math:`1 / 2^{23}`|
 * +-------------------+-------------------+
 * | F32               | :math:`1`         |
 * +-------------------+-------------------+
 * | F364              | :math:`1`         |
 * +-------------------+-------------------+
 *
 * \endrst
 *
 * For more information on audio formats, see e.g.
 * https://gstreamer.freedesktop.org/documentation/design/mediatype-audio-raw.html
 *
 * In a typical implementation, the HAPPlatformMicrophoneDataCallback calls are invoked by a separate thread. The
 * called code is in the IPCamera or Remote accessory logic, where it must be synchronized with the main thread as
 * needed.
 *
 */

/**
 * Microphone.
 */
typedef struct HAPPlatformMicrophone HAPPlatformMicrophone;
typedef struct HAPPlatformMicrophone* HAPPlatformMicrophoneRef;
HAP_NONNULL_SUPPORT(HAPPlatformMicrophone)

/**
 * Microphone stream.
 */
typedef struct HAPPlatformMicrophoneStream HAPPlatformMicrophoneStream;
typedef struct HAPPlatformMicrophoneStream* HAPPlatformMicrophoneStreamRef;
HAP_NONNULL_SUPPORT(HAPPlatformMicrophoneStream)

/**
 * Invoked when new audio data is available.
 *
 * The callback is called from a pipeline thread.
 *
 * @param      context              Context.
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 * @param      bytes                Audio data.
 * @param      numBytes             Length of audio data.
 * @param      sampleTime           Sample time of first data entry [ns].
 * @param      rms                  RMS of raw audio data in range -1 .. +1.
 */
typedef void (*_Nullable HAPPlatformMicrophoneDataCallback)(
        void* _Nullable context,
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const void* bytes,
        size_t numBytes,
        HAPTimeNS sampleTime,
        float rms);

/**
 * Starts a microphone Opus audio stream.
 *
 * @param      microphone           Microphone.
 * @param[out] microphoneStream     Microphone stream.
 * @param      sampleRate           Audio sample rate.
 * @param      bitRateMode          Bit-rate mode.
 * @param      bitrate              Bit-rate [bit/s].
 * @param      packetTime           Audio block length [ms].
 * @param      callback             Callback to be called for each block.
 * @param      context              Callback context.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If no more concurrent streams may be started.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartOpusStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        uint32_t packetTime,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
/**
 * Starts a microphone AAC-LC audio stream.
 *
 * @param      microphone           Microphone.
 * @param[out] microphoneStream     Microphone stream.
 * @param      sampleRate           Audio sample rate.
 * @param      bitRateMode          Bit-rate mode.
 * @param      bitrate              Bit-rate [bit/s].
 * @param      callback             Callback to be called for each block.
 * @param      context              Callback context.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACLCStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context);

/**
 * Starts a microphone AAC-ELD audio stream.
 *
 * @param      microphone           Microphone.
 * @param[out] microphoneStream     Microphone stream.
 * @param      sampleRate           Audio sample rate.
 * @param      bitRateMode          Bit-rate mode.
 * @param      bitrate              Bit-rate [bit/s].
 * @param      callback             Callback to be called for each block.
 * @param      context              Callback context.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an I/O error occurred.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformMicrophoneStartAACELDStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef _Nullable* _Nonnull microphoneStream,
        HAPAudioCodecSampleRate sampleRate,
        HAPAudioCodecBitRateControlMode bitRateMode,
        uint32_t bitrate,
        HAPPlatformMicrophoneDataCallback callback,
        void* _Nullable context);

#endif

/**
 * Stops the microphone audio stream.
 *
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 */
void HAPPlatformMicrophoneStopStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream);

/**
 * Suspends the microphone audio stream.
 *
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 */
void HAPPlatformMicrophoneSuspendStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream);

/**
 * Resumes the microphone audio stream.
 *
 * @param      microphone           Microphone.
 * @param      microphoneStream     Microphone stream.
 */
void HAPPlatformMicrophoneResumeStream(
        HAPPlatformMicrophoneRef microphone,
        HAPPlatformMicrophoneStreamRef microphoneStream);

/**
 * Sets the microphone volume.
 *
 * @param      microphone           Microphone.
 * @param      volume               Volume [%].
 */
void HAPPlatformMicrophoneSetVolume(HAPPlatformMicrophoneRef microphone, unsigned int volume);

/**
 * Mutes or un-mutes the microphone.
 *
 * @param      microphone           Microphone.
 * @param      muted                Whether or not to mute the stream.
 */
void HAPPlatformMicrophoneSetMuted(HAPPlatformMicrophoneRef microphone, bool muted);

/**
 * Check whether stream driver is in the running state.
 *
 * This function should be used for debug purpose to check the sanity of the driver state.
 *
 * @param      microphoneStream   Microphone stream
 * @param[out] stateString        variable to store pointer to string describing the current state
 *
 * @return true if the microphone stream driver is in running state. false, otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformMicrophoneIsStreamRunning(
        HAPPlatformMicrophoneStreamRef microphoneStream,
        const char* _Nullable* _Nonnull stateString);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
