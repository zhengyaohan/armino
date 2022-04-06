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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h> // Required on Raspi.
#include <unistd.h>

#include "HAPPlatformFileManager.h"
#include "HAPPlatformSystemCommand.h"
#include "HAPPlatformWiFiManager+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "WiFiManager" };

#define kHAPPlatformWiFiManager_WPASupplicantConfPath     "/etc/wpa_supplicant/wpa_supplicant.conf"
#define kHAPPlatformWiFiManager_WPASupplicantConfOrigPath "/etc/wpa_supplicant/wpa_supplicant.conf.orig"

#define kHAPPlatformWiFiManager_ControlInterfaceDirectory "/var/run/wpa_supplicant"

#define kHAPPlatformWiFiManager_DefaultInterfaceName "wlan0"

#define UINT16_MAX_LEN 6

#define UINT32_MAX_LEN 11

#define WIFI_CONFIGURATION_STRING_MAX_LEN 512

/**
 * Configuration reader state.
 */
HAP_ENUM_BEGIN(uint8_t, ConfigurationReaderState) {
    kConfigurationReaderState_ExpectingName,
    kConfigurationReaderState_ReadingName,
    kConfigurationReaderState_ExpectingNameValueSeparator,
    kConfigurationReaderState_ExpectingValue,
    kConfigurationReaderState_ReadingValue,
    kConfigurationReaderState_ExpectingNewLine,
    kConfigurationReaderState_Done
} HAP_ENUM_END(uint8_t, ConfigurationReaderState);

/**
 * Configuration block type.
 */
HAP_ENUM_BEGIN(uint8_t, ConfigurationBlockType) {
    kConfigurationBlockType_None,
    kConfigurationBlockType_Unknown,
    kConfigurationBlockType_Network
} HAP_ENUM_END(uint8_t, ConfigurationBlockType);

/**
 * Configuration entry type.
 */
HAP_ENUM_BEGIN(uint8_t, ConfigurationEntryType) {
    kConfigurationEntryType_None,
    kConfigurationEntryType_Unknown,
    kConfigurationEntryType_Network,
    kConfigurationEntryType_SSID
} HAP_ENUM_END(uint8_t, ConfigurationEntryType);

void HAPPlatformWiFiManagerCreate(HAPPlatformWiFiManagerRef wiFiManager, const HAPPlatformWiFiManagerOptions* options) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(options);

    HAPLogDebug(&logObject, "Storage configuration: wiFiManager = %lu", (unsigned long) sizeof *wiFiManager);

    HAPRawBufferZero(wiFiManager, sizeof *wiFiManager);

    const char* interfaceName =
            options->interfaceName ? options->interfaceName : kHAPPlatformWiFiManager_DefaultInterfaceName;
    size_t numInterfaceNameBytes = HAPStringGetNumBytes(interfaceName);
    if ((numInterfaceNameBytes == 0) || (numInterfaceNameBytes >= sizeof wiFiManager->interfaceName)) {
        HAPLogError(&logObject, "Invalid local network interface name.");
        HAPFatalError();
    }
    HAPRawBufferCopyBytes(wiFiManager->interfaceName, interfaceName, numInterfaceNameBytes);
}

static ConfigurationEntryType GetEntryType(char nameBuffer[], size_t numNameBufferChars) {
    HAPPrecondition(nameBuffer);
    if (numNameBufferChars == 4 && nameBuffer[0] == 's' && nameBuffer[1] == 's' && nameBuffer[2] == 'i' &&
        nameBuffer[3] == 'd') {
        return kConfigurationEntryType_SSID;
    } else {
        return kConfigurationEntryType_Unknown;
    }
}

static bool IsControlChar(char c) {
    return c < 0x20 || c == 0x1a || c == 0x1b || c == 0x1c || c == 0x1d || c == 0x1e || c == 0x1f || c == 0x7f;
}

static bool IsWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
static bool IsSpaceOrTab(char c) {
    return c == ' ' || c == '\t';
}
#endif

static void ReadSSID(int fd, bool* isConfigured, HAPPlatformSSID* ssid) {
    HAPPrecondition(fd >= 0);
    HAPPrecondition(isConfigured);
    HAPPrecondition(ssid);

#define LOG_UNEXPECTED_FORMAT() \
    do { \
        HAPLogError(&logObject, "Unexpected format of file %s.", kHAPPlatformWiFiManager_WPASupplicantConfPath); \
    } while (0)

    HAPError err;

    bool ignoreLine = false;
    ConfigurationBlockType blockType = kConfigurationBlockType_None;
    ConfigurationEntryType entryType = kConfigurationEntryType_Network;
    ConfigurationReaderState state = kConfigurationReaderState_ExpectingValue;

    char nameBuffer[64];
    size_t numNameBufferChars = 0;
    char valueBuffer[1] = { '\0' };
    size_t numValueBufferChars = 0;

    bool inQuotes = false;
    ssid->numBytes = 0;

    for (;;) {
        // Read next byte.
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            HAPFatalError();
        }
        if (n == 0) {
            LOG_UNEXPECTED_FORMAT();
            HAPFatalError();
        }
        HAPAssert(n == sizeof c);

        if (IsControlChar(c) && !IsWhitespace(c)) {
            LOG_UNEXPECTED_FORMAT();
            HAPFatalError();
        }
        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
        } else {
            switch (state) {
                case kConfigurationReaderState_ExpectingName: {
                    HAPAssert(entryType == kConfigurationEntryType_None);
                    if (IsWhitespace(c)) {
                        // skip whitespace
                    } else if (c == '#') {
                        ignoreLine = true;
                    } else if (c == '{') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (c == '}') {
                        switch (blockType) {
                            case kConfigurationBlockType_None: {
                                LOG_UNEXPECTED_FORMAT();
                                HAPFatalError();
                                break;
                            }
                            case kConfigurationBlockType_Unknown: {
                                blockType = kConfigurationBlockType_None;
                                state = kConfigurationReaderState_ExpectingNewLine;
                                break;
                            }
                            case kConfigurationBlockType_Network: {
                                blockType = kConfigurationBlockType_None;
                                state = kConfigurationReaderState_Done;
                                break;
                            }
                        }
                    } else if (c == '"') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else {
                        nameBuffer[0] = c;
                        numNameBufferChars = 1;
                        state = kConfigurationReaderState_ReadingName;
                    }
                    break;
                }
                case kConfigurationReaderState_ReadingName: {
                    HAPAssert(entryType == kConfigurationEntryType_None);
                    if (c == '\n') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (IsWhitespace(c)) {
                        state = kConfigurationReaderState_ExpectingNameValueSeparator;
                    } else if (c == '{') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (c == '}') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (c == '"') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (c == '=') {
                        entryType = GetEntryType(nameBuffer, numNameBufferChars);
                        state = kConfigurationReaderState_ExpectingValue;
                    } else {
                        if (numNameBufferChars >= sizeof nameBuffer) {
                            LOG_UNEXPECTED_FORMAT();
                            HAPFatalError();
                        }
                        nameBuffer[numNameBufferChars] = c;
                        numNameBufferChars++;
                    }
                    break;
                }
                case kConfigurationReaderState_ExpectingNameValueSeparator: {
                    HAPAssert(entryType == kConfigurationEntryType_None);
                    if (c == '\n') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (IsWhitespace(c)) {
                        // skip whitespace
                    } else if (c == '=') {
                        entryType = GetEntryType(nameBuffer, numNameBufferChars);
                        state = kConfigurationReaderState_ExpectingValue;
                    } else {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    }
                    break;
                }
                case kConfigurationReaderState_ExpectingValue: {
                    HAPAssert(entryType != kConfigurationEntryType_None);
                    if (c == '\n') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else if (IsWhitespace(c)) {
                        // skip whitespace
                    } else if (c == '{') {
                        if (blockType != kConfigurationBlockType_None) {
                            LOG_UNEXPECTED_FORMAT();
                            HAPFatalError();
                        }
                        switch (entryType) {
                            case kConfigurationEntryType_None: {
                                HAPFatalError();
                                break;
                            }
                            case kConfigurationEntryType_Unknown: {
                                blockType = kConfigurationBlockType_Unknown;
                                break;
                            }
                            case kConfigurationEntryType_Network: {
                                blockType = kConfigurationBlockType_Network;
                                break;
                            }
                            case kConfigurationEntryType_SSID: {
                                LOG_UNEXPECTED_FORMAT();
                                HAPFatalError();
                                break;
                            }
                        }
                        state = kConfigurationReaderState_ExpectingNewLine;
                    } else if (c == '}') {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    } else {
                        if (blockType == kConfigurationBlockType_Network && entryType == kConfigurationEntryType_SSID) {
                            if (ssid->numBytes) {
                                LOG_UNEXPECTED_FORMAT();
                                HAPFatalError();
                            }
                            if (c == '"') {
                                inQuotes = true;
                            } else {
                                if (!HAPASCIICharacterIsHexDigit(c)) {
                                    LOG_UNEXPECTED_FORMAT();
                                    HAPFatalError();
                                }
                                valueBuffer[0] = c;
                                numValueBufferChars = 1;
                            }
                        }
                        state = kConfigurationReaderState_ReadingValue;
                    }
                    break;
                }
                case kConfigurationReaderState_ReadingValue: {
                    HAPAssert(entryType != kConfigurationEntryType_None);
                    if (blockType == kConfigurationBlockType_Network && entryType == kConfigurationEntryType_SSID) {
                        if (inQuotes) {
                            if (c == '\n') {
                                LOG_UNEXPECTED_FORMAT();
                                HAPFatalError();
                            } else if (c == '"') {
                                inQuotes = false;
                                if (!ssid->numBytes) {
                                    LOG_UNEXPECTED_FORMAT();
                                    HAPFatalError();
                                }
                                state = kConfigurationReaderState_ExpectingNewLine;
                            } else {
                                if (ssid->numBytes >= sizeof ssid->bytes) {
                                    LOG_UNEXPECTED_FORMAT();
                                    HAPFatalError();
                                }
                                ssid->bytes[ssid->numBytes] = (uint8_t) c;
                                ssid->numBytes++;
                            }
                        } else {
                            if (IsWhitespace(c)) {
                                if (numValueBufferChars) {
                                    HAPAssert(numValueBufferChars == 1);
                                    LOG_UNEXPECTED_FORMAT();
                                    HAPFatalError();
                                }
                                HAPAssert(ssid->numBytes);
                                if (c == '\n') {
                                    entryType = kConfigurationEntryType_None;
                                    state = kConfigurationReaderState_ExpectingName;
                                } else {
                                    state = kConfigurationReaderState_ExpectingNewLine;
                                }
                            } else {
                                if (!HAPASCIICharacterIsHexDigit(c)) {
                                    LOG_UNEXPECTED_FORMAT();
                                    HAPFatalError();
                                }
                                if (numValueBufferChars) {
                                    HAPAssert(numValueBufferChars == 1);
                                    if (ssid->numBytes >= sizeof ssid->bytes) {
                                        LOG_UNEXPECTED_FORMAT();
                                        HAPFatalError();
                                    }
                                    // For converting hex to decimal
                                    uint8_t valueBufferOffset0 = 0;
                                    uint8_t cDecimal = 0;
                                    err = HAPUInt8FromHexDigit(c, &cDecimal);
                                    if (err != kHAPError_None) {
                                        HAPFatalError();
                                    }
                                    err = HAPUInt8FromHexDigit(valueBuffer[0], &valueBufferOffset0);
                                    if (err != kHAPError_None) {
                                        HAPFatalError();
                                    }
                                    ssid->bytes[ssid->numBytes] = ((uint8_t)(valueBufferOffset0 << 4)) | cDecimal;
                                    ssid->numBytes++;
                                    numValueBufferChars = 0;
                                } else {
                                    valueBuffer[numValueBufferChars] = c;
                                    numValueBufferChars++;
                                }
                            }
                        }
                    } else if (c == '\n') {
                        entryType = kConfigurationEntryType_None;
                        state = kConfigurationReaderState_ExpectingName;
                    }
                    break;
                }
                case kConfigurationReaderState_ExpectingNewLine: {
                    HAPAssert(entryType != kConfigurationEntryType_None);
                    if (c == '\n') {
                        entryType = kConfigurationEntryType_None;
                        state = kConfigurationReaderState_ExpectingName;
                    } else if (IsWhitespace(c)) {
                        // skip whitespace
                    } else {
                        LOG_UNEXPECTED_FORMAT();
                        HAPFatalError();
                    }
                    break;
                }
                case kConfigurationReaderState_Done: {
                    HAPAssert(blockType == kConfigurationBlockType_None);
                    HAPAssert(entryType == kConfigurationEntryType_None);
                    *isConfigured = ssid->numBytes;
                    return;
                }
            }
        }
    }

    *isConfigured = false;

#undef LOG_UNEXPECTED_FORMAT
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
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading SSID.
 * @return kHAPError_InvalidState   If SSID is not configured.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetSSID(
        HAPPlatformWiFiManagerRef wiFiManager,
        bool* isConfigured,
        HAPPlatformSSID* ssid) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isConfigured);
    HAPPrecondition(ssid);

    // Open wpa_supplicant configuration file.
    int fd;
    *isConfigured = false;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            return kHAPError_InvalidState;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        return kHAPError_Unknown;
    }

    // Look for network.
    size_t i = 0;
    bool ignoreLine = false;
    for (;;) {
        // Read next byte.
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
            break;
        }
        HAPAssert(n == sizeof c);

        // Skip lines until finding network configuration.
        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }

// Match network prefix.
#define prefixString "network="
        size_t numPrefixBytes = sizeof prefixString - 1;
        HAPAssert(i < numPrefixBytes);
        if (c != prefixString[i]) {
            ignoreLine = c != '\n';
            i = 0;
            continue;
        }

        // Character matched.
        i++;

        if (i == numPrefixBytes) {
            ReadSSID(fd, isConfigured, ssid);
            if (ssid->numBytes > 0) {
                HAPRawBufferZero(ssid->stringValue, sizeof(ssid->stringValue));
                HAPRawBufferCopyBytes(ssid->stringValue, ssid->bytes, ssid->numBytes);
            }
            (void) close(fd);
            return kHAPError_None;
        }
#undef prefixString
    }

    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);

    *isConfigured = false;
    (void) close(fd);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsConfigured(HAPPlatformWiFiManagerRef wiFiManager) {
    HAPPrecondition(wiFiManager);

    bool isConfigured;
    HAPPlatformSSID ssid;

    HAPError err = HAPPlatformWiFiManagerGetSSID(wiFiManager, &isConfigured, &ssid);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        return kHAPError_Unknown;
    }

    return isConfigured;
}

/**
 * Gets the regulatory domain in which the device is operating.
 *
 * - Regulatory domain information can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
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
        HAPPlatformRegulatoryDomain* regulatoryDomain) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(regulatoryDomain);

    HAPRawBufferZero(regulatoryDomain, sizeof *regulatoryDomain);

    // Open wpa_supplicant configuration file.
    int fd;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            return kHAPError_InvalidState;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        return kHAPError_Unknown;
    }

    // Look for country key.
    size_t i = 0;
    bool countryCodeFound = false;
    bool ignoreLine = false;
    for (;;) {
        // Read next byte.
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            break;
        }
        HAPAssert(n == sizeof c);

        // Skip lines until finding country code configuration.
        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }

// Match country code prefix.
#define prefixString "country="
        size_t numPrefixBytes = sizeof prefixString - 1;
        if (i < numPrefixBytes) {
            if (c != prefixString[i]) {
                ignoreLine = c != '\n';
                i = 0;
                continue;
            }

            // Character matched.
            i++;

            // Once full prefix is read, check that there are not multiple country codes.
            if (i == numPrefixBytes) {
                if (countryCodeFound) {
                    HAPLog(&logObject, "Multiple country codes in %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
                    (void) close(fd);
                    return kHAPError_InvalidState;
                }
                countryCodeFound = true;
            }

            continue;
        }
#undef prefixString

        // Match country code. It consists of two uppercase alphabet characters.
        size_t o = i - numPrefixBytes;
        if (o < sizeof regulatoryDomain->stringValue - 1) {
            if (!HAPASCIICharacterIsUppercaseLetter(c)) {
                HAPLog(&logObject, "Malformed country code in %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
                (void) close(fd);
                return kHAPError_InvalidState;
            }
            regulatoryDomain->stringValue[o] = c;
            i++;

            continue;
        }

        // Match line terminator after country code.
        HAPAssert(o == sizeof regulatoryDomain->stringValue - 1);
        if (c != '\n') {
            HAPLog(&logObject,
                   "Malformed country code in %s (too long).",
                   kHAPPlatformWiFiManager_WPASupplicantConfPath);
            (void) close(fd);
            return kHAPError_InvalidState;
        }
        i = 0;
    }
    if (!countryCodeFound) {
        HAPLog(&logObject, "Country code has not been set in %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        (void) close(fd);
        return kHAPError_InvalidState;
    }
    if (!regulatoryDomain->stringValue[0] || !regulatoryDomain->stringValue[1]) {
        HAPLog(&logObject, "Malformed country code in %s (too short).", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        (void) close(fd);
        return kHAPError_InvalidState;
    }
    HAPAssert(!regulatoryDomain->stringValue[2]);
    (void) close(fd);

    HAPLogInfo(&logObject, "Regulatory domain: %s.", regulatoryDomain->stringValue);
    return kHAPError_None;
}

/**
 * Restarts the WiFi interface after making changes to the wpa_supplicant
 *
 * @param      wiFiManager          WiFi manager
 *
 * @return kHAPError_None           If WiFi restarts successfully
 * @return kHAPError_Unknown        If WiFi fails to restart
 * @return kHAPError_OutOfResources If the buffer was not big enough to store the result
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerRestartWiFi(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);
    HAPError err;

    char* const cmd[] = { "/usr/bin/sudo", "/sbin/wpa_cli", "-i", wiFiManager->interfaceName, "reconfigure", NULL };

    char result[10] = { 0 };
    size_t written;
    err = HAPPlatformSystemCommandRun(cmd, result, sizeof(result), &written);

    if (err) {
        HAPLogBufferError(&logObject, result, written, "%s: Restarting WiFi failed with error %d", __func__, err);
        return err;
    }
    return kHAPError_None;
}

/**
 * Updates the wpa_supplicant configuration.
 *
 * @param      wiFiManager          WiFi manager.
 * @param      filePath             File Path of the configuration file.
 * @param      wiFiConfiguration    Configuration to write. NULL-terminated.
 *
 * @return kHAPError_None           If the file was updated successfully.
 * @return kHAPError_Unknown        If updating the supplicant fails.
 */
HAP_RESULT_USE_CHECK
static HAPError
        UpdateWiFiConfiguration(HAPPlatformWiFiManagerRef wiFiManager, char* filePath, const char* wiFiConfiguration) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(wiFiManager->interfaceName);
    HAPPrecondition(wiFiConfiguration);

    HAPError err;

    // Write configuration.
    HAPLogSensitiveInfo(&logObject, "Writing %s.\n%s", filePath, wiFiConfiguration);
    err = HAPPlatformFileManagerWriteFile(filePath, wiFiConfiguration, HAPStringGetNumBytes(wiFiConfiguration));
    if (err) {
        HAPLogError(&logObject, "Write to %s failed.", filePath);
    }
    return err;
}

/**
 * Applies the wpa_supplicant configuration. Persists the WiFi credentials passed in.
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
 * Restarts the WiFi interface if restartWiFi is set to true
#endif
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
 * @return kHAPError_None           Applying the configuration was successful.
 * @return kHAPError_Unknown        Applying the configuration fails.
 * @return kHAPError_OutOfResources Restarting wifi fails as the buffer passed to store results is small.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerApplyConfiguration(
        HAPPlatformWiFiManagerRef wiFiManager,
        const char* _Nullable ssid,
        const char* _Nullable const passphrase,
        const char* _Nullable const regulatoryDomain
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
        ,
        HAPPlatformWiFiManagerCookie cookie,
        uint32_t updateStatus,
        bool restartWiFi
#endif
) {
    HAPPrecondition(wiFiManager);

    HAPError err;
    HAPLogDebug(&logObject, "%s", __func__);

    // Get regulatory domain.
    char regulatoryDomainCountryCode[HAPPlatformCountryCode_MaxBytes + 1] = { 0 };
    if (regulatoryDomain) {
        HAPPrecondition(HAPASCIICharacterIsUppercaseLetter(regulatoryDomain[0]));
        HAPPrecondition(HAPASCIICharacterIsUppercaseLetter(regulatoryDomain[1]));
        HAPPrecondition(regulatoryDomain[2] == '\0');
        HAPRawBufferCopyBytes(regulatoryDomainCountryCode, regulatoryDomain, HAPPlatformCountryCode_MaxBytes + 1);
    } else {
        HAPPlatformRegulatoryDomain configuredRegulatoryDomain;
        err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &configuredRegulatoryDomain);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
            HAPLogError(
                    &logObject,
                    "%s: Regulatory domain has not been configured in wpa_supplicant configuration.",
                    __func__);
            HAPFatalError();
        }
        HAPRawBufferCopyBytes(
                regulatoryDomainCountryCode,
                configuredRegulatoryDomain.stringValue,
                HAPPlatformCountryCode_MaxBytes + 1);
    }

    // Initial configuration file.
    char wiFiConfiguration[WIFI_CONFIGURATION_STRING_MAX_LEN];
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
    err = HAPStringWithFormat(
            wiFiConfiguration,
            sizeof wiFiConfiguration,
            "country=%s\n"
            "ctrl_interface=DIR=%s GROUP=netdev\n"
            "update_config=1\n"
            "#cookie=%u\n"
            "#updateStatus=%u\n",
            regulatoryDomainCountryCode,
            kHAPPlatformWiFiManager_ControlInterfaceDirectory,
            cookie,
            updateStatus);
#else
    err = HAPStringWithFormat(
            wiFiConfiguration,
            sizeof wiFiConfiguration,
            "country=%s\n"
            "ctrl_interface=DIR=%s GROUP=netdev\n"
            "update_config=1\n",
            regulatoryDomainCountryCode,
            kHAPPlatformWiFiManager_ControlInterfaceDirectory);
#endif

    HAPAssert(!err);

    // Append network configuration.
    if (ssid && (HAPStringGetNumBytes(ssid) > 0)) {
        size_t n = HAPStringGetNumBytes(wiFiConfiguration);
        err = HAPStringWithFormat(
                &wiFiConfiguration[n],
                sizeof wiFiConfiguration - n,
                "\n"
                "network={\n"
                "    ssid=\"");
        HAPAssert(!err);
        n += HAPStringGetNumBytes(&wiFiConfiguration[n]);

        // SSID.
        size_t numSSIDBytes = HAPStringGetNumBytes(ssid);
        for (size_t i = 0; i < numSSIDBytes; i++) {
            err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "%c", ssid[i]);
            HAPAssert(!err);
            n += HAPStringGetNumBytes(&wiFiConfiguration[n]);
        }

        err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "\"\n");
        HAPAssert(!err);
        n += HAPStringGetNumBytes(&wiFiConfiguration[n]);

        // Passphrase.
        if (!passphrase) {
            err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "    key_mgmt=NONE\n");
            HAPAssert(!err);
            n += HAPStringGetNumBytes(&wiFiConfiguration[n]);
        } else {
            err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "    psk=");
            HAPAssert(!err);
            n += HAPStringGetNumBytes(&wiFiConfiguration[n]);

            size_t numPassphraseBytes = HAPStringGetNumBytes(passphrase);

            if (numPassphraseBytes >= 2 * kHAPWiFiWPAPSK_NumBytes) {
                // PSK, 64 hex characters.
                for (size_t i = 0; i < 2 * kHAPWiFiWPAPSK_NumBytes; i++) {
                    err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "%c", passphrase[i]);
                    HAPAssert(!err);
                    n += HAPStringGetNumBytes(&wiFiConfiguration[n]);
                }
            } else {
                HAPAssert(numPassphraseBytes >= 8 && numPassphraseBytes <= 63);

                // Derive PSK.
                // - Supports special characters like " in the passphrase.
                // - Added security because the raw passphrase (which may be shared with other services) is not stored.
                uint8_t psk[kHAPWiFiWPAPSK_NumBytes];
                HAPWiFiGetWPAPSKForPassphrase(psk, ssid, passphrase);
                HAPLogSensitiveBufferDebug(&logObject, psk, sizeof psk, "PSK derived from passphrase.");

                for (size_t i = 0; i < sizeof psk; i++) {
                    err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "%02X", psk[i]);
                    HAPAssert(!err);
                    n += HAPStringGetNumBytes(&wiFiConfiguration[n]);
                }
            }

            err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "\n");
            HAPAssert(!err);
            n += HAPStringGetNumBytes(&wiFiConfiguration[n]);
        }

        // Network configuration terminator.
        err = HAPStringWithFormat(&wiFiConfiguration[n], sizeof wiFiConfiguration - n, "}\n");
        HAPAssert(!err);
    }

    // Update WiFi configuration.
    err = UpdateWiFiConfiguration(wiFiManager, kHAPPlatformWiFiManager_WPASupplicantConfPath, wiFiConfiguration);
    if (err) {
        HAPLogError(
                &logObject, "Failed to update WiFi configuration to %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        return err;
    }

    // Restart WiFi interface
#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
    if (restartWiFi) {
        err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
    }
#else
    err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
#endif
    return err;
}

/**
 * Clears the current wpa_supplicant configuration.
 *
 * @param      wiFiManager          WiFi manager
 *
 */
void HAPPlatformWiFiManagerClearConfiguration(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);
    HAPError err;

    HAPLogDebug(&logObject, "%s", __func__);

    // Restore original configuration.
    char wiFiConfigurationBytes[WIFI_CONFIGURATION_STRING_MAX_LEN];

    HAPPlatformRegulatoryDomain regulatoryDomain;
    err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(
                &logObject, "%s: Regulatory domain has not been configured in wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    err = HAPStringWithFormat(
            wiFiConfigurationBytes,
            sizeof wiFiConfigurationBytes,
            "country=%s\n"
            "ctrl_interface=DIR=%s GROUP=netdev\n"
            "update_config=1\n",
            regulatoryDomain.stringValue,
            kHAPPlatformWiFiManager_ControlInterfaceDirectory);
    HAPAssert(!err);

    // Update WiFi configuration.
    err = UpdateWiFiConfiguration(wiFiManager, kHAPPlatformWiFiManager_WPASupplicantConfPath, wiFiConfigurationBytes);
    if (err) {
        HAPLogError(
                &logObject, "Failed to update WiFi configuration to %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        HAPFatalError();
    }

    err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
    if (err) {
        HAPLogError(
                &logObject, "Failed to update WiFi configuration to %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
    }
}

/**
 * Removes the current wpa_supplicant configuration. If wpa supplicant original file exists, restore the wpa supplicant
 * from that file otherwise clear out the network configuration.
 *
 * @param      wiFiManager          WiFi manager
 *
 */
void HAPPlatformWiFiManagerRemoveConfiguration(HAPPlatformWiFiManagerRef wiFiManager) {
    HAPPrecondition(wiFiManager);

    HAPError err;

    HAPLogDebug(&logObject, "%s", __func__);

    // Restore original configuration.
    char wiFiConfigurationBytes[WIFI_CONFIGURATION_STRING_MAX_LEN];
    size_t numWiFiConfigurationBytes;
    bool wiFiConfigurationFound;

    err = HAPPlatformFileManagerReadFile(
            kHAPPlatformWiFiManager_WPASupplicantConfOrigPath,
            wiFiConfigurationBytes,
            sizeof wiFiConfigurationBytes - 1,
            &numWiFiConfigurationBytes,
            &wiFiConfigurationFound);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Read from %s failed.", kHAPPlatformWiFiManager_WPASupplicantConfOrigPath);
        HAPFatalError();
    }

    if (wiFiConfigurationFound) {
        wiFiConfigurationBytes[numWiFiConfigurationBytes] = '\0';
    } else {
        HAPPlatformRegulatoryDomain regulatoryDomain;
        err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
            HAPLogError(
                    &logObject,
                    "%s: Regulatory domain has not been configured in wpa_supplicant configuration.",
                    __func__);
            HAPFatalError();
        }

        err = HAPStringWithFormat(
                wiFiConfigurationBytes,
                sizeof wiFiConfigurationBytes,
                "country=%s\n"
                "ctrl_interface=DIR=%s GROUP=netdev\n"
                "update_config=1\n",
                regulatoryDomain.stringValue,
                kHAPPlatformWiFiManager_ControlInterfaceDirectory);
        HAPAssert(!err);
    }

    // Update WiFi configuration.
    err = UpdateWiFiConfiguration(wiFiManager, kHAPPlatformWiFiManager_WPASupplicantConfPath, wiFiConfigurationBytes);
    if (err) {
        HAPLogError(
                &logObject, "Failed to update WiFi configuration to %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        HAPFatalError();
    }
    err = HAPPlatformWiFiManagerRestartWiFi(wiFiManager);
    if (err) {
        HAPLogError(
                &logObject, "Failed to update WiFi configuration to %s", kHAPPlatformWiFiManager_WPASupplicantConfPath);
    }
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_WIFI_RECONFIGURATION)
/**
 * Helper function to get the psk from the wpa supplicant.
 *
 * @param wiFiManager               WiFi manager
 * @param[out] isPskConfigured      Set to true if PSK is present in the supplicant, false otherwise
 * @param[out] psk                  Actual PSK from the wpa supplicant
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading PSK.
 * @return kHAPError_InvalidState   If there is some error in retreiving the PSK.
 */
HAP_RESULT_USE_CHECK
HAPError getPSK(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPskConfigured, char* _Nonnull psk) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(psk);
    HAPPrecondition(isPskConfigured);

    *isPskConfigured = false;
    // Open wpa_supplicant configuration file.
    int fd;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            return kHAPError_InvalidState;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        return kHAPError_Unknown;
    }

    size_t i = 0;
    bool ignoreLine = false;
    for (;;) {
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
            break;
        }
        HAPAssert(n == sizeof c);

        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }
// Match psk prefix.
#define prefixString "psk="
        size_t numPrefixBytes = sizeof prefixString - 1;
        while (IsSpaceOrTab(c)) {
            do {
                n = read(fd, &c, sizeof c);
            } while (n == -1 && errno == EINTR);
            if (n < 0) {
                int _errno = errno;
                HAPAssert(n == -1);
                HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                return kHAPError_Unknown;
            }
            if (n == 0) {
                HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                break;
            }
            HAPAssert(n == sizeof c);
        }
        if (c != prefixString[i]) {
            ignoreLine = c != '\n';
            i = 0;
            continue;
        }

        i++;
        if (i == numPrefixBytes) {
            size_t index = 0;
            while (c != '\n' || (n == -1 && errno == EINTR)) {
                n = read(fd, &c, sizeof c);
                if (n < 0) {
                    int _errno = errno;
                    HAPAssert(n == -1);
                    HAPLogError(
                            &logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                    return kHAPError_Unknown;
                }
                if (n == 0) {
                    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                    break;
                }
                HAPAssert(n == sizeof c);
                if (IsSpaceOrTab(c) || c == '\"') {
                    continue;
                } else {
                    if (c != '\n') {
                        psk[index++] = c;
                    }
                }
            };
            if (index > 0 && index <= 2 * kHAPWiFiWPAPSK_NumBytes + 1) {
                *isPskConfigured = true;
            }
            break;
        }
#undef prefixString
    }
    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);

    (void) close(fd);
    return kHAPError_None;
}

/**
 * Sets psk configured to true if PSK is configured in the wpa supplicant, false otherwise.
 *
 *  - PSK can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param wiFiManager               WiFi manager.
 * @param[out] isPskConfigured      Set to true if PSK is present, false otherwise.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is an error in reading PSK.
 * @return kHAPError_InvalidState   If there is some error in retreiving the PSK to check if it's configured.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerIsPSKConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, bool* isPskConfigured) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(isPskConfigured);

    *isPskConfigured = false;
    // Open wpa_supplicant configuration file.
    int fd;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            return kHAPError_InvalidState;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        return kHAPError_Unknown;
    }

    size_t i = 0;
    bool ignoreLine = false;
    for (;;) {
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
            break;
        }
        HAPAssert(n == sizeof c);

        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }
// Match psk prefix.
#define prefixString "psk="
        size_t numPrefixBytes = sizeof prefixString - 1;
        while (IsSpaceOrTab(c)) {
            do {
                n = read(fd, &c, sizeof c);
            } while (n == -1 && errno == EINTR);
            if (n < 0) {
                int _errno = errno;
                HAPAssert(n == -1);
                HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                return kHAPError_Unknown;
            }
            if (n == 0) {
                HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                break;
            }
            HAPAssert(n == sizeof c);
        }
        if (c != prefixString[i]) {
            ignoreLine = c != '\n';
            i = 0;
            continue;
        }

        i++;
        if (i == numPrefixBytes) {
            size_t index = 0;
            while (c != '\n' || (n == -1 && errno == EINTR)) {
                n = read(fd, &c, sizeof c);
                if (n < 0) {
                    int _errno = errno;
                    HAPAssert(n == -1);
                    HAPLogError(
                            &logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                    return kHAPError_Unknown;
                }
                if (n == 0) {
                    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                    break;
                }
                HAPAssert(n == sizeof c);
                if (IsSpaceOrTab(c) || c == '\"') {
                    continue;
                } else {
                    index++;
                }
            };
            if (index > 0 && index <= 2 * kHAPWiFiWPAPSK_NumBytes + 1) {
                *isPskConfigured = true;
            }
            break;
        }
#undef prefixString
    }
    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);

    (void) close(fd);
    return kHAPError_None;
}

/**
 * Get the cookie value persisted in the wpa supplicant
 *
 *  - Cookie can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
 *
 * @param wiFiManager               WiFi manager
 * @param[out] cookie               Cookie value configured.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If there is some error in retreiving the cookie.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerGetCookie(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint16_t* _Nonnull cookie) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(cookie);

    *cookie = kHAPPlatformWiFiManagerCookie_Unmanaged;
    // Open wpa_supplicant configuration file.
    int fd;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            // Cookie value defaults to 0 if unconfigured
            return kHAPError_None;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        return kHAPError_Unknown;
    }

    size_t i = 0;
    bool ignoreLine = false;
    // Match #cookie prefix.
    const char* prefixString = "#cookie=";
    size_t numPrefixBytes = sizeof prefixString - 1;
    for (;;) {
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
            break;
        }
        HAPAssert(n == sizeof c);

        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }
        if (c != prefixString[i]) {
            ignoreLine = c != '\n';
            i = 0;
            continue;
        }

        i++;
        if (i == numPrefixBytes) {
            char cookieValue[UINT16_MAX_LEN] = { 0 };
            int index = 0;
            while (c != '\n' || (n == -1 && errno == EINTR)) {
                n = read(fd, &c, sizeof c);
                if (n < 0) {
                    int _errno = errno;
                    HAPAssert(n == -1);
                    HAPLogError(
                            &logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                    return kHAPError_Unknown;
                }
                if (n == 0) {
                    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                    break;
                }
                HAPAssert(n == sizeof c);
                if (IsSpaceOrTab(c)) {
                    continue;
                } else if (HAPASCIICharacterIsNumber(c)) {
                    cookieValue[index++] = c;
                }
            };
            *cookie = (uint16_t) atoi(&cookieValue[0]) & UINT16_MAX;
            break;
        }
    }
    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);

    (void) close(fd);
    return kHAPError_None;
}

/**
 * Get the updateStatus value persisted in the wpa supplicant
 *
 *  - Update Status can be configured system-wide in file /etc/wpa_supplicant/wpa_supplicant.conf.
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
        uint32_t* _Nonnull updateStatus) {
    HAPPrecondition(wiFiManager);
    HAPPrecondition(updateStatus);

    *updateStatus = 0;
    // Open wpa_supplicant configuration file.
    int fd;
    do {
        fd = open(kHAPPlatformWiFiManager_WPASupplicantConfPath, O_RDONLY);
    } while (fd == -1 && errno == EINTR);
    if (fd < 0) {
        int _errno = errno;
        if (_errno == ENOENT) {
            // Update status value defaults to 0 if unconfigured
            return kHAPError_None;
        }
        HAPAssert(fd == -1);
        HAPLogError(&logObject, "open %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
        HAPFatalError();
    }

    size_t i = 0;
    bool ignoreLine = false;
    // Match #updateStatus prefix.
    const char* prefixString = "#updateStatus=";
    size_t numPrefixBytes = sizeof prefixString - 1;
    for (;;) {
        char c;
        ssize_t n;
        do {
            n = read(fd, &c, sizeof c);
        } while (n == -1 && errno == EINTR);
        if (n < 0) {
            int _errno = errno;
            HAPAssert(n == -1);
            HAPLogError(&logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
            return kHAPError_Unknown;
        }
        if (n == 0) {
            HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
            break;
        }
        HAPAssert(n == sizeof c);

        if (ignoreLine) {
            if (c == '\n') {
                ignoreLine = false;
            }
            continue;
        }
        if (c != prefixString[i]) {
            ignoreLine = c != '\n';
            i = 0;
            continue;
        }

        i++;
        if (i == numPrefixBytes) {
            char updateStatusValue[UINT32_MAX_LEN] = { 0 };
            int index = 0;
            while (c != '\n' || (n == -1 && errno == EINTR)) {
                n = read(fd, &c, sizeof c);
                if (n < 0) {
                    int _errno = errno;
                    HAPAssert(n == -1);
                    HAPLogError(
                            &logObject, "read %s failed: %d.", kHAPPlatformWiFiManager_WPASupplicantConfPath, _errno);
                    return kHAPError_Unknown;
                }
                if (n == 0) {
                    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);
                    break;
                }
                HAPAssert(n == sizeof c);
                if (IsSpaceOrTab(c)) {
                    continue;
                } else if (HAPASCIICharacterIsNumber(c)) {
                    updateStatusValue[index++] = c;
                }
            };
            *updateStatus = (uint32_t) atoi(&updateStatusValue[0]);
            break;
        }
    }
    HAPLogDebug(&logObject, "%s:%u", __func__, __LINE__);

    (void) close(fd);
    return kHAPError_None;
}

/**
 * Backs up the WiFi network configuration. Reads from wpa_supplicant and backs up to the wpa_supplicant.orig file.
 *
 * @param      wiFiManager          WiFi manager.
 *
 * @return kHAPError_None           If the file was backed up successfully.
 * @return kHAPError_Unknown        If the file failed to back up.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerBackUpConfiguration(HAPPlatformWiFiManagerRef wiFiManager) {
    HAPPrecondition(wiFiManager);

    // Restore original configuration.
    char wiFiConfigurationBytes[WIFI_CONFIGURATION_STRING_MAX_LEN];
    size_t numWiFiConfigurationBytes;
    bool wiFiConfigurationFound;

    HAPError err = HAPPlatformFileManagerReadFile(
            kHAPPlatformWiFiManager_WPASupplicantConfPath,
            wiFiConfigurationBytes,
            sizeof wiFiConfigurationBytes - 1,
            &numWiFiConfigurationBytes,
            &wiFiConfigurationFound);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "Read from %s failed.", kHAPPlatformWiFiManager_WPASupplicantConfPath);
        return err;
    }

    if (wiFiConfigurationFound) {
        wiFiConfigurationBytes[numWiFiConfigurationBytes] = '\0';
    }

    // Update WiFi configuration.
    err = UpdateWiFiConfiguration(
            wiFiManager, kHAPPlatformWiFiManager_WPASupplicantConfOrigPath, wiFiConfigurationBytes);
    if (err) {
        HAPLogError(
                &logObject,
                "Failed to update WiFi configuration to %s",
                kHAPPlatformWiFiManager_WPASupplicantConfOrigPath);
        return err;
    }
    return err;
}

/**
 * Sets the updateStatus value in the wpa supplicant to the new value passed in
 *
 * @param wiFiManager               WiFi manager
 * @param updateStatus              UpdateStatus value to be set in the wpa supplicant
 *
 * @return kHAPError_None           Persisting the new update status value was successful.
 * @return kHAPError_Unknown        Persisting the new update status value fails.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetUpdateStatus(HAPPlatformWiFiManagerRef _Nonnull wiFiManager, uint32_t updateStatus) {
    HAPPrecondition(wiFiManager);

    HAPPlatformWiFiManagerCookie cookie = kHAPPlatformWiFiManagerCookie_Unmanaged;
    HAPError err = HAPPlatformWiFiManagerGetCookie(HAPNonnull(wiFiManager), &cookie);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: cookie not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformRegulatoryDomain regulatoryDomain;
    err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(
                &logObject, "%s: Regulatory domain has not been configured in wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformSSID ssid;
    bool isSSIDConfigured;
    err = HAPPlatformWiFiManagerGetSSID(HAPNonnull(wiFiManager), &isSSIDConfigured, &ssid);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    char psk[2 * kHAPWiFiWPAPSK_NumBytes + 1] = { 0 };

    bool isPSKConfigured;
    err = getPSK(HAPNonnull(wiFiManager), &isPSKConfigured, psk);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: PSK not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    bool isRegulatoryDomainConfigured = HAPStringGetNumBytes(regulatoryDomain.stringValue) != 0;

    err = HAPPlatformWiFiManagerApplyConfiguration(
            HAPNonnull(wiFiManager),
            ssid.stringValue,
            isPSKConfigured ? psk : NULL,
            isRegulatoryDomainConfigured ? regulatoryDomain.stringValue : NULL,
            cookie,
            updateStatus,
            false);
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
HAP_RESULT_USE_CHECK
HAPError HAPPlatformWiFiManagerSetCookie(
        HAPPlatformWiFiManagerRef _Nonnull wiFiManager,
        HAPPlatformWiFiManagerCookie cookie) {
    HAPPrecondition(wiFiManager);

    uint32_t updateStatus = 0;
    HAPError err = HAPPlatformWiFiManagerGetUpdateStatus(HAPNonnull(wiFiManager), &updateStatus);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: Update status not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformRegulatoryDomain regulatoryDomain;
    err = HAPPlatformWiFiManagerGetRegulatoryDomain(wiFiManager, &regulatoryDomain);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(
                &logObject, "%s: Regulatory domain has not been configured in wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    HAPPlatformSSID ssid;
    bool isSSIDConfigured;
    err = HAPPlatformWiFiManagerGetSSID(HAPNonnull(wiFiManager), &isSSIDConfigured, &ssid);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: SSID not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    char psk[2 * kHAPWiFiWPAPSK_NumBytes + 1] = { 0 };

    bool isPSKConfigured;
    err = getPSK(HAPNonnull(wiFiManager), &isPSKConfigured, psk);
    if (err) {
        HAPAssert(err == kHAPError_InvalidState || err == kHAPError_Unknown);
        HAPLogError(&logObject, "%s: PSK not read successfully from wpa_supplicant configuration.", __func__);
        HAPFatalError();
    }

    bool isRegulatoryDomainConfigured = HAPStringGetNumBytes(regulatoryDomain.stringValue) != 0;

    if (HAPStringGetNumBytes(ssid.stringValue) > 0) {
        err = HAPPlatformWiFiManagerApplyConfiguration(
                HAPNonnull(wiFiManager),
                ssid.stringValue,
                isPSKConfigured ? psk : NULL,
                isRegulatoryDomainConfigured ? regulatoryDomain.stringValue : NULL,
                cookie,
                updateStatus,
                false);
    } else {
        err = HAPPlatformWiFiManagerApplyConfiguration(
                HAPNonnull(wiFiManager),
                NULL,
                NULL,
                isRegulatoryDomainConfigured ? regulatoryDomain.stringValue : NULL,
                cookie,
                updateStatus,
                false);
    }
    return err;
}
#endif

/**
 * Parses the wpa_status result returned using the wpa_cli status command and finds the string to match and returns the
 * value.
 *
 * @param      result               Result of the wpa_cli status command
 * @param      size                 Size of the result
 * @param      stringToMatch        String token to match
 * @param[out] status               Value assigned to the string token in the result
 *
 */
void getWpaStatus(char* result, size_t size, const char* stringToMatch, char* status) {
    size_t index = 0;
    while (index < size) {
        if (result[index] == stringToMatch[0]) {
            index++;
            size_t j = 1;
            while (result[index] == stringToMatch[j] && j < HAPStringGetNumBytes(stringToMatch)) {
                index++;
                j++;
            }
            if (j == HAPStringGetNumBytes(stringToMatch)) {
                int i = 0;
                while (result[index] != '\n') {
                    status[i++] = result[index++];
                }
            }
        }
        index++;
    }
}

/**
 * Parses the wpa_status result returned using the wpa_cli status command and returns true if wpa state is connected
 *
 * @param      wiFiManager          WiFi manager
 *
 */
HAP_RESULT_USE_CHECK
bool HAPPlatformWiFiManagerIsWiFiLinkEstablished(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);
    HAPError err;
    char result[2000] = { 0 };

    char* const cmd[] = { "/sbin/wpa_cli", "-i", wiFiManager->interfaceName, "status", "verbose", NULL };

    size_t written;
    err = HAPPlatformSystemCommandRun(cmd, result, sizeof(result), &written);

    if (written > 0 && !err) {
        const char* stringToMatch = "wpa_state=";
        char ret[256] = { 0 };
        getWpaStatus(result, written, stringToMatch, &ret[0]);
        return HAPRawBufferAreEqual(ret, "COMPLETED", HAPStringGetNumBytes("COMPLETED")) ? true : false;
    }
    return false;
}

/**
 * Parses the wpa_status result returned using the wpa_cli status command and returns true if ip address is obtained
 * and not empty
 *
 * @param      wiFiManager          WiFi manager
 *
 */
bool HAPPlatformWiFiManagerIsWiFiNetworkConfigured(HAPPlatformWiFiManagerRef _Nonnull wiFiManager) {
    HAPPrecondition(wiFiManager);
    HAPError err;
    char result[2000] = { 0 };

    char* const cmd[] = { "/sbin/wpa_cli", "-i", wiFiManager->interfaceName, "status", "verbose", NULL };

    HAPRawBufferZero(result, sizeof(result));
    size_t written;
    err = HAPPlatformSystemCommandRun(cmd, result, sizeof(result), &written);

    if (written > 0 && !err) {
        const char* stringToMatch = "ip_address=";
        char ret[256] = { 0 };
        getWpaStatus(result, written, stringToMatch, &ret[0]);
        return (HAPStringGetNumBytes(ret) > 0) ? true : false;
    }
    return false;
}
