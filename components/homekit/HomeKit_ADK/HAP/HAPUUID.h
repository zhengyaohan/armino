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

#ifndef HAP_UUID_H
#define HAP_UUID_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAP+API.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Creates a HAPUUID structure from a short UUID that is based on the Apple-defined HAP Base UUID.
 *
 * - Full UUIDs have the form XXXXXXXX-0000-1000-8000-0026BB765291.
 *   The short form consists of just the front part, e.g., 0x43 for the HomeKit Light Bulb service.
 *   UUID strings use hexadecimal digits - remember to use the 0x prefix.
 *
 * - This function may only be used for Apple-defined types.
 *   For vendor-specific UUIDs, a different base UUID must be used.
 *
 * @param      uuid                 Short UUID, e.g., 0x43 for the HomeKit Light Bulb service.
 *
 * @return Initialized HAPUUID structure.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.6.1 Service and Characteristic Types
 */
#define HAPUUIDCreateAppleDefined(uuid) \
    { \
        { 0x91, 0x52, 0x76, 0xBB, 0x26, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, HAPExpandLittleUInt32(uuid) } \
    }

/**
 * Creates a HAPUUID structure from a short UUID that is used for debugging purposes only.
 */
#if (HAP_TESTING == 1)
#define HAPUUIDCreateDebug(uuid) \
    { \
        { 0xC6, 0xCF, 0x82, 0xA1, 0xA3, 0xB4, 0xD2, 0xA3, 0xDF, 0x40, 0xF4, 0xE8, HAPExpandLittleUInt32(uuid) } \
    }
#endif

/**
 * Returns whether a HAP UUID is Apple defined.
 *
 * @param      uuid                 UUID.
 *
 * @return true                     If the UUID is Apple defined.
 * @return false                    Otherwise.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.6.1 Service and Characteristic Types
 */
HAP_RESULT_USE_CHECK
bool HAPUUIDIsAppleDefined(const HAPUUID* uuid);

/**
 * Determines the space needed by the string representation of a HAP UUID.
 *
 * @param      uuid                 UUID.
 *
 * @return Number of bytes that the UUID's string representation needs (excluding NULL-terminator).
 */
HAP_RESULT_USE_CHECK
size_t HAPUUIDGetNumDescriptionBytes(const HAPUUID* uuid);

/**
 * Gets the string representation of a HAP UUID.
 *
 * @param      uuid                 UUID.
 * @param[out] bytes                Buffer to fill with the UUID's string representation. Will be NULL-terminated.
 * @param      maxBytes             Capacity of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 */
HAP_RESULT_USE_CHECK
HAPError HAPUUIDGetDescription(const HAPUUID* uuid, char* bytes, size_t maxBytes);

/**
 * Gets the short form of a HAP UUID.
 *
 * - When Apple-defined UUIDs based on the HAP Base UUID 00000000-0000-1000-8000-0026BB765291 are encoded
 *   in short form, the -0000-1000-8000-0026BB765291 suffix is omitted and leading zero bytes are removed.
 *   The remaining bytes are sent in the same order as when sending a full UUID.
 *   To convert back to a full UUID, the process is reversed.
 *
 * - Custom types do not use the HAP Base UUID and are encoded in the same format as the full UUID.
 *
 * - Examples:
 *   00000000-0000-1000-8000-0026BB765291 -> []
 *   0000003E-0000-1000-8000-0026BB765291 -> [0x3E]
 *   00000001-0000-1000-8000-0026BB765291 -> [0x01]
 *   00000F25-0000-1000-8000-0026BB765291 -> [0x25, 0x0F]
 *   0000BBAB-0000-1000-8000-0026BB765291 -> [0xAB, 0xBB]
 *   00112233-0000-1000-8000-0026BB765291 -> [0x33, 0x22, 0x11]
 *   010004FF-0000-1000-8000-0026BB765291 -> [0xFF, 0x04, 0x00, 0x01]
 *   FF000000-0000-1000-8000-0026BB765291 -> [0x00, 0x00, 0x00, 0xFF]
 *
 * @param      uuid                 UUID.
 * @param[out] bytes                Buffer to fill with the UUID's compact representation.
 * @param      maxBytes             Capacity of buffer.
 * @param[out] numBytes             Effective length of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 6.6.1 Service and Characteristic Types
 */
HAP_RESULT_USE_CHECK
HAPError HAPUUIDGetShortFormBytes(const HAPUUID* uuid, void* bytes, size_t maxBytes, size_t* numBytes);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
