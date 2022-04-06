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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.
#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

/**
 * @file
 *
 * This file contains general video codec functions, such as:
 * -Get the text description of a video codec value.
 * -Is this data value a valid video codec type?
 * -Parse a video codec value received from a HAP characteristic into it's ADK equivalent.
 * -Convert a video codec value from it's ADK representation to it's HAP characteristic equivalent.
 */
#include "HAPVideo.h"

#include "HAPCharacteristicTypes.h"
#include "HAPLogSubsystem.h"
#include "HAPStringBuilder.h"

/**
 * Pixel limit for H.264 Profile Level 3.1.
 *
 * @see Recommendation ITU-T H.264 (10/2016)
 *     Table A-1 - Level limits
 *
 * Pixel limit is defined by the max frame rate, which is 3,600 MBs (macro blocks) = 3600*(16*16), which gives
 * a value of 921,600.
 */
#define kHAPH264VideoCodecProfileLevel_3_1_PixelLimit (921600)

/**
 * Pixel limit for H.264 Profile Level 3.2.
 *
 * @see Recommendation ITU-T H.264 (10/2016)
 *     Table A-1 - Level limits
 *
 * Pixel limit is defined by the max frame rate, which is 5,120 MBs (macro blocks) = 5120*(16*16), which gives
 * a value of 1,310,720.
 */
#define kHAPH264VideoCodecProfileLevel_3_2_PixelLimit (1310720)

/**
 * Pixel limit for H.264 Profile Level 4.
 *
 * @see Recommendation ITU-T H.264 (10/2016)
 *     Table A-1 - Level limits
 *
 * Pixel limit is defined by the max frame rate, which is 8,192 MBs (macro blocks) = 8,912*(16*16), which
 * gives a value of 2,097,152.
 */
#define kHAPH264VideoCodecProfileLevel_4_PixelLimit (2097152)

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "HAPVideo" };

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns text description of the video codec type.
 *
 * @param      videoCodecType       Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
const char* HAPVideoGetVideoCodecTypeDescription(HAPVideoCodecType videoCodecType) {
    switch (videoCodecType) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPVideoCodecType_H264: {
            return "H.264";
        }
        default: {
            HAPLogError(&logObject, "%s: Unknown video codec type: %u.", __func__, videoCodecType);
        }
            return "Unknown";
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Identifies whether or not a value represents a valid Profile ID.
 *
 * @param value          Value to be evaluated.
 *
 * @return true          If value is a valid HAPH264VideoCodecProfile.
 * @return false         Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPVideoH264ProfileIDIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPH264VideoCodecProfile));
    switch ((HAPH264VideoCodecProfile) value) {
        case kHAPH264VideoCodecProfile_ConstrainedBaseline:
        case kHAPH264VideoCodecProfile_Main:
        case kHAPH264VideoCodecProfile_High: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Identifies whether or not a value represents a valid Characteristic representation of a Profile ID.
 *
 * @param value          Value to be evaluated.
 *
 * @return true          If value is a valid HAPCharacteristicValue_H264VideoCodecProfile.
 * @return false         Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPVideo_HAPCharacteristicValue_H264VideoCodecProfileIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_H264VideoCodecProfile));
    switch ((HAPCharacteristicValue_H264VideoCodecProfile) value) {
        case kHAPCharacteristicValue_H264VideoCodecProfile_ConstrainedBaseline:
        case kHAPCharacteristicValue_H264VideoCodecProfile_Main:
        case kHAPCharacteristicValue_H264VideoCodecProfile_High: {
            return true;
        }
        default:
            return false;
    }
}

// String descriptions of H.264 Video Codec Profiles
#define kHAPH264VideoCodecProfile_ConstrainedBaselineDesc "Constrained Baseline Profile"
#define kHAPH264VideoCodecProfile_MainDesc                "Main Profile"
#define kHAPH264VideoCodecProfile_HighDesc                "High Profile"

/**
 * Returns text description of H.264 Profile ID.
 *
 * @param      h264ProfileID        Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
const char* HAPVideoGetH264ProfileIDDescription(HAPH264VideoCodecProfile h264ProfileID) {
    switch (h264ProfileID) {
        case kHAPH264VideoCodecProfile_ConstrainedBaseline:
            return kHAPH264VideoCodecProfile_ConstrainedBaselineDesc;
        case kHAPH264VideoCodecProfile_Main:
            return kHAPH264VideoCodecProfile_MainDesc;
        case kHAPH264VideoCodecProfile_High:
            return kHAPH264VideoCodecProfile_HighDesc;
        default: {
            HAPLogError(&logObject, "%s: Unknown profile ID type: %u.", __func__, h264ProfileID);
        }
            return "Unknown";
    }
}

/**
 * Appends the text descriptions of the profile IDs to the HAPStringBuilder passed in.
 *
 * @param stringBuilder      The StringBuilder to which the profile ID descriptions will be appended.
 * @param h264ProfileID      The flag containing the profileID bits.
 */
void HAPVideoStringBuilderAppendH264ProfileIDs(
        HAPStringBuilder* stringBuilder,
        HAPH264VideoCodecProfile h264ProfileID) {
    HAPPrecondition(stringBuilder);

    bool needsSeparator = false;
    if (h264ProfileID & (uint8_t) kHAPH264VideoCodecProfile_ConstrainedBaseline) {
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfile_ConstrainedBaselineDesc);
        needsSeparator = true;
    }
    if (h264ProfileID & (uint8_t) kHAPH264VideoCodecProfile_Main) {
        if (needsSeparator) {
            HAPStringBuilderAppend(stringBuilder, ", ");
        }
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfile_MainDesc);
        needsSeparator = true;
    }
    if (h264ProfileID & (uint8_t) kHAPH264VideoCodecProfile_High) {
        if (needsSeparator) {
            HAPStringBuilderAppend(stringBuilder, ", ");
        }
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfile_HighDesc);
    }
}

/**
 * Serializes H.264 Profile ID.
 *
 * Converts from ADK to Characteristic representation of value.
 *
 * @param      h264ProfileID        Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
HAPCharacteristicValue_H264VideoCodecProfile HAPVideoSerializeH264ProfileID(HAPH264VideoCodecProfile h264ProfileID) {
    switch (h264ProfileID) {
        case kHAPH264VideoCodecProfile_ConstrainedBaseline:
            return kHAPCharacteristicValue_H264VideoCodecProfile_ConstrainedBaseline;
        case kHAPH264VideoCodecProfile_Main:
            return kHAPCharacteristicValue_H264VideoCodecProfile_Main;
        case kHAPH264VideoCodecProfile_High:
            return kHAPCharacteristicValue_H264VideoCodecProfile_High;
        default:
            HAPFatalError();
    }
}

/**
 * Parses H.264 Profile ID.
 *
 * Converts from Characteristic to ADK representation of value.
 *
 * @param[out] h264ProfileID        Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
HAPError HAPVideoTryParseH264ProfileID(HAPH264VideoCodecProfile* h264ProfileID, uint8_t rawValue) {
    HAPPrecondition(h264ProfileID);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_H264VideoCodecProfile));
    switch ((HAPCharacteristicValue_H264VideoCodecProfile) rawValue) {
        case kHAPCharacteristicValue_H264VideoCodecProfile_ConstrainedBaseline: {
            *h264ProfileID = kHAPH264VideoCodecProfile_ConstrainedBaseline;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_H264VideoCodecProfile_Main: {
            *h264ProfileID = kHAPH264VideoCodecProfile_Main;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_H264VideoCodecProfile_High: {
            *h264ProfileID = kHAPH264VideoCodecProfile_High;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "H.264 Profile ID invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Identifies whether or not a value represents a valid Proflie Level.
 *
 * @param value          Value to be evaluated.
 *
 * @return true          If value is a valid HAPH264VideoCodecProfileLevel.
 * @return false         Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPVideoH264ProfileLevelIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPH264VideoCodecProfileLevel));
    switch ((HAPH264VideoCodecProfileLevel) value) {
        case kHAPH264VideoCodecProfileLevel_3_1:
        case kHAPH264VideoCodecProfileLevel_3_2:
        case kHAPH264VideoCodecProfileLevel_4: {
            return true;
        }
        default:
            return false;
    }
}

/**
 * Identifies whether or not a value represents a valid Characteristic representation of a Profile Level.
 *
 * @param value          Value to be evaluated.
 *
 * @return true          If value is a valid HAPCharacteristicValue_H264VideoCodecProfileLevel.
 * @return false         Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPVideo_HAPCharacteristicValue_H264VideoCodecProfileLevelIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPCharacteristicValue_H264VideoCodecProfileLevel));
    switch ((HAPCharacteristicValue_H264VideoCodecProfileLevel) value) {
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_1:
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_2:
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_4: {
            return true;
        }
        default:
            return false;
    }
}

// String descriptions of H.264 Video Codec Profile Levels
#define kHAPH264VideoCodecProfileLevel_3_1_Desc "3.1"
#define kHAPH264VideoCodecProfileLevel_3_2_Desc "3.2"
#define kHAPH264VideoCodecProfileLevel_4_Desc   "4"

/**
 * Returns text description of H.264 Level.
 *
 * @param      h264Level            Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
const char* HAPVideoGetH264ProfileLevelDescription(HAPH264VideoCodecProfileLevel h264Level) {
    switch (h264Level) {
        case kHAPH264VideoCodecProfileLevel_3_1:
            return kHAPH264VideoCodecProfileLevel_3_1_Desc;
        case kHAPH264VideoCodecProfileLevel_3_2:
            return kHAPH264VideoCodecProfileLevel_3_2_Desc;
        case kHAPH264VideoCodecProfileLevel_4:
            return kHAPH264VideoCodecProfileLevel_4_Desc;
        default: {
            HAPLogError(&logObject, "%s: Unknown profile level: %u.", __func__, h264Level);
        }
            return "Unknown";
    }
}

/**
 * Appends the text description of the profile levels to the HAPStringBuilder passed in.
 *
 * @param stringBuilder      The StringBuilder to which the profile ID descriptions will be appended.
 * @param h264Level          The flag containing the profile level bits.
 */
void HAPVideoStringBuilderAppendH264ProfileLevels(
        HAPStringBuilder* stringBuilder,
        HAPH264VideoCodecProfileLevel h264Level) {

    bool needsSeparator = false;
    if (h264Level & (uint8_t) kHAPH264VideoCodecProfileLevel_3_1) {
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfileLevel_3_1_Desc);
        needsSeparator = true;
    }
    if (h264Level & (uint8_t) kHAPH264VideoCodecProfileLevel_3_2) {
        if (needsSeparator) {
            HAPStringBuilderAppend(stringBuilder, ", ");
        }
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfileLevel_3_2_Desc);
        needsSeparator = true;
    }
    if (h264Level & (uint8_t) kHAPH264VideoCodecProfileLevel_4) {
        if (needsSeparator) {
            HAPStringBuilderAppend(stringBuilder, ", ");
        }
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecProfileLevel_4_Desc);
    }
}

/**
 * Returns pixel limit of H.264 Level.
 *
 * @see Recommendation ITU-T H.264 (10/2016)
 *     Table A-1 - Level limits
 *
 * The pixel limit is defined by the max frame rate of the profile level.
 * Note: The max frame rate is stated in referenced specification in MBs (macro blocks), so it needs to be
 * multipled by (16*16) to convert it to pixels.
 *
 * @param      h264Level            Value.
 *
 * @return    uint32_t              Value of pixel limit
 */
HAP_RESULT_USE_CHECK
uint32_t HAPVideoGetH264ProfileLevelPixelLimit(HAPH264VideoCodecProfileLevel h264Level) {
    switch (h264Level) {
        case kHAPH264VideoCodecProfileLevel_3_1:
            return kHAPH264VideoCodecProfileLevel_3_1_PixelLimit;
        case kHAPH264VideoCodecProfileLevel_3_2:
            return kHAPH264VideoCodecProfileLevel_3_2_PixelLimit;
        case kHAPH264VideoCodecProfileLevel_4:
            return kHAPH264VideoCodecProfileLevel_4_PixelLimit;
        default:
            return kHAPH264VideoCodecProfileLevel_4_PixelLimit;
    }
}

/**
 * Serializes H.264 Level.
 *
 * Converts from ADK to Characteristic representation of value.
 *
 * @param      h264Level            Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
HAPCharacteristicValue_H264VideoCodecProfileLevel
        HAPVideoSerializeH264ProfileLevel(HAPH264VideoCodecProfileLevel h264Level) {
    switch (h264Level) {
        case kHAPH264VideoCodecProfileLevel_3_1:
            return kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_1;
        case kHAPH264VideoCodecProfileLevel_3_2:
            return kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_2;
        case kHAPH264VideoCodecProfileLevel_4:
            return kHAPCharacteristicValue_H264VideoCodecProfileLevel_4;
        default:
            HAPFatalError();
    }
}

/**
 * Parses H.264 Level.
 *
 * Converts from Characteristic to ADK representation of value.
 *
 * @param[out] h264Level            Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
HAPError HAPVideoTryParseH264ProfileLevel(HAPH264VideoCodecProfileLevel* h264Level, uint8_t rawValue) {
    HAPPrecondition(h264Level);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_H264VideoCodecProfileLevel));
    switch ((HAPCharacteristicValue_H264VideoCodecProfileLevel) rawValue) {
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_1: {
            *h264Level = kHAPH264VideoCodecProfileLevel_3_1;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_2: {
            *h264Level = kHAPH264VideoCodecProfileLevel_3_2;
        }
            return kHAPError_None;
        case kHAPCharacteristicValue_H264VideoCodecProfileLevel_4: {
            *h264Level = kHAPH264VideoCodecProfileLevel_4;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "H.264 Level invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Identifies whether or not a value represents a valid Packetization Mode.
 *
 * @param value          Value to be evaluated.
 *
 * @return true          If value is a valid HAPH264VideoCodecPacketizationMode.
 * @return false         Otherwise.
 */
HAP_RESULT_USE_CHECK
HAP_RESULT_USE_CHECK
bool HAPVideoH264PacketizationModeIsValid(uint8_t value) {
    HAPAssert(sizeof value == sizeof(HAPH264VideoCodecPacketizationMode));
    switch ((HAPH264VideoCodecPacketizationMode) value) {
        case kHAPH264VideoCodecPacketizationMode_NonInterleaved: {
            return true;
        }
        default:
            return false;
    }
}

// String descriptions of H.264 Video Codec Packetization Modes
#define kHAPH264VideoCodecPacketizationMode_NonInterleavedDesc ("Non-interleaved mode")

/**
 * Returns text description of H.264 Packetization mode.
 *
 * @param      h264PacketMode       Value.
 *
 * @return Description of value.
 */
HAP_RESULT_USE_CHECK
const char* HAPVideoGetH264PacketizationModeDescription(HAPH264VideoCodecPacketizationMode h264PacketMode) {
    switch (h264PacketMode) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPH264VideoCodecPacketizationMode_NonInterleaved:
            return kHAPH264VideoCodecPacketizationMode_NonInterleavedDesc;
        default: {
            HAPLogError(&logObject, "%s: Unknown Packetization Mode: %u.", __func__, h264PacketMode);
        }
            return "Unknown";
    }
}

/**
 * Appends the text description of the packetization modes to the HAPStringBuilder passed in.
 *
 * @param stringBuilder      The StringBuilder to which the profile ID descriptions will be appended.
 * @param h264PacketMode     The flag containing the packetization mode bits.
 */
void HAPVideoStringBuilderAppendH264PacketizationModes(
        HAPStringBuilder* stringBuilder,
        HAPH264VideoCodecPacketizationMode h264PacketMode) {
    HAPPrecondition(stringBuilder);

    if (h264PacketMode & (uint8_t) kHAPH264VideoCodecPacketizationMode_NonInterleaved) {
        HAPStringBuilderAppend(stringBuilder, kHAPH264VideoCodecPacketizationMode_NonInterleavedDesc);
    }
}

/**
 * Serializes H.264 Packetization mode.
 *
 * Converts from ADK to Characteristic representation of value.
 *
 * @param      h264PacketMode       Value.
 *
 * @return Serialized value.
 */
HAP_RESULT_USE_CHECK
HAPCharacteristicValue_H264VideoCodecPacketizationMode
        HAPVideoSerializeH264PacketizationMode(HAPH264VideoCodecPacketizationMode h264PacketMode) {
    switch (h264PacketMode) { // NOLINT(hicpp-multiway-paths-covered)
        case kHAPH264VideoCodecPacketizationMode_NonInterleaved:
            return kHAPCharacteristicValue_H264VideoCodecPacketizationMode_NonInterleaved;
        default:
            HAPFatalError();
    }
}

/**
 * Parses H.264 Packetization mode.
 *
 * Converts from Characteristic to ADK representation of value.
 *
 * @param[out] h264PacketMode       Parsed value.
 * @param      rawValue             Raw value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the value is invalid.
 */
HAP_RESULT_USE_CHECK
HAPError HAPVideoTryParseH264PacketizationMode(HAPH264VideoCodecPacketizationMode* h264PacketMode, uint8_t rawValue) {
    HAPPrecondition(h264PacketMode);

    HAPAssert(sizeof rawValue == sizeof(HAPCharacteristicValue_H264VideoCodecPacketizationMode));
    switch ((HAPCharacteristicValue_H264VideoCodecPacketizationMode) rawValue) {
        case kHAPCharacteristicValue_H264VideoCodecPacketizationMode_NonInterleaved: {
            *h264PacketMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved;
        }
            return kHAPError_None;
        default: {
            HAPLog(&logObject, "H.264 Packetization mode invalid: %u.", rawValue);
        }
            return kHAPError_InvalidData;
    }
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
