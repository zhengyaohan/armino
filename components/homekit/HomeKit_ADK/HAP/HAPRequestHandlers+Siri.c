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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#include "HAP+API.h"
#include "HAP.h"
#include "HAPAccessory.h"
#include "HAPCharacteristic.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

// static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

// Spec defined Packet Time for Audio Stream
#define kPacketTime (20) // [ms]

//----------------------------------------------------------------------------------------------------------------------

/**
 * Appends a separator to the TLV writer payload.
 *
 * @param      responseWriter       Writer to append separator TLV to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If writer does not have enough capacity.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 18.1.1 TLV Rules
 */
HAP_RESULT_USE_CHECK
static HAPError AppendSeparator(HAPTLVWriter* responseWriter) {
    HAPPrecondition(responseWriter);

    HAPError err;
    err = HAPTLVWriterAppend(
            responseWriter, &(const HAPTLV) { .type = 0x00, .value = { .bytes = NULL, .numBytes = 0 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }
    return kHAPError_None;
}

/**
 * Indicates whether an audio codec type is supported for Siri.
 *
 * @param      audioCodecType       Audio codec type.
 *
 * @return true                     If the audio codec type is supported for Siri.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool AudioCodecTypeIsSupportedForSiri(HAPAudioCodecType audioCodecType) {
    switch (audioCodecType) {
        case kHAPAudioCodecType_Opus: {
            return true;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_ELD:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            HAPLogError(&kHAPLog_Default, "Audio codec not supported for streaming: %u.", audioCodecType);
            return false;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes audio codec type.
 *
 * @param      audioCodecType       Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecType SerializeAudioCodecType(HAPAudioCodecType audioCodecType) {
    HAPPrecondition(AudioCodecTypeIsSupportedForSiri(audioCodecType));

    switch (audioCodecType) {
        case kHAPAudioCodecType_Opus: {
            return kHAPCharacteristicValue_AudioCodecType_Opus;
        }
        case kHAPAudioCodecType_PCMU: {
            return kHAPCharacteristicValue_AudioCodecType_Opus; // TODO
        }
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_ELD:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes audio bit-rate.
 *
 * @param      audioBitRate         Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecBitRateControlMode
        SerializeAudioBitRate(HAPAudioCodecBitRateControlMode audioBitRate) {
    switch (audioBitRate) {
        case kHAPAudioCodecBitRateControlMode_Variable:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable;
        case kHAPAudioCodecBitRateControlMode_Constant:
            return kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant;
        default:
            HAPFatalError();
    }
}

/**
 * Indicates whether an audio sample rate is supported for Siri.
 *
 * @param      audioSampleRate      Audio sample rate.
 *
 * @return true                     If the audio sample rate is supported for streaming.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool AudioSampleRateIsSupportedForSiri(HAPAudioCodecSampleRate audioSampleRate) {
    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_16KHz: {
            return true;
        }
        case kHAPAudioCodecSampleRate_8KHz:
        case kHAPAudioCodecSampleRate_24KHz:
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz: {
            return false;
        }
        default:
            HAPFatalError();
    }
}

/**
 * Serializes audio sample rate.
 *
 * @param      audioSampleRate      Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
static HAPCharacteristicValue_AudioCodecSampleRate SerializeAudioSampleRate(HAPAudioCodecSampleRate audioSampleRate) {
    HAPPrecondition(AudioSampleRateIsSupportedForSiri(audioSampleRate));

    switch (audioSampleRate) {
        case kHAPAudioCodecSampleRate_16KHz: {
            return kHAPCharacteristicValue_AudioCodecSampleRate_16KHz;
        }
        case kHAPAudioCodecSampleRate_8KHz:
        case kHAPAudioCodecSampleRate_24KHz:
        case kHAPAudioCodecSampleRate_32KHz:
        case kHAPAudioCodecSampleRate_44_1KHz:
        case kHAPAudioCodecSampleRate_48KHz:
        default:
            HAPFatalError();
    }
}

/**
 * Serializes an audio stream configuration.
 *
 * @param      configuration        Audio stream configuration.
 * @param      responseWriter       TLV writer for serializing the response.
 * @param      includePacketTime    Boolean indicating if packet time should be populated in TLV
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If out of resources to process request.
 */
static HAPError SerializeAudioCodecConfiguration(
        const HAPCameraSupportedAudioCodecConfiguration* configuration,
        HAPTLVWriter* responseWriter,
        bool includePacketTime) {
    HAPPrecondition(configuration);
    HAPPrecondition(AudioCodecTypeIsSupportedForSiri(configuration->codecType));
    HAPPrecondition(responseWriter);

    HAPError err;

    // Audio Codec Type.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) {
                    .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecType,
                    .value = { .bytes = (const uint8_t[]) { SerializeAudioCodecType(configuration->codecType) },
                               .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // Audio Codec Parameters.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        const HAPAudioCodecParameters* parameters = configuration->codecParameters;

        // Audio channels.
        err = HAPTLVWriterAppend(
                &subWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_AudioCodecStreamParameter_AudioChannels,
                        .value = { .bytes = (const uint8_t[]) { parameters->numberOfChannels }, .numBytes = 1 } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        // Bit-rate.
        bool appended = false;
        HAPAudioCodecBitRateControlMode bitRateMode = parameters->bitRateMode;
        for (size_t j = 0; bitRateMode; j++) {
            HAPAudioCodecBitRateControlMode rate = (HAPAudioCodecBitRateControlMode)(1U << j);

            if (bitRateMode & rate) {
                bitRateMode &= ~rate;

                // Append separator if necessary.
                if (appended) {
                    err = AppendSeparator(&subWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    appended = true;
                }

                // Serialize.
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_AudioCodecStreamParameter_BitRate,
                                          .value = { .bytes = (const uint8_t[]) { SerializeAudioBitRate(rate) },
                                                     .numBytes = 1 } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
        HAPPrecondition(appended);

        // Sample rate.
        appended = false;
        HAPAudioCodecSampleRate sampleRate = parameters->sampleRate;
        for (size_t j = 0; sampleRate; j++) {
            HAPAudioCodecSampleRate rate = (HAPAudioCodecSampleRate)(1U << j);

            if (sampleRate & rate) {
                HAPPrecondition(AudioSampleRateIsSupportedForSiri(rate));
                sampleRate &= ~rate;

                // Append separator if necessary.
                if (appended) {
                    err = AppendSeparator(&subWriter);
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                } else {
                    appended = true;
                }

                // Serialize.
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) { .type = kHAPCharacteristicValue_AudioCodecStreamParameter_SampleRate,
                                          .value = { .bytes = (const uint8_t[]) { SerializeAudioSampleRate(rate) },
                                                     .numBytes = 1 } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }
        }
        HAPPrecondition(appended);

        // Packet Time.
        if (includePacketTime) {
            err = HAPTLVWriterAppend(
                    &subWriter,
                    &(const HAPTLV) { .type = kHAPCharacteristicValue_AudioCodecStreamParameter_RTPTime,
                                      .value = { .bytes = (const uint8_t[]) { kPacketTime }, .numBytes = 1 } });
            if (err) {
                HAPAssert(err == kHAPError_OutOfResources);
                return err;
            }
        }

        // Finalize.
        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecParameters,
                        .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Handle read request to the 'Supported Audio Stream Configuration' characteristic
 * of the Audio Stream Management service associated with Siri.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSiriAudioStreamManagementSupportedAudioStreamConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SupportedAudioStreamConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AudioStreamManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPLog(&kHAPLog_Default, "%s", __func__);

    // We have to use Opus, VBR, 16kHz.
    // See HomeKit Accessory Protocol Specification R17
    // Section 13.2.1.2 Binary Data
    HAPAudioCodecParameters parameters = { .numberOfChannels = 1,
                                           .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
                                           .sampleRate = kHAPAudioCodecSampleRate_16KHz };
    HAPCameraSupportedAudioCodecConfiguration configuration = { .codecType = kHAPAudioCodecType_Opus,
                                                                .codecParameters = &parameters };

    // Audio Codec Configuration.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        err = SerializeAudioCodecConfiguration(&configuration, &subWriter, false);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration,
                                  .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    // Comfort Noise.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPCharacteristicValue_SupportedAudioStreamConfiguration_ComfortNoiseSupport,
                              .value = { .bytes = (const uint8_t[]) { (uint8_t) 0 }, .numBytes = 1 } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

/**
 * Handle read request to the 'Selected Audio Stream Configuration' characteristic
 * of the Audio Stream Management service associated with Siri.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSiriAudioStreamManagementSelectedAudioStreamConfigurationRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedAudioStreamConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AudioStreamManagement));
    HAPPrecondition(responseWriter);

    HAPError err;

    HAPLog(&kHAPLog_Default, "%s", __func__);

    // Configuration is Opus, VBR, 16kHz.
    HAPAudioCodecParameters parameters = { .numberOfChannels = 1,
                                           .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
                                           .sampleRate = kHAPAudioCodecSampleRate_16KHz };
    HAPCameraSupportedAudioCodecConfiguration configuration = { .codecType = kHAPAudioCodecType_Opus,
                                                                .codecParameters = &parameters };

    // Audio Codec Configuration.
    {
        HAPTLVWriter subWriter;
        {
            void* bytes;
            size_t maxBytes;
            HAPTLVWriterGetScratchBytes(responseWriter, &bytes, &maxBytes);
            HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
        }

        err = SerializeAudioCodecConfiguration(&configuration, &subWriter, true);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }

        void* bytes;
        size_t numBytes;
        HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
        err = HAPTLVWriterAppend(
                responseWriter,
                &(const HAPTLV) {
                        .type = kHAPCharacteristicValue_SelectedAudioStreamConfiguration_SelectedAudioInputStreamConfiguration,
                        .value = { .bytes = bytes, .numBytes = numBytes } });
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            return err;
        }
    }

    return kHAPError_None;
}

/**
 * Parses audio codec type.
 *
 * @param[out] audioCodecType       Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioCodecType(HAPAudioCodecType* audioCodecType, uint8_t rawValue) {
    HAPPrecondition(audioCodecType);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_AudioCodecType));
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wdeprecated-declarations")
    HAP_DIAGNOSTIC_IGNORED_ARMCC(2570)
    HAP_DIAGNOSTIC_IGNORED_ICCARM(Pe1444)
    switch ((HAPCharacteristicValue_AudioCodecType) rawValue) {
        case kHAPCharacteristicValue_AudioCodecType_Opus: {
            *audioCodecType = kHAPAudioCodecType_Opus;
            return kHAPError_None;
        }
        case kHAPCharacteristicValue_AudioCodecType_AAC_ELD:
        case kHAPCharacteristicValue_AudioCodecType_AMR:
        case kHAPCharacteristicValue_AudioCodecType_AMR_WB: {
            HAPLog(&kHAPLog_Default, "Audio codec type invalid: %u.", rawValue);
            return kHAPError_None;
        }
        default: {
            HAPLog(&kHAPLog_Default, "Audio codec type invalid: %u.", rawValue);
            return kHAPError_InvalidData;
        }
    }
    HAP_DIAGNOSTIC_RESTORE_ICCARM(Pe1444)
    HAP_DIAGNOSTIC_POP
}

/**
 * Parses audio bit-rate.
 *
 * @param[out] audioBitRate         Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioBitRate(HAPAudioCodecBitRateControlMode* audioBitRate, uint8_t rawValue) {
    HAPPrecondition(audioBitRate);

    switch (rawValue) {
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable: {
            *audioBitRate = kHAPAudioCodecBitRateControlMode_Variable;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant: {
            *audioBitRate = kHAPAudioCodecBitRateControlMode_Constant;
        }
            return kHAPError_None;
        default: {
            HAPLogError(&kHAPLog_Default, "Audio bit-rate invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

/**
 * Parses audio sample rate.
 *
 * @param[out] audioSampleRate      Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseAudioSampleRate(HAPAudioCodecSampleRate* audioSampleRate, uint8_t rawValue) {
    HAPPrecondition(audioSampleRate);

    switch (rawValue) {
        case kHAPCharacteristicValue_AudioCodecSampleRate_16KHz: {
            *audioSampleRate = kHAPAudioCodecSampleRate_16KHz;
            return kHAPError_None;
        }
        case kHAPCharacteristicValue_AudioCodecSampleRate_8KHz:
        case kHAPCharacteristicValue_AudioCodecSampleRate_24KHz:
        default: {
            HAPLogError(&kHAPLog_Default, "Audio sample rate invalid: %u.", rawValue);
            return kHAPError_InvalidData;
        }
    }
}

/**
 * Returns whether a packet time is supported by a given audio codec and audio sample rate.
 *
 * @param      audioCodecType       Audio codec type.
 * @param      audioSampleRate      Audio sample rate.
 * @param      packetTime           Packet time in ms.
 *
 * @return true                     If the given packet time is supported by the audio codec and audio sample rate.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
static bool IsPacketTimeSupportedByAudioCodec(
        HAPAudioCodecType audioCodecType,
        HAPAudioCodecSampleRate audioSampleRate,
        uint8_t packetTime) {
    HAPPrecondition(AudioCodecTypeIsSupportedForSiri(audioCodecType));
    HAPPrecondition(AudioSampleRateIsSupportedForSiri(audioSampleRate));

    // General HAP limitations: Only 20, 30, 40, 60 ms.
    switch (audioCodecType) {
        case kHAPAudioCodecType_Opus: {
            // 2.5, 5, 10, 20, 40, 60 ms.
            return packetTime == 20 || packetTime == 40 || packetTime == 60;
        }
        case kHAPAudioCodecType_PCMU:
        case kHAPAudioCodecType_PCMA:
        case kHAPAudioCodecType_AAC_ELD:
        case kHAPAudioCodecType_AAC_LC:
        case kHAPAudioCodecType_MSBC:
        case kHAPAudioCodecType_AMR:
        case kHAPAudioCodecType_AMR_WB: {
            HAPFatalError();
        }
        default:
            HAPFatalError();
    }
}

/**
 * Parses selected audio parameters of a Selected Audio Stream Configuration command.
 *
 * @param[out] audioCodecType       Parsed codec type.
 * @param[out] codecParameters      Parsed codec parameters.
 * @param[out] packetTime           Parsed packet time.
 * @param      requestReader        TLV reader.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the request reader contains invalid data.
 */
HAP_RESULT_USE_CHECK
static HAPError TryParseSelectedAudioStreamConfiguration(
        HAPAudioCodecType* audioCodecType,
        HAPAudioCodecParameters* codecParameters,
        uint8_t* packetTime,
        HAPTLVReader* requestReader) {
    HAPPrecondition(audioCodecType);
    HAPPrecondition(codecParameters);
    HAPPrecondition(packetTime);
    HAPPrecondition(requestReader);

    HAPError err;

    HAPRawBufferZero(audioCodecType, sizeof *audioCodecType);
    HAPRawBufferZero(codecParameters, sizeof *codecParameters);
    HAPRawBufferZero(packetTime, sizeof *packetTime);

    HAPTLV codecTypeTLV, codecParametersTLV;
    codecTypeTLV.type = kHAPCharacteristicValue_SelectedAudioStreamConfiguration_Configuration_SelectedAudioCodecType;
    codecParametersTLV.type =
            kHAPCharacteristicValue_SelectedAudioStreamConfiguration_Configuration_SelectedAudioCodecParameters;
    err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]) { &codecTypeTLV, &codecParametersTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Audio Codec type.
    if (!codecTypeTLV.value.bytes) {
        HAPLogError(&kHAPLog_Default, "Selected Audio Codec type missing.");
        return kHAPError_InvalidData;
    }
    if (codecTypeTLV.value.numBytes > 1) {
        HAPLogError(
                &kHAPLog_Default,
                "Selected Audio Codec type has invalid length (%lu).",
                (unsigned long) codecTypeTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    err = TryParseAudioCodecType(
            audioCodecType, HAPReadUIntMax8(codecTypeTLV.value.bytes, codecTypeTLV.value.numBytes));
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }
    HAPAssert(AudioCodecTypeIsSupportedForSiri(*audioCodecType));

    // Selected Audio Codec parameters.
    if (!codecParametersTLV.value.bytes) {
        HAPLogError(&kHAPLog_Default, "Selected Audio Codec parameters missing.");
        return kHAPError_InvalidData;
    }
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) codecParametersTLV.value.bytes, codecParametersTLV.value.numBytes);

        HAPTLV audioChannelsTLV, bitRateTLV, sampleRateTLV, packetTimeTLV;
        audioChannelsTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_AudioChannels;
        bitRateTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_BitRate;
        sampleRateTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_SampleRate;
        packetTimeTLV.type = kHAPCharacteristicValue_AudioCodecStreamParameter_RTPTime;
        err = HAPTLVReaderGetAll(
                &subReader, (HAPTLV* const[]) { &audioChannelsTLV, &bitRateTLV, &sampleRateTLV, &packetTimeTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Audio channels.
        if (!audioChannelsTLV.value.bytes) {
            HAPLogError(&kHAPLog_Default, "Audio channels missing.");
            return kHAPError_InvalidData;
        }
        if (audioChannelsTLV.value.numBytes > 1) {
            HAPLogError(
                    &kHAPLog_Default,
                    "Audio channels has invalid length (%lu).",
                    (unsigned long) audioChannelsTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        codecParameters->numberOfChannels =
                HAPReadUIntMax8(audioChannelsTLV.value.bytes, audioChannelsTLV.value.numBytes);

        // Bit rate.
        if (!bitRateTLV.value.bytes) {
            HAPLogError(&kHAPLog_Default, "Audio bit rate missing.");
            return kHAPError_InvalidData;
        }
        if (bitRateTLV.value.numBytes > 1) {
            HAPLogError(
                    &kHAPLog_Default,
                    "Audio bit rate has invalid length (%lu).",
                    (unsigned long) bitRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        err = TryParseAudioBitRate(
                &codecParameters->bitRateMode, HAPReadUIntMax8(bitRateTLV.value.bytes, bitRateTLV.value.numBytes));
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Sample rate.
        if (!sampleRateTLV.value.bytes) {
            HAPLogError(&kHAPLog_Default, "Audio sample rate missing.");
            return kHAPError_InvalidData;
        }
        if (sampleRateTLV.value.numBytes > 1) {
            HAPLogError(
                    &kHAPLog_Default,
                    "Audio sample rate has invalid length (%lu).",
                    (unsigned long) sampleRateTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        err = TryParseAudioSampleRate(
                &codecParameters->sampleRate, HAPReadUIntMax8(sampleRateTLV.value.bytes, sampleRateTLV.value.numBytes));
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
        HAPAssert(AudioSampleRateIsSupportedForSiri(codecParameters->sampleRate));

        // Packet time.
        if (!packetTimeTLV.value.bytes) {
            HAPLog(&kHAPLog_Default, "Audio packet time missing. Using 20ms packet time.");
            *packetTime = kPacketTime;
        } else {
            if (packetTimeTLV.value.numBytes > 1) {
                HAPLogError(
                        &kHAPLog_Default,
                        "Audio packet time has invalid length (%lu).",
                        (unsigned long) packetTimeTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            *packetTime = HAPReadUIntMax8(packetTimeTLV.value.bytes, packetTimeTLV.value.numBytes);
            if (*packetTime != 20 && *packetTime != 30 && *packetTime != 40 && *packetTime != 60) {
                HAPLogError(&kHAPLog_Default, "Audio packet time invalid: %u.", *packetTime);
                return kHAPError_InvalidData;
            }
            if (!IsPacketTimeSupportedByAudioCodec(*audioCodecType, codecParameters->sampleRate, *packetTime)) {
                HAPLog(&kHAPLog_Default, "Audio packet time is not supported by codec: %u.", *packetTime);
                return kHAPError_InvalidData;
            }
        }
    }

    return kHAPError_None;
}

/**
 * Handle write request to the 'Selected Audio Stream Configuration' characteristic
 * of the Audio Stream Management service associated with Siri.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSiriAudioStreamManagementSelectedAudioStreamConfigurationWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(request->session);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_SelectedAudioStreamConfiguration));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_AudioStreamManagement));
    HAPPrecondition(requestReader);

    HAPError err;

    HAPLog(&kHAPLog_Default, "%s", __func__);

    HAPTLV selectedAudioTLV;
    selectedAudioTLV.type =
            kHAPCharacteristicValue_SelectedAudioStreamConfiguration_SelectedAudioInputStreamConfiguration;
    err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]) { &selectedAudioTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Selected Audio Input Stream Configuration.
    if (!selectedAudioTLV.value.bytes) {
        HAPLogError(&kHAPLog_Default, "Selected Audio Input Stream Configuration missing.");
        return kHAPError_InvalidData;
    }
    HAPAudioCodecType codecType;
    HAPAudioCodecParameters codecParameters;
    uint8_t packetTime;
    {
        HAPTLVReader subReader;
        HAPTLVReaderCreate(
                &subReader, (void*) (uintptr_t) selectedAudioTLV.value.bytes, selectedAudioTLV.value.numBytes);

        err = TryParseSelectedAudioStreamConfiguration(&codecType, &codecParameters, &packetTime, &subReader);
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }
    }

    // Check selected codec configuration
    if (codecType != kHAPAudioCodecType_Opus || codecParameters.numberOfChannels != 1 ||
        codecParameters.bitRateMode != kHAPAudioCodecBitRateControlMode_Variable ||
        codecParameters.sampleRate != kHAPAudioCodecSampleRate_16KHz || packetTime != kPacketTime) {
        HAPLogError(&kHAPLog_Default, "Invalid Selected Audio Input Stream Configuration.");
        return kHAPError_InvalidData;
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Handle read request to the 'Siri Input Type' characteristic
 * of the Siri service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSiriInputTypeRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {

    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(value);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_SiriInputType));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_Siri));
    HAPPrecondition(request->accessory->callbacks.siri.getSiriInputType);
    HAPError err;
    err = request->accessory->callbacks.siri.getSiriInputType(server, value, context);
    HAPLog(&kHAPLog_Default, "%s: %u", __func__, *value);

    return err;
}
