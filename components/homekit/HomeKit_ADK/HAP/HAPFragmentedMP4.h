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

#ifndef HAP_FRAGMENTED_MP4_H
#define HAP_FRAGMENTED_MP4_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Video media type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPFragmentedMP4VideoType) { /** H264 Video. */
                                                     kHAPFragmentedMP4VideoType_H264 = 1
} HAP_ENUM_END(uint8_t, HAPFragmentedMP4VideoType);

/**
 * Audio media type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPFragmentedMP4AudioType) { /** AAC-LC Audio. */
                                                     kHAPFragmentedMP4AudioType_AAC_LC = 1,

                                                     /** AAC-ELD Audio. */
                                                     kHAPFragmentedMP4AudioType_AAC_ELD
} HAP_ENUM_END(uint8_t, HAPFragmentedMP4AudioType);

/**
 * Recording configuration.
 */
typedef struct {
    /** Video parameters. */
    struct {
        HAPFragmentedMP4VideoType type; /**< Video media type. */

        uint16_t width;     /**< Image width in pixels. */
        uint16_t height;    /**< Image height in pixels. */
        uint16_t timescale; /**< Timescale. [Hz] */

        /** H264 SPS NAL unit. */
        struct {
            uint8_t* bytes;    /**< Buffer. */
            uint16_t numBytes; /**< Length of buffer. */
        } sps;

        /** H264 PPS NAL unit. */
        struct {
            uint8_t* bytes;    /**< Buffer. */
            uint16_t numBytes; /**< Length of buffer. */
        } pps;

        uint8_t naluSizeBytes; /**< Number of bytes used for NAL unit size prefix. */
    } video;

    /** Audio parameters. */
    struct {
        HAPFragmentedMP4AudioType type; /**< Audio media type. */

        uint8_t numberOfChannels; /** Number of audio channels. */
        uint16_t sampleRate;      /** Sample rate. [Hz] */
        uint32_t bitRate;         /** Bit rate. [bit/s] */
    } audio;
} HAPFragmentedMP4RecordingConfiguration;

/**
 * Writes a fragmented MP4 movie header.
 * The header consists of a 'ftyp' and a 'moov' box.
 *
 * @param[out] bytes                The buffer to write the header to.
 * @param      maxBytes             The capacity of the buffer.
 * @param[out] numBytes             The number of bytes written.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the header.
 */
HAP_RESULT_USE_CHECK
HAPError HAPFragmentedMP4WriteMovieHeader(
        const HAPFragmentedMP4RecordingConfiguration* config,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes);

/**
 * Fragment track type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPFragmentedMP4TrackType) { /** Video Track. */
                                                     kHAPFragmentedMP4TrackType_Video = 1,

                                                     /** Audio Track. */
                                                     kHAPFragmentedMP4TrackType_Audio
} HAP_ENUM_END(uint8_t, HAPFragmentedMP4TrackType);

/**
 * FMP4 sample table entry descriptor.
 */
typedef struct {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // type of bit-field '###' is a GCC extension
    uint32_t size;                           /**< Sample size */
    uint32_t duration : 31;                  /**< Sample duration */
    uint32_t iFrameFlag : 1;                 /**< Set to 1 if sample is a key frame */
    HAP_DIAGNOSTIC_POP
} HAPFragmentedMP4SampleDescriptor;

/**
 * FMP4 sample run descriptor.
 */
typedef struct {
    /** Track type. */
    HAPFragmentedMP4TrackType trackType;

    /** Decode time of first sample in timescale units. */
    uint64_t decodeTime; // In timescale units.

    /** Number of samples included in this run. */
    uint32_t numSamples;

    /**
     * Array of sample descriptors.
     * One entry must be provided per sample included in this run.
     */
    const HAPFragmentedMP4SampleDescriptor* sampleTable;

    /**
     * Total number of bytes for this run.
     * Must be equal to the sum of the bytes of all samples.
     */
    uint32_t numRunDataBytes;

    /**
     * Reserved for internal use.
     */
    void* _Nullable fixup;
} HAPFragmentedMP4RunDescriptor;

/**
 * Fragment configuration.
 */
typedef struct {
    /** Fragment sequence number. */
    uint32_t sequenceNumber;

    /** Number of runs included in this fragment. */
    uint32_t numRuns;

    /**
     * Array of run descriptors.
     * One entry must be provided per run included in this fragment.
     */
    HAPFragmentedMP4RunDescriptor* run;
} HAPFragmentedMP4FragmentConfiguration;

/**
 * Writes a fragmented MP4 fragment header.
 * The fragment header consists of a 'moof' box and the header of a 'mdat' box.
 * The actual sample data has to follow immediately after the header.
 *
 * @param      config               The fragment configuration used.
 * @param[out] bytes                The buffer to write the fragment to.
 * @param      maxBytes             The capacity of the buffer.
 * @param[out] numBytes             The number of bytes written.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the fragment.
 */
HAP_RESULT_USE_CHECK
HAPError HAPFragmentedMP4WriteFragmentHeader(
        const HAPFragmentedMP4FragmentConfiguration* config,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
