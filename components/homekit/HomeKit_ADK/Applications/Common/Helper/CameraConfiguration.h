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

#ifndef CAMERA_CONFIGURATION_H
#define CAMERA_CONFIGURATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ApplicationFeatures.h"

#include "HAPBase.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if (HAVE_BATTERY_POWERED_RECORDER == 1)
#define kDefaultAppPrebufferDurationInMS ((HAPTime)(0 * HAPSecond))
#else
#define kDefaultAppPrebufferDurationInMS ((HAPTime)(4 * HAPSecond))
#endif
//----------------------------------------------------------------------------------------------------------------------
/**
 * H.264 video codec parameters.
 */
static const HAPH264VideoCodecParameters h264VideoCodecParameters = {
    .profile = kHAPH264VideoCodecProfile_Main,
    .level = kHAPH264VideoCodecProfileLevel_3_1 | kHAPH264VideoCodecProfileLevel_4,
    .packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved,
};

/**
 *Defines a constant structure named videoAttribute_param1_param2_param3, with the parameters as fields in the
 *structure.
 */
#define VIDEO_ATTRIBUTE(w, h, fps) \
    static const HAPVideoAttributes videoAttribute_##w##_##h##_##fps = { .width = w, .height = h, .maxFrameRate = fps }

/*
 * List of supported Resolutions
 */
// 16:9, 30 fps
VIDEO_ATTRIBUTE(1920, 1080, 30);
VIDEO_ATTRIBUTE(1280, 720, 30);
VIDEO_ATTRIBUTE(640, 360, 30);
VIDEO_ATTRIBUTE(480, 270, 30);
VIDEO_ATTRIBUTE(320, 180, 30);

// 4:3, 30 fps
VIDEO_ATTRIBUTE(1280, 960, 30);
VIDEO_ATTRIBUTE(1024, 768, 30);
VIDEO_ATTRIBUTE(640, 480, 30);
VIDEO_ATTRIBUTE(480, 360, 30);
VIDEO_ATTRIBUTE(320, 240, 30);

// 16:9, 24 fps
VIDEO_ATTRIBUTE(1920, 1080, 24);
VIDEO_ATTRIBUTE(1280, 720, 24);
VIDEO_ATTRIBUTE(640, 360, 24);
VIDEO_ATTRIBUTE(480, 270, 24);
VIDEO_ATTRIBUTE(320, 180, 24);

// 4:3, 24 fps
VIDEO_ATTRIBUTE(1280, 960, 24);
VIDEO_ATTRIBUTE(1024, 768, 24);
VIDEO_ATTRIBUTE(640, 480, 24);
VIDEO_ATTRIBUTE(480, 360, 24);
VIDEO_ATTRIBUTE(320, 240, 24);

// 16:9, 15 fps
VIDEO_ATTRIBUTE(1920, 1080, 15);
VIDEO_ATTRIBUTE(1280, 720, 15);
VIDEO_ATTRIBUTE(640, 360, 15);
VIDEO_ATTRIBUTE(480, 270, 15);
VIDEO_ATTRIBUTE(320, 180, 15);

// 4:3, 15 fps
VIDEO_ATTRIBUTE(1280, 960, 15);
VIDEO_ATTRIBUTE(1024, 768, 15);
VIDEO_ATTRIBUTE(640, 480, 15);
VIDEO_ATTRIBUTE(480, 360, 15);
VIDEO_ATTRIBUTE(320, 240, 15);

// 4:3, 30 fps
VIDEO_ATTRIBUTE(1600, 1200, 30);
VIDEO_ATTRIBUTE(1440, 1080, 30);

// 9:16, 30 fps
VIDEO_ATTRIBUTE(1080, 1920, 30);
VIDEO_ATTRIBUTE(720, 1280, 30);
VIDEO_ATTRIBUTE(360, 640, 30);
VIDEO_ATTRIBUTE(270, 480, 30);
VIDEO_ATTRIBUTE(180, 320, 30);

// 3:4, 30 fps
VIDEO_ATTRIBUTE(1200, 1600, 30);
VIDEO_ATTRIBUTE(1080, 1440, 30);
VIDEO_ATTRIBUTE(960, 1280, 30);
VIDEO_ATTRIBUTE(768, 1024, 30);
VIDEO_ATTRIBUTE(480, 640, 30);
VIDEO_ATTRIBUTE(360, 480, 30);
VIDEO_ATTRIBUTE(240, 320, 30);

// 1:1, 30 fps
VIDEO_ATTRIBUTE(1080, 1080, 30);
VIDEO_ATTRIBUTE(720, 720, 30);
VIDEO_ATTRIBUTE(320, 320, 30);

// 4:3, 24 fps
VIDEO_ATTRIBUTE(1600, 1200, 24);
VIDEO_ATTRIBUTE(1440, 1080, 24);

// 9:16, 24 fps
VIDEO_ATTRIBUTE(1080, 1920, 24);
VIDEO_ATTRIBUTE(720, 1280, 24);
VIDEO_ATTRIBUTE(360, 640, 24);
VIDEO_ATTRIBUTE(270, 480, 24);
VIDEO_ATTRIBUTE(180, 320, 24);

// 3:4, 24 fps
VIDEO_ATTRIBUTE(1200, 1600, 24);
VIDEO_ATTRIBUTE(1080, 1440, 24);
VIDEO_ATTRIBUTE(960, 1280, 24);
VIDEO_ATTRIBUTE(768, 1024, 24);
VIDEO_ATTRIBUTE(480, 640, 24);
VIDEO_ATTRIBUTE(360, 480, 24);
VIDEO_ATTRIBUTE(240, 320, 24);

// 1:1, 24 fps
VIDEO_ATTRIBUTE(1080, 1080, 24);
VIDEO_ATTRIBUTE(720, 720, 24);
VIDEO_ATTRIBUTE(320, 320, 24);

// 4:3, 15 fps
VIDEO_ATTRIBUTE(1600, 1200, 15);
VIDEO_ATTRIBUTE(1440, 1080, 15);

// 9:16, 15 fps
VIDEO_ATTRIBUTE(1080, 1920, 15);
VIDEO_ATTRIBUTE(720, 1280, 15);
VIDEO_ATTRIBUTE(360, 640, 15);
VIDEO_ATTRIBUTE(270, 480, 15);
VIDEO_ATTRIBUTE(180, 320, 15);

// 3:4, 15 fps
VIDEO_ATTRIBUTE(1200, 1600, 15);
VIDEO_ATTRIBUTE(1080, 1440, 15);
VIDEO_ATTRIBUTE(960, 1280, 15);
VIDEO_ATTRIBUTE(768, 1024, 15);
VIDEO_ATTRIBUTE(480, 640, 15);
VIDEO_ATTRIBUTE(360, 480, 15);
VIDEO_ATTRIBUTE(240, 320, 15);

// 1:1  15 fps
VIDEO_ATTRIBUTE(1080, 1080, 15);
VIDEO_ATTRIBUTE(720, 720, 15);
VIDEO_ATTRIBUTE(320, 320, 15);

/*
 *  Additional Resolutions
 *  These resolutions are supported by ADK, but are not currrently compatible with Raspi, which is the example
 * implementation for camera.
 *
 *  Note that the resolutions below require H.264 Profile Level 5, and Raspi supports up to H.264 Profile Level 4.2, so
 * these resolutions are not compatible with Raspi (see the HAPVideoAttributes data structures defined below for
 * details).
 */
// 16:9, 30 fps
VIDEO_ATTRIBUTE(2048, 1536, 30);

// 4:3, 30 fps
VIDEO_ATTRIBUTE(3840, 2160, 30);
VIDEO_ATTRIBUTE(2560, 1440, 30);

// 9:16, 30 fps
VIDEO_ATTRIBUTE(1536, 2048, 30);

// 3:4, 30 fps
VIDEO_ATTRIBUTE(2160, 3840, 30);
VIDEO_ATTRIBUTE(1440, 2560, 30);

// 1:1, 30 fps
VIDEO_ATTRIBUTE(1536, 1536, 30);

// 16:9, 24 fps
VIDEO_ATTRIBUTE(2048, 1536, 24);

// 4:3, 24 fps
VIDEO_ATTRIBUTE(3840, 2160, 24);
VIDEO_ATTRIBUTE(2560, 1440, 24);

// 9:16, 24 fps
VIDEO_ATTRIBUTE(1536, 2048, 24);

// 3:4, 24 fps
VIDEO_ATTRIBUTE(2160, 3840, 24);
VIDEO_ATTRIBUTE(1440, 2560, 24);

// 1:1, 24 fps
VIDEO_ATTRIBUTE(1536, 1536, 24);

// 16:9, 15 fps
VIDEO_ATTRIBUTE(2048, 1536, 15);

// 4:3, 15 fps
VIDEO_ATTRIBUTE(3840, 2160, 15);
VIDEO_ATTRIBUTE(2560, 1440, 15);

// 9:16, 15 fps
VIDEO_ATTRIBUTE(1536, 2048, 15);

// 3:4, 15 fps
VIDEO_ATTRIBUTE(2160, 3840, 15);
VIDEO_ATTRIBUTE(1440, 2560, 15);

// 1:1  15 fps
VIDEO_ATTRIBUTE(1536, 1536, 15);

/**
 * Full screen streaming video parameters, with 30fps max frame rate. All resolutions present in this data
 * structure MUST conform to H.264 Profile Level 4. This is because this data structure is used for the example Raspi
 * implementation, which does not support above Profile Level 4 (note: Raspi can support up to H.264 Profile
 * Level 4.2).
 *
 * To see if a resolution qualifies to be added here, make sure the max frame size of H.264 Profile Level 4 is
 * greater than the width * height of the resolution.
 * @see Specification: Recommendation ITU-T H.264 (10/2016), Section A.3.1, e)
 *
 * Example: Max frame rate for H.264 Profile Level 4 = 8,192 MBs (macro blocks) = 8,912*(16*16), which gives
 * us a pixel limit of 2,097,152. Sample resolution of 1080x1920p = 2,073,600. The width * height (2,073,600)
 * is less than the H.264 Profile Level 4 pixel limit (2,097,152), so we can add this resolution to the data
 * structure.
 */
static const HAPVideoAttributes* _Nonnull fullVideoAttributes[] = {
#if (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_1920_1080_30, // 1080p @ 30 (16:9)
    &videoAttribute_1280_720_30,  //  720p @ 30 (16:9)
    &videoAttribute_640_360_30,   //  360p @ 30 (16:9)
    &videoAttribute_480_270_30,   //  270p @ 30 (16:9)
    &videoAttribute_320_180_30,   //  180p @ 30 (16:9)
    &videoAttribute_1280_960_30,  //  960p @ 30 (4:3)
    &videoAttribute_1024_768_30,  //  XVGA @ 30 (4:3)
    &videoAttribute_640_480_30,   //   VGA @ 30 (4:3)
    &videoAttribute_480_360_30,   //  360p @ 30 (4:3)
    &videoAttribute_320_240_30,   //  240p @ 30 (4:3)
    &videoAttribute_1600_1200_30, //  UXGA @ 30 (4:3)
    &videoAttribute_1440_1080_30, //   HDV @ 30 (4:3)
#else                             // (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_1080_1920_30, // 1080p @ 30 (9:16)
    &videoAttribute_720_1280_30,  //  720p @ 30 (9:16)
    &videoAttribute_360_640_30,   //  360p @ 30 (9:16)
    &videoAttribute_270_480_30,   //  270p @ 30 (9:16)
    &videoAttribute_180_320_30,   //  180p @ 30 (9:16)
    &videoAttribute_1200_1600_30, //  UXGA @ 30 (3:4)
    &videoAttribute_1080_1440_30, //   HDV @ 30 (3:4)
    &videoAttribute_960_1280_30,  //  960p @ 30 (3:4)
    &videoAttribute_768_1024_30,  //  XVGA @ 30 (3:4)
    &videoAttribute_480_640_30,   //   VGA @ 30 (3:4)
    &videoAttribute_360_480_30,   //  360p @ 30 (3:4)
    &videoAttribute_240_320_30,   //  240p @ 30 (3:4)
#endif                            // (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_1080_1080_30, // 1080p @ 30 (1:1)
    &videoAttribute_720_720_30,   //  720p @ 30 (1:1)
    &videoAttribute_320_320_30,   //  320p @ 30 (1:1)
    NULL,
};

/**
 * Small screen video parameters, with 30fps max frame rate. All resolutions present in this data
 * structure are a resolution of up to (not including) 1080p.
 *
 * For our purposes, in this data structure, resolutions up to (not including) 1080p means that
 * 1) If the aspect ratio is greater than 1, the height must be less than 1080.
 * 2) If the aspect ratio is less than 1, the width must be less than 1080.
 * 3) If the aspect ratio is 1, the width and heigh must be less than 1080.
 */
static const HAPVideoAttributes* _Nonnull smallVideoAttributes[] = {
#if (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_1280_720_30, //  720p @ 30 (16:9)
    &videoAttribute_640_360_30,  //  360p @ 30 (16:9)
    &videoAttribute_480_270_30,  //  270p @ 30 (16:9)
    &videoAttribute_320_180_30,  //  180p @ 30 (16:9)
    // Note: no 960p resolution
    &videoAttribute_1024_768_30, //  XVGA @ 30 (4:3)
    &videoAttribute_640_480_30,  //   VGA @ 30 (4:3)
    &videoAttribute_480_360_30,  //  360p @ 30 (4:3)
    &videoAttribute_320_240_30,  //  240p @ 30 (4:3)
#else                            // (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_720_1280_30,  //  720p @ 30 (9:16)
    &videoAttribute_360_640_30,   //  360p @ 30 (9:16)
    &videoAttribute_270_480_30,   //  270p @ 30 (9:16)
    &videoAttribute_180_320_30,   //  180p @ 30 (9:16)
    // Note: no 960p resolution
    &videoAttribute_768_1024_30, //  XVGA @ 30 (3:4)
    &videoAttribute_480_640_30,  //   VGA @ 30 (3:4)
    &videoAttribute_360_480_30,  //  360p @ 30 (3:4)
    &videoAttribute_240_320_30,  //  240p @ 30 (3:4)
#endif                           // (HAVE_PORTRAIT_MODE == 0)
    &videoAttribute_720_720_30,  //  720p @ 30 (1:1)
    &videoAttribute_320_320_30,  //  320p @ 30 (1:1)
    NULL,
};

/**
 * Recording video parameters, with exact frame rates. All resolutions present in this data structure MUST
 * conform to H.264 Profile Level 4. This is because this data structure is used for the example Raspi
 * implementation, which does not support above Profile Level 4 (note: Raspi can support up to H.264 Profile
 * Level 4.2).
 *
 * To see if a resolution qualifies to be added here, make sure the max frame size of H.264 Profile Level 4 is
 * greater than the width * height of the resolution.
 * @see Specification: Recommendation ITU-T H.264 (10/2016), Section A.3.1, e)
 *
 * Example: Max frame rate for H.264 Profile Level 4 = 8,192 MBs (macro blocks) = 8,912*(16*16), which gives
 * us a pixel limit of 2,097,152. Sample resolution of 1080x1920p = 2,073,600. The width * height (2,073,600)
 * is less than the H.264 Profile Level 4 pixel limit (2,097,152), so we can add this resolution to the data
 * structure.
 */
static const HAPVideoAttributes* _Nonnull recordingVideoAttributes[] = {
#if (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_1920_1080_30, // 1080p @ 30 (16:9)
    &videoAttribute_1280_720_30,  //  720p @ 30 (16:9)
    &videoAttribute_640_360_30,   //  360p @ 30 (16:9)
    &videoAttribute_1280_960_30,  //  960p @ 30 (4:3)
    &videoAttribute_1024_768_30,  //  XVGA @ 30 (4:3)
    &videoAttribute_640_480_30,   //   VGA @ 30 (4:3)
    &videoAttribute_1600_1200_30, //  UXGA @ 30 (4:3)
    &videoAttribute_1440_1080_30, //   HDV @ 30 (4:3)

    // 24 fps
    &videoAttribute_1920_1080_24, // 1080p @ 24 (16:9)
    &videoAttribute_1280_720_24,  //  720p @ 24 (16:9)
    &videoAttribute_640_360_24,   //  360p @ 24 (16:9)
    &videoAttribute_1280_960_24,  //  960p @ 24 (4:3)
    &videoAttribute_1024_768_24,  //  XVGA @ 24 (4:3)
    &videoAttribute_640_480_24,   //   VGA @ 24 (4:3)
    &videoAttribute_1600_1200_24, //  UXGA @ 24 (4:3)
    &videoAttribute_1440_1080_24, //   HDV @ 24 (4:3)

    // 15 fps
    &videoAttribute_1920_1080_15, // 1080p @ 15 (16:9)
    &videoAttribute_1280_720_15,  //  720p @ 15 (16:9)
    &videoAttribute_640_360_15,   //  360p @ 15 (16:9)
    &videoAttribute_1280_960_15,  //  960p @ 15 (4:3)
    &videoAttribute_1024_768_15,  //  XVGA @ 15 (4:3)
    &videoAttribute_640_480_15,   //   VGA @ 15 (4:3)
    &videoAttribute_1600_1200_15, //  UXGA @ 15 (4:3)
    &videoAttribute_1440_1080_15, //   HDV @ 15 (4:3)
#else                             // (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_1080_1920_30, // 1080p @ 30 (9:16)
    &videoAttribute_720_1280_30,  //  720p @ 30 (9:16)
    &videoAttribute_360_640_30,   //  360p @ 30 (9:16)
    &videoAttribute_1200_1600_30, //  UXGA @ 30 (3:4)
    &videoAttribute_1080_1440_30, //   HDV @ 30 (3:4)
    &videoAttribute_960_1280_30,  //  960p @ 30 (3:4)
    &videoAttribute_768_1024_30,  //  XVGA @ 30 (3:4)
    &videoAttribute_480_640_30,   //   VGA @ 30 (3:4)

    // 24 fps
    &videoAttribute_1080_1920_24, // 1080p @ 24 (9:16)
    &videoAttribute_720_1280_24,  //  720p @ 24 (9:16)
    &videoAttribute_360_640_24,   //  360p @ 24 (9:16)
    &videoAttribute_1200_1600_24, //  UXGA @ 24 (3:4)
    &videoAttribute_1080_1440_24, //   HDV @ 24 (3:4)
    &videoAttribute_960_1280_24,  //  960p @ 24 (3:4)
    &videoAttribute_768_1024_24,  //  XVGA @ 24 (3:4)
    &videoAttribute_480_640_24,   //   VGA @ 24 (3:4)

    // 15 fps
    &videoAttribute_1080_1920_15, // 1080p @ 15 (9:16)
    &videoAttribute_720_1280_15,  //  720p @ 15 (9:16)
    &videoAttribute_360_640_15,   //  360p @ 15 (9:16)
    &videoAttribute_1200_1600_15, //  UXGA @ 15 (3:4)
    &videoAttribute_1080_1440_15, //   HDV @ 15 (3:4)
    &videoAttribute_960_1280_15,  //  960p @ 15 (3:4)
    &videoAttribute_768_1024_15,  //  XVGA @ 15 (3:4)
    &videoAttribute_480_640_15,   //   VGA @ 15 (3:4)
#endif                            // (HAVE_PORTRAIT_MODE == 0)
    NULL,
};

/**
 * Recording low resolution video parameters, with exact frame rates. All resolutions present in this data
 * structure are a resolution of up to (not including) 1080p.
 *
 * For our purposes, in this data structure, resolutions up to (not including) 1080p means that:
 * 1) If the aspect ratio is greater than 1, the height must be less than 1080.
 * 2) If the aspect ratio is less than 1, the width must be less than 1080.
 * 3) If the aspect ratio is 1, the width and heigh must be less than 1080.
 */
static const HAPVideoAttributes* _Nonnull recordingLowResVideoAttributes[] = {
#if (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_1280_720_30, //  720p @ 30 (16:9)
    &videoAttribute_640_360_30,  //  360p @ 30 (16:9)
    &videoAttribute_1024_768_30, //  XVGA @ 30 (4:3)
    &videoAttribute_640_480_30,  //   VGA @ 30 (4:3)

    // 24 fps
    &videoAttribute_1280_720_24, //  720p @ 24 (16:9)
    &videoAttribute_640_360_24,  //  360p @ 24 (16:9)
    &videoAttribute_1024_768_24, //  XVGA @ 24 (4:3)
    &videoAttribute_640_480_24,  //   VGA @ 24 (4:3)

    // 15 fps
    &videoAttribute_1280_720_15, //  720p @ 15 (16:9)
    &videoAttribute_640_360_15,  //  360p @ 15 (16:9)
    &videoAttribute_1024_768_15, //  XVGA @ 15 (4:3)
    &videoAttribute_640_480_15,  //   VGA @ 15 (4:3)

#else  // (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_720_1280_30, //  720p @ 30 (9:16)
    &videoAttribute_360_640_30,  //  360p @ 30 (9:16)
    &videoAttribute_768_1024_30, //  XVGA @ 30 (3:4)
    &videoAttribute_480_640_30,  //   VGA @ 30 (3:4)

    // 24 fps
    &videoAttribute_720_1280_24, //  720p @ 24 (9:16)
    &videoAttribute_360_640_24,  //  360p @ 24 (9:16)
    &videoAttribute_768_1024_24, //  XVGA @ 24 (3:4)
    &videoAttribute_480_640_24,  //   VGA @ 24 (3:4)

    // 15 fps
    &videoAttribute_720_1280_15, //  720p @ 15 (9:16)
    &videoAttribute_360_640_15,  //  360p @ 15 (9:16)
    &videoAttribute_768_1024_15, //  XVGA @ 15 (3:4)
    &videoAttribute_480_640_15,  //   VGA @ 15 (3:4)
#endif // (HAVE_PORTRAIT_MODE == 0)
    NULL,
};

/**
 * Recording lowest resolution video parameters, with exact frame rates. All resolutions present in this data
 * structure are a resolution of up to (not including) 720p.
 *
 * For our purposes, in this data structure, resolutions up to (not including) 720p means that:
 * 1) If the aspect ratio is greater than 1, the height must be less than 720.
 * 2) If the aspect ratio is less than 1, the width must be less than 720.
 * 3) If the aspect ratio is 1, the width and heigh must be less than 720.
 */
static const HAPVideoAttributes* _Nonnull recordingLowestResVideoAttributes[] = {
#if (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_640_360_30, //  360p @ 30 (16:9)
    &videoAttribute_640_480_30, //   VGA @ 30 (4:3)

    // 24 fps
    &videoAttribute_640_360_24, //  360p @ 24 (16:9)
    &videoAttribute_640_480_24, //   VGA @ 24 (4:3)

    // 15 fps
    &videoAttribute_640_360_15, //  360p @ 15 (16:9)
    &videoAttribute_640_480_15, //   VGA @ 15 (4:3)
#else                           // (HAVE_PORTRAIT_MODE == 0)
    // 30 fps
    &videoAttribute_360_640_30, //  360p @ 30 (9:16)
    &videoAttribute_480_640_30, //   VGA @ 30 (3:4)

    // 24 fps
    &videoAttribute_360_640_24, //  360p @ 24 (9:16)
    &videoAttribute_480_640_24, //   VGA @ 24 (3:4)

    // 15 fps
    &videoAttribute_360_640_15, //  360p @ 15 (9:16)
    &videoAttribute_480_640_15, //   VGA @ 15 (3:4)
#endif                          // (HAVE_PORTRAIT_MODE == 0)
    NULL,
};

/**
 * H.264 video codec configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264FullVideoCodecConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecParameters,
    .attributes = fullVideoAttributes
};

/**
 * H.264 video codec configuration with restricted resolution
 */
static const HAPCameraSupportedVideoCodecConfiguration h264SmallVideoCodecConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecParameters,
    .attributes = smallVideoAttributes
};

/**
 * Opus audio codec parameters
 */
static const HAPAudioCodecParameters varAudioCodecParameters = {
    .numberOfChannels = 1,
    .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
    .sampleRate = (HAPAudioCodecSampleRate)(kHAPAudioCodecSampleRate_16KHz | kHAPAudioCodecSampleRate_24KHz)
};

/**
 * Opus audio configuration
 */
static const HAPCameraSupportedAudioCodecConfiguration opusAudioCodecConfiguration = {
    .codecType = kHAPAudioCodecType_Opus,
    .codecParameters = &varAudioCodecParameters
};

/**
 * First video stream:
 *  - Video: H264, Level 3.1 and 4, all resolutions
 *  - Audio: Opus, 16kHz and 24kHz sample rate
 */
const HAPCameraStreamSupportedConfigurations supportedCameraStreamConfigurations0 = {
    .videoStream = { .configurations =
                             (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                                     &h264FullVideoCodecConfiguration,
                                     NULL } },
    .audioStream = { .configurations =
                             (const HAPCameraSupportedAudioCodecConfiguration* const[]) { &opusAudioCodecConfiguration,
                                                                                          NULL },
                     .comfortNoise = { .supported = false } },
    .rtp = { .srtpCryptoSuites = (HAPSRTPCryptoSuite)(
                     kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 | kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80 |
                     kHAPSRTPCryptoSuite_Disabled) }
};

/**
 * Second video stream:
 *  - Video: H264, Level 3.1 and 4, up to 720p resolution
 *  - Audio: Opus, 16kHz and 24kHz sample rate
 */
const HAPCameraStreamSupportedConfigurations supportedCameraStreamConfigurations1 = {
    .videoStream = { .configurations =
                             (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                                     &h264SmallVideoCodecConfiguration,
                                     NULL } },
    .audioStream = { .configurations =
                             (const HAPCameraSupportedAudioCodecConfiguration* const[]) { &opusAudioCodecConfiguration,
                                                                                          NULL },
                     .comfortNoise = { .supported = false } },
    .rtp = { .srtpCryptoSuites = (HAPSRTPCryptoSuite)(
                     kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 | kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80 |
                     kHAPSRTPCryptoSuite_Disabled) }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fragmented MP4 container parameters.
 */
static const HAPFragmentedMP4MediaContainerParameters fragmentedMP4ContainerParameters = {
    .fragmentDuration = kHAPPlatformCameraRecorder_MaxFragmentDuration // ms
};

/**
 * Fragmented MP4 container configuration.
 */
static const HAPCameraSupportedMediaContainerConfiguration fragmentedMP4ContainerConfiguration = {
    .containerType = kHAPMediaContainerType_FragmentedMP4,
    .containerParameters = &fragmentedMP4ContainerParameters
};

/**
 * H.264 video codec recording parameters.
 */
static const HAPH264VideoCodecParameters h264VideoCodecRecordingParameters = {
    .profile = kHAPH264VideoCodecProfile_Main,
    .level = kHAPH264VideoCodecProfileLevel_3_1 | kHAPH264VideoCodecProfileLevel_4,
    .packetizationMode = kHAPH264VideoCodecPacketizationMode_NonInterleaved
};

/**
 * H.264 video codec recording configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264VideoCodecRecordingConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecRecordingParameters,
    .attributes = recordingVideoAttributes
};

/**
 * H.264 video codec low resolution recording configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264VideoCodecLowResRecordingConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecRecordingParameters,
    .attributes = recordingLowResVideoAttributes
};

/**
 * H.264 video codec lowest resolution recording configuration.
 */
static const HAPCameraSupportedVideoCodecConfiguration h264VideoCodecLowestResRecordingConfiguration = {
    .codecType = kHAPVideoCodecType_H264,
    .codecParameters = &h264VideoCodecRecordingParameters,
    .attributes = recordingLowestResVideoAttributes
};

/**
 * Audio codec recording parameters.
 */
static const HAPAudioCodecParameters varAudioCodecRecordingParameters = {
    .numberOfChannels = 1,
    .bitRateMode = kHAPAudioCodecBitRateControlMode_Variable,
    .sampleRate = (HAPAudioCodecSampleRate)(kHAPAudioCodecSampleRate_16KHz | kHAPAudioCodecSampleRate_24KHz)
};

/**
 * AAC-LC audio recording configuration.
 */
static const HAPCameraSupportedAudioCodecConfiguration aacLcAudioCodecRecordingConfiguration = {
    .codecType = kHAPAudioCodecType_AAC_LC,
    .codecParameters = &varAudioCodecRecordingParameters
};
#if (HAP_CAMERA_AAC_ELD == 1)

/**
 * AAC-ELD audio recording configuration.
 */
static const HAPCameraSupportedAudioCodecConfiguration aacEldAudioCodecRecordingConfiguration = {
    .codecType = kHAPAudioCodecType_AAC_ELD,
    .codecParameters = &varAudioCodecRecordingParameters
};
#endif

/**
 * Recording configuration.
 */
HAPCameraRecordingSupportedConfiguration supportedCameraRecordingConfiguration = {
    .recording = { .prebufferDuration = kDefaultAppPrebufferDurationInMS,
                   .eventTriggerTypes = kHAPCameraEventTriggerTypes_Motion,
                   .containerConfigurations =
                           (const HAPCameraSupportedMediaContainerConfiguration* const[]) {
                                   &fragmentedMP4ContainerConfiguration,
                                   NULL } },
    .video = { .configurations =
                       (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                               &h264VideoCodecRecordingConfiguration,
                               NULL } },
    .audio = { .configurations =
                       (const HAPCameraSupportedAudioCodecConfiguration* const[]) {
                               &aacLcAudioCodecRecordingConfiguration,
#if (HAP_CAMERA_AAC_ELD == 1)
                               &aacEldAudioCodecRecordingConfiguration,
#endif
                               NULL } }
};

/**
 * Low resolution recording configuration.
 */
HAPCameraRecordingSupportedConfiguration supportedLowResCameraRecordingConfiguration = {
    .recording = { .prebufferDuration = kDefaultAppPrebufferDurationInMS,
                   .eventTriggerTypes = kHAPCameraEventTriggerTypes_Motion,
                   .containerConfigurations =
                           (const HAPCameraSupportedMediaContainerConfiguration* const[]) {
                                   &fragmentedMP4ContainerConfiguration,
                                   NULL } },
    .video = { .configurations =
                       (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                               &h264VideoCodecLowResRecordingConfiguration,
                               NULL } },
    .audio = { .configurations =
                       (const HAPCameraSupportedAudioCodecConfiguration* const[]) {
                               &aacLcAudioCodecRecordingConfiguration,
#if (HAP_CAMERA_AAC_ELD == 1)
                               &aacEldAudioCodecRecordingConfiguration,
#endif
                               NULL } }
};

/**
 * Lowest resolution recording configuration.
 */
HAPCameraRecordingSupportedConfiguration supportedLowestResCameraRecordingConfiguration = {
    .recording = { .prebufferDuration = kDefaultAppPrebufferDurationInMS,
                   .eventTriggerTypes = kHAPCameraEventTriggerTypes_Motion,
                   .containerConfigurations =
                           (const HAPCameraSupportedMediaContainerConfiguration* const[]) {
                                   &fragmentedMP4ContainerConfiguration,
                                   NULL } },
    .video = { .configurations =
                       (const HAPCameraSupportedVideoCodecConfiguration* const[]) {
                               &h264VideoCodecLowestResRecordingConfiguration,
                               NULL } },
    .audio = { .configurations =
                       (const HAPCameraSupportedAudioCodecConfiguration* const[]) {
                               &aacLcAudioCodecRecordingConfiguration,
#if (HAP_CAMERA_AAC_ELD == 1)
                               &aacEldAudioCodecRecordingConfiguration,
#endif
                               NULL } }
};

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
