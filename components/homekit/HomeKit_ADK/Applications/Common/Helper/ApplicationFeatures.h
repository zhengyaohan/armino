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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.

#ifndef APPLICATION_FEATURES_H
#define APPLICATION_FEATURES_H

#include "HAPPlatformFeatures.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * HAP over BLE feature
 */
#ifndef HAVE_BLE
#define HAVE_BLE 0
#endif

/**
 * BLE scan feature
 */
#ifndef HAVE_BLE_SCAN
#define HAVE_BLE_SCAN 0
#endif

/**
 * BLE peripheral feature
 */
#ifndef HAVE_BLE_PERIPHERAL
#define HAVE_BLE_PERIPHERAL 0
#endif

#ifndef HAVE_IP
#define HAVE_IP 1
#endif

#ifndef HAVE_THREAD
#define HAVE_THREAD 0
#endif

#ifndef HAVE_DISPLAY
#define HAVE_DISPLAY 0
#endif

#ifndef HAVE_NFC
#define HAVE_NFC 0
#endif

#ifndef HAVE_MFI_HW_AUTH
#define HAVE_MFI_HW_AUTH 0
#endif

#ifndef HAVE_MFI_TOKEN_AUTH
#define HAVE_MFI_TOKEN_AUTH 1
#endif


#ifndef HAVE_WAC
#define HAVE_WAC 1
#endif

#ifndef HAVE_ADAPTIVE_LIGHT
#define HAVE_ADAPTIVE_LIGHT 0
#endif

#ifndef HAVE_ACCESS_CODE
#define HAVE_ACCESS_CODE 0
#endif

#ifndef HAVE_NFC_ACCESS
#define HAVE_NFC_ACCESS 0
#endif

#ifndef HAVE_DIAGNOSTICS_SERVICE
#define HAVE_DIAGNOSTICS_SERVICE 0
#endif

#ifndef HAVE_ACCESSORY_METRICS
#define HAVE_ACCESSORY_METRICS 0
#endif

#ifndef HAVE_ACCESSORY_REACHABILITY
#define HAVE_ACCESSORY_REACHABILITY 0
#endif

#ifndef HAVE_VIDEODOORBELL_OPERATING_STATE
#define HAVE_VIDEODOORBELL_OPERATING_STATE 0
#endif

#ifndef HAVE_VIDEODOORBELL_SILENT_MODE
#define HAVE_VIDEODOORBELL_SILENT_MODE 0
#endif

#ifndef HAVE_FIRMWARE_UPDATE
#define HAVE_FIRMWARE_UPDATE 1
#endif

#ifndef HAVE_UARP_SUPPORT
#define HAVE_UARP_SUPPORT 1
#endif

#ifndef HAVE_HDS_TRANSPORT_OVER_HAP
#define HAVE_HDS_TRANSPORT_OVER_HAP 0
#endif

#ifndef HAVE_WIFI_RECONFIGURATION
#define HAVE_WIFI_RECONFIGURATION 1
#endif

#ifndef HAVE_BLE_DISCOVERY
#define HAVE_BLE_DISCOVERY 0
#endif

#ifndef HAVE_BLE_PERIPHERAL
#define HAVE_BLE_PERIPHERAL 0
#endif

#ifndef HAVE_BLE_SCAN
#define HAVE_BLE_SCAN 0
#endif

#ifndef HAVE_CUSTOM_LPM
#define HAVE_CUSTOM_LPM 0
#endif

#ifndef HAVE_KEY_EXPORT
#define HAVE_KEY_EXPORT 0
#endif

#ifndef HAVE_LOCK_ENC
#define HAVE_LOCK_ENC 0
#endif

#ifndef HAVE_GATEWAY_PING
#define HAVE_GATEWAY_PING 0
#endif

#ifndef HAP_APP_USES_HDS
#define HAP_APP_USES_HDS 0
#endif

#ifndef HAP_APP_USES_AAD
#define HAP_APP_USES_AAD 0
#endif

#ifndef HAP_APP_USES_HDS_STREAM
#define HAP_APP_USES_HDS_STREAM 0
#endif

#ifndef SENSOR_MOTION
#define SENSOR_MOTION 0
#endif

#ifndef SENSOR_AIR_QUALITY
#define SENSOR_AIR_QUALITY 0
#endif

#ifndef SENSOR_CARBON_DIOXIDE
#define SENSOR_CARBON_DIOXIDE 0
#endif

#ifndef SENSOR_CARBON_MONOXIDE
#define SENSOR_CARBON_MONOXIDE 0
#endif

#ifndef SENSOR_HUMIDITY
#define SENSOR_HUMIDITY 0
#endif

#ifndef SENSOR_LEAK
#define SENSOR_LEAK 0
#endif

#ifndef SENSOR_LIGHT
#define SENSOR_LIGHT 0
#endif

#ifndef SENSOR_OCCUPANCY
#define SENSOR_OCCUPANCY 0
#endif

#ifndef SENSOR_SMOKE
#define SENSOR_SMOKE 0
#endif

#ifndef SENSOR_TEMPERATURE
#define SENSOR_TEMPERATURE 0
#endif

#ifndef SENSOR_AIR_QUALITY
#define SENSOR_AIR_QUALITY 0
#endif

#ifndef SENSOR_CONTACT
#define SENSOR_CONTACT 0
#endif

#ifndef HAVE_PORTRAIT_MODE
#define HAVE_PORTRAIT_MODE 0
#endif

#ifndef HAVE_COLOR_TEMPERATURE
#define HAVE_COLOR_TEMPERATURE 0
#endif

#ifndef HAP_SIRI_REMOTE
#define HAP_SIRI_REMOTE 0
#endif

#ifndef HAVE_BATTERY_POWERED_RECORDER
#define HAVE_BATTERY_POWERED_RECORDER 0
#endif

#ifndef HAVE_THREAD_DECOMMISSION_ON_UNPAIR
#define HAVE_THREAD_DECOMMISSION_ON_UNPAIR 0
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
