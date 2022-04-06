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

#ifndef HAP_ACCESSORY_SETUP_OWNERSHIP_H
#define HAP_ACCESSORY_SETUP_OWNERSHIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Accessory setup ownership.
 */
typedef struct HAPAccessorySetupOwnership HAPAccessorySetupOwnership;
typedef struct HAPAccessorySetupOwnership* HAPAccessorySetupOwnershipRef;

/**
 * Indicates whether an ownership proof token is required for accessory setup.
 *
 * @param      setupOwnership       Accessory setup ownership.
 *
 * @return true                     If an ownership proof token is required for accessory setup.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessorySetupOwnershipIsTokenRequired(HAPAccessorySetupOwnershipRef setupOwnership);

/**
 * Maximum duration that an ownership proof token for accessory setup may be valid.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 5.6 Ownership Proof Token
 */
#define kHAPAccessorySetupOwnership_MaxTokenDuration ((HAPTime)(10 * HAPMinute))

/**
 * Invalidates the current ownership proof token for accessory setup if applicable.
 *
 * - This may be used to invalidate an ownership proof token for accessory setup before its maximum validity duration.
 *
 * @param      setupOwnership       Accessory setup ownership.
 */
void HAPAccessorySetupOwnershipInvalidateToken(HAPAccessorySetupOwnershipRef setupOwnership);

/**
 * Indicates whether a given ownership proof token for accessory setup is valid.
 *
 * @param      setupOwnership       Accessory setup ownership.
 * @param      ownershipToken       Ownership proof token for accessory setup.
 *
 * @return true                     If the given ownership proof token for accessory setup is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessorySetupOwnershipIsTokenValid(
        HAPAccessorySetupOwnershipRef setupOwnership,
        const HAPAccessorySetupOwnershipProofToken* ownershipToken);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Accessory setup ownership.
 */
struct HAPAccessorySetupOwnership {
    // Do not access the instance fields directly.
    /**@cond */
    struct {
        HAPAccessorySetupOwnershipProofToken value;
        HAPPlatformTimerRef validityTimer;
    } token;
    bool isTokenRequired : 1;
    /**@endcond */
};

/**
 * Initializes the accessory setup ownership.
 *
 * @param[out] setupOwnership       Pointer to an allocated but uninitialized HAPAccessorySetupOwnership structure.
 */
void HAPAccessorySetupOwnershipCreate(HAPAccessorySetupOwnershipRef setupOwnership);

/**
 * De-initializes an Apple Authentication Coprocessor.
 *
 * @param      setupOwnership       Initialized accessory setup ownership.
 */
void HAPAccessorySetupOwnershipRelease(HAPAccessorySetupOwnershipRef setupOwnership);

/**
 * Updates whether an ownership proof token is required for accessory setup.
 *
 * - Ownership proof tokens are used when an accessory server supports out-of-band pairing mechanisms.
 *   A controller may only complete accessory setup successfully if a valid ownership proof token is presented.
 *   The ownership proof token has to be transferred to authorized controllers using an out-of-band mechanism.
 *
 * - This function may be called before the accessory server is started.
 *   This way, an ownership proof token is required for accessory setup as soon as the accessory server is started.
 *
 * - Whenever this function is called, the generated ownership proof token is invalidated if applicable.
 *
 * @param      setupOwnership       Accessory setup ownership.
 * @param      tokenRequired        Whether an ownership proof token should be required for accessory setup.
 */
void HAPAccessorySetupOwnershipSetTokenRequired(HAPAccessorySetupOwnershipRef setupOwnership, bool tokenRequired);

/**
 * Generates and returns a valid ownership proof token for accessory setup.
 *
 * - A controller may only complete accessory setup successfully if a valid ownership proof token is presented.
 *   The ownership proof token has to be transferred to authorized controllers using an out-of-band mechanism.
 *
 * - The ownership proof token is only valid for a limited time and for one pairing attempt.
 *   Accessory setup fails if an expired ownership proof token is presented.
 *
 * - Only the most recently generated ownership proof token is valid.
 *
 * @param      setupOwnership       Accessory setup ownership.
 * @param[out] ownershipToken       Ownership proof token for accessory setup.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If ownership proof token is not required for accessory setup.
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessorySetupOwnershipGenerateToken(
        HAPAccessorySetupOwnershipRef setupOwnership,
        HAPAccessorySetupOwnershipProofToken* ownershipToken);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
