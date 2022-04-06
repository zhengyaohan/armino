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

#ifndef HAP_BLE_SESSION_H
#define HAP_BLE_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPSession.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

/**
 * Initialize BLE specific part of a session.
 *
 * @param      server               Accessory server.
 * @param      session              Session of which the BLE specific part shall be initialized.
 */
void HAPBLESessionCreate(HAPAccessoryServer* server, HAPSession* session);

/**
 * De-initializes BLE specific part of a session.
 *
 * @param      bleSession           BLE session.
 */
void HAPBLESessionRelease(HAPBLESession* bleSession);

/**
 * Invalidates a BLE session.
 *
 * @param      server               Accessory server.
 * @param      bleSession           BLE Session.
 * @param      terminateLink        Whether or not the underlying connection should also be terminated.
 */
void HAPBLESessionInvalidate(HAPAccessoryServer* server, HAPBLESession* bleSession, bool terminateLink);

/**
 * Returns whether the session is terminal soon and no new transactions should be started.
 * This is to prevent ambiguities whether HAP-BLE transactions have been completed successfully, as the
 * HomeKit Accessory Protocol Specification does not define at which stage a transaction actually executes.
 *
 * - If this function is called in response to a GATT request, it is safe to disconnect immediately.
 *   Otherwise, recently written data may still be transmitting and one should wait until
 *   #HAPBLESessionIsSafeToDisconnect returns true.
 *
 * @param      bleSession           BLE session.
 *
 * @return true                     If session is terminal soon.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPBLESessionIsTerminalSoon(const HAPBLESession* bleSession);

/**
 * Returns whether the session is terminal and must be disconnected.
 *
 * - If this function is called in response to a GATT request, it is safe to disconnect immediately.
 *   Otherwise, recently written data may still be transmitting and one should wait until
 *   #HAPBLESessionIsSafeToDisconnect returns true.
 *
 * @param      bleSession           BLE session.
 *
 * @return true                     If session is terminal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPBLESessionIsTerminal(const HAPBLESession* bleSession);

/**
 * Returns whether it is safe to disconnect the Bluetooth link.
 *
 * @param      bleSession           BLE session.
 *
 * @return true                     If it is safe to disconnect the Bluetooth link.
 * @return false                    Otherwise. Data may still being sent.
 */
HAP_RESULT_USE_CHECK
bool HAPBLESessionIsSafeToDisconnect(const HAPBLESession* bleSession);

/**
 * Handles a sent GATT response.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 */
void HAPBLESessionDidSendGATTResponse(HAPAccessoryServer* server, HAPSession* session);

/**
 * Handles a started HAP-BLE procedure.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 */
void HAPBLESessionDidStartBLEProcedure(HAPAccessoryServer* server, HAPSession* session);

/**
 * Handles a started pairing procedure (Pair Verify / Add Pairing / Remove Pairing / List Pairing).
 * Pair Setup is not listed on purpose.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      pairingProcedureType Pairing procedure type.
 */
void HAPBLESessionDidStartPairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType);

/**
 * Handles a completed pairing procedure (Pair Verify / Add Pairing / Remove Pairing / List Pairing).
 * Pair Setup is not listed on purpose.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      pairingProcedureType Pairing procedure type.
 */
void HAPBLESessionDidCompletePairingProcedure(
        HAPAccessoryServer* server,
        HAPSession* session,
        HAPPairingProcedureType pairingProcedureType);

/**
 * Handles a completed PairSetup procedure.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 */
void HAPBLESessionDidPairSetupProcedure(HAPAccessoryServer* server, HAPSession* session);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
