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

#ifndef HAP_MFI_HW_AUTH_TYPES_H
#define HAP_MFI_HW_AUTH_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Coprocessor register map.
 *
 * @see Accessory Interface Specification R32
 *      Section 67.5.7 Registers
 *
 * @see Accessory Interface Specification R29
 *      Section 69.8.1 Register Addresses
 */
HAP_ENUM_BEGIN(uint8_t, HAPMFiHWAuthRegister) {
    /**
     * Device Version.
     *
     * - Block: 0
     * - Length: 1 byte
     * - Power-Up Value: See HAPMFiHWAuthDeviceVersion
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_DeviceVersion = 0x00,

    /**
     * Authentication Revision / Firmware Version (2.0C).
     *
     *  - Block: 0
     *  - Length: 1 byte
     *  - Power-Up Value: 0x01
     *  - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AuthenticationRevision = 0x01,

    /**
     * Authentication Protocol Major Version.
     *
     * - Block: 0
     * - Length: 1 byte
     * - Power-Up Value: 0x03 / 0x02 (2.0C)
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AuthenticationProtocolMajorVersion = 0x02,

    /**
     * Authentication Protocol Minor Version.
     *
     * - Block: 0
     * - Length: 1 byte
     * - Power-Up Value: 0x00
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AuthenticationProtocolMinorVersion = 0x03,

    /**
     * Device ID.
     *
     * - Block: 0
     * - Length: 4 bytes
     * - Power-Up Value: 0x00000300 / 0x00000200 (2.0C)
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_DeviceID = 0x04,

    /**
     * Error Code.
     *
     * - Block: 0
     * - Length: 1 byte
     * - Power-Up Value: 0x00
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_ErrorCode = 0x05,

    /**
     * Authentication Control and Status.
     *
     * - Block: 1
     * - Length: 1 byte
     * - Power-Up Value: 0x00
     * - Access: Read/Write
     */
    kHAPMFiHWAuthRegister_AuthenticationControlAndStatus = 0x10,

    /**
     * Challenge Response Data Length.
     *
     * - Block: 1
     * - Length: 2 bytes
     * - Power-Up Value: 0 / 128 (2.0C)
     * - Access: Read-only / Read/Write (2.0C)
     */
    kHAPMFiHWAuthRegister_ChallengeResponseDataLength = 0x11,

    /**
     * Challenge Response Data.
     *
     * - Block: 1
     * - Length: 64 bytes / 128 bytes (2.0C)
     * - Power-Up Value: Undefined
     * - Access: Read-only / Read/Write (2.0C)
     */
    kHAPMFiHWAuthRegister_ChallengeResponseData = 0x12,

    /**
     * Challenge Data Length.
     *
     * - Block: 2
     * - Length: 2 bytes
     * - Power-Up Value: 0 / 20 (2.0C)
     * - Access: Read-only / Read/Write (2.0C)
     */
    kHAPMFiHWAuthRegister_ChallengeDataLength = 0x20,

    /**
     * Challenge Data.
     *
     * - Block: 2
     * - Length: 32 bytes / 128 bytes (2.0C)
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     */
    kHAPMFiHWAuthRegister_ChallengeData = 0x21,

    /**
     * Accessory Certificate Data Length.
     *
     * - Block: 3
     * - Length: 2 bytes
     * - Power-Up Value: 607-609 / <= 1280 (2.0C)
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateDataLength = 0x30,

    /**
     * Accessory Certificate Data 1.
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData1 = 0x31,

    /**
     * Accessory Certificate Data 2.
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData2 = 0x32,

    /**
     * Accessory Certificate Data 3.
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData3 = 0x33,

    /**
     * Accessory Certificate Data 4.
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData4 = 0x34,

    /**
     * Accessory Certificate Data 5.
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData5 = 0x35,

    /**
     * Accessory Certificate Data 6 (2.0C).
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData6 = 0x36,

    /**
     * Accessory Certificate Data 7 (2.0C).
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData7 = 0x37,

    /**
     * Accessory Certificate Data 8 (2.0C).
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData8 = 0x38,

    /**
     * Accessory Certificate Data 9 (2.0C).
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData9 = 0x39,

    /**
     * Accessory Certificate Data 10 (2.0C).
     *
     * - Block: 3
     * - Length: 128 bytes
     * - Power-Up Value: Certificate
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateData10 = 0x3A,

    /**
     * Self-Test Status / Self-Test Control and Status (2.0C).
     *
     * - Block: 4
     * - Length: 1 byte
     * - Power-Up Value: 0x00
     * - Access: Read-only / Read/Write (2.0C)
     */
    kHAPMFiHWAuthRegister_SelfTestStatus = 0x40,

    /**
     * System Event Counter (SEC) (2.0C).
     *
     * - Block: 4
     * - Length: 1 byte
     * - Power-Up Value: Undefined
     * - Access: Read-only
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_SystemEventCounter = 0x4D,

    /**
     * Device Certificate Serial Number / Accessory Certificate Serial Number (2.0C).
     *
     * - Block: 4
     * - Length: 32 bytes / 31 bytes (2.0C)
     * - Power-Up Value: Certificate / Null-terminated string (2.0C)
     * - Access: Read-only
     */
    kHAPMFiHWAuthRegister_AccessoryCertificateSerialNumber = 0x4E,

    /**
     * Apple Device Certificate Data Length (2.0C).
     *
     * - Block: 5
     * - Length: 2 bytes
     * - Power-Up Value: 0x0000
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateDataLength = 0x50,

    /**
     * Apple Device Certificate Data 1 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData1 = 0x51,

    /**
     * Apple Device Certificate Data 2 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData2 = 0x52,

    /**
     * Apple Device Certificate Data 3 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData3 = 0x53,

    /**
     * Apple Device Certificate Data 4 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData4 = 0x54,

    /**
     * Apple Device Certificate Data 5 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData5 = 0x55,

    /**
     * Apple Device Certificate Data 6 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData6 = 0x56,

    /**
     * Apple Device Certificate Data 7 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData7 = 0x57,

    /**
     * Apple Device Certificate Data 8 (2.0C).
     *
     * - Block: 5
     * - Length: 128 bytes
     * - Power-Up Value: Undefined
     * - Access: Read/Write
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthRegister_AppleDeviceCertificateData8 = 0x58,

    /**
     * Sleep.
     *
     * - Block: 1
     * - Length: 1 byte
     * - Power-Up Value: Undefined
     * - Access: Write-only
     */
    kHAPMFiHWAuthRegister_Sleep = 0x60
} HAP_ENUM_END(uint8_t, HAPMFiHWAuthRegister);

/**
 * Coprocessor device versions.
 */
HAP_ENUM_BEGIN(uint8_t, HAPMFiHWAuthDeviceVersion) { /**
                                                      * 2.0C.
                                                      *
                                                      * @see Accessory Interface Specification R29
                                                      *      Section 69.8.1 Register Addresses
                                                      *
                                                      * @remark Obsolete since R30.
                                                      */
                                                     kHAPMFiHWAuthDeviceVersion_2_0C = 0x05,

                                                     /**
                                                      * 3.0.
                                                      *
                                                      * @see Accessory Interface Specification R32
                                                      *      Section 67.5.7.1 Device Version
                                                      */
                                                     kHAPMFiHWAuthDeviceVersion_3_0 = 0x07
} HAP_ENUM_END(uint8_t, HAPMFiHWAuthDeviceVersion);

/**
 * Coprocessor error codes.
 *
 * @see Accessory Interface Specification R32
 *      Section 67.5.7.6 Error Code
 */
HAP_ENUM_BEGIN(uint8_t, HAPMFiHWAuthError) {
    /** No error. */
    kHAPMFiHWAuthError_NoError = 0x00,

    /**
     * Invalid register for read (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InvalidRegisterForRead = 0x01,

    /**
     * Invalid register specified or register is read-only / Invalid register for write (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     */
    kHAPMFiHWAuthError_InvalidRegister = 0x02,

    /**
     * Invalid challenge response length (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InvalidChallengeResponseLength = 0x03,

    /**
     * Invalid challenge length (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InvalidChallengeLength = 0x04,

    /**
     * Sequence error, command out of sequence / Invalid certificate length (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     */
    kHAPMFiHWAuthError_InvalidCommandSequence = 0x05,

    /** Internal process error during challenge response generation. */
    kHAPMFiHWAuthError_InternalChallengeResponseGenerationError = 0x06,

    /**
     * Internal process error during challenge generation (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InternalChallengeGenerationError = 0x07,

    /**
     * Internal process error during challenge response verification (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InternalChallengeResponseVerificationError = 0x08,

    /**
     * Internal process error during certificate validation (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InternalCertificateValidationError = 0x09,

    /**
     * Invalid process control (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InvalidProcessControl = 0x0A,

    /**
     * Process control out of sequence (2.0C).
     *
     * @see Accessory Interface Specification R29
     *      Section 69.8.2.5 Error Code
     *
     * @remark Obsolete since R30.
     */
    kHAPMFiHWAuthError_InvalidProcessControlSequence = 0x0B
} HAP_ENUM_END(uint8_t, HAPMFiHWAuthError);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
