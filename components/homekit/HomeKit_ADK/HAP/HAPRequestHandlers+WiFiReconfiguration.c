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

#include "HAP+API.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCharacteristic.h"
#include "HAPCharacteristicTypes+TLV.h"
#include "HAPIPAccessoryServer.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"
#include "HAPServiceTypes.h"
#include "HAPTLV+Internal.h"
#include "HAPWACEngine.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "RequestHandlers" };

/**
 * This is the maximum length that the current transport characteristics of the WiFi transport service can have.
 * Current transport values - "Ethernet", "WiFi", "None"
 */
#define CURRENT_TRANSPORT_MAX_SIZE 10

/**
 * Time to wait for the simple/fail safe update callback to be called to ensure the write response is sent before
 */
#define UPDATE_WIFI_CONFIGURATION_TIMEOUT HAPSecond

/**
 * Time to wait for the fail safe update to timeout
 */
#define FAILSAFE_UPDATE_TIMEOUT 60

/**
 * Timeout to poll the WiFi status (WiFi connected, ip address received) after the fail safe update configuration
 */
#define CHECK_WIFI_STATUS_RETRY_TIMEOUT 6 * HAPSecond

/**
 * Time to wait for the simple update clear configuration callback to be called to ensure the write response is sent
 */
#define SIMPLE_UPDATE_CLEAR_CONFIGURATION_TIMEOUT 5 * HAPSecond

/**
 * Maximum number of retries to poll the WiFi status after the fail safe update configuration
 */
#define CHECK_WIFI_STATUS_MAX_RETRIES 3

/**
 * Flag to track if WiFi status has been processed either by the WiFi status polling timer already or needs to be
 * processed by the fail safe update timer callback.
 */
bool isWiFiStatusProcessed = false;

bool commitMessageReceived = false;

HAPPlatformTimerRef handleSimpleUpdateClearConfigurationTimer;

//---------------------------------------------------------------------------------------------------------------------
/**
 * Handles read operation of current transport of the accessory via the WiFi Transport service
 *
 * @param      server          Accessory server
 * @param      request         Read request for the current transport
 * @param[out] value           True if WiFi is the current transport, false otherwise
 * @param      context
 *
 * @return kHAPError_None if the read operation succeeds
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiTransportCurrentTransportRead(
        HAPAccessoryServer* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.ip.tcpStreamManager);
    HAPPrecondition(server->platform.ip.wiFi.wiFiManager);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_CurrentTransport));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiTransport));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);
    *value = HAPPlatformTCPStreamManagerIsWiFiCurrentTransport(server->platform.ip.tcpStreamManager);
    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------
/**
 * Handles read operation of WiFi capabilities for accessory via the WiFi Transport service
 *
 * @param      server          Accessory server
 * @param      request         Read request for the WiFi capabilities
 * @param[out] value           HAPPlatformWiFiCapability (flags set - supports 2.4 GHz, 5 GHz, station mode or wake on
 * WLAN)
 * @param      context
 *
 * @return kHAPError_None if the read operation succeeds
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiTransportWiFiCapabilityRead(
        HAPAccessoryServer* server,
        const HAPUInt32CharacteristicReadRequest* request,
        uint32_t* value,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(
            HAPUUIDAreEqual(request->characteristic->characteristicType, &kHAPCharacteristicType_WiFiCapability));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiTransport));
    HAPPrecondition(request->accessory->aid == 1);
    HAPPrecondition(value);

    // Get WiFi capabilities.
    HAPPlatformWiFiCapability wiFiCapability =
            HAPPlatformTCPStreamManagerGetWiFiCapability(server->platform.ip.tcpStreamManager);
    if ((!wiFiCapability.supports2_4GHz) && (!wiFiCapability.supports5GHz)) {
        HAPLogCharacteristicError(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "%s returned invalid WiFi capabilities: WiFi manager must support joining 2.4 GHz or 5 GHz networks.",
                "HAPPlatformTCPStreamManagerGetWiFiCapability");
        HAPFatalError();
    }

    HAPCharacteristicValue_WiFiCapability wiFiCapabilityValue = 0;
    HAPAssert(sizeof(HAPCharacteristicValue_WiFiCapability) == sizeof(uint32_t));
    if (wiFiCapability.supports2_4GHz) {
        wiFiCapabilityValue |= (uint32_t) kHAPCharacteristicValue_WiFiCapability_Supports2_4GHz;
    }
    if (wiFiCapability.supports5GHz) {
        wiFiCapabilityValue |= (uint32_t) kHAPCharacteristicValue_WiFiCapability_Supports5GHz;
    }

    // Station mode is not supported for WiFi Routers
    if ((server->transports.ip != NULL) && (HAPNonnull(server->primaryAccessory)) &&
        (server->primaryAccessory->category != kHAPAccessoryCategory_WiFiRouters)) {
        wiFiCapabilityValue |= (uint32_t) kHAPCharacteristicValue_WiFiCapability_SupportsStationMode;
    }

#if HAP_LOG_LEVEL >= HAP_LOG_LEVEL_INFO
    {
        char logBytes[1024];
        HAPStringBuilder stringBuilder;
        HAPStringBuilderCreate(&stringBuilder, logBytes, sizeof logBytes);
        HAPStringBuilderAppend(&stringBuilder, "WiFi Capability: [");
        bool needsSeparator = false;
        if (wiFiCapability.supports2_4GHz) {
            HAPStringBuilderAppend(&stringBuilder, "Supports 2.4 GHz");
            needsSeparator = true;
        }
        if (wiFiCapability.supports5GHz) {
            if (needsSeparator) {
                HAPStringBuilderAppend(&stringBuilder, ", ");
            }
            HAPStringBuilderAppend(&stringBuilder, "Supports 5 GHz");
        }
        if (server->transports.ip != NULL) {
            if (needsSeparator) {
                HAPStringBuilderAppend(&stringBuilder, ", ");
            }
            HAPStringBuilderAppend(&stringBuilder, "Supports station mode");
        }
        HAPStringBuilderAppend(&stringBuilder, "]");
        if (HAPStringBuilderDidOverflow(&stringBuilder)) {
            HAPLogCharacteristicError(
                    &logObject, request->characteristic, request->service, request->accessory, "Logs were truncated.");
        }
        HAPLogCharacteristicInfo(
                &logObject,
                request->characteristic,
                request->service,
                request->accessory,
                "%s",
                HAPStringBuilderGetString(&stringBuilder));
    }
#endif

    *value = (uint32_t) wiFiCapabilityValue;

    return kHAPError_None;
}

/**
 * Returns true if the operation type is a Read configuration request
 *
 * @param operationType          Operation type of the WiFi reconfiguration request
 *
 */
HAP_RESULT_USE_CHECK
static bool IsReadConfigurationOperation(HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType) {
    return (operationType == kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read);
}

/**
 * Returns true if the operation type is an Update configuration request or a commit configuration request
 *
 * @param operationType          Operation type of the WiFi reconfiguration request
 *
 */
HAP_RESULT_USE_CHECK
static bool IsUpdateConfigurationOperationType(
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType operationType) {
    return (operationType == kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple) ||
           (operationType == kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe) ||
           (operationType == kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit);
}

/**
 * Handles read operation of WiFi configuration control characteristics of the accessory via the WiFi Transport service
 *
 * @param      server             Accessory server
 * @param      request            Read request for the WiFi configuration control characteristics
 * @param[out] responseWriter     The write response writer that encodes the TLV and returns the value of this TLV
 * @param      context
 *
 * @return kHAPError_None         If the read operation succeeds
 * @return kHAPError_Unknown      If the read operation fails
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiTransportWiFiConfigurationControlRead(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriter* responseWriter,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_WiFiConfigurationControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiTransport));
    HAPPrecondition(responseWriter);

    HAPCharacteristicValue_WiFiConfigurationControl wiFiConfigurationControl;
    HAPRawBufferZero(&wiFiConfigurationControl, sizeof wiFiConfigurationControl);

    wiFiConfigurationControl.operationTypeIsSet = false;
    HAPError err = HAPPlatformWiFiManagerGetUpdateStatus(
            HAPNonnull(server->platform.ip.wiFi.wiFiManager), &wiFiConfigurationControl.updateStatus);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
        return kHAPError_Unknown;
    } else {
        wiFiConfigurationControl.updateStatusIsSet = true;
    }

    wiFiConfigurationControl.stationConfigIsSet = false;
    err = HAPPlatformWiFiManagerGetCookie(
            HAPNonnull(server->platform.ip.wiFi.wiFiManager), &wiFiConfigurationControl.cookie);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: cookie not read successfully from WiFi configuration.", __func__);
        return kHAPError_Unknown;
    } else {
        wiFiConfigurationControl.cookieIsSet = true;
    }

    HAPPlatformRegulatoryDomain regulatoryDomain;
    HAPPlatformSSID ssid;

    if (IsReadConfigurationOperation(server->ip.wiFiReconfiguration.operationType)) {
        server->ip.wiFiReconfiguration.operationType =
                kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid;
        bool isConfigured;
        err = HAPPlatformWiFiManagerGetSSID(HAPNonnull(server->platform.ip.wiFi.wiFiManager), &isConfigured, &ssid);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
            HAPLogError(&logObject, "%s: SSID not read successfully from WiFi configuration.", __func__);
            return kHAPError_Unknown;
        } else {
            if (isConfigured && (ssid.numBytes > 0)) {
                wiFiConfigurationControl.stationConfig.ssid = ssid.stringValue;
                wiFiConfigurationControl.stationConfig.ssidIsSet = true;
            }
        }
        if (wiFiConfigurationControl.stationConfig.ssidIsSet) {
            bool isPSKConfigured = false;

            err = HAPPlatformWiFiManagerIsPSKConfigured(
                    HAPNonnull(server->platform.ip.wiFi.wiFiManager), &isPSKConfigured);
            if (err) {
                HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
                HAPLogError(&logObject, "%s: PSK not read successfully from WiFi configuration.", __func__);
                return kHAPError_Unknown;
            }
            wiFiConfigurationControl.stationConfig.securityMode =
                    isPSKConfigured ?
                            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK :
                            kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_None;
            wiFiConfigurationControl.stationConfig.securityModeIsSet = true;
            // If PSK is present, send an empty string to signify the presence of it but not send the actual PSK.
            if (isPSKConfigured) {
                wiFiConfigurationControl.stationConfig.psk = "";
                wiFiConfigurationControl.stationConfig.pskIsSet = true;
            }
            wiFiConfigurationControl.stationConfigIsSet = true;
        }

        err = HAPPlatformWiFiManagerGetRegulatoryDomain(
                HAPNonnull(server->platform.ip.wiFi.wiFiManager), &regulatoryDomain);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
            HAPLogError(&logObject, "%s: Regulatory domain not read successfully from WiFi configuration.", __func__);
            return kHAPError_Unknown;
        } else {
            wiFiConfigurationControl.countryCode = regulatoryDomain.stringValue;
            wiFiConfigurationControl.countryCodeIsSet = true;
        }
    }

    err = HAPTLVWriterEncode(
            responseWriter, &kHAPCharacteristicTLVFormat_WiFiConfigurationControl, &wiFiConfigurationControl);
    if (err) {
        HAPLogError(&logObject, "TLV Writer failed to encode WiFi Configuration.");
        return kHAPError_Unknown;
    }
    return kHAPError_None;
}

/**
 * Processes the reconfiguration of the WiFi interface after the WiFi credentials are persisted in the accessory for
 * WiFi Reconfiguration. Reconfigures the WiFi interface and starts TCP and HAP sessions as well as registers
 * for Bonjour service discovery
 *
 * @param server          Accessory server reference
 *
 */
HAP_RESULT_USE_CHECK
HAPError ReconfigureWiFi(HAPAccessoryServer* server) {
    HAPPrecondition(server);
    HAPError err = kHAPError_None;

    HAPLogInfo(&logObject, "Restart WiFi after applying WiFi configuration.");
    err = HAPPlatformWiFiManagerRestartWiFi(server->platform.ip.wiFi.wiFiManager);
    return err;
}

/**
 * Persists the WiFi credentials in the accessory for WiFi Reconfiguration (simple and fail safe update)
 *
 * @param server                    Accessory server
 * @param wiFiConfigurationControl  Payload of the request for simple and fail safe update WiFi reconfiguration
 *
 * @return kHAPError_None           Persisting the configuration was successful.
 * @return kHAPError_Unknown        Persisting the configuration fails.
 * @return kHAPError_OutOfResources Restarting wifi fails as the buffer passed to store results is small.
 */
HAP_RESULT_USE_CHECK
HAPError PersistWiFiCredentials(
        HAPAccessoryServer* server,
        HAPCharacteristicValue_WiFiConfigurationControl wiFiConfigurationControl) {
    if (wiFiConfigurationControl.countryCodeIsSet) {
        HAPRawBufferZero(
                &server->ip.wiFiReconfiguration.countryCode, sizeof server->ip.wiFiReconfiguration.countryCode);
        HAPRawBufferCopyBytes(
                server->ip.wiFiReconfiguration.countryCode,
                wiFiConfigurationControl.countryCode,
                HAPStringGetNumBytes(wiFiConfigurationControl.countryCode));
    }
    HAPError err;

    server->ip.wiFiReconfiguration.stationConfig.ssidIsSet =
            wiFiConfigurationControl.stationConfig.ssidIsSet && wiFiConfigurationControl.stationConfigIsSet;
    if (server->ip.wiFiReconfiguration.stationConfig.ssidIsSet) {
        HAPRawBufferZero(
                &server->ip.wiFiReconfiguration.stationConfig.ssid,
                sizeof server->ip.wiFiReconfiguration.stationConfig.ssid);
        HAPRawBufferCopyBytes(
                server->ip.wiFiReconfiguration.stationConfig.ssid,
                wiFiConfigurationControl.stationConfig.ssid,
                HAPStringGetNumBytes(wiFiConfigurationControl.stationConfig.ssid));
        if (wiFiConfigurationControl.stationConfig.securityModeIsSet) {
            server->ip.wiFiReconfiguration.stationConfig.securityMode =
                    wiFiConfigurationControl.stationConfig.securityMode;
            if (server->ip.wiFiReconfiguration.stationConfig.securityMode ==
                kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK) {
                HAPRawBufferZero(
                        &server->ip.wiFiReconfiguration.stationConfig.psk,
                        sizeof server->ip.wiFiReconfiguration.stationConfig.psk);
                HAPRawBufferCopyBytes(
                        server->ip.wiFiReconfiguration.stationConfig.psk,
                        wiFiConfigurationControl.stationConfig.psk,
                        HAPStringGetNumBytes(wiFiConfigurationControl.stationConfig.psk));
            }
        }
        bool isWiFiSecured = (server->ip.wiFiReconfiguration.stationConfig.securityMode ==
                              kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK) &&
                             HAPStringGetNumBytes(server->ip.wiFiReconfiguration.stationConfig.psk) != 0;
        bool isRegulatoryDomainConfigured = HAPStringGetNumBytes(server->ip.wiFiReconfiguration.countryCode) != 0;
        err = HAPPlatformWiFiManagerApplyConfiguration(
                HAPNonnull(server->platform.ip.wiFi.wiFiManager),
                server->ip.wiFiReconfiguration.stationConfig.ssid,
                isWiFiSecured ? server->ip.wiFiReconfiguration.stationConfig.psk : NULL,
                isRegulatoryDomainConfigured ? server->ip.wiFiReconfiguration.countryCode : NULL,
                server->ip.wiFiReconfiguration.cookie,
                server->ip.wiFiReconfiguration.updateStatus,
                false);
    } else {
        err = HAPPlatformWiFiManagerSetCookie(
                HAPNonnull(server->platform.ip.wiFi.wiFiManager), server->ip.wiFiReconfiguration.cookie);
    }
    return err;
}

/**
 * Check if the bits set in the mask are set in the updateStatus value. Returns true if set, false otherwise
 *
 * @param updateStatus          UpdateStatus value to check for the bit mask(s)
 * @param mask                  Mask with bit(s) to be checked in updateStatus
 *
 */
HAP_RESULT_USE_CHECK
bool IsUpdateStatusBitSet(uint32_t updateStatus, uint32_t mask) {
    return (updateStatus & mask) == mask;
}

/**
 * Sets the bits enabled in the mask in the updateStatus value in the accessory server cache. Persists the new
 * update Status value in the accessory
 *
 * @param server          Accessory server
 * @param mask            Mask with bit(s) to be set in updateStatus
 *
 */
void SetUpdateStatusBit(HAPAccessoryServer* server, uint32_t mask) {
    HAPPrecondition(server);
    if (!IsUpdateStatusBitSet(server->ip.wiFiReconfiguration.updateStatus, mask)) {
        server->ip.wiFiReconfiguration.updateStatus |= mask;
        HAPError err = HAPPlatformWiFiManagerSetUpdateStatus(
                HAPNonnull(server->platform.ip.wiFi.wiFiManager), server->ip.wiFiReconfiguration.updateStatus);
        if (err) {
            HAPLogError(&logObject, "Failed to update the WiFi configuration");
        }
    }
}

/**
 * Clears the bits enabled in the clearBitMask and sets the bits enabled in setBitMask in the updateStatus value in
 * the accessory server cache. Persists the new update Status value in the accessory
 *
 * @param server          Accessory server
 * @param clearBitMask    Mask with bit(s) to be cleared in updateStatus
 * @param setBitMask      Mask with bit(s) to be set in updateStatus
 *
 */
void ChangeUpdateStatusBits(HAPAccessoryServer* server, uint32_t clearBitMask, uint32_t setBitMask) {
    HAPPlatformWiFiManagerCookie cookie = server->ip.wiFiReconfiguration.updateStatus & 0x0000ffff;
    server->ip.wiFiReconfiguration.updateStatus &= ~(clearBitMask);
    server->ip.wiFiReconfiguration.updateStatus |= cookie;

    if (!IsUpdateStatusBitSet(server->ip.wiFiReconfiguration.updateStatus, setBitMask)) {
        server->ip.wiFiReconfiguration.updateStatus |= setBitMask;
    }

    HAPError err = HAPPlatformWiFiManagerSetUpdateStatus(
            HAPNonnull(server->platform.ip.wiFi.wiFiManager), server->ip.wiFiReconfiguration.updateStatus);
    if (err) {
        HAPLogError(&logObject, "Failed to update the WiFi configuration");
    }
}

/**
 * Clears all the other flags and sets the success flag in updateStatus. Persists the new update Status value
 *
 * @param server          Accessory server
 *
 * @param setBitMask      Mask to set. 0 to just set the success bit
 */
void ResetUpdateStatusOnSuccess(HAPAccessoryServer* server, int setBitMask) {
    HAPPlatformWiFiManagerCookie cookie = server->ip.wiFiReconfiguration.updateStatus & 0x0000ffff;
    if (!IsUpdateStatusBitSet(
                server->ip.wiFiReconfiguration.updateStatus,
                kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Success)) {
        server->ip.wiFiReconfiguration.updateStatus |=
                kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Success;
    }
    // Clear all other flags
    server->ip.wiFiReconfiguration.updateStatus &=
            kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Success;

    server->ip.wiFiReconfiguration.updateStatus |= (cookie | setBitMask);

    HAPError err = HAPPlatformWiFiManagerSetUpdateStatus(
            HAPNonnull(server->platform.ip.wiFi.wiFiManager), server->ip.wiFiReconfiguration.updateStatus);
    if (err) {
        HAPLogError(&logObject, "Failed to update the WiFi configuration");
    }
}

/**
 * Check if restart is required. This is set if the current transport is WiFi and we are trying to reconfigure the
 * WiFi using WiFi Reconfiguration
 *
 * @param server          Accessory server
 * @param updateStatus    UpdateStatus value
 *
 */
HAP_RESULT_USE_CHECK
bool IsRestartRequired(uint32_t updateStatus) {
    return IsUpdateStatusBitSet(
            updateStatus, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Restart_Required);
}

/**
 * Gets the WiFi status (if WiFi is connected or ip address is obtained) when the WiFi status update timer expires
 * Set the appropriate bits in the updateStatus and persist it in the accessory.
 *
 * @param context                      Data passed into the timer callback
 * @param[out] isWiFiLinkEstablished   Set to true if WiFi is connected, false otherwise
 * @param[out] isWiFiNetworkConfigured Set to true if ip address is obtained, false otherwise
 *
 */
void GetWiFiStatusForFailSafeUpdate(
        void* _Nullable context,
        bool* isWiFiLinkEstablished,
        bool* isWiFiNetworkConfigured) {
    HAPAccessoryServer* const server = context;

    *isWiFiLinkEstablished =
            HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    if (*isWiFiLinkEstablished) {
        SetUpdateStatusBit(server, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Link_Established);
    }

    *isWiFiNetworkConfigured =
            HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    if (*isWiFiNetworkConfigured) {
        SetUpdateStatusBit(server, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Network_Configured);
    }
}

/**
 * Handles failure for the fail safe update. Sets/clears the appropriate flags and rolls back to the previous WiFi
 * credentials
 *
 * @param      server          Accessory server
 *
 */
void HandleFailSafeUpdateFailed(HAPAccessoryServer* server) {
    HAPLogDebug(&logObject, "Reconfiguring WiFi (fail safe update) failed. Restoring previous WiFi credentials");

    HAPPlatformWiFiManagerRemoveConfiguration(HAPNonnull(server->platform.ip.wiFi.wiFiManager));

    HAPAccessoryServerHandleWiFiReconfigurationDone(server);

    HAPError err = HAPPlatformWiFiManagerSetCookie(
            HAPNonnull(server->platform.ip.wiFi.wiFiManager), server->ip.wiFiReconfiguration.cookie);
    if (err) {
        HAPLogDebug(&logObject, "Reconfiguring WiFi (fail safe update) failed. Failed to update cookie");
    }

    uint32_t mask = kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Success |
                    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Pending |
                    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Restart_Required;
    // Clear the Update Pending bit and set the failed bit in the update status and rollback the configuration
    ChangeUpdateStatusBits(server, mask, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Failed);
    isWiFiStatusProcessed = true;
    server->ip.wiFiReconfiguration.operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid;
}

/**
 * Handles success for the fail safe update. Sets/clears the appropriate flags.
 *
 * @param      server          Accessory server
 *
 */
void HandleFailSafeUpdateSuccess(HAPAccessoryServer* server) {
    HAPLogDebug(&logObject, "Reconfiguring WiFi (fail safe update) was successful.");

    // Clear all other flags and set the Success bit in the update status
    ResetUpdateStatusOnSuccess(server, 0);
    isWiFiStatusProcessed = true;
    server->ip.wiFiReconfiguration.operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid;
}

/**
 * Callback that is called to poll the WiFi status after WiFi reconfiguration is complete. There are maximum 3 retries
 * to get the WiFi status and set the appropriate flags in the updateStatus.
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void OnCheckWiFiStatusTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;

    HAPPrecondition(timer == server->ip.checkWiFiStatusTimer);
    HAPLogInfo(&logObject, "WiFi reconfiguration - check WiFi status timer expired.");
    server->ip.checkWiFiStatusTimer = 0;

    if (!isWiFiStatusProcessed) {
        server->ip.numCheckWiFiStatusRetries++;
        bool isWiFiLinkEstablished = false;
        bool isWiFiNetworkConfigured = false;

        uint32_t updateStatus = 0;
        HAPError err =
                HAPPlatformWiFiManagerGetUpdateStatus(HAPNonnull(server->platform.ip.wiFi.wiFiManager), &updateStatus);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
            HAPFatalError();
        }
        bool restartRequired = IsRestartRequired(updateStatus);

        GetWiFiStatusForFailSafeUpdate(context, &isWiFiLinkEstablished, &isWiFiNetworkConfigured);

        if (isWiFiLinkEstablished && isWiFiNetworkConfigured && !restartRequired) {
            HandleFailSafeUpdateSuccess(server);
        } else {
            if (server->ip.numCheckWiFiStatusRetries >= CHECK_WIFI_STATUS_MAX_RETRIES) {
                server->ip.numCheckWiFiStatusRetries = 0;
                HAPLogDebug(
                        &logObject, "All check WiFi status retries expired. Wait for fail safe update timeout timer");
            } else {
                // Retry polling WiFi status until maximum number of retries is reached
                HAPError err = HAPPlatformTimerRegister(
                        &server->ip.checkWiFiStatusTimer,
                        HAPPlatformClockGetCurrent() + CHECK_WIFI_STATUS_RETRY_TIMEOUT,
                        OnCheckWiFiStatusTimerExpired,
                        context);
                if (err) {
                    server->ip.checkWiFiStatusTimer = 0;
                    HAPLogError(&logObject, "Register time for checking WiFi status failed");
                }
            }
        }
    }
}

/**
 * Callback that is called after fail safe timeout expires. This is the maximum time allowed after the fail safe
 * update is received to check for WiFi status and set the appropriate flags and terminate the WiFi
 * reconfiguration process
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void HandleFailSafeUpdateTimeout(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;

    HAPPrecondition(timer == server->ip.failSafeUpdateTimeoutTimer);
    server->ip.failSafeUpdateTimeoutTimer = 0;
    HAPLogInfo(&logObject, "WiFi reconfiguration - fail safe timer expired.");
    uint32_t updateStatus = 0;
    HAPError err =
            HAPPlatformWiFiManagerGetUpdateStatus(HAPNonnull(server->platform.ip.wiFi.wiFiManager), &updateStatus);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
        HAPFatalError();
    }
    bool restartRequired = IsRestartRequired(updateStatus);
    if (isWiFiStatusProcessed && !restartRequired) {
        return;
    }

    // Handle success/failure for Wifi only use case
    if (restartRequired) {
        if (commitMessageReceived) {
            HandleFailSafeUpdateSuccess(server);
        } else {
            // Stop _hap service discovery.
            if (server->ip.isServiceDiscoverable) {
                HAPIPServiceDiscoveryStop(server);
                server->ip.isServiceDiscoverable = false;
            }

            HAPAccessoryServerCloseTCPSessionsAndListener(server);
            HandleFailSafeUpdateFailed(server);
        }
        return;
    }

    // Fall through for other cases to detect if WiFi is connected
    bool isWiFiLinkEstablished = false;
    bool isWiFiNetworkConfigured = false;

    GetWiFiStatusForFailSafeUpdate(context, &isWiFiLinkEstablished, &isWiFiNetworkConfigured);
    if (isWiFiLinkEstablished && isWiFiNetworkConfigured) {
        HandleFailSafeUpdateSuccess(server);
    } else {
        // Stop _hap service discovery.
        if (server->ip.isServiceDiscoverable) {
            HAPIPServiceDiscoveryStop(server);
            server->ip.isServiceDiscoverable = false;
        }

        HAPAccessoryServerCloseTCPSessionsAndListener(server);
        HandleFailSafeUpdateFailed(server);
    }
}

/**
 * Callback that is called after simple update request is received to continue the processing of the WiFi
 * reconfiguration after the write response is sent back to the controller
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void HandleSimpleUpdateCallback(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;

    HAPPrecondition(timer == server->ip.updateWiFiConfigurationTimer);

    server->ip.updateWiFiConfigurationTimer = 0;

    HAPAccessoryServerCloseTCPSessionsAndListener(server);
    if (server->ip.wiFiReconfiguration.stationConfig.ssidIsSet) {
        HAPError err = ReconfigureWiFi(server);
        if (err) {
            HAPLogError(&logObject, "Reconfiguring WiFi (simple update) failed.");
        }
    }
    HAPAccessoryServerHandleWiFiReconfigurationDone(server);
    server->ip.wiFiReconfiguration.operationType =
            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid;
}

/**
 * Callback that is called after fail safe update request is received to continue the processing of the WiFi
 * reconfiguration after the write response is sent back to the controller
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void HandleFailSafeUpdateCallback(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);

    HAPAccessoryServer* const server = context;

    HAPPrecondition(timer == server->ip.updateWiFiConfigurationTimer);
    server->ip.updateWiFiConfigurationTimer = 0;
    HAPAccessoryServerCloseTCPSessionsAndListener(server);
    HAPError err = ReconfigureWiFi(server);
    if (err) {
        HandleFailSafeUpdateFailed(server);
    } else {
        err = HAPPlatformTimerRegister(
                &server->ip.checkWiFiStatusTimer,
                HAPPlatformClockGetCurrent() + CHECK_WIFI_STATUS_RETRY_TIMEOUT,
                OnCheckWiFiStatusTimerExpired,
                server);
        if (err) {
            server->ip.checkWiFiStatusTimer = 0;
            HAPLogError(&logObject, "Register timer for checking WiFi status failed");
        }
    }
    HAPAccessoryServerHandleWiFiReconfigurationDone(server);
}

/**
 * Validates the WiFi credentials that are sent for WiFi reconfiguration. Checks if ssid, psk (if security mode
 * is WPA-PSK) and country code have non-zero valid lengths as defined by the spec. Also checks for invalid
 * characters/format in all of the strings.
 *
 * @param ssid                 SSID to configure on the accessory
 * @param countryCode          Country code for the configuration
 * @param securityMode         Security mode for the configuration (WPA-PSK, etc.)
 * @param psk                  WPA passphrase
 *
 * @return kHAPError_None        if the credentials are valid
 * @return kHAPError_InvalidData if the credentials are not valid
 */
HAPError ValidateWiFiCredentials(const char* ssid, const char* psk, uint8_t securityMode, const char* countryCode) {
    // Validate Wi-Fi PSK.
    if (securityMode == kHAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode_WPA2_PSK) {
        if (psk && HAPStringGetNumBytes(psk)) {
            if (!HAPWACEngineIsValidWPAPassphrase(HAPNonnullVoid(psk))) {
                HAPLog(&logObject, "Wi-Fi PSK has invalid format.");
                return kHAPError_InvalidData;
            }
        } else {
            HAPLog(&logObject, "Wi-Fi PSK not provided for security mode WPA2.");
            return kHAPError_InvalidData;
        }
    } else {
        if (psk && HAPStringGetNumBytes(psk)) {
            HAPLog(&logObject, "Wi-Fi PSK provided for security mode None");
            return kHAPError_InvalidData;
        }
    }

    // Validate Wi-Fi SSID.
    size_t ssidNumBytes = HAPStringGetNumBytes(ssid);
    if (ssidNumBytes) {
        if (!HAPUTF8IsValidData(HAPNonnullVoid(ssid), ssidNumBytes)) {
            HAPLog(&logObject, "Wi-Fi SSID is not a valid UTF-8 string.");
            return kHAPError_InvalidData;
        }
    }

    // Validate country code.
    if (countryCode && HAPStringGetNumBytes(countryCode)) {
        if (!HAPWACEngineIsValidCountryCode(HAPNonnullVoid(countryCode))) {
            HAPLog(&logObject, "Country code has invalid format.");
            return kHAPError_InvalidData;
        }
    }
    return kHAPError_None;
}

/**
 * Callback that is called when we receive a simple update to clear the wifi configuration. We need to defer the
 * actual clearing of the wifi and entering WAC mode subsequently after sending the write response.
 *
 * @param timer          Timer that was registered and expired
 * @param context        Data passed into the callback
 *
 */
void HandleSimpleUpdateClearConfiguration(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessoryServer* const server = context;

    HAPPrecondition(timer == handleSimpleUpdateClearConfigurationTimer);
    handleSimpleUpdateClearConfigurationTimer = 0;

    if (server->ip.checkWiFiStatusTimer) {
        HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
        server->ip.checkWiFiStatusTimer = 0;
    }

    HAPAccessoryServerCloseTCPSessionsAndListener(server);

    if (HAPPlatformWiFiManagerIsConfigured(HAPNonnull(server->platform.ip.wiFi.wiFiManager))) {
        HAPPlatformWiFiManagerClearConfiguration(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
    }

    HAPAccessoryServerEnterWACMode(server);
}

/**
 * Handles write operation on WiFi configuration control characteristics of the accessory
 * Performs simple update, fail safe update for WiFi reconfiguration
 * Also send confirmation of the successful configuration of WiFi via the commit configuration message
 *
 * @param      server          Accessory server
 * @param      request         Write request to the WiFi configuration control characteristics
 * @param[out] requestReader   Write request reader that decodes the TLV
 * @param      context
 *
 * @return kHAPError_None if the read operation succeeds
 * @return kHAPError_Unknown for any failures
 * @return kHAPError_InvalidState if commit configuration message doesn't match the params of the fail safe update
 * @return kHAPError_InvalidData for incorrect data in requests
 * @return kHAPError_Busy if we there is a fail safe update pending
 *
 */
HAP_RESULT_USE_CHECK
HAPError HAPHandleWiFiTransportWiFiConfigurationControlWrite(
        HAPAccessoryServer* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReader* requestReader,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(HAPUUIDAreEqual(
            request->characteristic->characteristicType, &kHAPCharacteristicType_WiFiConfigurationControl));
    HAPPrecondition(HAPUUIDAreEqual(request->service->serviceType, &kHAPServiceType_WiFiTransport));
    HAPPrecondition(requestReader);

    HAPCharacteristicValue_WiFiConfigurationControl wiFiConfigurationControl;
    HAPRawBufferZero(&wiFiConfigurationControl, sizeof wiFiConfigurationControl);

    HAPError err = HAPTLVReaderDecode(
            requestReader, &kHAPCharacteristicTLVFormat_WiFiConfigurationControl, &wiFiConfigurationControl);
    if (err) {
        HAPLogError(&logObject, "TLV Reader failed to decode WiFi Configuration.");
        return kHAPError_InvalidData;
    }

    if (!(wiFiConfigurationControl.operationTypeIsSet)) {
        HAPLogError(&logObject, "Operation Type must be set for write request");
        return kHAPError_InvalidData;
    }

    server->ip.wiFiReconfiguration.operationType = wiFiConfigurationControl.operationType;

    if (wiFiConfigurationControl.cookieIsSet) {
        if (wiFiConfigurationControl.cookie == 0) {
            HAPLogError(&logObject, "Cookie value must be set to non-zero value for Update Configuration operations");
            return kHAPError_InvalidData;
        }
        server->ip.wiFiReconfiguration.cookie = wiFiConfigurationControl.cookie;
    } else {
        if (IsUpdateConfigurationOperationType(wiFiConfigurationControl.operationType)) {
            HAPLogError(&logObject, "Cookie value must be set for Update Configuration operations");
            return kHAPError_InvalidData;
        }
    }

    if (wiFiConfigurationControl.stationConfigIsSet && wiFiConfigurationControl.stationConfig.ssidIsSet &&
        HAPStringAreEqual(wiFiConfigurationControl.stationConfig.ssid, "")) {
        HAPLogError(&logObject, "WiFi ssid should not be empty");
        return kHAPError_InvalidData;
    }

    if (wiFiConfigurationControl.stationConfigIsSet && wiFiConfigurationControl.stationConfig.ssidIsSet) {
        HAPError err = ValidateWiFiCredentials(
                wiFiConfigurationControl.stationConfig.ssid,
                wiFiConfigurationControl.stationConfig.psk,
                wiFiConfigurationControl.stationConfig.securityMode,
                wiFiConfigurationControl.countryCodeIsSet ? wiFiConfigurationControl.countryCode : NULL);
        if (err == kHAPError_InvalidData) {
            return err;
        }
    }

    switch (server->ip.wiFiReconfiguration.operationType) {
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_Simple: {
            uint32_t updateStatus = 0;
            err = HAPPlatformWiFiManagerGetUpdateStatus(
                    HAPNonnull(server->platform.ip.wiFi.wiFiManager), &updateStatus);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
                return kHAPError_Unknown;
            }
            // If a current fail-safe update is pending, return Resource busy error
            if (IsUpdateStatusBitSet(
                        updateStatus, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Pending)) {
                return kHAPError_Busy;
            }
            server->ip.wiFiReconfiguration.updateStatus = 0;

            // Clear configuration if station config or ssid is not set
            if (!wiFiConfigurationControl.stationConfigIsSet || !wiFiConfigurationControl.stationConfig.ssidIsSet) {
                // Set the cookie value so the response can get the correct value
                err = HAPPlatformWiFiManagerSetCookie(
                        HAPNonnull(server->platform.ip.wiFi.wiFiManager), wiFiConfigurationControl.cookie);
                if (err) {
                    HAPLogDebug(&logObject, "Failed to set cookie");
                }
                // Stop _hap service discovery.
                if (server->ip.isServiceDiscoverable) {
                    HAPIPServiceDiscoveryStop(server);
                    server->ip.isServiceDiscoverable = false;
                }
                err = HAPPlatformTimerRegister(
                        &handleSimpleUpdateClearConfigurationTimer,
                        HAPPlatformClockGetCurrent() + SIMPLE_UPDATE_CLEAR_CONFIGURATION_TIMEOUT,
                        HandleSimpleUpdateClearConfiguration,
                        server);
                if (err) {
                    HAPLogError(&logObject, "Failed to register simple update clear configuration callback timer");
                    HAPFatalError();
                }
                return kHAPError_None;
            }

            err = PersistWiFiCredentials(server, wiFiConfigurationControl);
            if (err) {
                HAPLogError(&logObject, "Persisting WiFi credentials failed");
                return err;
            }

            // Stop _hap service discovery.
            if (server->ip.isServiceDiscoverable) {
                HAPIPServiceDiscoveryStop(server);
                server->ip.isServiceDiscoverable = false;
            }

            err = HAPPlatformTimerRegister(
                    &server->ip.updateWiFiConfigurationTimer,
                    HAPPlatformClockGetCurrent() + HAPSecond,
                    HandleSimpleUpdateCallback,
                    server);
            if (err) {
                HAPLogError(&logObject, "Failed to register simple update callback timer");
            }
            return err;
        }
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe: {
            if (!wiFiConfigurationControl.stationConfigIsSet || !wiFiConfigurationControl.stationConfig.ssidIsSet) {
                HAPLogError(&logObject, "WiFi station config or ssid should not be empty for fail safe update");
                return kHAPError_InvalidData;
            }
            isWiFiStatusProcessed = false;
            commitMessageReceived = false;
            uint32_t updateStatus = 0;
            err = HAPPlatformWiFiManagerGetUpdateStatus(
                    HAPNonnull(server->platform.ip.wiFi.wiFiManager), &updateStatus);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
                return kHAPError_Unknown;
            }

            // If a current update is pending, return Resource busy error
            if (IsUpdateStatusBitSet(
                        updateStatus, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Pending)) {
                return kHAPError_Busy;
            }
            server->ip.numCheckWiFiStatusRetries = 0;

            uint8_t operationTimeout = (wiFiConfigurationControl.operationTimeoutIsSet) ?
                                               wiFiConfigurationControl.operationTimeout :
                                               FAILSAFE_UPDATE_TIMEOUT;

            HAPError err = HAPPlatformTimerRegister(
                    &server->ip.failSafeUpdateTimeoutTimer,
                    HAPPlatformClockGetCurrent() + (operationTimeout * HAPSecond),
                    HandleFailSafeUpdateTimeout,
                    server);
            if (err) {
                HAPLogError(&logObject, "Failed to register fail safe timeout timer");
                HAPFatalError();
            }

            /**
             * Backs up the existing WiFi credentials in the accessory for fail safe WiFi Reconfiguration so that if the
             * configuration fails we can rollback to the previous credentials
             */
            err = HAPPlatformWiFiManagerBackUpConfiguration(HAPNonnull(server->platform.ip.wiFi.wiFiManager));
            if (err) {
                HAPLogError(&logObject, "Backing up existing WiFi credentials failed");
                return err;
            }

            // Set update pending status flag and cookie value in updateStatus
            server->ip.wiFiReconfiguration.updateStatus = wiFiConfigurationControl.updateStatus;
            server->ip.wiFiReconfiguration.updateStatus |=
                    kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Pending |
                    wiFiConfigurationControl.cookie;

            if (HAPPlatformTCPStreamManagerIsWiFiCurrentTransport(server->platform.ip.tcpStreamManager)) {
                SetUpdateStatusBit(
                        server, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Restart_Required);
            }

            err = PersistWiFiCredentials(server, wiFiConfigurationControl);
            if (err) {
                HAPLogError(&logObject, "Persisting WiFi credentials failed");
                return err;
            }
            // Stop _hap service discovery.
            if (server->ip.isServiceDiscoverable) {
                HAPIPServiceDiscoveryStop(server);
                server->ip.isServiceDiscoverable = false;
            }
            err = HAPPlatformTimerRegister(
                    &server->ip.updateWiFiConfigurationTimer,
                    HAPPlatformClockGetCurrent() + UPDATE_WIFI_CONFIGURATION_TIMEOUT,
                    HandleFailSafeUpdateCallback,
                    server);
            if (err) {
                HAPLogError(&logObject, "Failed to register fail safe update callback timer");
                HAPFatalError();
            }
            return err;
        }
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Update_FailSafe_Commit: {
            uint32_t updateStatus = 0;
            err = HAPPlatformWiFiManagerGetUpdateStatus(
                    HAPNonnull(server->platform.ip.wiFi.wiFiManager), &updateStatus);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "%s: Update status not read successfully from WiFi configuration.", __func__);
                return kHAPError_Unknown;
            }

            HAPPlatformWiFiManagerCookie cookie = kHAPPlatformWiFiManagerCookie_Unmanaged;
            err = HAPPlatformWiFiManagerGetCookie(HAPNonnull(server->platform.ip.wiFi.wiFiManager), &cookie);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPLogError(&logObject, "%s: cookie not read successfully from WiFi configuration.", __func__);
                return kHAPError_Unknown;
            }

            bool isRestartRequired =
                    HAPPlatformTCPStreamManagerIsWiFiCurrentTransport(server->platform.ip.tcpStreamManager);

            if (isRestartRequired && (cookie == wiFiConfigurationControl.cookie)) {
                commitMessageReceived = true;
                if (server->ip.failSafeUpdateTimeoutTimer != 0) {
                    HAPLogDebug(&logObject, "Reconfiguring WiFi (fail safe update) was successful.");
                    ResetUpdateStatusOnSuccess(
                            server, kHAPCharacteristicValue_WiFiConfigurationControl_UpdateStatus_Connection_Verified);
                    server->ip.wiFiReconfiguration.operationType =
                            kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Invalid;
                    if (server->ip.checkWiFiStatusTimer) {
                        HAPPlatformTimerDeregister(server->ip.checkWiFiStatusTimer);
                        server->ip.checkWiFiStatusTimer = 0;
                    }
                    if (server->ip.failSafeUpdateTimeoutTimer) {
                        HAPPlatformTimerDeregister(server->ip.failSafeUpdateTimeoutTimer);
                        server->ip.failSafeUpdateTimeoutTimer = 0;
                    }
                } else {
                    HAPLogDebug(&logObject, "Reconfiguring WiFi (fail safe update) failed.");
                }
                return kHAPError_None;
            } else {
                return kHAPError_InvalidState;
            }
        }
        case kHAPCharacteristicValue_WiFiConfigurationControl_OperationType_Read:
        default:
            return kHAPError_None;
    }
}
#endif
