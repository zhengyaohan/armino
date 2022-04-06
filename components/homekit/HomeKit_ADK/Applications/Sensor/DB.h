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

// Basic sensor database example. This header file, and the corresponding DB.c implementation in the ADK, is
// platform-independent.

#ifndef DB_H
#define DB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP.h"

#include "AccessoryInformationServiceDB.h"
#include "ApplicationFeatures.h"
#include "ServiceIIDs.h"
#if (HAP_TESTING == 1)
#include "DebugCommandServiceDB.h"
#endif
#if (HAP_APP_USES_HDS == 1)
#include "DataStreamTransportManagementServiceDB.h"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
#include "FirmwareUpdateServiceDB.h"
#endif
#if (HAVE_THREAD == 1)
#include "ThreadManagementServiceDB.h"
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
#include "AccessoryRuntimeInformationServiceDB.h"
#endif
#include "BatteryServiceDB.h"
#if (SENSOR_MOTION == 1)
#include "MotionSensorServiceDB.h"
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
#include "DiagnosticsServiceDB.h"
#endif
#include "PairingServiceDB.h"
#include "ProtocolInformationServiceDB.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#if (SENSOR_AIR_QUALITY == 1)
#define kIID_AirQualitySensor                 (kIID_AirQualitySensorServiceBase + (uint64_t) 0x0)
#define kIID_AirQualitySensorServiceSignature (kIID_AirQualitySensorServiceBase + (uint64_t) 0x1)
#define kIID_AirQualitySensorName             (kIID_AirQualitySensorServiceBase + (uint64_t) 0x2)
#define kIID_AirQualitySensorAirQuality       (kIID_AirQualitySensorServiceBase + (uint64_t) 0x3)
#define kIID_AirQualitySensorStatusActive     (kIID_AirQualitySensorServiceBase + (uint64_t) 0x4)

enum { kAirQualitySensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_CARBON_DIOXIDE == 1)
#define kIID_CarbonDioxideSensor                      (kIID_CarbonDioxideSensorServiceBase + (uint64_t) 0x0)
#define kIID_CarbonDioxideSensorServiceSignature      (kIID_CarbonDioxideSensorServiceBase + (uint64_t) 0x1)
#define kIID_CarbonDioxideSensorName                  (kIID_CarbonDioxideSensorServiceBase + (uint64_t) 0x2)
#define kIID_CarbonDioxideSensorCarbonDioxideDetected (kIID_CarbonDioxideSensorServiceBase + (uint64_t) 0x3)
#define kIID_CarbonDioxideSensorStatusActive          (kIID_CarbonDioxideSensorServiceBase + (uint64_t) 0x4)

enum { kCarbonDioxideSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_CARBON_MONOXIDE == 1)
#define kIID_CarbonMonoxideSensor                       (kIID_CarbonMonoxideSensorServiceBase + (uint64_t) 0x0)
#define kIID_CarbonMonoxideSensorServiceSignature       (kIID_CarbonMonoxideSensorServiceBase + (uint64_t) 0x1)
#define kIID_CarbonMonoxideSensorName                   (kIID_CarbonMonoxideSensorServiceBase + (uint64_t) 0x2)
#define kIID_CarbonMonoxideSensorCarbonMonoxideDetected (kIID_CarbonMonoxideSensorServiceBase + (uint64_t) 0x3)
#define kIID_CarbonMonoxideSensorStatusActive           (kIID_CarbonMonoxideSensorServiceBase + (uint64_t) 0x4)

enum { kCarbonMonoxideSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_CONTACT == 1)
#define kIID_ContactSensor                   (kIID_ContactSensorServiceBase + (uint64_t) 0x0)
#define kIID_ContactSensorServiceSignature   (kIID_ContactSensorServiceBase + (uint64_t) 0x1)
#define kIID_ContactSensorName               (kIID_ContactSensorServiceBase + (uint64_t) 0x2)
#define kIID_ContactSensorContactSensorState (kIID_ContactSensorServiceBase + (uint64_t) 0x3)
#define kIID_ContactSensorStatusActive       (kIID_ContactSensorServiceBase + (uint64_t) 0x4)

enum { kContactSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_HUMIDITY == 1)
#define kIID_HumiditySensor                        (kIID_HumiditySensorServiceBase + (uint64_t) 0x0)
#define kIID_HumiditySensorServiceSignature        (kIID_HumiditySensorServiceBase + (uint64_t) 0x1)
#define kIID_HumiditySensorName                    (kIID_HumiditySensorServiceBase + (uint64_t) 0x2)
#define kIID_HumiditySensorCurrentRelativeHumidity (kIID_HumiditySensorServiceBase + (uint64_t) 0x3)
#define kIID_HumiditySensorStatusActive            (kIID_HumiditySensorServiceBase + (uint64_t) 0x4)

enum { kHumiditySensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_LEAK == 1)
#define kIID_LeakSensor                 (kIID_LeakSensorServiceBase + (uint64_t) 0x0)
#define kIID_LeakSensorServiceSignature (kIID_LeakSensorServiceBase + (uint64_t) 0x1)
#define kIID_LeakSensorName             (kIID_LeakSensorServiceBase + (uint64_t) 0x2)
#define kIID_LeakSensorLeakDetected     (kIID_LeakSensorServiceBase + (uint64_t) 0x3)
#define kIID_LeakSensorStatusActive     (kIID_LeakSensorServiceBase + (uint64_t) 0x4)

enum { kLeakSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_LIGHT == 1)
#define kIID_LightSensor                         (kIID_LightSensorServiceBase + (uint64_t) 0x0)
#define kIID_LightSensorServiceSignature         (kIID_LightSensorServiceBase + (uint64_t) 0x1)
#define kIID_LightSensorName                     (kIID_LightSensorServiceBase + (uint64_t) 0x2)
#define kIID_LightSensorCurrentAmbientLightLevel (kIID_LightSensorServiceBase + (uint64_t) 0x3)
#define kIID_LightSensorStatusActive             (kIID_LightSensorServiceBase + (uint64_t) 0x4)

enum { kLightSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_OCCUPANCY == 1)
#define kIID_OccupancySensor                  (kIID_OccupancySensorServiceBase + (uint64_t) 0x0)
#define kIID_OccupancySensorServiceSignature  (kIID_OccupancySensorServiceBase + (uint64_t) 0x1)
#define kIID_OccupancySensorName              (kIID_OccupancySensorServiceBase + (uint64_t) 0x2)
#define kIID_OccupancySensorOccupancyDetected (kIID_OccupancySensorServiceBase + (uint64_t) 0x3)
#define kIID_OccupancySensorStatusActive      (kIID_OccupancySensorServiceBase + (uint64_t) 0x4)

enum { kOccupancySensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_SMOKE == 1)
#define kIID_SmokeSensor                 (kIID_SmokeSensorServiceBase + (uint64_t) 0x0)
#define kIID_SmokeSensorServiceSignature (kIID_SmokeSensorServiceBase + (uint64_t) 0x1)
#define kIID_SmokeSensorName             (kIID_SmokeSensorServiceBase + (uint64_t) 0x2)
#define kIID_SmokeSensorSmokeDetected    (kIID_SmokeSensorServiceBase + (uint64_t) 0x3)
#define kIID_SmokeSensorStatusActive     (kIID_SmokeSensorServiceBase + (uint64_t) 0x4)

enum { kSmokeSensorServiceAttributeCount = 5 };
#endif

#if (SENSOR_TEMPERATURE == 1)
#define kIID_TemperatureSensor                   (kIID_TemperatureSensorServiceBase + (uint64_t) 0x0)
#define kIID_TemperatureSensorServiceSignature   (kIID_TemperatureSensorServiceBase + (uint64_t) 0x1)
#define kIID_TemperatureSensorName               (kIID_TemperatureSensorServiceBase + (uint64_t) 0x2)
#define kIID_TemperatureSensorCurrentTemperature (kIID_TemperatureSensorServiceBase + (uint64_t) 0x3)
#define kIID_TemperatureSensorStatusActive       (kIID_TemperatureSensorServiceBase + (uint64_t) 0x4)

enum { kTemperatureSensorServiceAttributeCount = 5 };
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Total number of services and characteristics contained in the accessory.
 */
enum {
    kAttributeCount = kAccessoryInformationServiceAttributeCount  // Accessory Information service
                      + kProtocolInformationServiceAttributeCount // Protocol Information service
                      + kPairingServiceAttributeCount             // Pairing service
#if (SENSOR_AIR_QUALITY == 1)
                      + kAirQualitySensorServiceAttributeCount // Air Quality Sensor service
#endif
#if (SENSOR_CARBON_DIOXIDE == 1)
                      + kCarbonDioxideSensorServiceAttributeCount // Carbon Dioxide Sensor service
#endif
#if (SENSOR_CARBON_MONOXIDE == 1)
                      + kCarbonMonoxideSensorServiceAttributeCount // Carbon Monoxide Sensor service
#endif
#if (SENSOR_CONTACT == 1)
                      + kContactSensorServiceAttributeCount // Contact Sensor service
#endif
#if (SENSOR_HUMIDITY == 1)
                      + kHumiditySensorServiceAttributeCount // Humidity Sensor service
#endif
#if (SENSOR_LEAK == 1)
                      + kLeakSensorServiceAttributeCount // Leak Sensor service
#endif
#if (SENSOR_LIGHT == 1)
                      + kLightSensorServiceAttributeCount // Light Sensor service
#endif
#if (SENSOR_MOTION == 1)
                      + kMotionSensorServiceAttributeCount // Motion Sensor service
#endif
#if (SENSOR_OCCUPANCY == 1)
                      + kOccupancySensorServiceAttributeCount // Occupancy Sensor service
#endif
#if (SENSOR_SMOKE == 1)
                      + kSmokeSensorServiceAttributeCount // Smoke Sensor service
#endif
#if (SENSOR_TEMPERATURE == 1)
                      + kTemperatureSensorServiceAttributeCount // Temperature Sensor service
#endif
#if (HAVE_THREAD == 1)
                      + kThreadManagementServiceAttributeCount // Thread Management service
#endif
#if (HAVE_ACCESSORY_REACHABILITY == 1)
                      + kAccessoryRuntimeInformationServiceAttributeCount // Accessory Runtime Information service
#endif
#if (HAP_APP_USES_HDS == 1)
                      + kDataStreamTransportManagementServiceAttributeCount // Data Stream Transport Management service
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
                      + kFirmwareUpdateServiceAttributeCount // Firmware Update service
#endif
#if (HAP_TESTING == 1)
                      + kDebugCommandServiceAttributeCount // Debug command service
#endif
#if (HAVE_DIAGNOSTICS_SERVICE == 1)
                      + kDiagnosticsServiceAttributeCount // Accessory Diagnostics service
#endif
                      + kBatteryServiceAttributeCount // Battery service
};

//----------------------------------------------------------------------------------------------------------------------
#if (SENSOR_AIR_QUALITY == 1)
/**
 * Air Quality Sensor service.
 */
extern const HAPService airQualitySensorService;

/**
 * The 'Air Quality' characteristic of the Air Quality Sensor service.
 */
extern const HAPUInt8Characteristic airQualitySensorAirQualityCharacteristic;

/**
 * The 'Status Active' characteristic of the Air Quality Sensor service.
 */
extern const HAPBoolCharacteristic airQualitySensorStatusActiveCharacteristic;
#endif

#if (SENSOR_CARBON_DIOXIDE == 1)
/**
 * Carbon Dioxide Sensor service.
 */
extern const HAPService carbonDioxideSensorService;

/**
 * The 'Carbon Dioxide Detected' characteristic of the Carbon Dioxide Sensor service.
 */
extern const HAPUInt8Characteristic carbonDioxideSensorCarbonDioxideDetectedCharacteristic;

/**
 * The 'Status Active' characteristic of the Carbon Dioxide Sensor service.
 */
extern const HAPBoolCharacteristic carbonDioxideSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_CARBON_MONOXIDE == 1)
/**
 * Carbon Monoxide Sensor service.
 */
extern const HAPService carbonMonoxideSensorService;

/**
 * The 'Carbon Monoxide Detected' characteristic of the Carbon Monoxide Sensor service.
 */
extern const HAPUInt8Characteristic carbonMonoxideSensorCarbonMonoxideDetectedCharacteristic;

/**
 * The 'Status Active' characteristic of the Carbon Monoxide Sensor service.
 */
extern const HAPBoolCharacteristic carbonMonoxideSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_CONTACT == 1)
/**
 * Contact Sensor service.
 */
extern const HAPService contactSensorService;

/**
 * The 'Contact Sensor State' characteristic of the Contact Sensor service.
 */
extern const HAPUInt8Characteristic contactSensorContactSensorStateCharacteristic;

/**
 * The 'Status Active' characteristic of the Contact Sensor service.
 */
extern const HAPBoolCharacteristic contactSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_HUMIDITY == 1)
/**
 * Humidity Sensor service.
 */
extern const HAPService humiditySensorService;

/**
 * The 'Current Relative Humidity' characteristic of the Humidity Sensor service.
 */
extern const HAPFloatCharacteristic humiditySensorCurrentRelativeHumidityCharacteristic;

/**
 * The 'Status Active' characteristic of the Humidity Sensor service.
 */
extern const HAPBoolCharacteristic humiditySensorStatusActiveCharacteristic;
#endif

#if (SENSOR_LEAK == 1)
/**
 * Leak Sensor service.
 */
extern const HAPService leakSensorService;

/**
 * The 'Leak Detected' characteristic of the Leak Sensor service.
 */
extern const HAPUInt8Characteristic leakSensorLeakDetectedCharacteristic;

/**
 * The 'Status Active' characteristic of the Leak Sensor service.
 */
extern const HAPBoolCharacteristic leakSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_LIGHT == 1)
/**
 * Light Sensor service.
 */
extern const HAPService lightSensorService;

/**
 * The 'Current Ambient Light Level' characteristic of the Light Sensor service.
 */
extern const HAPFloatCharacteristic lightSensorCurrentAmbientLightLevelCharacteristic;

/**
 * The 'Status Active' characteristic of the Light Sensor service.
 */
extern const HAPBoolCharacteristic lightSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_OCCUPANCY == 1)
/**
 * Occupancy Sensor service.
 */
extern const HAPService occupancySensorService;

/**
 * The 'Occupancy Detected' characteristic of the Occupancy Sensor service.
 */
extern const HAPUInt8Characteristic occupancySensorOccupancyDetectedCharacteristic;

/**
 * The 'Status Active' characteristic of the Occupancy Sensor service.
 */
extern const HAPBoolCharacteristic occupancySensorStatusActiveCharacteristic;
#endif

#if (SENSOR_SMOKE == 1)
/**
 * Smoke Sensor service.
 */
extern const HAPService smokeSensorService;

/**
 * The 'Smoke Detected' characteristic of the Smoke Sensor service.
 */
extern const HAPUInt8Characteristic smokeSensorSmokeDetectedCharacteristic;

/**
 * The 'Status Active' characteristic of the Smoke Sensor service.
 */
extern const HAPBoolCharacteristic smokeSensorStatusActiveCharacteristic;
#endif

#if (SENSOR_TEMPERATURE == 1)
/**
 * Temperature Sensor service.
 */
extern const HAPService temperatureSensorService;

/**
 * The 'Current Temperature' characteristic of the Temperature Sensor service.
 */
extern const HAPFloatCharacteristic temperatureSensorCurrentTemperatureCharacteristic;

/**
 * The 'Status Active' characteristic of the Temperature Sensor service.
 */
extern const HAPBoolCharacteristic temperatureSensorStatusActiveCharacteristic;
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
