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

#ifndef HAP_PLATFORM_WIFI_MANAGER_H
#define HAP_PLATFORM_WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * WiFi manager.
 */
typedef struct HAPPlatformWiFiManager HAPPlatformWiFiManager;
typedef struct HAPPlatformWiFiManager* HAPPlatformWiFiManagerRef;
HAP_NONNULL_SUPPORT(HAPPlatformWiFiManager)

/**
 * WiFi manager capabilities.
 */
typedef struct {
    /**
     * WiFi manager supports joining 2.4 GHz networks.
     */
    bool supports2_4GHz : 1;

    /**
     * WiFi manager supports joining 5 GHz networks.
     */
    bool supports5GHz : 1;
} HAPPlatformWiFiManagerCapabilities;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * Cookie value that must be stored together with the Wi-Fi configuration.
 * If the Wi-Fi configuration is reset, or modified out-of-band, the cookie must be cleared.
 */
typedef uint16_t HAPPlatformWiFiManagerCookie;

/** Wi-Fi is not configured, or is using an un-managed configuration. */
#define kHAPPlatformWiFiManagerCookie_Unmanaged ((HAPPlatformWiFiManagerCookie) 0)

/**
 * Update Status that must be stored together with the Wi-Fi configuration.
 * If the Wi-Fi configuration is reset, or modified out-of-band, the update Status must be cleared.
 */
typedef uint32_t HAPPlatformWiFiManagerUpdateStatus;

/** Wi-Fi is not configured, or is using an un-managed configuration. */
#define kHAPPlatformWiFiManagerUpdateStatus_Unmanaged ((HAPPlatformWiFiManagerUpdateStatus) 0)
#endif
/**
 * Returns the supported WiFi manager capabilities.
 *
 * @param      wiFiManager          WiFi manager.
 *
 * @return WiFi capabilities.
 */
HAP_RESULT_USE_CHECK
HAPPlatformWiFiManagerCapabilities HAPPlatformWiFiManagerGetCapabilities(HAPPlatformWiFiManagerRef wiFiManager);

/**
 * Returns whether a WiFi network has been configured.
 *
 * @param      wiFiManager          WiFi manager.
 *
 * @return true                     If a WiFi network has been configured.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsConfigured(HAPPlatformWiFiManagerRef wiFiManager);

/**
 * Applies the WiFi configuration. Persists the WiFi credentials passed in.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
 * Restarts the WiFi interface if restartWiFi is set to true
#endif
 *
 * @param      wiFiManager          WiFi manager.
 * @param      ssid                 The SSID of the WPA/WPA2 personal WiFi network.
 * @param      passphrase           If the WiFi network is secured, the network’s passphrase credential:
 *                                  8-63 printable ASCII characters or 64 hexadecimal digits.
 * @param      regulatoryDomain     Optional regulatory domain in which the device is operating:
 *                                  NULL-terminated country code (ISO 3166-1 alpha-2).
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
 * @param      cookie               Cookie value sent from the controller in WiFi reconfiguration request
 * @param      updateStatus         UpdateStatus processed while handling fail safe WiFi reconfiguration request
 * @param      restartWiFi          Flag set to indicate whether WiFi should be restarted
#endif
 *
 * @return kHAPError_None           Applying the configuration was successful.
 * @return kHAPError_Unknown        Applying the configuration fails.
 * @return kHAPError_OutOfResources Restarting wifi fails as the buffer passed to store results is small.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerApplyConfiguration(
        HAPPlatformWiFiManagerRef wiFiManager,
        const char* ssid,
        const char* _Nullable passphrase,
        const char* _Nullable regulatoryDomain
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
        ,
        HAPPlatformWiFiManagerCookie cookie,
        uint32_t updateStatus,
        bool restartWiFi
#endif
);

/**
 * Clears the current WiFi configuration.
 *
 * @param      wiFiManager          WiFi manager
 *
 */
void HAPPlatformWiFiManagerClearConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);

/**
 * Removes the stored WiFi configuration. Reverts the WiFi configuration to backed up configuration if present.
 */
void HAPPlatformWiFiManagerRemoveConfiguration(HAPPlatformWiFiManagerRef wiFiManager);

/** Maximum length of a WiFi SSID in bytes. */
#define HAPPlatformSSID_MaxBytes ((size_t) 32)

/**
 * SSID.
 */
typedef struct {
    /** Value */
    uint8_t bytes[HAPPlatformSSID_MaxBytes];

    /** Length of value. */
    size_t numBytes;

    char stringValue[HAPPlatformSSID_MaxBytes + 1];
} HAPPlatformSSID;

/** Maximum length of country code in bytes. */
#define HAPPlatformCountryCode_MaxBytes ((size_t) 2)

/**
 * Country code (ISO 3166-1 alpha-2) to indicate the country-specific
 * regulatory domain in which the device is operating.
 */
typedef struct {
    char stringValue[HAPPlatformCountryCode_MaxBytes + 1]; /**< NULL-terminated. */
} HAPPlatformRegulatoryDomain;

/**
 * Gets the WiFi regulatory domain in which the device is operating.
 *
 * @param      wiFiManager          WiFi manager.
 * @param[out] regulatoryDomain     Regulatory domain in which the device is operating.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading regulatory domain.
 * @return kHAPError_InvalidState   If no regulatory domain has been configured.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetRegulatoryDomain(
        HAPPlatformWiFiManagerRef wiFiManager,
        HAPPlatformRegulatoryDomain* regulatoryDomain);

/**
 * Gets the WiFi SSID configured on the device.
 *
 * @param      wiFiManager          WiFi manager.
 * @param[out] isConfigured         Indicates whether SSID is configured on the device.
 * @param[out] ssid                 Struct containing SSID of the WiFi network configured on this device.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading SSID.
 * @return kHAPError_InvalidState   If SSID is not configured.
 */
HAP_RESULT_USE_CHECK
HAPError
        HAPPlatformWiFiManagerGetSSID(HAPPlatformWiFiManagerRef wiFiManager, bool* isConfigured, HAPPlatformSSID* ssid);

HAP_RESULT_USE_CHECK
const char* _Nullable HAPPlatformWiFiManagerGetConfiguredSSID(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);

/**
 * Restarts the WiFi interface after making changes to the WiFi configuration
 *
 * @param      wiFiManager          WiFi manager
 *
 * @return kHAPError_None           If WiFi restarts successfully
 * @return kHAPError_Unknown        If WiFi fails to restart
 * @return kHAPError_OutOfResources If the buffer was not big enough to store the result
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerRestartWiFi(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * Backs up the WiFi network configuration.
 *
 * @param      wiFiManager          WiFi manager.
 *
 * @return kHAPError_None           If the file was backed up successfully.
 * @return kHAPError_Unknown        If the file failed to back up.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerBackUpConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);

/**
 * Sets psk configured to true if PSK is configured in the WiFi configuration, false otherwise.
 *
 * @param wiFiManager               WiFi manager.
 * @param[out] isPskConfigured      Set to true if PSK is present, false otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading PSK.
 * @return kHAPError_InvalidState   If there is some error in retreiving the PSK to check if it's configured.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerIsPSKConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPskConfigured);

/**
 * Get the cookie value persisted in the WiFi configuration
 *
 * @param wiFiManager               WiFi manager
 * @param[out] cookie               Cookie value configured.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is some error in retreiving the cookie.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetCookie(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint16_t* _Nonnull cookie);

/**
 * Set the cookie value to be persisted in the wpa supplicant
 *
 * @param wiFiManager               WiFi manager
 * @param cookie                    Cookie value to be updated in the wpa supplicant
 *
 * @return kHAPError_None           Persisting the new cookie value was successful.
 * @return kHAPError_Unknown        Persisting the new cookie value fails.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetCookie(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        HAPPlatformWiFiManagerCookie cookie);

/**
 * Get the updateStatus value persisted in the WiFi configuration
 *
 * @param wiFiManager               WiFi manager
 * @param[out] updateStatus         Latest update Status value
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is some error in retreiving the update status.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetUpdateStatus(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        uint32_t* _Nonnull updateStatus);

/**
 * Sets the updateStatus value in the WiFi configuration to the new value passed in
 *
 * @param wiFiManager               WiFi manager
 * @param updateStatus              UpdateStatus value to be set in the WiFi configuration
 *
 * @return kHAPError_None           Persisting the new update status value was successful.
 * @return kHAPError_Unknown        Persisting the new update status value fails.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetUpdateStatus(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint32_t updateStatus);

/**
 * Checks to see if WiFi Link is established after reconfiguration of WiFi
 *
 * @param      wiFiManager          WiFi manager
 *
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);

/**
 * Checks to see if the WiFi network is configured and we are connected after reconfiguration of WiFi
 *
 * @param      wiFiManager          WiFi manager
 *
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager);
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
