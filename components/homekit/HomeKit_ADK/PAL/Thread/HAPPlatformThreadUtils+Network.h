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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.
#ifndef HAP_PLATFORM_THREAD_UTILS_NETWORK_H
#define HAP_PLATFORM_THREAD_UTILS_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"
#include "HAPPlatformFeatures.h"

#include "HAPPlatformThreadUtils+Commissioning.h"

//*****************************************************************************
// HAP Thread Utils - Network module.
//    This module provides operations that manage the existing Thread Network.
//    It allows clients to scan for viable networks, look for nodes on the network,
//    and maintain our existing connection to the Thread Network.
//*****************************************************************************
#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Thread device current role
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformThreadDeviceRole) {
    /** Disabled */
    kHAPPlatformThreadDeviceRole_Disabled = 0,
    /** Detached */
    kHAPPlatformThreadDeviceRole_Detached = 1,
    /** Joining */
    kHAPPlatformThreadDeviceRole_Joining = 2,
    /** Child */
    kHAPPlatformThreadDeviceRole_Child = 3,
    /** Router */
    kHAPPlatformThreadDeviceRole_Router = 4,
    /** Leader */
    kHAPPlatformThreadDeviceRole_Leader = 5,
    /** Border Router */
    kHAPPlatformThreadDeviceRole_BR = 6,
} HAP_ENUM_END(uint8_t, HAPPlatformThreadDeviceRole);

//*****************************************************************************
// Network Scanning - These functions handle scanning for thread networks that
//                    match our configuration
//*****************************************************************************

/**
 * Starts reachability test with a Thread network
 *
 * @param parameters    network parameters
 * @param duration      test duration
 * @param callback      callback to call. The passed argument should be the accessory server server pointer
 * @param server        accessory server pointer<br>
 *                      HAPAccessoryServer* type is obscured due to header file dependencies.
 */
void HAPPlatformThreadTestNetworkReachability(
        const HAPPlatformThreadNetworkParameters* parameters,
        HAPTime testDuration,
        HAPPlatformRunLoopCallback callback,
        void* server);

//*****************************************************************************
// Border Router Functions - These functions detect a border router on the
// network
//*****************************************************************************

/**
 * Type for the callback function to receive Thread border router presence state update
 *
 * @param borderRouterIsPresent  true if a border router is present. false, otherwise.
 *                               Border router must have an interface to reach beyond
 *                               the Thread mesh network.
 * @param context                context data
 */
typedef void (*HAPPlatformThreadBorderRouterStateCallback)(bool borderRouterIsPresent, void* context);

/**
 * Registers a callback function to receive border router presence state update.
 *
 * @param callback   callback function
 * @param context    data to pass to the callback function
 */
void HAPPlatformThreadRegisterBorderRouterStateCallback(
        HAPPlatformThreadBorderRouterStateCallback callback,
        void* context);

/**
 * Clears registration of the callback function to receive border router presence state update.
 *
 * @param callback   callback function
 */
void HAPPlatformThreadDeregisterBorderRouterStateCallback(HAPPlatformThreadBorderRouterStateCallback callback);

//*****************************************************************************
// Network State - These functions handle Thread network state updates and
//                 Our accessory's role in the network
//*****************************************************************************

/**
 * This method must be called whenever there is a change in the Thread network.
 * It will determine the Accessory's role in the network, update Role Listeners,
 * and refresh MDNS advertisement as necessary
 *
 * @param flags     A bitfield indicating specific state that changed
 * @param context   The Thread Accessory Server
 */
void HAPPlatformThreadReportStateChanged(uint32_t flags, void* context);

/**
 * Updates current device role and make callback if necessary.
 *
 * @param newRole   new device role
 */
void HAPPlatformThreadUpdateDeviceRole(HAPPlatformThreadDeviceRole newRole);

/**
 * Retrieves Thread device current role
 *
 * @param[out] role  pointer to a variable to store the current role
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadGetRole(HAPPlatformThreadDeviceRole* role);

/**
 * Registers a callback function to handle role update
 *
 * @param callback  Callback function to be called in RunLoop context.
 *                  The function will receive @ref HAPPlatformThreadDeviceRole as its argument.
 */
void HAPPlatformThreadRegisterRoleUpdateCallback(HAPPlatformRunLoopCallback callback);

/**
 * Clears the callback function for Thread Role changes.
 */
void HAPPlatformThreadUnregisterRoleUpdateCallback(void);

//*****************************************************************************
// Network Viability - These functions determine if the thread network is
//   'Viable'.  A network is 'Viable' if
//   * The accessory can attach to it
//   * The network has a border router
//   * The network is serving srp-mdns-proxy
//*****************************************************************************

/**
 * Determines whether the thread network is viable.  A viable
 * network is one that ensures the node can be reached from
 * outside the mesh.  It must have a border router and a mdns
 * proxy.
 *
 * @return true if the network is viable, false otherwise
 */
bool HAPPlatformThreadNetworkIsViable(void);

//*****************************************************************************
// Network Upkeep - These functions must be called to allow the thread network
//                  to perform its processing tasks
//*****************************************************************************

/**
 * Function that must be called periodically to manage the thread network.
 */
void HAPPlatformThreadTick(void);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
