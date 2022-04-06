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

#ifndef HAP_WIFI_ROUTER_H
#define HAP_WIFI_ROUTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPCharacteristicTypes.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Wi-Fi Router version.
 *
 * Represents the latest version of the specification supported by the router.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.48 Wi-Fi Router
 */
#define kHAPWiFiRouter_Version "1.1.0"

// To achieve a compact memory layout the format is embedded in each sub-structure.
//
// A more straight-forward implementation would look like this:
//     struct {
//         HAPPlatformWiFiRouterClientStatusIdentifierFormat format;
//         union {
//             HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
//             HAPMACAddress macAddress;
//             HAPIPAddress ipAddress;
//         } _;
//     }
// In this case, the _ union would have to be 4-byte aligned due to the clientIdentifier.
// ipAddress is its largest member, so the _ union would be 20 bytes.
// The format preceding the union would add another 4 byte to satisfy the alignment requirement of the union.
//     struct {
//         HAPPlatformWiFiRouterClientStatusIdentifierFormat format;
//         char padding[3]; // Padding to satisfy alignment requirement of the _ union.
//         union {
//             HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
//             HAPMACAddress macAddress;
//             HAPIPAddress ipAddress;
//         } _;
//     }
// Embedding the format into each sub-structure allows moving of macAddress and ipAddress into the padding area,
// and reducing the total struct size from 24 bytes to 20 bytes.
// This allows to support more Network Client Status Identifiers in a single bulk operation with the same resources.
HAP_STATIC_ASSERT(
        sizeof(HAPPlatformWiFiRouterClientStatusIdentifier) <= 20,
        HAPPlatformWiFiRouterClientStatusIdentifier);

/**
 * Maximum number of bulk operations that are accepted in one request.
 *
 * - 4 bytes per identifier (+ 1 bit for isAddOperation).
 */
#define kHAPWiFiRouter_MaxBulkOperations ((size_t)(6 * 5)) // Should be multiple of 5 (20 / 4).
HAP_STATIC_ASSERT(kHAPWiFiRouter_MaxBulkOperations <= UINT8_MAX, MaxBulkOperations);

/**
 * Maximum number of Network Client Status Identifiers that are accepted in one request.
 *
 * - 20 bytes per identifier.
 */
#define kHAPWiFiRouter_MaxStatusIdentifiers ((size_t)(kHAPWiFiRouter_MaxBulkOperations * 4 / 20))
HAP_STATIC_ASSERT(kHAPWiFiRouter_MaxStatusIdentifiers <= UINT8_MAX, MaxStatusIdentifiers);

/**
 * Network control point operation.
 */
HAP_ENUM_BEGIN(uint8_t, HAPWiFiRouterOperation) { /** No operations specified. */
                                                  kHAPWiFiRouterOperation_Undefined,

                                                  /** List all identifiers. */
                                                  kHAPWiFiRouterOperation_Enumerate,

                                                  /** Read configurations of specified identifiers. */
                                                  kHAPWiFiRouterOperation_Query,

                                                  /** Modify configurations of specified identifiers. */
                                                  kHAPWiFiRouterOperation_Modify
} HAP_ENUM_END(uint8_t, HAPWiFiRouterOperation);

/**
 * Wi-Fi router specific state. Stored as part of HAPAccessoryServer structure.
 */
typedef struct {
    /** Control point specific data. */
    union {
        /** Identifiers being accessed. */
        HAPPlatformWiFiRouterIdentifier identifiers[kHAPWiFiRouter_MaxBulkOperations];

        /** Network Client Status Identifiers being accessed. */
        HAPPlatformWiFiRouterClientStatusIdentifier statusIdentifiers[kHAPWiFiRouter_MaxStatusIdentifiers];
    } _;

    /** Bit-field that contains 1 bit for each identifier that denotes whether it corresponds to an Add operation. */
    uint8_t isAddOperation[(kHAPWiFiRouter_MaxBulkOperations + CHAR_BIT - 1) / CHAR_BIT];

    /** Number of identifiers being accessed. */
    uint8_t numIdentifiers;

    /** Current network control point operation (to keep track of state between write and read handlers). */
    HAPWiFiRouterOperation operation;

    /** Status code in case an operation failed. */
    HAPCharacteristicValue_WiFiRouter_OperationStatus status;

    /** Index of operation that failed, if an error status is indicated. */
    uint8_t failedOperationIndex;

    /** Whether or not shared network configuration access has been acquired. */
    bool hasSharedConfigurationAccess : 1;

    /** Whether or not exclusive network configuration access has been acquired. */
    bool hasExclusiveConfigurationAccess : 1;

    /** Whether or not the network configuration is being changed. */
    bool isChangingConfiguration : 1;
} HAPWiFiRouter;

//----------------------------------------------------------------------------------------------------------------------

/** Maximum number of identifiers to cache for Wi-Fi Router specific events. */
#define kHAPWiFiRouterEventState_MaxIdentifiers ((size_t) 8)

/**
 * Wi-Fi router specific HAP event state. Stored as part of HAPAccessoryServer structure.
 */
typedef struct {
    /** Network Client Profile Identifiers that have recently been modified. */
    HAPPlatformWiFiRouterClientIdentifier identifiers[kHAPWiFiRouterEventState_MaxIdentifiers];

    /** Logical timestamp of most recently updated entry. 0 if no entries were updated yet. */
    uint8_t timestamp;

    /** Index where the next Network Client Profile Identifier will be stored. */
    uint8_t nextIndex;
} HAPWiFiRouterEventState;

/**
 * Wi-Fi router specific session state. Stored as part of HAPSession structure.
 */
typedef struct {
    /** Logical timestamp of most recently sent HAP Event, if subscribed. 0 otherwise. */
    uint8_t timestamp;

    /** Bitmask indicating which Network Client Profile Identifiers were updated from this HAP session. */
    uint8_t isOriginator[(kHAPWiFiRouterEventState_MaxIdentifiers + CHAR_BIT - 1) / CHAR_BIT];
} HAPWiFiRouterSessionState;

/**
 * Handles a subscribe request to a Wi-Fi Router characteristic.
 *
 * @param      eventState           Wi-Fi router specific HAP event state associated with the characteristic.
 * @param      sessionState         Wi-Fi router specific session state associated with the characteristic.
 */
void HAPWiFiRouterSubscribeForEvents(HAPWiFiRouterEventState* eventState, HAPWiFiRouterSessionState* sessionState);

/**
 * Handles an unsubscribe request to a Wi-Fi Router characteristic.
 *
 * @param      eventState           Wi-Fi router specific HAP event state associated with the characteristic.
 * @param      sessionState         Wi-Fi router specific session state associated with the characteristic.
 */
void HAPWiFiRouterUnsubscribeFromEvents(HAPWiFiRouterEventState* eventState, HAPWiFiRouterSessionState* sessionState);

/**
 * Raises a network client profile related event on a Wi-Fi Router characteristic.
 *
 * @param      server               Accessory server.
 * @param      characteristic       Characteristic.
 * @param      clientIdentifier     Network client profile identifier.
 * @param      session              Session on which the network client profile event originated, if applicable.
 */
void HAPWiFiRouterRaiseClientEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPSession* _Nullable session);

/**
 * Handles a successful send of a network client profile related event on a Wi-Fi Router characteristic.
 *
 * @param      server               Accessory server.
 * @param      request              Characteristic request associated with the event.
 */
void HAPWiFiRouterHandleClientEventSent(HAPAccessoryServer* server, const HAPTLV8CharacteristicReadRequest* request);

/**
 * Retrieves the network client profile identifier list of pending events on a Wi-Fi Router characteristic.
 *
 * @param      server               Accessory server.
 * @param      request              Characteristic request associated with the event.
 * @param[out] clientList           Network Client List to serialize into.
 * @param[out] clientListIsSet      Whether or not the network client list should be included in the response.
 */
void HAPWiFiRouterGetEventClientList(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPCharacteristicValue_WiFiRouter_ClientList* clientList,
        bool* clientListIsSet);

//----------------------------------------------------------------------------------------------------------------------

/**
 * HAPPlatformWiFiRouterDelegate implementation.
 *
 * - Context is HAPAccessoryServer.
 */
/**@{*/
void HAPWiFiRouterHandleReadyStateChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

void HAPWiFiRouterHandleManagedNetworkStateChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

void HAPWiFiRouterHandleWANConfigurationChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

void HAPWiFiRouterHandleWANStatusChanged(HAPPlatformWiFiRouterRef wiFiRouter, void* _Nullable context);

void HAPWiFiRouterHandleAccessViolationMetadataChanged(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        void* _Nullable context);

void HAPWiFiRouterHandleSatelliteStatusChanged(
        HAPPlatformWiFiRouterRef wiFiRouter,
        size_t satelliteIndex,
        void* _Nullable context);
/**@}*/

/**
 * Determines whether a value represents a valid DNS name WAN host URI.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is a valid DNS name.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWiFiRouterHostDNSNameIsValid(const char* value);

/**
 * Determines whether a value represents a valid DNS name WAN host URI pattern.
 *
 * @param      value                Value.
 *
 * @return true                     If the provided value is a valid DNS name pattern.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWiFiRouterHostDNSNamePatternIsValid(const char* value);

// ISO C99 requires at least one argument for the "..." in a variadic macro.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC system_header
#endif

/**
 * Logs a default-level message related to a WAN.
 *
 * @param      logObject            Log object.
 * @param      wanIdentifier        WAN identifier.
 * @param      format               Format string that produces a human-readable log message.
 */
#define HAPLogWANError(logObject, wanIdentifier, format, ...) \
    HAPLogError(logObject, "[%08lX] " format, (unsigned long) (wanIdentifier), ##__VA_ARGS__)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
