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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef HAP_PLATFORM_WIFI_ROUTER_INIT_H
#define HAP_PLATFORM_WIFI_ROUTER_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_ROUTER)

#include <pthread.h>

// SQLite Dependency.
// It is not possible to import SQLite without causing warnings. For this reason, they are switched off locally:
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_CLANG("-Wreserved-id-macro")
#include "sqlite3.h"
HAP_DIAGNOSTIC_POP

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * FreeSafe wrapper around sqlite3_free.
 *
 * @param      ptr                  Pointer to object to be deallocated.
 */
#define sqlite3_free_safe(ptr) \
    do { \
        HAPAssert(ptr); \
        sqlite3_free(ptr); \
        ptr = NULL; \
    } while (0)

/**
 * FreeSafe wrapper around sqlite3_close.
 *
 * @param      ptr                  Pointer to object to be deallocated.
 */
#define sqlite3_close_safe(ptr) \
    do { \
        HAPAssert(ptr); \
        int e_ = sqlite3_close(ptr); \
        if (e_) { \
            HAPLogError(&logObject, "sqlite3_close failed: %d.", e_); \
            HAPFatalError(); \
        } \
        ptr = NULL; \
    } while (0)

/**
 * FreeSafe wrapper around sqlite3_finalize.
 *
 * @param      ptr                  Pointer to object to be deallocated.
 */
#define sqlite3_finalize_safe(ptr) \
    do { \
        HAPAssert(ptr); \
        (void) sqlite3_finalize(ptr); \
        ptr = NULL; \
    } while (0)

/** Maximum number of network client profile configurations that may be edited before committing the changes. */
#define kHAPPlatformWiFiRouter_MaxEdits ((size_t) 32)

/** Maximum number of WAN configurations that may be in use. */
#define kHAPPlatformWiFiRouter_MaxWANs ((size_t) 8)

/** Maximum number of concurrently connected network clients. */
#define kHAPPlatformWiFiRouter_MaxConnections ((size_t) 64)

/** Maximum number of IP addresses per connected network client. */
#define kHAPPlatformWiFiRouterConnection_MaxIPAddresses ((size_t) 4)

/** Maximum length of a network client name. */
#define kHAPPlatformWiFiRouterConnection_MaxNameBytes ((size_t) 255)

/** Maximum number of supported satellite accessories. */
#define kHAPPlatformWiFiRouter_MaxSatellites ((size_t) 2)

/**
 * Wi-Fi router.
 */
struct HAPPlatformWiFiRouter {
    // Do not access the instance fields directly.
    /**@cond */
    HAPPlatformWiFiRouterDelegate delegate;

    sqlite3* _Nullable db;

    struct {
        pthread_mutex_t mutex;
        pthread_key_t key; // [value >> 0 & 0xFF] numSharedAccessRefs / [value >> 8 & 0xFF] numExclusiveAccessRefs
        pthread_rwlock_t rwLock;
        volatile uint16_t numAccessRefs;
        uint8_t enumerationNestingLevel;
    } synchronization;

    struct {
        HAPPlatformWiFiRouterClientIdentifier clients[kHAPPlatformWiFiRouter_MaxEdits];
        size_t numClients;
        HAPMACAddress macAddresses[kHAPPlatformWiFiRouter_MaxEdits];
        size_t numMACAddresses;
        uint32_t firewallIndex;
        bool isEditingWANFirewall : 1;
        bool isEditingLANFirewall : 1;
        bool isEditing : 1;
    } edits;

    struct {
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier; // If set to 0, entry is unused.
        HAPPlatformWiFiRouterWANStatus wanStatus;
    } wans[kHAPPlatformWiFiRouter_MaxWANs];

    struct {
        bool isActive : 1;
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier;
        HAPMACAddress macAddress;
        HAPIPAddress ipAddresses[kHAPPlatformWiFiRouterConnection_MaxIPAddresses];
        size_t numIPAddresses;
        char name[kHAPPlatformWiFiRouterConnection_MaxNameBytes + 1];
        struct {
            HAPRSSI value;
            bool isDefined : 1;
        } rssi;
    } connections[kHAPPlatformWiFiRouter_MaxConnections];

    bool isReady : 1;

    struct {
        HAPPlatformWiFiRouterSatelliteStatus satelliteStatus;
    } satellites[kHAPPlatformWiFiRouter_MaxSatellites];
    /**@endcond */
};

/**
 * Wi-Fi router initialization options.
 */
typedef struct {
    /**
     * Database file location. NULL to use in-memory storage.
     *
     * - This location is relative to the directory from which the application is executing,
     *   i.e., not relative to the application binary.
     */
    const char* _Nullable dbFile;
} HAPPlatformWiFiRouterOptions;

/**
 * Initializes the Wi-Fi router.
 *
 * @param[out] wiFiRouter           Pointer to an allocated but uninitialized HAPPlatformWiFiRouter structure.
 * @param      options              Initialization options.
 */
void HAPPlatformWiFiRouterCreate(HAPPlatformWiFiRouterRef wiFiRouter, const HAPPlatformWiFiRouterOptions* options);

/**
 * Releases resources associated with an initialized Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 */
void HAPPlatformWiFiRouterRelease(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Returns whether network configuration access (shared or exclusive) is available.
 *
 * @param      wiFiRouter           Wi-Fi router.
 *
 * @return true                     If network configuration access may be requested.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiRouterIsConfigurationAccessAvailable(HAPPlatformWiFiRouterRef wiFiRouter);

/**
 * Connects a network client to a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      credential           Credential used by the network client if a Wi-Fi network interface is specified.
 * @param      macAddress           MAC address of the network client.
 * @param      name                 Name of the network client, if available.
 * @param[out] ipAddress            IP address of the network client. Optional.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If a network client with the given MAC address is already connected.
 * @return kHAPError_InvalidData    If the given parameters are not supported.
 * @return kHAPError_OufOfResources If no more network clients may join the network interface.
 * @return kHAPError_NotAuthorized  If the specified credentials are not valid for the network interface.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterConnectClient(
        HAPPlatformWiFiRouterRef wiFiRouter,
        const HAPWiFiWPAPersonalCredential* _Nullable credential,
        const HAPMACAddress* macAddress,
        const char* _Nullable name,
        HAPIPAddress* _Nullable ipAddress);

/**
 * Disconnects a network client from a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      macAddress           MAC address of the network client.
 */
void HAPPlatformWiFiRouterDisconnectClient(HAPPlatformWiFiRouterRef wiFiRouter, const HAPMACAddress* macAddress);

/**
 * Sets the ready state of a Wi-Fi router.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      ready                Whether the Wi-Fi router should indicate that it is ready.
 */
void HAPPlatformWiFiRouterSetReady(HAPPlatformWiFiRouterRef wiFiRouter, bool ready);

/**
 * Sets the WAN type of a WAN.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param      wanType              WAN type.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetType(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANType wanType);

/**
 * Sets the WAN status of a WAN.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      wanIdentifier        WAN identifier.
 * @param      wanStatus            WAN status.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no WAN with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterWANSetStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterWANIdentifier wanIdentifier,
        HAPPlatformWiFiRouterWANStatus wanStatus);

/**
 * Records a network access violation for a network client.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      clientIdentifier     Network client identifier to record network access violation for.
 * @param      timestamp            Network access violation timestamp.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If network configuration access failed.
 * @return kHAPError_InvalidState   If no network client with the given identifier exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterClientRecordAccessViolation(
        HAPPlatformWiFiRouterRef wiFiRouter,
        HAPPlatformWiFiRouterClientIdentifier clientIdentifier,
        HAPPlatformWiFiRouterTimestamp timestamp);

/**
 * Sets the status of a Wi-Fi router's satellite accessory.
 *
 * @param      wiFiRouter           Wi-Fi router.
 * @param      satelliteIndex       Index of the satellite accessory.
 * @param      status               Wi-Fi router's satellite accessory status.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If no satellite accessory with the given index exists.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiRouterSetSatelliteStatus(
        HAPPlatformWiFiRouterRef wiFiRouter,
        size_t satelliteIndex,
        HAPPlatformWiFiRouterSatelliteStatus status);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
