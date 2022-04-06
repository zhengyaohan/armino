

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
#include "HAPPlatformWiFiManager+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"

#include "bk_api_mem.h"
#include "bk_api_rtos.h"
#include "bk_api_str.h"


#include "wlan_ui_pub.h"
#include "bk_wifi_wrapper.h"










static const HAPLogObject logObject =
{
    .subsystem = kHAPPlatform_LogSubsystem, .category = "WiFiManager"
};


#define kHAPPlatformWiFiManager_DefaultInterfaceName "wlan0"



/**
 ************************************************************
 * @brief HAPPlatformWiFiManagerConfigSave.
 *
 * @param[in] 
 * @param[out] 
 * @return 
 *************************************************************
 */
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerConfigSave(HAPPlatformWiFiManagerRef wiFiManager, 
    const char * _Nonnull ssid, 
    const char * _Nullable passphrase, 
    const char * _Nullable regulatoryDomain

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION) 
, 
    HAPPlatformWiFiManagerCookie cookie, 
    uint32_t updateStatus
#endif

)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(ssid);
    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    HAPError err;

    if (ssid)
    {
        wiFiManager->isSSIDConfigured = 1;
        os_strcpy(wiFiManager->ssid, ssid);
        err = HAPPlatformKeyValueStoreSet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
            kSDKKeyValueStoreKey_WiFiManager_ssid, 
            (void *) ssid, os_strlen(ssid) + 1);
        HAPAssert(err == kHAPError_None);
    }
    if (passphrase)
    {
        wiFiManager->isPassphraseConfigured = 1;
        os_strcpy(wiFiManager->passphrase, passphrase);
        err = HAPPlatformKeyValueStoreSet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
            kSDKKeyValueStoreKey_WiFiManager_psk, 
            (void *) passphrase, os_strlen(passphrase) + 1);
        HAPAssert(err == kHAPError_None);
    }


    if (regulatoryDomain)
    {
        wiFiManager->isRegulatoryDomainConfigured = 1;
        os_strcpy(wiFiManager->regulatoryDomain, regulatoryDomain);
        err = HAPPlatformKeyValueStoreSet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
            kSDKKeyValueStoreKey_WiFiManager_country, 
            (void *) regulatoryDomain, os_strlen(regulatoryDomain) + 1);
        HAPAssert(err == kHAPError_None);
    }

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION) 

    wiFiManager->isCookieConfigured = 1;
    wiFiManager->cookie = cookie;
    err = HAPPlatformKeyValueStoreSet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_cookie, 
        (void *) &cookie, sizeof(cookie));
    HAPAssert(err == kHAPError_None);

    err = HAPPlatformKeyValueStoreSet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_status, 
        (void *) &updateStatus, sizeof(updateStatus));
    HAPAssert(err == kHAPError_None);

#endif

    return err;
}



/**
 ************************************************************
 * @brief HAPPlatformWiFiManagerGetSSID.
 *
 * @param[in] 
 * @param[out] 
 * @return 
 *************************************************************
 */
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerGetSSID(HAPPlatformWiFiManagerRef wiFiManager, 
    bool * isConfigured, 
    HAPPlatformSSID * ssid)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isConfigured);
    HAPPrecondition(ssid);
    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    uint32 size;
    HAPError err;

    HAPRawBufferZero(ssid, sizeof * ssid);

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_ssid, 
        (void *) ssid->stringValue, sizeof(ssid->stringValue), &size, isConfigured);
    if (*isConfigured)
    {
        ssid->numBytes = os_strlen(ssid->stringValue);
        os_memcpy(ssid->bytes, ssid->stringValue, ssid->numBytes);
    }
    return err;
}





/**
 ************************************************************
 * @brief HAPPlatformWiFiManagerGetRegulatoryDomain.
 *
 * @param[in] 
 * @param[out] 
 * @return 
 *************************************************************
*/
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerGetRegulatoryDomain(HAPPlatformWiFiManagerRef wiFiManager, 
    HAPPlatformRegulatoryDomain * regulatoryDomain)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(regulatoryDomain);
    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    bool found;
    uint32 size;
    HAPError err;

    HAPRawBufferZero(regulatoryDomain, sizeof * regulatoryDomain);

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_country, 
        (void *) regulatoryDomain->stringValue, sizeof(regulatoryDomain->stringValue), &size, &found);
    if ((err !=kHAPError_None) ||( !found))
    {
        os_strcpy(regulatoryDomain->stringValue,"CN");
    }

    return kHAPError_None;


}



void HAPPlatformWiFiManagerCreate(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    const HAPPlatformWiFiManagerOptions * _Nonnull options)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(options);
    HAPPrecondition(options->keyValueStore);

    HAPLogDebug(&logObject, "Storage configuration: wiFiManager = %lu", (unsigned long) sizeof * wiFiManager);

    HAPRawBufferZero(wiFiManager, sizeof * wiFiManager);

    const char * interfaceName =
         options->interfaceName ? options->interfaceName: kHAPPlatformWiFiManager_DefaultInterfaceName;
    size_t numInterfaceNameBytes = HAPStringGetNumBytes(interfaceName);

    if ((numInterfaceNameBytes == 0) || (numInterfaceNameBytes >= sizeof wiFiManager->interfaceName))
    {
        HAPLogError(&logObject, "Invalid local network interface name.");
        HAPFatalError();
    }
    HAPRawBufferCopyBytes(wiFiManager->interfaceName, interfaceName, numInterfaceNameBytes);
    wiFiManager->keyValueStore = options->keyValueStore;

}


HAP_RESULT_USE_CHECK HAPPlatformWiFiManagerCapabilities HAPPlatformWiFiManagerGetCapabilities(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    HAPPrecondition(wiFiManager);

    return (HAPPlatformWiFiManagerCapabilities)
    {
        .supports2_4GHz = true, .supports5GHz = false
    };
}


HAP_RESULT_USE_CHECK bool HAPPlatformWiFiManagerIsConfigured(HAPPlatformWiFiManagerRef wiFiManager)
{
    HAPPrecondition(wiFiManager);

    bool isConfigured;
    HAPPlatformSSID ssid;

    HAPError err = HAPPlatformWiFiManagerGetSSID(wiFiManager, &isConfigured, &ssid);

    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        return kHAPError_Unknown;
    }

    return isConfigured;
}

HAP_RESULT_USE_CHECK bool HAPPlatformWiFiManagerIsDHCP(HAPPlatformWiFiManagerRef wiFiManager)
{
    HAPPrecondition(wiFiManager);

    bool isConfigured=0;
    HAPPlatformSSID ssid;
    size_t size;


    HAPPlatformKeyValueStoreGet(wiFiManager->keyValueStore, kSDKKeyValueStoreDomain_WiFiManager, 
            kSDKKeyValueStoreKey_WiFiManager_dhcp, 
            (void *) ssid.stringValue, sizeof(ssid.stringValue), &size, &isConfigured);
    return isConfigured;
}



/**
 * Persists the WiFi credentials passed in.
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
 * @return kHAPError_None           Success
 */
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerApplyConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    const char * _Nonnull ssid, 
    const char * _Nullable passphrase, 
    const char * _Nullable regulatoryDomain

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION) 
, 
    HAPPlatformWiFiManagerCookie cookie, 
    uint32_t updateStatus, 
    bool restartWiFi
#endif

)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(ssid);
    HAPError err=kHAPError_None;

    HAPPlatformWiFiManagerConfigSave(wiFiManager, ssid, passphrase, regulatoryDomain

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION) 
    , 
        cookie, 
        updateStatus
#endif

);

    // Restart WiFi interface
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION) 
    if (restartWiFi)
    {
        err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
    }

#else

    err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
#endif

    return err;
}


void HAPPlatformWiFiManagerClearConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    HAPPrecondition(wiFiManager);

    HAPPlatformKeyValueStorePurgeDomain(wiFiManager->keyValueStore, kSDKKeyValueStoreDomain_WiFiManager);
}


void HAPPlatformWiFiManagerRemoveConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    HAPPrecondition(wiFiManager);

    HAPPlatformKeyValueStorePurgeDomain(wiFiManager->keyValueStore, kSDKKeyValueStoreDomain_WiFiManager);
}


HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerRestartWiFi(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    if (wiFiManager->isSSIDConfigured)
    {
        bk_wlan_stop(BK_STATION);
        if( wiFiManager->isPassphraseConfigured && (os_strlen(wiFiManager->passphrase) >=8) )
            if(HAPPlatformWiFiManagerIsDHCP(wiFiManager))
                demo_sta_app_init(wiFiManager->ssid, wiFiManager->passphrase);
            else
                demo_sta_app_init(wiFiManager->ssid, wiFiManager->passphrase);
        else
            demo_sta_app_init(wiFiManager->ssid, "1");
    }
    else 
        bk_printf("!!!wiFiManager->isSSIDConfigured not config \r\n");
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
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerIsPSKConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    bool * _Nonnull isPskConfigured)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isPskConfigured);

    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    uint32 size;
    HAPError err;
    char passphrase[64 + 1];

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_psk, 
        (void *) passphrase, sizeof(passphrase), &size, isPskConfigured);
    return err;

}


/**
 ************************************************************
 * @brief HAPPlatformWiFiManagerGetPSK.
 *
 * @param[in] 
 * @param[out] 
 * @return 
 *************************************************************
*/
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerGetPSK(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    bool * isPskConfigured, char * _Nonnull psk)
{

    HAPPrecondition(wiFiManager);
    HAPPrecondition(psk);
    HAPPrecondition(isPskConfigured);

    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    uint32 size;
    HAPError err;
    char passphrase[64 + 1];

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_psk, 
        (void *) passphrase, sizeof(passphrase), &size, isPskConfigured);
    if (*isPskConfigured)
    {
        os_strcpy(psk, passphrase);
    }
    return err;

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
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerGetCookie(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    uint16_t * _Nonnull cookie)
{

    HAPPrecondition(wiFiManager);
    HAPPrecondition(cookie);

    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    bool found;
    uint32 size;
    HAPError err;

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_cookie, 
        (void *) cookie, sizeof(*cookie), &size, &found);
    if (found)
    {
        err = kHAPError_None;
    }
    else 
        err = kHAPError_Unknown;
    return err;

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
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerGetUpdateStatus(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    uint32_t * _Nonnull updateStatus)
{
    HAPPrecondition(wiFiManager);
    HAPPrecondition(updateStatus);

    HAPPlatformKeyValueStore * key_value = wiFiManager->keyValueStore;
    bool found;
    uint32 size;
    HAPError err;

    err = HAPPlatformKeyValueStoreGet(key_value, kSDKKeyValueStoreDomain_WiFiManager, 
        kSDKKeyValueStoreKey_WiFiManager_status, 
        (void *) updateStatus, sizeof(*updateStatus), &size, &found);
    if (found)
    {
        err = kHAPError_None;
    }
    else 
        err = kHAPError_Unknown;
    return err;

}


HAP_RESULT_USE_CHECK bool HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    LinkStatusTypeDef outStatus;

    os_memset((void *)&outStatus, 0, sizeof(outStatus));
    bk_wlan_get_link_status(&outStatus);
    return outStatus.conn_state;
}


HAP_RESULT_USE_CHECK bool HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{

    return HAPPlatformWiFiManagerIsWiFiLinkEstablished(wiFiManager);

}


HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerBackUpConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager)
{
    HAPPrecondition(wiFiManager);

    return kHAPError_None;
}


HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerSetUpdateStatus(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint32_t updateStatus)
{
    HAPPrecondition(wiFiManager);

    HAPPlatformWiFiManagerCookie cookie = kHAPPlatformWiFiManagerCookie_Unmanaged;
    HAPError err = HAPPlatformWiFiManagerGetCookie(HAPNonnull(wiFiManager), &cookie);
    if (err)
    {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: cookie not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformRegulatoryDomain regulatoryDomain;
    err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Regulatory domain has not been configured in wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformSSID ssid;
    bool isSSIDConfigured;
    err = HAPPlatformWiFiManagerGetSSID(HAPNonnull(wiFiManager), &isSSIDConfigured, &ssid);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    char psk[2 * kHAPWiFiWPAPSK_NumBytes + 1] =
    {
        0
    };

    bool isPSKConfigured;
    err = HAPPlatformWiFiManagerGetPSK(HAPNonnull(wiFiManager), &isPSKConfigured, psk);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: PSK not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    bool isRegulatoryDomainConfigured = HAPStringGetNumBytes(regulatoryDomain.stringValue) != 0;
    bool Restart_wifi=0;
    if(updateStatus == 0)
        Restart_wifi = 1;
    err = HAPPlatformWiFiManagerApplyConfiguration(HAPNonnull(wiFiManager), 
        ssid.stringValue, 
        isPSKConfigured ? psk: NULL, 
        isRegulatoryDomainConfigured ? regulatoryDomain.stringValue: NULL, 
        cookie, 
        updateStatus, 
        Restart_wifi);
    return err;

}


/**
 * Set the cookie value to be persisted in the wpa supplicant
 *
 * @param wiFiManager               WiFi manager
 * @param cookie                    Cookie value to be updated in the wpa supplicant
 *
 * @return kHAPError_None           Persisting the new cookie value was successful.
 * @return kHAPError_Unknown        Persisting the new cookie value fails.
 */
HAP_RESULT_USE_CHECK HAPError HAPPlatformWiFiManagerSetCookie(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, 
    HAPPlatformWiFiManagerCookie cookie)
{
    HAPPrecondition(wiFiManager);

    uint32_t updateStatus = 0;
    HAPError err = HAPPlatformWiFiManagerGetUpdateStatus(HAPNonnull(wiFiManager), &updateStatus);
    if (err)
    {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Update status not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformRegulatoryDomain regulatoryDomain;
    err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Regulatory domain has not been configured in wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformSSID ssid;
    bool isSSIDConfigured;
    err = HAPPlatformWiFiManagerGetSSID(HAPNonnull(wiFiManager), &isSSIDConfigured, &ssid);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    char psk[2 * kHAPWiFiWPAPSK_NumBytes + 1] =
    {
        0
    };

    bool isPSKConfigured;
    err = HAPPlatformWiFiManagerGetPSK(HAPNonnull(wiFiManager), &isPSKConfigured, psk);
    if (err)
    {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: PSK not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    bool isRegulatoryDomainConfigured = HAPStringGetNumBytes(regulatoryDomain.stringValue) != 0;

    err = HAPPlatformWiFiManagerApplyConfiguration(HAPNonnull(wiFiManager), 
        ssid.stringValue, 
        isPSKConfigured ? psk: NULL, 
        isRegulatoryDomainConfigured ? regulatoryDomain.stringValue: NULL, 
        cookie, 
        updateStatus, 
        false);
    return err;
}



#endif

