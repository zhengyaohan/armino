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

#ifndef HAP_SERVICE_TYPES_H
#define HAP_SERVICE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Accessory Information.
 *
 * Every accessory must expose a single instance of the Accessory information service with the following definition.
 * The values of `Manufacturer`, `Model`, `Name`, `Serial Number` and `Product Data` must be persistent through
 * the lifetime of the accessory. These values must match with the data as specified in the MFi Portal at the time of
 * firmware submission for self-certification. Note: These values may be visible to the user in the accessory settings
 * of the Apple Home app.
 *
 * The value for the `Product Data` characteristic is assigned to each Product Plan on the MFi Portal at the time of
 * Product Plan submission.
 *
 * Any other Apple-defined characteristics added to this service must only contain one or more of the following
 * permissions: Paired Read or Notify. Custom characteristics added to this service must only contain one or more of the
 * following permissions: Paired Read, Notify, Broadcast, and Hidden. All other permissions are not permitted.
 *
 * Required Characteristics:
 * - Firmware Revision
 * - Identify
 * - Manufacturer
 * - Model
 * - Name
 * - Serial Number
 * - Product Data
 *
 * Optional Characteristics:
 * - Accessory Flags
 * - Hardware Revision
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.1 Accessory Information
 */
/**@{*/
#define kHAPServiceDebugDescription_AccessoryInformation "accessory-information"

extern const HAPUUID kHAPServiceType_AccessoryInformation;
/**@}*/

/**
 * Garage Door Opener.
 *
 * This service describes a garage door opener that controls a single door. If a garage has more than one door, then
 * each door should have its own Garage Door Opener Service.
 *
 * Required Characteristics:
 * - Current Door State
 * - Target Door State
 * - Obstruction Detected
 *
 * Optional Characteristics:
 * - Lock Current State
 * - Lock Target State
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.3 Garage Door Opener
 */
/**@{*/
#define kHAPServiceDebugDescription_GarageDoorOpener "garage-door-opener"

extern const HAPUUID kHAPServiceType_GarageDoorOpener;
/**@}*/

/**
 * Light Bulb.
 *
 * This service describes a light bulb.
 *
 * Required Characteristics:
 * - On
 *
 * Optional Characteristics:
 * - Brightness
 * - Hue
 * - Name
 * - Saturation
 * - Color Temperature
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.4 Light Bulb
 */
/**@{*/
#define kHAPServiceDebugDescription_LightBulb "lightbulb"

extern const HAPUUID kHAPServiceType_LightBulb;

/**@}*/

/**
 * Lock Mechanism.
 *
 * The HomeKit Lock Mechanism Service is designed to expose and control the physical lock mechanism on a device.
 *
 * Required Characteristics:
 * - Lock Current State
 * - Lock Target State
 *
 * Optional Characteristics:
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.5 Lock
 */
/**@{*/
#define kHAPServiceDebugDescription_LockMechanism "lock-mechanism"

extern const HAPUUID kHAPServiceType_LockMechanism;
/**@}*/

/**
 * Outlet.
 *
 * This service describes a power outlet.
 *
 * This service is updated in iOS 13.
 *
 * Required Characteristics:
 * - On
 *
 * Optional Characteristics:
 * - Name
 * - Outlet In Use
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.6 Outlet
 */
/**@{*/
#define kHAPServiceDebugDescription_Outlet "outlet"

extern const HAPUUID kHAPServiceType_Outlet;
/**@}*/

/**
 * Switch.
 *
 * This service describes a binary switch.
 *
 * Required Characteristics:
 * - On
 *
 * Optional Characteristics:
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.7 Switch
 */
/**@{*/
#define kHAPServiceDebugDescription_Switch "switch"

extern const HAPUUID kHAPServiceType_Switch;
/**@}*/

/**
 * Thermostat.
 *
 * This service describes a thermostat.
 *
 * Required Characteristics:
 * - Current Heating Cooling State
 * - Target Heating Cooling State
 * - Current Temperature
 * - Target Temperature
 * - Temperature Display Units
 *
 * Optional Characteristics:
 * - Cooling Threshold Temperature
 * - Current Relative Humidity
 * - Heating Threshold Temperature
 * - Name
 * - Target Relative Humidity
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.8 Thermostat
 */
/**@{*/
#define kHAPServiceDebugDescription_Thermostat "thermostat"

extern const HAPUUID kHAPServiceType_Thermostat;
/**@}*/

/**
 * Pairing.
 *
 * Defines characteristics to support pairing between a controller and an accessory.
 *
 * Required Characteristics:
 * - Pair Setup
 * - Pair Verify
 * - Pairing Features
 * - Pairing Pairings
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.14.1 Pairing Service
 */
/**@{*/
#define kHAPServiceDebugDescription_Pairing "pairing"

extern const HAPUUID kHAPServiceType_Pairing;
/**@}*/

/**
 * Security System.
 *
 * This service describes a security system service.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Security System Current State
 * - Security System Target State
 *
 * Optional Characteristics:
 * - Name
 * - Security System Alarm Type
 * - Status Fault
 * - Status Tampered
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.10 Security System
 */
/**@{*/
#define kHAPServiceDebugDescription_SecuritySystem "security-system"

extern const HAPUUID kHAPServiceType_SecuritySystem;
/**@}*/

/**
 * Carbon Monoxide Sensor.
 *
 * This service describes a carbon monoxide sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Carbon Monoxide Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 * - Carbon Monoxide Level
 * - Carbon Monoxide Peak Level
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.11 Carbon Monoxide Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_CarbonMonoxideSensor "sensor.carbon-monoxide"

extern const HAPUUID kHAPServiceType_CarbonMonoxideSensor;
/**@}*/

/**
 * Contact Sensor.
 *
 * This service describes a Contact Sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Contact Sensor State
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.12 Contact Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_ContactSensor "sensor.contact"

extern const HAPUUID kHAPServiceType_ContactSensor;
/**@}*/

/**
 * Door.
 *
 * This service describes a motorized door.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Current Position
 * - Target Position
 * - Position State
 *
 * Optional Characteristics:
 * - Name
 * - Hold Position
 * - Obstruction Detected
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.13 Door
 */
/**@{*/
#define kHAPServiceDebugDescription_Door "door"

extern const HAPUUID kHAPServiceType_Door;
/**@}*/

/**
 * Humidity Sensor.
 *
 * This service describes a Humidity Sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Current Relative Humidity
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.14 Humidity Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_HumiditySensor "sensor.humidity"

extern const HAPUUID kHAPServiceType_HumiditySensor;
/**@}*/

/**
 * Leak Sensor.
 *
 * This service describes a leak sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Leak Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.15 Leak Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_LeakSensor "sensor.leak"

extern const HAPUUID kHAPServiceType_LeakSensor;
/**@}*/

/**
 * Light Sensor.
 *
 * This service describes a light sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Current Ambient Light Level
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Low Battery
 * - Status Tampered
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.16 Light Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_LightSensor "sensor.light"

extern const HAPUUID kHAPServiceType_LightSensor;
/**@}*/

/**
 * Motion Sensor.
 *
 * This service describes a motion sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Motion Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.17 Motion Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_MotionSensor "sensor.motion"

extern const HAPUUID kHAPServiceType_MotionSensor;
/**@}*/

/**
 * Occupancy Sensor.
 *
 * This service describes an occupancy sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Occupancy Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.18 Occupancy Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_OccupancySensor "sensor.occupancy"

extern const HAPUUID kHAPServiceType_OccupancySensor;
/**@}*/

/**
 * Smoke Sensor.
 *
 * This service describes a Smoke detector Sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Smoke Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.19 Smoke Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_SmokeSensor "sensor.smoke"

extern const HAPUUID kHAPServiceType_SmokeSensor;
/**@}*/

/**
 * Stateless Programmable Switch.
 *
 * This service describes a stateless programmable switch.
 *
 * The following rules apply to a stateless programmable switch accessory:
 * - Each physical switch on the accessory must be represented by a unique instance of this service.
 * - If there are multiple instances of this service on the accessory, they must be linked to a `Service Label`.
 * - If there are multiple instances of this service on the accessory,
 *   `Service Label Index` is a required characteristic.
 * - `Service Label Index` value for each instance of this service linked to the same `Service Label` must be unique.
 * - The User visible label on the physical accessory should match the `Service Label Namespace`
 *   described by the accessory.
 * - If there is only one instance of this service on the accessory, `Service Label` is not required and consequently
 *   `Service Label Index` must not be present.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Programmable Switch Event
 *
 * Optional Characteristics:
 * - Name
 * - Service Label Index
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.20 Stateless Programmable Switch
 */
/**@{*/
#define kHAPServiceDebugDescription_StatelessProgrammableSwitch "stateless-programmable-switch"

extern const HAPUUID kHAPServiceType_StatelessProgrammableSwitch;
/**@}*/

/**
 * Temperature Sensor.
 *
 * This service describes a temperature sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Current Temperature
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Low Battery
 * - Status Tampered
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.21 Temperature Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_TemperatureSensor "sensor.temperature"

extern const HAPUUID kHAPServiceType_TemperatureSensor;
/**@}*/

/**
 * Window.
 *
 * This service describes a motorized window.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Current Position
 * - Target Position
 * - Position State
 *
 * Optional Characteristics:
 * - Name
 * - Hold Position
 * - Obstruction Detected
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.22 Window
 */
/**@{*/
#define kHAPServiceDebugDescription_Window "window"

extern const HAPUUID kHAPServiceType_Window;
/**@}*/

/**
 * Window Covering.
 *
 * This service describes motorized window coverings or shades - examples include shutters, blinds, awnings etc.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Target Position
 * - Current Position
 * - Position State
 *
 * Optional Characteristics:
 * - Name
 * - Hold Position
 * - Current Horizontal Tilt Angle
 * - Target Horizontal Tilt Angle
 * - Current Vertical Tilt Angle
 * - Target Vertical Tilt Angle
 * - Obstruction Detected
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.23 Window Covering
 */
/**@{*/
#define kHAPServiceDebugDescription_WindowCovering "window-covering"

extern const HAPUUID kHAPServiceType_WindowCovering;
/**@}*/

/**
 * Air Quality Sensor.
 *
 * This service describes an air quality sensor. `Air Quality` refers to the cumulative air quality recorded
 * by the accessory which may be based on multiple sensors present.
 *
 * This service requires iOS 9 or later and is updated in iOS 10.
 *
 * Required Characteristics:
 * - Air Quality
 *
 * Optional Characteristics:
 * - Name
 * - Ozone Density
 * - Nitrogen Dioxide Density
 * - Sulphur Dioxide Density
 * - PM2.5 Density
 * - PM10 Density
 * - VOC Density
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.9 Air Quality Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_AirQualitySensor "sensor.air-quality"

extern const HAPUUID kHAPServiceType_AirQualitySensor;
/**@}*/

/**
 * Battery Service.
 *
 * This service describes a battery service.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Battery Level
 * - Charging State
 * - Status Low Battery
 *
 * Optional Characteristics:
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.24 Battery Service
 */
/**@{*/
#define kHAPServiceDebugDescription_BatteryService "battery"

extern const HAPUUID kHAPServiceType_BatteryService;
/**@}*/

/**
 * Carbon Dioxide Sensor.
 *
 * This service describes a carbon dioxide sensor.
 *
 * This service requires iOS 9 or later.
 *
 * Required Characteristics:
 * - Carbon Dioxide Detected
 *
 * Optional Characteristics:
 * - Name
 * - Status Active
 * - Status Fault
 * - Status Tampered
 * - Status Low Battery
 * - Carbon Dioxide Level
 * - Carbon Dioxide Peak Level
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.25 Carbon Dioxide Sensor
 */
/**@{*/
#define kHAPServiceDebugDescription_CarbonDioxideSensor "sensor.carbon-dioxide"

extern const HAPUUID kHAPServiceType_CarbonDioxideSensor;
/**@}*/

/**
 * HAP Protocol Information.
 *
 * Every accessory must expose a single instance of the HAP protocol information. For a bridge accessory, only the
 * primary HAP accessory object must contain this service. The `Version` value is transport dependent. Refer to
 * BLE Protocol Version Characteristic for BLE protocol version. Refer to IP Protocol Version for IP protocol version.
 *
 * Required Characteristics:
 * - Version
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.2 HAP Protocol Information
 */
/**@{*/
#define kHAPServiceDebugDescription_HAPProtocolInformation "protocol.information.service"

extern const HAPUUID kHAPServiceType_HAPProtocolInformation;
/**@}*/

/**
 * Fan.
 *
 * This service describes a fan.
 *
 * If the fan service is included in air purifier accessories, `Current Fan State` and `Target Fan State`
 * are required characteristics.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Active
 *
 * Optional Characteristics:
 * - Name
 * - Current Fan State
 * - Target Fan State
 * - Rotation Direction
 * - Rotation Speed
 * - Swing Mode
 * - Lock Physical Controls
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.30 Fan
 */
/**@{*/
#define kHAPServiceDebugDescription_Fan "fanv2"

extern const HAPUUID kHAPServiceType_Fan;

/**@}*/

/**
 * Slat.
 *
 * This service describes a slat which tilts on a vertical or a horizontal axis.
 *
 * `Current Tilt Angle` and `Target Tilt Angle` may be included in this service if the user can set the slats to a
 * particular tilt angle.
 *
 * `Swing Mode` implies that the slats can swing automatically (e.g., vents on a fan).
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Current Slat State
 * - Slat Type
 *
 * Optional Characteristics:
 * - Name
 * - Swing Mode
 * - Current Tilt Angle
 * - Target Tilt Angle
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.31 Slat
 */
/**@{*/
#define kHAPServiceDebugDescription_Slat "vertical-slat"

extern const HAPUUID kHAPServiceType_Slat;
/**@}*/

/**
 * Filter Maintenance.
 *
 * This service can be used to describe maintenance operations for a filter.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Filter Change Indication
 *
 * Optional Characteristics:
 * - Name
 * - Filter Life Level
 * - Reset Filter Indication
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.32 Filter Maintenance
 */
/**@{*/
#define kHAPServiceDebugDescription_FilterMaintenance "filter-maintenance"

extern const HAPUUID kHAPServiceType_FilterMaintenance;
/**@}*/

/**
 * Air Purifier.
 *
 * This service describes an air purifier. An air purifier accessory can have additional linked services such as:
 * - `Filter Maintenance` service(s) to describe one or more air filters.
 * - `Air Quality Sensor` services to describe air quality sensors.
 * - `Fan` service to describe a fan which can be independently controlled.
 * - `Slat` service to control vents.
 *
 * If `Fan` is included as a linked service in an air purifier accessory:
 * - Changing `Active` characteristic on the `Air Purifier` must result in corresponding change to
 *   `Active` characteristic on the `Fan`.
 * - Changing `Active` characteristic on the `Fan` from "Inactive" to "Active" does not require the `Active` on the
 *   `Air Purifier` to change. This enables "Fan Only" mode on air purifier.
 * - Changing `Active` characteristic on the `Fan` from "Active" to "Inactive" must result in the `Active` on the
 *   `Air Purifier` to change to "Inactive".
 *
 * An air purifier accessory service may include `Rotation Speed` to control fan speed
 * if the fan cannot be independently controlled.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Active
 * - Current Air Purifier State
 * - Target Air Purifier State
 *
 * Optional Characteristics:
 * - Name
 * - Rotation Speed
 * - Swing Mode
 * - Lock Physical Controls
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.33 Air Purifier
 */
/**@{*/
#define kHAPServiceDebugDescription_AirPurifier "air-purifier"

extern const HAPUUID kHAPServiceType_AirPurifier;
/**@}*/

/**
 * Heater Cooler.
 *
 * This service can be used to describe either of the following:
 * - a heater
 * - a cooler
 * - a heater and a cooler
 *
 * A heater/cooler accessory may have additional:
 * - `Fan` service to describe a fan which can be independently controlled
 * - `Slat` service to control vents
 *
 * A heater must include `Heating Threshold Temperature`. A cooler must include `Cooling Threshold Temperature`.
 *
 * A heater/cooler accessory service may include `Rotation Speed` to control fan speed if the fan cannot be
 * independently controlled.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Active
 * - Current Temperature
 * - Current Heater Cooler State
 * - Target Heater Cooler State
 *
 * Optional Characteristics:
 * - Name
 * - Rotation Speed
 * - Temperature Display Units
 * - Swing Mode
 * - Cooling Threshold Temperature
 * - Heating Threshold Temperature
 * - Lock Physical Controls
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.34 Heater Cooler
 */
/**@{*/
#define kHAPServiceDebugDescription_HeaterCooler "heater-cooler"

extern const HAPUUID kHAPServiceType_HeaterCooler;
/**@}*/

/**
 * Humidifier Dehumidifier.
 *
 * This service can be used to describe either of the following:
 * - an air humidifier
 * - an air dehumidifier
 * - an air humidifier and an air dehumidifier
 *
 * An air humidifier/dehumidifier accessory may have additional:
 * - `Fan` service to describe a fan which can be independently controlled
 * - `Slat` service to control vents
 *
 * A dehumidifier must include `Relative Humidity Dehumidifier Threshold`.
 * A humidifier must include `Relative Humidity Humidifier Threshold`.
 *
 * A humidifier/dehumidifier accessory service may include `Rotation Speed` to control fan speed if the fan cannot be
 * independently controlled.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Active
 * - Current Relative Humidity
 * - Current Humidifier Dehumidifier State
 * - Target Humidifier Dehumidifier State
 *
 * Optional Characteristics:
 * - Name
 * - Relative Humidity Dehumidifier Threshold
 * - Relative Humidity Humidifier Threshold
 * - Rotation Speed
 * - Swing Mode
 * - Water Level
 * - Lock Physical Controls
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.35 Humidifier Dehumidifier
 */
/**@{*/
#define kHAPServiceDebugDescription_HumidifierDehumidifier "humidifier-dehumidifier"

extern const HAPUUID kHAPServiceType_HumidifierDehumidifier;
/**@}*/

/**
 * Service Label.
 *
 * This service describes label scheme.
 *
 * This service requires iOS 10.3 or later.
 *
 * Required Characteristics:
 * - Service Label Namespace
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.36 Service Label
 */
/**@{*/
#define kHAPServiceDebugDescription_ServiceLabel "service-label"

extern const HAPUUID kHAPServiceType_ServiceLabel;
/**@}*/

/**
 * Irrigation System.
 *
 * This service describes an irrigation system. This service must be present on an irrigation systems which supports
 * on-device schedules or supports a top-level `Active` control across multiple valves.
 *
 * A sprinkler system accessory maybe:
 *
 * - a combination of `Irrigation System` service on a bridge accessory with a collection of one or more `Valve`
 *   services (with `Valve Type` set to "Irrigation") as bridged accessories (The bridge accessory is typically
 *   connected to each valve using wires). OR
 *
 * - a combination of `Irrigation System` service with a collection of one or more linked `Valve` services (with
 *   `Valve Type` set to "Irrigation") (The valves are collocated e.g., hose based system). OR
 *
 * - a combination of `Valve` service(s) with `Valve Type` set to "Irrigation" (e.g., a system with one or more valves
 *   which does not support scheduling)
 *
 * An irrigation system is set to "Active" when the system is enabled. When one of the valves is set to "In Use", the
 * irrigation system must be set to in use.
 *
 * An accessory which includes this services must include the `Set Duration` in the `Valve`.
 *
 * An irrigation system accessory which does not auto detect the number of valves it is connected to and requires user
 * to provide this information must include the `Is Configured` in the `Valve`.
 *
 * `Remaining Duration` on this service implies the total remaining duration to water across all the valves.
 *
 * This service requires iOS 11.2 or later.
 *
 * Required Characteristics:
 * - Active
 * - Program Mode
 * - In Use
 *
 * Optional Characteristics:
 * - Remaining Duration
 * - Name
 * - Status Fault
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.37 Irrigation System
 */
/**@{*/
#define kHAPServiceDebugDescription_IrrigationSystem "irrigation-system"

extern const HAPUUID kHAPServiceType_IrrigationSystem;
/**@}*/

/**
 * Valve.
 *
 * This service describes accessories like irrigation valves or water outlets. A valve is set to "In Use" when there are
 * fluid flowing through the valve.
 *
 * If an accessory has this service with `Valve Type` set to Irrigation it must include the `Set Duration` and
 * `Remaining Duration` characteristic on the `Valve` service.
 *
 * `Service Label Index` must be present on each instance of this service if the accessory consists of:
 *
 * - a bridge accessory (the `Service Label` service must be included here) which includes multiple bridged accessories
 *   each with `Valve` service.
 *
 * - an accessory (the `Service Label` service must be included here) which includes multiple linked `Valve` services.
 *
 * If an accessory has this service with `Service Label Index` included, the default `Name` must be empty string unless
 * user has already assigned a name to this valve before accessory is HomeKit paired. In such a case, the default name
 * should be the user configured name for this valve.
 *
 * `Is Configured` must be present on each instance of this service if the accessory is used in an irrigation system or
 * shower system where all valves may not be configured to use (e.g., depending on physical wire connection).
 *
 * Setting the value of `Active` to "Active" on this service must result in `Irrigation System` bridge to be set to
 * "Active" if this service is used in context of an Irrigation system.
 *
 * This service requires iOS 11.2 or later.
 *
 * Required Characteristics:
 * - Active
 * - In Use
 * - Valve Type
 *
 * Optional Characteristics:
 * - Set Duration
 * - Remaining Duration
 * - Is Configured
 * - Service Label Index
 * - Status Fault
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.38 Valve
 */
/**@{*/
#define kHAPServiceDebugDescription_Valve "valve"

extern const HAPUUID kHAPServiceType_Valve;
/**@}*/

/**
 * Faucet.
 *
 * The Faucet service describes accessories like faucets or shower heads. This service must only be included
 * when an accessory has either a linked `Heater Cooler` with single linked `Valve` service or multiple linked
 * Valve services (with or without the Heater Cooler service) to describe water outlets. This service serves
 * as a top level service for such accessories.
 *
 * A Faucet service that supports the heating of water must include the Heater Cooler service and the Valve service
 * as linked services. An accessory that supports one or multiple water outlets and the heating of water through a
 * common temperature control, must include Heater Cooler and Valve service(s) as linked services to the Faucet service.
 *
 * Setting the value of `Active` characteristic to 0 ("Inactive") on this service must turn off the faucet accessory.
 * The accessory must retain the state of Active characteristics on any linked Valve services if the Active
 * characteristic on this service is toggled. The accessory must set the value of Active to 0 ("Inactive") of any
 * linked Heater Cooler service if the Active on this service is set to 0 ("Inactive").
 *
 * This service requires iOS 11.2 or later.
 *
 * Required Characteristics:
 * - Active
 *
 * Optional Characteristics:
 * - Status Fault
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.39 Faucet
 */
/**@{*/
#define kHAPServiceDebugDescription_Faucet "faucet"

extern const HAPUUID kHAPServiceType_Faucet;
/**@}*/

/**
 * Camera RTP Stream Management.
 *
 * This service enables the accessory to advertise the audio/video codecs and parameters it supports,
 * and to configure and control a RTP session to stream the audio/video stream to a device.
 *
 * This service requires iOS 10 or later.
 *
 * Required Characteristics:
 * - Streaming Status
 * - Selected RTP Stream Configuration
 * - Setup Endpoints
 * - Supported Audio Stream Configuration
 * - Supported RTP Configuration
 * - Supported Video Stream Configuration
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.26 Camera RTP Stream Management
 */
/**@{*/
#define kHAPServiceDebugDescription_CameraRTPStreamManagement "camera-rtp-stream-management"

extern const HAPUUID kHAPServiceType_CameraRTPStreamManagement;
/**@}*/

/**
 * Microphone.
 *
 * A Microphone service is used to control the sourcing of the input audio - primarily through a microphone.
 *
 * This service requires iOS 10 or later.
 *
 * Required Characteristics:
 * - Mute
 *
 * Optional Characteristics:
 * - Name
 * - Volume
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.27 Microphone
 */
/**@{*/
#define kHAPServiceDebugDescription_Microphone "microphone"

extern const HAPUUID kHAPServiceType_Microphone;
/**@}*/

/**
 * Speaker.
 *
 * A Speaker service is to use to control the audio output settings on a speaker device.
 *
 * This service requires iOS 10 or later.
 *
 * Required Characteristics:
 * - Mute
 *
 * Optional Characteristics:
 * - Name
 * - Volume
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.28 Speaker
 */
/**@{*/
#define kHAPServiceDebugDescription_Speaker "speaker"

extern const HAPUUID kHAPServiceType_Speaker;
/**@}*/

/**
 * Doorbell.
 *
 * The Doorbell service describes a doorbell and is the primary service of the Video Doorbell Profile.
 *
 * This service requires iOS 10 or later.
 *
 * Required Characteristics:
 * - Programmable Switch Event
 *
 * Optional Characteristics:
 * - Name
 * - Volume
 * - Brightness
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.29 Doorbell
 */
/**@{*/
#define kHAPServiceDebugDescription_Doorbell "doorbell"

extern const HAPUUID kHAPServiceType_Doorbell;
/**@}*/

/**
 * Target Control Management.
 *
 * This service manages the configuration for a remote accessory and allows it to indicate the supported configuration.
 * This service must be marked as primary service for an accessory whose primary functionality is to act as a target
 * controller (e.g., Apple TV remote accessory).
 *
 * This service requires iOS 12 or later.
 *
 * Required Characteristics:
 * - Target Control Supported Configuration
 * - Target Control List
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.40 Target Control Management
 */
/**@{*/
#define kHAPServiceDebugDescription_TargetControlManagement "target-control-management"

extern const HAPUUID kHAPServiceType_TargetControlManagement;
/**@}*/

/**
 * Target Control.
 *
 * This service handles the control of a selected target from the remote accessory. If an accessory can support control
 * of multiple concurrent Apple TVs at the same time without requiring the user to select an Apple TV on the
 * remote accessory UI, it must expose multiple instances of this service.
 *
 * This service requires iOS 12 or later.
 *
 * Required Characteristics:
 * - Active Identifier
 * - Active
 * - Button Event
 *
 * Optional Characteristics:
 * - Name
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.41 Target Control
 */
/**@{*/
#define kHAPServiceDebugDescription_TargetControl "target-control"

extern const HAPUUID kHAPServiceType_TargetControl;
/**@}*/

/**
 * Audio Stream Management.
 *
 * This service manages the configuration for audio input from the accessory, if applicable.
 *
 * This service requires iOS 12 or later.
 *
 * Required Characteristics:
 * - Supported Audio Stream Configuration
 * - Selected Audio Stream Configuration
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.42 Audio Stream Management
 */
/**@{*/
#define kHAPServiceDebugDescription_AudioStreamManagement "audio-stream-management"

extern const HAPUUID kHAPServiceType_AudioStreamManagement;
/**@}*/

/**
 * Data Stream Transport Management.
 *
 * The Data Stream Transport Management service enables the configuration and management of the transport data stream.
 * The version field must be set to "1.0".
 *
 * This service requires iOS 12 or later.
 *
 * Required Characteristics:
 * - Setup Data Stream Transport
 * - Supported Data Stream Transport Configuration
 * - Version
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.43 Data Stream Transport Management
 */
/**@{*/
#define kHAPServiceDebugDescription_DataStreamTransportManagement "data-stream-transport-management"

extern const HAPUUID kHAPServiceType_DataStreamTransportManagement;
/**@}*/

/**
 * Siri.
 *
 * This service allows configuration and management of Siri.
 *
 * This service must be linked to the `Audio Stream Management` and `Data Stream Transport Management`.
 *
 * This service requires iOS 12 or later.
 *
 * Required Characteristics:
 * - Siri Service Signature
 * - Siri Input Type
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.44 Siri
 */
/**@{*/
#define kHAPServiceDebugDescription_Siri "siri"

extern const HAPUUID kHAPServiceType_Siri;
/**@}*/

/**
 * Camera Event Recording Management.
 *
 * This service enables the accessory to advertise support for audio and video codecs, and parameters
 * to configure and control camera event recording sessions.
 *
 * This service requires iOS 13.2 or later.
 *
 * Required Characteristics:
 * - Active
 * - Selected Camera Recording Configuration
 * - Supported Audio Recording Configuration
 * - Supported Video Recording Configuration
 * - Supported Camera Recording Configuration
 * - Recording Audio Active
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.45 Camera Event Recording Management
 */
/**@{*/
#define kHAPServiceDebugDescription_CameraEventRecordingManagement "camera-event-recording-management"

extern const HAPUUID kHAPServiceType_CameraEventRecordingManagement;
/**@}*/

/**
 * Camera Operating Mode.
 *
 * This service allows control of the operating mode of an IP camera, including the enabling or disabling of snapshots,
 * turning the camera on/off, etc.
 *
 * This service requires iOS 13.2 or later.
 *
 * Required Characteristics:
 * - HomeKit Camera Active
 * - Event Snapshots Active
 * - Periodic Snapshots Active
 *
 * Optional Characteristics:
 * - Camera Operating Mode Indicator
 * - Diagonal Field of View
 * - Manually Disabled
 * - Night Vision
 * - Third Party Camera Active
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.46 Camera Operating Mode
 */
/**@{*/
#define kHAPServiceDebugDescription_CameraOperatingMode "camera-operating-mode"

extern const HAPUUID kHAPServiceType_CameraOperatingMode;
/**@}*/

/**
 * Wi-Fi Router.
 *
 * This service describes Wi-Fi routers that can be controlled, and provides the ability to manage access and
 * security policies for IP accessories. A Wi-Fi router can only expose a single instance of this service.
 *
 * This service requires iOS 13.2 or later.
 *
 * Required Characteristics:
 * - Version
 * - Supported Router Configuration
 * - Configured Name
 * - Router Status
 * - WAN Configuration List
 * - WAN Status List
 * - Managed Network Enable
 * - Network Client Profile Control
 * - Network Client Status Control
 * - Network Access Violation Control
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.47 Wi-Fi Router
 */
/**@{*/
#define kHAPServiceDebugDescription_WiFiRouter "wifi-router"

extern const HAPUUID kHAPServiceType_WiFiRouter;
/**@}*/

/**
 * Wi-Fi Satellite.
 *
 * This service is used to manage the Wi-Fi Satellite devices that extend the Wi-Fi network of a Wi-Fi Router accessory.
 * A Wi-Fi Satellite implementing this service must not implement the `Wi-Fi Router` and vice-versa.
 *
 * When a Wi-Fi Router has any associated satellite devices, the main router must also implement a HAP bridge
 * that publishes each Satellite device as a bridged accessory implementing the `Wi-Fi Satellite`.
 * (Note: Contrary to the regular rules for HAP bridges, a bridged accessory implementing the `Wi-Fi Satellite`
 * is allowed to support an IP transport.)
 *
 * This service requires iOS 13.2 or later.
 *
 * Required Characteristics:
 * - Wi-Fi Satellite Status
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.48 Wi-Fi Satellite
 */
/**@{*/
#define kHAPServiceDebugDescription_WiFiSatellite "wifi-satellite"

extern const HAPUUID kHAPServiceType_WiFiSatellite;
/**@}*/

/**
 * Wi-Fi Transport.
 *
 * This service exposes capabilities, configuration, and status information for the Wi-Fi interface
 * of an accessory, as well as control points to update the configuration.
 * This service must be supported by all accessories that support Wi-Fi.
 *
 * Required Characteristics:
 * - Current Transport Flag
 * - Wi-Fi Capability Flag
 * - Wi-Fi Configuration Control
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.49 Wi-Fi Transport
 */
/**@{*/
#define kHAPServiceDebugDescription_WiFiTransport "wifi-transport"

extern const HAPUUID kHAPServiceType_WiFiTransport;
/**@}*/

/**
 * Accessory Runtime Information
 *
 * The Accessory Runtime Information Service enables the retrieval of runtime information of an accessory.
 *
 * This service requires iOS 14.0 or later.
 *
 * Required Characteristics:
 * - Sleep Interval
 * - Ping
 *
 * Optional Characteristics:
 * - Activity Interval
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.50 Accessory Runtime Information
 */
/**@{*/
#define kHAPServiceDebugDescription_AccessoryRuntimeInformation "accessory-runtime-information"

extern const HAPUUID kHAPServiceType_AccessoryRuntimeInformation;
/**@}*/

/**
 * Thread Management
 *
 * The Thread Management Service is used to manage Thread node configuration of an accessory.
 *
 * This service requires iOS XX.X or later.
 *
 * Required Characteristics:
 * - Network Name
 * - Network Channel
 * - PAN ID
 * - Ext PAN ID
 * - Border Router Capable
 * - ED Capability
 * - Poll Period
 * - Status
 * - Control
 *
 * Optional Characteristics:
 * - Open Thread Ver
 * - Border Router
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.51 Thread Transport
 */
/**@{*/
#define kHAPServiceDebugDescription_ThreadManagement "thread-transport"

extern const HAPUUID kHAPServiceType_ThreadManagement;
/**@}*/

/**
 * Diagnostics
 *
 * The Diagnostics Service enables the retrieval of diagnostic data from the accessory.
 *
 * This service requires iOS 14.0 or later.
 *
 * Required Characteristics:
 * - Supported Diagnostics Snapshot
 *
 * Optional Characteristics:
 * None
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.52 Diagnostics
 */
/**@{*/
#define kHAPServiceDebugDescription_Diagnostics "diagnostics"

extern const HAPUUID kHAPServiceType_Diagnostics;
/**@}*/

/**
 * Accessory Metrics
 *
 * The Accessory Metrics Service enables the reporting of metrics data from the accessory.
 *
 * This service requires iOS 14.0 or later.
 *
 * Required Characteristics:
 * - Active
 * - Supported Metrics
 * - Metrics Buffer Full State
 *
 * Optional Characteristics:
 * None
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.53 AccessoryMetrics
 */
/**@{*/
#define kHAPServiceDebugDescription_Metrics "metrics"

extern const HAPUUID kHAPServiceType_Metrics;
/**@}*/

/**
 * Access Code
 *
 * An Access Code service is to used to manage access code control on a HAP accessory.
 *
 * This service requires iOS 14 or later.
 *
 * Required Characteristics:
 * - Access Code Supported Configuration
 * - Access Code Control Point
 * - Configuration State
 *
 * Optional Characteristics:
 * None
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.54 Access Code
 */
/**@{*/
#define kHAPServiceDebugDescription_AccessCode "access-code"

extern const HAPUUID kHAPServiceType_AccessCode;
/**@}*/

/**
 * NFC Access
 *
 * An NFC Access service is to used to manage NFC access control on a HAP accessory.
 *
 * This service requires iOS 14 or later.
 *
 * Required Characteristics:
 * - NFC Access Supported Configuration
 * - NFC Access Control Point
 * - Configuration State
 *
 * Optional Characteristics:
 * None
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.55 NFC Access
 */
/**@{*/
#define kHAPServiceDebugDescription_NfcAccess "nfc-access"

extern const HAPUUID kHAPServiceType_NfcAccess;
/**@}*/

/**
 * Firmware Update
 *
 * The Firmware Update Service is used to manage the firmware update of an accessory.
 *
 * This service requires iOS 14.0 or later.
 *
 * Required Characteristics:
 * - Supported Firmware Update Configuration
 * - Firmware Update Readiness
 * - Firmware Update Status
 *
 * Optional Characteristics:
 * None
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 10.56 Firmware Update
 */
/**@{*/
#define kHAPServiceDebugDescription_FirmwareUpdate "firmware-update"

extern const HAPUUID kHAPServiceType_FirmwareUpdate;
/**@}*/

#if (HAP_TESTING == 1)
#define kHAPServiceDebugDescription_HAPDebugCommand "hap-debug"

extern const HAPUUID kHAPServiceType_HAPDebugCommand;
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
