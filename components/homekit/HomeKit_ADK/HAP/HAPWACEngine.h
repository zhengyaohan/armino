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

#ifndef HAP_WAC_ENGINE_H
#define HAP_WAC_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"
#include "HAPIPAccessoryServer.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

/**
 * Returns whether a given passphrase is valid for a WPA/WPA2 personal network.
 *
 * @param      value                Value.
 *
 * @return true                     If the value is a valid passphrase for a WPA/WPA2 personal network.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWACEngineIsValidWPAPassphrase(const char* value);

/**
 * Returns whether a given country code is a valid ISO 3166-1 alpha-2 code.
 *
 * @param      value                Value.
 *
 * @return true                     If the value is a valid ISO 3166-1 alpha-2 country code.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPWACEngineIsValidCountryCode(const char* value);

/**
 * Type of a WAC engine message handler.
 *
 * - All WAC engine message handlers share the same signature.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      requestBytes         Request buffer. May be modified.
 * @param      numRequestBytes      Length of request buffer.
 * @param[out] responseBytes        Response buffer.
 * @param      maxResponseBytes     Capacity of response buffer.
 * @param[out] numResponseBytes     Effective length of response buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with the Apple Authentication Coprocessor failed.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_OutOfResources If out of resources to process request.
 * @return kHAPError_NotAuthorized  If the controller is not authorized to perform the operation.
 */
HAP_RESULT_USE_CHECK
typedef HAPError (*HAPWACEngineHandler)(
        HAPAccessoryServer* server,
        HAPIPSecuritySession* session,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes);

/**
 * Handles a /config message from a controller.
 *
 * @param      server               Accessory server.
 * @param      session              Session.
 * @param      requestBytes         Request buffer. May be modified.
 * @param      numRequestBytes      Length of request buffer.
 * @param[out] responseBytes        Response buffer.
 * @param      maxResponseBytes     Capacity of response buffer.
 * @param[out] numResponseBytes     Effective length of response buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
 * @return kHAPError_InvalidData    If the controller sent a malformed request.
 * @return kHAPError_NotAuthorized  If the controller is not authorized to perform the operation.
 */
HAP_RESULT_USE_CHECK
HAPError HAPWACEngineHandleConfig(
        HAPAccessoryServer* server,
        HAPIPSecuritySession* session,
        void* requestBytes,
        size_t numRequestBytes,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
