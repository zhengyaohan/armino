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

#ifndef ACCESS_CODE_HELPER_H
#define ACCESS_CODE_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if (HAVE_ACCESS_CODE == 1)
/**
 * Minimum length of an access code
 */
#define kAccessCodeMinimumLength 4

/**
 * Maximum length of an access code
 */
#define kAccessCodeMaximumLength 8

/**
 * Maximum number of access codes
 */
#define kAccessCodeListSize 8

/**
 * Configuration state change callback function
 */
typedef void (*HAPConfigurationStateChangeCallback)(void);

/**
 * Creates access code service platform.
 *
 * @param      keyValueStore                    key value store where the access code list is stored.
 * @param      storeDomain                      key value store domain dedicated to the access code list.
 * @param      configurationStateChangeCallback the callback function called when configuration state has changed
 * @param[out] hapAccessoryServerOptions        accessory server options some of which will be setup by this function
 *
 * @return kHAPError_None when successful.
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeCreate(
        HAPPlatformKeyValueStoreRef _Nonnull keyValueStore,
        HAPPlatformKeyValueStoreDomain storeDomain,
        HAPConfigurationStateChangeCallback _Nullable configurationStateChangeCallback,
        HAPAccessoryServerOptions* _Nonnull hapAccessoryServerOptions);

/**
 * Handles accessory service restarts.
 *
 * This may be called when the key value store content changes out of bound
 * for example by factory reset.
 *
 * @return kHAPError_None when successful.
 */
HAPError AccessCodeRestart(void);

/**
 * Clears all stored access code.
 *
 * @return kHAPError_None when successful.
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeClear(void);

/**
 * Access Code operation callback function
 *
 * @param[inout] op     operation and result
 * @param        ctx    context
 *
 * @return kHAPError_None when operation could result into a response whether successful or not.<br>
 *         Other error code when an unexpected error occurred and a normal response cannot be generated.
 */
HAPError AccessCodeHandleOperation(HAPAccessCodeOperation* op, void* _Nullable ctx);

/**
 * Handle read request to the 'Access Code Supported Configuration' characteristic of the Access Code service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeSupportedConfigurationRead(
        HAPAccessoryServer* _Nonnull server,
        const HAPTLV8CharacteristicReadRequest* _Nonnull request,
        HAPTLVWriter* _Nonnull responseWriter,
        void* _Nullable context HAP_UNUSED);

/**
 * Handle read request to the 'Configuration State' characteristic of the Access Code service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleAccessCodeConfigurationStateRead(
        HAPAccessoryServer* _Nonnull server,
        const HAPUInt16CharacteristicReadRequest* _Nonnull request HAP_UNUSED,
        uint16_t* _Nonnull value,
        void* _Nullable context HAP_UNUSED);

#if (HAP_TESTING == 1)
/**
 * Look up the access code to see if it is stored
 *
 * @param       accessCode            The access code to look up
 * @param[out]  accessCodeIdentifier  The access code identifier if access code is found
 *
 * @return true if found, false otherwise
 */
HAP_RESULT_USE_CHECK
bool AccessCodeLookUp(const char* accessCode, uint32_t* accessCodeIdentifier);

/**
 * Sets access code supported configuration character set
 *
 * @param   value   The new character set value
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeSetCharacterSet(uint8_t value);

/**
 * Sets access code supported configuration minimum length
 *
 * @param   value   The new minimum length value
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMinimumLength(uint8_t value);

/**
 * Sets access code supported configuration maximum length
 *
 * @param   value   The new maximum length value
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMaximumLength(uint8_t value);

/**
 * Sets access code supported configuration maximum access codes
 *
 * @param   value   The new maximum access codes value
 */
HAP_RESULT_USE_CHECK
HAPError AccessCodeSetMaximumAccessCodes(uint16_t value);
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif // (HAVE_ACCESS_CODE == 1)
#endif // ACCESS_CODE_HELPER_H
