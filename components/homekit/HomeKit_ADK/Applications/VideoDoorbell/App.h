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

// An accessory that represents a Video Doorbell.
//
// This header file is platform-independent.

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#include "HAPPlatformMicrophone.h"
#include "HAPPlatformSpeaker.h"

#include "ApplicationFeatures.h"
#include "CameraHelper.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Feature Configuration for accessory
 */
// Each application needs to define this in order to use data stream so undef any previous definitions
#undef HAP_APP_USES_HDS
#define HAP_APP_USES_HDS 1

#if (HAVE_FIRMWARE_UPDATE == 1)
// Each application needs to define this in order use stream protocol so undef any previous definitions
#undef HAP_APP_USES_HDS_STREAM
#define HAP_APP_USES_HDS_STREAM 1
#endif

// App doesn't support Thread
#undef HAVE_THREAD
#define HAVE_THREAD 0

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
HAP_RESULT_USE_CHECK
HAPError GetAccessoryDiagnosticsConfig(
        HAPAccessoryServer* _Nullable server,
        const HAPAccessory* _Nullable accessory,
        HAPAccessoryDiagnosticsConfig* diagnosticsConfigStruct,
        void* _Nullable context);
#endif

/**
 * Handle read request to the 'Programmable Switch Event' characteristic of the Doorbell service.
 *
 * For HAP over IP, this function will only be called after HAPAccessoryServerRaiseEvent is called by application logic.
 * Paired read will not trigger a call to this function. Refer to the ADK integration guide.
 */
HAP_RESULT_USE_CHECK
HAPError HandleDoorbellProgrammableSwitchEventRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

#if (HAVE_VIDEODOORBELL_SILENT_MODE == 1)
/**
 * Handle read request to the 'Mute' characteristic of the Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleChimeMuteRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Mute' characteristic of the Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleChimeMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context);
#endif

/**
 * Handle read request to the 'Mute' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Mute' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context);

/**
 * Handle read request to the 'Volume' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneVolumeRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Volume' characteristic of the Microphone service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMicrophoneVolumeWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context);

/**
 * Handle read request to the 'Mute' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerMuteRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Mute' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerMuteWrite(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context);

/**
 * Handle read request to the 'Volume' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerVolumeRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Volume' characteristic of the Speaker service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSpeakerVolumeWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request to the 'Motion Detected' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorMotionDetectedRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Motion Sensor service.
 */
HAPError HandleMotionSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
/**
 * Handle read request to the 'Operating State Response' characteristic of the Video Doorbell service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleVideoDoorbellOperatingStateResponseRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context);

HAP_RESULT_USE_CHECK
HAPError HandleOperatingStateResponse(
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        uint8_t operatingStateType,
        uint8_t abnormalReason);
#endif

void ToggleMotionSensorState(void);
void ChangeSupportedRecordingConfig(void);
void SimulateSinglePressEvent(void);
void SimulateDoublePressEvent(void);
void SimulateLongPressEvent(void);
#if (HAVE_VIDEODOORBELL_OPERATING_STATE == 1)
void SimulateOperatingStates(void);
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
