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

#ifndef HAP_SRTP_H
#define HAP_SRTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/** SRTP Authentication Key Size */
#define kHAPSRTP_AuthKeySize (20)

/** SRTP Salt Size */
#define kHAPSRTP_SaltSize (14)

/** SRTP Maximum Key Size */
#define kHAPSRTP_MaxKeySize (32)

/** SRTP Context */
typedef struct {
    /** Key size [bytes] */
    uint32_t keySize;

    /** Tag size [bytes] */
    uint32_t tagSize;

    /** Session encryption key (max 256 bit) */
    uint8_t encrKey[kHAPSRTP_MaxKeySize];

    /** Session authentication key (160 bit) */
    uint8_t authKey[kHAPSRTP_AuthKeySize];

    /** Session salt (112 bit) */
    uint8_t saltKey[kHAPSRTP_SaltSize];
} HAPSRTPContext;

/**
 * Setup SRTP contexts.
 *
 * @param[out] srtpContext          SRTP context to be setup.
 * @param[out] srtcpContext         SRTCP context to be setup.
 * @param      key                  Master key.
 * @param      keySize              Size of the master key, if one is specified (16, 24, or 32 byte).
 * @param      salt                 Master salt, if a master key is specified.
 * @param      tagSize              Size of the authentication tag.
 * @param      ssrc                 Synchronization source.
 */
void HAPSRTPSetupContext(
        HAPSRTPContext* srtpContext,
        HAPSRTPContext* srtcpContext,
        const uint8_t* _Nullable key,
        uint32_t keySize,
        const uint8_t* _Nullable salt,
        uint32_t tagSize,
        uint32_t ssrc);

/**
 * Encrypt SRTP packet.
 *
 * The final packet consists of @p numHeaderBytes encrypted in place, followed
 * by @p numDataBytes copied from @p dataBytes during encryption.
 *
 * @param         srtpContext       SRTP context.
 * @param[in,out] packet            Encrypted packet.
 * @param         dataBytes         Data bytes to be encrypted.
 * @param         numHeaderBytes    Number of header bytes.
 * @param         numDataBytes      Number of data bytes.
 * @param         index             Packet index.
 */
void HAPSRTPEncrypt(
        const HAPSRTPContext* srtpContext,
        uint8_t* packet,
        const void* dataBytes,
        size_t numHeaderBytes,
        size_t numDataBytes,
        uint32_t index);

/**
 * Decrypt SRTP packet.
 *
 * @param      srtpContext          SRTP context.
 * @param[out] data                 Decrypted data.
 * @param      packetBytes          Packet bytes.
 * @param      numPacketBytes       Number of packet bytes.
 * @param      index                Packet index.
 */
void HAPSRTPDecrypt(
        const HAPSRTPContext* srtpContext,
        uint8_t* data,
        const void* packetBytes,
        size_t numPacketBytes,
        uint32_t index);

/**
 * Generate SRTP authentication tag from bytes and index.
 *
 * @param      context              SRTP context.
 * @param[out] tag                  Authentication tag generated.
 * @param      bytes                Byte buffer.
 * @param      numBytes             Number of bytes in buffer.
 * @param      index                Index.
 */
void HAPSRTPAuthenticate(
        const HAPSRTPContext* context,
        uint8_t* tag,
        const void* bytes,
        size_t numBytes,
        uint32_t index);

/**
 * Check SRTP authentication tag against bytes and index.
 *
 * @param      context              SRTP context.
 * @param      tag                  Tag.
 * @param      bytes                Byte buffer.
 * @param      numBytes             Number of bytes in buffer.
 * @param      index                Index.
 *
 * @return                          1 if the tag is valid, 0 otherwise.
 */
int HAPSRTPVerifyAuthentication(
        const HAPSRTPContext* context,
        const uint8_t* tag,
        const void* bytes,
        size_t numBytes,
        uint32_t index);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
