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

#include "HAPMFiTokenAuth.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPLogSubsystem.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "MFiTokenAuth" };

/**
 * TLV types used in HAP-Token-Response and HAP-Update-Token-Request.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Table 5-4 Software Authentication TLV types
 */
HAP_ENUM_BEGIN(uint8_t, HAPMFiTokenAuthTLVType) { /**
                                                   * UUID (The matching UUID for the initial token provisioned on the
                                                   * accessory). 16 bytes.
                                                   */
                                                  kHAPMFiTokenAuthTLVType_UUID = 0x01,

                                                  /**
                                                   * Software Token.
                                                   * Opaque blob, up to kHAPPlatformMFiTokenAuth_MaxMFiTokenBytes bytes.
                                                   */
                                                  kHAPMFiTokenAuthTLVType_SoftwareToken = 0x02
} HAP_ENUM_END(uint8_t, HAPMFiTokenAuthTLVType);

HAP_RESULT_USE_CHECK
HAPError HAPMFiTokenAuthGetTokenResponse(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        HAPTLVWriter* responseWriter) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(accessory);
    HAPPrecondition(responseWriter);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.17.2 HAP-Token-Response

    // Load Software Token.
    HAPPlatformMFiTokenAuthUUID mfiTokenUUID;
    void* mfiTokenBytes;
    size_t maxMFiTokenBytes;
    size_t numMFiTokenBytes = 0;
    HAPTLVWriterGetScratchBytes(responseWriter, &mfiTokenBytes, &maxMFiTokenBytes);
    bool hasMFiToken = false;
    if (server->platform.authentication.mfiTokenAuth) {
        err = HAPPlatformMFiTokenAuthLoad(
                HAPNonnull(server->platform.authentication.mfiTokenAuth),
                &hasMFiToken,
                &mfiTokenUUID,
                mfiTokenBytes,
                maxMFiTokenBytes,
                &numMFiTokenBytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            return err;
        }
    }
    if (!hasMFiToken) {
        HAPLog(&logObject, "Software Token requested but no token is provisioned.");
        return kHAPError_InvalidState;
    }

    // Software Token.
    // Do this first because scratch buffer gets invalidated on TLV append.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPMFiTokenAuthTLVType_SoftwareToken,
                              .value = { .bytes = mfiTokenBytes, .numBytes = numMFiTokenBytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    // UUID.
    err = HAPTLVWriterAppend(
            responseWriter,
            &(const HAPTLV) { .type = kHAPMFiTokenAuthTLVType_UUID,
                              .value = { .bytes = mfiTokenUUID.bytes, .numBytes = sizeof mfiTokenUUID.bytes } });
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMFiTokenAuthHandleTokenUpdateRequest(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        HAPTLVReader* requestReader) {
    HAPPrecondition(server);
    HAPPrecondition(session);
    HAPPrecondition(accessory);
    HAPPrecondition(requestReader);

    HAPError err;

    // See HomeKit Accessory Protocol Specification R17
    // Section 5.17.3 HAP-Token-Update-Request

    HAPTLV softwareTokenTLV;
    softwareTokenTLV.type = kHAPMFiTokenAuthTLVType_SoftwareToken;
    err = HAPTLVReaderGetAll(requestReader, (HAPTLV* const[]) { &softwareTokenTLV, NULL });
    if (err) {
        HAPAssert(err == kHAPError_InvalidData);
        return err;
    }

    // Validate Software Token.
    if (!softwareTokenTLV.value.bytes) {
        HAPLog(&logObject, "Software Token missing.");
        return kHAPError_InvalidData;
    }
    if (softwareTokenTLV.value.numBytes > kHAPPlatformMFiTokenAuth_MaxMFiTokenBytes) {
        HAPLog(&logObject, "Software Token has invalid length (%lu).", (unsigned long) softwareTokenTLV.value.numBytes);
        return kHAPError_InvalidData;
    }
    HAPLogSensitiveBuffer(&logObject, softwareTokenTLV.value.bytes, softwareTokenTLV.value.numBytes, "Software Token");

    // Update Token.
    HAPLogInfo(
            &logObject,
            "Updating Software Token (length = %lu bytes).",
            (unsigned long) softwareTokenTLV.value.numBytes);
    if (!server->platform.authentication.mfiTokenAuth) {
        HAPLog(&logObject, "Software Authentication not supported.");
        return kHAPError_Unknown;
    }
    err = HAPPlatformMFiTokenAuthUpdate(
            HAPNonnull(server->platform.authentication.mfiTokenAuth),
            HAPNonnullVoid(softwareTokenTLV.value.bytes),
            softwareTokenTLV.value.numBytes);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_TOKEN_AUTH)
