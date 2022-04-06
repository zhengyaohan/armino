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

// An example that implements a Sensor HomeKit profile. Actions on the Sensor are exposed as
// individual functions listed below.
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
// Each application needs to define this in order to use data stream so undef any previous definitions
#undef HAP_APP_USES_HDS
#define HAP_APP_USES_HDS 1

// Each application needs to define this in order use stream protocol so undef any previous definitions
#undef HAP_APP_USES_HDS_STREAM
#define HAP_APP_USES_HDS_STREAM 1
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

#if (SENSOR_AIR_QUALITY == 1)
/**
 * Handle read request to the 'Air Quality' characteristic of the Air Quality service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAirQualitySensorAirQualityRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Air Quality Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAirQualitySensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the air quality state
 *
 * @param   newState   New air quality state
 */
void SetAirQualityState(HAPCharacteristicValue_AirQuality newState);
#endif
#endif

#if (SENSOR_CARBON_DIOXIDE == 1)
/**
 * Handle read request to the 'Carbon Dioxide Detected' characteristic of the Carbon Dioxide Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCarbonDioxideSensorCarbonDioxideDetectedRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Carbon Dioxide Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCarbonDioxideSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the carbon dioxide detected state
 *
 * @param   newState   New carbon dioxide state
 */
void SetCarbonDioxideDetectedState(HAPCharacteristicValue_CarbonDioxideDetected newState);
#endif
#endif

#if (SENSOR_CARBON_MONOXIDE == 1)
/**
 * Handle read request to the 'Carbon Monoxide Detected' characteristic of the Carbon Monoxide Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCarbonMonoxideSensorCarbonMonoxideDetectedRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Carbon Monoxide Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleCarbonMonoxideSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the carbon monoxide detected state
 *
 * @param   newState   New carbon monoxide state
 */
void SetCarbonMonoxideDetectedState(HAPCharacteristicValue_CarbonMonoxideDetected newState);
#endif
#endif

#if (SENSOR_CONTACT == 1)
/**
 * Handle read request to the 'Contact Sensor State' characteristic of the Contact Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleContactSensorContactSensorStateRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Contact Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleContactSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set through the contact sensor state
 *
 * @param   newState   New contact sensor state
 */
void SetContactSensorState(HAPCharacteristicValue_ContactSensorState newState);
#endif
#endif

#if (SENSOR_HUMIDITY == 1)
/**
 * Handle read request to the 'Current Relative Humidity' characteristic of the Humidity Sensor service.
 */
HAPError HandleHumiditySensorCurrentRelativeHumidityRead(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Humidity Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleHumiditySensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);
#endif

#if (SENSOR_LEAK == 1)
/**
 * Handle read request to the 'Leak Detected' characteristic of the Leak Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLeakSensorLeakDetectedRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Leak Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLeakSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the leak detected state
 *
 * @param   newState   New leak detected state
 */
void SetLeakDetectedState(HAPCharacteristicValue_LeakDetected newState);
#endif
#endif

#if (SENSOR_LIGHT == 1)
/**
 * Handle read request to the 'Current Ambient Light Level' characteristic of the Light Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightSensorCurrentAmbientLightLevelRead(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Light Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);
#endif

#if (SENSOR_MOTION == 1)
/**
 * Handle read request to the 'Motion Detected' characteristic of the Motion Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMotionSensorMotionDetectedRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Motion Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleMotionSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Toggle the motion detected state
 */
void ToggleMotionDetectedState(void);
#endif
#endif

#if (SENSOR_OCCUPANCY == 1)
/**
 * Handle read request to the 'Occupancy Detected' characteristic of the Occupancy Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleOccupancySensorOccupancyDetectedRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Occupancy Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleOccupancySensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the occupancy detected state
 *
 * @param   newState   New occupancy detected state
 */
void SetOccupancyDetectedState(HAPCharacteristicValue_OccupancyDetected newState);
#endif
#endif

#if (SENSOR_SMOKE == 1)
/**
 * Handle read request to the 'Smoke Detected' characteristic of the Smoke Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSmokeSensorSmokeDetectedRead(
        HAPAccessoryServer* server,
        const HAPUInt8CharacteristicReadRequest* request,
        uint8_t* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Smoke Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleSmokeSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);

#if (HAP_TESTING == 1)
/**
 * Set the smoke detected state
 *
 * @param   newState   New smoke detected state
 */
void SetSmokeDetectedState(HAPCharacteristicValue_SmokeDetected newState);
#endif
#endif

#if (SENSOR_TEMPERATURE == 1)
/**
 * Handle read request to the 'Current Temperature' characteristic of the Temperature Sensor service.
 */
HAPError HandleTemperatureSensorCurrentTemperatureRead(
        HAPAccessoryServer* server,
        const HAPFloatCharacteristicReadRequest* request,
        float* value,
        void* _Nullable context);

/**
 * Handle read request to the 'Status Active' characteristic of the Temperature Sensor service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleTemperatureSensorStatusActiveRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context);
#endif

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

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
