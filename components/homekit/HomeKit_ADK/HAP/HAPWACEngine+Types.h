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

#ifndef HAP_WAC_ENGINE_TYPES_H
#define HAP_WAC_ENGINE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * TLV Values.
 *
 * @see Accessory Interface Specification - Wi-Fi Accessory Configuration Addendum R1
 *      Table 2-1 Configuration TLVs
 */
HAP_ENUM_BEGIN(uint8_t, HAPWACTLVType) {
    /**
     * Unique 10 character string assigned by Apple to an app via the Provisioning Portal (e.g., 24D4XFAF43).
     * String.
     */
    kHAPWACTLVType_BundleSeedID = 0x01,

    /**
     * Firmware revision of the accessory.
     * String.
     */
    kHAPWACTLVType_FirmwareRevision = 0x02,

    /**
     * Hardware revision of the accessory.
     * String.
     */
    kHAPWACTLVType_HardwareRevision = 0x03,

    /**
     * BCP-47 language to configure the accessory for. See http://www.iana.org/assignments/language-subtag-registry.
     * String.
     */
    kHAPWACTLVType_Language = 0x04,

    /**
     * Manufacturer of the accessory (e.g., Apple).
     * String.
     */
    kHAPWACTLVType_Manufacturer = 0x05,

    /**
     * Reverse-DNS string describing supported MFi accessory protocols (e.g., com.acme.gadget) for accompanying
     * applications. Note: there may be more than one of this item if multiple protocols are supported.
     * String.
     */
    kHAPWACTLVType_MFiProtocol = 0x06,

    /**
     * Model name of the accessory (e.g., Accessory1,1).
     * String.
     */
    kHAPWACTLVType_Model = 0x07,

    /**
     * Name that accessory should use to advertise itself to the user.
     * String.
     */
    kHAPWACTLVType_Name = 0x08,

    /**
     * Password used to start an AirPlay stream to the accessory.
     * String.
     */
    kHAPWACTLVType_PlayPassword = 0x09,

    /**
     * Serial number of the accessory.
     * String.
     */
    kHAPWACTLVType_SerialNumber = 0x0A,

    /**
     * Wi-Fi PSK for joining a WPA-protected Wi-Fi network. If it is between 8 and 63 bytes each being 32-126 decimal,
     * inclusive then it is a pre-hashed password. Otherwise, it is expected to be a pre-hashed, 256-bit pre-shared key.
     * Data.
     */
    kHAPWACTLVType_WiFiPSK = 0x0B,

    /**
     * Wi-Fi SSID (network name) for the accessory to join. This should be UTF-8.
     * String.
     */
    kHAPWACTLVType_WiFiSSID = 0x0C,

    /**
     * ISO 3166-1 country code to indicate the regulatory domain in which the accessory is operating.
     * String.
     */
    kHAPWACTLVType_CountryCode = 0x10
} HAP_ENUM_END(uint8_t, HAPWACTLVType);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
