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

#include "HAPPlatformWiFiManager+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "WiFiManager(Mock)" };

void HAPPlatformWiFiManagerCreate(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        const HAPPlatformWiFiManagerOptions* _Nonnull options HAP_UNUSED) {
    HAPPrecondition(wiFiManager);

    HAPRawBufferZero(wiFiManager, sizeof *wiFiManager);
}

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);

    return wiFiManager->isSSIDConfigured;
}

/**
 * Gets the regulatory domain in which the device is operating.
 *
 * - Regulatory domain information can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param      wiFiManager          WiFi manager.
 * @param[out] regulatoryDomain     Regulatory domain in which the device is operating.
 *
 * @return kHAPError_None           Success.
 */
HAPError HAPPlatformWiFiManagerGetRegulatoryDomain(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        HAPPlatformRegulatoryDomain* _Nonnull regulatoryDomain) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(regulatoryDomain);

    HAPRawBufferZero(regulatoryDomain, sizeof *regulatoryDomain);

    if (!wiFiManager->isRegulatoryDomainConfigured) {
        char* stringToCopy = "US";
        HAPRawBufferCopyBytes(regulatoryDomain->stringValue, stringToCopy, HAPStringGetNumBytes(stringToCopy));
    } else {
        size_t numRegulatoryDomainBytes = HAPStringGetNumBytes(wiFiManager->regulatoryDomain);
        HAPAssert(numRegulatoryDomainBytes < sizeof regulatoryDomain->stringValue);
        HAPRawBufferCopyBytes(regulatoryDomain->stringValue, wiFiManager->regulatoryDomain, numRegulatoryDomainBytes);
    }

    return kHAPError_None;
}

/**
 * Gets the SSID configured on the device.
 *
 * - SSID can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param      wiFiManager          WiFi manager.
 * @param[out] isConfigured         Indicates whether SSID is configured on the device.
 * @param[out] ssid                 Struct containing SSID of the WiFi network configured on this device.
 *
 * @return kHAPError_None           Success.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetSSID(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        bool* _Nonnull isConfigured,
        HAPPlatformSSID* _Nonnull ssid) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isConfigured);
    HAPPrecondition(ssid);

    if (wiFiManager->isSSIDConfigured) {
        HAPRawBufferCopyBytes((char*) ssid->bytes, wiFiManager->ssid, HAPPlatformSSID_MaxBytes);
    } else {
        char* stringToCopy = "AcmeTestSSID";
        size_t numBytes = HAPStringGetNumBytes(stringToCopy);
        HAPRawBufferCopyBytes((char*) ssid->bytes, stringToCopy, numBytes);
        ssid->numBytes = numBytes;
    }
    return kHAPError_None;
}

/**
 * Persists the WiFi credentials passed in.
 *
 * @param      wiFiManager          WiFi manager.
 * @param      ssid                 The SSID of the WPA/WPA2 personal WiFi network. If NULL, do not write the network
                                    info.
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
 * @return kHAPError_None           Success
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerApplyConfiguration(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        const char* _Nullable ssid,
        const char* _Nullable passphrase,
        const char* _Nullable regulatoryDomain
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
        ,
        HAPPlatformWiFiManagerCookie cookie,
        uint32_t updateStatus HAP_UNUSED,
        bool restartWiFi HAP_UNUSED
#endif
) {
    HAPPrecondition(wiFiManager);

    if (ssid && (HAPStringGetNumBytes(ssid) > 0)) {
        HAPLogInfo(
                &logObject,
                "%s - ssid = %s / passphrase = %s / regulatoryDomain = %s",
                __func__,
                ssid,
                passphrase,
                regulatoryDomain);

        HAPRawBufferZero(wiFiManager->ssid, sizeof wiFiManager->ssid);
        HAPRawBufferZero(wiFiManager->passphrase, sizeof wiFiManager->passphrase);
        HAPRawBufferZero(wiFiManager->regulatoryDomain, sizeof wiFiManager->regulatoryDomain);

        size_t numSSIDBytes = HAPStringGetNumBytes(ssid);
        HAPPrecondition(numSSIDBytes < sizeof wiFiManager->ssid);
        HAPRawBufferCopyBytes(wiFiManager->ssid, ssid, numSSIDBytes);
        wiFiManager->isSSIDConfigured = true;
        if (passphrase) {
            size_t numPassphraseBytes = HAPStringGetNumBytes(HAPNonnull(passphrase));
            HAPPrecondition(numPassphraseBytes < sizeof wiFiManager->passphrase);
            HAPRawBufferCopyBytes(wiFiManager->passphrase, HAPNonnull(passphrase), numPassphraseBytes);
            wiFiManager->isPassphraseConfigured = true;
        }
    }
    if (regulatoryDomain) {
        size_t numRegulatoryDomainBytes = HAPStringGetNumBytes(HAPNonnull(regulatoryDomain));
        HAPPrecondition(numRegulatoryDomainBytes < sizeof wiFiManager->regulatoryDomain);
        HAPRawBufferCopyBytes(wiFiManager->regulatoryDomain, HAPNonnull(regulatoryDomain), numRegulatoryDomainBytes);
        wiFiManager->isRegulatoryDomainConfigured = true;
    }
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
    if (cookie > 0) {
        wiFiManager->isCookieConfigured = true;
        wiFiManager->cookie = cookie;
    }
#endif
    return kHAPError_None;
}

void HAPPlatformWiFiManagerClearConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPlatformWiFiManagerRemoveConfiguration(wiFiManager);
}

void HAPPlatformWiFiManagerRemoveConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);

    HAPLogInfo(&logObject, "%s", __func__);

    wiFiManager->isSSIDConfigured = false;
    HAPRawBufferZero(wiFiManager->ssid, sizeof wiFiManager->ssid);
    wiFiManager->isPassphraseConfigured = false;
    HAPRawBufferZero(wiFiManager->passphrase, sizeof wiFiManager->passphrase);
    wiFiManager->isRegulatoryDomainConfigured = false;
    HAPRawBufferZero(wiFiManager->regulatoryDomain, sizeof wiFiManager->regulatoryDomain);
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerRestartWiFi(HAPPlatformWiFiManagerRef _Nonnull wiFiManager HAP_UNUSED) {
    // no-op
    return kHAPError_None;
}

/**
 * Sets psk configured to true if PSK is configured in the wpa supplicant, false otherwise.
 *
 *  - PSK can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param wiFiManager               WiFi manager.
 * @param[out] isPskConfigured      Set to false.
 *
 * @return kHAPError_None           Success.
 */
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerIsPSKConfigured(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        bool* _Nonnull isPskConfigured) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isPskConfigured);

    *isPskConfigured = false;
    return kHAPError_None;
}

/**
 * Get the cookie value persisted in the wpa supplicant
 *
 *  - Cookie can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param wiFiManager               WiFi manager
 * @param[out] cookie               Cookie value set to 0.
 *
 * @return kHAPError_None           Success.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetCookie(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint16_t* _Nonnull cookie) {
    HAPPrecondition(wiFiManager);
    if (wiFiManager->isCookieConfigured) {
        *cookie = wiFiManager->cookie;
    } else {
        *cookie = 0;
    }
    return kHAPError_None;
}

/**
 * Get the updateStatus value persisted in the wpa supplicant
 *
 *  - Update Status can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param wiFiManager               WiFi manager
 * @param[out] updateStatus         Update status set to 0.
 *
 * @return kHAPError_None           Success.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetUpdateStatus(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager HAP_UNUSED,
        uint32_t* _Nonnull updateStatus) {
    *updateStatus = 0;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerBackUpConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetUpdateStatus(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager HAP_UNUSED,
        uint32_t updateStatus HAP_UNUSED) {
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetCookie(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        HAPPlatformWiFiManagerCookie cookie) {
    wiFiManager->cookie = cookie;
    return kHAPError_None;
}
#endif

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPPlatformWiFiManagerRef _Nonnull wiFiManager HAP_UNUSED) {
    return false;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager HAP_UNUSED) {
    return false;
}
