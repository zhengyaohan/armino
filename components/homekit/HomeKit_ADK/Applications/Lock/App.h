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

// The most basic HomeKit example: an accessory that represents a light bulb that
// only supports switching the light on and off. Actions are exposed as individual
// functions below.
//
// This header file is platform-independent.

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#include "ApplicationFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Feature Configuration for accessory
 */
#if (HAVE_FIRMWARE_UPDATE == 1)
// Each application needs to define this in order use data stream so undef any previous definitions
#undef HAP_APP_USES_HDS
#define HAP_APP_USES_HDS 1

// Each application needs to define this in order use stream protocol so undef any previous definitions
#undef HAP_APP_USES_HDS_STREAM
#define HAP_APP_USES_HDS_STREAM 1
#endif

#if (HAVE_DIAGNOSTICS_SERVICE == 1)
// Each application needs to define this in order to support firmware update so undef any previous definitions
#undef HAP_APP_USES_HDS
#define HAP_APP_USES_HDS 1

HAP_RESULT_USE_CHECK
HAPError GetAccessoryDiagnosticsConfig(
        HAPAccessoryServer* _Nullable server,
        const HAPAccessory* _Nullable accessory,
        HAPAccessoryDiagnosticsConfig* diagnosticsConfigStruct,
        void* _Nullable context);
#endif

#if (HAVE_ACCESSORY_REACHABILITY == 1)
/**
 * Sleep interval for this application
 */
#define kAccessorySleepInterval ((HAPTime)(2 * HAPSecond))
#endif

#if (HAVE_THREAD == 1)
/**
 * Thread device type:  MTD = Minimal / Sleepy device
 *                      FTD = Full thread device (Non sleepy)
 */
#define kThreadDeviceType (kHAPPlatformThreadDeviceCapabilities_MTD)
#endif
#if (HAVE_LOCK_ENC == 1)
HAPError HandleLockReadContextData(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        HAPTLVWriter* _Nullable* _Nullable writer,
        void* _Nullable context);

HAPError HandleLockReadContextDataCompact(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        uint8_t* buffer,
        size_t bufferSize,
        void* _Nullable context);
#endif
/**
 * Handle read request to the 'CurrentState' characteristic of the Lock Mechanism service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLockMechanismLockCurrentStateRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'TargetState' characteristic of the Lock Mechanism service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLockMechanismLockTargetStateRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle write request to the 'TargetState' characteristic of the Lock Mechanism service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLockMechanismLockTargetStateWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context);

/**
 * Handle read request to the 'Battery Level' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryLevelRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Charging State' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryChargingStateRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Low Battery' characteristic of the Battery service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleBatteryStatusLowRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

#if (HAVE_ACCESS_CODE == 1)
/**
 * Handle read request to the 'Active' characteristic of the Access Code service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeActiveRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle write request to the 'Active' characteristic of the Access Code service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeActiveWrite(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Verify the access code has been added previously. If valid access code, toggle the lock state.
 *
 * @param  accessCode The access code to validate
 */
void ValidateAccessCode(char* accessCode);
#endif
#endif

/**
 * Set the lock target state
 *
 * @param  targetState  The new target state
 */
void SetLockTargetState(HAPCharacteristicValue_LockTargetState targetState);

#if (HAP_TESTING == 1)
/**
 * Set the lock current state
 *
 * @param  newCurrentState  The new current state
 */
void SetLockCurrentState(HAPCharacteristicValue_LockCurrentState newCurrentState);

/**
 * Toggle the lock state, simulating user manually locking/unlocking a lock
 */
void ToggleLockState(void);

/**
 * Toggle if the lock should be jammed, simulating a lock that is unable to lock
 * or unlock due to a physical problem
 */
void ToggleLockJammed(void);

/**
 * Set the status low battery state
 *
 * @param  state  The new battery state
 */
void SetStatusLowBatteryState(HAPCharacteristicValue_StatusLowBattery state);
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
