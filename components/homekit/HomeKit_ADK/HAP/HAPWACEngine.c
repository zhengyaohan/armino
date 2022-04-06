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

#include "HAPPlatformFeatures.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

#include "HAPAccessoryServer+Internal.h"
#include "HAPCrypto.h"
#include "HAPIPAccessoryServer.h"
#include "HAPLogSubsystem.h"
#include "HAPWACEngine+Types.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "WACEngine" };

/**
 * Returns whether a given passphrase is valid for a WPA/WPA2 personal network.
 *
 * @param      value                Value.
 *
 * @return true                     If the value is a valid passphrase for a WPA/WPA2 personal network.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWACEngineIsValidWPAPassphrase(const char* value) {
    HAPPrecondition(value);

    // Validate characters.
    bool isValidHexKey = true;
    const char* c;
    for (c = value; *c; c++) {
        if (*c < 32 || *c > 126) {
            HAPLog(&logObject, "Wi-Fi passphrase contains invalid character: 0x%02X.", *c);
            return false;
        }
        if (!HAPASCIICharacterIsHexDigit(*c)) {
            isValidHexKey = false;
        }
    }

    // Validate length.
    // 8-63 printable ASCII characters or 64 hexadecimal digits.
    size_t numBytes = (size_t)(c - value);
    if (numBytes == 64 && isValidHexKey) {
        return true;
    }
    if (numBytes < 8 || numBytes > 63) {
        HAPLog(&logObject, "Wi-Fi passphrase has invalid length: %lu.", (unsigned long) numBytes);
        return false;
    }
    return true;
}

/**
 * Returns whether a given country code is a valid ISO 3166-1 alpha-2 code.
 *
 * @param      value                Value.
 *
 * @return true                     If the value is a valid ISO 3166-1 alpha-2 country code.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWACEngineIsValidCountryCode(const char* value) {
    HAPPrecondition(value);

    return (HAPASCIICharacterIsUppercaseLetter(value[0])) && (HAPASCIICharacterIsUppercaseLetter(value[1])) &&
           (value[2] == '\0');
}

HAP_RESULT_USE_CHECK
HAPError HAPWACEngineHandleConfig(
        HAPAccessoryServer* server,
        HAPIPSecuritySession* session,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes HAP_UNUSED,
        size_t* numResponseBytes) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(session->type == kHAPIPSecuritySessionType_HAP);
    HAPPrecondition(session->isOpen);
    HAPPrecondition(session->isSecured);
    HAPPrecondition(requestBytes);
    HAPPrecondition(responseBytes);
    HAPPrecondition(numResponseBytes);

    HAPError err;

    HAPLogDebug(&logObject, "/config.");

    if (!server->ip.wac.softwareAccessPointIsActive) {
        HAPLog(&logObject, "Rejecting /config message because it is only supported on the Software Access Point.");
        return kHAPError_InvalidState;
    }

    // Admin access only.
    if (session->type == kHAPIPSecuritySessionType_HAP) {
        HAPSession* hapSession = &session->_.hap;
        if (!hapSession->hap.active) {
            HAPLog(&logObject, "Rejecting /config message because HAP session is not secured.");
            return kHAPError_NotAuthorized;
        }
        HAPAssert(hapSession->hap.pairingID >= 0);

        bool exists;
        HAPPairing pairing;
        err = HAPPairingGet(server, (HAPPairingIndex) hapSession->hap.pairingID, &pairing, &exists);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return kHAPError_NotAuthorized;
        }
        if (!exists) {
            HAPLog(&logObject, "Rejecting /config message from unpaired controller.");
            return kHAPError_NotAuthorized;
        }
        if (!(pairing.permissions & 0x01U)) {
            HAPLog(&logObject, "Rejecting /config message from non-admin controller.");
            return kHAPError_NotAuthorized;
        }
    }

    if (server->ip.wac.wiFiConfiguration.isSet && !server->ip.wac.wiFiConfiguration.isApplied) {
        HAPLog(&logObject, "Rejecting /config message because a Wi-Fi configuration is pending application.");
        return kHAPError_InvalidState;
    }
    if (server->ip.wac.wiFiConfiguration.isSet) {
        HAPLog(&logObject, "Rejecting /config message because a Wi-Fi configuration has already been received.");
        return kHAPError_InvalidState;
    }

    // Decrypt message.
    if (session->type == kHAPIPSecuritySessionType_MFiSAP) {
        HAP_aes_ctr_decrypt(&session->_.mfiSAP.aesMasterContext, requestBytes, requestBytes, numRequestBytes);
    }
    HAPLogSensitiveBufferDebug(&logObject, requestBytes, numRequestBytes, "/config.");

    // Parse request.
    // See Accessory Interface Specification - Wi-Fi Accessory Configuration Addendum R1
    // Section 2.3 Wi-Fi Accessory Configuration Setup Experience
    HAPTLV wiFiPSKTLV, wiFiSSIDTLV;
    wiFiPSKTLV.type = kHAPWACTLVType_WiFiPSK;
    wiFiSSIDTLV.type = kHAPWACTLVType_WiFiSSID;
    HAPTLV countryCodeTLV;
    countryCodeTLV.type = kHAPWACTLVType_CountryCode;
    {
        HAPTLVReader tlvReader;
        HAPTLVReaderCreate(&tlvReader, requestBytes, numRequestBytes);

        err = HAPTLVReaderGetAll(&tlvReader, (HAPTLV* const[]) { &wiFiPSKTLV, &wiFiSSIDTLV, &countryCodeTLV, NULL });
        if (err) {
            HAPAssert(err == kHAPError_InvalidData);
            return err;
        }

        // Validate Wi-Fi PSK.
        if (wiFiPSKTLV.value.bytes) {
            if (wiFiPSKTLV.value.numBytes > sizeof server->ip.wac.wiFiConfiguration.passphrase - 1) {
                HAPLog(&logObject,
                       "/config: Wi-Fi PSK has invalid length (%lu).",
                       (unsigned long) wiFiPSKTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            if (HAPStringGetNumBytes(HAPNonnullVoid(wiFiPSKTLV.value.bytes)) != wiFiPSKTLV.value.numBytes) {
                HAPLog(&logObject, "/config: Wi-Fi PSK contains NULL characters.");
                return kHAPError_InvalidData;
            }
            if (!HAPWACEngineIsValidWPAPassphrase(HAPNonnullVoid(wiFiPSKTLV.value.bytes))) {
                HAPLog(&logObject, "/config: Wi-Fi PSK has invalid format.");
                return kHAPError_InvalidData;
            }
        }

        // Validate Wi-Fi SSID.
        if (!wiFiSSIDTLV.value.bytes) {
            HAPLog(&logObject, "/config: Wi-Fi SSID missing.");
            return kHAPError_InvalidData;
        }
        if (wiFiSSIDTLV.value.numBytes > sizeof server->ip.wac.wiFiConfiguration.ssid - 1) {
            HAPLog(&logObject,
                   "/config: Wi-Fi SSID has invalid length (%lu).",
                   (unsigned long) wiFiSSIDTLV.value.numBytes);
            return kHAPError_InvalidData;
        }
        if (HAPStringGetNumBytes(HAPNonnullVoid(wiFiSSIDTLV.value.bytes)) != wiFiSSIDTLV.value.numBytes) {
            HAPLog(&logObject, "/config: Wi-Fi SSID contains NULL characters.");
            return kHAPError_InvalidData;
        }
        if (!HAPUTF8IsValidData(HAPNonnullVoid(wiFiSSIDTLV.value.bytes), wiFiSSIDTLV.value.numBytes)) {
            HAPLog(&logObject, "/config: Wi-Fi SSID is not a valid UTF-8 string.");
            return kHAPError_InvalidData;
        }

        // Validate country code.
        if (countryCodeTLV.value.bytes) {
            if (countryCodeTLV.value.numBytes > sizeof server->ip.wac.wiFiConfiguration.regulatoryDomain - 1) {
                HAPLog(&logObject,
                       "/config: Country code has invalid length (%lu).",
                       (unsigned long) countryCodeTLV.value.numBytes);
                return kHAPError_InvalidData;
            }
            if (HAPStringGetNumBytes(HAPNonnullVoid(countryCodeTLV.value.bytes)) != countryCodeTLV.value.numBytes) {
                HAPLog(&logObject, "/config: Country code contains NULL characters.");
                return kHAPError_InvalidData;
            }
            if (!HAPWACEngineIsValidCountryCode(HAPNonnullVoid(countryCodeTLV.value.bytes))) {
                HAPLog(&logObject, "/config: Country code has invalid format.");
                return kHAPError_InvalidData;
            }
        }
    }

    HAPAssert(!server->ip.wac.wiFiConfiguration.isSet);

    // Store Wi-Fi SSID.
    HAPLogSensitiveBufferInfo(&logObject, wiFiSSIDTLV.value.bytes, wiFiSSIDTLV.value.numBytes, "/config: SSID.");
    HAPRawBufferCopyBytes(
            server->ip.wac.wiFiConfiguration.ssid, HAPNonnullVoid(wiFiSSIDTLV.value.bytes), wiFiSSIDTLV.value.numBytes);
    server->ip.wac.wiFiConfiguration.ssid[wiFiSSIDTLV.value.numBytes] = '\0';

    // Store Wi-Fi passphrase.
    if (wiFiPSKTLV.value.bytes) {
        HAPLogSensitiveBufferInfo(
                &logObject, wiFiPSKTLV.value.bytes, wiFiPSKTLV.value.numBytes, "/config: Passphrase.");
        HAPRawBufferCopyBytes(
                server->ip.wac.wiFiConfiguration.passphrase,
                HAPNonnullVoid(wiFiPSKTLV.value.bytes),
                wiFiPSKTLV.value.numBytes);
        server->ip.wac.wiFiConfiguration.passphrase[wiFiPSKTLV.value.numBytes] = '\0';
    } else {
        server->ip.wac.wiFiConfiguration.passphrase[0] = '\0';
    }

    // Store regulatory domain.
    if (countryCodeTLV.value.bytes) {
        HAPLogBufferInfo(
                &logObject, countryCodeTLV.value.bytes, countryCodeTLV.value.numBytes, "/config: Country code.");
        HAPRawBufferCopyBytes(
                server->ip.wac.wiFiConfiguration.regulatoryDomain,
                HAPNonnullVoid(countryCodeTLV.value.bytes),
                countryCodeTLV.value.numBytes);
        server->ip.wac.wiFiConfiguration.regulatoryDomain[countryCodeTLV.value.numBytes] = '\0';
    } else {
        server->ip.wac.wiFiConfiguration.regulatoryDomain[0] = '\0';
    }

    server->ip.wac.wiFiConfiguration.isSet = true;

    // Apply Wi-Fi configuration on disconnect of this session.
    session->receivedConfig = true;
    *numResponseBytes = 0;
    return kHAPError_None;
}

#endif
