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

#include "HAPFragmentedMP4.h"

#include "HAPMP4BoxWriter.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#define HAP_FRAGMENTED_MP4_ONE_TRAF_PER_TRAK 1

/**
 * MP4 sample flags.
 * @see ISO/IEC 15596-12 8.8.3.1 & 8.6.4.3
 */
HAP_OPTIONS_BEGIN(uint32_t, SampleFlag) {
    kSampleFlag_IsLeading = 0x4000000U,
    kSampleFlag_NotDependsOn = 0x2000000U,
    kSampleFlag_DependsOn = 0x1000000U,
    kSampleFlag_IsNotDependedOn = 0x0800000U,
    kSampleFlag_IsDependedOn = 0x0400000U,
    kSampleFlag_HasNoRedundancy = 0x0200000U,
    kSampleFlag_HasRedundancy = 0x0100000U,
    kSampleFlag_IsNonSync = 0x0010000U,
    kSampleFlag_VideoIFrame = kSampleFlag_NotDependsOn,
    kSampleFlag_VideoNonIFrame = (uint32_t) kSampleFlag_DependsOn | (uint32_t) kSampleFlag_IsNonSync
} HAP_OPTIONS_END(uint32_t, SampleFlag);

/**
 * MP4 sample flags options.
 */
HAP_OPTIONS_BEGIN(uint8_t, SampleFlagOptions) { /** No sample flags needed. */
                                                kSampleFlagOptions_None = 0x00U,

                                                /** Default sample flags needed. */
                                                kSampleFlagOptions_Default = 0x01U,

                                                /** First sample flags needed. */
                                                kSampleFlagOptions_First = 0x02U,

                                                /** Sample flag table needed. */
                                                kSampleFlagOptions_Table = 0x04U
} HAP_OPTIONS_END(uint8_t, SampleFlagOptions);

/**
 * MP4 track fragment header flags.
 * @see ISO/IEC 15596-12 8.8.7.1
 */
HAP_OPTIONS_BEGIN(uint32_t, FragmentFlag) {
    kFragmentFlag_BaseDataOffsetPresent = 0x000001U,
    kFragmentFlag_SampleDescriptorIndexPresent = 0x000002U,
    kFragmentFlag_DefaultSampleDurationPresent = 0x000008U,
    kFragmentFlag_DefaultSampleSizePresent = 0x000010U,
    kFragmentFlag_DefaultSampleFlagsPresent = 0x000020U,
    kFragmentFlag_DurationIsEmpty = 0x010000U,
    kFragmentFlag_DefaultBaseIsMoof = 0x020000U
} HAP_OPTIONS_END(uint32_t, FragmentFlag);

/**
 * MP4 track fragment run flags.
 * @see ISO/IEC 15596-12 8.8.8.1
 */
HAP_OPTIONS_BEGIN(uint16_t, FragmentRunFlag) {
    kFragmentRunFlag_DataOffsetPresent = 0x000001U,     kFragmentRunFlag_FirstSampleFlagPresent = 0x000004U,
    kFragmentRunFlag_SampleDurationPresent = 0x000100U, kFragmentRunFlag_SampleSizePresent = 0x000200U,
    kFragmentRunFlag_SampleFlagsPresent = 0x000400U,    kFragmentRunFlag_SampleTimeOffsetPresent = 0x000800U
} HAP_OPTIONS_END(uint16_t, FragmentRunFlag);

/**
 * ESDS box constants.
 *
 * @see Common File Format & Media Formats Specification V2.1 5.3.2.1
 */
/**@{*/
HAP_ENUM_BEGIN(uint8_t, ESDSTag) { kESDSTag_ESDescriptor = 0x03,
                                   kESDSTag_Configuration = 0x04,
                                   kESDSTag_DecoderSpecific = 0x05,
                                   kESDSTag_SLConfiguration = 0x06 } HAP_ENUM_END(uint8_t, ESDSTag);
#define kESDSES_ID               (0x00)
#define kESDSAudioProfileAAC     (0x40)
#define kESDSStreamTypeAudio     (0x05U)
#define kAudioObjectType_AAC_LC  (2)
#define kAudioObjectType_AAC_ELD (39)
#define kAudioObjectType_ESC     (31)
#define kAudioObjectType_OFFSET  (32)
#define kSLConfigDescriptor      (2)
/**@}*/

/**
 * Other constants.
 */
/**@{*/
#define kVideoTrackId (1)
#define kAudioTrackId (2)
/**@}*/

/**
 * AVC decoder compressor name.
 */
static const uint8_t h264Name[32] = { 4, 'h', '2', '6', '4', 0 };

/**
 * Unity matrix field.
 */
static const uint8_t unityMatrix[] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00 };

// static const HAPLogObject logObject = {
//    .subsystem = kHAP_LogSubsystem,
//    .category = "FragmentedMP4"
//};

/**
 * Write file type (ftyp) box.
 *
 * @param[out] bytes                The buffer to write the segment to.
 * @param      maxBytes             The capacity of the buffer.
 * @param[out] numBytes             The number of bytes written.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteFileTypeBox(void* bytes, size_t maxBytes, size_t* numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpen(&writer, bytes, maxBytes, "ftyp");
    if (err != kHAPError_None) {
        return err;
    }
    // Major brand.
    err = HAPMP4BoxWriterAppendBytes(&writer, "mp42", 4);
    if (err != kHAPError_None) {
        return err;
    }
    // Minor version.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Compatible brands.
    err = HAPMP4BoxWriterAppendBytes(&writer, "isom", 4);
    if (err != kHAPError_None) {
        return err;
    }
    err = HAPMP4BoxWriterAppendBytes(&writer, "mp42", 4);
    if (err != kHAPError_None) {
        return err;
    }
    err = HAPMP4BoxWriterAppendBytes(&writer, "avc1", 4);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, numBytes);

    return kHAPError_None;
}

/**
 * Write movie header (mvhd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieHeaderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mvhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Creation time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Modification time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Timescale.
    err = HAPMP4BoxWriterAppendUInt32(&writer, config->video.timescale);
    if (err != kHAPError_None) {
        return err;
    }
    // Duration.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0); // no data in moov box
    if (err != kHAPError_None) {
        return err;
    }
    // Rate (16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, (uint32_t) 1U << 16U); // 1.0
    if (err != kHAPError_None) {
        return err;
    }
    // Volume (8.8).
    err = HAPMP4BoxWriterAppendUInt16(&writer, 1U << 8U); // 1.0
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 10);
    if (err != kHAPError_None) {
        return err;
    }
    // Transformation matrix.
    err = HAPMP4BoxWriterAppendBytes(&writer, unityMatrix, sizeof unityMatrix);
    if (err != kHAPError_None) {
        return err;
    }
    // Predefined (24 bytes).
    err = HAPMP4BoxWriterAppendZero(&writer, 24);
    if (err != kHAPError_None) {
        return err;
    }
    // Next track.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0xFFFFFFFF); // undefined
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write movie extends header (mehd) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieExtendsHeaderBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mehd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Fragment duration.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write track extends (trex) box.
 *
 * @param      container            The writer to write the box to.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackExtendsBox(HAPMP4BoxWriter* container, uint32_t trackId) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "trex");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Track ID.
    err = HAPMP4BoxWriterAppendUInt32(&writer, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Default sample description index.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Default sample duration.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Default sample size.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Default sample flags.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write movie extends (mvex) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieExtendsBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mvex");
    if (err != kHAPError_None) {
        return err;
    }
    // Movie extends header box.
    err = HAPFragmentedMP4WriteMovieExtendsHeaderBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    // Video track extends box.
    err = HAPFragmentedMP4WriteTrackExtendsBox(&writer, kVideoTrackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Audio track extends box.
    if (config->audio.numberOfChannels) {
        err = HAPFragmentedMP4WriteTrackExtendsBox(&writer, kAudioTrackId);
        if (err != kHAPError_None) {
            return err;
        }
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write media header (mdhd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMediaHeaderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mdhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Creation time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Modification time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Timescale.
    err = HAPMP4BoxWriterAppendUInt32(
            &writer, trackId == kVideoTrackId ? config->video.timescale : config->audio.sampleRate);
    if (err != kHAPError_None) {
        return err;
    }
    // Duration.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Language.
    err = HAPMP4BoxWriterAppendUInt16(&writer, ('u' - 0x60U) << 10U | ('n' - 0x60U) << 5U | ('d' - 0x60U));
    if (err != kHAPError_None) {
        return err;
    }
    // Predefined.
    err = HAPMP4BoxWriterAppendZero(&writer, 2);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write handler reference (hdlr) box.
 *
 * @param      container            The writer to write the box to.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteHandlerReferenceBox(HAPMP4BoxWriter* container, uint32_t trackId) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "hdlr");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Predefined.
    err = HAPMP4BoxWriterAppendZero(&writer, 4);
    if (err != kHAPError_None) {
        return err;
    }
    // Handler type.
    if (trackId == kVideoTrackId) {
        err = HAPMP4BoxWriterAppendBytes(&writer, "vide", 4);
        if (err != kHAPError_None) {
            return err;
        }
    } else {
        err = HAPMP4BoxWriterAppendBytes(&writer, "soun", 4);
        if (err != kHAPError_None) {
            return err;
        }
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 12);
    if (err != kHAPError_None) {
        return err;
    }
    // Name.
    if (trackId == kVideoTrackId) {
        err = HAPMP4BoxWriterAppendBytes(&writer, "Video Handler", 14);
        if (err != kHAPError_None) {
            return err;
        }
    } else {
        err = HAPMP4BoxWriterAppendBytes(&writer, "Sound Handler", 14);
        if (err != kHAPError_None) {
            return err;
        }
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write video media header (vmhd) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteVideoMediaHeaderBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "vmhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 1.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Graphics mode.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Op colors.
    err = HAPMP4BoxWriterAppendZero(&writer, 6);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write sound media header (smhd) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteSoundMediaHeaderBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "smhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 1.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Balance (8.8).
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write data entry url (url) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteDataEntryUrlBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "url ");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = same file.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 1);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write data reference (dref) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteDataReferenceBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "dref");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Entry count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Data entry box.
    err = HAPFragmentedMP4WriteDataEntryUrlBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write data information (dinf) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteDataInformationBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "dinf");
    if (err != kHAPError_None) {
        return err;
    }
    // Data reference box.
    err = HAPFragmentedMP4WriteDataReferenceBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write elementary stream descriptor (esds) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteElementaryStreamDescriptorBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;

    uint16_t frequency = 8;
    switch (config->audio.sampleRate) {
        case 8000:
            frequency = 11;
            break;
        case 16000:
            frequency = 8;
            break;
        case 24000:
            frequency = 6;
            break;
        case 32000:
            frequency = 5;
            break;
        case 44100:
            frequency = 4;
            break;
        case 48000:
            frequency = 3;
            break;
    }

    uint32_t decoderSpecificLength = 2;
    uint32_t decoderSpecificData = 0;
    uint8_t streamType = kESDSStreamTypeAudio << 2U | 1U;
    switch (config->audio.type) {
        case kHAPFragmentedMP4AudioType_AAC_LC: {
            decoderSpecificLength = 2;
            decoderSpecificData =
                    ((uint32_t) kAudioObjectType_AAC_LC << 11U |       // 5 bit object type
                     (uint32_t) frequency << 7U |                      // 4 bit frequency
                     (uint32_t) config->audio.numberOfChannels << 3U | // 4 bit channels
                     0U);                                              // 3 bit flags
            streamType = kESDSStreamTypeAudio << 2U | 1U;
            break;
        }
        case kHAPFragmentedMP4AudioType_AAC_ELD: {
            decoderSpecificLength = 4;
            decoderSpecificData =
                    ((uint32_t) kAudioObjectType_ESC << 27U | // 5 + 6 bit object type
                     (uint32_t)(kAudioObjectType_AAC_ELD - kAudioObjectType_OFFSET) << 21U |
                     (uint32_t) frequency << 17U |                      // 4 bit frequency
                     (uint32_t) config->audio.numberOfChannels << 13U | // 4 bit channels
                     1U << 12U);                                        // 13 bit flags
            streamType = kESDSStreamTypeAudio << 2U | 0U;
            break;
        }
    }

    err = HAPMP4BoxWriterOpenLocal(&writer, container, "esds");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // ES descriptor tag.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kESDSTag_ESDescriptor);
    if (err != kHAPError_None) {
        return err;
    }
    // Length.
    err = HAPMP4BoxWriterAppendUInt8(&writer, (uint8_t)(23 + decoderSpecificLength));
    if (err != kHAPError_None) {
        return err;
    }
    // ES ID.
    err = HAPMP4BoxWriterAppendUInt16(&writer, kESDSES_ID);
    if (err != kHAPError_None) {
        return err;
    }
    // Flags.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Decoder configuration tag.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kESDSTag_Configuration);
    if (err != kHAPError_None) {
        return err;
    }
    // Length.
    err = HAPMP4BoxWriterAppendUInt8(&writer, (uint8_t)(15 + decoderSpecificLength));
    if (err != kHAPError_None) {
        return err;
    }
    // Profile.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kESDSAudioProfileAAC);
    if (err != kHAPError_None) {
        return err;
    }
    // Stream type / upstream / reserved.
    err = HAPMP4BoxWriterAppendUInt8(&writer, streamType);
    if (err != kHAPError_None) {
        return err;
    }
    // Buffer size (24 bit).
    err = HAPMP4BoxWriterAppendUInt8(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Max bitrate.
    err = HAPMP4BoxWriterAppendUInt32(&writer, config->audio.bitRate);
    if (err != kHAPError_None) {
        return err;
    }
    // Avg bitrate.
    err = HAPMP4BoxWriterAppendUInt32(&writer, config->audio.bitRate);
    if (err != kHAPError_None) {
        return err;
    }
    // Decoder specific tag.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kESDSTag_DecoderSpecific);
    if (err != kHAPError_None) {
        return err;
    }
    // Decoder specific length.
    err = HAPMP4BoxWriterAppendUInt8(&writer, (uint8_t) decoderSpecificLength);
    if (err != kHAPError_None) {
        return err;
    }
    // Decoder specific data.
    if (decoderSpecificLength == 2) {
        err = HAPMP4BoxWriterAppendUInt16(&writer, (uint16_t) decoderSpecificData);
    } else {
        HAPAssert(decoderSpecificLength == 4);
        err = HAPMP4BoxWriterAppendUInt32(&writer, decoderSpecificData);
    }
    if (err != kHAPError_None) {
        return err;
    }
    // SL configuration descriptor tag.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kESDSTag_SLConfiguration);
    if (err != kHAPError_None) {
        return err;
    }
    // SL configuration descriptor length.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // SL configuration descriptor.
    err = HAPMP4BoxWriterAppendUInt8(&writer, kSLConfigDescriptor);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write AVC configuration (avcC) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteAVCConfigurationBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);
    HAPPrecondition(config);
    HAPPrecondition(config->video.naluSizeBytes > 0 && config->video.naluSizeBytes <= 4);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "avcC");
    if (err != kHAPError_None) {
        return err;
    }
    // Configuration version.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Profile / flags / level from SPS.
    err = HAPMP4BoxWriterAppendBytes(&writer, config->video.sps.bytes + 1, 3);
    if (err != kHAPError_None) {
        return err;
    }
    // Number of NALU size bytes - 1.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 0xFC + config->video.naluSizeBytes - 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Number of SPS.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 0xE0 + 1);
    if (err != kHAPError_None) {
        return err;
    }
    // SPS NALU length.
    err = HAPMP4BoxWriterAppendUInt16(&writer, config->video.sps.numBytes);
    if (err != kHAPError_None) {
        return err;
    }
    // SPS NALU bytes.
    err = HAPMP4BoxWriterAppendBytes(&writer, config->video.sps.bytes, config->video.sps.numBytes);
    if (err != kHAPError_None) {
        return err;
    }
    // Number of PPS.
    err = HAPMP4BoxWriterAppendUInt8(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // PPS NALU length.
    err = HAPMP4BoxWriterAppendUInt16(&writer, config->video.pps.numBytes);
    if (err != kHAPError_None) {
        return err;
    }
    // PPS NALU bytes.
    err = HAPMP4BoxWriterAppendBytes(&writer, config->video.pps.bytes, config->video.pps.numBytes);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write AVC decoder (avc1) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteAVCDecoderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "avc1");
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 6);
    if (err != kHAPError_None) {
        return err;
    }
    // Data reference Index.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Version.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Revision level.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Vendor.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Temporal quality.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Spatial quality.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Width.
    err = HAPMP4BoxWriterAppendUInt16(&writer, config->video.width);
    if (err != kHAPError_None) {
        return err;
    }
    // Height.
    err = HAPMP4BoxWriterAppendUInt16(&writer, config->video.height);
    if (err != kHAPError_None) {
        return err;
    }
    // Horizontal resolution (16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, (uint32_t) 72U << 16U); // 72.0
    if (err != kHAPError_None) {
        return err;
    }
    // Vertical resolution (16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, (uint32_t) 72U << 16U); // 72.0
    if (err != kHAPError_None) {
        return err;
    }
    // Entry data size.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Frames per sample.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Compressor name.
    err = HAPMP4BoxWriterAppendBytes(&writer, h264Name, 32);
    if (err != kHAPError_None) {
        return err;
    }
    // Bit depth.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 24);
    if (err != kHAPError_None) {
        return err;
    }
    // Color table index.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0xFFFF);
    if (err != kHAPError_None) {
        return err;
    }
    // AVC configuration box.
    err = HAPFragmentedMP4WriteAVCConfigurationBox(&writer, config);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write AAC decoder (mp4a) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteAACDecoderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mp4a");
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 6);
    if (err != kHAPError_None) {
        return err;
    }
    // Data reference Index.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    // Version.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Revision level.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Vendor.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Channels.
    err = HAPMP4BoxWriterAppendUInt16(&writer, config->audio.numberOfChannels);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample size.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 16); // 16 bits
    if (err != kHAPError_None) {
        return err;
    }
    // Compression id.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample rate (16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, (uint32_t) config->audio.sampleRate << 16U);
    if (err != kHAPError_None) {
        return err;
    }
    // Elementary stream descriptor box.
    err = HAPFragmentedMP4WriteElementaryStreamDescriptorBox(&writer, config);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write sample description (stsd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteSampleDescriptionBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stsd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Entry count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 1);
    if (err != kHAPError_None) {
        return err;
    }
    if (trackId == kVideoTrackId) {
        // AVC decoder box.
        err = HAPFragmentedMP4WriteAVCDecoderBox(&writer, config);
        if (err != kHAPError_None) {
            return err;
        }
    } else {
        // AAC decoder box.
        err = HAPFragmentedMP4WriteAACDecoderBox(&writer, config);
        if (err != kHAPError_None) {
            return err;
        }
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write sample size (stsz) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteSampleSizeBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stsz");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample size.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write sample to chunk (stsc) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteSampleToChunkBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stsc");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Entry Count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write time to sample (stts) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTimeToSampleBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stts");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Entry Count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write chunk offset (stco) box.
 *
 * @param      container            The writer to write the box to.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteChunkOffsetBox(HAPMP4BoxWriter* container) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stco");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Entry Count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write sample table (stbl) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteSampleTableBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "stbl");
    if (err != kHAPError_None) {
        return err;
    }
    // Sample description box.
    err = HAPFragmentedMP4WriteSampleDescriptionBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample size box.
    err = HAPFragmentedMP4WriteSampleSizeBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample to chunk box.
    err = HAPFragmentedMP4WriteSampleToChunkBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    // Time to sample box.
    err = HAPFragmentedMP4WriteTimeToSampleBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    // Chunk offset box.
    err = HAPFragmentedMP4WriteChunkOffsetBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write media information (minf) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMediaInformationBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "minf");
    if (err != kHAPError_None) {
        return err;
    }
    if (trackId == kVideoTrackId) {
        // Video media header box.
        err = HAPFragmentedMP4WriteVideoMediaHeaderBox(&writer);
        if (err != kHAPError_None) {
            return err;
        }
    } else {
        // Sound media header box.
        err = HAPFragmentedMP4WriteSoundMediaHeaderBox(&writer);
        if (err != kHAPError_None) {
            return err;
        }
    }
    // Data information box.
    err = HAPFragmentedMP4WriteDataInformationBox(&writer);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample table box.
    err = HAPFragmentedMP4WriteSampleTableBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write media (mdia) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMediaBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mdia");
    if (err != kHAPError_None) {
        return err;
    }
    // Media header box.
    err = HAPFragmentedMP4WriteMediaHeaderBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Handler reference box.
    err = HAPFragmentedMP4WriteHandlerReferenceBox(&writer, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Media information box.
    err = HAPFragmentedMP4WriteMediaInformationBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write track header (tkhd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackHeaderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "tkhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = track_enabled | track_in_movie | track_in_preview.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 7);
    if (err != kHAPError_None) {
        return err;
    }
    // Creation time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Modification time (0 for first sample).
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Track ID.
    err = HAPMP4BoxWriterAppendUInt32(&writer, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 4);
    if (err != kHAPError_None) {
        return err;
    }
    // Duration.
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 8);
    if (err != kHAPError_None) {
        return err;
    }
    // Layer.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Alternate group.
    err = HAPMP4BoxWriterAppendUInt16(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Volume (audio only, 8.8).
    err = HAPMP4BoxWriterAppendUInt16(&writer, trackId == kAudioTrackId ? 1U << 8U : 0U); // 1.0
    if (err != kHAPError_None) {
        return err;
    }
    // Reserved.
    err = HAPMP4BoxWriterAppendZero(&writer, 2);
    if (err != kHAPError_None) {
        return err;
    }
    // Transformation matrix.
    err = HAPMP4BoxWriterAppendBytes(&writer, unityMatrix, sizeof unityMatrix);
    if (err != kHAPError_None) {
        return err;
    }
    // Width (video only, 16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, trackId == kVideoTrackId ? (uint32_t) config->video.width << 16U : 0U);
    if (err != kHAPError_None) {
        return err;
    }
    // Height (video only, 16.16).
    err = HAPMP4BoxWriterAppendUInt32(&writer, trackId == kVideoTrackId ? (uint32_t) config->video.height << 16U : 0U);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write track (trak) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The recording configuration used.
 * @param      trackId              The id of the current track.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the segment.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4RecordingConfiguration* config,
        uint32_t trackId) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "trak");
    if (err != kHAPError_None) {
        return err;
    }
    // Track header box.
    err = HAPFragmentedMP4WriteTrackHeaderBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Media box.
    err = HAPFragmentedMP4WriteMediaBox(&writer, config, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write movie (moov) box.
 *
 * @param[out] bytes                The buffer to write the data to.
 * @param      maxBytes             The capacity of the buffer.
 * @param[out] numBytes             The number of bytes written.
 * @param      config               The recording configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieBox(
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        const HAPFragmentedMP4RecordingConfiguration* config) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpen(&writer, bytes, maxBytes, "moov");
    if (err != kHAPError_None) {
        return err;
    }
    // Movie header box.
    err = HAPFragmentedMP4WriteMovieHeaderBox(&writer, config);
    if (err != kHAPError_None) {
        return err;
    }
    // Video track box.
    err = HAPFragmentedMP4WriteTrackBox(&writer, config, kVideoTrackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Audio track box.
    if (config->audio.numberOfChannels) {
        err = HAPFragmentedMP4WriteTrackBox(&writer, config, kAudioTrackId);
        if (err != kHAPError_None) {
            return err;
        }
    }
    // Movie extends box.
    err = HAPFragmentedMP4WriteMovieExtendsBox(&writer, config);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, numBytes);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPFragmentedMP4WriteMovieHeader(
        const HAPFragmentedMP4RecordingConfiguration* config,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(config);
    HAPPrecondition(config->video.sps.bytes);
    HAPPrecondition(config->video.pps.bytes);

    *numBytes = 0;
    size_t length;
    HAPError err;
    // File type box.
    err = HAPFragmentedMP4WriteFileTypeBox(bytes, maxBytes, &length);
    if (err != kHAPError_None) {
        return err;
    }
    bytes = (void*) ((uint8_t*) bytes + length);
    maxBytes -= length;
    *numBytes += length;

    // Movie box.
    err = HAPFragmentedMP4WriteMovieBox(bytes, maxBytes, &length, config);
    if (err != kHAPError_None) {
        return err;
    }
    *numBytes += length;

    return kHAPError_None;
}

// ---------- Fragments ----------

/**
 * Write movie fragment header (mfhd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The fragment configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieFragmentHeaderBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4FragmentConfiguration* config) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "mfhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Sequence number.
    err = HAPMP4BoxWriterAppendUInt32(&writer, config->sequenceNumber);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Gets the sample flags encoding options.
 *
 * @param      run                  The descriptor of the run.
 *
 * @return                          The options needed to store the sample flags.
 */
HAP_RESULT_USE_CHECK
static SampleFlagOptions HAPFragmentedMP4SampleFlagOptions(HAPFragmentedMP4RunDescriptor* run) {
    HAPPrecondition(run);
    HAPPrecondition(run->sampleTable);

    if (run->trackType != kHAPFragmentedMP4TrackType_Video) {
        return kSampleFlagOptions_None;
    }

    size_t i;
    for (i = 1; i < run->numSamples; i++) {
        if (run->sampleTable[i].iFrameFlag) {
            // Contains multiple key frames.
            return kSampleFlagOptions_Table;
        }
    }
    if (run->sampleTable[0].iFrameFlag) {
        // Contains an initial key frame.
        return (SampleFlagOptions)((uint8_t) kSampleFlagOptions_Default | (uint8_t) kSampleFlagOptions_First);
    }
    return kSampleFlagOptions_Default;
}

/**
 * Write track fragment header (tfhd) box.
 *
 * @param      container            The writer to write the box to.
 * @param      trackId              The id of the current track.
 * @param      flagOptions          The flag options to use.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackFragmentHeaderBox(
        HAPMP4BoxWriter* container,
        uint32_t trackId,
        SampleFlagOptions flagOptions) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "tfhd");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = (default-sample-flags-present) | default-base-is-moof.
    err = HAPMP4BoxWriterAppendFullBoxHeader(
            &writer,
            0,
            (uint32_t)(
                    flagOptions & (uint8_t) kSampleFlagOptions_Default ? kFragmentFlag_DefaultSampleFlagsPresent : 0U) |
                    (uint32_t) kFragmentFlag_DefaultBaseIsMoof);
    if (err != kHAPError_None) {
        return err;
    }
    // Track ID.
    err = HAPMP4BoxWriterAppendUInt32(&writer, trackId);
    if (err != kHAPError_None) {
        return err;
    }
    // Default Sample Flags.
    if (flagOptions & (uint8_t) kSampleFlagOptions_Default) {
        err = HAPMP4BoxWriterAppendUInt32(&writer, kSampleFlag_VideoNonIFrame);
        if (err != kHAPError_None) {
            return err;
        }
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write track fragment decode time (tfdt) box.
 *
 * @param      container            The writer to write the box to.
 * @param      decodeTime           The decode time.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackFragmentDecodeTimeBox(HAPMP4BoxWriter* container, uint64_t decodeTime) {
    HAPPrecondition(container);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "tfdt");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 1, flags = 0.
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 1, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // Base media decode time.
    err = HAPMP4BoxWriterAppendUInt64(&writer, decodeTime);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}

/**
 * Write track fragment run (trun) box.
 *
 * @param      container            The writer to write the box to.
 * @param      run                  The descriptor of the run.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError
        HAPFragmentedMP4WriteTrackFragmentRunBox(HAPMP4BoxWriter* container, HAPFragmentedMP4RunDescriptor* run) {
    HAPPrecondition(container);
    HAPPrecondition(run);
    HAPPrecondition(run->sampleTable);

    SampleFlagOptions flagOptions = HAPFragmentedMP4SampleFlagOptions(run);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "trun");
    if (err != kHAPError_None) {
        return err;
    }
    // Version = 0, flags = data-offset-present | (first-sample-flags-present) |
    //                      sample-duration-present | sample-size-present | (sample-flags-present).
    FragmentRunFlag flags = 0;
    flags |= (uint16_t) kFragmentRunFlag_DataOffsetPresent;
    if (flagOptions & (uint8_t) kSampleFlagOptions_First) {
        flags |= (uint16_t) kFragmentRunFlag_FirstSampleFlagPresent;
    }
    flags |= (uint16_t) kFragmentRunFlag_SampleDurationPresent;
    flags |= (uint16_t) kFragmentRunFlag_SampleSizePresent;
    if (flagOptions & (uint8_t) kSampleFlagOptions_Table) {
        flags |= (uint16_t) kFragmentRunFlag_SampleFlagsPresent;
    }
    err = HAPMP4BoxWriterAppendFullBoxHeader(&writer, 0, flags);
    if (err != kHAPError_None) {
        return err;
    }
    // Sample count.
    err = HAPMP4BoxWriterAppendUInt32(&writer, run->numSamples);
    if (err != kHAPError_None) {
        return err;
    }
    // Data offset = start of data - start of moof box.
    // Actual offset set in HAPFragmentedMP4FixupTrackFragmentRunBox.
    run->fixup = writer.position;
    err = HAPMP4BoxWriterAppendUInt32(&writer, 0);
    if (err != kHAPError_None) {
        return err;
    }
    // First sample flags.
    if (flagOptions & (uint8_t) kSampleFlagOptions_First) {
        err = HAPMP4BoxWriterAppendUInt32(&writer, kSampleFlag_VideoIFrame);
        if (err != kHAPError_None) {
            return err;
        }
    }
    // Sample table.
    size_t i, numRunBytes = 0;
    for (i = 0; i < run->numSamples; i++) {
        // Sample duration.
        err = HAPMP4BoxWriterAppendUInt32(&writer, run->sampleTable[i].duration);
        if (err != kHAPError_None) {
            return err;
        }
        // Sample size.
        err = HAPMP4BoxWriterAppendUInt32(&writer, run->sampleTable[i].size);
        if (err != kHAPError_None) {
            return err;
        }
        // Sample flags.
        if (flagOptions & (uint8_t) kSampleFlagOptions_Table) {
            if (run->sampleTable[i].iFrameFlag) {
                err = HAPMP4BoxWriterAppendUInt32(&writer, kSampleFlag_VideoIFrame);
            } else {
                err = HAPMP4BoxWriterAppendUInt32(&writer, kSampleFlag_VideoNonIFrame);
            }
            if (err != kHAPError_None) {
                return err;
            }
        }
        numRunBytes += run->sampleTable[i].size;
    }
    HAPMP4BoxWriterClose(&writer, NULL);
    HAPAssert(run->numRunDataBytes == numRunBytes);

    return kHAPError_None;
}

/**
 * Fixup offset in track fragment run (trun) box.
 *
 * @param      run                  The descriptor of the run.
 * @param      offset               The offset to be written to the box.
 */
static void HAPFragmentedMP4FixupTrackFragmentRunBox(HAPFragmentedMP4RunDescriptor* run, size_t offset) {
    HAPAssert(run);
    HAPAssert(run->fixup);
    HAPWriteBigUInt32(run->fixup, (uint32_t) offset);
}

#if HAP_FRAGMENTED_MP4_ONE_TRAF_PER_TRAK
/**
 * Write track fragment (traf) box.
 *
 * @param      container            The writer to write the box to.
 * @param      config               The fragment configuration.
 * @param      trackType            The track type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackFragmentBox(
        HAPMP4BoxWriter* container,
        const HAPFragmentedMP4FragmentConfiguration* config,
        HAPFragmentedMP4TrackType trackType) {
    HAPPrecondition(container);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;

    uint32_t trackId;
    SampleFlagOptions flagOptions;
    if (trackType == kHAPFragmentedMP4TrackType_Video) {
        trackId = kVideoTrackId;
        flagOptions = kSampleFlagOptions_Default;
    } else {
        trackId = kAudioTrackId;
        flagOptions = kSampleFlagOptions_None;
    }

    bool isOpen = false;
    size_t i;
    for (i = 0; i < config->numRuns; i++) {
        HAPFragmentedMP4RunDescriptor* run = &config->run[i];
        if (run->trackType == trackType) {
            if (!isOpen) {
                err = HAPMP4BoxWriterOpenLocal(&writer, container, "traf");
                if (err != kHAPError_None) {
                    return err;
                }
                // Track fragment header box.
                err = HAPFragmentedMP4WriteTrackFragmentHeaderBox(&writer, trackId, flagOptions);
                if (err != kHAPError_None) {
                    return err;
                }
                // Track fragment decode time box.
                err = HAPFragmentedMP4WriteTrackFragmentDecodeTimeBox(&writer, run->decodeTime);
                if (err != kHAPError_None) {
                    return err;
                }
                isOpen = true;
            }
            // Track fragment run box.
            err = HAPFragmentedMP4WriteTrackFragmentRunBox(&writer, run);
            if (err != kHAPError_None) {
                return err;
            }
        }
    }
    if (isOpen) {
        HAPMP4BoxWriterClose(&writer, NULL);
    }

    return kHAPError_None;
}
#else
/**
 * Write track fragment (traf) box.
 *
 * @param      container            The writer to write the box to.
 * @param      run                  The descriptor of the run.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteTrackFragmentBox(HAPMP4BoxWriter* container, HAPFragmentedMP4RunDescriptor* run) {
    HAPPrecondition(container);
    HAPPrecondition(run);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpenLocal(&writer, container, "traf");
    if (err != kHAPError_None) {
        return err;
    }
    // Track fragment header box.
    err = HAPFragmentedMP4WriteTrackFragmentHeaderBox(
            &writer,
            run->trackType == kHAPFragmentedMP4TrackType_Video ? kVideoTrackId : kAudioTrackId,
            HAPFragmentedMP4SampleFlagOptions(run));
    if (err != kHAPError_None) {
        return err;
    }
    // Track fragment decode time box.
    err = HAPFragmentedMP4WriteTrackFragmentDecodeTimeBox(&writer, run->decodeTime);
    if (err != kHAPError_None) {
        return err;
    }
    // Track fragment run box.
    err = HAPFragmentedMP4WriteTrackFragmentRunBox(&writer, run);
    if (err != kHAPError_None) {
        return err;
    }
    HAPMP4BoxWriterClose(&writer, NULL);

    return kHAPError_None;
}
#endif

/**
 * Write movie fragment box (moof) box.
 *
 * @param[out] bytes                The buffer to write the data to.
 * @param      maxBytes             The capacity of the buffer.
 * @param[out] numBytes             The number of bytes written.
 * @param      config               The fragment configuration used.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is not large enough to hold the data.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPFragmentedMP4WriteMovieFragmentBox(
        void* bytes,
        size_t maxBytes,
        size_t* numBytes,
        const HAPFragmentedMP4FragmentConfiguration* config) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(config);

    HAPMP4BoxWriter writer;
    HAPError err;
    err = HAPMP4BoxWriterOpen(&writer, bytes, maxBytes, "moof");
    if (err != kHAPError_None) {
        return err;
    }
    // Movie fragment header box.
    err = HAPFragmentedMP4WriteMovieFragmentHeaderBox(&writer, config);
    if (err != kHAPError_None) {
        return err;
    }

#if HAP_FRAGMENTED_MP4_ONE_TRAF_PER_TRAK
    err = HAPFragmentedMP4WriteTrackFragmentBox(&writer, config, kHAPFragmentedMP4TrackType_Video);
    if (err != kHAPError_None) {
        return err;
    }
    err = HAPFragmentedMP4WriteTrackFragmentBox(&writer, config, kHAPFragmentedMP4TrackType_Audio);
    if (err != kHAPError_None) {
        return err;
    }
#else
    size_t i;
    for (i = 0; i < config->numRuns; i++) {
        // Movie track fragment box.
        err = HAPFragmentedMP4WriteTrackFragmentBox(&writer, &config->run[i]);
        if (err != kHAPError_None) {
            return err;
        }
    }
#endif
    HAPMP4BoxWriterClose(&writer, numBytes);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPFragmentedMP4WriteFragmentHeader(
        const HAPFragmentedMP4FragmentConfiguration* config,
        void* bytes,
        size_t maxBytes,
        size_t* numBytes) {
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);
    HAPPrecondition(config);

    *numBytes = 0;
    size_t fragmentBoxSize;
    HAPError err;
    // Movie fragment box.
    err = HAPFragmentedMP4WriteMovieFragmentBox(bytes, maxBytes, &fragmentBoxSize, config);
    if (err != kHAPError_None) {
        return err;
    }
    bytes = (void*) ((uint8_t*) bytes + fragmentBoxSize);
    maxBytes -= fragmentBoxSize;
    *numBytes += fragmentBoxSize;

    // Media data box header.
    HAPMP4BoxWriter writer;
    err = HAPMP4BoxWriterOpen(&writer, bytes, maxBytes, "mdat");
    if (err != kHAPError_None) {
        return err;
    }
    size_t dataBoxSize;
    HAPMP4BoxWriterClose(&writer, &dataBoxSize);
    *numBytes += dataBoxSize; // Header only.

    // Fixup data offsets in trun boxes.
    // Data offset = start of data - start of moof box = size of moof + mdat header + preceding data.
    size_t i;
    for (i = 0; i < config->numRuns; i++) {
        HAPFragmentedMP4FixupTrackFragmentRunBox(&config->run[i], fragmentBoxSize + dataBoxSize);
        // Include data size of this run.
        dataBoxSize += config->run[i].numRunDataBytes;
    }

    // Set real data box size.
    HAPWriteBigUInt32(bytes, (uint32_t) dataBoxSize); // Header + data.

    return kHAPError_None;
}

#endif
