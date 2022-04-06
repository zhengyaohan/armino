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

// This debug command line serves development purposes only.
// It must not be included in production accessories and can be safely compiled out for production builds once no longer
// required for testing.

#include "HAP.h"

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

#include <string.h>
#include "AppBase.h"
#include "CameraSimulationHandlers.h"
#include "HAPAccessory+Camera.h"
#include "HAPPlatformCamera.h"
#include "HAPPlatformCameraInput+Init.h"
#include "HAPPlatformCameraInput.h"
#include "HAPPlatformCameraOperatingMode.h"
#include "HAPPlatformMicrophone+Init.h"
#include "HAPPlatformMicrophone.h"

extern const HAPUInt8Characteristic cameraOperatingModeManuallyDisabledCharacteristic;
extern const HAPService cameraOperatingModeService;
extern const HAPUInt8Characteristic cameraOperatingModeThirdPartyCameraActiveCharacteristic;

extern const HAPService cameraEventRecordingManagementService;

/**
 * Memory for camera/microphone media path. This is used in testing when the user specifies a particular video
 * path to use for a command(such as triggerMotion -m <file_path>).
 */
char rootMediaPath[PATH_MAX];

/**
 * Simulate Manual Camera Disable.
 */
void SimulateSetCameraManuallyDisable(bool value) {
    HAPError err = kHAPError_None;
    bool wasDisabled = false;

    const HAPAccessory* acc = AppGetAccessoryInfo();
    HAPAccessoryServer* appServer = AppGetAccessoryServer();

    err = HAPPlatformCameraIsManuallyDisabled(HAPCameraAccessoryGetCamera(appServer), &wasDisabled);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsManuallyDisabled failed: %u.", err);
        return;
    }
    if (value == wasDisabled) {
        return;
    }

    err = HAPPlatformCameraSetManuallyDisabled(HAPCameraAccessoryGetCamera(appServer), value);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraSetManuallyDisabled failed: %u.", err);
        return;
    }

    HAPAccessoryServerRaiseEvent(
            appServer, &cameraOperatingModeManuallyDisabledCharacteristic, &cameraOperatingModeService, acc);

    SimulateSetThirdPartyActive(!value);
}

/**
 * Simulate Third Party Active setting.
 */
void SimulateSetThirdPartyActive(bool value) {
    HAPError err = kHAPError_None;
    bool wasActive = true;

    const HAPAccessory* acc = AppGetAccessoryInfo();
    HAPAccessoryServer* appServer = AppGetAccessoryServer();

    err = HAPPlatformCameraIsThirdPartyCameraActive(HAPCameraAccessoryGetCamera(appServer), &wasActive);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraIsThirdPartyCameraActive failed: %u.", err);
        return;
    }

    if (value == wasActive) {
        return;
    }

    err = HAPPlatformCameraSetThirdPartyCameraActive(HAPCameraAccessoryGetCamera(appServer), value);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformCameraSetThirdPartyCameraActive failed: %u.", err);
        return;
    }

    HAPAccessoryServerRaiseEvent(
            appServer, &cameraOperatingModeThirdPartyCameraActiveCharacteristic, &cameraOperatingModeService, acc);
}

/**
 * Change supported recording configuration.
 *
 * Cycles through the following configurations:
 * - high resolution   (up to 1080p)
 * - low resolution    (up to 720p)
 * - lowest resolution (up to 480p)
 */
void SimulateChangeSupportedRecordingConfig(int* pMaxVideoRecordingResolution) {
    HAPPrecondition(pMaxVideoRecordingResolution);

    const HAPAccessory* acc = AppGetAccessoryInfo();
    HAPAccessoryServer* appServer = AppGetAccessoryServer();

    int prevRes = *pMaxVideoRecordingResolution;

    if (*pMaxVideoRecordingResolution > 720) {
        *pMaxVideoRecordingResolution = 720;
    } else if (*pMaxVideoRecordingResolution > 480) {
        *pMaxVideoRecordingResolution = 480;
    } else {
        *pMaxVideoRecordingResolution = 1080;
    }

    HAPLogInfo(
            &kHAPLog_Default,
            "Changing supported recording configuration from %d to %d",
            prevRes,
            *pMaxVideoRecordingResolution);

    HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
            appServer,
            &cameraEventRecordingManagementService,
            acc,
            kHAPCameraSupportedRecordingConfigurationChange_Video);
}

#ifdef DARWIN
/**
 * Return recorder associated with accessory camera.
 *
 * @return HAPPlatformCameraRecorderRef         Accessory's recorder
 */
HAPPlatformCameraRecorderRef GetAccessoryCameraRecorder(void) {
    HAPAccessoryServer* appServer = AppGetAccessoryServer();
    HAPPlatformCameraRef camera = HAPCameraAccessoryGetCamera(appServer);
    HAPAssert(camera);
    HAPPlatformCameraRecorderRef recorder = camera->recorder;
    HAPAssert(recorder);
    return recorder;
}

/**
 * Update accessory's root media path, used for camera and microphone.
 *
 * @param newMediaPath         Directory path for camera/microphone files.
 *
 * @return HAPError            kHAPError_None if success, kHAPError_InvalidData if path is too long.
 */
HAPError ChangeMediaPathSetting(const char* _Nullable newMediaPath) {
    HAPAccessoryServer* appServer = AppGetAccessoryServer();

    if (newMediaPath == NULL) {
        // If media path is null, we do not immediately start any video, and instead assume video data will come in
        // later
        appServer->platform.ip.camera->cameraInput->mediaPath = NULL;
        appServer->platform.ip.camera->microphone->mediaPath = NULL;
        return kHAPError_None;
    }

    HAPRawBufferZero(&rootMediaPath, sizeof(rootMediaPath));
    if (PATH_MAX < strlen(newMediaPath) + 1) {
        HAPLogError(
                &kHAPLog_Default,
                "Specified media path %lu is longer than available buffer %d.",
                strlen(newMediaPath) + 1,
                PATH_MAX);
        return kHAPError_InvalidData;
    }
    HAPRawBufferCopyBytes(&rootMediaPath, newMediaPath, strlen(newMediaPath) + 1);
    appServer->platform.ip.camera->cameraInput->mediaPath = (char*) &rootMediaPath;
    appServer->platform.ip.camera->microphone->mediaPath = (char*) &rootMediaPath;
    return kHAPError_None;
}

#endif // DARWIN
#endif // (HAP_TESTING == 1) && HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)

HAP_DIAGNOSTIC_POP
