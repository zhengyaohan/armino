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

#ifndef HAP_JSON_UTILS_H
#define HAP_JSON_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "util_json_reader.h"

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)

/**
 * Skips over a JSON value (object, array, string, number, 'true', 'false', or 'null').
 *
 * @param      reader               Reader used to skip over a JSON value.
 * @param      bytes                Buffer to read from.
 * @param      maxBytes             Maximum number of bytes to skip over.
 * @param[out] numBytes             Number of bytes skipped.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If a JSON syntax error was encountered.
 * @return kHAPError_OutOfResources If the JSON value is nested too deeply.
 */
HAP_RESULT_USE_CHECK
HAPError HAPJSONUtilsSkipValue(struct util_json_reader* reader, const char* bytes, size_t maxBytes, size_t* numBytes);

/**
 * Determines the space needed by the string representation of a float in JSON format.
 *
 * @param      value                Numeric value.
 *
 * @return Number of bytes that the value's string representation needs (excluding NULL-terminator).
 */
HAP_RESULT_USE_CHECK
size_t HAPJSONUtilsGetFloatNumDescriptionBytes(float value);

/**
 * Gets the string representation of a float value in JSON format.
 *
 * @param      value                Numeric value.
 * @param[out] bytes                Buffer to fill with the UUID's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPJSONUtilsGetFloatDescription(float value, char* bytes, size_t maxBytes);

/**
 * Returns the number of bytes of the provided UTF-8 encoded string data after escaping according to RFC 7159, Section 7
 * "Strings" (http://www.rfc-editor.org/rfc/rfc7159.txt).
 *
 * @param      bytes                Buffer with UTF-8 encoded string data bytes.
 * @param      numBytes             Number of string data bytes to escape.
 *
 * @return Number of bytes of the provided string data after escaping.
 */
HAP_RESULT_USE_CHECK
size_t HAPJSONUtilsGetNumEscapedStringDataBytes(const char* bytes, size_t numBytes);

/**
 * Escapes UTF-8 encoded string data according to RFC 7159, Section 7 "Strings"
 * (http://www.rfc-editor.org/rfc/rfc7159.txt).
 *
 * @param[in,out] bytes             Buffer with UTF-8 encoded string data bytes.
 * @param      maxBytes             Maximum number of string data bytes that may be filled into the buffer.
 * @param[in,out] numBytes          Number of string data bytes to escape on input,
 *                                  number of escaped string data bytes on output.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the buffer is too small for the escaped string data bytes.
 */
HAP_RESULT_USE_CHECK
HAPError HAPJSONUtilsEscapeStringData(char* bytes, size_t maxBytes, size_t* numBytes);

/**
 * Unescapes UTF-8 encoded string data according to RFC 7159, Section 7 "Strings"
 * (http://www.rfc-editor.org/rfc/rfc7159.txt).
 *
 * @param[in,out] bytes             Buffer with UTF-8 encoded string data bytes.
 * @param[in,out] numBytes          Number of string data bytes to unescape on input,
 *                                  number of unescaped string data bytes on output.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If a JSON syntax error was encountered.
 */
HAP_RESULT_USE_CHECK
HAPError HAPJSONUtilsUnescapeStringData(char* bytes, size_t* numBytes);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
