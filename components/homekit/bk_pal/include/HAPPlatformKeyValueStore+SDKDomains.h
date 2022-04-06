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

#ifndef HAP_PLATFORM_KEY_VALUE_STORE_SDK_DOMAINS_H
#define HAP_PLATFORM_KEY_VALUE_STORE_SDK_DOMAINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Statically provisioned data.
 *
 * Purged: Never.
 */
#define kSDKKeyValueStoreDomain_Provisioning ((HAPPlatformKeyValueStoreDomain) 0x40)

/**
 * IP Camera Operating Mode configuration.
 *
 * Purged: On factory reset.
 */
#define kSDKKeyValueStoreDomain_IPCameraOperatingMode ((HAPPlatformKeyValueStoreDomain) 0x51)

/**
 * IP Camera paired Operating Mode configuration.
 *
 * Purged: On factory reset and on pairing reset.
 */
#define kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode ((HAPPlatformKeyValueStoreDomain) 0x61)

/**
 * IP Camera Recorder configuration.
 *
 * Purged: On factory reset and on pairing reset.
 */
#define kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration ((HAPPlatformKeyValueStoreDomain) 0x71)


#define kSDKKeyValueStoreDomain_Vendor ((HAPPlatformKeyValueStoreDomain) 0x7d)

#define kSDKKeyValueStoreDomain_WiFiManager ((HAPPlatformKeyValueStoreDomain) 0x7e)
#define kSDKKeyValueStoreDomain_WiFiManager_bak ((HAPPlatformKeyValueStoreDomain) 0x7f)




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Static setup code SRP salt & verifier.
 *
 * Format: HAPSetupInfo.
 */
#define kSDKKeyValueStoreKey_Provisioning_SetupInfo ((HAPPlatformKeyValueStoreKey) 0x10)

/**
 * Setup ID.
 *
 * Format: HAPSetupID.
 */
#define kSDKKeyValueStoreKey_Provisioning_SetupID ((HAPPlatformKeyValueStoreKey) 0x11)

/**
 * Setup code for NFC.
 *
 * Format: HAPSetupCode.
 */
#define kSDKKeyValueStoreKey_Provisioning_SetupCode ((HAPPlatformKeyValueStoreKey) 0x12)

/**
 * Software Token UUID.
 *
 * Format: HAPPlatformMFiTokenAuthUUID.
 */
#define kSDKKeyValueStoreKey_Provisioning_MFiTokenUUID ((HAPPlatformKeyValueStoreKey) 0x20)

/**
 * Software Token.
 *
 * Format: Opaque blob, up to kHAPPlatformMFiTokenAuth_MaxMFiTokenBytes bytes.
 */
#define kSDKKeyValueStoreKey_Provisioning_MFiToken ((HAPPlatformKeyValueStoreKey) 0x21)


#define kSDKKeyValueStoreKey_Vendor_cat ((HAPPlatformKeyValueStoreKey) 0x00)
#define kSDKKeyValueStoreKey_Vendor_name ((HAPPlatformKeyValueStoreKey) 0x01)
#define kSDKKeyValueStoreKey_Vendor_product ((HAPPlatformKeyValueStoreKey) 0x02)
#define kSDKKeyValueStoreKey_Vendor_manu ((HAPPlatformKeyValueStoreKey) 0x03)
#define kSDKKeyValueStoreKey_Vendor_model ((HAPPlatformKeyValueStoreKey) 0x04)
#define kSDKKeyValueStoreKey_Vendor_sn ((HAPPlatformKeyValueStoreKey) 0x05)
#define kSDKKeyValueStoreKey_Vendor_UpdateSize ((HAPPlatformKeyValueStoreKey) 0x06)



#define kSDKKeyValueStoreKey_WiFiManager_country ((HAPPlatformKeyValueStoreKey) 0x00)
#define kSDKKeyValueStoreKey_WiFiManager_cookie ((HAPPlatformKeyValueStoreKey) 0x01)
#define kSDKKeyValueStoreKey_WiFiManager_ssid ((HAPPlatformKeyValueStoreKey) 0x02)
#define kSDKKeyValueStoreKey_WiFiManager_psk ((HAPPlatformKeyValueStoreKey) 0x03)
#define kSDKKeyValueStoreKey_WiFiManager_status ((HAPPlatformKeyValueStoreKey) 0x04)
#define kSDKKeyValueStoreKey_WiFiManager_dhcp ((HAPPlatformKeyValueStoreKey) 0x05)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// kSDKKeyValueStoreDomain_IPCameraOperatingMode.
// Format: little endian
//     <version = 0 : 1 byte>
//     <cameraIdentifier : uint64_t>
//     <streamingActive : 128 bits (1U << streamIndex represents status for streamIndex)>
//     <thirdPartyActive: uint8_t>
//     <manuallyDisabled: uint8_t>
// Length: 27 bytes

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// kSDKKeyValueStoreDomain_IPCameraPairedOperatingMode.
// Format: little endian
//     <version = 0 : 1 byte>
//     <cameraIdentifier : uint64_t>
//     <eventSnapshotsActive: uint8_t>
//     <periodicSnapshotsActive: uint8_t>
//     <homeKitActive: uint8_t>
//     <indicatorEnabled: uint8_t>
// Length: 13 bytes

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// kSDKKeyValueStoreDomain_IPCameraRecorderConfiguration.
// Format: little endian
//     <version = 0 : 1 byte>
//     <cameraIdentifier : uint64_t>
//     <recordingActive : uint8_t>
//     <audioEnabled: uint8_t>
//     Selected Configuration (optional):
//         <prebufferDuration : uint32_t (milliseconds)>
//         <eventTriggerTypes : 64 bits HAPCameraEventTriggerTypes>
//         <containerType : HAPMediaContainerType>
//         union of
//             fragmentedMP4:
//                 <fragmentDuration : uint32_t (milliseconds)>
//         <videoCodecType : HAPVideoCodecType>
//         union of
//             h.264 :
//                 <profile : uint8_t (1 + bit index of HAPH264VideoCodecProfile)>
//                 <level : uint8_t (1 + bit index of HAPH264VideoCodecProfileLevel)>
//                 <packetizationMode : uint8_t (1 + bit index of HAPH264VideoCodecPacketizationMode)>
//                 <bitRate : uint32_t (kbit/s)>
//                 <iFrameInterval : uint32_t (ms)>
//         <videoWidth : uint16_t>
//         <videoHeight : uint16_t>
//         <videoMaxFrameRate : uint8_t>
//         <audioCodecType : HAPAudioCodecType>
//         <audioNumberOfChannels : uint8_t>
//         <audioBitRateMode : uint8_t (1 + bit index of HAPAudioCodecBitRateControlMode)>
//         <audioSampleRate : uint8_t (1 + bit index of HAPAudioCodecSampleRate)>
//         <audioBitRate : uint32_t (kbit/s)>
// Length: 11 bytes + 42 bytes

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
