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

#if (HAP_TESTING == 1)
#include "App.h"
#include "CameraSimulationHandlers.h"

#ifdef DARWIN
/**
 * Stops recording, updates the root directory for the source of video/audio, and then starts recording again.
 *
 * @param rootDirectory         The new root directory for the source of video/audio.
 */
void UpdateCameraRecordingSource(const char* rootDirectory) {
    HAPPrecondition(rootDirectory);

    HAPError err;
    HAPPlatformCameraRecorderRef recorder = GetAccessoryCameraRecorder();
    // Stop recording to apply the new recording configuration.
    if (recorder->recordingState != kHAPPlatformCameraRecordingState_Disabled) {
        HAPPlatformCameraStopRecording(recorder);
    }

    // Update video/audio file path to be used for HAPPlatformCamera.
    err = ChangeMediaPathSetting(rootDirectory);
    if (err != kHAPError_None) {
        HAPLogInfo(&kHAPLog_Default, "Could not start change directory path");
    }

    // Start recording with updated video/audio file path.
    err = HAPPlatformCameraStartRecording(recorder);
    if (err != kHAPError_None) {
        HAPLogInfo(&kHAPLog_Default, "Could not start recording");
    }
}
#endif

HAP_RESULT_USE_CHECK
HAPError ProcessCommandLine(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPSession* session HAP_UNUSED,
        const HAPAccessory* accessory HAP_UNUSED,
        int numCommands,
        char** commands) {
    HAPPrecondition(commands);

    for (int index = 0; index < numCommands; index++) {
        const char* command = commands[index];
        if (HAPStringAreEqual(command, "triggerMotion")) {
            HAPLogInfo(&kHAPLog_Default, "Triggering motion");
#ifdef DARWIN
            if ((index + 2 < numCommands) && HAPStringAreEqual(commands[index + 1], "-m")) {
                const char* rootSourceDirectory = kMediaFilePath;
                // Looking for extended command "triggerMotion -m <path>"
                // Two additional arguments are needed, and the first additional one must be "-m"
                HAPLogInfo(&kHAPLog_Default, "Triggering motion: extended command");
                rootSourceDirectory = commands[index + 2];
                index += 2;
                UpdateCameraRecordingSource(rootSourceDirectory);
            }
#endif
            // Notify app that the motion sensor has been toggled.
            ToggleMotionSensorState();
        }
#ifdef DARWIN
        else if (HAPStringAreEqual(command, "triggerMotionWithoutMedia")) {
            HAPLogInfo(&kHAPLog_Default, "Triggering motion without associated media");
            HAPError err = ChangeMediaPathSetting(NULL);
            if (err != kHAPError_None) {
                HAPLogInfo(&kHAPLog_Default, "Could not start change directory path");
            }
            ToggleMotionSensorState();
        } else if (HAPStringAreEqual(command, "setNullMediaPath")) {
            HAPError err = ChangeMediaPathSetting(NULL);
            if (err != kHAPError_None) {
                HAPLogInfo(&kHAPLog_Default, "Could not start change directory path");
            }
        }
#endif
        else if (HAPStringAreEqual(command, "changeResolution")) {
            HAPLogInfo(&kHAPLog_Default, "Triggering resolution change");
            ChangeSupportedRecordingConfig();
        } else if (HAPStringAreEqual(command, "disableCamera")) {
            HAPLogInfo(&kHAPLog_Default, "Disable camera manually");
            SimulateSetCameraManuallyDisable(true);
        } else if (HAPStringAreEqual(command, "enableCamera")) {
            HAPLogInfo(&kHAPLog_Default, "Enable camera manually");
            SimulateSetCameraManuallyDisable(false);
        } else if (HAPStringAreEqual(command, "setThirdPartyInactive")) {
            HAPLogInfo(&kHAPLog_Default, "Third party inactive");
            SimulateSetThirdPartyActive(false);
        } else if (HAPStringAreEqual(command, "setThirdPartyActive")) {
            HAPLogInfo(&kHAPLog_Default, "Third party active");
            SimulateSetThirdPartyActive(true);
        } else {
            HAPLogError(&kHAPLog_Default, "Unknown command; ignoring");
            continue;
        }
    }
    return kHAPError_None;
}
#endif

HAP_DIAGNOSTIC_POP
