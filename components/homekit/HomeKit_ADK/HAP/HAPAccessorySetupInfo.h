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

#ifndef HAP_ACCESSORY_SETUP_INFO_H
#define HAP_ACCESSORY_SETUP_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPAccessorySetup.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Duration after which a dynamic setup code expires.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 4.4.2.1 Requirements
 */
#define kHAPAccessorySetupInfo_DynamicRefreshInterval ((HAPTime)(5 * HAPMinute))

/**
 * Fetches the currently active setup info.
 *
 * - This may only be called while a pairing attempt is in progress.
 *
 * @param      server               Accessory server.
 * @param      restorePrevious      Whether or not a setup info from a previous pairing attempt should be reused.
 *
 * @return Setup info, if available. NULL otherwise. Always non-NULL if restorePrevious is false.
 */
HAP_RESULT_USE_CHECK
HAPSetupInfo* _Nullable HAPAccessorySetupInfoGetSetupInfo(HAPAccessoryServer* server, bool restorePrevious);

/**
 * Handles accessory server start.
 *
 * - This starts periodically generating setup codes for displays.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoHandleAccessoryServerStart(HAPAccessoryServer* server);

/**
 * Handles accessory server stop.
 *
 * - This invalidates generated setup codes for displays and exits NFC pairing mode.
 *
 * - Must not be called if there are still active connections.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoHandleAccessoryServerStop(HAPAccessoryServer* server);

/**
 * Handles accessory server state update.
 *
 * - This starts periodically generating setup codes for displays if the accessory server was unpaired.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoHandleAccessoryServerStateUpdate(HAPAccessoryServer* server);

/**
 * Handles start of a pairing attempt.
 *
 * - This locks the current setup code so it does not change during the pairing attempt.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoHandlePairingStart(HAPAccessoryServer* server);

/**
 * Handles completion of a pairing attempt.
 *
 * - If pairing is successful, this stops advertising a setup code.
 *   Otherwise for displays a new setup code is generated.
 *
 * @param      server               Accessory server.
 * @param      keepSetupInfo        Whether or not the current setup info should be preserved for next pairing attempt.
 */
void HAPAccessorySetupInfoHandlePairingStop(HAPAccessoryServer* server, bool keepSetupInfo);

/**
 * Refreshes the setup payload.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoRefreshSetupPayload(HAPAccessoryServer* server);

/**
 * Enters NFC pairing mode.
 *
 * - NFC pairing mode exits automatically after 5 minutes or when pairing completes.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoEnterNFCPairingMode(HAPAccessoryServer* server);

/**
 * Exits NFC pairing mode.
 *
 * @param      server               Accessory server.
 */
void HAPAccessorySetupInfoExitNFCPairingMode(HAPAccessoryServer* server);

/**
 * Generates a setup payload based on the given parameters
 *
 * @param      server               Accessory server.
 * @param      flags                Setup payload flags
 * @param      setupCode            Setup code
 * @param      pairable             True if it should be a pairable payload, otherwise false
 * @param[out] payload              The payload to write to
 */
void HAPAccessorySetupInfoGenerateSetupPayload(
        HAPAccessoryServer* server,
        HAPAccessorySetupSetupPayloadFlags flags,
        HAPSetupCode* _Nullable setupCode,
        bool pairable,
        HAPSetupPayload* payload);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
