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

#include "HAPAccessorySetupOwnership.h"
#include "HAPLog+Attributes.h"
#include "HAPLogSubsystem.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "AccessorySetupOwnership" };

//----------------------------------------------------------------------------------------------------------------------

void HAPAccessorySetupOwnershipCreate(HAPAccessorySetupOwnershipRef setupOwnership) {
    HAPPrecondition(setupOwnership);

    HAPRawBufferZero(setupOwnership, sizeof *setupOwnership);
}

void HAPAccessorySetupOwnershipRelease(HAPAccessorySetupOwnershipRef setupOwnership) {
    HAPPrecondition(setupOwnership);

    HAPAccessorySetupOwnershipInvalidateToken(setupOwnership);
    HAPRawBufferZero(setupOwnership, sizeof *setupOwnership);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
bool HAPAccessorySetupOwnershipIsTokenRequired(HAPAccessorySetupOwnershipRef setupOwnership) {
    HAPPrecondition(setupOwnership);

    return setupOwnership->isTokenRequired;
}

void HAPAccessorySetupOwnershipSetTokenRequired(HAPAccessorySetupOwnershipRef setupOwnership, bool tokenRequired) {
    HAPPrecondition(setupOwnership);

    HAPLogInfo(&logObject, "Ownership proof token required for accessory setup: %s", tokenRequired ? "Yes" : "No");
    HAPAccessorySetupOwnershipInvalidateToken(setupOwnership);

    if (tokenRequired != setupOwnership->isTokenRequired) {
        setupOwnership->isTokenRequired = tokenRequired;
    }
}

//----------------------------------------------------------------------------------------------------------------------

static void HandleTokenValidityTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPAccessorySetupOwnershipRef setupOwnership = context;
    HAPPrecondition(timer == setupOwnership->token.validityTimer);
    HAPLogDebug(&logObject, "Ownership proof token validity timer expired.");
    setupOwnership->token.validityTimer = 0;

    HAPRawBufferZero(&setupOwnership->token, sizeof setupOwnership->token);
}

HAP_RESULT_USE_CHECK
HAPError HAPAccessorySetupOwnershipGenerateToken(
        HAPAccessorySetupOwnershipRef setupOwnership,
        HAPAccessorySetupOwnershipProofToken* ownershipToken) {
    HAPPrecondition(setupOwnership);
    HAPPrecondition(ownershipToken);

    HAPError err;

    if (!setupOwnership->isTokenRequired) {
        HAPLogError(&logObject, "Cannot generate ownership proof token: Not required for accessory setup.");
        return kHAPError_InvalidState;
    }

    HAPAccessorySetupOwnershipInvalidateToken(setupOwnership);

    HAPPlatformRandomNumberFill(setupOwnership->token.value.bytes, sizeof setupOwnership->token.value.bytes);
    HAPLogSensitiveBufferDebug(
            &logObject,
            setupOwnership->token.value.bytes,
            sizeof setupOwnership->token.value.bytes,
            "Generated ownership proof token.");
    HAPRawBufferCopyBytes(ownershipToken, &setupOwnership->token.value, sizeof *ownershipToken);

    err = HAPPlatformTimerRegister(
            &setupOwnership->token.validityTimer,
            HAPPlatformClockGetCurrent() + kHAPAccessorySetupOwnership_MaxTokenDuration,
            HandleTokenValidityTimerExpired,
            setupOwnership);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(&logObject, "%s failed: %u.", "HAPPlatformTimerRegister", err);
        HAPFatalError();
    }

    return kHAPError_None;
}

void HAPAccessorySetupOwnershipInvalidateToken(HAPAccessorySetupOwnershipRef setupOwnership) {
    HAPPrecondition(setupOwnership);

    if (!setupOwnership->token.validityTimer) {
        return;
    }

    HAPLogInfo(&logObject, "Invalidating ownership proof token.");

    HAPPlatformTimerDeregister(setupOwnership->token.validityTimer);
    setupOwnership->token.validityTimer = 0;

    HAPRawBufferZero(&setupOwnership->token, sizeof setupOwnership->token);
}

HAP_RESULT_USE_CHECK
bool HAPAccessorySetupOwnershipIsTokenValid(
        HAPAccessorySetupOwnershipRef setupOwnership,
        const HAPAccessorySetupOwnershipProofToken* ownershipToken) {
    HAPPrecondition(setupOwnership);
    HAPPrecondition(ownershipToken);

    if (!setupOwnership->isTokenRequired) {
        HAPLog(&logObject, "Accepting ownership proof token: Not required for accessory setup.");
        return true;
    }
    if (!setupOwnership->token.validityTimer) {
        HAPLog(&logObject, "Rejecting ownership proof token: No valid ownership proof token is present.");
        return false;
    }
    return HAPRawBufferAreEqual(ownershipToken->bytes, setupOwnership->token.value.bytes, sizeof ownershipToken->bytes);
}
