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

#include "HAPSRTP.h"

#include "HAPCrypto.h"

/**
 * Crypto definitions.
 */
#define kHAPSRTP_IVSize (16)

#define kHAPSRTP_IVSSRCPos  (4)
#define kHAPSRTP_IVLabelPos (7)
#define kHAPSRTP_IVIndexPos (10)

#define kHAPSRTP_HMACBytes (SHA1_BYTES)

/**
 * Get SRTP session key.
 *
 * @param[out] keyBytes             Key bytes.
 * @param      numKeyBytes          Number of key bytes.
 * @param      masterKeyBytes       Master key bytes.
 * @param      numMasterKeyBytes    Number of master key bytes.
 * @param      salt                 Salt.
 * @param      label                Label.
 */
static void HAPSRTPSessionKey(
        uint8_t* keyBytes,
        size_t numKeyBytes,
        const uint8_t* masterKeyBytes,
        size_t numMasterKeyBytes,
        const uint8_t salt[kHAPSRTP_SaltSize],
        int label) {
    HAP_aes_ctr_ctx aesContext;
    uint8_t iv[kHAPSRTP_IVSize];

    HAP_constant_time_fill_zero(keyBytes, numKeyBytes);
    HAP_constant_time_fill_zero(iv, kHAPSRTP_IVSize);
    HAP_constant_time_copy(iv, salt, kHAPSRTP_SaltSize);
    iv[kHAPSRTP_IVLabelPos] ^= (uint8_t) label;
    HAP_aes_ctr_init(&aesContext, masterKeyBytes, numMasterKeyBytes, iv);
    HAP_aes_ctr_encrypt(&aesContext, keyBytes, keyBytes, numKeyBytes);
    HAP_aes_ctr_done(&aesContext);
}

/**
 * XOR big-endian uint32 with byte buffer.
 *
 * @param[out] buffer               Byte buffer.
 * @param      bytes                Uint32 field to XOR.
 */
static void HAPXorBigUInt32(uint8_t* buffer, uint32_t bytes) {
    buffer[0] ^= (uint8_t)(bytes >> 24U);
    buffer[1] ^= (uint8_t)(bytes >> 16U);
    buffer[2] ^= (uint8_t)(bytes >> 8U);
    buffer[3] ^= (uint8_t)(bytes >> 0U);
}

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
        uint32_t ssrc) {
    if (srtpContext) {
        srtpContext->keySize = keySize;
        srtpContext->tagSize = tagSize;
        if (keySize) {
            HAPSRTPSessionKey(srtpContext->encrKey, keySize, key, keySize, salt, 0);
            HAPSRTPSessionKey(srtpContext->authKey, kHAPSRTP_AuthKeySize, key, keySize, salt, 1);
            HAPSRTPSessionKey(srtpContext->saltKey, kHAPSRTP_SaltSize, key, keySize, salt, 2);
            HAPXorBigUInt32(&srtpContext->saltKey[kHAPSRTP_IVSSRCPos], ssrc); // add ssrc to salt
        }
    }

    if (srtcpContext) {
        srtcpContext->keySize = keySize;
        srtcpContext->tagSize = tagSize;
        if (keySize) {
            HAPSRTPSessionKey(srtcpContext->encrKey, keySize, key, keySize, salt, 3);
            HAPSRTPSessionKey(srtcpContext->authKey, kHAPSRTP_AuthKeySize, key, keySize, salt, 4);
            HAPSRTPSessionKey(srtcpContext->saltKey, kHAPSRTP_SaltSize, key, keySize, salt, 5);
            HAPXorBigUInt32(&srtcpContext->saltKey[kHAPSRTP_IVSSRCPos], ssrc); // add ssrc to salt
        }
    }
}

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
        uint32_t index) {
    HAP_aes_ctr_ctx aesContext;
    uint8_t iv[kHAPSRTP_IVSize];

    HAP_constant_time_fill_zero(iv, kHAPSRTP_IVSize);
    HAP_constant_time_copy(iv, srtpContext->saltKey, kHAPSRTP_SaltSize);
    HAPXorBigUInt32(&iv[kHAPSRTP_IVIndexPos], index);
    HAP_aes_ctr_init(&aesContext, srtpContext->encrKey, srtpContext->keySize, iv);

    // encrypt header in packet
    if (numHeaderBytes) {
        HAP_aes_ctr_encrypt(&aesContext, packet, packet, numHeaderBytes);
        packet += numHeaderBytes;
    }
    // encrypt data to packet
    HAP_aes_ctr_encrypt(&aesContext, packet, dataBytes, numDataBytes);
    HAP_aes_ctr_done(&aesContext);
}

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
        uint32_t index) {
    HAP_aes_ctr_ctx aesContext;
    uint8_t iv[kHAPSRTP_IVSize];

    HAP_constant_time_fill_zero(iv, kHAPSRTP_IVSize);
    HAP_constant_time_copy(iv, srtpContext->saltKey, kHAPSRTP_SaltSize);
    HAPXorBigUInt32(&iv[kHAPSRTP_IVIndexPos], index);
    HAP_aes_ctr_init(&aesContext, srtpContext->encrKey, srtpContext->keySize, iv);

    // decrypt packet to data
    HAP_aes_ctr_encrypt(&aesContext, data, packetBytes, numPacketBytes);
    HAP_aes_ctr_done(&aesContext);
}

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
        uint32_t index) {
    uint8_t hmac[kHAPSRTP_HMACBytes];
    uint8_t aad[4];

    HAPWriteBigUInt32(aad, index);
    HAP_hmac_sha1_aad(hmac, context->authKey, kHAPSRTP_AuthKeySize, bytes, numBytes, aad, sizeof aad);
    HAP_constant_time_copy(tag, hmac, context->tagSize);
}

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
        uint32_t index) {
    uint8_t hmac[kHAPSRTP_HMACBytes];
    uint8_t aad[4];

    HAPWriteBigUInt32(aad, index);
    HAP_hmac_sha1_aad(hmac, context->authKey, kHAPSRTP_AuthKeySize, bytes, numBytes, aad, sizeof aad);
    return HAP_constant_time_equal(tag, hmac, context->tagSize) == 1;
}
