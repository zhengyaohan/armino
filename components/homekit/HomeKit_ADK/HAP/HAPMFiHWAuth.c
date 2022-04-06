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

#include "HAPMFiHWAuth.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPCrypto.h"
#include "HAPLogSubsystem.h"
#include "HAPMFiHWAuth+Types.h"

static const HAPLogObject logObject = { .subsystem = kHAP_LogSubsystem, .category = "MFiHWAuth" };

void HAPMFiHWAuthCreate(HAPMFiHWAuth* mfiHWAuth, HAPPlatformMFiHWAuthRef platformMFiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    mfiHWAuth->platformMFiHWAuth = platformMFiHWAuth;
    mfiHWAuth->powerOffTimer = 0;
}

void HAPMFiHWAuthRelease(HAPMFiHWAuth* mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    if (!HAPMFiHWAuthIsSafeToRelease(mfiHWAuth)) {
        HAPLog(&logObject,
               "De-initializing Apple Authentication Coprocessor that does not report ready for power off.");
    }

    // Deinitialize timer.
    if (mfiHWAuth->powerOffTimer) {
        HAPPlatformTimerDeregister(mfiHWAuth->powerOffTimer);
        mfiHWAuth->powerOffTimer = 0;
    }

    HAPRawBufferZero(mfiHWAuth, sizeof *mfiHWAuth);
}

HAP_RESULT_USE_CHECK
bool HAPMFiHWAuthIsSafeToRelease(HAPMFiHWAuth* mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    if (!mfiHWAuth->platformMFiHWAuth) {
        return true;
    }

    if (!HAPPlatformMFiHWAuthIsPoweredOn(HAPNonnull(mfiHWAuth->platformMFiHWAuth))) {
        return false;
    }

    HAPError err;

    // Read Authentication Protocol Version.
    uint8_t protocolVersionMajor;
    {
        uint8_t bytes[1];
        err = HAPPlatformMFiHWAuthRead(
                HAPNonnull(mfiHWAuth->platformMFiHWAuth),
                kHAPMFiHWAuthRegister_AuthenticationProtocolMajorVersion,
                bytes,
                sizeof bytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Failed to read Authentication Protocol Major Version. Reporting safe to disable.");
            return true;
        }
        protocolVersionMajor = bytes[0];
    }

    if (protocolVersionMajor == 2) {
        // The System Event Counter (SEC) is a non-volatile register that holds the current value of the CP's event
        // counter. The event counter automatically decrements one count per second while the CP is powered,
        // stopping at 0. If the accessory controls power to the CP, it must wait until the SEC has decremented to 0
        // before removing power.
        // See Accessory Interface Specification R29
        // Section 69.8.2.14 System Event Counter
        uint8_t bytes[1];
        err = HAPPlatformMFiHWAuthRead(
                HAPNonnull(mfiHWAuth->platformMFiHWAuth),
                kHAPMFiHWAuthRegister_SystemEventCounter,
                bytes,
                sizeof bytes);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPLog(&logObject, "Failed to read System Event Counter. Reporting safe to disable.");
            return true;
        }
        uint8_t systemEventCounter = bytes[0];

        HAPLogDebug(&logObject, "System Event Counter = %u.", systemEventCounter);
        return systemEventCounter == 0;
    }

    return true;
}

static void PowerOffTimerExpired(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    HAPMFiHWAuth* mfiHWAuth = context;
    HAPPrecondition(timer == mfiHWAuth->powerOffTimer);
    mfiHWAuth->powerOffTimer = 0;
    HAPPrecondition(mfiHWAuth->platformMFiHWAuth);

    HAPError err;

    HAPAssert(HAPPlatformMFiHWAuthIsPoweredOn(HAPNonnull(mfiHWAuth->platformMFiHWAuth)));
    if (!HAPMFiHWAuthIsSafeToRelease(mfiHWAuth)) {
        // Apple Authentication Coprocessor should not be disabled. Extend power off timer.
        err = HAPPlatformTimerRegister(
                &mfiHWAuth->powerOffTimer,
                HAPPlatformClockGetCurrent() + 1 * HAPSecond,
                PowerOffTimerExpired,
                mfiHWAuth);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&logObject, "Not enough resources to extend power off timer. Leaving HW on!");
        }
    } else {
        HAPLogInfo(&logObject, "Turning off Apple Authentication Coprocessor.");

        // Disable Apple Authentication Coprocessor.
        HAPPlatformMFiHWAuthPowerOff(HAPNonnull(mfiHWAuth->platformMFiHWAuth));
    }
}

/**
 * Enables the Apple Authentication Coprocessor, if necessary.
 *
 * @param      mfiHWAuth            Apple Authentication Coprocessor manager.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If communication with the Apple Authentication Coprocessor failed.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPMFiHWAuthEnable(HAPMFiHWAuth* mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    HAPError err;

    if (!mfiHWAuth->platformMFiHWAuth) {
        return kHAPError_Unknown;
    }

    if (HAPPlatformMFiHWAuthIsPoweredOn(HAPNonnull(mfiHWAuth->platformMFiHWAuth))) {
        return kHAPError_None;
    }

    HAPLogInfo(&logObject, "Turning on Apple Authentication Coprocessor.");

    // Switch MFi on.
    err = HAPPlatformMFiHWAuthPowerOn(HAPNonnull(mfiHWAuth->platformMFiHWAuth));
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Schedule checking for power off.
    err = HAPPlatformTimerRegister(
            &mfiHWAuth->powerOffTimer, HAPPlatformClockGetCurrent() + 3 * HAPSecond, PowerOffTimerExpired, mfiHWAuth);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLog(&logObject, "Not enough resources to start power off timer. Leaving HW on!");
    }

    return kHAPError_None;
}

#define HAP_MFI_HW_AUTH_READ_OR_RETURN_FAIL_VALUE(mfiHWAuth, registerAddress, bytes, numBytes, failValue) \
    do { \
        err = HAPPlatformMFiHWAuthRead( \
                HAPNonnull((mfiHWAuth)->platformMFiHWAuth), (registerAddress), (bytes), (numBytes)); \
        if (err) { \
            HAPAssert(err == kHAPError_Unknown); \
            return (failValue); \
        } \
    } while (0)

#define HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(mfiHWAuth, registerAddress, bytes, numBytes) \
    HAP_MFI_HW_AUTH_READ_OR_RETURN_FAIL_VALUE(mfiHWAuth, registerAddress, bytes, numBytes, false)

#define HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(mfiHWAuth, registerAddress, bytes, numBytes) \
    HAP_MFI_HW_AUTH_READ_OR_RETURN_FAIL_VALUE(mfiHWAuth, registerAddress, bytes, numBytes, kHAPError_Unknown)

#define HAP_MFI_HW_AUTH_WRITE_OR_RETURN_FAIL_VALUE(mfiHWAuth, bytes, numBytes, failValue) \
    do { \
        err = HAPPlatformMFiHWAuthWrite(HAPNonnull((mfiHWAuth)->platformMFiHWAuth), (bytes), (numBytes)); \
        if (err) { \
            HAPAssert(err == kHAPError_Unknown); \
            return (failValue); \
        } \
    } while (0)

#define HAP_MFI_HW_AUTH_WRITE_OR_RETURN_FALSE(mfiHWAuth, bytes, numBytes) \
    HAP_MFI_HW_AUTH_WRITE_OR_RETURN_FAIL_VALUE(mfiHWAuth, bytes, numBytes, false)

#define HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(mfiHWAuth, bytes, numBytes) \
    HAP_MFI_HW_AUTH_WRITE_OR_RETURN_FAIL_VALUE(mfiHWAuth, bytes, numBytes, kHAPError_Unknown)

HAP_RESULT_USE_CHECK
bool HAPMFiHWAuthIsAvailable(HAPMFiHWAuth* mfiHWAuth) {
    HAPPrecondition(mfiHWAuth);

    HAPError err;

    // Enable Apple Authentication Coprocessor.
    err = HAPMFiHWAuthEnable(mfiHWAuth);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return false;
    }

    // Reset Error Code.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(mfiHWAuth, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
    }

    // Read Device Version.
    uint8_t deviceVersion;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(mfiHWAuth, kHAPMFiHWAuthRegister_DeviceVersion, bytes, sizeof bytes);
        deviceVersion = bytes[0];
    }

    // Read Authentication Revision.
    uint8_t authenticationRevision;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(
                mfiHWAuth, kHAPMFiHWAuthRegister_AuthenticationRevision, bytes, sizeof bytes);
        authenticationRevision = bytes[0];
    }

    // Read Authentication Protocol Version.
    uint8_t protocolVersionMajor;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(
                mfiHWAuth, kHAPMFiHWAuthRegister_AuthenticationProtocolMajorVersion, bytes, sizeof bytes);
        protocolVersionMajor = bytes[0];
    }
    uint8_t protocolVersionMinor;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(
                mfiHWAuth, kHAPMFiHWAuthRegister_AuthenticationProtocolMinorVersion, bytes, sizeof bytes);
        protocolVersionMinor = bytes[0];
    }

    // Check for error.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(mfiHWAuth, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
        HAPMFiHWAuthError errorCode = (HAPMFiHWAuthError) bytes[0];
        if (errorCode) {
            HAPLog(&logObject, "Error occurred while getting information: 0x%02x.", errorCode);
            return false;
        }
    }

    // Log coprocessor information.
    const char* deviceVersionString;
    switch (deviceVersion) {
        case kHAPMFiHWAuthDeviceVersion_2_0C: {
            deviceVersionString = "2.0C";
            break;
        }
        case kHAPMFiHWAuthDeviceVersion_3_0: {
            deviceVersionString = "3.0";
            break;
        }
        default: {
            deviceVersionString = "Unknown";
            break;
        }
    }
    HAPLog(&logObject,
           "Apple Authentication Coprocessor information:\n"
           "- Device Version: %s (0x%02x)\n"
           "- %s: %u\n"
           "- Authentication Protocol Version: %u.%u",
           deviceVersionString,
           deviceVersion,
           deviceVersion >= kHAPMFiHWAuthDeviceVersion_3_0 ? "Authentication Revision" : "Firmware Version",
           authenticationRevision,
           protocolVersionMajor,
           protocolVersionMinor);

    // Write self-test control status.
    if (protocolVersionMajor == 2) {
        uint8_t bytes[2];
        bytes[0] = kHAPMFiHWAuthRegister_SelfTestStatus;
        bytes[1] = 0x01; // Run X.509 certificate and private key tests.
        HAP_MFI_HW_AUTH_WRITE_OR_RETURN_FALSE(mfiHWAuth, bytes, sizeof bytes);
    }

    // Read self-test control status.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_FALSE(mfiHWAuth, kHAPMFiHWAuthRegister_SelfTestStatus, bytes, sizeof bytes);

        // Verify if bit 7 and 6 are set.
        // Bit 7: Certificate found in memory.
        // Bit 6: Private key found in memory.
        if (!(bytes[0] & 1U << 7U)) {
            HAPLog(&logObject, "Apple Authentication Coprocessor reports %s not found in memory.", "certificate");
            return false;
        }
        if (!(bytes[0] & 1U << 6U)) {
            HAPLog(&logObject, "Apple Authentication Coprocessor reports %s not found in memory.", "private key");
            return false;
        }
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPMFiHWAuthCopyCertificate(
        HAPAccessoryServer* server,
        void* certificateBytes,
        size_t maxCertificateBytes,
        size_t* numCertificateBytes) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.authentication.mfiHWAuth);
    HAPPrecondition(certificateBytes);
    HAPPrecondition(numCertificateBytes);

    HAPError err;

    // Enable Apple Authentication Coprocessor.
    err = HAPMFiHWAuthEnable(&server->mfi);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Reset Error Code.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(&server->mfi, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
    }

    // Read Authentication Protocol Version.
    uint8_t protocolVersionMajor;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi, kHAPMFiHWAuthRegister_AuthenticationProtocolMajorVersion, bytes, sizeof bytes);
        protocolVersionMajor = bytes[0];

        if (protocolVersionMajor != 2 && protocolVersionMajor != 3) {
            HAPLog(&logObject, "Unsupported Authentication Protocol Major Version: %u.", protocolVersionMajor);
            return kHAPError_Unknown;
        }
    }

    // Read accessory certificate data length.
    uint16_t accessoryCertificateDataLength;
    {
        uint8_t bytes[2];

        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi, kHAPMFiHWAuthRegister_AccessoryCertificateDataLength, bytes, sizeof bytes);
        accessoryCertificateDataLength = HAPReadBigUInt16(&bytes[0]);

        // See Accessory Interface Specification R32
        // Section 67.5.7.12 Accessory Certificate Data Length
        // See Accessory Interface Specification R29
        // Section 69.8.2.11 Accessory Certificate Data Length
        if ((protocolVersionMajor == 3 &&
             (accessoryCertificateDataLength < 607 || accessoryCertificateDataLength > 609)) ||
            (protocolVersionMajor == 2 && accessoryCertificateDataLength > 1280)) {
            HAPLog(&logObject,
                   "Apple Authentication Coprocessor returned %u for accessory certificate data length.",
                   accessoryCertificateDataLength);
            return kHAPError_Unknown;
        }
    }

    // Read accessory certificate data.
    if (accessoryCertificateDataLength > maxCertificateBytes) {
        HAPLog(&logObject, "Not enough space to get certificate.");
        return kHAPError_OutOfResources;
    }
    *numCertificateBytes = 0;
    for (uint8_t i = 0; accessoryCertificateDataLength; i++) {
        if (protocolVersionMajor == 3) {
            HAPAssert(i < 5);
        } else {
            HAPAssert(protocolVersionMajor == 2);
            HAPAssert(i < 10);
        }

        uint16_t numBytes = HAPMin(accessoryCertificateDataLength, (uint16_t) 128);
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi,
                (uint8_t)(kHAPMFiHWAuthRegister_AccessoryCertificateData1 + i),
                &((uint8_t*) certificateBytes)[*numCertificateBytes],
                numBytes);
        accessoryCertificateDataLength -= numBytes;
        *numCertificateBytes += numBytes;
    }
    HAPAssert(*numCertificateBytes <= maxCertificateBytes);

    // Check for error.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(&server->mfi, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
        HAPMFiHWAuthError errorCode = (HAPMFiHWAuthError) bytes[0];
        if (errorCode) {
            HAPLog(&logObject, "Error occurred while getting accessory certificate: 0x%02x.", errorCode);
            return kHAPError_Unknown;
        }
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPMFiHWAuthCreateSignature(
        HAPAccessoryServer* server,
        const void* challengeBytes,
        size_t numChallengeBytes,
        void* signatureBytes,
        size_t maxSignatureBytes,
        size_t* numSignatureBytes) {
    HAPPrecondition(server);
    HAPPrecondition(server->platform.authentication.mfiHWAuth);
    HAPPrecondition(challengeBytes);
    HAPPrecondition(signatureBytes);
    HAPPrecondition(numSignatureBytes);

    HAPError err;

    // Enable Apple Authentication Coprocessor.
    err = HAPMFiHWAuthEnable(&server->mfi);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Reset Error Code.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(&server->mfi, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
    }

    // Read Authentication Protocol Version.
    uint8_t protocolVersionMajor;
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi, kHAPMFiHWAuthRegister_AuthenticationProtocolMajorVersion, bytes, sizeof bytes);
        protocolVersionMajor = bytes[0];

        if (protocolVersionMajor != 2 && protocolVersionMajor != 3) {
            HAPLog(&logObject, "Unsupported Authentication Protocol Major Version: %u.", protocolVersionMajor);
            return kHAPError_Unknown;
        }
    }

    // Write challenge.
    if (protocolVersionMajor == 3) {
        // Write challenge data.
        // Additional SHA256 hash computation is necessary.
        // Apple Authentication Coprocessor will compute ECDSA signature.
        // See HomeKit Accessory Protocol Specification R17
        // Section 5.7.4 M4: Accessory -> iOS Device - `SRP Verify Response'
        {
            uint8_t bytes[1 + SHA256_BYTES];
            bytes[0] = kHAPMFiHWAuthRegister_ChallengeData;
            HAP_sha256(&bytes[1], challengeBytes, numChallengeBytes);
            HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(&server->mfi, bytes, sizeof bytes);
        }
    } else {
        HAPAssert(protocolVersionMajor == 2);

        // Write challenge data length.
        {
            uint8_t bytes[1 + sizeof(uint16_t)];
            bytes[0] = kHAPMFiHWAuthRegister_ChallengeDataLength;
            HAPWriteBigUInt16(&bytes[1], (uint16_t) SHA1_BYTES);
            HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(&server->mfi, bytes, sizeof bytes);
        }

        // Write challenge data.
        // Additional SHA1 hash computation is necessary.
        // Apple Authentication Coprocessor will compute RSA signature.
        // See HomeKit Accessory Protocol Specification R13
        // Section 5.6.4 M4: Accessory -> iOS Device - `SRP Verify Response'
        {
            uint8_t bytes[1 + SHA1_BYTES];
            bytes[0] = kHAPMFiHWAuthRegister_ChallengeData;
            HAP_sha1(&bytes[1], challengeBytes, numChallengeBytes);
            HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(&server->mfi, bytes, sizeof bytes);
        }

        // Write challenge response data length.
        // Before a challenge response-generation process begins, this register should contain 0x80.
        // See Accessory Interface Specification R29
        // Section 69.8.2.7 Challenge Response Data Length
        {
            uint8_t bytes[1 + sizeof(uint16_t)];
            bytes[0] = kHAPMFiHWAuthRegister_ChallengeResponseDataLength;
            HAPWriteBigUInt16(&bytes[1], 0x80U);
            HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(&server->mfi, bytes, sizeof bytes);
        }
    }

    // Write authentication control.
    {
        uint8_t bytes[2];
        bytes[0] = kHAPMFiHWAuthRegister_AuthenticationControlAndStatus;
        bytes[1] = 1; // PROC_CONTROL
        HAP_MFI_HW_AUTH_WRITE_OR_RETURN_ERROR(&server->mfi, bytes, sizeof bytes);
    }

    // Read status.
    // The proc results are stored in bits 6|5|4
    // The bits 3, 2, 1 and 0 are 0.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi, kHAPMFiHWAuthRegister_AuthenticationControlAndStatus, bytes, sizeof bytes);
        if (bytes[0] != (1U << 4U)) {
            HAPLog(&logObject,
                   "Apple Authentication Coprocessor returned %02x for authentication protocol status.",
                   bytes[0]);
            return kHAPError_Unknown;
        }
    }

    // Read challenge response data length.
    uint16_t challengeResponseDataLength;
    {
        uint8_t bytes[2];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
                &server->mfi, kHAPMFiHWAuthRegister_ChallengeResponseDataLength, bytes, sizeof bytes);
        challengeResponseDataLength = HAPReadBigUInt16(&bytes[0]);

        // See Accessory Interface Specification R32
        // Section 67.5.7.8 Challenge Response Data Length
        // See Accessory Interface Specification R29
        // Section 69.8.2.7 Challenge Response Data Length
        if ((protocolVersionMajor == 3 && challengeResponseDataLength != 64) ||
            (protocolVersionMajor == 2 && challengeResponseDataLength > 0x80)) {
            HAPLog(&logObject,
                   "Apple Authentication Coprocessor returned %u for challenge response data length.",
                   challengeResponseDataLength);
            return kHAPError_Unknown;
        }
    }

    // Read challenge response data.
    if (challengeResponseDataLength > maxSignatureBytes) {
        HAPLog(&logObject, "Not enough space to get signature.");
        return kHAPError_OutOfResources;
    }
    HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(
            &server->mfi, kHAPMFiHWAuthRegister_ChallengeResponseData, signatureBytes, challengeResponseDataLength);
    *numSignatureBytes = challengeResponseDataLength;

    // Check for error.
    {
        uint8_t bytes[1];
        HAP_MFI_HW_AUTH_READ_OR_RETURN_ERROR(&server->mfi, kHAPMFiHWAuthRegister_ErrorCode, bytes, sizeof bytes);
        HAPMFiHWAuthError errorCode = (HAPMFiHWAuthError) bytes[0];
        if (errorCode) {
            HAPLog(&logObject, "Error occurred while getting signature: 0x%02x.", errorCode);
            return kHAPError_Unknown;
        }
    }
    return kHAPError_None;
}
