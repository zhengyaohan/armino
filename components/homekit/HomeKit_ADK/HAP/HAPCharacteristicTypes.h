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

#ifndef HAP_CHARACTERISTIC_TYPES_H
#define HAP_CHARACTERISTIC_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

struct _HAPSequenceTLVFormat;
/**
 * data source structure to enumerate decoded sequence TLVs
 */
typedef struct {
    const struct _HAPSequenceTLVFormat* format;
    HAPTLVReader reader;
} HAPSequenceTLVDataSource;

/**
 * Access Code Control Point.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Paired Write, Write Response, Admin-only
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.176 Access Code Control Point
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AccessCodeControlPoint "access-code-control-point"

extern const HAPUUID kHAPCharacteristicType_AccessCodeControlPoint;

/**
 * Operation type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AccesCodeControlPoint_Operation_Type) {
    /**
     * List.
     * Requests the list of all access code identifiers.
     */
    kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List = 1,
    /**
     * Read.
     * Requests an access code.
     */
    kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read = 2,
    /**
     * Add.
     * Adds a new access code.
     */
    kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add = 3,
    /**
     * Update.
     * Updates an existing access code.
     */
    kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update = 4,
    /**
     * Remove.
     * Removes an existing access code.
     */
    kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AccessCodeControlPoint_Operation_Type);

/**
 * Response Status
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AccessCodeControlPointResponse_Status) {
    /** Success */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success = 0,
    /** Error. Unknown. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorUnknown = 1,
    /** Error. Exceeded maximum allowed access codes. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes = 2,
    /** Error. Too many bulk operations. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorTooManyBulkOperations = 3,
    /** Error. Duplicate (an entry matching the same access code configuration exists). */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate = 4,
    /** Error. Smaller than minimum length. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength = 5,
    /** Error. Larger than maximum length. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength = 6,
    /** Error. Invalid character (when the access code contains a character not in the supported character sets). */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter = 7,
    /** Error. Invalid request. */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidRequest = 8,
    /** Error. Does not exist (an entry matching the same identifier does not exist). */
    kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist = 9,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AccessCodeControlPointResponse_Status);

/**
 * Access Code Control Point Request
 */
typedef struct {
    /** Optional. The identifier for the access code. */
    uint32_t identifier;
    bool identifierIsSet;

    /** Optional. The access code. */
    char* _Nullable accessCode;
    bool accessCodeIsSet;
} HAPCharacteristicValue_AccessCodeControlPointRequest;

/**
 * Access Code Control Point Response
 */
typedef struct {
    /** Optional. The identifier for the access code. */
    uint32_t identifier;
    bool identifierIsSet;

    /** Optional. The access code. */
    char* _Nullable accessCode;
    bool accessCodeIsSet;

    /** Optional. Bitmask indicating the restrictions on the access code. */
    uint32_t flags;
    bool flagsIsSet;

    /** Optional. Status of the response */
    uint8_t status;
    bool statusIsSet;
} HAPCharacteristicValue_AccessCodeControlPointResponse;

/**
 * Access Code Control Point used for write response
 */
typedef struct {
    uint8_t operationType;
    bool operationTypeIsSet;

    /** Response */
    HAPCharacteristicValue_AccessCodeControlPointResponse response;
    bool responseIsSet;
} HAPCharacteristicValue_AccessCodeControlPoint;

/**@}*/

/**
 * Access Code Supported Configuration.
 *
 * - Format: TLV8
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.175 Access Code Supported Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AccessCodeSupportedConfiguration "access-code-supported-configuration"

extern const HAPUUID kHAPCharacteristicType_AccessCodeSupportedConfiguration;

/**
 * Access Code Character Set type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AccessCode_CharacterSet) {
    /**
     * Arabic Numerals.
     * Access code consists of Arabic numerals (0-9)
     */
    kHAPCharacteristicValue_AccessCode_CharacterSet_ArabicNumerals = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AccessCode_CharacterSet);

/**
 * Access Code Supported Configuration
 */
typedef struct {
    /** Character set */
    uint8_t characterSet;
    /** The minimum length for an access code */
    uint8_t minimumLength;
    /** The maximum length for an access code */
    uint8_t maximumLength;
    /** The maximum number of access codes supported */
    uint16_t maximumAccessCodes;
} HAPCharacteristicValue_AccessCodeSupportedConfiguration;

/**@}*/

/**
 * Brightness.
 *
 * This characteristic describes a perceived level of brightness, e.g., for lighting, and can be used for backlights or
 * color. The value is expressed as a percentage (%) of the maximum level of supported brightness.
 *
 * - Format: Int
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.2 Brightness
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Brightness "brightness"

extern const HAPUUID kHAPCharacteristicType_Brightness;
/**@}*/

/**
 * Cooling Threshold Temperature.
 *
 * This characteristic describes the cooling threshold in Celsius for accessories that support simultaneous heating and
 * cooling. The value of this characteristic represents the maximum temperature that must be reached before cooling is
 * turned on.
 *
 * For example, if the `Target Heating Cooling State` is set to "Auto" and the current temperature goes above the
 * maximum temperature, then the cooling mechanism should turn on to decrease the current temperature until the
 * minimum temperature is reached.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 10
 * - Maximum Value: 35
 * - Step Value: 0.1
 * - Unit: Celsius
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.3 Cooling Threshold Temperature
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CoolingThresholdTemperature "temperature.cooling-threshold"

extern const HAPUUID kHAPCharacteristicType_CoolingThresholdTemperature;
/**@}*/

/**
 * Current Door State.
 *
 * This characteristic describes the current state of a door.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 4
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.4 Current Door State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentDoorState "door-state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentDoorState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentDoorState) {
    /** Open. The door is fully open. */
    kHAPCharacteristicValue_CurrentDoorState_Open = 0,

    /** Closed. The door is fully closed. */
    kHAPCharacteristicValue_CurrentDoorState_Closed = 1,

    /** Opening. The door is actively opening. */
    kHAPCharacteristicValue_CurrentDoorState_Opening = 2,

    /** Closing. The door is actively closing. */
    kHAPCharacteristicValue_CurrentDoorState_Closing = 3,

    /** Stopped. The door is not moving, and it is not fully open nor fully closed. */
    kHAPCharacteristicValue_CurrentDoorState_Stopped = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentDoorState);
/**@}*/

/**
 * Current Heating Cooling State.
 *
 * This characteristic describes the current mode of an accessory that supports cooling or heating its environment,
 * e.g., a thermostat is "heating" a room to 75 degrees Fahrenheit.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.5 Current Heating Cooling State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentHeatingCoolingState "heating-cooling.current"

extern const HAPUUID kHAPCharacteristicType_CurrentHeatingCoolingState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentHeatingCoolingState) {
    /** Off. */
    kHAPCharacteristicValue_CurrentHeatingCoolingState_Off = 0,

    /** Heat. The Heater is currently on. */
    kHAPCharacteristicValue_CurrentHeatingCoolingState_Heat = 1,

    /** Cool. Cooler is currently on. */
    kHAPCharacteristicValue_CurrentHeatingCoolingState_Cool = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentHeatingCoolingState);
/**@}*/

/**
 * Current Relative Humidity.
 *
 * This characteristic describes the current relative humidity of the accessory's environment.
 * The value is expressed as a percentage (%).
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.6 Current Relative Humidity
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentRelativeHumidity "relative-humidity.current"

extern const HAPUUID kHAPCharacteristicType_CurrentRelativeHumidity;
/**@}*/

/**
 * Current Temperature.
 *
 * This characteristic describes the current temperature of the environment in Celsius irrespective of display units
 * chosen in `Temperature Display Units`.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 0.1
 * - Unit: Celsius
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.7 Current Temperature
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentTemperature "temperature.current"

extern const HAPUUID kHAPCharacteristicType_CurrentTemperature;
/**@}*/

/**
 * Heating Threshold Temperature.
 *
 * This characteristic describes the heating threshold in Celsius for accessories that support simultaneous heating and
 * cooling. The value of this characteristic represents the minimum temperature that must be reached before heating is
 * turned on.
 *
 * For example, if the `Target Heating Cooling State` is set to "Auto" and the current temperature goes below the
 * minimum temperature, then the heating mechanism should turn on to increase the current temperature until the
 * minimum temperature is reached.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 25
 * - Step Value: 0.1
 * - Unit: Celsius
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.10 Heating Threshold Temperature
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_HeatingThresholdTemperature "temperature.heating-threshold"

extern const HAPUUID kHAPCharacteristicType_HeatingThresholdTemperature;
/**@}*/

/**
 * Hue.
 *
 * This characteristic describes hue or color.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 360
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.11 Hue
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Hue "hue"

extern const HAPUUID kHAPCharacteristicType_Hue;
/**@}*/

/**
 * Identify.
 *
 * This characteristic is used to cause the accessory to run its identify routine.
 *
 * Only the `Accessory Information` is allowed to contain the Identify characteristic.
 *
 * - Format: Bool
 * - Permissions: Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.12 Identify
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Identify "identify"

extern const HAPUUID kHAPCharacteristicType_Identify;
/**@}*/

/**
 * Lock Current State.
 *
 * The current state of the physical security mechanism (e.g., deadbolt).
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.13 Lock Current State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_LockCurrentState "lock-mechanism.current-state"

extern const HAPUUID kHAPCharacteristicType_LockCurrentState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_LockCurrentState) {
    /** Unsecured. */
    kHAPCharacteristicValue_LockCurrentState_Unsecured = 0,

    /** Secured. */
    kHAPCharacteristicValue_LockCurrentState_Secured = 1,

    /** Jammed. */
    kHAPCharacteristicValue_LockCurrentState_Jammed = 2,

    /** Unknown. */
    kHAPCharacteristicValue_LockCurrentState_Unknown = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_LockCurrentState);
/**@}*/

/**
 * Lock Target State.
 *
 * The target state of the physical security mechanism (e.g., deadbolt).
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.14 Lock Target State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_LockTargetState "lock-mechanism.target-state"

extern const HAPUUID kHAPCharacteristicType_LockTargetState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_LockTargetState) {
    /** Unsecured. */
    kHAPCharacteristicValue_LockTargetState_Unsecured = 0,

    /** Secured. */
    kHAPCharacteristicValue_LockTargetState_Secured = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_LockTargetState);
/**@}*/

/**
 * Manufacturer.
 *
 * This characteristic contains the name of the company whose brand will appear on the accessory, e.g., "Acme".
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.15 Manufacturer
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Manufacturer "manufacturer"

extern const HAPUUID kHAPCharacteristicType_Manufacturer;
/**@}*/

/**
 * Model.
 *
 * This characteristic contains the manufacturer-specific model of the accessory, e.g., "A1234". The minimum length of
 * this characteristic must be 1.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.16 Model
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Model "model"

extern const HAPUUID kHAPCharacteristicType_Model;
/**@}*/

/**
 * Motion Detected.
 *
 * This characteristic indicates if motion (e.g., a person moving) was detected.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.17 Motion Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_MotionDetected "motion-detected"

extern const HAPUUID kHAPCharacteristicType_MotionDetected;
/**@}*/

/**
 * Name.
 *
 * This characteristic describes a name and must not be a null value.
 *
 * The naming of the accessory:
 * - must start and end with a letter or number with the only exception of ending with a "." (period).
 * - may have special characters " - " (dashes), " " " (quotes), " ' " (apostrophe), " , " (comma),
 *   " . " (period), " # " (hash) and " & " (ampersand) only.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.18 Name
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Name "name"

extern const HAPUUID kHAPCharacteristicType_Name;
/**@}*/

/**
 * Obstruction Detected.
 *
 * This characteristic describes the current state of an obstruction sensor, such as one that is used in a garage door.
 * If the state is true then there is an obstruction detected.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.19 Obstruction Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ObstructionDetected "obstruction-detected"

extern const HAPUUID kHAPCharacteristicType_ObstructionDetected;
/**@}*/

/**
 * On.
 *
 * This characteristic represents the states for "on" and "off".
 *
 * - Format: Bool
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.20 On
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_On "on"

extern const HAPUUID kHAPCharacteristicType_On;
/**@}*/

/**
 * Outlet In Use.
 *
 * This characteristic describes if the power outlet has an appliance e.g., a floor lamp, physically plugged in. This
 * characteristic is set to "True" even if the plugged-in appliance is off.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.21 Outlet In Use
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_OutletInUse "outlet-in-use"

extern const HAPUUID kHAPCharacteristicType_OutletInUse;
/**@}*/

/**
 * Rotation Direction.
 *
 * This characteristic describes the direction of rotation of a fan.
 *
 * - Format: Int
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.22 Rotation Direction
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RotationDirection "rotation.direction"

extern const HAPUUID kHAPCharacteristicType_RotationDirection;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_RotationDirection) {
    /** Clockwise. */
    kHAPCharacteristicValue_RotationDirection_Clockwise = 0,

    /** Counter-clockwise. */
    kHAPCharacteristicValue_RotationDirection_CounterClockwise = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_RotationDirection);
/**@}*/

/**
 * Rotation Speed.
 *
 * This characteristic describes the rotation speed of a fan.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.23 Rotation Speed
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RotationSpeed "rotation.speed"

extern const HAPUUID kHAPCharacteristicType_RotationSpeed;
/**@}*/

/**
 * Saturation.
 *
 * This characteristic describes color saturation.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.24 Saturation
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Saturation "saturation"

extern const HAPUUID kHAPCharacteristicType_Saturation;
/**@}*/

/**
 * Serial Number.
 *
 * This characteristic contains the manufacturer-specific serial number of the accessory, e.g., 1A2B3C4D5E6F. The
 * length must be greater than 1.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.25 Serial Number
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SerialNumber "serial-number"

extern const HAPUUID kHAPCharacteristicType_SerialNumber;
/**@}*/

/**
 * Target Door State.
 *
 * This characteristic describes the target state of a door.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.26 Target Door State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetDoorState "door-state.target"

extern const HAPUUID kHAPCharacteristicType_TargetDoorState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetDoorState) {
    /** Open. */
    kHAPCharacteristicValue_TargetDoorState_Open = 0,

    /** Closed. */
    kHAPCharacteristicValue_TargetDoorState_Closed = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetDoorState);
/**@}*/

/**
 * Target Heating Cooling State.
 *
 * This characteristic describes the target mode of an accessory that supports heating/cooling, e.g., a thermostat.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.27 Target Heating Cooling State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetHeatingCoolingState "heating-cooling.target"

extern const HAPUUID kHAPCharacteristicType_TargetHeatingCoolingState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetHeatingCoolingState) {
    /** Off. */
    kHAPCharacteristicValue_TargetHeatingCoolingState_Off = 0,

    /** Heat. If the current temperature is below the target temperature then turn on heating. */
    kHAPCharacteristicValue_TargetHeatingCoolingState_Heat = 1,

    /** Cool. If the current temperature is above the target temperature then turn on cooling. */
    kHAPCharacteristicValue_TargetHeatingCoolingState_Cool = 2,

    /**
     * Auto. Turn on heating or cooling to maintain temperature within the heating and cooling threshold
     * of the target temperature.
     */
    kHAPCharacteristicValue_TargetHeatingCoolingState_Auto = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetHeatingCoolingState);
/**@}*/

/**
 * Target Relative Humidity.
 *
 * This characteristic describes the target relative humidity that the accessory is actively attempting to reach.
 * The value is expressed as a percentage (%).
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.28 Target Relative Humidity
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetRelativeHumidity "relative-humidity.target"

extern const HAPUUID kHAPCharacteristicType_TargetRelativeHumidity;
/**@}*/

/**
 * Target Temperature.
 *
 * This characteristic describes the target temperature in Celsius that the accessory is actively attempting to reach.
 * For example, a thermostat cooling a room to 75 degrees Fahrenheit would set the target temperature value to
 * 23.9 degrees Celsius.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 10.0
 * - Maximum Value: 38.0
 * - Step Value: 0.1
 * - Unit: Celsius
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.29 Target Temperature
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetTemperature "temperature.target"

extern const HAPUUID kHAPCharacteristicType_TargetTemperature;
/**@}*/

/**
 * Temperature Display Units.
 *
 * This characteristic describes units of temperature used for presentation purposes (e.g., the units of temperature
 * displayed on the screen).
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.30 Temperature Display Units
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TemperatureDisplayUnits "temperature.units"

extern const HAPUUID kHAPCharacteristicType_TemperatureDisplayUnits;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TemperatureDisplayUnits) {
    /** Celsius. */
    kHAPCharacteristicValue_TemperatureDisplayUnits_Celsius = 0,

    /** Fahrenheit. */
    kHAPCharacteristicValue_TemperatureDisplayUnits_Fahrenheit = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TemperatureDisplayUnits);
/**@}*/

/**
 * Version.
 *
 * This characteristic contains a version string.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.31 Version
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Version "version"

extern const HAPUUID kHAPCharacteristicType_Version;
/**@}*/

/**
 * Pair Setup.
 *
 * Accessories must accept reads and writes to this characteristic to perform Pair Setup.
 *
 * - Format: TLV
 * - Permissions: Read, Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.14.1.1 Pair Setup
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PairSetup "pairing.pair-setup"

extern const HAPUUID kHAPCharacteristicType_PairSetup;
/**@}*/

/**
 * Pair Verify.
 *
 * Accessories must accept reads and writes to this characteristic to perform Pair Verify.
 *
 * - Format: TLV
 * - Permissions: Read, Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.14.1.2 Pair Verify
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PairVerify "pairing.pair-verify"

extern const HAPUUID kHAPCharacteristicType_PairVerify;
/**@}*/

/**
 * Pairing Features.
 *
 * Read-only characteristic that exposes pairing features must be supported by the accessory.
 *
 * - Format: UInt8
 * - Permissions: Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.14.1.3 Pairing Features
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PairingFeatures "pairing.features"

extern const HAPUUID kHAPCharacteristicType_PairingFeatures;

HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_PairingFeatures) {
    /** Supports Apple Authentication Coprocessor. */
    kHAPCharacteristicValue_PairingFeatures_SupportsAppleAuthenticationCoprocessor = 1U << 0U,

    /** Supports Software Authentication. */
    kHAPCharacteristicValue_PairingFeatures_SupportsSoftwareAuthentication = 1U << 1U,

} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_PairingFeatures);
/**@}*/

/**
 * Pairing Pairings.
 *
 * Accessories must accept reads and writes to this characteristic to add, remove, and list pairings.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.14.1.4 Pairing Pairings
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PairingPairings "pairing.pairings"

extern const HAPUUID kHAPCharacteristicType_PairingPairings;
/**@}*/

/**
 * Firmware Revision.
 *
 * This characteristic describes a firmware revision string x[.y[.z]] (e.g., "100.1.1"):
 * - \<x\> is the major version number, required.
 * - \<y\> is the minor version number, required if it is non-zero or if \<z\> is present.
 * - \<z\> is the revision version number, required if non-zero.
 *
 * The firmware revision must follow the below rules:
 * - \<x\> is incremented when there is significant change. e.g., 1.0.0, 2.0.0, 3.0.0, etc.
 * - \<y\> is incremented when minor changes are introduced such as 1.1.0, 2.1.0, 3.1.0 etc.
 * - \<z\> is incremented when bug-fixes are introduced such as 1.0.1, 2.0.1, 3.0.1 etc.
 * - Subsequent firmware updates can have a lower \<y\> version only if \<x\> is incremented
 * - Subsequent firmware updates can have a lower \<z\> version only if \<x\> or \<y\> is incremented
 * - Each number (major, minor and revision version) must not be greater than (2^32 -1)
 *
 * The characteristic value must change after every firmware update.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.8 Firmware Revision
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_FirmwareRevision "firmware.revision"

extern const HAPUUID kHAPCharacteristicType_FirmwareRevision;
/**@}*/

/**
 * Hardware Revision.
 *
 * This characteristic describes a hardware revision string x[.y[.z]] (e.g., "100.1.1") and tracked when the board or
 * components of the same accessory is changed:
 * - \<x\> is the major version number, required.
 * - \<y\> is the minor version number, required if it is non-zero or if \<z\> is present.
 * - \<z\> is the revision version number, required if non-zero.
 *
 * The hardware revision must follow the below rules:
 * - \<x\> is incremented when there is significant change, e.g., 1.0.0, 2.0.0, 3.0.0, etc.
 * - \<y\> is incremented when minor changes are introduced such as 1.1.0, 2.1.0, 3.1.0 etc.
 * - \<z\> is incremented when bug-fixes are introduced such as 1.0.1, 2.0.1, 3.0.1 etc.
 * - Subsequent hardware updates can have a lower \<y\> version only if \<x\> is incremented
 * - Subsequent hardware updates can have a lower \<z\> version only if \<x\> or \<y\> is incremented
 *
 * The characteristic value must change after every hardware update.
 *
 * - Format: String
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.9 Hardware Revision
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_HardwareRevision "hardware.revision"

extern const HAPUUID kHAPCharacteristicType_HardwareRevision;
/**@}*/

/**
 * Hardware Finish.
 *
 * - Format: TLV8
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.186 Hardware Finish
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_HardwareFinish "hardware.finish"

extern const HAPUUID kHAPCharacteristicType_HardwareFinish;

/**
 * Hardware Finish
 */
typedef struct {
    /** RGB color value to describe the physical appearance of the hardware */
    uint32_t rgbColorValue;
} HAPCharacteristicValue_HardwareFinish;
/**@}*/

/**
 * Air Particulate Density.
 *
 * This characteristic indicates the current air particulate matter density in micrograms/m^3.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.32 Air Particulate Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AirParticulateDensity "air-particulate.density"

extern const HAPUUID kHAPCharacteristicType_AirParticulateDensity;
/**@}*/

/**
 * Air Particulate Size.
 *
 * This characteristic indicates the size of air particulate matter in micrometers.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.33 Air Particulate Size
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AirParticulateSize "air-particulate.size"

extern const HAPUUID kHAPCharacteristicType_AirParticulateSize;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AirParticulateSize) {
    /** 2.5 Micrometers. */
    kHAPCharacteristicValue_AirParticulateSize_2_5 = 0,

    /** 10 Micrometers. */
    kHAPCharacteristicValue_AirParticulateSize_10 = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AirParticulateSize);
/**@}*/

/**
 * Security System Current State.
 *
 * This characteristic describes the state of a security system.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 4
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.34 Security System Current State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SecuritySystemCurrentState "security-system-state.current"

extern const HAPUUID kHAPCharacteristicType_SecuritySystemCurrentState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SecuritySystemCurrentState) {
    /** Stay Arm. The home is occupied and the residents are active. e.g., morning or evenings. */
    kHAPCharacteristicValue_SecuritySystemCurrentState_StayArm = 0,

    /** Away Arm. The home is unoccupied. */
    kHAPCharacteristicValue_SecuritySystemCurrentState_AwayArm = 1,

    /** Night Arm. The home is occupied and the residents are sleeping. */
    kHAPCharacteristicValue_SecuritySystemCurrentState_NightArm = 2,

    /** Disarmed. */
    kHAPCharacteristicValue_SecuritySystemCurrentState_Disarmed = 3,

    /** Alarm Triggered. */
    kHAPCharacteristicValue_SecuritySystemCurrentState_AlarmTriggered = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SecuritySystemCurrentState);
/**@}*/

/**
 * Security System Target State.
 *
 * This characteristic describes the target state of the security system.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.35 Security System Target State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SecuritySystemTargetState "security-system-state.target"

extern const HAPUUID kHAPCharacteristicType_SecuritySystemTargetState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SecuritySystemTargetState) {
    /** Stay Arm. The home is occupied and the residents are active. e.g., morning or evenings. */
    kHAPCharacteristicValue_SecuritySystemTargetState_StayArm = 0,

    /** Away Arm. The home is unoccupied. */
    kHAPCharacteristicValue_SecuritySystemTargetState_AwayArm = 1,

    /** Night Arm. The home is occupied and the residents are sleeping. */
    kHAPCharacteristicValue_SecuritySystemTargetState_NightArm = 2,

    /** Disarm. */
    kHAPCharacteristicValue_SecuritySystemTargetState_Disarm = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SecuritySystemTargetState);
/**@}*/

/**
 * Battery Level.
 *
 * This characteristic describes the current level of the battery.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.36 Battery Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_BatteryLevel "battery-level"

extern const HAPUUID kHAPCharacteristicType_BatteryLevel;
/**@}*/

/**
 * Carbon Monoxide Detected.
 *
 * This characteristic indicates if a sensor detects abnormal levels of Carbon Monoxide. Value should revert to 0 after
 * the Carbon Monoxide levels drop to normal levels.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.37 Carbon Monoxide Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonMonoxideDetected "carbon-monoxide.detected"

extern const HAPUUID kHAPCharacteristicType_CarbonMonoxideDetected;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CarbonMonoxideDetected) {
    /** Carbon Monoxide levels are normal. */
    kHAPCharacteristicValue_CarbonMonoxideDetected_Normal = 0,

    /** Carbon Monoxide levels are abnormal. */
    kHAPCharacteristicValue_CarbonMonoxideDetected_Abnormal = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CarbonMonoxideDetected);
/**@}*/

/**
 * Contact Sensor State.
 *
 * This characteristic describes the state of a door/window contact sensor. A value of 0 indicates that the contact is
 * detected. A value of 1 indicates that the contact is not detected.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.38 Contact Sensor State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ContactSensorState "contact-state"

extern const HAPUUID kHAPCharacteristicType_ContactSensorState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ContactSensorState) {
    /** Contact is detected. */
    kHAPCharacteristicValue_ContactSensorState_Detected = 0,

    /** Contact is not detected. */
    kHAPCharacteristicValue_ContactSensorState_NotDetected = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ContactSensorState);
/**@}*/

/**
 * Current Ambient Light Level.
 *
 * This characteristic indicates the current light level. The value is expressed in Lux units (lumens / m^2).
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0.0001
 * - Maximum Value: 100000
 * - Unit: Lux
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.39 Current Ambient Light Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentAmbientLightLevel "light-level.current"

extern const HAPUUID kHAPCharacteristicType_CurrentAmbientLightLevel;
/**@}*/

/**
 * Current Horizontal Tilt Angle.
 *
 * This characteristic describes the current angle of horizontal slats for accessories such as windows, fans, portable
 * heater/coolers etc. This characteristic takes values between -90 and 90. A value of 0 indicates that the slats are
 * rotated to a fully open position. A value of -90 indicates that the slats are rotated all the way in a direction
 * where the user-facing edge is higher than the window-facing edge.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.40 Current Horizontal Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentHorizontalTiltAngle "horizontal-tilt.current"

extern const HAPUUID kHAPCharacteristicType_CurrentHorizontalTiltAngle;
/**@}*/

/**
 * Current Position.
 *
 * This characteristic describes the current position of accessories. This characteristic can be used with doors,
 * windows, awnings or window coverings. For windows and doors, a value of 0 indicates that a window (or door) is fully
 * closed while a value of 100 indicates a fully open position. For blinds/shades/awnings, a value of 0 indicates a
 * position that permits the least light and a value of 100 indicates a position that allows most light.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.41 Current Position
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentPosition "position.current"

extern const HAPUUID kHAPCharacteristicType_CurrentPosition;
/**@}*/

/**
 * Current Vertical Tilt Angle.
 *
 * This characteristic describes the current angle of vertical slats for accessories such as windows, fans, portable
 * heater/coolers etc. This characteristic takes values between -90 and 90. A value of 0 indicates that the slats are
 * rotated to be fully open. A value of -90 indicates that the slats are rotated all the way in a direction where the
 * user-facing edge is to the left of the window-facing edge.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.42 Current Vertical Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentVerticalTiltAngle "vertical-tilt.current"

extern const HAPUUID kHAPCharacteristicType_CurrentVerticalTiltAngle;
/**@}*/

/**
 * Hold Position.
 *
 * This characteristic causes the service such as door or window covering to stop at its current position. A value of 1
 * must hold the state of the accessory. For e.g, the window must stop moving when this characteristic is written a
 * value of 1. A value of 0 should be ignored.
 *
 * A write to `Target Position` characteristic will release the hold.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.43 Hold Position
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_HoldPosition "position.hold"

extern const HAPUUID kHAPCharacteristicType_HoldPosition;
/**@}*/

/**
 * Leak Detected.
 *
 * This characteristic indicates if a sensor detected a leak (e.g., water leak, gas leak). A value of 1 indicates that a
 * leak is detected. Value should return to 0 when leak stops.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.44 Leak Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_LeakDetected "leak-detected"

extern const HAPUUID kHAPCharacteristicType_LeakDetected;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_LeakDetected) {
    /** Leak is not detected. */
    kHAPCharacteristicValue_LeakDetected_NotDetected = 0,

    /** Leak is detected. */
    kHAPCharacteristicValue_LeakDetected_Detected = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_LeakDetected);
/**@}*/

/**
 * Occupancy Detected.
 *
 * This characteristic indicates if occupancy was detected (e.g., a person present). A value of 1 indicates occupancy is
 * detected. Value should return to 0 when occupancy is not detected.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.45 Occupancy Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_OccupancyDetected "occupancy-detected"

extern const HAPUUID kHAPCharacteristicType_OccupancyDetected;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_OccupancyDetected) {
    /** Occupancy is not detected. */
    kHAPCharacteristicValue_OccupancyDetected_NotDetected = 0,

    /** Occupancy is detected. */
    kHAPCharacteristicValue_OccupancyDetected_Detected = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_OccupancyDetected);
/**@}*/

/**
 * Position State.
 *
 * This characteristic describes the state of the position of accessories. This characteristic can be used with doors,
 * windows, awnings or window coverings for presentation purposes.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.46 Position State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PositionState "position.state"

extern const HAPUUID kHAPCharacteristicType_PositionState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_PositionState) {
    /** Going to the minimum value specified in metadata. */
    kHAPCharacteristicValue_PositionState_GoingToMinimum = 0,

    /** Going to the maximum value specified in metadata. */
    kHAPCharacteristicValue_PositionState_GoingToMaximum = 1,

    /** Stopped. */
    kHAPCharacteristicValue_PositionState_Stopped = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_PositionState);
/**@}*/

/**
 * Programmable Switch Event.
 *
 * This characteristic describes an event generated by a programmable switch. Reading this characteristic must return
 * the last event triggered for BLE. For IP accessories, the accessory must set the value of Paired Read to
 * null (i.e., "value" : null) in the attribute database. A read of this characteristic must always return a null value
 * for IP accessories. The value must only be reported in the events ("ev") property.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.47 Programmable Switch Event
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent "input-event"

extern const HAPUUID kHAPCharacteristicType_ProgrammableSwitchEvent;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ProgrammableSwitchEvent) {
    /** Single Press. */
    kHAPCharacteristicValue_ProgrammableSwitchEvent_SinglePress = 0,

    /** Double Press. */
    kHAPCharacteristicValue_ProgrammableSwitchEvent_DoublePress = 1,

    /** Long Press. */
    kHAPCharacteristicValue_ProgrammableSwitchEvent_LongPress = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ProgrammableSwitchEvent);
/**@}*/

/**
 * Status Active.
 *
 * This characteristic describes an accessory's current working status. A value of true indicates that the accessory is
 * active and is functioning without any errors.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.48 Status Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StatusActive "status-active"

extern const HAPUUID kHAPCharacteristicType_StatusActive;
/**@}*/

/**
 * Smoke Detected.
 *
 * This characteristic indicates if a sensor detects abnormal levels of smoke. A value of 1 indicates that smoke levels
 * are abnormal. Value should return to 0 when smoke levels are normal.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.49 Smoke Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SmokeDetected "smoke-detected"

extern const HAPUUID kHAPCharacteristicType_SmokeDetected;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SmokeDetected) {
    /** Smoke is not detected. */
    kHAPCharacteristicValue_SmokeDetected_NotDetected = 0,

    /** Smoke is detected. */
    kHAPCharacteristicValue_SmokeDetected_Detected = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SmokeDetected);
/**@}*/

/**
 * Status Fault.
 *
 * This characteristic describes an accessory which has a fault. A non-zero value indicates that the accessory has
 * experienced a fault that may be interfering with its intended functionality. A value of 0 indicates that there is no
 * fault.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.50 Status Fault
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StatusFault "status-fault"

extern const HAPUUID kHAPCharacteristicType_StatusFault;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StatusFault) {
    /** No Fault. */
    kHAPCharacteristicValue_StatusFault_None = 0,

    /** General Fault. */
    kHAPCharacteristicValue_StatusFault_General = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StatusFault);
/**@}*/

/**
 * Status Jammed.
 *
 * This characteristic describes an accessory which is in a jammed state. A status of 1 indicates that an accessory's
 * mechanisms are jammed prevents it from functionality normally. Value should return to 0 when conditions that jam the
 * accessory are rectified.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.51 Status Jammed
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StatusJammed "status-jammed"

extern const HAPUUID kHAPCharacteristicType_StatusJammed;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StatusJammed) {
    /** Not Jammed. */
    kHAPCharacteristicValue_StatusJammed_NotJammed = 0,

    /** Jammed. */
    kHAPCharacteristicValue_StatusJammed_Jammed = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StatusJammed);
/**@}*/

/**
 * Status Low Battery.
 *
 * This characteristic describes an accessory's battery status. A status of 1 indicates that the battery level of the
 * accessory is low. Value should return to 0 when the battery charges to a level thats above the low threshold.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.52 Status Low Battery
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StatusLowBattery "status-lo-batt"

extern const HAPUUID kHAPCharacteristicType_StatusLowBattery;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StatusLowBattery) {
    /** Battery level is normal. */
    kHAPCharacteristicValue_StatusLowBattery_Normal = 0,

    /** Battery level is low. */
    kHAPCharacteristicValue_StatusLowBattery_Low = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StatusLowBattery);
/**@}*/

/**
 * Status Tampered.
 *
 * This characteristic describes an accessory which has been tampered with. A status of 1 indicates that the accessory
 * has been tampered with. Value should return to 0 when the accessory has been reset to a non-tampered state.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.53 Status Tampered
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StatusTampered "status-tampered"

extern const HAPUUID kHAPCharacteristicType_StatusTampered;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StatusTampered) {
    /** Accessory is not tampered. */
    kHAPCharacteristicValue_StatusTampered_NotTampered = 0,

    /** Accessory is tampered with. */
    kHAPCharacteristicValue_StatusTampered_Tampered = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StatusTampered);
/**@}*/

/**
 * Target Horizontal Tilt Angle.
 *
 * This characteristic describes the target angle of horizontal slats for accessories such as windows, fans, portable
 * heater/coolers etc.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.54 Target Horizontal Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetHorizontalTiltAngle "horizontal-tilt.target"

extern const HAPUUID kHAPCharacteristicType_TargetHorizontalTiltAngle;
/**@}*/

/**
 * Target Position.
 *
 * This characteristic describes the target position of accessories. This characteristic can be used with doors,
 * windows, awnings or window coverings. For windows and doors, a value of 0 indicates that a window (or door) is fully
 * closed while a value of 100 indicates a fully open position. For blinds/shades/awnings, a value of 0 indicates a
 * position that permits the least light and a value of 100 indicates a position that allows most light.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.55 Target Position
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetPosition "position.target"

extern const HAPUUID kHAPCharacteristicType_TargetPosition;
/**@}*/

/**
 * Target Vertical Tilt Angle.
 *
 * This characteristic describes the target angle of vertical slats for accessories such as windows, fans, portable
 * heater/coolers etc.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.56 Target Vertical Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetVerticalTiltAngle "vertical-tilt.target"

extern const HAPUUID kHAPCharacteristicType_TargetVerticalTiltAngle;
/**@}*/

/**
 * Security System Alarm Type.
 *
 * This characteristic describes the type of alarm triggered by a security system. A value of 1 indicates an 'unknown'
 * cause. Value should revert to 0 when the alarm conditions are cleared.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.57 Security System Alarm Type
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SecuritySystemAlarmType "security-system.alarm-type"

extern const HAPUUID kHAPCharacteristicType_SecuritySystemAlarmType;
/**@}*/

/**
 * Charging State.
 *
 * This characteristic describes the charging state of a battery or an accessory.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.58 Charging State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ChargingState "charging-state"

extern const HAPUUID kHAPCharacteristicType_ChargingState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ChargingState) {
    /** Not Charging. */
    kHAPCharacteristicValue_ChargingState_NotCharging = 0,

    /** Charging. */
    kHAPCharacteristicValue_ChargingState_Charging = 1,

    /** Not Chargeable. */
    kHAPCharacteristicValue_ChargingState_NotChargeable = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ChargingState);
/**@}*/

/**
 * Carbon Monoxide Level.
 *
 * This characteristic indicates the Carbon Monoxide levels detected in parts per million (ppm).
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.59 Carbon Monoxide Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonMonoxideLevel "carbon-monoxide.level"

extern const HAPUUID kHAPCharacteristicType_CarbonMonoxideLevel;
/**@}*/

/**
 * Carbon Monoxide Peak Level.
 *
 * This characteristic indicates the highest detected level (ppm) of Carbon Monoxide detected by a sensor.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.60 Carbon Monoxide Peak Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonMonoxidePeakLevel "carbon-monoxide.peak-level"

extern const HAPUUID kHAPCharacteristicType_CarbonMonoxidePeakLevel;
/**@}*/

/**
 * Carbon Dioxide Detected.
 *
 * This characteristic indicates if a sensor detects abnormal levels of Carbon Dioxide. Value should revert to 0 after
 * the Carbon Dioxide levels drop to normal levels.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.61 Carbon Dioxide Detected
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonDioxideDetected "carbon-dioxide.detected"

extern const HAPUUID kHAPCharacteristicType_CarbonDioxideDetected;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CarbonDioxideDetected) {
    /** Carbon Dioxide levels are normal. */
    kHAPCharacteristicValue_CarbonDioxideDetected_Normal = 0,

    /** Carbon Dioxide levels are abnormal. */
    kHAPCharacteristicValue_CarbonDioxideDetected_Abnormal = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CarbonDioxideDetected);
/**@}*/

/**
 * Carbon Dioxide Level.
 *
 * This characteristic indicates the detected level of Carbon Dioxide in parts per million (ppm).
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.62 Carbon Dioxide Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonDioxideLevel "carbon-dioxide.level"

extern const HAPUUID kHAPCharacteristicType_CarbonDioxideLevel;
/**@}*/

/**
 * Carbon Dioxide Peak Level.
 *
 * This characteristic indicates the highest detected level (ppm) of carbon dioxide detected by a sensor.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.63 Carbon Dioxide Peak Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CarbonDioxidePeakLevel "carbon-dioxide.peak-level"

extern const HAPUUID kHAPCharacteristicType_CarbonDioxidePeakLevel;
/**@}*/

/**
 * Air Quality.
 *
 * This characteristic describes the subject assessment of air quality by an accessory.
 *
 * This characteristic requires iOS 9 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 5
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.64 Air Quality
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AirQuality "air-quality"

extern const HAPUUID kHAPCharacteristicType_AirQuality;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AirQuality) {
    /** Unknown. */
    kHAPCharacteristicValue_AirQuality_Unknown = 0,

    /** Excellent. */
    kHAPCharacteristicValue_AirQuality_Excellent = 1,

    /** Good. */
    kHAPCharacteristicValue_AirQuality_Good = 2,

    /** Fair. */
    kHAPCharacteristicValue_AirQuality_Fair = 3,

    /** Inferior. */
    kHAPCharacteristicValue_AirQuality_Inferior = 4,

    /** Poor. */
    kHAPCharacteristicValue_AirQuality_Poor = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AirQuality);
/**@}*/

/**
 * Service Signature.
 *
 * - Format: Data
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.4.4.5.4 Service Signature Characteristic
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ServiceSignature "service-signature"

extern const HAPUUID kHAPCharacteristicType_ServiceSignature;
/**@}*/

/**
 * Accessory Flags.
 *
 * When set indicates accessory requires additional setup. Use of Accessory Flags requires written approval by Apple in
 * advance.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.78 Accessory Flags
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_AccessoryFlags "accessory-properties"

extern const HAPUUID kHAPCharacteristicType_AccessoryFlags;

HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_AccessoryFlags) {
    /** Requires additional setup. */
    kHAPCharacteristicValue_AccessoryFlags_RequiresAdditionalSetup = 1U << 0U
} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_AccessoryFlags);
/**@}*/

/**@}*/

/**
 * Lock Physical Controls.
 *
 * This characteristic describes a way to lock a set of physical controls on an accessory (e.g., child lock).
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.79 Lock Physical Controls
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_LockPhysicalControls "lock-physical-controls"

extern const HAPUUID kHAPCharacteristicType_LockPhysicalControls;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_LockPhysicalControls) {
    /** Control lock disabled. */
    kHAPCharacteristicValue_LockPhysicalControls_Disabled = 0,

    /** Control lock enabled. */
    kHAPCharacteristicValue_LockPhysicalControls_Enabled = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_LockPhysicalControls);
/**@}*/

/**
 * Target Air Purifier State.
 *
 * This characteristic describes the target state of the air purifier.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.86 Target Air Purifier State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetAirPurifierState "air-purifier.state.target"

extern const HAPUUID kHAPCharacteristicType_TargetAirPurifierState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetAirPurifierState) {
    /** Manual. */
    kHAPCharacteristicValue_TargetAirPurifierState_Manual = 0,

    /** Auto. */
    kHAPCharacteristicValue_TargetAirPurifierState_Auto = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetAirPurifierState);
/**@}*/

/**
 * Current Air Purifier State.
 *
 * This characteristic describes the current state of the air purifier.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.80 Current Air Purifier State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentAirPurifierState "air-purifier.state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentAirPurifierState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentAirPurifierState) {
    /** Inactive. */
    kHAPCharacteristicValue_CurrentAirPurifierState_Inactive = 0,

    /** Idle. */
    kHAPCharacteristicValue_CurrentAirPurifierState_Idle = 1,

    /** Purifying Air. */
    kHAPCharacteristicValue_CurrentAirPurifierState_PurifyingAir = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentAirPurifierState);
/**@}*/

/**
 * Current Slat State.
 *
 * This characteristic describes the current state of the slats.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.81 Current Slat State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentSlatState "slat.state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentSlatState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentSlatState) {
    /** Fixed. */
    kHAPCharacteristicValue_CurrentSlatState_Fixed = 0,

    /** Jammed. */
    kHAPCharacteristicValue_CurrentSlatState_Jammed = 1,

    /** Swinging. */
    kHAPCharacteristicValue_CurrentSlatState_Swinging = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentSlatState);
/**@}*/

/**
 * Filter Life Level.
 *
 * This characteristic describes the current filter life level.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.83 Filter Life Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_FilterLifeLevel "filter.life-level"

extern const HAPUUID kHAPCharacteristicType_FilterLifeLevel;
/**@}*/

/**
 * Filter Change Indication.
 *
 * This characteristic describes if a filter needs to be changed.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.84 Filter Change Indication
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_FilterChangeIndication "filter.change-indication"

extern const HAPUUID kHAPCharacteristicType_FilterChangeIndication;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_FilterChangeIndication) {
    /** Filter does not need to be changed. */
    kHAPCharacteristicValue_FilterChangeIndication_Ok = 0,

    /** Filter needs to be changed. */
    kHAPCharacteristicValue_FilterChangeIndication_Change = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_FilterChangeIndication);
/**@}*/

/**
 * Reset Filter Indication.
 *
 * This characteristic allows a user to reset the filter indication. When the value of 1 is written to this
 * characteristic by the user, the accessory should reset it to 0 once the relevant action to reset the filter
 * indication is executed. If the accessory supports Filter Change Indication, the value of that characteristic should
 * also reset back to 0.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Write
 * - Minimum Value: 1
 * - Maximum Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.85 Reset Filter Indication
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ResetFilterIndication "filter.reset-indication"

extern const HAPUUID kHAPCharacteristicType_ResetFilterIndication;
/**@}*/

/**
 * Current Fan State.
 *
 * This characteristic describes the current state of the fan.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.88 Current Fan State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentFanState "fan.state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentFanState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentFanState) {
    /** Inactive. */
    kHAPCharacteristicValue_CurrentFanState_Inactive = 0,

    /** Idle. */
    kHAPCharacteristicValue_CurrentFanState_Idle = 1,

    /** Blowing Air. */
    kHAPCharacteristicValue_CurrentFanState_BlowingAir = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentFanState);
/**@}*/

/**
 * Active.
 *
 * This characteristic indicates whether the service is currently active.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.89 Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Active "active"

extern const HAPUUID kHAPCharacteristicType_Active;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_Active) {
    /** Inactive. */
    kHAPCharacteristicValue_Active_Inactive = 0,

    /** Active. */
    kHAPCharacteristicValue_Active_Active = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_Active);
/**@}*/

/**
 * Current Heater Cooler State.
 *
 * This characteristic describes the current state of a heater cooler.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.102 Current Heater Cooler State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentHeaterCoolerState "heater-cooler.state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentHeaterCoolerState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentHeaterCoolerState) {
    /** Inactive. */
    kHAPCharacteristicValue_CurrentHeaterCoolerState_Inactive = 0,

    /** Idle. */
    kHAPCharacteristicValue_CurrentHeaterCoolerState_Idle = 1,

    /** Heating. */
    kHAPCharacteristicValue_CurrentHeaterCoolerState_Heating = 2,

    /** Cooling. */
    kHAPCharacteristicValue_CurrentHeaterCoolerState_Cooling = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentHeaterCoolerState);
/**@}*/

/**
 * Target Heater Cooler State.
 *
 * This characteristic describes the target state of heater cooler.
 *
 * "Heat or Cool" state must only be included for accessories which include both a cooler and a heater.
 * "Heat or Cool" state (see `Target Heater Cooler State`) implies that the accessory will always try to maintain the
 * `Current Temperature` between `Heating Threshold Temperature` and `Cooling Threshold Temperature` and if the
 * `Current Temperature` increases above/falls below the threshold temperatures the equipment will start
 * heating or cooling respectively.
 *
 * In "Heat" state the accessory will start heating if the current temperature is below the
 * `Heating Threshold Temperature`. In "Cool" state the accessory will start cooling if the current temperature
 * is greater than the `Cooling Threshold Temperature`.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.103 Target Heater Cooler State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetHeaterCoolerState "heater-cooler.state.target"

extern const HAPUUID kHAPCharacteristicType_TargetHeaterCoolerState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetHeaterCoolerState) {
    /** Heat or Cool. */
    kHAPCharacteristicValue_TargetHeaterCoolerState_HeatOrCool = 0,

    /** Heat. */
    kHAPCharacteristicValue_TargetHeaterCoolerState_Heat = 1,

    /** Cool. */
    kHAPCharacteristicValue_TargetHeaterCoolerState_Cool = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetHeaterCoolerState);
/**@}*/

/**
 * Current Humidifier Dehumidifier State.
 *
 * This characteristic describes the current state of a humidifier or/and a dehumidifier.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.104 Current Humidifier Dehumidifier State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentHumidifierDehumidifierState "humidifier-dehumidifier.state.current"

extern const HAPUUID kHAPCharacteristicType_CurrentHumidifierDehumidifierState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CurrentHumidifierDehumidifierState) {
    /** Inactive. */
    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Inactive = 0,

    /** Idle. */
    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Idle = 1,

    /** Humidifying. */
    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Humidifying = 2,

    /** Dehumidifying. */
    kHAPCharacteristicValue_CurrentHumidifierDehumidifierState_Dehumidifying = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CurrentHumidifierDehumidifierState);
/**@}*/

/**
 * Target Humidifier Dehumidifier State.
 *
 * This characteristic describes the target state of a humidifier or/and a dehumidifier.
 *
 * "Humidifier or Dehumidifier" state must only be included for accessories which include both a humidifier and a
 * dehumidifier. "Humidifier or Dehumidifier" state implies that the accessory will always try to maintain the
 * `Current Relative Humidity` between `Relative Humidity Humidifier Threshold` and
 * `Relative Humidity Dehumidifier Threshold` and if the `Current Relative Humidity` increases above/falls below the
 * threshold relative humidity the equipment will start dehumidifying or humidifying respectively.
 *
 * In "Humidifier" state the accessory will start humidifying if the current humidity is below the
 * `Relative Humidity Humidifier Threshold`. In "Dehumidifier" mode the accessory will start dehumidifying if the
 * current humidity is greater than the `Relative Humidity Dehumidifier Threshold`.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.105 Target Humidifier Dehumidifier State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetHumidifierDehumidifierState "humidifier-dehumidifier.state.target"

extern const HAPUUID kHAPCharacteristicType_TargetHumidifierDehumidifierState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetHumidifierDehumidifierState) {
    /** Humidifier or Dehumidifier. */
    kHAPCharacteristicValue_TargetHumidifierDehumidifierState_HumidifierOrDehumidifier = 0,

    /** Humidifier. */
    kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Humidifier = 1,

    /** Dehumidifier. */
    kHAPCharacteristicValue_TargetHumidifierDehumidifierState_Dehumidifier = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetHumidifierDehumidifierState);
/**@}*/

/**
 * Water Level.
 *
 * This characteristic describes the current water level.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.106 Water Level
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WaterLevel "water-level"

extern const HAPUUID kHAPCharacteristicType_WaterLevel;
/**@}*/

/**
 * Swing Mode.
 *
 * This characteristic describes if swing mode is enabled.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.90 Swing Mode
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SwingMode "swing-mode"

extern const HAPUUID kHAPCharacteristicType_SwingMode;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SwingMode) {
    /** Swing disabled. */
    kHAPCharacteristicValue_SwingMode_Disabled = 0,

    /** Swing enabled. */
    kHAPCharacteristicValue_SwingMode_Enabled = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SwingMode);
/**@}*/

/**
 * Target Fan State.
 *
 * This characteristic describes the target state of the fan.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.87 Target Fan State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetFanState "fan.state.target"

extern const HAPUUID kHAPCharacteristicType_TargetFanState;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetFanState) {
    /** Manual. */
    kHAPCharacteristicValue_TargetFanState_Manual = 0,

    /** Auto. */
    kHAPCharacteristicValue_TargetFanState_Auto = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetFanState);
/**@}*/

/**
 * Slat Type.
 *
 * This characteristic describes the type of the slats.
 * If the slats can tilt on a horizontal axis, the value of this characteristic must be set to "Horizontal".
 * If the slats can tilt on a vertical axis, the value of this characteristic must be set to "Vertical".
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.82 Slat Type
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SlatType "type.slat"

extern const HAPUUID kHAPCharacteristicType_SlatType;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SlatType) {
    /** Horizontal. */
    kHAPCharacteristicValue_SlatType_Horizontal = 0,

    /** Vertical. */
    kHAPCharacteristicValue_SlatType_Vertical = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SlatType);
/**@}*/

/**
 * Current Tilt Angle.
 *
 * This characteristic describes the current angle of slats for accessories such as windows, fans, portable
 * heater/coolers etc. This characteristic takes values between -90 and 90. A value of 0 indicates that the slats are
 * rotated to be fully open. At value 0 the user-facing edge and the window-facing edge are perpendicular to the window.
 *
 * For "Horizontal" slat (see `Slat Type`):
 * A value of -90 indicates that the slats are rotated all the way in a direction where the user-facing edge is to the
 * left of the window-facing edge.
 *
 * For "Vertical" slat (see `Slat Type`):
 * A value of -90 indicates that the slats are rotated all the way in a direction where the user-facing edge is higher
 * than the window-facing edge.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.91 Current Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentTiltAngle "tilt.current"

extern const HAPUUID kHAPCharacteristicType_CurrentTiltAngle;
/**@}*/

/**
 * Target Tilt Angle.
 *
 * This characteristic describes the target angle of slats for accessories such as windows, fans, portable
 * heater/coolers etc.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Int
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: -90
 * - Maximum Value: 90
 * - Step Value: 1
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.92 Target Tilt Angle
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetTiltAngle "tilt.target"

extern const HAPUUID kHAPCharacteristicType_TargetTiltAngle;
/**@}*/

/**
 * Ozone Density.
 *
 * This characteristic indicates the current ozone density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.93 Ozone Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_OzoneDensity "density.ozone"

extern const HAPUUID kHAPCharacteristicType_OzoneDensity;
/**@}*/

/**
 * Nitrogen Dioxide Density.
 *
 * This characteristic indicates the current NO2 density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.94 Nitrogen Dioxide Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NitrogenDioxideDensity "density.no2"

extern const HAPUUID kHAPCharacteristicType_NitrogenDioxideDensity;
/**@}*/

/**
 * Sulphur Dioxide Density.
 *
 * This characteristic indicates the current SO2 density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.95 Sulphur Dioxide Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SulphurDioxideDensity "density.so2"

extern const HAPUUID kHAPCharacteristicType_SulphurDioxideDensity;
/**@}*/

/**
 * PM2.5 Density.
 *
 * This characteristic indicates the current PM2.5 micrometer particulate density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.96 PM2.5 Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PM2_5Density "density.pm2_5"

extern const HAPUUID kHAPCharacteristicType_PM2_5Density;
/**@}*/

/**
 * PM10 Density.
 *
 * This characteristic indicates the current PM10 micrometer particulate density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.97 PM10 Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PM10Density "density.pm10"

extern const HAPUUID kHAPCharacteristicType_PM10Density;
/**@}*/

/**
 * VOC Density.
 *
 * This characteristic indicates the current volatile organic compound density in micrograms/m^3.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1000
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.98 VOC Density
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_VOCDensity "density.voc"

extern const HAPUUID kHAPCharacteristicType_VOCDensity;
/**@}*/

/**
 * Relative Humidity Dehumidifier Threshold.
 *
 * This characteristic describes the relative humidity dehumidifier threshold. The value of this characteristic
 * represents the 'maximum relative humidity' that must be reached before dehumidifier is turned on.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.107 Relative Humidity Dehumidifier Threshold
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RelativeHumidityDehumidifierThreshold \
    "relative-humidity.dehumidifier-threshold"

extern const HAPUUID kHAPCharacteristicType_RelativeHumidityDehumidifierThreshold;
/**@}*/

/**
 * Relative Humidity Humidifier Threshold.
 *
 * This characteristic describes the relative humidity humidifier threshold. The value of this characteristic represents
 * the 'minimum relative humidity' that must be reached before humidifier is turned on.
 *
 * This characteristic requires iOS 11 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.108 Relative Humidity Humidifier Threshold
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RelativeHumidityHumidifierThreshold "relative-humidity.humidifier-threshold"

extern const HAPUUID kHAPCharacteristicType_RelativeHumidityHumidifierThreshold;
/**@}*/

/**
 * Service Label Index.
 *
 * This characteristic should be used identify the index of the label that maps to `Service Label Namespace` used by the
 * accessory.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read
 * - Minimum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.99 Service Label Index
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ServiceLabelIndex "service-label-index"

extern const HAPUUID kHAPCharacteristicType_ServiceLabelIndex;
/**@}*/

/**
 * Service Label Namespace.
 *
 * This characteristic describes the naming schema for an accessory. For example, this characteristic can be used to
 * describe the type of labels used to identify individual services of an accessory.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.100 Service Label Namespace
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ServiceLabelNamespace "service-label-namespace"

extern const HAPUUID kHAPCharacteristicType_ServiceLabelNamespace;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ServiceLabelNamespace) {
    /** Dots. For example, "." ".." "..." "....". */
    kHAPCharacteristicValue_ServiceLabelNamespace_Dots = 0,

    /** Arabic numerals. For e.g., 0,1,2,3. */
    kHAPCharacteristicValue_ServiceLabelNamespace_ArabicNumerals = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ServiceLabelNamespace);
/**@}*/

/**
 * Color Temperature.
 *
 * This characteristic describes color temperature which is represented in reciprocal megaKelvin (MK^-1) or mirek scale.
 * (M = 1,000,000 / K where M is the desired mirek value and K is temperature in Kelvin).
 *
 * If this characteristic is included in the `Light Bulb`, `Hue` and `Saturation` must not be included as optional
 * characteristics in `Light Bulb`. This characteristic must not be used for lamps which support color.
 *
 * This characteristic requires iOS 10.3 or later.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 50
 * - Maximum Value: 400
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.101 Color Temperature
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ColorTemperature "color-temperature"

extern const HAPUUID kHAPCharacteristicType_ColorTemperature;
/**@}*/

/**
 * Program Mode.
 *
 * This characteristic describes if there are programs scheduled on the accessory. If there are Programs scheduled on
 * the accessory and the accessory is used for manual operation, the value of this characteristic must be
 * "Program Scheduled, currently overridden to manual mode".
 *
 * This characteristic requires iOS 11.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.109 Program Mode
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ProgramMode "program-mode"

extern const HAPUUID kHAPCharacteristicType_ProgramMode;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ProgramMode) {
    /** No Programs Scheduled. */
    kHAPCharacteristicValue_ProgramMode_NotScheduled = 0,

    /** Program Scheduled. */
    kHAPCharacteristicValue_ProgramMode_Scheduled = 1,

    /** Program Scheduled, currently overridden to manual mode. */
    kHAPCharacteristicValue_ProgramMode_ScheduleOverriddenToManual = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ProgramMode);
/**@}*/

/**
 * In Use.
 *
 * This characteristic describes if the service is in use. The service must be "Active" before the value of this
 * characteristic can be set to in use.
 *
 * This characteristic requires iOS 11.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.110 In Use
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_InUse "in-use"

extern const HAPUUID kHAPCharacteristicType_InUse;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_InUse) {
    /** Not in use. */
    kHAPCharacteristicValue_InUse_NotInUse = 0,

    /** In use. */
    kHAPCharacteristicValue_InUse_InUse = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_InUse);
/**@}*/

/**
 * Set Duration.
 *
 * This characteristic describes the set duration. For a `Valve` this duration defines how long a valve should be set to
 * "In Use". Once the valve is "In Use", any changes to this characteristic take affect in the next operation when the
 * `Valve` is "Active".
 *
 * This duration is defined in seconds.
 *
 * This characteristic requires iOS 11.2 or later.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3600
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.111 Set Duration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SetDuration "set-duration"

extern const HAPUUID kHAPCharacteristicType_SetDuration;
/**@}*/

/**
 * Remaining Duration.
 *
 * This characteristic describes the remaining duration on the accessory. Notifications on this characteristic must only
 * be used if the remaining duration increases/decreases from the accessory's usual countdown of remaining duration and
 * when the duration reaches 0. e.g., It must not send notifications when the remaining duration is ticking down from
 * 100,99,98... if 100 was the initial Set duration. However, if the remaining duration changes to 95 from 92 (increase)
 * or 85 from 92 (decrease which is not part of the usual duration countdown), it must send a notification.
 *
 * This duration is defined in seconds.
 *
 * This characteristic requires iOS 11.2 or later.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3600
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.112 Remaining Duration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RemainingDuration "remaining-duration"

extern const HAPUUID kHAPCharacteristicType_RemainingDuration;
/**@}*/

/**
 * Valve Type.
 *
 * This characteristic describes the type of valve.
 *
 * This characteristic requires iOS 11.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 3
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.113 Valve Type
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ValveType "valve-type"

extern const HAPUUID kHAPCharacteristicType_ValveType;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ValveType) {
    /** Generic valve. */
    kHAPCharacteristicValue_ValveType_GenericValve = 0,

    /** Irrigation. */
    kHAPCharacteristicValue_ValveType_Irrigation = 1,

    /** Shower head. */
    kHAPCharacteristicValue_ValveType_ShowerHead = 2,

    /** Water faucet. */
    kHAPCharacteristicValue_ValveType_WaterFaucet = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ValveType);
/**@}*/

/**
 * Is Configured.
 *
 * This characteristic describes if the service is configured for use. For example, all of the valves in an irrigation
 * system may not be configured depending on physical wire connection.
 *
 * If the accessory supports updating through HAP, then it must also advertise Paired Write in the permissions.
 *
 * This characteristic requires iOS 12.x.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.114 Is Configured
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_IsConfigured "is-configured"

extern const HAPUUID kHAPCharacteristicType_IsConfigured;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_IsConfigured) {
    /** Not Configured. */
    kHAPCharacteristicValue_IsConfigured_NotConfigured = 0,

    /** Configured. */
    kHAPCharacteristicValue_IsConfigured_Configured = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_IsConfigured);
/**@}*/

/**
 * Configured Name.
 *
 * This characteristic describes an UTF-8 encoded user visible name on the accessory. If this can be updated on the
 * accessory and from HomeKit controller, the characteristic must support Paired Write permission as well.
 *
 * This characteristic requires iOS 12.2 or later.
 *
 * - Format: String
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.123 Configured Name
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ConfiguredName "configured-name"

extern const HAPUUID kHAPCharacteristicType_ConfiguredName;
/**@}*/

/**
 * Active Identifier.
 *
 * The Active Identifier characteristic may describe the current target of an accessory.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification for Televisions R17
 *      Section 11.118 Active Identifier
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ActiveIdentifier "active-identifier"

extern const HAPUUID kHAPCharacteristicType_ActiveIdentifier;
/**@}*/

/**
 * Number of bytes of Ed25519 public key
 */
#define ED25519_PUBLIC_KEY_BYTES 32

/**
 * Number of bytes of NIST256 private key
 */
#define NIST256_PUBLIC_KEY_BYTES 64

/**
 * Number of bytes of NIST256 private key
 */
#define NIST256_PRIVATE_KEY_BYTES 32

/**
 * Number of bytes of an NFC Access Key Identifier
 */
#define NFC_ACCESS_KEY_IDENTIFIER_BYTES 8

/**
 * Number of bytes of the NFC Access Issuer Public key
 */
#define NFC_ACCESS_ISSUER_KEY_BYTES ED25519_PUBLIC_KEY_BYTES

/**
 * Number of bytes of the NFC Access Device Credential Public key
 */
#define NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES NIST256_PUBLIC_KEY_BYTES

/**
 * Number of bytes of the NFC Access Reader Private key
 */
#define NFC_ACCESS_READER_KEY_BYTES NIST256_PRIVATE_KEY_BYTES

/**
 * NFC Access Supported Configuration.
 *
 * - Format: TLV8
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.180 NFC Access Supported Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NfcAccessSupportedConfiguration "nfc-access-supported-configuration"

extern const HAPUUID kHAPCharacteristicType_NfcAccessSupportedConfiguration;

/**
 * NFC Access Supported Configuration
 */
typedef struct {
    /** Maximum number of issuer keys supported */
    uint16_t maximumIssuerKeys;

    /** Maximum number of suspended device credential keys supported */
    uint16_t maximumSuspendedDeviceCredentialKeys;

    /** Maximum number of active device credential keys supported */
    uint16_t maximumActiveDeviceCredentialKeys;
} HAPCharacteristicValue_NfcAccessSupportedConfiguration;

/**@}*/

/**
 * NFC Access Control Point.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.181 NFC Access Control Point
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NfcAccessControlPoint "nfc-access-control-point"

extern const HAPUUID kHAPCharacteristicType_NfcAccessControlPoint;

/**
 * Operation type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NfcAccessControlPoint_OperationType) {
    /**
     * List.
     * Lists all NFC access control keys specified by the request.
     */
    kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List = 1,
    /**
     * Add.
     * Adds a new NFC access control key specified by the request.
     */
    kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add = 2,
    /**
     * Remove.
     * Removes an existing NFC access control key specified by the request.
     */
    kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NfcAccessControlPoint_OperationType);

/**
 * NFC Access Control Point Key Type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NfcAccessControlPoint_KeyType) {
    /** Ed25519 */
    kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519 = 1,
    /** NIST256 */
    kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256 = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NfcAccessControlPoint_KeyType);

/**
 * NFC Access Response Status Codes
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NfcAccessResponseStatusCode) {
    /** Success */
    kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success = 0,
    /** Error. Out of resources. */
    kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorOutOfResources = 1,
    /** Error. Duplicate. */
    kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate = 2,
    /** Error. Does not exist. */
    kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist = 3,
    /** Error. Not supported. */
    kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NfcAccessResponseStatusCode);

/**
 * NFC Access Device Credential Key State
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NfcAccessDeviceCredentialKey_State) {
    /** Device Credential Key is suspended */
    kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended = 0,
    /** Device Credential Key is active */
    kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NfcAccessDeviceCredentialKey_State);

/**
 * NFC Access Control Point Data
 */
typedef struct {
    void* bytes;     /**< Value buffer */
    size_t numBytes; /**< Length of value buffer */
} HAPCharacteristicValue_NfcAccessControlPoint_Data;

/**
 * NFC Access Issuer Key Request
 */
typedef struct {
    /** Optional. Enum defining type of the key. */
    HAPCharacteristicValue_NfcAccessControlPoint_KeyType type;
    bool typeIsSet;

    /** Optional. Issuer key used to authenticate the Unified Access Document presented to the accessory over NFC. This
     * is the public key of the asymmetric key pair. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data key;
    bool keyIsSet;

    /** Optional. 8 byte identifier that uniquely identifies the issuer key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;
} HAPCharacteristicValue_NfcAccessIssuerKeyRequest;

/**
 * NFC Access Issuer Key Response
 */
typedef struct {
    /** Optional. 8 byte Identifier that uniquely identifies the issuer key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;

    /** Optional. The status code of the response. */
    HAPCharacteristicValue_NfcAccessResponseStatusCode statusCode;
    bool statusCodeIsSet;
} HAPCharacteristicValue_NfcAccessIssuerKeyResponse;

/**
 * NFC Access Control Point containing Issuer Key Response
 */
typedef struct {
    /** Issuer Key Response */
    HAPCharacteristicValue_NfcAccessIssuerKeyResponse data;
} HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse;

/**
 * NFC Access Device Credential Key Request
 */
typedef struct {
    /** Optional. Enum defining type of the key. */
    HAPCharacteristicValue_NfcAccessControlPoint_KeyType type;
    bool typeIsSet;

    /** Optional. Device Credential Key as defined in section 3.2.3 of the Unified Access Document. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data key;
    bool keyIsSet;

    /** Optional. 8 byte identifier of an issuer key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data issuerKeyIdentifier;
    bool issuerKeyIdentifierIsSet;

    /** Optional. State indicating whether device credential key is suspended or active. */
    HAPCharacteristicValue_NfcAccessDeviceCredentialKey_State state;
    bool stateIsSet;

    /** Optional. 8 byte identifier that uniquely identifies the device credential key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;
} HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest;

/**
 * NFC Access Device Credential Key Response
 */
typedef struct {
    /** Optional. 8 byte Identifier that uniquely identifies the reader key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;

    /** Optional. 8 byte identifier of an issuer key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data issuerKeyIdentifier;
    bool issuerKeyIdentifierIsSet;

    /** Optional. The status code of the response. */
    HAPCharacteristicValue_NfcAccessResponseStatusCode statusCode;
    bool statusCodeIsSet;
} HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse;

/**
 * NFC Access Control Point containing Device Credential Key Response
 */
typedef struct {
    /** Device Credential Key Response */
    HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse data;
} HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse;

/**
 * NFC Access Reader Key Request
 */
typedef struct {
    /** Optional. Enum defining type of the key. */
    HAPCharacteristicValue_NfcAccessControlPoint_KeyType type;
    bool typeIsSet;

    /** Optional. Reader Key as defined in the Unified Access Document. This is the private key of the asymmetric key
     * pair. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data key;
    bool keyIsSet;

    /** Optional. 8 byte identifier that uniquely identifies a reader. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data readerIdentifier;
    bool readerIdentifierIsSet;

    /** Optional. 8 byte identifier that uniquely identifies the reader key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;
} HAPCharacteristicValue_NfcAccessReaderKeyRequest;

/**
 * NFC Access Reader Key Response
 */
typedef struct {
    /** Optional. 8 byte Identifier that uniquely identifies the reader key. */
    HAPCharacteristicValue_NfcAccessControlPoint_Data identifier;
    bool identifierIsSet;

    /** Optional. The status code of the response. */
    HAPCharacteristicValue_NfcAccessResponseStatusCode statusCode;
    bool statusCodeIsSet;
} HAPCharacteristicValue_NfcAccessReaderKeyResponse;

/**
 * NFC Access Control Point containing Reader Key Response
 */
typedef struct {
    /** Reader Key Response */
    HAPCharacteristicValue_NfcAccessReaderKeyResponse data;
} HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse;

/**@}*/

/**
 * Configuration State.
 *
 * - Format: UInt16
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.177 Configuration State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ConfigurationState "configuration-state"

extern const HAPUUID kHAPCharacteristicType_ConfigurationState;

/**@}*/

/**
 * Supported Video Stream Configuration.
 *
 * This characteristic allows an IP Camera accessory to describe the parameters supported for streaming video
 * over an RTP session. Status characteristic allows an IP Camera accessory to describe the status of
 * the RTP Stream Management service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.66 Supported Video Stream Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedVideoStreamConfiguration "supported-video-stream-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedVideoStreamConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedVideoStreamConfiguration) {
    /**
     * Codec information and the configurations supported for the codec.
     *
     * There is one TLV of this type per supported codec.
     */
    kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedVideoStreamConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration) {
    /** Type of video codec. */
    kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_CodecType = 1,

    /** Video codec-specific parameters. */
    kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_CodecParameters = 2,

    /** Video Attributes supported for the codec. */
    kHAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration_Attributes = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedVideoStreamConfiguration_VideoConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CameraStream_VideoCodecType) {
    /** H.264. */
    kHAPCharacteristicValue_CameraStream_VideoCodecType_H264 = 0,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CameraStream_VideoCodecType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CameraStream_H264VideoCodecParameter) {
    /** Type of H.264 Profile. One instance of this TLV must be present for each supported profile. */
    kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Profile = 1,

    /** Profile support level. */
    kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_Level = 2,

    /** Packetization Mode. One instance of this TLV must be present for each supported mode. */
    kHAPCharacteristicValue_CameraStream_H264VideoCodecParameter_PacketizationMode = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CameraStream_H264VideoCodecParameter);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_H264VideoCodecProfile) {
    /** Constrained Baseline Profile. */
    kHAPCharacteristicValue_H264VideoCodecProfile_ConstrainedBaseline = 0,

    /** Main Profile. Note:Interlaced coding (PicAFF, MBAFF) must not be used. */
    kHAPCharacteristicValue_H264VideoCodecProfile_Main = 1,

    /** High Profile. Note:Interlaced coding (PicAFF, MBAFF) must not be used. */
    kHAPCharacteristicValue_H264VideoCodecProfile_High = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_H264VideoCodecProfile);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_H264VideoCodecProfileLevel) {
    /** 3.1. */
    kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_1 = 0,

    /** 3.2. */
    kHAPCharacteristicValue_H264VideoCodecProfileLevel_3_2 = 1,

    /** 4. */
    kHAPCharacteristicValue_H264VideoCodecProfileLevel_4 = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_H264VideoCodecProfileLevel);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_H264VideoCodecPacketizationMode) {
    /** Non-interleaved mode. */
    kHAPCharacteristicValue_H264VideoCodecPacketizationMode_NonInterleaved = 0
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_H264VideoCodecPacketizationMode);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_VideoCodecAttribute) {
    /** Image width in pixels. */
    kHAPCharacteristicValue_VideoCodecAttribute_ImageWidth = 1,

    /** Image height in pixels. */
    kHAPCharacteristicValue_VideoCodecAttribute_ImageHeight = 2,

    /** Maximum frame rate. */
    kHAPCharacteristicValue_VideoCodecAttribute_FrameRate = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_VideoCodecAttribute);

/**@}*/

/**
 * Supported Audio Stream Configuration.
 *
 * This characteristic allows an accessory to indicate the parameters supported for streaming audio
 * (from a microphone and/or to a speaker) over an RTP session.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.67 Supported Audio Stream Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedAudioStreamConfiguration "supported-audio-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedAudioStreamConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedAudioStreamConfiguration) {
    /** Codec information and the configurations supported for the codec. */
    kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration = 1,

    /** Boolean, indicating support for Comfort Noise Codec. */
    kHAPCharacteristicValue_SupportedAudioStreamConfiguration_ComfortNoiseSupport = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedAudioStreamConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration) {
    /** Type of codec. */
    kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecType = 1,

    /** Codec-specific parameters. */
    kHAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration_CodecParameters = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedAudioStreamConfiguration_AudioConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecType) {
    /** AAC-ELD. */
    kHAPCharacteristicValue_AudioCodecType_AAC_ELD = 2,

    /** Opus. */
    kHAPCharacteristicValue_AudioCodecType_Opus = 3,

    /** AMR. */
    kHAPCharacteristicValue_AudioCodecType_AMR = 5,

    /** AMR-WB. */
    kHAPCharacteristicValue_AudioCodecType_AMR_WB = 6,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecRecordingType) {
    /** AAC-LC. */
    kHAPCharacteristicValue_AudioCodecRecordingType_AAC_LC = 0,

    /** AAC-ELD. */
    kHAPCharacteristicValue_AudioCodecRecordingType_AAC_ELD = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecRecordingType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecStreamParameter) {
    /** Number of audio channels. Default is 1. */
    kHAPCharacteristicValue_AudioCodecStreamParameter_AudioChannels = 1,

    /** Bit-rate. */
    kHAPCharacteristicValue_AudioCodecStreamParameter_BitRate = 2,

    /** Sample rate. */
    kHAPCharacteristicValue_AudioCodecStreamParameter_SampleRate = 3,

    /**
     * Packet Time - Length of time represented by the media in a packet RFC 4566.
     *
     * Supported values - 20ms, 30ms, 40 ms & 60ms.
     *
     * Note: This TLV will only be presented in the Selected Audio Codec Parameters TLV.
     */
    kHAPCharacteristicValue_AudioCodecStreamParameter_RTPTime = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecStreamParameter);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecBitRateControlMode) {
    /** Variable bit-rate. */
    kHAPCharacteristicValue_AudioCodecBitRateControlMode_Variable = 0,

    /** Constant bit-rate. */
    kHAPCharacteristicValue_AudioCodecBitRateControlMode_Constant = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecBitRateControlMode);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecSampleRate) {
    /** 8 KHz. */
    kHAPCharacteristicValue_AudioCodecSampleRate_8KHz = 0,

    /** 16 KHz. */
    kHAPCharacteristicValue_AudioCodecSampleRate_16KHz = 1,

    /** 24 KHz. */
    kHAPCharacteristicValue_AudioCodecSampleRate_24KHz = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecSampleRate);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecRecordingSampleRate) {
    /** 8 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_8KHz = 0,

    /** 16 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_16KHz = 1,

    /** 24 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_24KHz = 2,

    /** 32 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_32KHz = 3,

    /** 44.1 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_44_1KHz = 4,

    /** 48 KHz. */
    kHAPCharacteristicValue_AudioCodecRecordingSampleRate_48KHz = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecRecordingSampleRate);

/**@}*/

/**
 * Supported RTP Configuration.
 *
 * This characteristic allows an accessory to describe the supported configuration parameters
 * for the RTP video service used for streaming and other operations.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.68 Supported RTP Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedRTPConfiguration "supported-rtp-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedRTPConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedRTPConfiguration) {
    /**
     * Supported SRTP Crypto Suite.
     *
     * If multiple crypto suites are supported, multiple instances of this TLV should be present.
     * Use delimiter "0x00" to separate TLV items.
     */
    kHAPCharacteristicValue_SupportedRTPConfiguration_SRTPCryptoSuite = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedRTPConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SRTPCryptoSuite) {
    /** AES_CM_128_HMAC_SHA1_80. */
    kHAPCharacteristicValue_SRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 = 0,

    /** AES_256_CM_HMAC_SHA1_80. */
    kHAPCharacteristicValue_SRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80 = 1,

    /** Disabled. */
    kHAPCharacteristicValue_SRTPCryptoSuite_Disabled = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SRTPCryptoSuite);

/**@}*/

/**
 * Selected RTP Stream Configuration.
 *
 * This characteristic is a control point characteristic that allows a controller to specify the selected
 * video and audio attributes to be used for streaming audio and video from an IP camera accessory.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.70 Selected RTP Stream Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SelectedRTPStreamConfiguration "selected-rtp-stream-configuration"

extern const HAPUUID kHAPCharacteristicType_SelectedRTPStreamConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration) {
    /** Session Control command and identifier. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl = 1,

    /** Video parameters selected for the streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video = 2,

    /** Input Audio parameters selected for the streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio = 3,

} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl) {
    /** UUID identifying the session that identifies the command. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_SessionIdentifier = 1,

    /** Session control command. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command) {
    /** End streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_End = 0,

    /** Start streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Start = 1,

    /** Suspend streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Suspend = 2,

    /** Resume streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Resume = 3,

    /** Reconfigure streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command_Reconfigure = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_SessionControl_Command);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_Video) {
    /** Type of video codec. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecType = 1,

    /** Video codec-specific parameters for the streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_CodecParameters = 2,

    /** Video attributes selected for the streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_Attributes = 3,

    /** RTP parameters selected for the video streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Video_RTPParameters = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_Video);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio) {
    /** Type of codec. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_CodecType = 1,

    /** Audio codec specific parameters. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_CodecParameters = 2,

    /** RTP parameters selected for the streaming session. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_RTPParameters = 3,

    /**
     * Boolean. A value of 1 indicates that Comfort Noise has been selected and that both the camera and iOS device
     * will both use the Comfort Noise codec.
     */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio_ComfortNoise = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_Audio);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters) {
    /** Video: Type of video codec / Audio: Payload type as defined in RFC 3551. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_PayloadType = 1,

    /** SSRC for video / audio stream. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_SynchronizationSource = 2,

    /** Maximum bit rate generated by the codec in kbps and averaged over 1 second. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxBitrate = 3,

    /** Minimum RTCP interval in seconds formatted as a 4 byte little endian IEEE 754 floating point value. */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MinRTCPInterval = 4,

    /**
     * MTU that the IP camera must use to transmit Video RTP packets. This value will be populated only if the
     * controller intends the camera to use a non-default value of the MTU.
     *
     * - Only present for video.
     */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_MaxMTU = 5,

    /**
     * Only required when Comfort Noise is chosen in the Selected Audio Parameters TLV.
     *
     * - Only present for audio.
     */
    kHAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters_ComfortNoisePayloadType = 6,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedRTPStreamConfiguration_RTPParameters);
/**@}*/

/**
 * Setup Endpoints.
 *
 * The Setup Endpoints characteristic allows a controller to exchange IP address and port information with the IP
 * camera.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.69 Setup Endpoints
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SetupEndpoints "setup-endpoints"

extern const HAPUUID kHAPCharacteristicType_SetupEndpoints;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupEndpoints) {
    /** UUID identifying the session that the command applies to. */
    kHAPCharacteristicValue_SetupEndpoints_SessionID = 1,

    /**
     * Status.
     *
     * - Only present in response.
     */
    kHAPCharacteristicValue_SetupEndpoints_Status = 2,

    /** Address of the controller / IP camera for the streaming session. */
    kHAPCharacteristicValue_SetupEndpoints_Address = 3,

    /** RTP parameters selected for the video streaming session. */
    kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForVideo = 4,

    /** RTP parameters selected for the audio streaming session. */
    kHAPCharacteristicValue_SetupEndpoints_SRTPParametersForAudio = 5,

    /**
     * SSRC for video RTP stream.
     *
     * - Only present in response.
     */
    kHAPCharacteristicValue_SetupEndpoints_SynchronizationSourceForVideo = 6,

    /**
     * SSRC for audio RTP stream.
     *
     * - Only present in response.
     */
    kHAPCharacteristicValue_SetupEndpoints_SynchronizationSourceForAudio = 7,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupEndpoints);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupEndpoints_Status) {
    /** Success. */
    kHAPCharacteristicValue_SetupEndpoints_Status_Success = 0,

    /** Busy. */
    kHAPCharacteristicValue_SetupEndpoints_Status_Busy = 1,

    /** Error. */
    kHAPCharacteristicValue_SetupEndpoints_Status_Error = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupEndpoints_Status);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupEndpoints_Address) {
    /** Version of IP Address. */
    kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion = 1,

    /** IP address of the controller / accessory. */
    kHAPCharacteristicValue_SetupEndpoints_Address_IPAddress = 2,

    /** Receive port of the controller / accessory for the video stream of the RTP session. */
    kHAPCharacteristicValue_SetupEndpoints_Address_VideoRTPPort = 3,

    /** Receive port of the controller / accessory for the audio stream of the RTP session. */
    kHAPCharacteristicValue_SetupEndpoints_Address_AudioRTPPort = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupEndpoints_Address);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion) {
    /** IPv4. */
    kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv4 = 0,

    /** IPv6. */
    kHAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion_IPv6 = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupEndpoints_Address_IPAddressVersion);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupEndpoints_SRTPParameters) {
    /** Supported SRTP Crypto Suite. */
    kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_CryptoSuite = 1,

    /** Master key for the SRTP session. */
    kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterKey = 2,

    /** Master salt for the SRTP session. */
    kHAPCharacteristicValue_SetupEndpoints_SRTPParameters_MasterSalt = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupEndpoints_SRTPParameters);
/**@}*/

/**
 * Volume.
 *
 * A Volume characteristic allows the control of input or output volume of an audio input or output device respectively.
 * The value of this characteristic indicates the percentage of the maximum volume supported by the service.
 * If the Speaker service can support controlling the volume, this characteristic must support Paired Write
 * permission as well.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 100
 * - Step Value: 1
 * - Unit: Percentage
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.71 Volume
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Volume "volume"

extern const HAPUUID kHAPCharacteristicType_Volume;
/**@}*/

/**
 * Mute.
 *
 * A Mute characteristic allows the control of audio input or output accessory respectively.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.72 Mute
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_Mute "mute"

extern const HAPUUID kHAPCharacteristicType_Mute;
/**@}*/

/**
 * Night Vision.
 *
 * This characteristic allows the control of night vision mode on a video RTP service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.73 Night Vision
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NightVision "night-vision"

extern const HAPUUID kHAPCharacteristicType_NightVision;
/**@}*/

/**
 * Optical Zoom.
 *
 * The Optical Zoom characteristic allows the control of zoom of a video RTP service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.74 Optical Zoom
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_OpticalZoom "zoom-optical"

extern const HAPUUID kHAPCharacteristicType_OpticalZoom;
/**@}*/

/**
 * Digital Zoom.
 *
 * The Digital Zoom characteristic allows the control of digital zoom of a video RTP service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.75 Digital Zoom
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_DigitalZoom "zoom-digital"

extern const HAPUUID kHAPCharacteristicType_DigitalZoom;
/**@}*/

/**
 * Image Rotation.
 *
 * An Image Rotation characteristic allows the control of rotation of the image of a video RTP service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 270
 * - Step Value: 90
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.76 Image Rotation
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ImageRotation "image-rotation"

extern const HAPUUID kHAPCharacteristicType_ImageRotation;
/**@}*/

/**
 * Image Mirroring.
 *
 * An Image Mirroring characteristic allows the control of mirroring state of the image of a video RTP service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.77 Image Mirroring
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ImageMirroring "image-mirror"

extern const HAPUUID kHAPCharacteristicType_ImageMirroring;
/**@}*/

/**
 * Streaming Status.
 *
 * A Streaming Status characteristic allows an IP Camera accessory to describe the status of the RTP Stream Management
 * service.
 *
 * This characteristic requires iOS 10 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.65 Streaming Status
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_StreamingStatus "streaming-status"

extern const HAPUUID kHAPCharacteristicType_StreamingStatus;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StreamingStatus) {
    /** Status of the stream RTP management service. */
    kHAPCharacteristicValue_StreamingStatus_Status = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StreamingStatus);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_StreamingStatus_Status) {
    /** Available. */
    kHAPCharacteristicValue_StreamingStatus_Status_Available = 0,

    /** In Use. */
    kHAPCharacteristicValue_StreamingStatus_Status_InUse = 1,

    /** Unavailable. */
    kHAPCharacteristicValue_StreamingStatus_Status_Unavailable = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_StreamingStatus_Status);
/**@}*/

/**
 * Target Control Supported Configuration.
 *
 * This characteristic allows the accessory to indicate the configuration it supports and is encoded as a list of TLV8
 * tuples.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.115 Target Control Supported Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetControlSupportedConfiguration "supported-target-configuration"

extern const HAPUUID kHAPCharacteristicType_TargetControlSupportedConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlSupportedConfiguration) {
    /**
     * Maximum number of targets that can be supported by this accessory.
     * If a tuple of this TLV type is omitted, it is assumed that the accessory supports 16 targets.
     */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_MaximumTargets = 1,

    /** Resolution of the timestamp sent in the button value characteristic. The value is encoded as a uint64 value. */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_TicksPerSecond = 2,

    /** Configuration of the supported buttons - The value is a TLV8-encoded list. */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_SupportedButtonConfiguration = 3,

    /**
     * Must be set to 1 if the accessory is implemented as a hardware entity.
     * Siri is only allowed for a hardware entity.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 13.3 Additional Security Requirements
     */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_Type = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlSupportedConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControl_Type) {
    /** Software entity. */
    kHAPCharacteristicValue_TargetControl_Type_Software = 0,

    /** Hardware entity. */
    kHAPCharacteristicValue_TargetControl_Type_Hardware = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControl_Type);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlSupportedConfiguration_ButtonConfiguration) {
    /**
     * ID of a button; a value of 0 is invalid.
     * When a TLV of this type is encountered, configuration of a new button is being described.
     */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_ButtonConfiguration_ButtonID = 1,

    /** Type of button. */
    kHAPCharacteristicValue_TargetControlSupportedConfiguration_ButtonConfiguration_ButtonType = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlSupportedConfiguration_ButtonConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControl_ButtonType) {
    /** Menu. */
    kHAPCharacteristicValue_TargetControl_ButtonType_Menu = 1,

    /** Play/Pause. */
    kHAPCharacteristicValue_TargetControl_ButtonType_PlayPause = 2,

    /** TV/Home. */
    kHAPCharacteristicValue_TargetControl_ButtonType_TVHome = 3,

    /** Select. */
    kHAPCharacteristicValue_TargetControl_ButtonType_Select = 4,

    /** Arrow Up. */
    kHAPCharacteristicValue_TargetControl_ButtonType_ArrowUp = 5,

    /** Arrow Right. */
    kHAPCharacteristicValue_TargetControl_ButtonType_ArrowRight = 6,

    /** Arrow Down. */
    kHAPCharacteristicValue_TargetControl_ButtonType_ArrowDown = 7,

    /** Arrow Left. */
    kHAPCharacteristicValue_TargetControl_ButtonType_ArrowLeft = 8,

    /** Volume Up. */
    kHAPCharacteristicValue_TargetControl_ButtonType_VolumeUp = 9,

    /** Volume Down. */
    kHAPCharacteristicValue_TargetControl_ButtonType_VolumeDown = 10,

    /** Siri. */
    kHAPCharacteristicValue_TargetControl_ButtonType_Siri = 11,

    /** Power. */
    kHAPCharacteristicValue_TargetControl_ButtonType_Power = 12,

    /** Generic. */
    kHAPCharacteristicValue_TargetControl_ButtonType_Generic = 13,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControl_ButtonType);

const char* HAPCharacteristicDebugDescription_TargetControl_ButtonType(
        HAPCharacteristicValue_TargetControl_ButtonType value);
/**@}*/

/**
 * Target Control List.
 *
 * This HAP characteristic allows a controller to manage the association of targets with the accessory. This
 * characteristic must support write response and is implemented as a control point characteristic with the write value
 * indicating the type of operation and the result expected in the write response. The write response must include all
 * the target configurations present on the accessory after the write operation. The accessory must store the
 * configuration from the Target Control List in its persistent store and this configuration must not be erased on
 * reboot of the accessory. The configuration must be erased on factory reset of the accessory and when the last admin
 * pairing is removed from the accessory.
 *
 * Only admin controllers are allowed to perform any operations on this characteristic. Any read/writes to this
 * characteristic by non-admin controllers must result in error -70401 (Insufficient Privileges).
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.116 Target Control List
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TargetControlList "target-list"

extern const HAPUUID kHAPCharacteristicType_TargetControlList;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlList) {
    /** Operation for the control point. */
    kHAPCharacteristicValue_TargetControlList_Operation = 1,

    /**
     * Configuration for a specific target. Value is tlv8 encoded.
     * Not needed in the write value for "List" and "Reset" operations.
     */
    kHAPCharacteristicValue_TargetControlList_Configuration = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlList);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlList_Operation) {
    /** List; response contains the current configuration. */
    kHAPCharacteristicValue_TargetControlList_Operation_List = 1,

    /** Add; response contains the updated configuration. */
    kHAPCharacteristicValue_TargetControlList_Operation_Add = 2,

    /** Remove; response contains the updated configuration. */
    kHAPCharacteristicValue_TargetControlList_Operation_Remove = 3,

    /** Reset; response contains no value. */
    kHAPCharacteristicValue_TargetControlList_Operation_Reset = 4,

    /** Update; response contains the updated configuration. */
    kHAPCharacteristicValue_TargetControlList_Operation_Update = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlList_Operation);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlList_Configuration) {
    /**
     * Identifier of the target; value is a uint32. 0 indicates an invalid Target or that no Target is currently
     * selected (e.g., the target controller is controlling a non-HomeKit entity).
     * When a TLV of this type is encountered, configuration of a new target is being described.
     */
    kHAPCharacteristicValue_TargetControlList_Configuration_TargetIdentifier = 1,

    /** Name of the target; value is a UTF8 string. */
    kHAPCharacteristicValue_TargetControlList_Configuration_TargetName = 2,

    /** Category of the target; value is a uint16. */
    kHAPCharacteristicValue_TargetControlList_Configuration_TargetCategory = 3,

    /** Configuration of an enabled button; value is encoded as a TLV8. One instance present per button enabled. */
    kHAPCharacteristicValue_TargetControlList_Configuration_ButtonConfiguration = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlList_Configuration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TargetControlList_ButtonConfiguration) {
    /**
     * ID of button; a value of 0 is invalid.
     * When a TLV of this type is encountered, configuration of a new button is being described.
     */
    kHAPCharacteristicValue_TargetControlList_ButtonConfiguration_ButtonID = 1,

    /** Type of button. May be optionally present. */
    kHAPCharacteristicValue_TargetControlList_ButtonConfiguration_ButtonType = 2,

    /**
     * Name of the Button - value is a UTF8 string.
     * May be optionally present. If present, accessory must use this name
     * instead of the name derived from the button type.
     */
    kkHAPCharacteristicValue_TargetControlList_ButtonConfiguration_ButtonName = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TargetControlList_ButtonConfiguration);
/**@}*/

/**
 * Button Event.
 *
 * Notifications on this characteristic can only be enabled by Admin controllers. Any requests to enable notification on
 * this characteristic by non admin controllers must result in error -70401 (Insufficient Privileges).
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.117 Button Event
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ButtonEvent "button-event"

extern const HAPUUID kHAPCharacteristicType_ButtonEvent;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ButtonEvent) {
    /** ID of button; a value of 0 is invalid. */
    kHAPCharacteristicValue_ButtonEvent_ButtonID = 1,

    /** State of the button. */
    kHAPCharacteristicValue_ButtonEvent_ButtonState = 2,

    /** Timestamp of the event. Units are ticks. */
    kHAPCharacteristicValue_ButtonEvent_Timestamp = 3,

    /** Identifier of the target; value is a uint32. */
    kHAPCharacteristicValue_ButtonEvent_ActiveIdentifier = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ButtonEvent);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ButtonEvent_ButtonState) {
    /** Up. */
    kHAPCharacteristicValue_ButtonEvent_ButtonState_Up = 0,

    /** Down. */
    kHAPCharacteristicValue_ButtonEvent_ButtonState_Down = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ButtonEvent_ButtonState);
/**@}*/

/**
 * Selected Audio Stream Configuration.
 *
 * This is a control point characteristic that allows a controller to specify the selected audio attributes to be used
 * for streaming audio from the accessory.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.119 Selected Audio Stream Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SelectedAudioStreamConfiguration "selected-audio-stream-configuration"

extern const HAPUUID kHAPCharacteristicType_SelectedAudioStreamConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicType_SelectedAudioStreamConfiguration) {
    /** The codec that is to be used for input-audio (ie, audio sent from accessory to controller). */
    kHAPCharacteristicValue_SelectedAudioStreamConfiguration_SelectedAudioInputStreamConfiguration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicType_SelectedAudioStreamConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicType_SelectedAudioInputStreamConfiguration_Configuration) {
    /** Same as Selected Audio Parameter TLV 1, where Opus = 3. */
    kHAPCharacteristicValue_SelectedAudioStreamConfiguration_Configuration_SelectedAudioCodecType = 1,

    /**
     * Same as Selected Audio Parameter TLV 2, which is defined to be the same as the Codec Param in the
     * Supported Audio Stream Configuration.
     */
    kHAPCharacteristicValue_SelectedAudioStreamConfiguration_Configuration_SelectedAudioCodecParameters = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicType_SelectedAudioInputStreamConfiguration_Configuration);
/**@}*/

/**
 * Supported Data Stream Transport Configuration.
 *
 * This characteristic describes the data stream transport supported by the accessory.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.120 Supported Data Stream Transport Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedDataStreamTransportConfiguration \
    "supported-data-stream-transport-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedDataStreamTransportConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedDataStreamTransportConfiguration) {
    /** The configuration supported for the transport. There is one TLV of this type per supported transport type. */
    kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedDataStreamTransportConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration) {
    /** Type of transport. */
    kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration_TransportType = 1,

    /** Maximum MTU that controller can use to send to accessory. */
    kHAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration_MaxControllerTransportMTU = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedDataStreamTransportConfiguration_Configuration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_DataStreamTransport_TransportType) {
    /** HomeKit Data Stream over TCP. */
    kHAPCharacteristicValue_DataStreamTransport_TransportType_TCP = 0,

    /** HomeKit Data Stream over HAP. */
    kHAPCharacteristicValue_DataStreamTransport_TransportType_HAP = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_DataStreamTransport_TransportType);
/**@}*/

/**
 * Setup Data Stream Transport.
 *
 * This is a control point characteristic which allows the controller to set up the data stream.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.121 Setup Data Stream Transport
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SetupDataStreamTransport "setup-data-stream-transport"

extern const HAPUUID kHAPCharacteristicType_SetupDataStreamTransport;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Request) {
    /** Session Command Type. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Request_SessionCommandType = 1,

    /** Transport Type. See Transport Type in `Supported` characteristic. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Request_TransportType = 2,

    /** Controller Key Salt. 32 bytes of random data. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Request_ControllerKeySalt = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Request);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType) {
    /** Start Session. */
    kHAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType_StartSession = 0,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_SessionCommandType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Response) {
    /** Status of the previous write issued. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Response_Status = 1,

    /** The transport type specific session parameters. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Response_SessionParameters = 2,

    /** Accessory Key Salt. 32 bytes of random data. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Response_AccessoryKeySalt = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Response);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Status) {
    /** Success. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Status_Success = 0,

    /** Generic error. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Status_GenericError = 1,

    /** Busy, maximum number of transfer transport sessions reached. */
    kHAPCharacteristicValue_SetupDataStreamTransport_Status_Busy = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SetupDataStreamTransport_Status);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_DataStreamTransport_SessionParameter_TCP) {
    /** TCP Listening Port. The port the accessory is listening to accept the transport connection. */
    kHAPCharacteristicValue_DataStreamTransport_SessionParameter_TCP_ListeningPort = 1,

    /** Session Identifier. An identifier used by the HAP transport to distinguish different streams. */
    kHAPCharacteristicValue_DataStreamTransport_SessionParameter_SessionIdentifier = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_DataStreamTransport_SessionParameter_TCP);
/**@}*/

/**
 * Data Stream HAP Transport.
 *
 * This characteristic provides a mechanism to run a Data Stream over plain HAP.
 *
 * This characteristic requires iOS 14 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.120 Supported Data Stream Transport Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_DataStreamHAPTransport "data-stream-hap-transport"

extern const HAPUUID kHAPCharacteristicType_DataStreamHAPTransport;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_DataStreamHAPTransport_Request) {
    /** Payload of the controller byte stream. */
    kHAPCharacteristicValue_DataStreamHAPTransport_Request_Payload = 1,

    /** Session Identifier. */
    kHAPCharacteristicValue_DataStreamHAPTransport_Request_SessionIdentifier = 2,

    /** Force Close. */
    kHAPCharacteristicValue_DataStreamHAPTransport_Request_ForceClose = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_DataStreamHAPTransport_Request);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_DataStreamHAPTransport_Response) {
    /** Payload of the accessory byte stream. */
    kHAPCharacteristicValue_DataStreamHAPTransport_Response_Payload = 1,

    /** Accessory Request to Send flag to signal whether controller should pull more data. */
    kHAPCharacteristicValue_DataStreamHAPTransport_Response_AccessoryRequestToSend = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_DataStreamHAPTransport_Response);
/**@}*/

/**
 * Data Stream HAP Transport Interrupt.
 *
 * This characteristic provides a notification to trigger Data Stream read operations over HAP.
 *
 * This characteristic requires iOS 14 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.120 Supported Data Stream Transport Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_DataStreamHAPTransportInterrupt "data-stream-hap-transport-interrupt"

extern const HAPUUID kHAPCharacteristicType_DataStreamHAPTransportInterrupt;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_DataStreamHAPTransportInterrupt) {
    /** Request to Send session identifier array. */
    kHAPCharacteristicValue_DataStreamHAPTransportInterrupt_RequestToSendIdentifiers = 1,

    /** Interrupt Sequence Number (to keep the characteristic value updates flowing). */
    kHAPCharacteristicValue_DataStreamHAPTransportInterrupt_SequenceNumber = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_DataStreamHAPTransportInterrupt);
/**@}*/

/**
 * Siri Input Type.
 *
 * This characteristic describes the type of Siri input used by the accessory.
 *
 * This characteristic requires iOS 12 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read
 * - Minimum Value: 0
 * - Maximum Value: 0 or 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.122 Siri Input Type
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SiriInputType "siri-input-type"

extern const HAPUUID kHAPCharacteristicType_SiriInputType;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SiriInputType) {
    /** Push button triggered Apple TV. */
    kHAPCharacteristicValue_SiriInputType_PushButtonTriggeredAppleTV = 0,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SiriInputType);
/**@}*/

/**
 * Supported Camera Recording Configuration.
 *
 * This characteristic allows an accessory to describe the general parameters supported when recording is triggered.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.125 Supported Camera Recording Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedCameraRecordingConfiguration \
    "supported-camera-recording-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedCameraRecordingConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedCameraRecordingConfiguration) {
    /**
     * The number of milliseconds of prebuffer recording available prior to the trigger event detection.
     * The minimum required is 4000 milliseconds.
     */
    kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_PrebufferDuration = 1,

    /** Bit mask of the different event trigger types supported. */
    kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_EventTriggerType = 2,

    /**
     * The recording media container information and supported configuration.
     * There is one TLV of this type per supported container format.
     */
    kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedCameraRecordingConfiguration);

HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_CameraRecordingEventTriggerType) {
    /** Motion. */
    kHAPCharacteristicValue_CameraRecordingEventTriggerType_Motion = 1U << 0U,

    /** Doorbell. */
    kHAPCharacteristicValue_CameraRecordingEventTriggerType_Doorbell = 1U << 1U
} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_CameraRecordingEventTriggerType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration) {
    /** Type of media container. */
    kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration_Type = 1,

    /** Media container type specific parameters. */
    kHAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration_Parameters = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedCameraRecordingConfiguration_ContainerConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_MediaContainerType) {
    /** Fragmented MP4. */
    kHAPCharacteristicValue_MediaContainerType_FragmentedMP4 = 0
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_MediaContainerType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_FragmentedMP4MediaContainerParameter) {
    /**
     * The maximum duration of a MP4 fragment in milliseconds.
     * If this is part of the Media Container Configuration TLV in the Selected Camera Recording Parameters,
     * then it is the selected fragment duration.
     */
    kHAPCharacteristicValue_FragmentedMP4MediaContainerParameter_FragmentDuration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_FragmentedMP4MediaContainerParameter);
/**@}*/

/**
 * Supported Video Recording Configuration.
 *
 * This characteristic enables IP camera accessories to advertise supported parameters for streaming video
 * over an RTP session.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.126 Supported Video Recording Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedVideoRecordingConfiguration \
    "supported-video-recording-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedVideoRecordingConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedVideoRecordingConfiguration) {
    /**
     * Specifies codec information and supported configurations for the codec.
     * There is one TLV of this type per supported codec.
     */
    kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedVideoRecordingConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration) {
    /** Specifies the type of video codec. */
    kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_CodecType = 1,

    /**
     * Specifies video codec-specific parameters.
     * The Video Codec Recording Parameters is encoded as a TLV8.
     */
    kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_CodecParameters = 2,

    /**
     * Specifies video attributes supported for the codec.
     * There is one TLV of this type per supported resolution.
     */
    kHAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration_Attributes = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedVideoRecordingConfiguration_VideoConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CameraRecording_VideoCodecType) {
    /** AVC/H.264. */
    kHAPCharacteristicValue_CameraRecording_VideoCodecType_H264 = 0,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CameraRecording_VideoCodecType);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CameraRecording_H264VideoCodecParameter) {
    /**
     * Specifies the type AVC/H.264 Profile.
     * Instance of this TLV must be present for each supported profile.
     */
    kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Profile = 1,

    /**
     * Specifies profile support level.
     * One instance of this TLV must be present for each supported level.
     */
    kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_Level = 2,

    /**
     * Applicable only to Selected Camera Recording Configuration.
     * Requested target bit rate in kbps.
     */
    kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_BitRate = 3,

    /**
     * Applicable only to Selected Camera Recording Configuration.
     * Requested I-frame interval in milliseconds basis supported represented as an unsigned integer.
     * The default value to be supported is 4 secs. The camera may send I-frames at a smaller interval
     * provided that the bit rate is respected.
     */
    kHAPCharacteristicValue_CameraRecording_H264VideoCodecParameter_IFrameInterval = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CameraRecording_H264VideoCodecParameter);
/**@}*/

/**
 * Supported Audio Recording Configuration.
 *
 * This characteristic allows an accessory to describe the parameters supported for recording audio.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.127 Supported Audio Recording Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedAudioRecordingConfiguration \
    "supported-audio-recording-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedAudioRecordingConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedAudioRecordingConfiguration) {
    /**
     * Specifies codec information and the configurations supported for the codec.
     * There is one TLV of this type per supported codec.
     */
    kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedAudioRecordingConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration) {
    /** Type of codec. */
    kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration_CodecType = 1,

    /** Specifies codec-specific parameters. */
    kHAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration_CodecParameters = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedAudioRecordingConfiguration_AudioConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AudioCodecRecordingParameter) {
    /** Specifies the number of audio channels. */
    kHAPCharacteristicValue_AudioCodecRecordingParameter_AudioChannels = 1,

    /** Specifies the bit rate mode. There is only one of this instance per supported bit rate mode. */
    kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRateMode = 2,

    /** Specifies the sample rate. */
    kHAPCharacteristicValue_AudioCodecRecordingParameter_SampleRate = 3,

    /** Specifies the target bit rate in kbps. Applicable to Selected Camera Recording Configuration only. */
    kHAPCharacteristicValue_AudioCodecRecordingParameter_BitRate = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AudioCodecRecordingParameter);
/**@}*/

/**
 * Selected Camera Recording Configuration.
 *
 * This is a control point characteristic that enables the device to specify the recording parameters
 * used by an IP camera accessory. The accessory applies configuration changes to the next recording session.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.128 Selected Camera Recording Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SelectedCameraRecordingConfiguration \
    "selected-camera-recording-configuration"

extern const HAPUUID kHAPCharacteristicType_SelectedCameraRecordingConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration) {
    /** Specifies general parameters selected for the recording. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording = 1,

    /** Specifies video parameters selected for the recording. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video = 2,

    /** Specifies audio parameters selected for the recording. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording) {
    /**
     * Specifies in milliseconds a period prior to an event that is included in the recording.
     * The specified value shall be less or equal to the value reported by the accessory
     * in the Supported Camera Recording Configuration characteristic.
     */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_PrebufferDuration = 1,

    /**
     * Specifies a bit mask of the enabled event trigger types that can start a recording.
     * By default, all event triggers are disabled.
     */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_EventTriggerType = 2,

    /** Specifies the recording media container configuration. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording_ContainerConfiguration = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Recording);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration) {
    /** Type of media container. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration_Type = 1,

    /** Media container type specific parameters. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration_Parameters = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_ContainerConfiguration);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video) {
    /** Specifies the type of video codec. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecType = 1,

    /** Specifies the type of specific parameters for the recording session. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_CodecParameters = 2,

    /** Specifies video attributes selected for the recording session. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video_Attributes = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Video);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio) {
    /** Specifies the type of audio codec. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecType = 1,

    /** Specifies audio codec-specific parameters. */
    kHAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio_CodecParameters = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SelectedCameraRecordingConfiguration_Audio);
/**@}*/

/**
 * Network Client Profile Control.
 *
 * This characteristic allows a controller to manage network client profiles, credentials,
 * and associated firewall rules.
 *
 * Only admin controllers are allowed to perform operations on this characteristic;
 * access by non-admin controllers must result in HAP status code -70401 - "Insufficient Privileges".
 * The `Managed Network Enable` characteristic must be enabled to write to this characteristic;
 * otherwise, HAP status code -70412 - "Not allowed" is returned.
 *
 * The Wi-Fi router must ensure that PSK credentials managed via this characteristic do not conflict with
 * other PSKs managed by the router, including the default PSK. Any operation that would introduce a PSK collision
 * must be rejected with 7 - "Error. Duplicate".
 *
 * When this characteristic is updated or removed, any established conversations that are no longer permitted
 * by the updated firewall rules must be closed. Established conversations that would still be allowed under the
 * updated rules should not be affected. If a Wi-Fi router places clients in different subnets and a profile change
 * results in the need to move a client to a different subnet the Wi-Fi router must (1) immediately attempt to force
 * the client to obtain a new IP address, and (2) not forward any packets to/from the client using the old IP address.
 *
 * Modifications to Network Client Profiles managed by this characteristic must only be done through writing
 * to this characteristic. Additional network client configurations managed by the Wi-Fi router (if any)
 * must be kept separate from the profiles in this characteristic.
 *
 * At factory reset or when the last pairing is removed, all data written to this characteristic
 * and associated data (e.g., installed firewall rules, installed PSKs) must be deleted.
 * Further, any state related to generating the next Network Client Profile Identifier, if any, must be reset.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response, Timed Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.142 Network Client Profile Control
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NetworkClientProfileControl "network-client-profile-control"

extern const HAPUUID kHAPCharacteristicType_NetworkClientProfileControl;

/**
 * Wi-Fi Router Control Point Operation Status.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_OperationStatus) {
    /** Success. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_Success = 0,

    /** Error. Unknown. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_Unknown = 1,

    /** Error. Not allowed. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_NotAllowed = 2,

    /** Error. Out of resources. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_OutOfResources = 3,

    /** Error. Bulk operation failed. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_BulkOperationFailed = 4,

    /** Error. Invalid parameters (when more specific error is unavailable). */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidParameters = 5,

    /** Error. Invalid Identifier. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidIdentifier = 6,

    /** Error. Duplicate. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_Duplicate = 7,

    /** Error. Invalid firewall rule. */
    kHAPCharacteristicValue_WiFiRouter_OperationStatus_InvalidFirewallRule = 8,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_OperationStatus);

/**
 * IP Address.
 */
typedef struct {
    /** IPv4 Address. The bytes containing the IPv4 address. */
    HAPIPv4Address ipv4Address;
    bool ipv4AddressIsSet;

    /** IPv6 Address. The bytes containing the IPv6 address. */
    HAPIPv6Address ipv6Address;
    bool ipv6AddressIsSet;
} HAPCharacteristicValue_WiFiRouter_IPAddress;

/**
 * Credential Data Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValueType_WiFiRouter_Credential) {
    kHAPCharacteristicValueType_WiFiRouter_Credential_MACAddress = 1,
    kHAPCharacteristicValueType_WiFiRouter_Credential_PSK = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValueType_WiFiRouter_Credential);

/**
 * Credential Data.
 */
typedef struct {
    /** Type of the value. */
    HAPCharacteristicValueType_WiFiRouter_Credential type;

    /** Type-specific value. */
    union {
        /**
         * MAC Address Credential. Must be defined for MAC address based credential.
         * This is used for Ethernet clients and for Wi-Fi clients where the default PSK is used.
         * A data buffer of 6 bytes containing the MAC address.
         */
        HAPMACAddress macAddress;

        /**
         * PSK Credential. Must be defined for PSK based credential.
         * This is used for Wi-Fi clients where a unique PSK is used.
         * A data buffer containing the PSK.
         * If the length is 8 to 63 bytes each being 32 to 126 decimal, then it's a plain-text password.
         * Otherwise, it's expected to be a pre-hashed, 256-bit pre-shared key.
         *
         * For security reasons, if the Credential Data TLV is returned as part of a read operation
         * (e.g., as part of a Network Client Profile Read), this field (if present) must be masked by replacing it
         * with an empty buffer to indicate the presence of a PSK without revealing the PSK value itself.
         */
        const char* psk;
    } _;
} HAPCharacteristicValue_WiFiRouter_Credential;

/**
 * ICMP Protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_ICMPProtocol) {
    /** ICMPv4. */
    kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv4 = 0,

    /** ICMPv6. */
    kHAPCharacteristicValue_WiFiRouter_ICMPProtocol_ICMPv6 = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_ICMPProtocol);

/**
 * ICMP Type.
 */
typedef struct {
    /** ICMP Protocol. */
    HAPCharacteristicValue_WiFiRouter_ICMPProtocol icmpProtocol;

    /**
     * Optional. ICMP type value to match.
     * If absent, all types of ICMP messages for the specified protocol match this record.
     */
    uint8_t typeValue;
    bool typeValueIsSet;
} HAPCharacteristicValue_WiFiRouter_ICMPType;

/**
 * ICMP Type List.
 *
 * - The TLV can contain 1 through 16 ICMP Type TLV items.
 */
typedef struct {
    /** ICMP Types. */
    HAPCharacteristicValue_WiFiRouter_ICMPType icmpTypes[16];
    size_t numICMPTypes;
} HAPCharacteristicValue_WiFiRouter_ICMPList;

/**
 * Firewall Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_FirewallType) {
    /** Full Access. No access restrictions on WAN/within same LAN. */
    kHAPCharacteristicValue_WiFiRouter_FirewallType_FullAccess = 0,

    /** Allowlist. No access except those specified in the rule list. */
    kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_FirewallType);

/**
 * WAN Port Rule Protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol) {
    /** TCP. */
    kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_TCP = 0,

    /** UDP. */
    kHAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol_UDP = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol);

/**
 * WAN Port Rule.
 *
 * A single WAN firewall rule for TCP or UDP traffic.
 */
typedef struct {
    /** Protocol. */
    HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol transportProtocol;

    /**
     * Optional. Fully-qualified DNS name or pattern of the host(s).
     * Either this field or Host IP Address Start must be specified, but not both.
     *
     * DNS names must adhere to RFC 1123: 1 to 253 characters in length, consisting of a sequence of labels
     * delimited by dots ("."). Each label must be 1 to 63 characters in length, contain only
     * ASCII letters ("a"-"Z"), digits ("0"-"9"), or hyphens ("-") and must not start or end with a hyphen.
     *
     * Patterns follow the syntax of DNS names, but additionally allow the wildcard character "*" to be used up to
     * twice per label to match 0 or more characters within that label. Note that the wildcard never matches a dot
     * (e.g., "*.example.com" matches "api.example.com" but not "api.us.example.com").
     *
     * A valid name or pattern must be fully qualified, i.e., consist of at least two labels.
     * The final label must not be fully numeric, and must not be the "local" pseudo-TLD.
     * A pattern must end with at least two literal (non-wildcard) labels.
     *
     * Examples:
     * Valid: "example.com", "*.example.com", "video*.example.com", "api*.*.example.com"
     * Invalid: "ipcamera", "ipcamera.local", "*", "*.com", "8.8.8.8"
     */
    const char* hostDNSName;
    bool hostDNSNameIsSet;

    /**
     * Optional IP Address TLV item.
     * IP address of host, or IP address of host starting range (inclusive) if Host IP Address End is specified.
     * Addresses of all zeros have special meaning as a wildcard.
     * Either this field or Host DNS Name must be specified, but not both.
     */
    HAPCharacteristicValue_WiFiRouter_IPAddress hostIPStart;
    bool hostIPStartIsSet;

    /**
     * Optional IP Address TLV item.
     * IP address of host ending range (inclusive).
     * Can be set only if Host IP Address Start is also set.
     * Must represent an IP address after Host IP Address Start.
     * The IP address type must be the same type as in Host IP Address Start,
     * and neither field can be a wildcard address.
     */
    HAPCharacteristicValue_WiFiRouter_IPAddress hostIPEnd;
    bool hostIPEndIsSet;

    /** Port, or starting port range (inclusive) if the Host Port End is specified. A value of 0 means any port. */
    HAPNetworkPort hostPortStart;

    /**
     * Optional. Ending port range (inclusive).
     * Only allowed if Host Port Start is specified and not 0.
     * Must be greater than Host Port Start.
     */
    HAPNetworkPort hostPortEnd;
    bool hostPortEndIsSet;
} HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule;

/**
 * WAN ICMP Rule.
 *
 * A WAN firewall rule for ICMP traffic.
 */
typedef struct {
    /**
     * Optional. Fully-qualified DNS name or pattern of the host(s).
     * Either this field or Host IP Address Start must be specified, but not both.
     *
     * DNS names must adhere to RFC 1123: 1 to 253 characters in length, consisting of a sequence of labels
     * delimited by dots ("."). Each label must be 1 to 63 characters in length, contain only
     * ASCII letters ("a"-"Z"), digits ("0"-"9"), or hyphens ("-") and must not start or end with a hyphen.
     *
     * Patterns follow the syntax of DNS names, but additionally allow the wildcard character "*" to be used up to
     * twice per label to match 0 or more characters within that label. Note that the wildcard never matches a dot
     * (e.g., "*.example.com" matches "api.example.com" but not "api.us.example.com").
     *
     * A valid name or pattern must be fully qualified, i.e., consist of at least two labels.
     * The final label must not be fully numeric, and must not be the "local" pseudo-TLD.
     * A pattern must end with at least two literal (non-wildcard) labels.
     *
     * Examples:
     * Valid: "example.com", "*.example.com", "video*.example.com", "api*.*.example.com"
     * Invalid: "ipcamera", "ipcamera.local", "*", "*.com", "8.8.8.8"
     */
    const char* hostDNSName;
    bool hostDNSNameIsSet;

    /**
     * Optional IP Address TLV item.
     * IP address of host, or IP address of host starting range (inclusive) if Host IP Address End is specified.
     * Addresses of all zeros have special meaning as a wildcard.
     * Either this field or Host DNS Name must be specified, but not both.
     */
    HAPCharacteristicValue_WiFiRouter_IPAddress hostIPStart;
    bool hostIPStartIsSet;

    /**
     * Optional IP Address TLV item.
     * IP address of host ending range (inclusive).
     * Can be set only if Host IP Address Start is also set.
     * Must represent an IP address after Host IP Address Start.
     * The IP address type must be the same type as in Host IP Address Start,
     * and neither field can be a wildcard address.
     */
    HAPCharacteristicValue_WiFiRouter_IPAddress hostIPEnd;
    bool hostIPEndIsSet;

    /**
     * Must contain at least one ICMP Type record. Specifies the protocol and types to allow.
     * An ICMP packet is allowed by this rule if it matches any of the ICMP Type records in the list.
     */
    HAPCharacteristicValue_WiFiRouter_ICMPList icmpList;
} HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule;

/**
 * WAN Firewall Rule Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule) {
    kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_Port = 1,
    kHAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule_ICMP = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule);

/**
 * WAN Firewall Rule.
 */
typedef struct {
    /** Type of the value. */
    HAPCharacteristicValueType_WiFiRouter_WANFirewall_Rule type;

    /** Type-specific value. */
    union {
        /** WAN Port Rule. */
        HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule port;

        /** WAN ICMP Rule. */
        HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule icmp;
    } _;
} HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule;

/**
 * Callback that should be invoked for each WAN Firewall Rule.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WiFiRouter_WANFirewall_RuleList)(
        void* _Nullable context,
        HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule* value,
        bool* shouldContinue);

/**
 * WAN Firewall Rule List.
 */
typedef struct {
    /**
     * Enumerates all WAN Firewall Rules.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WiFiRouter_WANFirewall_RuleList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule _;
} HAPCharacteristicValue_WiFiRouter_WANFirewall_RuleList;

/**
 * WAN Firewall Configuration.
 */
typedef struct {
    /** Specifies the WAN Firewall type. */
    HAPCharacteristicValue_WiFiRouter_FirewallType type;

    /**
     * List of WAN firewall rules.
     * Optional, but must be defined if WAN Firewall Type is Allowlist.
     */
    HAPCharacteristicValue_WiFiRouter_WANFirewall_RuleList ruleList;
    bool ruleListIsSet;
} HAPCharacteristicValue_WiFiRouter_WANFirewall;

/**
 * LAN Firewall Rule Direction.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction) {
    /** Outbound. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Outbound = 0,

    /** Inbound. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction_Inbound = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction);

/**
 * Client Group Identifier.
 *
 * - 0: Invalid.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_GroupIdentifier) {
    /** Main. */
    kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Main = 1,

    /** Restricted. */
    kHAPCharacteristicValue_WiFiRouter_GroupIdentifier_Restricted = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_GroupIdentifier);

/**
 * Endpoint List.
 *
 * - The TLV can contain 1 through 32 Client Group Identifier TLV items.
 */
typedef struct {
    /** Client Group Identifiers. */
    uint32_t groupIdentifiers[32];
    size_t numGroupIdentifiers;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList;

/**
 * Multicast Bridging Rule.
 *
 * A LAN firewall rule that allows UDP broadcast or multicast traffic.
 */
typedef struct {
    /** Direction. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction direction;

    /** List of endpoint identifiers to allow traffic with. Must contain at least one identifier. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList endpointList;

    /** Destination multicast or broadcast IP address. This is an IP Address TLV item. */
    HAPCharacteristicValue_WiFiRouter_IPAddress ipAddress;

    /** Destination port. 0 indicates any port. */
    HAPNetworkPort port;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule;

/**
 * Static Port Rule Protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol) {
    /** TCP. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_TCP = 0,

    /** UDP. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol_UDP = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol);

/**
 * Static Port Rule.
 *
 * A LAN firewall rule that allows TCP or unicast UDP traffic between static ports.
 */
typedef struct {
    /** Direction. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction direction;

    /** List of endpoint identifiers to allow traffic with. Must contain at least one identifier. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList endpointList;

    /** Protocol. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol transportProtocol;

    /**
     * Destination port, or starting destination port range (inclusive) if Destination Port End is specified.
     * A value of 0 means any port.
     */
    HAPNetworkPort portStart;

    /**
     * Optional. Ending destination port range (inclusive).
     * Only allowed if Destination Port Start is specified and not 0. Must be greater than Destination Port Start.
     */
    HAPNetworkPort portEnd;
    bool portEndIsSet;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule;

/**
 * Advertisement Protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol) {
    /** DNS-SD. */
    kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_DNSSD = 0,

    /** SSDP. */
    kHAPCharacteristicValue_WiFiRouter_AdvertisementProtocol_SSDP = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol);

/**
 * Dynamic Port Rule Flags.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags) {
    /** Advertisement only. Only bridge the advertisements. Do not open any pinholes. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags_AdvertisementOnly = 1U << 0U
} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Flags);

/**
 * Dynamic Port Rule Service Type.
 */
typedef struct {
    /**
     * For DNS-SD, Name represents the DNS-SD service type.
     * Note this is only the logical service name (e.g., "hap", "airplay", "raop");
     * the full DNS-SD service label (e.g., "_hap._tcp" is derived from this value and the Protocol.
     *
     * For SSDP, Name represents the SSDP service type URI.
     */
    const char* name;
} HAPCharacteristicValue_WiFiRouter_ServiceType;

/**
 * Dynamic Port Rule Protocol.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol) {
    /** TCP. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_TCP = 0,

    /** UDP. */
    kHAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol_UDP = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol);

/**
 * Dynamic Port Rule.
 *
 * A LAN firewall rule that allows advertisement or discovery of a particular type of service, and optionally allows
 * TCP or unicast UDP traffic to or from the advertised service.
 */
typedef struct {
    /**
     * Direction.
     *
     * - Outbound: The client is allowed to listen for advertisements and discover services as specified by the
     *   Service Type field. Unless the "advertisement only" flag is set, the client is also allowed to initiate
     *   conversations with the discovered services.
     * - Inbound: The client is allowed to advertise services as specified by the Service Type field.
     *   Unless the "advertisement only" flag is set, the client is also allowed to accept conversations on
     *   any advertised service ports.
     *
     * - All advertisement traffic required by the service discovery protocol is implicitly allowed by this rule.
     */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction direction;

    /** List of endpoint identifiers to allow traffic with. Must contain at least one identifier. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList endpointList;

    /** Protocol (excluding traffic related to the advertisement). */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol transportProtocol;

    /** The protocol advertising the port to listen to. */
    HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol advertProtocol;

    /** Additional bitmask flags. */
    uint32_t flags;

    /**
     * Optional. If set, only ports associated with advertisements that match the specified service type
     * will be advertised and optionally opened.
     */
    HAPCharacteristicValue_WiFiRouter_ServiceType service;
    bool serviceIsSet;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule;

/**
 * Static ICMP Rule.
 *
 * A LAN firewall rule that allows ICMP traffic to static IP addresses and/or ports.
 */
typedef struct {
    /** Direction. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction direction;

    /** List of endpoint identifiers to allow traffic with. Must contain at least one identifier. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList endpointList;

    /**
     * Must contain at least one ICMP Type record. Specifies the protocol and types to allow.
     * An ICMP packet is allowed by this rule if it matches any of the ICMP Type records in the list.
     */
    HAPCharacteristicValue_WiFiRouter_ICMPList icmpList;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule;

/**
 * LAN Firewall Rule Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule) {
    kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_MulticastBridging = 1,
    kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticPort = 2,
    kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_DynamicPort = 3,
    kHAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule_StaticICMP = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule);

/**
 * LAN Firewall Rule.
 */
typedef struct {
    /** Type of the value. */
    HAPCharacteristicValueType_WiFiRouter_LANFirewall_Rule type;

    /** Type-specific value. */
    union {
        /** TLV describing the multicast bridging rule. */
        HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule multicastBridging;

        /** TLV describing the static port rule. */
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule staticPort;

        /** TLV describing the dynamic port rule. */
        HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule dynamicPort;

        /** TLV describing the ICMP rule. */
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule staticICMP;
    } _;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule;

/**
 * Callback that should be invoked for each LAN Firewall Rule.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_RuleList)(
        void* _Nullable context,
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule* value,
        bool* shouldContinue);

/**
 * LAN Firewall Rule List.
 */
typedef struct {
    /**
     * Enumerates all LAN Firewall Rules.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_RuleList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule _;
} HAPCharacteristicValue_WiFiRouter_LANFirewall_RuleList;

/**
 * LAN Firewall Configuration.
 */
typedef struct {
    /** Specifies the LAN Firewall type. */
    HAPCharacteristicValue_WiFiRouter_FirewallType type;

    /**
     * List of LAN firewall rules.
     * Optional, but must be defined if LAN Firewall Type is Allowlist.
     */
    HAPCharacteristicValue_WiFiRouter_LANFirewall_RuleList ruleList;
    bool ruleListIsSet;
} HAPCharacteristicValue_WiFiRouter_LANFirewall;

/**
 * Network Client Profile Configuration.
 */
typedef struct {
    /**
     * Identifier of the network client profile. 0 is invalid.
     * The Wi-Fi router must assign this value monotonically as new entries are added.
     */
    uint32_t clientIdentifier;
    bool clientIsSet;

    /** Identifier of the Client Group associated with this network client profile. */
    uint32_t groupIdentifier;
    bool groupIsSet;

    /**
     * The credential data.
     * For security reasons, read operations must mask the PSK Credential fields inside this value.
     */
    HAPCharacteristicValue_WiFiRouter_Credential credential;
    bool credentialIsSet;

    /** Specifies the WAN firewall rules for this network client profile. */
    HAPCharacteristicValue_WiFiRouter_WANFirewall wanFirewall;
    bool wanFirewallIsSet;

    /** Specifies the LAN firewall rules for this network client profile. */
    HAPCharacteristicValue_WiFiRouter_LANFirewall lanFirewall;
    bool lanFirewallIsSet;
} HAPCharacteristicValue_NetworkClientProfileControl_Config;

/**
 * Network Client Profile Control Operation Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type) {
    /**
     * List.
     * Returns the list of all Network Client Profile Identifiers.
     * The Network Client Profile Configuration in the request is not required.
     * The write response returns a list of Network Client Profile Control Operation Responses
     * with just the Network Client Profile Identifier in the Network Client Profile Configuration specified.
     */
    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_List = 1,

    /**
     * Read.
     * Returns the Network Client Profile Configuration specified by the Network Client Profile Identifier
     * in the Network Client Profile Configuration in the request; all other TLV items in the request are ignored.
     */
    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Read = 2,

    /**
     * Add.
     * Adds a new network client profile configuration as specified in the Network Client Profile Configuration;
     * the Network Client Profile Identifier in the request must not be defined.
     * If successful, the Network Client Profile Configuration in the response contains just the
     * Network Client Profile Identifier for the newly added configuration.
     * If not successful, the response does not include the Network Client Profile Configuration.
     *
     * If the new network client profile credential already exists, the operation must fail and return
     * 7 - "Error. Duplicate".
     */
    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add = 3,

    /**
     * Remove.
     * Removes a network client profile configuration as specified by the Network Client Profile Identifier
     * in the Network Client Profile Configuration; all other TLV items in the request are ignored.
     * Removing a non-existent Network Client Profile Identifier will not return an error.
     * The response does not include the Network Client Profile Configuration.
     */
    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove = 4,

    /**
     * Update.
     * Updates a network client profile configuration. In the Network Client Profile Configuration of the request,
     * only the Network Client Profile Identifier must be defined; all other TLV items are optional.
     * Only the TLV items that are specified in the request will be updated; all other items remain unchanged.
     * The response does not include the Network Client Profile Configuration.
     *
     * If the updated network client profile credential already exists, the operation must fail and return
     * 7 - "Error. Duplicate".
     */
    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type);

/**
 * Network Client Profile Control Operation.
 */
typedef struct {
    /** The requested operation. */
    HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type operationType;

    /** The parameters of the requested operation. */
    HAPCharacteristicValue_NetworkClientProfileControl_Config config;
    bool configIsSet;
} HAPCharacteristicValue_NetworkClientProfileControl_Operation;

/**
 * Callback that should be invoked for each Network Client Profile Control Operation.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkClientProfileControl)(
        void* _Nullable context,
        HAPCharacteristicValue_NetworkClientProfileControl_Operation* value,
        bool* shouldContinue);

/**
 * Network Client Profile Control.
 *
 * - The "Out of resources" HAP status is returned if the operation contains more bulk operations
 *   than the accessory can process in a single write.
 * - The Network Client Profile Control Response is returned with a Network Client Profile Control Operation Response
 *   for each corresponding Network Client Profile Control Operations in the Network Client Profile Control TLV,
 *   in the same order as in the request.
 *   The Status indicates if that configuration operation was successful, or the error if not.
 * - For the list operation, no other operations can be included in the bulk operation.
 * - For read operations, only read operations can be included in the bulk operation.
 * - Add, update, and remove operations can be combined into a single bulk operation
 *   provided the operations do not modify the same network client profile configuration
 *   (e.g., same Network Client Profile Identifier).
 *   The operations in the bulk operation are performed atomically.
 *   If one operation fails, then none of the operations are committed;
 *   in this case, the error status will be specified in the first operation that failed;
 *   all other Status will return 4 - "Bulk Operation Failed".
 */
typedef struct {
    /**
     * Enumerates all Network Client Profile Control Operations.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkClientProfileControl callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_NetworkClientProfileControl_Operation _;
} HAPCharacteristicValue_NetworkClientProfileControl;

/**
 * Network Client Profile Control Operation Response.
 */
typedef struct {
    /** Operation status. */
    HAPCharacteristicValue_WiFiRouter_OperationStatus status;

    /** The response to the requested operation. */
    HAPCharacteristicValue_NetworkClientProfileControl_Config config;
    bool configIsSet;
} HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp;

/**
 * Callback that should be invoked for each Network Client Profile Control Operation Response.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkClientProfileControl_Response)(
        void* _Nullable context,
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp* value,
        bool* shouldContinue);

/**
 * Network Client Profile Control Response.
 */
typedef struct {
    /**
     * Enumerates all Network Client Profile Control Operation Responses.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkClientProfileControl_Response callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp _;
} HAPCharacteristicValue_NetworkClientProfileControl_Response;

/**
 * Callback that should be invoked for each Network Client Profile Identifier.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WiFiRouter_ClientList)(
        void* _Nullable context,
        uint32_t* value,
        bool* shouldContinue);

/**
 * Network Client Profile Identifier List.
 */
typedef struct {
    /**
     * Enumerates all Network Client Profile Identifiers.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WiFiRouter_ClientList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    uint32_t _;
} HAPCharacteristicValue_WiFiRouter_ClientList;

/**
 * Network Client Profile Control Event.
 *
 * If notifications are configured, this TLV is sent to the controller to report network client profile changes.
 */
typedef struct {
    /**
     * Optional. If specified, this describes the list of Network Client Profile Identifiers of
     * network client profile configurations that have changed. Otherwise, an unknown set of
     * network client profile configurations have changed.
     */
    HAPCharacteristicValue_WiFiRouter_ClientList clientList;
    bool clientListIsSet;
} HAPCharacteristicValue_NetworkClientProfileControl_Event;
/**@}*/

/**
 * Network Client Status Control.
 *
 * This characteristic allows a controller to retrieve information about specific network clients.
 * Only admin controllers are allowed to perform operations on this characteristic;
 * access by non-admin controllers must result in HAP status code -70401 - "Insufficient Privileges".
 * The `Managed Network Enable` characteristic must be enabled to write to this characteristic;
 * otherwise, HAP status code -70412 - "Not allowed" is returned.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.143 Network Client Status Control
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NetworkClientStatusControl "network-client-status-control"

extern const HAPUUID kHAPCharacteristicType_NetworkClientStatusControl;

/**
 * Network Client Status Identifier Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValueType_NetworkClientStatusControl_Identifier) {
    kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_Client = 1,
    kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_MACAddress = 2,
    kHAPCharacteristicValueType_NetworkClientStatusControl_Identifier_IPAddress = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValueType_NetworkClientStatusControl_Identifier);

/**
 * Network Client Status Identifier.
 */
typedef struct {
    /** Type of the value. */
    HAPCharacteristicValueType_NetworkClientStatusControl_Identifier type;

    /** Type-specific value. */
    union {
        /** Identifier of the network client profile. */
        uint32_t clientIdentifier;

        /** MAC address of the network client. Must be 6 bytes. */
        HAPMACAddress macAddress;

        /** IP Address of the network client. */
        HAPCharacteristicValue_WiFiRouter_IPAddress ipAddress;
    } _;
} HAPCharacteristicValue_NetworkClientStatusControl_Identifier;

/**
 * Callback that should be invoked for each Network Client Status Identifier.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkClientStatusControl_IdentifierList)(
        void* _Nullable context,
        HAPCharacteristicValue_NetworkClientStatusControl_Identifier* value,
        bool* shouldContinue);

/**
 * Network Client Status Identifier List.
 */
typedef struct {
    /**
     * Enumerates all Network Client Status Identifiers.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkClientStatusControl_IdentifierList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_NetworkClientStatusControl_Identifier _;
} HAPCharacteristicValue_NetworkClientStatusControl_IdentifierList;

/**
 * Network Client Status Control Operation Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NetworkClientStatusControl_OperationType) {
    /**
     * Read network client status.
     * Returns the network client status for each client specified in the Network Client Status Identifier List.
     * The write response returns a Network Client Status List TLV, which contains a Network Client Status TLV for each
     * network client status requested. The response will omit responses where the requested Network Client Status
     * Identifier does not match any existing connected network clients.
     */
    kHAPCharacteristicValue_NetworkClientStatusControl_OperationType_Read = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NetworkClientStatusControl_OperationType);

/**
 * Network Client Status Control.
 */
typedef struct {
    /** Operation Type. */
    HAPCharacteristicValue_NetworkClientStatusControl_OperationType operationType;

    /** List of Network Client Status Identifiers to be read. */
    HAPCharacteristicValue_NetworkClientStatusControl_IdentifierList identifierList;
} HAPCharacteristicValue_NetworkClientStatusControl;

/**
 * Callback that should be invoked for each IP Address.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkClientStatusControl_IPAddressList)(
        void* _Nullable context,
        HAPCharacteristicValue_WiFiRouter_IPAddress* value,
        bool* shouldContinue);

/**
 * IP Address List.
 */
typedef struct {
    /**
     * Enumerates all IP Addresses.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkClientStatusControl_IPAddressList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WiFiRouter_IPAddress _;
} HAPCharacteristicValue_NetworkClientStatusControl_IPAddressList;

/**
 * Network Client Status.
 */
typedef struct {
    /**
     * The identifier of the network client profile, if it is managed.
     * If not, then this TLV item is not specified.
     */
    uint32_t clientIdentifier;
    bool clientIsSet;

    /** The network client's MAC address. Must be 6 bytes. */
    HAPMACAddress macAddress;

    /**
     * Optional. The network client's list of IP addresses.
     * Must be present if the network client has been assigned an IP address.
     * The TLV must contain at least 1 IP Address TLV item.
     */
    HAPCharacteristicValue_NetworkClientStatusControl_IPAddressList ipAddressList;
    bool ipAddressListIsSet;

    /** Optional. The network client's advertised name (such as the hostname from the DHCP request), if any. */
    const char* name;
    bool nameIsSet;

    /**
     * Optional. RSSI for the network client in dBm.
     * Not present if network client is not connected via Wi-Fi or if the information is unknown.
     */
    int32_t rssi;
    bool rssiIsSet;
} HAPCharacteristicValue_NetworkClientStatusControl_Status;

/**
 * Callback that should be invoked for each Network Client Status.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkClientStatusControl_Response)(
        void* _Nullable context,
        HAPCharacteristicValue_NetworkClientStatusControl_Status* value,
        bool* shouldContinue);

/**
 * Network Client Status List.
 */
typedef struct {
    /**
     * Enumerates all Network Client Statuses.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkClientStatusControl_Response callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_NetworkClientStatusControl_Status _;
} HAPCharacteristicValue_NetworkClientStatusControl_Response;
/**@}*/

/**
 * Router Status.
 *
 * This characteristic describes the router status. A value of 0 ("Ready") indicates that the current
 * network configurations, including configurations related to the `Managed Network Enable` and
 * `Network Client Profile Control`, have been deployed and are fully operational. Note, the router status
 * does not represent the state of the WAN link (online, offline); see `WAN Status List` for WAN link status.
 * A value of 1 ("Not Ready") indicates that the router is in the process of setting up the network.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.138 Router Status
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RouterStatus "router-status"

extern const HAPUUID kHAPCharacteristicType_RouterStatus;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_RouterStatus) {
    /** Ready. */
    kHAPCharacteristicValue_RouterStatus_Ready = 0,

    /** Not Ready. */
    kHAPCharacteristicValue_RouterStatus_NotReady = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_RouterStatus);
/**@}*/

/**
 * Supported Router Configuration.
 *
 * This characteristic describes the supported router features.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.137 Supported Router Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedRouterConfiguration "supported-router-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedRouterConfiguration;

/**
 * Router Flags.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_SupportedRouterConfiguration_Flags) {
    /** Supports Managed Network. Must be set. */
    kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsManagedNetwork = 1U << 0U,

    /** Supports MAC address based credentials. Must be set. */
    kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsMACAddressCredentials = 1U << 1U,

    /** Supports PSK based credentials. Must be set. */
    kHAPCharacteristicValue_SupportedRouterConfiguration_Flags_SupportsPSKCredentials = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_SupportedRouterConfiguration_Flags);

/**
 * Supported Router Configuration.
 */
typedef struct {
    /** Flags of supported features. */
    uint32_t flags;
} HAPCharacteristicValue_SupportedRouterConfiguration;
/**@}*/

/**
 * WAN Configuration List.
 *
 * This characteristic manages the WAN links.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.139 WAN Configuration List
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WANConfigurationList "wan-configuration-list"

extern const HAPUUID kHAPCharacteristicType_WANConfigurationList;

/**
 * WAN Identifier.
 *
 * - 0: Invalid.
 * - 1 - 1023: Reserved for predefined purposes.
 * - 1024 - 2047: Can be used by vendors to represent non-standard WANs.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiRouter_WANIdentifier) {
    /**
     * Main WAN.
     * The main WAN.
     */
    kHAPCharacteristicValue_WiFiRouter_WANIdentifier_Main = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiRouter_WANIdentifier);

/**
 * WAN Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WANConfigurationList_Config_WANType) {
    /** Unconfigured. */
    kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Unconfigured = 0,

    /** Other. */
    kHAPCharacteristicValue_WANConfigurationList_Config_WANType_Other = 1,

    /** DHCP. */
    kHAPCharacteristicValue_WANConfigurationList_Config_WANType_DHCP = 2,

    /** Bridge Mode. */
    kHAPCharacteristicValue_WANConfigurationList_Config_WANType_BridgeMode = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WANConfigurationList_Config_WANType);

/**
 * WAN Configuration.
 */
typedef struct {
    /** Identifier of this WAN link configuration. */
    uint32_t wanIdentifier;

    /** The WAN connection type. */
    HAPCharacteristicValue_WANConfigurationList_Config_WANType wanType;
} HAPCharacteristicValue_WANConfigurationList_Config;

/**
 * Callback that should be invoked for each WAN Configuration.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WANConfigurationList)(
        void* _Nullable context,
        HAPCharacteristicValue_WANConfigurationList_Config* value,
        bool* shouldContinue);

/**
 * WAN Configuration List.
 */
typedef struct {
    /**
     * Enumerates all WAN Configurations.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WANConfigurationList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WANConfigurationList_Config _;
} HAPCharacteristicValue_WANConfigurationList;
/**@}*/

/**
 * WAN Status List.
 *
 * This characteristic describes WAN/Internet link statuses.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.140 WAN Status List
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WANStatusList "wan-status-list"

extern const HAPUUID kHAPCharacteristicType_WANStatusList;

/**
 * WAN Link Status.
 */
HAP_OPTIONS_BEGIN(uint16_t, HAPCharacteristicValue_WANStatusList_Status_LinkStatus) {
    /** Unknown. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Unknown = 1U << 0U,

    /** No cable connected. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoCableConnected = 1U << 1U,

    /** No IP address. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoIPAddress = 1U << 2U,

    /** No gateway specified. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoGatewaySpecified = 1U << 3U,

    /** Gateway unreachable. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_GatewayUnreachable = 1U << 4U,

    /** No DNS server(s) specified. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_NoDNSServerSpecified = 1U << 5U,

    /** DNS server(s) unreachable. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_DNSServerUnreachable = 1U << 6U,

    /** Authentication failed. */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_AuthenticationFailed = 1U << 7U,

    /** Walled. (WAN link is available, but crippled.) */
    kHAPCharacteristicValue_WANStatusList_Status_LinkStatus_Walled = 1U << 8U
} HAP_OPTIONS_END(uint16_t, HAPCharacteristicValue_WANStatusList_Status_LinkStatus);

/**
 * WAN Status.
 */
typedef struct {
    /** Identifier of the WAN link that this status describes. */
    uint32_t wanIdentifier;

    /** Bitmask describing the WAN link status and error. No bits set indicates the WAN link is online. */
    uint64_t linkStatus;
} HAPCharacteristicValue_WANStatusList_Status;

/**
 * Callback that should be invoked for each WAN Status.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WANStatusList)(
        void* _Nullable context,
        HAPCharacteristicValue_WANStatusList_Status* value,
        bool* shouldContinue);

/**
 * WAN Status List.
 */
typedef struct {
    /**
     * Enumerates all WAN Statuses.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WANStatusList callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WANStatusList_Status _;
} HAPCharacteristicValue_WANStatusList;
/**@}*/

/**
 * Managed Network Enable.
 *
 * This characteristic enables or disables the managed network. The network is fully operational after the
 * `Router Status` state changes to "Ready". When the `Managed Network Enable` characteristic is set
 * to 1 ("Enabled"), the network client profile configurations can be created and managed (via the
 * `Network Client Profile Control` characteristic) and network policies are enforced. When the
 * Managed Network Enable characteristic is set to 0 ("Disabled"), all network client profile configurations,
 * (set via the `Network Client Profile Control`) are deleted and any corresponding network client devices
 * are disassociated from the network.
 *
 * Only admin controllers are allowed to write to this characteristic;
 * access by non-admin controllers must result in HAP status code -70401 - "Insufficient Privileges".
 * This characteristic cannot be set to 1 ("Enabled") if any of the WAN links are configured to bridge mode;
 * if this happens, HAP status code -70412 - "Not allowed" is returned.
 *
 * At factory reset, when the last pairing is removed or when any of the WAN links are configured to bridge mode
 * this characteristic must be set to 0 ("Disabled") and all network client profile configurations deleted
 * and clients disassociated.
 *
 * The following characteristics are only accessible when this characteristic is enabled:
 * `Network Client Profile Control`, `Network Client Status Control` and `Network Access Violation Control`.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Timed Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.141 Managed Network Enable
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ManagedNetworkEnable "managed-network-enable"

extern const HAPUUID kHAPCharacteristicType_ManagedNetworkEnable;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ManagedNetworkEnable) {
    /** Disabled. */
    kHAPCharacteristicValue_ManagedNetworkEnable_Disabled = 0,

    /** Enabled. */
    kHAPCharacteristicValue_ManagedNetworkEnable_Enabled = 1,

    /** Unknown (read only). */
    kHAPCharacteristicValue_ManagedNetworkEnable_Unknown = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ManagedNetworkEnable);
/**@}*/

/**
 * HomeKit Camera Active.
 *
 * This characteristic allows HomeKit to turn off the IP camera accessory.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.131 HomeKit Camera Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_HomeKitCameraActive "homekit-camera-active"

extern const HAPUUID kHAPCharacteristicType_HomeKitCameraActive;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_HomeKitCameraActive) {
    /** Off. */
    kHAPCharacteristicValue_HomeKitCameraActive_Off = 0,

    /** On. */
    kHAPCharacteristicValue_HomeKitCameraActive_On = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_HomeKitCameraActive);
/**@}*/

/**
 * Third Party Camera Active.
 *
 * This characteristic indicates whether an IP camera accessory was turned off by an accessory app.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.132 Third Party Camera Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ThirdPartyCameraActive "third-party-camera-active"

extern const HAPUUID kHAPCharacteristicType_ThirdPartyCameraActive;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ThirdPartyCameraActive) {
    /** Off. */
    kHAPCharacteristicValue_ThirdPartyCameraActive_Off = 0,

    /** On. */
    kHAPCharacteristicValue_ThirdPartyCameraActive_On = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ThirdPartyCameraActive);
/**@}*/

/**
 * Camera Operating Mode Indicator.
 *
 * This characteristic allows enabling/disabling the visual (e.g., LED) indication on an IP camera accessory.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.133 Camera Operating Mode Indicator
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CameraOperatingModeIndicator "camera-operating-mode-indicator"

extern const HAPUUID kHAPCharacteristicType_CameraOperatingModeIndicator;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_CameraOperatingModeIndicator) {
    /** Disable. */
    kHAPCharacteristicValue_CameraOperatingModeIndicator_Disable = 0,

    /** Enable. */
    kHAPCharacteristicValue_CameraOperatingModeIndicator_Enable = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_CameraOperatingModeIndicator);
/**@}*/

/**
 * Wi-Fi Satellite Status.
 *
 * This characteristic specifies the status of the Wi-Fi satellite accessory.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 2
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.145 Wi‐Fi Satellite Status
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WiFiSatelliteStatus "wifi-satellite-status"

extern const HAPUUID kHAPCharacteristicType_WiFiSatelliteStatus;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiSatelliteStatus) {
    /** Wi-Fi Satellite status is unknown. */
    kHAPCharacteristicValue_WiFiSatelliteStatus_Unknown = 0,

    /** Wi-Fi Satellite is connected. */
    kHAPCharacteristicValue_WiFiSatelliteStatus_Connected = 1,

    /** Wi-Fi Satellite is not connected. */
    kHAPCharacteristicValue_WiFiSatelliteStatus_NotConnected = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiSatelliteStatus);
/**@}*/

/**
 * Network Access Violation Control.
 *
 * This characteristic allows a controller to retrieve information about network access violations committed
 * by network clients that match a network client profile, also known as managed network clients.
 *
 * Only admin controllers are allowed to perform operations on this characteristic;
 * access by non-admin controllers must result in HAP status code -70401 - "Insufficient Privileges".
 * The `Managed Network Enable` characteristic must be enabled to write to this characteristic;
 * otherwise, HAP status code -70412 - "Not allowed" is returned.
 *
 * At factory reset or when the last pairing is removed, all data associated with this characteristic
 * (e.g., access violation logs, stats, and timestamps) must be deleted.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: TLV
 * - Permissions: Paired Read, Paired Write, Write Response, Timed Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.144 Network Access Violation Control
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_NetworkAccessViolationControl "network-access-violation-control"

extern const HAPUUID kHAPCharacteristicType_NetworkAccessViolationControl;

/**
 * Network Access Violation Control Operation Type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_NetworkAccessViolationControl_OperationType) {
    /**
     * List.
     * Returns the network access violation attempt metadata for all managed network clients.
     * The write response returns a Network Access Violation TLV for each managed network client.
     */
    kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_List = 1,

    /**
     * Reset.
     * Resets the access violation data for all network clients listed in the Network Client Profile Identifier List.
     * This operation is not atomic: when any of the reset operations fail, a HAP error is returned.
     * Resetting a non-existent Network Client Profile Identifier will not return an error.
     * An empty TLV (0 bytes) is returned.
     */
    kHAPCharacteristicValue_NetworkAccessViolationControl_OperationType_Reset = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_NetworkAccessViolationControl_OperationType);

/**
 * Network Access Violation Control.
 */
typedef struct {
    /** The requested operation. */
    HAPCharacteristicValue_NetworkAccessViolationControl_OperationType operationType;

    /**
     * List of Network Client Profile Identifiers to be reset.
     * Must be set when Operation Type is set to Reset.
     */
    HAPCharacteristicValue_WiFiRouter_ClientList clientList;
    bool clientListIsSet;
} HAPCharacteristicValue_NetworkAccessViolationControl;

/**
 * Network Access Violation.
 */
typedef struct {
    /** Identifier of the network client profile that this network access violation attempt is describing. */
    uint32_t clientIdentifier;

    /**
     * Timestamp of last network access violation attempt.
     * Set only if a network access violation has occurred since it was last reset
     * or since the Network Client Profile Identifier was created.
     * UTC time in seconds since UNIX Epoch (00:00:00 Thursday, 1 January 1970).
     */
    uint64_t lastTimestamp;
    bool timestampIsSet;

    /**
     * Timestamp of last time the Reset operation was performed for this managed network client.
     * Set only if the reset operation has been called on this managed network client.
     * UTC time in seconds since UNIX Epoch (00:00:00 Thursday, 1 January 1970).
     */
    uint64_t resetTimestamp;
    bool resetIsSet;
} HAPCharacteristicValue_NetworkAccessViolationControl_Violation;

/**
 * Callback that should be invoked for each Network Access Violation.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_NetworkAccessViolationControl_Response)(
        void* _Nullable context,
        HAPCharacteristicValue_NetworkAccessViolationControl_Violation* value,
        bool* shouldContinue);

/**
 * Network Access Violation Control Response.
 */
typedef struct {
    /**
     * Enumerates all Network Access Violations.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_NetworkAccessViolationControl_Response callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_NetworkAccessViolationControl_Violation _;
} HAPCharacteristicValue_NetworkAccessViolationControl_Response;

/**
 * Network Access Violation Control Event.
 *
 * If notifications are configured, this TLV is sent to the controller to report network access violation changes.
 * The TLV is sent when a managed network client has a network access violation for the first time or when a managed
 * network client has a network access violation for the first time after the reset operation is performed.
 */
typedef struct {
    /**
     * Optional. This describes the list of Network Client Profile Identifiers that have new access violations.
     * Otherwise, an unknown set of new network access violations have changed.
     */
    HAPCharacteristicValue_WiFiRouter_ClientList clientList;
    bool clientListIsSet;
} HAPCharacteristicValue_NetworkAccessViolationControl_Event;
/**@}*/

/**
 * Current Transport.
 *
 * This characteristic specifies the current transport used by the accessory (WiFi or ethernet).
 *
 * This characteristic requires iOS x.
 *
 * - Format: Bool
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification -
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_CurrentTransport "current-transport-flag"

extern const HAPUUID kHAPCharacteristicType_CurrentTransport;

/**
 * Wi-Fi Capability.
 *
 * This characteristic describes the Wi-Fi Capability of the accessory.
 *
 * This characteristic requires iOS x.
 *
 * - Format: UInt32
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification -
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WiFiCapability "wifi-capability"

extern const HAPUUID kHAPCharacteristicType_WiFiCapability;

/**
 * Wi-Fi Capability Flags.
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_WiFiCapability) {
    /** Supports 2.4 GHz. */
    kHAPCharacteristicValue_WiFiCapability_Supports2_4GHz = 1U << 0U,

    /** Supports 5 GHz. */
    kHAPCharacteristicValue_WiFiCapability_Supports5GHz = 1U << 1U,

    /** Support Wake on WLAN */
    kHAPCharacteristicValue_WiFiCapability_SupportsWakeonWLAN = 1U << 2U,

    /** Supports Station Mode */
    kHAPCharacteristicValue_WiFiCapability_SupportsStationMode = 1U << 3U
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_WiFiCapability);
/**@}*/

/**
 * Wi-Fi Configuration Control.
 *
 * The configuration of the Wi-Fi transport is managed for an accessory using this characteristic.
 * This characteristic is a control point characteristic.
 * Only admin controllers are allowed to perform operations on this characteristic;
 * writes by non-admin controllers must result in the "Insufficient Privileges" (-70401) HAP status code.
 * If another update request is received while a Fail-Safe Update is already in progress, the accessory
 * must respond with a the "Resource is busy" (-70403) HAP status code.
 *
 * The accessory, upon receiving this command, accessory should re-configure Wi-Fi-only
 * IP accessories over their existing Wi-Fi connection.
 * If the operation type is simple, the accessory will try to use the new wifi configuration
 * regardless of success or failure. If the operation type is fail-safe, if the accessory
 * is unable to join the new Wi-Fi settings, it must attempt to return back to the original
 * Wi-Fi network settings.
 *
 * Note, this is distinct from reconfiguring Wi-Fi through WAC
 *
 * For Paired Read, an empty response is returned.
 *
 * This characteristic requires iOS x.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Paired Write, Write Response, Timed Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification -
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_WiFiConfigurationControl "wifi-configuration-control"

extern const HAPUUID kHAPCharacteristicType_WiFiConfigurationControl;
/**@}*/

/**
 * Product Data.
 *
 * This characteristic contains the 8 byte product data value assigned to each Product Plan on the MFi Portal
 * upon Product Plan submission.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: Data
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.146 Product Data
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ProductData "product-data"

extern const HAPUUID kHAPCharacteristicType_ProductData;
/**@}*/

/**@}*/

/**
 * Event Snapshots Active.
 *
 * This characteristic allows the controller to disable or enable event snapshots
 * (e.g., for motion and doorbell event notifications).
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.129 Event Snapshots Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_EventSnapshotsActive "event-snapshots-active"

extern const HAPUUID kHAPCharacteristicType_EventSnapshotsActive;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_EventSnapshotsActive) {
    /** Disable snapshots. */
    kHAPCharacteristicValue_EventSnapshotsActive_DisableSnapshots = 0,

    /** Enable snapshots. */
    kHAPCharacteristicValue_EventSnapshotsActive_EnableSnapshots = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_EventSnapshotsActive);
/**@}*/

/**
 * Periodic Snapshots Active.
 *
 * This characteristic allows the controller to disable or enable periodic snapshots
 * (e.g., for Home app preview images).
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.130 Periodic Snapshots Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_PeriodicSnapshotsActive "periodic-snapshots-active"

extern const HAPUUID kHAPCharacteristicType_PeriodicSnapshotsActive;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_PeriodicSnapshotsActive) {
    /** Disable snapshots. */
    kHAPCharacteristicValue_PeriodicSnapshotsActive_DisableSnapshots = 0,

    /** Enables snapshots. */
    kHAPCharacteristicValue_PeriodicSnapshotsActive_EnableSnapshots = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_PeriodicSnapshotsActive);
/**@}*/

/**
 * Diagonal Field of View.
 *
 * This characteristic describes the diagonal Field of View currently used on the camera.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: Float
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 360
 * - Unit: Arcdegrees
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.134 Diagonal Field of View
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_DiagonalFieldOfView "diagonal-fov"

extern const HAPUUID kHAPCharacteristicType_DiagonalFieldOfView;
/**@}*/

/**
 * Recording Audio Active.
 *
 * This characteristic enables the device to turn audio recording on or off.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Paired Write, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.135 Recording Audio Active
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_RecordingAudioActive "recording-audio-active"

extern const HAPUUID kHAPCharacteristicType_RecordingAudioActive;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_RecordingAudioActive) {
    /** Disable audio in recordings. */
    kHAPCharacteristicValue_RecordingAudioActive_DisableAudio = 0,

    /** Enable audio in recordings. */
    kHAPCharacteristicValue_RecordingAudioActive_EnableAudio = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_RecordingAudioActive);
/**@}*/

/**
 * Manually Disabled.
 *
 * This characteristic indicates that the service is turned off manually by means of a physical button on the camera.
 *
 * This characteristic requires iOS 13.2 or later.
 *
 * - Format: UInt8
 * - Permissions: Paired Read, Notify
 * - Minimum Value: 0
 * - Maximum Value: 1
 * - Step Value: 1
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.136 Manually Disabled
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ManuallyDisabled "manually-disabled"

extern const HAPUUID kHAPCharacteristicType_ManuallyDisabled;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ManuallyDisabled) {
    /** Manually Enabled. */
    kHAPCharacteristicValue_ManuallyDisabled_ManuallyEnabled = 0,

    /** Manually Disabled. */
    kHAPCharacteristicValue_ManuallyDisabled_ManuallyDisabled = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ManuallyDisabled);
/**@}*/

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiConfigurationControl_OperationType) {
    /** Invalid Configuration. */
    kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid = 0,

    /** Read Configuration. */
    kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read = 1,

    /** Simple Update */
    kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple = 2,

    /** Fail safe update */
    kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe = 3,

    /** Fail safe update - Commit Configuration*/
    kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiConfigurationControl_OperationType);

HAP_ENUM_BEGIN(uint32_t, HAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus) {

    /* The accessory is currently processing a Fail-Safe Update. */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Pending = 0x00010000,

    /* The HAP session that issued the request is using Wi-Fi and as such will have to go through a
     * disconnect / reconnect as part of the configuration update */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Restart_Required = 0x00020000,

    /* The update was applied and persisted successfully. Update Pending must be cleared when this flag is set. */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Success = 0x00040000,

    /* The updated configuration could not be applied. Update Pending must be cleared when this flag is set.
     * Additional error and progress flags should be set to allow the controller to diagnose which part of
     * the connection process failed. */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Failed = 0x00080000,

    /* The accessory failed to connect due to an invalid Wi-Fi credential (e.g. PSK). */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Auth_Failed = 0x00100000,

    /* This progress flag is set after the accessory successfully associates with the Wi-Fi network. */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Link_Established = 0x00200000,

    /* This progress flag is set after the accessory successfully obtains an IP address. */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Network_Configured = 0x00400000,

    /* This progress flag is set after a successful Pair Verify over Wi-Fi with the controller that originated the
     * update (only if Session Restart Required is set).  */
    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Connection_Verified = 0x00800000,

} HAP_ENUM_END(uint32_t, HAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus);

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode) {
    kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_None = 0,
    kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode);

typedef struct {
    char* ssid;
    bool ssidIsSet;

    uint8_t securityMode;
    bool securityModeIsSet;

    char* psk;
    bool pskIsSet;
} HAPCharacteristicValue_WiFiConfigurationControl_StationConfig;

/**
 * WiFi Configuration Control
 */
typedef struct {
    HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType;
    bool operationTypeIsSet;

    uint16_t cookie;
    bool cookieIsSet;

    uint32_t updateStatus;
    bool updateStatusIsSet;

    uint8_t operationTimeout;
    bool operationTimeoutIsSet;

    char* countryCode;
    bool countryCodeIsSet;

    HAPCharacteristicValue_WiFiConfigurationControl_StationConfig stationConfig;
    bool stationConfigIsSet;
} HAPCharacteristicValue_WiFiConfigurationControl;

/**
 * Firmware Update Readiness
 *
 * This characteristic allows the accessory to provide information about the readiness for firmware update.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Notify
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_FirmwareUpdateReadiness "firmware-update-readiness"

extern const HAPUUID kHAPCharacteristicType_FirmwareUpdateReadiness;

/**
 * Staging Not Ready Reason
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason) {
    /** Other (catch all) */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_Other = 1U << 0U,
    /** Low battery */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_LowBattery = 1U << 1U,
    /** Connectivity */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_Connectivity = 1U << 2U,
    /** Apply Needed */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReady_ApplyNeeded = 1U << 3U
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason);

/**
 * Update Not Ready Reason
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason) {
    /** Other (catch all) */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_Other = 1U << 0U,
    /** Low battery */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_LowBattery = 1U << 1U,
    /** Staged firmware update unavailable */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_StagedUnavailable = 1U << 2U,
    /** Critical operation in-progress */
    kHAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReady_CriticalOperation = 1U << 3U
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason);

/**
 * Firmware Update Readiness
 */
typedef struct {
    /** Optional. Staging not ready reason */
    uint32_t stagingNotReadyReason;
    bool stagingNotReadyReasonIsSet;

    /** Optional. Update not ready reason */
    uint32_t updateNotReadyReason;
    bool updateNotReadyReasonIsSet;
} HAPCharacteristicValue_FirmwareUpdateReadiness;

/**@}*/

/**
 * Firmware Update Status
 *
 * This characteristic provides the status of the firmware update process on the accessory.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Notify
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_FirmwareUpdateStatus "firmware-update-status"

extern const HAPUUID kHAPCharacteristicType_FirmwareUpdateStatus;

/**
 * Firmware Update State
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState) {
    /** Idle */
    kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_Idle = 0,
    /** Staging in-progress */
    kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingInProgress = 1,
    /** Staging paused */
    kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingPaused = 2,
    /** Staging succeeded */
    kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_StagingSucceeded = 3,
    /** Update in-progress */
    kHAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState_UpdateInProgress = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState);

/**
 * Firmware Update Status
 */
typedef struct {
    /** Firmware update state */
    uint8_t firmwareUpdateState;
    /** Update duration (seconds) */
    uint16_t updateDuration;
    /** Optional. Staged firmware version */
    const char* _Nullable stagedFirmwareVersion;
    bool stagedFirmwareVersionIsSet;
} HAPCharacteristicValue_FirmwareUpdateStatus;

/**@}*/

/**
 * Operating State Response.
 *
 * This characteristic describes the operating state of the accessory.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.171 Operating State Response
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_OperatingStateResponse "operating-state-response"

extern const HAPUUID kHAPCharacteristicType_OperatingStateResponse;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_OperatingStateResponse_State) {
    /** Normal State. */
    kHAPCharacteristicValue_OperatingStateResponse_State_Normal = 0,

    /** Limited Functionality. */
    kHAPCharacteristicValue_OperatingStateResponse_State_LimitedFunctionality = 1,

    /** Shutdown Imminent. */
    kHAPCharacteristicValue_OperatingStateResponse_State_ShutdownImminent = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_OperatingStateResponse_State);

HAP_OPTIONS_BEGIN(uint8_t, HAPCharacteristicValue_OperatingStateResponse_AbnormalReason) {
    /** Other reason. */
    kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_Other = 1U << 0U,

    /** Low Temperature. */
    kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_LowTemperature = 1U << 1U,

    /** High Temperature. */
    kHAPCharacteristicValue_OperatingStateResponse_AbnormalReason_HighTemperature = 1U << 2U,
} HAP_OPTIONS_END(uint8_t, HAPCharacteristicValue_OperatingStateResponse_AbnormalReason);

const char* _Nullable HAPCharacteristicValue_OperatingStateResponse_StateType_GetDescription(
        HAPCharacteristicValue_OperatingStateResponse_State value);
const char* _Nullable HAPCharacteristicValue_OperatingStateResponse_AbnormalReason_GetDescription(
        HAPCharacteristicValue_OperatingStateResponse_AbnormalReason value);

/**
 * Thread Management service OpenThread Version characteristic
 *
 * This characteristic allows HomeKit to read the OpenThread version supported by the accessory.
 *
 * This characteristic requires iOS XX.X or later.
 *
 * - Format: string
 * - Permissions: Paired Read, Admin-Only
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.155 OpenThread Version
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ThreadManagementOpenThreadVersion "thread-openthread-version"

extern const HAPUUID kHAPCharacteristicType_ThreadManagementOpenThreadVersion;

/**@}*/

/**
 * Thread Management service Node Capabilities characteristic
 *
 * This characteristic allows HomeKit to determine what roles the Thread device is capable of performing
 *
 * This characteristic requires iOS XX.X or later.
 *
 * - Format: string
 * - Permissions: Paired Read, Admin-Only
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.156 Node Capabilities
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ThreadManagementNodeCapabilities "thread-node-capabilities"

extern const HAPUUID kHAPCharacteristicType_ThreadManagementNodeCapabilities;

/**@}*/

/**
 * Thread Management service Thread Status characteristic
 *
 * This characteristic allows HomeKit to determine the Accessory's current assigned role in the Thread Network.
 *
 * This characteristic requires iOS XX.X or later.
 *
 * - Format: string
 * - Permissions: Paired Read, Admin-Only, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.157 Thread Role
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ThreadManagementStatus "thread-role"

extern const HAPUUID kHAPCharacteristicType_ThreadManagementStatus;

/**@}*/

/**
 * Thread Management service Thread Control characteristic
 *
 * This characteristic controls Thead commissioning process on the accessory.
 *
 * This characteristic requires iOS XX.X or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Write, Admin-Only
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.158 Thread Control Point
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ThreadManagementControl "thread-control-point"

extern const HAPUUID kHAPCharacteristicType_ThreadManagementControl;

/**
 * Thread Control Operation Type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_ThreadManagementControl_OperationType) {
    /** Set Thread Parameters  */
    kHAPCharacteristicValue_ThreadManagementControl_OperationType_SetThreadParameters = 1,
    /** Clear Thread Parameters */
    kHAPCharacteristicValue_ThreadManagementControl_OperationType_ClearThreadParameters = 2,
    /** Read Thread Parameters */
    kHAPCharacteristicValue_ThreadManagementControl_OperationType_ReadThreadParameters = 3,
    /** Initiate Thread Joiner */
    kHAPCharacteristicValue_ThreadManagementControl_OperationType_InitiateThreadJoiner = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_ThreadManagementControl_OperationType);

/**
 * Ext PAN ID
 */
typedef struct {
    void* bytes;     /**< Value buffer */
    size_t numBytes; /**< Length of value buffer */
} HAPCharacteristicValue_ThreadManagementControl_ExtPanId;

/**
 * Master Key
 */
typedef struct {
    void* bytes;     /**< Value buffer */
    size_t numBytes; /**< Length of value buffer */
} HAPCharacteristicValue_ThreadManagementControl_MasterKey;

/**
 * Thread Management - Thread Network Credentials TLV
 */
typedef struct {
    /** Network name */
    char* networkName;

    /** Channel */
    uint16_t channel;

    /** PAN ID */
    uint16_t panId;

    /** Ext PAN ID */
    HAPCharacteristicValue_ThreadManagementControl_ExtPanId extPanId;

    /** Master Key */
    HAPCharacteristicValue_ThreadManagementControl_MasterKey masterKey;
    bool masterKeyIsSet;
} HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials;

/**
 * Thread Management Control
 */
typedef struct {
    /** Operation Type */
    uint8_t operationType;
    bool operationTypeIsSet;

    /** Network credentials */
    HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials networkCredentials;
    bool networkCredentialsIsSet;

    /** Forming Allowed */
    uint8_t formingAllowed;
    bool formingAllowedIsSet;
} HAPCharacteristicValue_ThreadManagementControl;
/**@}*/

/**
 * ADK Version.
 *
 * This characteristic describes a ADK version string x[.y[.z]];b (e.g., "100.1.1;1A1").
 *
 * - \<x\> is the major version number, required.
 * - \<y\> is the minor version number, required if it is non-zero or if \<z\> is present.
 * - \<z\> is the revision version number, required if non-zero.
 * - \<b\> is the build version string, required.
 *
 * The characteristic value must change after every firmware update that uses a new ADK version.
 *
 * - Format: String
 * - Permissions: Paired Read
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_ADKVersion "adk-version"

extern const HAPUUID kHAPCharacteristicType_ADKVersion;
/**@}*/

#if HAP_TESTING
/**
 * Debug Command Characteristic types.
 *
 * These characteristics provide debugging capabilities similar to command line arguments.
 *
 * - Format: String
 * - Permissions: Paired Read, Paired Write, Timed Write
 *
 */
/**@{*/
extern const HAPUUID kAppCharacteristicType_ShortDebugCommand;
#define kAppCharacteristicDebugDescription_ShortDebugCommand "debug.command.short"

extern const HAPUUID kAppCharacteristicType_DebugCommand;
#define kAppCharacteristicDebugDescription_DebugCommand "debug.command"

#endif

/**
 * Sleep Interval.
 *
 * This characteristic provides interval in milliseconds for which accessory goes to sleep.
 *
 * - Format: Uint32
 * - Permissions: Paired Read, Notify
 *
 */
/**@{*/
extern const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationSleepInterval;
#define kHAPCharacteristicDebugDescription_AccessoryRuntimeInformationSleepInterval "sleep-interval"

/**
 * Ping.
 *
 * This characteristic used to assess reachability to the accessory
 *
 * - Format: Data
 * - Permissions: Paired Read
 *
 */
/**@{*/
extern const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationPing;
#define kHAPCharacteristicDebugDescription_AccessoryRuntimeInformationPing "ping"

/**
 * Enum for Ping Payload
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_AccessoryRuntimeInformation_Ping) {
    /** Video Recording. */
    kHAPCharacteristicValue_AccessoryRuntimeInformation_Ping_VideoRecording = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_AccessoryRuntimeInformation_Ping);

/**
 * Heart Beat.
 *
 * This characteristic value increments periodically.
 *
 * - Format: Uint32
 * - Permissions: Paired Read, Notify
 *
 */
/**@{*/
extern const HAPUUID kHAPCharacteristicType_AccessoryRuntimeInformationHeartBeat;
#define kHAPCharacteristicDebugDescription_AccessoryRuntimeInformationHeartBeat "heart-beat"

/**
 * Supported Diagnostics Snapshot.
 *
 * This characteristic describes what diagnostic snapshot is supported.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.172 Supported Diagnostics Snapshot
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedDiagnosticsSnapshot "supported-diagnostics-snapshot"

extern const HAPUUID kHAPCharacteristicType_SupportedDiagnosticsSnapshot;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedDiagnosticsSnapshot) {
    /** Diagnostic snapshot file format. */
    kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Format = 1,

    /** Diagnostic snapshot type. */
    kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Type = 2,

    /** Diagnostic options. */
    kHAPCharacteristicValue_SupportedDiagnosticsSnapshot_Options = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedDiagnosticsSnapshot);

/**
 * Supported Diagnostics Modes.
 *
 * This characteristic describes the available logging modes on the accessory.
 *
 * This characteristic requires iOS 14.x or later.
 *
 * - Format: uint32
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.173 Supported Diagnostics Modes
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedDiagnosticsModes "supported-diagnostics-modes"

extern const HAPUUID kHAPCharacteristicType_SupportedDiagnosticsModes;

/**
 * Bitmask for Supported Diagnostics Modes
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_SupportedDiagnosticsModes) {
    /** None. */
    kHAPCharacteristicValue_SupportedDiagnosticsModes_None = 0U,
    /** Verbose Logging. */
    kHAPCharacteristicValue_SupportedDiagnosticsModes_VerboseLogging = 1U << 1U,
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_SupportedDiagnosticsModes);

/**
 * Selected Diagnostics Modes.
 *
 * This characteristic describes the logging modes that are currently enabled.
 *
 * This characteristic requires iOS 14.x or later.
 *
 * - Format: uint32
 * - Permissions: Paired Read
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.174 Selected Diagnostics Modes
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SelectedDiagnosticsModes "selected-diagnostics-modes"

extern const HAPUUID kHAPCharacteristicType_SelectedDiagnosticsModes;

/**
 * Bitmask for Selected Diagnostics Modes
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_SelectedDiagnosticsModes) {
    /** None. */
    kHAPCharacteristicValue_SelectedDiagnosticsModes_None = 0U,
    /** Verbose Logging. */
    kHAPCharacteristicValue_SelectedDiagnosticsModes_VerboseLogging = 1U << 1U,
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_SelectedDiagnosticsModes);

/**
 * Supported Metrics.
 *
 * This characteristic describes what metrics types are supported.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Paired Write
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.178 Supported Metrics
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedMetrics "supported-metrics"

extern const HAPUUID kHAPCharacteristicType_SupportedMetrics;

/**
 * Supported metrics configuration type
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedMetricsConfiguration) {
    /** Configuration of the supported metrics - The value is a TLV8-encoded list. */
    kHAPCharacteristicValue_SupportedMetrics_Configuration = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedMetricsConfiguration);

/**
 * Supported metrics configuration settings
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_SupportedMetrics) {
    /** Metric ID. */
    kHAPCharacteristicValue_SupportedMetrics_MetricID = 1,

    /** Status. */
    kHAPCharacteristicValue_SupportedMetrics_Status = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_SupportedMetrics);

/**
 * Metrics Buffer Full State.
 *
 * This characteristic describes the metrics buffer full state.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: Bool
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.179 Metrics Buffer Full State
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_MetricsBufferFullState "metrics-buffer-full-state"

extern const HAPUUID kHAPCharacteristicType_MetricsBufferFullState;

/**
 * Variable Length Integer
 */
typedef struct {
    const void* bytes; /**< Value buffer */
    size_t numBytes;   /**< Length of Integer */
} HAPCharacteristicValue_VariableLengthInteger;

/**
 * Supported Characteristic Value Transition Configuration.
 *
 * This characteristic describes supported characteristic value transition configurations.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.148 Supported Characteristic Value Transition Configuration
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_SupportedTransitionConfiguration \
    "supported-characteristic-value-transition-configuration"

extern const HAPUUID kHAPCharacteristicType_SupportedTransitionConfiguration;

/**
 * Bitmask for Transition Types
 */
HAP_OPTIONS_BEGIN(uint32_t, HAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type) {
    /** Linear Transitions. */
    kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_Linear = 1U << 0U,
    /** Linear Derived Transitions. */
    kHAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type_LinearDerived = 1U << 1U,
} HAP_OPTIONS_END(uint32_t, HAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type);

typedef struct {
    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPCharacteristicValue_VariableLengthInteger HAPInstanceID;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_SupportedTransitionConfiguration_SupportedTransition_Type types;
} HAPCharacteristicValue_SupportedTransition;

/**
 * Callback that should be invoked for each Supported Transition Configuration.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_SupportedTransitions)(
        void* _Nullable context,
        HAPCharacteristicValue_SupportedTransition* value,
        bool* shouldContinue);

/**
 * Supported Characteristic Value Transition Configuration Type.
 */
typedef struct {
    /**
     * Enumerates all Supported Transition Configurations.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_SupportedTransitions callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_SupportedTransition transitions;
} HAPCharacteristicValue_SupportedTransitionConfigurations;

/**
 * Characteristic Value Transition Control.

 * This characteristic configures value transitions.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: TLV8
 * - Permissions: Paired Read, Paired Write, Write Response
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.147 Characteristic Value Transition Control
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TransitionControl "characteristic-value-transition-control"

extern const HAPUUID kHAPCharacteristicType_TransitionControl;

/**
 * Transition End Behavior
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TransitionControl_EndBehavior) {
    /** Continue with End Value */
    kHAPCharacteristicValue_TransitionControl_EndBehavior_NoChange = 0,
    /** Loop the Transition */
    kHAPCharacteristicValue_TransitionControl_EndBehavior_Loop = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TransitionControl_EndBehavior);

/**
 * Linear Transition Start Condition Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicValue_TransitionControl_StartCondition) {
    /** Always set calculated value */
    kHAPCharacteristicValue_TransitionControl_StartCondition_None = 0,
    /** Set calculated value if it is greater than threshold */
    kHAPCharacteristicValue_TransitionControl_StartCondition_Ascends = 1,
    /** Set calculated value if it is lesser than threshold */
    kHAPCharacteristicValue_TransitionControl_StartCondition_Descends = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicValue_TransitionControl_StartCondition);

/**
 * Controller Context
 */
typedef struct {
    void* bytes;     /**< Value buffer */
    size_t numBytes; /**< Length of value buffer */
} HAPCharacteristicValue_TransitionControl_ControllerContext;

/**
 * Linear Transition Points
 */
typedef struct {
    /** Target Value to reach at the completion of given Transition Point. */
    HAPCharacteristicValue_VariableLengthInteger targetValue;

    /** Relative Duration (in ms) to complete requested Transition Point. */
    HAPCharacteristicValue_VariableLengthInteger completionDuration;
    bool completionDurationIsSet;

    /** Relative Duration (in ms) to start requested Transition Point. */
    HAPCharacteristicValue_VariableLengthInteger startDelayDuration;
    bool startDelayDurationIsSet;
} HAPCharacteristicValue_TransitionControl_LinearTransition_Point;

/**
 * Callback that should be invoked for Linear Transition Point.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_TransitionControl_LinearTransition_Point)(
        void* _Nullable context,
        HAPCharacteristicValue_TransitionControl_LinearTransition_Point* value,
        bool* shouldContinue);

/**
 * Linear Transition Point
 */
typedef struct {
    /**
     * Enumerates all Linear Transition Points.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_TransitionControl_LinearTransition_Point callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Linear Transition Point. */
    HAPCharacteristicValue_TransitionControl_LinearTransition_Point point;
} HAPCharacteristicValue_TransitionControl_LinearTransition_PointList;

/**
 * Linear Transition.
 */
typedef struct {
    /** Linear Transition Points. */
    HAPCharacteristicValue_TransitionControl_LinearTransition_PointList points;
    /** Start Condition for Linear Transition. */
    HAPCharacteristicValue_TransitionControl_StartCondition startCondition;
} HAPCharacteristicValue_TransitionControl_LinearTransition;

/**
 * Linear Derived Transition Points
 */
typedef struct {
    /** Scale
     *
     * This indicates slope of the line showing relationship between Derived characteristic and source characteristic.
     * This value has float type defined as IEEE 754 floating point.
     */
    uint32_t scale;
    /** Offset
     *
     * This indicates intercept of the line showing relationship between Derived characteristic and source
     * characteristic. This value has float type defined as IEEE 754 floating point.
     */
    uint32_t offset;

    /** Relative Duration (in ms) to complete requested Transition Point. */
    HAPCharacteristicValue_VariableLengthInteger completionDuration;
    bool completionDurationIsSet;

    /** Relative Duration (in ms) to start requested Transition Point. */
    HAPCharacteristicValue_VariableLengthInteger startDelayDuration;
    bool startDelayDurationIsSet;
} HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point;

/**
 * Callback that should be invoked for Linear Derived Transition Point.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_TransitionControl_LinearDerivedTransition_Point)(
        void* _Nullable context,
        HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point* value,
        bool* shouldContinue);

/**
 * Derived Linear Transition Point
 */
typedef struct {
    /**
     * Enumerates all Linear Derived Transition Points.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_TransitionControl_LinearDerivedTransition_Point callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Linear Derived Transition Points */
    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point point;
} HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_PointList;

/**
 * Source Value Range.
 */
typedef struct {
    /** Lower bound */
    HAPCharacteristicValue_VariableLengthInteger lower;

    /** Upper bound */
    HAPCharacteristicValue_VariableLengthInteger upper;
} HAPCharacteristicValue_TransitionControl_SourceValueRange;

/**
 * Linear Derived Transition.
 */
typedef struct {
    /** Transition Points for given Linear Derived Transition */
    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_PointList points;

    /** HAP Instance ID for the source characteristic */
    HAPCharacteristicValue_VariableLengthInteger HAPInstanceID;

    /** Source Value Range */
    HAPCharacteristicValue_TransitionControl_SourceValueRange range;
} HAPCharacteristicValue_TransitionControl_LinearDerivedTransition;

/**
 * Transition Control Fetch.
 */
typedef struct {
    /** HAP Instance ID for fetch request */
    HAPCharacteristicValue_VariableLengthInteger HAPInstanceID;
} HAPCharacteristicValue_TransitionControl_Fetch;

/**
 * Adaptive Light Transition.
 */
typedef struct {
    /** HAP Instance ID of the characteristic for given transition. */
    HAPCharacteristicValue_VariableLengthInteger HAPInstanceID;

    /** Controller Context to be stored by accessory. */
    HAPCharacteristicValue_TransitionControl_ControllerContext context;
    bool contextIsSet;

    /** Expected Behavior at the end of Transition */
    HAPCharacteristicValue_TransitionControl_EndBehavior endBehavior;

    /** Descriptor for Linear Transition */
    HAPCharacteristicValue_TransitionControl_LinearTransition linearTransition;
    bool linearTransitionIsSet;

    /** Descriptor for Linear Derived Transition */
    HAPCharacteristicValue_TransitionControl_LinearDerivedTransition linearDerivedTransition;
    bool linearDerivedTransitionIsSet;

    /** Value Update Time Interval */
    HAPCharacteristicValue_VariableLengthInteger updateInterval;
    bool updateIntervalIsSet;

    /** Notify Value Change Threshold */
    HAPCharacteristicValue_VariableLengthInteger notifyValueChangeThreshold;
    bool notifyValueChangeThresholdIsSet;

    /** Notify Time Interval Threshold */
    HAPCharacteristicValue_VariableLengthInteger notifyTimeIntervalThreshold;
    bool notifyTimeIntervalThresholdIsSet;
} HAPCharacteristicValue_TransitionControl_Transition;

/**
 * Callback that should be invoked for each Transition.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_TransitionControl_Transition)(
        void* _Nullable context,
        HAPCharacteristicValue_TransitionControl_Transition* value,
        bool* shouldContinue);

/**
 * Transition to start.
 */
typedef struct {
    /**
     * Enumerates all Transitions to start.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_TransitionControl_Transition callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_TransitionControl_Transition transitions;
} HAPCharacteristicValue_TransitionControl_Start;

/**
 * Transition Control.
 */
typedef struct {
    /** Transition Control- Fetch command. */
    HAPCharacteristicValue_TransitionControl_Fetch fetch;
    bool fetchIsSet;

    /** Transition Control- Start command. */
    HAPCharacteristicValue_TransitionControl_Start start;
    bool startIsSet;
} HAPCharacteristicValue_TransitionControl;

/**
 * Transition Control State Active Context.
 */
typedef struct {
    /** HAP Instance ID of the characteristic. */
    HAPCharacteristicValue_VariableLengthInteger HAPInstanceID;

    /** Controller Context set. */
    HAPCharacteristicValue_TransitionControl_ControllerContext context;

    /** Time elapsed since Transition was set. */
    HAPCharacteristicValue_VariableLengthInteger timeElapsedSinceStart;
} HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext;

/**
 * Callback that should be invoked for Active Transition Context.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_TransitionState_ActiveContext)(
        void* _Nullable context,
        HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext* value,
        bool* shouldContinue);

/**
 * Transition State
 */
typedef struct {
    /**
     * Enumerates all Active Transition Contexts.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_TransitionState_ActiveContext callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;
    HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext activeContext;
} HAPCharacteristicValue_TransitionState;

/**
 * Transition Control Response.
 */
typedef struct {
    /** Response to Fetch request. */
    HAPCharacteristicValue_TransitionControl_Transition transition;
    bool transitionIsSet;

    /** Response to start request. */
    HAPCharacteristicValue_TransitionState state;
    bool stateIsSet;
} HAPCharacteristicValue_TransitionControlResponse;

/**
 * Characteristic Value Active Transition Count.

 * This characteristic provides the total number of characteristics that have active transitions.
 *
 * This characteristic requires iOS 14.0 or later.
 *
 * - Format: UInt32
 * - Permissions: Paired Read, Notify
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.149 Characteristic Value Active Transition Count
 */
/**@{*/
#define kHAPCharacteristicDebugDescription_TransitionCount "characteristic-value-active-transition-count"

extern const HAPUUID kHAPCharacteristicType_TransitionCount;

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
