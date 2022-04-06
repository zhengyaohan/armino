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

#ifndef HAP_BASE_H
#define HAP_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

// C library header files that are also available in freestanding environments (-ffreestanding).
#include <float.h>
#include <iso646.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "HAPBase+CompilerAbstraction.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Gets the number of elements in an array.
 *
 * - This only works before the array is passed to other C functions!
 *
 * @param      array                Array.
 *
 * @return Number of elements in the array.
 */
#define HAPArrayCount(array) ((size_t)(sizeof(array) / sizeof((array)[0])))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns the lesser of two comparable values.
 *
 * @param      x                    A value to compare.
 * @param      y                    Another value to compare.
 *
 * @return The lesser of @p x and @p y. If @p x is equal to @p y, returns @p x.
 */
#define HAPMin(x, y) (((x) <= (y)) ? (x) : (y))

/**
 * Returns the greater of two comparable values.
 *
 * @param      x                    A value to compare.
 * @param      y                    Another value to compare.
 *
 * @return The greater of @p x and @p y. If @p x is equal to @p y, returns @p y.
 */
#define HAPMax(x, y) (((x) <= (y)) ? (y) : (x))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Sort order.
 */
HAP_ENUM_BEGIN(uint8_t, HAPComparisonResult) { /** The left operand is smaller than the right operand. */
                                               kHAPComparisonResult_OrderedAscending = 1,

                                               /** The two operands are equal. */
                                               kHAPComparisonResult_OrderedSame,

                                               /** The left operand is greater than the right operand. */
                                               kHAPComparisonResult_OrderedDescending
} HAP_ENUM_END(uint8_t, HAPComparisonResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Error type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPError) {
    kHAPError_None,           /**< No error occurred. */
    kHAPError_Unknown,        /**< Unknown error. */
    kHAPError_InvalidState,   /**< Operation is not supported in current state. */
    kHAPError_InvalidData,    /**< Data has unexpected format. */
    kHAPError_OutOfResources, /**< Out of resources. */
    kHAPError_NotAuthorized,  /**< Insufficient authorization. */
    kHAPError_Busy            /**< Operation failed temporarily, retry later. */
} HAP_ENUM_END(uint8_t, HAPError);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * System time expressed as milliseconds relative to an implementation-defined time in the past.
 */
typedef uint64_t HAPTime;

/**
 * Time interval in nanoseconds.
 */
typedef uint64_t HAPTimeNS;

/**
 * 1 millisecond in milliseconds.
 */
#define HAPMillisecond ((HAPTime) 1)

/**
 * 1 second in milliseconds.
 */
#define HAPSecond ((HAPTime)(1000 * HAPMillisecond))

/**
 * 1 minute in milliseconds.
 */
#define HAPMinute ((HAPTime)(60 * HAPSecond))

/**
 * 1 hour in milliseconds.
 */
#define HAPHour ((HAPTime)(60 * HAPMinute))

/**
 * The maximum HAPTime value. Used for sentinel.
 */
#define HAPTime_Max ((HAPTime)(-1))

/**
 * Index of a service with a given type within an attribute database.
 *
 * - This is a zero-based index specific to the service type.
 *
 * - This is useful to compress a (HAPAccessory, HAPService) tuple.
 *   Use HAPAccessoryServerGetServiceFromServiceTypeIndex to fetch the (HAPAccessory, HAPService) tuple.
 *   There can be at most 150 accessories with 100 services each. 150 * 100 = 15000 -> UInt16.
 */
typedef uint16_t HAPServiceTypeIndex;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *  The maximum number of authentication attempts
 */
#define HAPAuthAttempts_Max 100

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * RSSI signal strength.
 */
typedef int8_t HAPRSSI;

/** Length of a MAC address. */
#define kHAPMACAddress_NumBytes ((size_t) 6)

/**
 * MAC address.
 */
typedef struct {
    /** Network byte order (big-endian). */
    uint8_t bytes[kHAPMACAddress_NumBytes];
} HAPMACAddress;
HAP_STATIC_ASSERT(sizeof(HAPMACAddress) == kHAPMACAddress_NumBytes, HAPMACAddress);

/**
 * Gets the string representation of the given MAC address.
 *
 * @param      value                Value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMACAddressGetDescription(const HAPMACAddress* value, char* bytes, size_t maxBytes);

/**
 * Returns whether or not two MAC addresses are equal.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return true                     If both values are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPMACAddressAreEqual(const HAPMACAddress* value, const HAPMACAddress* otherValue);

/**
 * Creates a new MAC address value from the given string.
 *
 * - The string must be in format AA:BB:CC:DD:EE:FF with upper and lower hex digits being allowed.
 *
 * @param      description          The ASCII representation of a MAC address. NULL-terminated.
 * @param[out] value                MAC address value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format.
 */
HAP_RESULT_USE_CHECK
HAPError HAPMACAddressFromString(const char* description, HAPMACAddress* value);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define kEUI64Address_NumBytes ((size_t) 8)

/**
 * EUI 64
 */
typedef struct {
    uint8_t bytes[kEUI64Address_NumBytes];
} HAPEui64;
HAP_STATIC_ASSERT(sizeof(HAPEui64) == kEUI64Address_NumBytes, HAPEui64);

/**
 * Gets the string representation of the given EUI.
 *
 * @param      value                Value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPEui64GetDescription(const HAPEui64* value, char* bytes, size_t maxBytes);

/**
 * Returns whether or not two EUI64s are equal.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return true                     If both values are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPEui64sAreEqual(const HAPEui64* value, const HAPEui64* otherValue);

/**
 * Creates a new EUI64 value from the given string.
 *
 * - The string must be in format AA:BB:CC:DD:EE:FF:GG:HH with
 *   upper and lower hex digits being allowed.
 *
 * @param      description          The ASCII representation of
 *                                  an EUI64. NULL-terminated.
 * @param[out] value                EUI64 value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format.
 */
HAP_RESULT_USE_CHECK
HAPError HAPEui64FromString(const char* description, HAPEui64* value);

/**
 * Reads the Platform Specific EUI.
 *
 * @param[out] value                EUI value.
 *
 */
void HAPPlatformReadEui(HAPEui64* value);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IP address version.
 */
HAP_ENUM_BEGIN(uint8_t, HAPIPAddressVersion) { /** IPv4. */
                                               kHAPIPAddressVersion_IPv4 = 1,

                                               /** IPv6. */
                                               kHAPIPAddressVersion_IPv6
} HAP_ENUM_END(uint8_t, HAPIPAddressVersion);

/**
 * Determines whether a value represents a valid IP address version.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPAddressVersionIsValid(HAPIPAddressVersion value);

/** Length of an IPv4 address. */
#define kHAPIPv4Address_NumBytes ((size_t) 4)

/**
 * IPv4 address.
 */
typedef struct {
    /** IP address in network byte order. */
    uint8_t bytes[kHAPIPv4Address_NumBytes];
} HAPIPv4Address;
HAP_STATIC_ASSERT(sizeof(HAPIPv4Address) == kHAPIPv4Address_NumBytes, HAPIPv4Address);

/**
 * Gets the string representation of the given IPv4 address.
 *
 * @param      value                Value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPv4AddressGetDescription(const HAPIPv4Address* value, char* bytes, size_t maxBytes);

/**
 * Indicates whether an IPv4 address is multicast.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is a multicast IPv4 address.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPv4AddressIsMulticast(const HAPIPv4Address* value);

/**
 * Indicates the ordering of two IPv4 addresses.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return Sort order.
 */
HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPv4AddressCompare(const HAPIPv4Address* value, const HAPIPv4Address* otherValue);

/**
 * Returns whether or not two IPv4 addresses are equal.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return true                     If both values are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPv4AddressAreEqual(const HAPIPv4Address* value, const HAPIPv4Address* otherValue);

/** Length of an IPv4 address. */
#define kHAPIPv6Address_NumBytes ((size_t) 16)

/**
 * IPv6 address.
 */
typedef struct {
    /** IP address in network byte order. */
    uint8_t bytes[kHAPIPv6Address_NumBytes];
} HAPIPv6Address;
HAP_STATIC_ASSERT(sizeof(HAPIPv6Address) == kHAPIPv6Address_NumBytes, HAPIPv6Address);

/**
 * Gets the string representation of the given IPv6 address.
 *
 * @param      value                Value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPv6AddressGetDescription(const HAPIPv6Address* value, char* bytes, size_t maxBytes);

/**
 * Indicates whether an IPv6 address is multicast.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is a multicast IPv6 address.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPv6AddressIsMulticast(const HAPIPv6Address* value);

/**
 * Indicates the ordering of two IPv6 addresses.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return Sort order.
 */
HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPv6AddressCompare(const HAPIPv6Address* value, const HAPIPv6Address* otherValue);

/**
 * Returns whether or not two IPv6 addresses are equal.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return true                     If both values are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPv6AddressAreEqual(const HAPIPv6Address* value, const HAPIPv6Address* otherValue);

/**
 * IP address.
 */
typedef struct {
    /** IP address version. */
    HAPIPAddressVersion version;

    /** Version specific parameters. */
    union {
        /** IPv4. */
        HAPIPv4Address ipv4;

        /** IPv6. */
        HAPIPv6Address ipv6;
    } _;
} HAPIPAddress;

/** IPv4 any address. */
extern const HAPIPAddress kHAPIPAddress_IPv4Any;

/** IPv4 broadcast address. */
extern const HAPIPAddress kHAPIPAddress_IPv4Broadcast;

/** IPv6 any address. */
extern const HAPIPAddress kHAPIPAddress_IPv6Any;

/**
 * Determines whether a value represents a valid IP address.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPAddressIsValid(const HAPIPAddress* value);

/**
 * Gets the string representation of the given IP address.
 *
 * @param      value                Value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPAddressGetDescription(const HAPIPAddress* value, char* bytes, size_t maxBytes);

/**
 * Indicates whether an IP address is multicast.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is a multicast IP address.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPAddressIsMulticast(const HAPIPAddress* value);

/**
 * Indicates the ordering of two IP addresses.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return Sort order.
 */
HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPAddressCompare(const HAPIPAddress* value, const HAPIPAddress* otherValue);

/**
 * Returns whether or not two IP addresses are equal.
 *
 * @param      value                Value to compare.
 * @param      otherValue           Value to compare with.
 *
 * @return true                     If both values are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPAddressAreEqual(const HAPIPAddress* value, const HAPIPAddress* otherValue);

/**
 * Creates a new IP address value from the given string.
 *
 * - The string must be in format XXX.XXX.XXX.XXX for IPv4 or XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX for IPv6.
 *
 * @param      description          The ASCII representation of an IP address. NULL-terminated.
 * @param[out] value                IP address value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPAddressFromString(const char* description, HAPIPAddress* value);

/**
 * IPv4 address range.
 */
typedef struct {
    /** Start of range, inclusive. */
    HAPIPv4Address startAddress;

    /** End of range, inclusive. */
    HAPIPv4Address endAddress;
} HAPIPv4AddressRange;

/**
 * IPv6 address range.
 */
typedef struct {
    /** Start of range, inclusive. */
    HAPIPv6Address startAddress;

    /** End of range, inclusive. */
    HAPIPv6Address endAddress;
} HAPIPv6AddressRange;

/**
 * IP address range.
 */
typedef struct {
    /** IP address version. */
    HAPIPAddressVersion version;

    /** Version specific parameters. */
    union {
        /** IPv4. */
        HAPIPv4AddressRange ipv4;

        /** IPv6. */
        HAPIPv6AddressRange ipv6;
    } _;
} HAPIPAddressRange;

/**
 * Creates an IP address range.
 *
 * - Start and end of range must have the same IP address version.
 *   End of range must be equal or be after start of range.
 *
 * @param[out] ipAddressRange       IP address range.
 * @param      startAddress         Start of range, inclusive.
 * @param      endAddress           End of range, inclusive.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If end of range before start of range or non-matching IP address versions.
 */
HAP_RESULT_USE_CHECK
HAPError HAPIPAddressRangeCreate(
        HAPIPAddressRange* ipAddressRange,
        const HAPIPAddress* startAddress,
        const HAPIPAddress* endAddress);

/**
 * Determines whether a value represents a valid IP address range.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPIPAddressRangeIsValid(const HAPIPAddressRange* value);

/**
 * Network port.
 */
typedef uint16_t HAPNetworkPort;

/**
 * Any network port.
 */
#define kHAPNetworkPort_Any ((HAPNetworkPort) 0)

/**
 * The number of data streams.
 */
#define kApp_NumDataStreams ((size_t) 4)

/**
 * Network port range.
 */
typedef struct {
    /** Start of range, inclusive. */
    HAPNetworkPort startPort;

    /** End of range, inclusive. */
    HAPNetworkPort endPort;
} HAPNetworkPortRange;

/**
 * Determines whether a value represents a valid network port range.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPNetworkPortRangeIsValid(const HAPNetworkPortRange* value);

/**
 * Stream priority.
 *
 * These are arbitrary priority but higher value is considered to be of higher priority
 */
HAP_ENUM_BEGIN(uint8_t, HAPStreamPriority) {
    kHAPStreamPriority_NoPriority = 0,
    kHAPStreamPriority_High = 5,
} HAP_ENUM_END(uint8_t, HAPStreamPriority);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Minimum length of a WPA/WPA2 Personal passphrase. */
#define kHAPWiFiWPAPassphrase_MinBytes ((size_t) 8)

/** Maximum length of a WPA/WPA2 Personal passphrase. */
#define kHAPWiFiWPAPassphrase_MaxBytes ((size_t) 63)

/**
 * WPA/WPA2 Personal passphrase.
 */
typedef struct {
    /** NULL-terminated. */
    char stringValue[kHAPWiFiWPAPassphrase_MaxBytes + 1];
} HAPWiFiWPAPassphrase;

/**
 * Length of a WPA/WPA2 Personal PSK.
 */
#define kHAPWiFiWPAPSK_NumBytes ((size_t) 32)

/**
 * WPA/WPA2 Personal PSK.
 */
typedef struct {
    /** Value. */
    uint8_t bytes[kHAPWiFiWPAPSK_NumBytes];
} HAPWiFiWPAPSK;

/**
 * WPA/WPA2 Personal credential type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPWiFiWPAPersonalCredentialType) { /** Passphrase. */
                                                            kHAPWiFiWPAPersonalCredentialType_Passphrase = 1,

                                                            /** Pre-hashed key. */
                                                            kHAPWiFiWPAPersonalCredentialType_PSK
} HAP_ENUM_END(uint8_t, HAPWiFiWPAPersonalCredentialType);

/**
 * WPA/WPA2 Personal credential.
 */
typedef struct {
    /** Credential type. */
    HAPWiFiWPAPersonalCredentialType type;

    /** Type specific parameters. */
    union {
        /** Passphrase. */
        HAPWiFiWPAPassphrase passphrase;

        /** Pre-hashed key. */
        HAPWiFiWPAPSK psk;
    } _;
} HAPWiFiWPAPersonalCredential;

/**
 * Computes the WPA PSK from an ASCII passphrase for a SSID.
 *
 * - Passphrase must be valid.
 *
 * @param[out] psk                  WPA/WPA2 personal PSK.
 * @param      ssid                 SSID of the WPA/WPA2 personal Wi-Fi network.
 * @param      passphrase           The network's passphrase credential: 8-63 printable ASCII characters.
 */
void HAPWiFiGetWPAPSKForPassphrase(
        uint8_t psk[_Nonnull kHAPWiFiWPAPSK_NumBytes],
        const char* ssid,
        const char* passphrase);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Setup info.
 */
typedef struct {
    uint8_t salt[16];      /**< SRP salt. */
    uint8_t verifier[384]; /**< SRP verifier. */
} HAPSetupInfo;
HAP_STATIC_ASSERT(sizeof(HAPSetupInfo) == 400, HAPSetupInfo);

/**
 * NULL-terminated setup ID string (format: XXXX).
 */
typedef struct {
    char stringValue[4 + 1]; /**< NULL-terminated. */
} HAPSetupID;
HAP_STATIC_ASSERT(sizeof(HAPSetupID) == 5, HAPSetupID);

/**
 * NULL-terminated setup code string (format: XXX-XX-XXX).
 */
typedef struct {
    char stringValue[10 + 1]; /**< NULL-terminated. */
} HAPSetupCode;
HAP_STATIC_ASSERT(sizeof(HAPSetupCode) == 11, HAPSetupCode);

/**
 * NULL-terminated setup payload string.
 */
typedef struct {
    char stringValue[38 + 1]; /**< NULL-terminated. */
} HAPSetupPayload;
HAP_STATIC_ASSERT(sizeof(HAPSetupPayload) == 39, HAPSetupPayload);

/**
 * NULL-terminated joiner passphrase string
 */
typedef struct {
    char stringValue[32 + 1];
} HAPJoinerPassphrase;
HAP_STATIC_ASSERT(sizeof(HAPJoinerPassphrase) == 33, HAPJoinerPassphrase);

/**
 * 32 bit product number
 */
typedef struct {
    uint8_t bytes[4];
} HAPProductNumber;
HAP_STATIC_ASSERT(sizeof(HAPProductNumber) == 4, HAPProductNumber);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Advertising interval for Bluetooth LE.
 *
 * Unit: 0.625 ms
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 2 Part E Section 7.8.5 LE Set Advertising Parameters Command
 */
typedef uint16_t HAPBLEAdvertisingInterval;

/**
 * Converts an advertising interval in milliseconds to an advertising interval for Bluetooth LE.
 *
 * @param      milliseconds         Advertising interval in milliseconds.
 *
 * @return Advertising interval for Bluetooth LE.
 */
#define HAPBLEAdvertisingIntervalCreateFromMilliseconds(milliseconds) \
    ((HAPBLEAdvertisingInterval)((milliseconds) / (0.625F)))

/**
 * Converts an advertising interval for Bluetooth LE to an advertising interval in milliseconds.
 *
 * @param      advertisingInterval  Advertising interval for Bluetooth LE.
 *
 * @return Advertising interval in milliseconds.
 */
#define HAPBLEAdvertisingIntervalGetMilliseconds(advertisingInterval) \
    ((advertisingInterval) * (0.625F * HAPMillisecond))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Camera recording event trigger types.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPCameraEventTriggerTypes) { /** Motion. */
                                                         kHAPCameraEventTriggerTypes_Motion = 1U << 0U,

                                                         /** Doorbell. */
                                                         kHAPCameraEventTriggerTypes_Doorbell = 1U << 1U
} HAP_OPTIONS_END(uint8_t, HAPCameraEventTriggerTypes);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Media container type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPMediaContainerType) { /** Fragmented MP4. */
                                                 kHAPMediaContainerType_FragmentedMP4 = 1
} HAP_ENUM_END(uint8_t, HAPMediaContainerType);

/**
 * Media container parameters.
 */
typedef void HAPMediaContainerParameters;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fragmented MP4 media container parameters.
 */
typedef struct {
    /** MP4 fragment duration in milliseconds. */
    uint32_t fragmentDuration;
} HAPFragmentedMP4MediaContainerParameters;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Supported media container configuration.
 */
typedef struct {
    /** Media container type. */
    HAPMediaContainerType containerType;

    /** Media container parameters. */
    const HAPMediaContainerParameters* containerParameters;
} HAPCameraSupportedMediaContainerConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Video codec type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPVideoCodecType) {
    kHAPVideoCodecType_H264 = 1, /**< H.264. */
} HAP_ENUM_END(uint8_t, HAPVideoCodecType);

/**
 * Video codec parameters.
 */
typedef void HAPVideoCodecParameters;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Type of H.264 Profile.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPH264VideoCodecProfile) { /** Constrained Baseline Profile. */
                                                       kHAPH264VideoCodecProfile_ConstrainedBaseline = 1U << 0U,

                                                       /** Main Profile. */
                                                       kHAPH264VideoCodecProfile_Main = 1U << 1U,

                                                       /** High Profile. */
                                                       kHAPH264VideoCodecProfile_High = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPH264VideoCodecProfile);

/**
 * H.264 Profile support level.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPH264VideoCodecProfileLevel) { /** 3.1. */
                                                            kHAPH264VideoCodecProfileLevel_3_1 = 1U << 0U,

                                                            /** 3.2. */
                                                            kHAPH264VideoCodecProfileLevel_3_2 = 1U << 1U,

                                                            /** 4. */
                                                            kHAPH264VideoCodecProfileLevel_4 = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPH264VideoCodecProfileLevel);

/**
 * H.264 packetization mode.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPH264VideoCodecPacketizationMode) {
    /** Non-interleaved mode. */
    kHAPH264VideoCodecPacketizationMode_NonInterleaved = 1U << 0U
} HAP_OPTIONS_END(uint8_t, HAPH264VideoCodecPacketizationMode);

/**
 * H.264 parameters.
 */
typedef struct {
    /** Type(s) of H.264 Profile. */
    HAPH264VideoCodecProfile profile;

    /** Profile support level. */
    HAPH264VideoCodecProfileLevel level;

    /** Packetization mode(s). */
    HAPH264VideoCodecPacketizationMode packetizationMode;

    /**
     * Maximum target bit rate (kbit/s).
     * Used for Selected Camera Recording Parameters only.
     * Set to 0 if unused.
     */
    uint32_t bitRate;

    /**
     * Maximum requested I-Frame interval (ms).
     * Used for Selected Camera Recording Parameters only.
     * Set to 0 if unused.
     */
    uint32_t iFrameInterval;
} HAPH264VideoCodecParameters;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Video attributes.
 */
typedef struct {
    uint16_t width;       /**< Image width in pixels. */
    uint16_t height;      /**< Image height in pixels. */
    uint8_t maxFrameRate; /**< Maximum frame rate. */
} HAPVideoAttributes;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Minimum supported frame rate for any video stream.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 12.4.3 Multiple Camera RTP Stream Management
 */
#define kHAPCameraSupportedVideoCodecConfiguration_MinFrameRate ((uint8_t) 24)

/**
 * Minimum recommended frame rate for any video stream.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 12.4.3 Multiple Camera RTP Stream Management
 */
#define kHAPCameraSupportedVideoCodecConfiguration_RecommendedFrameRate ((uint8_t) 30)

/**
 * Supported video codec configuration.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 12.4.3 Multiple Camera RTP Stream Management
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 12.4.4 Media Codecs
 */
typedef struct {
    /** Video codec type. */
    HAPVideoCodecType codecType;

    /** Video codec parameters. */
    const HAPVideoCodecParameters* codecParameters;

    /** Video attributes. */
    const HAPVideoAttributes* _Nullable const* _Nullable attributes;
} HAPCameraSupportedVideoCodecConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Audio codec type.
 */
HAP_ENUM_BEGIN(uint8_t, HAPAudioCodecType) {
    kHAPAudioCodecType_PCMU = 1,    /**< PCMU. */
    kHAPAudioCodecType_PCMA = 2,    /**< PCMA. */
    kHAPAudioCodecType_AAC_ELD = 3, /**< AAC-ELD. */
    kHAPAudioCodecType_AAC_LC = 8,  /**< AAC-LC. */
    kHAPAudioCodecType_Opus = 4,    /**< Opus. */
    kHAPAudioCodecType_MSBC = 5,    /**< MSBC. */
    kHAPAudioCodecType_AMR = 6,     /**< AMR. */
    kHAPAudioCodecType_AMR_WB = 7   /**< AMR-WB. */
} HAP_ENUM_END(uint8_t, HAPAudioCodecType);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Audio codec bit rate control mode.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPAudioCodecBitRateControlMode) { /** Variable bit-rate. */
                                                              kHAPAudioCodecBitRateControlMode_Variable = 1U << 0U,

                                                              /** Constant bit-rate. */
                                                              kHAPAudioCodecBitRateControlMode_Constant = 1U << 1U
} HAP_OPTIONS_END(uint8_t, HAPAudioCodecBitRateControlMode);

/**
 * Audio codec sample rate.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPAudioCodecSampleRate) { /** 8 KHz. */
                                                      kHAPAudioCodecSampleRate_8KHz = 1U << 0U,

                                                      /** 16 KHz. */
                                                      kHAPAudioCodecSampleRate_16KHz = 1U << 1U,

                                                      /** 24 KHz. */
                                                      kHAPAudioCodecSampleRate_24KHz = 1U << 2U,

                                                      /** 32 KHz. */
                                                      kHAPAudioCodecSampleRate_32KHz = 1U << 3U,

                                                      /** 44.1 KHz. */
                                                      kHAPAudioCodecSampleRate_44_1KHz = 1U << 4U,

                                                      /** 48 KHz. */
                                                      kHAPAudioCodecSampleRate_48KHz = 1U << 5U
} HAP_OPTIONS_END(uint8_t, HAPAudioCodecSampleRate);

/**
 * Audio codec parameters.
 */
typedef struct {
    /** Number of audio channels. */
    uint8_t numberOfChannels;

    /** Bit rate control mode. */
    HAPAudioCodecBitRateControlMode bitRateMode;

    /** Sample rate. */
    HAPAudioCodecSampleRate sampleRate;

    /**
     * Maximum target bit rate (kbit/s).
     * Used for Selected Camera Recording Parameters only.
     * Set to 0 if unused.
     */
    uint32_t bitRate;
} HAPAudioCodecParameters;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Supported audio codec configuration.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 12.4.4 Media Codecs
 */
typedef struct {
    /** Audio codec type. */
    HAPAudioCodecType codecType;

    /** Audio codec parameters. */
    const HAPAudioCodecParameters* codecParameters;
} HAPCameraSupportedAudioCodecConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * SRTP crypto suite.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPSRTPCryptoSuite) {
    /** AES_CM_128_HMAC_SHA1_80. */
    kHAPSRTPCryptoSuite_AES_CM_128_HMAC_SHA1_80 = 1U << 0U,

    /** AES_256_CM_HMAC_SHA1_80. */
    kHAPSRTPCryptoSuite_AES_256_CM_HMAC_SHA1_80 = 1U << 1U,

    /**
     * Disabled.
     *
     * - This crypto suite is not supported on certain non-internal non-developer versions of iOS.
     *   If required, consider trying to test on a pre-release iOS Beta version.
     */
    kHAPSRTPCryptoSuite_Disabled = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPSRTPCryptoSuite);
//----------------------------------------------------------------------------------------------------------------------

/**
 * Collection of supported configurations for an individual IP Camera stream.
 */
typedef struct {
    /** Supported video parameters. */
    struct {
        /** Array of supported video codec configurations. NULL-terminated. */
        const HAPCameraSupportedVideoCodecConfiguration* _Nullable const* _Nullable configurations;
    } videoStream;

    /** Supported audio parameters. */
    struct {
        /** Array of supported audio codec configurations. NULL-terminated. */
        const HAPCameraSupportedAudioCodecConfiguration* _Nullable const* _Nullable configurations;

        /** Comfort Noise Codec parameters. */
        struct {
            /** Whether Comfort Noise Codec is supported. */
            bool supported;
        } comfortNoise;
    } audioStream;

    /** Supported RTP parameters. */
    struct {
        /** Supported SRTP crypto suites. */
        HAPSRTPCryptoSuite srtpCryptoSuites;
    } rtp;
} HAPCameraStreamSupportedConfigurations;
//----------------------------------------------------------------------------------------------------------------------

/**
 * The minimum and maximum prebuffer duration that may be supported.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 11.125 Supported Camera Recording Configuration
 */
/**@{*/
#define kHAPCameraRecordingSupportedConfiguration_MinPrebufferDuration ((HAPTime)(4000 * HAPMillisecond))
#define kHAPCameraRecordingSupportedConfiguration_MaxPrebufferDuration ((HAPTime)(UINT32_MAX * HAPMillisecond))
/**@}*/

/**
 * Supported IP Camera recording configuration.
 */
typedef struct {
    /** Supported camera recording parameters. */
    struct {
        /** Duration of prebuffer recording available prior to the trigger event detection. */
        HAPTime prebufferDuration;

        /** Supported event trigger types. */
        HAPCameraEventTriggerTypes eventTriggerTypes;

        /** Array of supported media container configurations. NULL-terminated. */
        const HAPCameraSupportedMediaContainerConfiguration* _Nullable const* _Nullable containerConfigurations;
    } recording;

    /** Supported video parameters. */
    struct {
        /** Array of supported video codec configurations. NULL-terminated. */
        const HAPCameraSupportedVideoCodecConfiguration* _Nullable const* _Nullable configurations;
    } video;

    /** Supported audio parameters. */
    struct {
        /** Array of supported audio codec recording configurations. NULL-terminated. */
        const HAPCameraSupportedAudioCodecConfiguration* _Nullable const* _Nullable configurations;
    } audio;
} HAPCameraRecordingSupportedConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Camera recording configuration.
 */
typedef struct {
    /** Camera recording parameters. */
    struct {
        /** Duration of prebuffer recording available prior to the trigger event detection. */
        HAPTime prebufferDuration;

        /** Enabled event trigger types. */
        HAPCameraEventTriggerTypes eventTriggerTypes;

        /** Media container configuration. */
        struct {
            /** Media container type. */
            HAPMediaContainerType containerType;

            /** Media container parameters. */
            union {
                /** Fragmented MP4. */
                HAPFragmentedMP4MediaContainerParameters fragmentedMP4;
            } containerParameters;
        } containerConfiguration;
    } recording;

    /** Video parameters. */
    struct {
        /** Video codec type. */
        HAPVideoCodecType codecType;

        /** Video codec parameters. */
        union {
            /** H.264. */
            HAPH264VideoCodecParameters h264;
        } codecParameters;

        /** Video attributes. */
        HAPVideoAttributes attributes;
    } video;

    /** Audio parameters. */
    struct {
        /** Audio codec type. */
        HAPAudioCodecType codecType;

        /** Audio codec parameters. */
        HAPAudioCodecParameters codecParameters;
    } audio;
} HAPCameraRecordingConfiguration;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Fills a buffer with zeros.
 *
 * @param[out] bytes                Buffer to fill with zeros.
 * @param      numBytes             Number of bytes to fill.
 */
void HAPRawBufferZero(void* bytes, size_t numBytes);

/**
 * Copies bytes from a source buffer to a destination buffer.
 *
 * @param[out] destinationBytes     Destination buffer.
 * @param      sourceBytes          Source buffer.
 * @param      numBytes             Number of bytes to copy.
 */
void HAPRawBufferCopyBytes(void* destinationBytes, const void* sourceBytes, size_t numBytes);

/**
 * Determines equality of two buffers in constant time.
 *
 * @param      bytes                Buffer to compare.
 * @param      otherBytes           Buffer to compare with.
 * @param      numBytes             Number of bytes to compare.
 *
 * @return true                     If the contents of both buffers are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPRawBufferAreEqual(const void* bytes, const void* otherBytes, size_t numBytes);

/**
 * Determines if a buffer contains only zeros in constant time.
 *
 * @param      bytes                Buffer to check for zeros.
 * @param      numBytes             Length of buffer.
 *
 * @return true                     If a buffer contains only zeros
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPRawBufferIsZero(const void* bytes, size_t numBytes);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    uint16_t ble_supported : 1;
    uint16_t wac_supported : 1;
    uint16_t thread_supported : 1;
} HAPSupportedTransportBitfield;

extern const HAPSupportedTransportBitfield kHAPSupportedTransports;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_STATIC_ASSERT(CHAR_BIT == 8, CHAR_BIT);
HAP_STATIC_ASSERT(sizeof(bool) == 1, bool);
HAP_STATIC_ASSERT(sizeof(uint8_t) == 1, uint8_t);
HAP_STATIC_ASSERT(sizeof(uint16_t) == 2, uint16_t);
HAP_STATIC_ASSERT(sizeof(uint32_t) == 4, uint32_t);
HAP_STATIC_ASSERT(sizeof(uint64_t) == 8, uint64_t);
HAP_STATIC_ASSERT(sizeof(int8_t) == 1, int8_t);
HAP_STATIC_ASSERT(sizeof(int16_t) == 2, int16_t);
HAP_STATIC_ASSERT(sizeof(int32_t) == 4, int32_t);
HAP_STATIC_ASSERT(sizeof(int64_t) == 8, int64_t);
HAP_STATIC_ASSERT(sizeof(float) == sizeof(uint32_t), float);
HAP_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t), double);
HAP_STATIC_ASSERT(sizeof(size_t) <= sizeof(uint64_t), size_t);

HAP_STATIC_ASSERT('\b' == 0x8, Backspace);
HAP_STATIC_ASSERT('\f' == 0xc, FormFeed);
HAP_STATIC_ASSERT('\n' == 0xa, NewLine);
HAP_STATIC_ASSERT('\r' == 0xd, CarriageReturn);
HAP_STATIC_ASSERT('\t' == 0x9, HorizontalTab);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Counts the number of bytes of a numeric value when serialized to a TLV item.
 *
 * @param      value                Numeric value.
 *
 * @return Number of bytes when serializing value to a TLV item.
 */
size_t HAPGetVariableIntEncodingLength(uint64_t value);

/**
 * Reads a UInt8 value from a buffer.
 *
 * @param      bytes                Buffer to read from. Must contain at least 1 byte.
 *
 * @return Value that has been read.
 */
#define HAPReadUInt8(bytes) (((const uint8_t*) (bytes))[0])

/**
 * Writes a UInt8 value to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 1 byte.
 * @param      value                Value to write.
 */
#define HAPWriteUInt8(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (value); \
    } while (0)

/**
 * Expands a UInt8 value to a sequence of bytes.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandUInt8(value) (value),

/**
 * Reads a UInt8 value from a buffer with optional 0-padding removed.
 *
 * @param      bytes_               Buffer to read from.
 * @param      numBytes             Length of buffer. Must be at most 1 byte.
 *
 * @return Value that has been read.
 */
HAP_RESULT_USE_CHECK
uint8_t HAPReadUIntMax8(const void* bytes_, size_t numBytes);

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int8 value from a buffer.
 *
 * @param      bytes                Buffer to read from. Must contain at least 1 byte.
 *
 * @return Value that has been read.
 */
#define HAPReadInt8(bytes) (int8_t)(((const uint8_t*) (bytes))[0])

/**
 * Writes an Int8 value to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 1 byte.
 * @param      value                Value to write.
 */
#define HAPWriteInt8(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = ((uint8_t)(value)); \
    } while (0)

/**
 * Expands an Int8 value to a sequence of bytes.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandInt8(value) (uint8_t)(value),

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt16 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 2 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleUInt16(bytes) \
    (uint16_t)( \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[1] << 0x08U))

/**
 * Writes a UInt16 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 2 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleUInt16(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint16_t)((value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint16_t)((value) >> 0x08U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt16 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleUInt16(value) \
    (uint8_t)((uint16_t)((value) >> 0x00U) & 0xFFU), (uint8_t)((uint16_t)((value) >> 0x08U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int16 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 2 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleInt16(bytes) \
    (int16_t)( \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[1] << 0x08U))

/**
 * Writes an Int16 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 2 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleInt16(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint16_t)((uint16_t)(value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint16_t)((uint16_t)(value) >> 0x08U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int16 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleInt16(value) \
    (uint8_t)((uint16_t)((uint16_t)(value) >> 0x00U) & 0xFFU), (uint8_t)((uint16_t)((uint16_t)(value) >> 0x08U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt24 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 3 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleUInt24(bytes) \
    (uint32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x10U))

/**
 * Writes a UInt24 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 3 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleUInt24(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((value) >> 0x10U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt24 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleUInt24(value) \
    (uint8_t)((uint32_t)((value) >> 0x00U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((value) >> 0x10U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int24 value from a buffer containing its corresponding little-endian representation.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param      bytes                Buffer to read from. Must contain at least 3 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleInt24(bytes) \
    (int32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x10U))

/**
 * Writes an Int24 value's little endian representation to a buffer.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 3 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleInt24(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int24 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleInt24(value) \
    (uint8_t)((uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt32 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 4 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleUInt32(bytes) \
    (uint32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[3] << 0x18U))

/**
 * Writes a UInt32 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 4 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleUInt32(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint32_t)((value) >> 0x18U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt32 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleUInt32(value) \
    (uint8_t)((uint32_t)((value) >> 0x00U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((value) >> 0x10U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x18U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int32 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 4 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleInt32(bytes) \
    (int32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[3] << 0x18U))

/**
 * Writes an Int32 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 4 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleInt32(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint32_t)((uint32_t)(value) >> 0x18U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int32 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleInt32(value) \
    (uint8_t)((uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x18U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt64 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 8 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleUInt64(bytes) \
    (uint64_t)( \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[2] << 0x10U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[3] << 0x18U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[4] << 0x20U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[5] << 0x28U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[6] << 0x30U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[7] << 0x38U))

/**
 * Writes a UInt64 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 8 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleUInt64(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint64_t)((value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint64_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint64_t)((value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint64_t)((value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[4] = (uint64_t)((value) >> 0x20U) & 0xFFU; \
        ((uint8_t*) (bytes))[5] = (uint64_t)((value) >> 0x28U) & 0xFFU; \
        ((uint8_t*) (bytes))[6] = (uint64_t)((value) >> 0x30U) & 0xFFU; \
        ((uint8_t*) (bytes))[7] = (uint64_t)((value) >> 0x38U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt64 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleUInt64(value) \
    (uint8_t)((uint64_t)((value) >> 0x00U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x10U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x18U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x20U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x28U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x30U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x38U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int64 value from a buffer containing its corresponding little-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 8 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadLittleInt64(bytes) \
    (int64_t)( \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[0] << 0x00U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[2] << 0x10U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[3] << 0x18U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[4] << 0x20U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[5] << 0x28U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[6] << 0x30U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[7] << 0x38U))

/**
 * Writes an Int64 value's little endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 8 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteLittleInt64(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint64_t)((uint64_t)(value) >> 0x00U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint64_t)((uint64_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint64_t)((uint64_t)(value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint64_t)((uint64_t)(value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[4] = (uint64_t)((uint64_t)(value) >> 0x20U) & 0xFFU; \
        ((uint8_t*) (bytes))[5] = (uint64_t)((uint64_t)(value) >> 0x28U) & 0xFFU; \
        ((uint8_t*) (bytes))[6] = (uint64_t)((uint64_t)(value) >> 0x30U) & 0xFFU; \
        ((uint8_t*) (bytes))[7] = (uint64_t)((uint64_t)(value) >> 0x38U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int64 value to a sequence of bytes containing its corresponding little-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandLittleInt64(value) \
    (uint8_t)((uint64_t)((uint64_t)(value) >> 0x00U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x18U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x20U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x28U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x30U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x38U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt16 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 2 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigUInt16(bytes) \
    (uint16_t)( \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[0] << 0x08U) | \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[1] << 0x00U))

/**
 * Writes a UInt16 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 2 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigUInt16(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint16_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint16_t)((value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt16 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigUInt16(value) \
    (uint8_t)((uint16_t)((value) >> 0x08U) & 0xFFU), (uint8_t)((uint16_t)((value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int16 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 2 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigInt16(bytes) \
    (uint16_t)( \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[0] << 0x08U) | \
            (uint16_t)((uint16_t)((const uint8_t*) (bytes))[1] << 0x00U))

/**
 * Writes an Int16 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 2 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigInt16(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint16_t)((uint16_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint16_t)((uint16_t)(value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int16 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigInt16(value) \
    (uint8_t)((uint16_t)((uint16_t)(value) >> 0x08U) & 0xFFU), (uint8_t)((uint16_t)((uint16_t)(value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt24 value from a buffer containing its corresponding big-endian representation.
 *
 * - UInt24 is represented as a UInt32 value.
 *
 * @param      bytes                Buffer to read from. Must contain at least 3 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigUInt24(bytes) \
    (uint32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x00U))

/**
 * Writes a UInt24 value's big endian representation to a buffer.
 *
 * - UInt24 is represented as a UInt32 value.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 3 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigUInt24(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt24 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * - UInt24 is represented as a UInt32 value.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigUInt24(value) \
    (uint8_t)((uint32_t)((value) >> 0x10U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int24 value from a buffer containing its corresponding big-endian representation.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param      bytes                Buffer to read from. Must contain at least 3 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigInt24(bytes) \
    (int32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x00U))

/**
 * Writes an Int24 value's big endian representation to a buffer.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 3 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigInt24(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int24 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * - Int24 is represented as an Int32 value.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigInt24(value) \
    (uint8_t)((uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt32 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 4 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigUInt32(bytes) \
    (uint32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x18U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[3] << 0x00U))

/**
 * Writes a UInt32 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 4 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigUInt32(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint32_t)((value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt32 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigUInt32(value) \
    (uint8_t)((uint32_t)((value) >> 0x18U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint32_t)((value) >> 0x08U) & 0xFFU), (uint8_t)((uint32_t)((value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int32 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 4 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigInt32(bytes) \
    (int32_t)( \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[0] << 0x18U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[1] << 0x10U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[2] << 0x08U) | \
            (uint32_t)((uint32_t)((const uint8_t*) (bytes))[3] << 0x00U))

/**
 * Writes an Int32 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 4 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigInt32(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint32_t)((uint32_t)(value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int32 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigInt32(value) \
    (uint8_t)((uint32_t)((uint32_t)(value) >> 0x18U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint32_t)((uint32_t)(value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads a UInt64 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 8 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigUInt64(bytes) \
    (uint64_t)( \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[0] << 0x38U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[1] << 0x30U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[2] << 0x28U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[3] << 0x20U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[4] << 0x18U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[5] << 0x10U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[6] << 0x08U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[7] << 0x00U))

/**
 * Writes a UInt64 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 8 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigUInt64(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint64_t)((value) >> 0x38U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint64_t)((value) >> 0x30U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint64_t)((value) >> 0x28U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint64_t)((value) >> 0x20U) & 0xFFU; \
        ((uint8_t*) (bytes))[4] = (uint64_t)((value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[5] = (uint64_t)((value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[6] = (uint64_t)((value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[7] = (uint64_t)((value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands a UInt64 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigUInt64(value) \
    (uint8_t)((uint64_t)((value) >> 0x38U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x30U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x28U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x20U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x18U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint64_t)((value) >> 0x08U) & 0xFFU), (uint8_t)((uint64_t)((value) >> 0x00U) & 0xFFU)

//----------------------------------------------------------------------------------------------------------------------

/**
 * Reads an Int64 value from a buffer containing its corresponding big-endian representation.
 *
 * @param      bytes                Buffer to read from. Must contain at least 8 bytes.
 *
 * @return Value that has been read.
 */
#define HAPReadBigInt64(bytes) \
    (int64_t)( \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[0] << 0x38U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[1] << 0x30U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[2] << 0x28U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[3] << 0x20U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[4] << 0x18U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[5] << 0x10U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[6] << 0x08U) | \
            (uint64_t)((uint64_t)((const uint8_t*) (bytes))[7] << 0x00U))

/**
 * Writes an Int64 value's big endian representation to a buffer.
 *
 * @param[out] bytes                Buffer to write to. Must have space for at least 8 bytes.
 * @param      value                Value to write.
 */
#define HAPWriteBigInt64(bytes, value) \
    do { \
        ((uint8_t*) (bytes))[0] = (uint64_t)((uint64_t)(value) >> 0x38U) & 0xFFU; \
        ((uint8_t*) (bytes))[1] = (uint64_t)((uint64_t)(value) >> 0x30U) & 0xFFU; \
        ((uint8_t*) (bytes))[2] = (uint64_t)((uint64_t)(value) >> 0x28U) & 0xFFU; \
        ((uint8_t*) (bytes))[3] = (uint64_t)((uint64_t)(value) >> 0x20U) & 0xFFU; \
        ((uint8_t*) (bytes))[4] = (uint64_t)((uint64_t)(value) >> 0x18U) & 0xFFU; \
        ((uint8_t*) (bytes))[5] = (uint64_t)((uint64_t)(value) >> 0x10U) & 0xFFU; \
        ((uint8_t*) (bytes))[6] = (uint64_t)((uint64_t)(value) >> 0x08U) & 0xFFU; \
        ((uint8_t*) (bytes))[7] = (uint64_t)((uint64_t)(value) >> 0x00U) & 0xFFU; \
    } while (0)

/**
 * Expands an Int64 value to a sequence of bytes containing its corresponding big-endian representation.
 *
 * @param      value                Value to expand.
 *
 * @return Sequence of bytes representing the value.
 */
#define HAPExpandBigInt64(value) \
    (uint8_t)((uint64_t)((uint64_t)(value) >> 0x38U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x30U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x28U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x20U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x18U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x10U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x08U) & 0xFFU), \
            (uint8_t)((uint64_t)((uint64_t)(value) >> 0x00U) & 0xFFU)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Maximum number of bytes needed by the string representation of a UInt8 in decimal format.
 *
 * - UINT8_MAX = 0xFF = 255.
 */
#define kHAPUInt8_MaxDescriptionBytes (sizeof "255")

/**
 * Maximum number of bytes needed by the string representation of a UInt16 in decimal format.
 *
 * - UINT16_MAX = 0xFFFF = 65535.
 */
#define kHAPUInt16_MaxDescriptionBytes (sizeof "65535")

/**
 * Maximum number of bytes needed by the string representation of a UInt32 in decimal format.
 *
 * - UINT32_MAX = 0xFFFFFFFF = 4294967295.
 */
#define kHAPUInt32_MaxDescriptionBytes (sizeof "4294967295")

/**
 * Maximum number of bytes needed by the string representation of a float in decimal format.
 *
 * - Maximum is 9 significant digits + decimal point + sign + two digit exponent.
 */
#define kHAPFloat_MaxDescriptionBytes (sizeof "-1.23456789e-33")

/**
 * Determines the space needed by the string representation of the given integer value in decimal format.
 *
 * @param      value                Numeric value.
 *
 * @return Number of bytes that the value's string representation needs (excluding NULL-terminator).
 */
HAP_RESULT_USE_CHECK
size_t HAPInt32GetNumDescriptionBytes(int32_t value);

/**
 * Determines the space needed by the string representation of the given integer value in decimal format.
 *
 * @param      value                Numeric value.
 *
 * @return Number of bytes that the value's string representation needs (excluding NULL-terminator).
 */
HAP_RESULT_USE_CHECK
size_t HAPUInt64GetNumDescriptionBytes(uint64_t value);

/**
 * Gets the string representation of the given integer value in decimal format.
 *
 * @param      value                Numeric value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt64GetDescription(uint64_t value, char* bytes, size_t maxBytes);

/**
 * Letter case.
 */
HAP_ENUM_BEGIN(uint8_t, HAPLetterCase) { /**
                                          * Lowercase.
                                          */
                                         kHAPLetterCase_Lowercase = 'a',

                                         /**
                                          * Uppercase.
                                          */
                                         kHAPLetterCase_Uppercase = 'A'
} HAP_ENUM_END(uint8_t, HAPLetterCase);

/**
 * Gets the string representation of the given integer value in hexadecimal format.
 *
 * @param      value                Numeric value.
 * @param[out] bytes                Buffer to fill with the value's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 * @param      letterCase           Lower case or upper case characters.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt64GetHexDescription(uint64_t value, char* bytes, size_t maxBytes, HAPLetterCase letterCase);

/**
 * Creates a new (uint64_t) integer value from the given string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes in base 10 is not representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt64FromString(const char* description, uint64_t* value);

/**
 * Creates a new (uint32_t) integer value from the given string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes in base 10 is not representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt32FromString(const char* description, uint32_t* value);

/**
 * Creates a new (uint16_t) integer value from the given string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes in base 10 is not representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt16FromString(const char* description, uint16_t* value);

/**
 * Creates a new (uint8_t) integer value from the given string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes in base 10 is not representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromString(const char* description, uint8_t* value);

/**
 * Creates a new integer value from the given string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes in base 10 is not representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPInt64FromString(const char* description, int64_t* value);

/**
 * Creates a new (uint64_t) integer value from the given Hex
 * string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt64FromHexString(const char* description, uint64_t* value);

/**
 * Creates a new (uint32_t) integer value from the given Hex
 * string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt32FromHexString(const char* description, uint32_t* value);

/**
 * Creates a new (uint16_t) integer value from the given Hex
 * string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt16FromHexString(const char* description, uint16_t* value);

/**
 * Creates a new (uint8_t) integer value from the given Hex
 * string.
 *
 * - The string may begin with a plus or minus sign character (+ or -), followed by one or more numeric digits (0-9).
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromHexString(const char* description, uint8_t* value);

/**
 * Creates a new (uint8_t) integer value from the given Hex Digit.
 *
 *
 * @param      description          The ASCII representation of a hex digit
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt8FromHexDigit(char description, uint8_t* value);

/**
 * Fills a pre-allocated uint8_t buffer from the given hex string.
 *
 *
 * @param      hexString            The ASCII representation of a hex string
 * @param      byteBufferSize       Size of the byteBuffer parameter
 * @param[out] byteBuffer           The buffer to hold the converted hex string
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format,
 *                                  or if the value it denotes
 *                                  in base 16 is not
 *                                  representable.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUInt8BufferFromHexString(const char* hexString, size_t byteBufferSize, uint8_t* byteBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates a new float value with the given bit pattern.
 *
 * - The bit pattern is interpreted in the binary interchange format defined by the IEEE 754 specification.
 *
 * @param      bitPattern           The integer encoding of the float value.
 *
 * @return Float value with the given bit pattern.
 */
HAP_RESULT_USE_CHECK
float HAPFloatFromBitPattern(uint32_t bitPattern);

/**
 * Returns the bit pattern of a float value.
 *
 * - The bit pattern matches the binary interchange format defined by the IEEE 754 specification.
 *
 * @param      value                Value.
 *
 * @return The bit pattern of the value's encoding.
 */
HAP_RESULT_USE_CHECK
uint32_t HAPFloatGetBitPattern(float value);

/**
 * Creates a new float value from the given string.
 *
 * - The string can represent a real number in decimal format.
 *
 * - The given string may begin with a plus or minus sign character (+ or -).
 *
 * - A decimal value contains the significand, a sequence of decimal digits that may include a decimal point.
 *   A decimal value may also include an exponent following the significand,
 *   indicating the power of 10 by which the significand should be multiplied.
 *   If included, the exponent is separated by a single character, e or E,
 *   and consists of an optional plus or minus sign character and a sequence of decimal digits.
 *
 * - Hexadecimal format is not supported at this time.
 *
 * - Special floating-point values for infinity and NaN ("not a number") are not supported at this time.
 *
 * @param      description          The ASCII representation of a number. NULL-terminated.
 * @param[out] value                Numeric value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If the string is in an invalid format.
 */
HAP_RESULT_USE_CHECK
HAPError HAPFloatFromString(const char* description, float* value);

/**
 * Creates a string representation of a float value.
 *
 * The string will represent the float in decimal format.
 * The string will use at most kHAPFloat_MaxDescriptionBytes bytes excluding NULL termination.
 * The output will contain:
 * - "nan", if the float represents a NAN.
 * - "inf" or "-inf" if the float represents plus or minus infinity.
 * - A decimal integer if the float represents as an integer in the range 0 to 999999.
 * - A decimal fixpoint number if the float is in the range 10^-4 to 10^6.
 * - A decimal float in scientific notation otherwise (x.xxxxxe-xx).
 * In any case the number of digits used is chosen such that reading the string with either
 * HAPFloatFromString() or the standard function strtof() will retrieve the original float.
 *
 * @param[out] bytes                Buffer to fill with the string. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 * @param      value                The float value.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPFloatGetDescription(char* bytes, size_t maxBytes, float value);

/**
 * Absolute value of the supplied floating point value.
 *
 * @param      value                Value.
 *
 * @return |value|.
 */
HAP_RESULT_USE_CHECK
float HAPFloatGetAbsoluteValue(float value);

/**
 * Fractional part of the supplied floating point value.
 *
 * @param      value                Value.
 *
 * @return fractional part of value.
 */
HAP_RESULT_USE_CHECK
float HAPFloatGetFraction(float value);

/**
 * Determines whether the supplied floating point value is zero.
 *
 * - This returns true for either -0.0 or +0.0.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the supplied value is zero.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPFloatIsZero(float value);

/**
 * Determines whether the supplied floating point value is finite.
 *
 * - All values other than NaN and infinity are considered finite, whether normal or subnormal.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the supplied value is finite.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPFloatIsFinite(float value);

/**
 * Determines whether the supplied floating point value is infinite.
 *
 * @param      value                Value to check.
 *
 * @return true                     If the supplied value is infinite.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPFloatIsInfinite(float value);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates a new double value with the given bit pattern.
 *
 * - The bit pattern is interpreted in the binary interchange format defined by the IEEE 754 specification.
 *
 * @param      bitPattern           The integer encoding of the float value.
 *
 * @return Double value with the given bit pattern.
 */
HAP_RESULT_USE_CHECK
double HAPDoubleFromBitPattern(uint64_t bitPattern);

/**
 * Returns the bit pattern of a double value.
 *
 * - The bit pattern matches the binary interchange format defined by the IEEE 754 specification.
 *
 * @param      value                Value.
 *
 * @return The bit pattern of the value's encoding.
 */
HAP_RESULT_USE_CHECK
uint64_t HAPDoubleGetBitPattern(double value);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates a string from a formatted string and a variable number of arguments.
 *
 * - The supported conversion specifiers follow the IEEE printf specification.
 *   See http://pubs.opengroup.org/onlinepubs/009695399/functions/printf.html
 *
 * @param[out] bytes                Buffer to fill with the formatted string. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 * @param      format               A format string.
 * @param      ...                  Arguments for the format string.
 *
 * Currently the following options are supported:
 * flags:  0, +, ' '
 * width:  number
 * length: l, ll, z
 * types:  %, d, i, u, x, X, g, c, s, p
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_PRINTFLIKE(3, 4)
HAP_RESULT_USE_CHECK
HAPError HAPStringWithFormat(char* bytes, size_t maxBytes, const char* format, ...);

/**
 * Creates a string from a formatted string and a variable number of arguments.
 *
 * - The supported conversion specifiers follow the IEEE printf specification.
 *   See http://pubs.opengroup.org/onlinepubs/009695399/functions/printf.html
 *
 * Currently the following options are supported:
 * flags:  0, +, ' '
 * width:  number
 * length: l, ll, z
 * types:  %, d, i, u, x, X, g, c, s, p
 *
 * @param[out] bytes                Buffer to fill with the formatted string. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 * @param      format               A format string.
 * @param      arguments            Arguments for the format string.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_PRINTFLIKE(3, 0)
HAP_RESULT_USE_CHECK
HAPError HAPStringWithFormatAndArguments(char* bytes, size_t maxBytes, const char* format, va_list arguments);

/**
 * Returns the number of bytes of a string, excluding the NULL-terminator.
 *
 * @param      string               String. NULL-terminated.
 *
 * @return Number of bytes of the string.
 */
HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
size_t HAPStringGetNumBytes(const char* string);

/**
 * Determines equality of two strings.
 *
 * @param      string               String to compare. NULL-terminated.
 * @param      otherString          String to compare with. NULL-terminated.
 *
 * @return true                     If the contents of both strings are equal.
 * @return false                    Otherwise.
 */
HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
bool HAPStringAreEqual(const char* string, const char* otherString);

/**
 * Indicates whether or not the string begins with the specified prefix.
 *
 * @param      string               String. NULL-terminated
 * @param      prefix               A possible prefix to test against the string. NULL-terminated.
 *
 * @return true                     If the string begins with the specified prefix.
 * @return false                    Otherwise.
 */
HAP_NO_SIDE_EFFECTS
HAP_RESULT_USE_CHECK
bool HAPStringHasPrefix(const char* string, const char* prefix);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Determines whether the supplied data is a valid UTF-8 byte sequence according to
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - Table 3-7, page 94.
 *
 * @param      bytes                Input data.
 * @param      numBytes             Length of input data.
 *
 * @return true                     If the supplied data is a valid UTF-8 byte sequence.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPUTF8IsValidData(const void* bytes, size_t numBytes);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Indicates whether an ASCII character represents a letter (A-Z, a-z).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents a letter.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsLetter(char character);

/**
 * Indicates whether an ASCII character represents an uppercase letter (A-Z).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents an uppercase letter.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsUppercaseLetter(char character);

/**
 * Indicates whether an ASCII character represents a number (0-9).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents a number.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsNumber(char character);

/**
 * Indicates whether an ASCII character represents an alphanumeric character (0-9, A-Z, a-z).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents an alphanumeric character.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsAlphanumeric(char character);

/**
 * Indicates whether an ASCII character represents an uppercase
 * alphanumeric character (0-9, A-Z).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents an uppercase alphanumeric character.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsUppercaseAlphanumeric(char character);

/**
 * Indicates whether an ASCII character represents a hexadecimal digit (0-9, A-F, a-f).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents a hexadecimal digit.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsHexDigit(char character);

/**
 * Indicates whether an ASCII character represents a lowercase hexadecimal digit (0-9, a-f).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents a lowercase hexadecimal digit.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsLowercaseHexDigit(char character);

/**
 * Indicates whether an ASCII character represents an uppercase hexadecimal digit (0-9, A-F).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents an uppercase hexadecimal digit.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsUppercaseHexDigit(char character);

/**
 * Indicates whether an ASCII character represents a lowercase hexadecimal character (a-f).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents a lowercase hexadecimal character.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsLowercaseHexLetter(char character);

/**
 * Indicates whether an ASCII character represents an uppercase hexadecimal character (A-F).
 *
 * @param      character            ASCII character to test.
 *
 * @return true                     If the ASCII character represents an uppercase hexadecimal character.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPASCIICharacterIsUppercaseHexLetter(char character);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Length of SHA-1 hash.
 */
#define kHAPSHA1Checksum_NumBytes ((size_t) 20)

/**
 * The SHA-1 based checksum of a given input block is computed and put into @p checksum.
 *
 * @param[out] checksum             Generated checksum.
 * @param      bytes                Input data.
 * @param      numBytes             Length of @p data.
 */
void HAPSHA1Checksum(uint8_t checksum[_Nonnull kHAPSHA1Checksum_NumBytes], const void* bytes, size_t numBytes);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#include "HAPAssert.h"
#include "HAPLog.h"

// Functions to convert a _Nullable type to its _Nonnull variant.
#if __has_feature(nullability) && __has_attribute(overloadable)
/**
 * Generates support functions to convert from _Nullable to _Nonnull.
 *
 * @param      type                 Type for which to generate support functions.
 */
#define HAP_NONNULL_SUPPORT(type) \
    HAP_DIAGNOSTIC_PUSH \
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wunused-function") \
    __attribute__((always_inline)) __attribute__(( \
            overloadable)) static type* _Nonnull HAPNonnull(/* NOLINT(bugprone-macro-parentheses) */ \
                                                            type* _Nullable const value) /* NOLINT(bugprone-macro-parentheses) \
                                                                                          */ \
    { \
        HAPAssert(value); \
        return value; \
    } \
\
    __attribute__((always_inline)) __attribute__(( \
            overloadable)) static const type* _Nonnull HAPNonnull(/* NOLINT(bugprone-macro-parentheses) */ \
                                                                  const type* _Nullable const value) /* NOLINT(bugprone-macro-parentheses) \
                                                                                                      */ \
    { \
        HAPAssert(value); \
        return value; \
    } \
    HAP_DIAGNOSTIC_POP

HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wunused-function")
__attribute__((always_inline))
__attribute__((overloadable)) static void* _Nonnull HAPNonnullVoid(void* _Nullable const value) {
    HAPAssert(value);
    return value;
}

__attribute__((always_inline))
__attribute__((overloadable)) static const void* _Nonnull HAPNonnullVoid(const void* _Nullable const value) {
    HAPAssert(value);
    return value;
}
HAP_DIAGNOSTIC_POP
#else
/**
 * Generates support functions to convert from _Nullable to _Nonnull.
 *
 * @param      type                 Type for which to generate support functions.
 */
#define HAP_NONNULL_SUPPORT(type)

static inline void HAPCheckNonnull(const void* value) {
    HAPAssert(value);
}

#define HAP_PP_COMMA          ,
#define HAPNonnullVoid(value) (HAPCheckNonnull(value) HAP_PP_COMMA value)
#define HAPNonnull(value)     (HAPNonnullVoid(value))
#endif
HAP_NONNULL_SUPPORT(char)
HAP_NONNULL_SUPPORT(uint8_t)
HAP_NONNULL_SUPPORT(int64_t)
HAP_NONNULL_SUPPORT(double)
HAP_NONNULL_SUPPORT(HAPIPAddress)
HAP_NONNULL_SUPPORT(HAPSetupCode)
HAP_NONNULL_SUPPORT(HAPSetupPayload)
HAP_NONNULL_SUPPORT(HAPCameraRecordingConfiguration)

#ifdef __cplusplus
}
#endif

#endif
