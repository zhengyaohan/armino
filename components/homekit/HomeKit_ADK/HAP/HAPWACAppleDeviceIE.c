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
#include "HAPAccessorySetup.h"
#include "HAPDeviceID.h"
#include "HAPLogSubsystem.h"
#include "HAPWACAppleDeviceIE.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "WACAppleDeviceIE" };

/**
 * Apple Device IE Flags.
 *
 * @see Accessory Interface Specification - Wi-Fi Accessory Configuration Addendum R1
 *      Table 2-8 Flags
 */
/**@{*/
/**
 * Flags Byte 0.
 */
HAP_ENUM_BEGIN(uint8_t, HAPWACAppleDeviceIEFlagsByte0) {
    /** Bit 0 - Supports AirPlay. */
    kHAPWACAppleDeviceIEFlagsByte0_SupportsAirPlay = 1U << 7U,

    /** Bit 1 - Accessory is not configured. */
    kHAPWACAppleDeviceIEFlagsByte0_NotConfigured = 1U << 6U,

    /** Bit 2 - Supports MFi Configuration v1. */
    kHAPWACAppleDeviceIEFlagsByte0_SupportsMFiConfigurationV1 = 1U << 5U,

    /** Bit 6 - Supports WPS. */
    kHAPWACAppleDeviceIEFlagsByte0_SupportsWPS = 1U << 1U,

    /** Bit 7 - WPS is active on the accessory. */
    kHAPWACAppleDeviceIEFlagsByte0_WPSIsActive = 1U << 0U
} HAP_ENUM_END(uint8_t, HAPWACAppleDeviceIEFlagsByte0);

/**
 * Flags Byte 1.
 */
HAP_ENUM_BEGIN(uint8_t, HAPWACAppleDeviceIEFlagsByte1) {
    /** Bit 8 - Supports AirPrint. */
    kHAPWACAppleDeviceIEFlagsByte1_SupportsAirPrint = 1U << 7U,

    /**
     * Bit 9 - Device is paired to a HomeKit controller.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 3.3.1.3 Reconfiguration
     */
    kHAPWACAppleDeviceIEFlagsByte1_PairedWithHomeKit = 1U << 6U,

    /**
     * Bit 10 - Supports CarPlay over Wireless.
     *
     * @see Accessory Interface Specification R29
     *      Table 50-8 Flags
     *
     * @remark Obsolete since R30.
     */
    kHAPWACAppleDeviceIEFlagsByte1_SupportsCarPlayOverWireless = 1U << 5U,

    /** Bit 11 - Provides Internet access (e.g., cellular connectivity is supported, provisioned, and enabled). */
    kHAPWACAppleDeviceIEFlagsByte1_ProvidesInternetAccess = 1U << 4U,

    /** Bit 14 - Supports 2.4 GHz Wi-Fi networks. */
    kHAPWACAppleDeviceIEFlagsByte1_Supports2_4GHzWiFiNetworks = 1U << 1U,

    /** Bit 15 - Supports 5 GHz Wi-Fi networks. */
    kHAPWACAppleDeviceIEFlagsByte1_Supports5GHzWiFiNetworks = 1U << 0U
} HAP_ENUM_END(uint8_t, HAPWACAppleDeviceIEFlagsByte1);

/**
 * Flags Byte 2.
 */
HAP_ENUM_BEGIN(uint8_t, HAPWACAppleDeviceIEFlagsByte2) {
    /** Bit 17 - Supports HomeKit Accessory Protocol v1. */
    kHAPWACAppleDeviceIEFlagsByte2_SupportsHomeKitAccessoryProtocolV1 = 1U << 6U,

    /**
     * Bit 20 - Supports WAC2 for HomeKit Accessories.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Table 3-1 Additional Apple Device Information Element (IE) Flags
     */
    kHAPWACAppleDeviceIEFlagsByte2_SupportsWAC2 = 1U << 3U,

    /**
     * Bit 21 - Supports Apple Authentication Coprocessor.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Table 3-1 Additional Apple Device Information Element (IE) Flags
     */
    kHAPWACAppleDeviceIEFlagsByte2_SupportsMFiHWAuth = 1U << 2U,

    /**
     * Bit 22 - Supports Software-based Authentication.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Table 3-1 Additional Apple Device Information Element (IE) Flags
     */
    kHAPWACAppleDeviceIEFlagsByte2_SupportsMFiTokenAuth = 1U << 1U,

} HAP_ENUM_END(uint8_t, HAPWACAppleDeviceIEFlagsByte2);
/**@}*/

/**
 * Apple Device IE Elements.
 *
 * @see Accessory Interface Specification - Wi-Fi Accessory Configuration Addendum R1
 *      Table 2-7 Apple Device IE elements
 */
HAP_ENUM_BEGIN(uint8_t, HAPWACAppleDeviceIEElement) {
    /**
     * Flags.
     * n:bits.
     *
     * Flags about the accessory: b0-b7, b8-b15, etc.
     * Each flag is a bit. Bit numbering starts from the leftmost bit of the first byte and uses the minimum number of
     * bytes needed to encode the bits. For example:
     * If only bit 1 is set, it would be 0x40.
     * If bit 1 (0x40) and bit 7 (0x1) are set, it would be 0x41.
     * If bit 1 (0x40), bit 7 (0x01), and bit 10 (0x0020) are set, it would be 0x41, 0x20.
     * If only bit 10 (0x0020), it would be 0x00, 0x20.
     */
    kHAPWACAppleDeviceIEElement_Flags = 0x00,

    /**
     * Name.
     * UTF-8.
     *
     * Friendly name of the accessory.
     * This should only be provided if the user configured a custom name or the firmware of the accessory has reason to
     * believe it can provide a name that is better than the default name the client software will provide for it based
     * on the model. Due to localization issues, it is often better to only provide this element if the user has
     * configured a name.
     */
    kHAPWACAppleDeviceIEElement_Name = 0x01,

    /**
     * Manufacturer.
     * UTF-8.
     *
     * Machine-parsable manufacturer of the accessory (e.g., "Manufacturer").
     */
    kHAPWACAppleDeviceIEElement_Manufacturer = 0x02,

    /**
     * Model.
     * UTF-8.
     *
     * Machine-parsable model of the accessory (e.g., "AB1234").
     */
    kHAPWACAppleDeviceIEElement_Model = 0x03,

    /**
     * OUI.
     * 3 bytes.
     *
     * OUI of the accessory including this IE. (e.g., 0x00 0xA0 0x40).
     */
    kHAPWACAppleDeviceIEElement_OUI = 0x04,

    /**
     * Bluetooth MAC.
     * 6 bytes.
     *
     * MAC address of the Bluetooth radio, if applicable.
     */
    kHAPWACAppleDeviceIEElement_BluetoothMAC = 0x06,

    /**
     * Device ID.
     * 6 bytes.
     *
     * Globally unique ID for the accessory (e.g., the primary MAC address, such as 00:11:22:33:44:55).
     * This should be the primary MAC address of the device. If the device has multiple MAC addresses, one must be
     * chosen as the primary MAC address such that it never changes (e.g., does not depend on the network interface
     * currently active). The main purpose of this element is to allow devices to discover the accessory via Wi-Fi scans
     * and then later associate it with an IP-based discovery method, such as Bonjour (where the Device ID is expected
     * to be reported via the TXT record).
     */
    kHAPWACAppleDeviceIEElement_DeviceID = 0x07,

    /**
     * Category.
     * Integer.
     *
     * Category of the accessory.
     *
     * @remark Obsolete since R27.
     *
     * @see Accessory Interface Specification R26
     *      Section 44.1.3 Payload
     */
    kHAPWACAppleDeviceIEElement_Category = 0x08,

    /**
     * Setup Hash.
     * 4 bytes.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 3.3.1.2 Procedure
     */
    kHAPWACAppleDeviceIEElement_SetupHash = 0x09,

    /**
     * Vendor-specific.
     * n bytes.
     *
     * Same format as a normal vendor-specific IE element.
     */
    kHAPWACAppleDeviceIEElement_VendorSpecific = 0xDD
} HAP_ENUM_END(uint8_t, HAPWACAppleDeviceIEElement);

/**
 * Serializes the Apple Device IE used for WAC.
 *
 * @param      server              Accessory Server
 * @param[out] bytes                Apple Device IE buffer.
 * @param      maxBytes             Capacity of Apple Device IE buffer.
 * @param[out] numBytes             Effective length of Apple Device IE buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If Apple Device IE buffer is not long enough or IE content is too long.
 */
HAP_RESULT_USE_CHECK
HAPError HAPWACAppleDeviceIECreate(HAPAccessoryServer* server, void* bytes, size_t maxBytes, size_t* numBytes) {
    HAPPrecondition(server);
    HAPPrecondition(server->primaryAccessory);
    const HAPAccessory* accessory = server->primaryAccessory;
    HAPPrecondition(bytes);
    HAPPrecondition(numBytes);

    HAPError err;

    if (maxBytes > kHAPWACAppleDeviceIE_MaxBytes) {
        maxBytes = kHAPWACAppleDeviceIE_MaxBytes;
    }

    // See Accessory Interface Specification - Wi-Fi Accessory Configuration Addendum R1
    // Section 50.5 Apple Device Information Element (IE)
    uint8_t* b = bytes;

#define TryAppendToIEOrReturn(elementBytes, numElementBytes, elementDescription) \
    do { \
        if (maxBytes < (numElementBytes)) { \
            HAPLog(&logObject, "Device IE Element buffer not long enough to add: %s.", (elementDescription)); \
            return kHAPError_OutOfResources; \
        } \
        HAPRawBufferCopyBytes(b, (elementBytes), (numElementBytes)); \
        b += (numElementBytes); \
        maxBytes -= (numElementBytes); \
    } while (0)

    /* IE header. */ {
        static const uint8_t headerBytes[] = {
            // Vendor-specific element ID as specified in Wireless LAN Medium Access Control (MAC) and Physical Layer
            // (PHY) Specification, IEEE Std. 802.11 - 2007.
            0xDD,

            // Number of bytes in IE (excludes element ID and length bytes).
            0x00, // Will be set later.

            // Apple Inc. OUI reserved for this IE.
            0x00,
            0xA0,
            0x40,

            // Sub-type of the 00-A0-40 Apple Inc. OUI.
            0x00
        };
        TryAppendToIEOrReturn(headerBytes, sizeof headerBytes, "IE header");
    }

    /* Flags. */ {
        // Get Wi-Fi capabilities.
        HAPPlatformWiFiCapability wiFiCapabilities =
                HAPPlatformTCPStreamManagerGetWiFiCapability(HAPNonnull(server->platform.ip.tcpStreamManager));
        HAPAssert(wiFiCapabilities.supports2_4GHz || wiFiCapabilities.supports5GHz);

        // Byte 0.
        uint8_t byte0 = kHAPWACAppleDeviceIEFlagsByte0_NotConfigured;

        // Byte 1.
        uint8_t byte1 = 0;
        if (wiFiCapabilities.supports2_4GHz) {
            byte1 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte1_Supports2_4GHzWiFiNetworks;
        }
        if (wiFiCapabilities.supports5GHz) {
            byte1 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte1_Supports5GHzWiFiNetworks;
        }
        if (HAPAccessoryServerIsPaired(server)) {
            byte1 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte1_PairedWithHomeKit;
        }

        // Byte 2.
        uint8_t byte2 = kHAPWACAppleDeviceIEFlagsByte2_SupportsHomeKitAccessoryProtocolV1;
        byte2 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte2_SupportsWAC2;

        bool supportsAuthentication = false;
        if (HAPAccessoryServerSupportsMFiHWAuth(server)) {
            byte2 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte2_SupportsMFiHWAuth;
            supportsAuthentication = true;
        }
        if (server->platform.authentication.mfiTokenAuth) {
            byte2 |= (uint8_t) kHAPWACAppleDeviceIEFlagsByte2_SupportsMFiTokenAuth;
            supportsAuthentication = true;
        }
        HAPAssert(supportsAuthentication);

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define BIT23_LOG byte2 & 1U << 0U ? "\n- Bit 23." : ""

        // Log bits.
        HAPLogInfo(
                &logObject,
                "Enabled Apple Device IE bits:"
                "%s%s%s%s%s%s%s%s"
                "%s%s%s%s%s%s%s%s"
                "%s%s%s%s%s%s%s%s",
                byte0 & 1U << 7U ? "\n- Bit 0 - Supports AirPlay." : "",
                byte0 & 1U << 6U ? "\n- Bit 1 - Accessory is not configured." : "",
                byte0 & 1U << 5U ? "\n- Bit 2 - Supports MFi Configuration v1." : "",
                byte0 & 1U << 4U ? "\n- Bit 3." : "",
                byte0 & 1U << 3U ? "\n- Bit 4." : "",
                byte0 & 1U << 2U ? "\n- Bit 5." : "",
                byte0 & 1U << 1U ? "\n- Bit 6 - Supports WPS." : "",
                byte0 & 1U << 0U ? "\n- Bit 7 - WPS is active on the accessory." : "",
                byte1 & 1U << 7U ? "\n- Bit 8 - Supports AirPrint." : "",
                byte1 & 1U << 6U ? "\n- Bit 9 - Device is paired to a HomeKit controller." : "",
                byte1 & 1U << 5U ? "\n- Bit 10 - Supports CarPlay over Wireless." : "",
                byte1 & 1U << 4U ? "\n- Bit 11 - Provides Internet access." : "",
                byte1 & 1U << 3U ? "\n- Bit 12." : "",
                byte1 & 1U << 2U ? "\n- Bit 13." : "",
                byte1 & 1U << 1U ? "\n- Bit 14 - Supports 2.4 GHz Wi-Fi networks." : "",
                byte1 & 1U << 0U ? "\n- Bit 15 - Supports 5 GHz Wi-Fi networks." : "",
                byte2 & 1U << 7U ? "\n- Bit 16." : "",
                byte2 & 1U << 6U ? "\n- Bit 17 - Supports HomeKit Accessory Protocol v1." : "",
                byte2 & 1U << 5U ? "\n- Bit 18." : "",
                byte2 & 1U << 4U ? "\n- Bit 19." : "",
                byte2 & 1U << 3U ? "\n- Bit 20 - Supports WAC2 for HomeKit Accessories." : "",
                byte2 & 1U << 2U ? "\n- Bit 21 - Supports Apple Authentication Coprocessor." : "",
                byte2 & 1U << 1U ? "\n- Bit 22 - Supports Software-based Authentication." : "",
                BIT23_LOG);

#undef BIT23_LOG

        // Serialize flags.
        uint8_t flagsBytes[] = { kHAPWACAppleDeviceIEElement_Flags, 3, byte0, byte1, byte2 };
        TryAppendToIEOrReturn(flagsBytes, sizeof flagsBytes, "Flags");
    }

    /* Name. */ {
        size_t n = HAPStringGetNumBytes(accessory->name);
        HAPAssert(n <= 64);
        uint8_t nameHeaderBytes[] = { kHAPWACAppleDeviceIEElement_Name, (uint8_t) n };
        TryAppendToIEOrReturn(nameHeaderBytes, sizeof nameHeaderBytes, "Name (header)");
        TryAppendToIEOrReturn(accessory->name, n, "Name");
    }

    /* Manufacturer. */ {
        size_t n = HAPStringGetNumBytes(accessory->manufacturer);
        HAPAssert(n <= 64);
        uint8_t manufacturerHeaderBytes[] = { kHAPWACAppleDeviceIEElement_Manufacturer, (uint8_t) n };
        TryAppendToIEOrReturn(manufacturerHeaderBytes, sizeof manufacturerHeaderBytes, "Manufacturer (header)");
        TryAppendToIEOrReturn(accessory->manufacturer, n, "Manufacturer");
    }

    /* Model. */ {
        size_t n = HAPStringGetNumBytes(accessory->model);
        HAPAssert(n <= 64);
        uint8_t modelHeaderBytes[] = { kHAPWACAppleDeviceIEElement_Model, (uint8_t) n };
        TryAppendToIEOrReturn(modelHeaderBytes, sizeof modelHeaderBytes, "Model (header)");
        TryAppendToIEOrReturn(accessory->model, n, "Model");
    }

    /* Device ID. */ {
        HAPDeviceID deviceID;
        err = HAPDeviceIDGet(server, &deviceID);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        uint8_t deviceIDHeaderBytes[] = { kHAPWACAppleDeviceIEElement_DeviceID, sizeof deviceID.bytes };
        TryAppendToIEOrReturn(deviceIDHeaderBytes, sizeof deviceIDHeaderBytes, "Device ID (header)");
        TryAppendToIEOrReturn(deviceID.bytes, sizeof deviceID.bytes, "Device ID");
    }

    /* Category. */ {
        uint8_t categoryBytes[] = { kHAPWACAppleDeviceIEElement_Category,
                                    sizeof(uint16_t),
                                    HAPExpandLittleUInt16((uint16_t) accessory->category) };
        TryAppendToIEOrReturn(categoryBytes, sizeof categoryBytes, "Category");
    }

    /* Setup Hash. */ {
        // See HomeKit Accessory Protocol Specification R17
        // Section 3.3.1.2 Procedure
        HAPSetupID setupID;
        bool hasSetupID;
        HAPPlatformAccessorySetupLoadSetupID(server->platform.accessorySetup, &hasSetupID, &setupID);
        if (hasSetupID) {
            // Get Device ID string.
            HAPDeviceIDString deviceIDString;
            err = HAPDeviceIDGetAsString(server, &deviceIDString);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }

            // Get setup hash.
            HAPAccessorySetupSetupHash setupHash;
            HAPAccessorySetupGetSetupHash(&setupHash, &setupID, &deviceIDString);
            uint8_t setupHashHeaderBytes[] = { kHAPWACAppleDeviceIEElement_SetupHash, sizeof setupHash.bytes };
            TryAppendToIEOrReturn(setupHashHeaderBytes, sizeof setupHashHeaderBytes, "Setup hash (header)");
            TryAppendToIEOrReturn(setupHash.bytes, sizeof setupHash.bytes, "Setup hash");
        }
    }

#undef TryAppendToIEOrReturn

    // Update total length.
    *numBytes = (size_t)(b - (uint8_t*) bytes);
    HAPAssert(*numBytes <= kHAPWACAppleDeviceIE_MaxBytes);
    HAPAssert(*numBytes - 2 <= UINT8_MAX);
    ((uint8_t*) bytes)[1] = (uint8_t)(*numBytes - 2);

    HAPLogBufferInfo(&logObject, bytes, *numBytes, "Apple Device IE.");
    return kHAPError_None;
}

#endif
