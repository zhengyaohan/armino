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

#include "HAPAccessorySetup.h"
#include "HAPAccessory.h"
#include "HAPBase36Util.h"
#include "HAPCrypto.h"
#include "HAPDeviceID.h"

HAP_RESULT_USE_CHECK
bool HAPAccessorySetupIsValidSetupCode(const char* stringValue) {
    HAPPrecondition(stringValue);

    if (HAPStringGetNumBytes(stringValue) != sizeof(HAPSetupCode) - 1) {
        return false;
    }

    uint8_t numEqual = 0;
    uint8_t numAscending = 0;
    uint8_t numDescending = 0;

    char previousCharacter = '\0';
    for (size_t i = 0; i < sizeof(HAPSetupCode) - 1; i++) {
        if (i == 3 || i == 6) {
            if (stringValue[i] != '-') {
                return false;
            }
        } else {
            if (!HAPASCIICharacterIsNumber(stringValue[i])) {
                return false;
            }
            numEqual += stringValue[i] == previousCharacter;
            numAscending += stringValue[i] == previousCharacter + 1;
            numDescending += stringValue[i] == previousCharacter - 1;
            previousCharacter = stringValue[i];
        }
    }

    // All equal, ascending, or descending?
    return numEqual != 7 && ((uint8_t)((uint8_t) stringValue[0] ^ (uint8_t) '1') | (uint8_t)(numAscending ^ 7U)) &&
           ((uint8_t)((uint8_t) stringValue[0] ^ (uint8_t) '8') | (uint8_t)(numDescending ^ 7U));
}

void HAPAccessorySetupGenerateRandomSetupCode(HAPSetupCode* setupCode) {
    HAPPrecondition(setupCode);

    do {
        // Format: XXX-XX-XXX with X being digit from 0-9.
        for (size_t i = 0; i < sizeof setupCode->stringValue - 1; i++) {
            if (i == 3 || i == 6) {
                setupCode->stringValue[i] = '-';
                continue;
            }

            // Add random digit.
            uint8_t randomByte;
            do {
                HAPPlatformRandomNumberFill(&randomByte, sizeof randomByte);
            } while ((uint8_t)(randomByte & 0xFU) > 9);
            setupCode->stringValue[i] = (char) ('0' + (char) (randomByte & 0xFU));
        }
        setupCode->stringValue[sizeof setupCode->stringValue - 1] = '\0';
    } while (!HAPAccessorySetupIsValidSetupCode(setupCode->stringValue));
}

HAP_RESULT_USE_CHECK
bool HAPAccessorySetupIsValidSetupID(const char* stringValue) {
    HAPPrecondition(stringValue);

    if (HAPStringGetNumBytes(stringValue) != sizeof(HAPSetupID) - 1) {
        return false;
    }

    for (size_t i = 0; i < sizeof(HAPSetupID) - 1; i++) {
        char c = stringValue[i];
        if (!HAPASCIICharacterIsUppercaseAlphanumeric(c)) {
            return false;
        }
    }

    return true;
}

void HAPAccessorySetupGenerateRandomSetupID(HAPSetupID* setupID) {
    HAPPrecondition(setupID);

    for (size_t i = 0; i < sizeof setupID->stringValue - 1; i++) {
        char c;
        do {
            HAPPlatformRandomNumberFill(&c, sizeof c);
        } while (!HAPASCIICharacterIsUppercaseAlphanumeric(c));
        setupID->stringValue[i] = c;
    }
    setupID->stringValue[sizeof setupID->stringValue - 1] = '\0';
}

void HAPAccessorySetupGenerateJoinerPassphrase(
        HAPJoinerPassphrase* passphrase,
        const HAPSetupCode* _Nullable setupCode) {
    HAPPrecondition(setupCode);
    HAPPrecondition(passphrase);

    HAPRawBufferZero(passphrase->stringValue, sizeof(passphrase->stringValue));

    // Generate Passphrase by running Sha256 over setup code and convert to hex string
    uint8_t md[SHA256_BYTES];
    HAPRawBufferZero(md, sizeof(md));
    uint8_t* stringData = (uint8_t*) setupCode->stringValue;
    HAP_sha256(md, stringData, HAPStringGetNumBytes(setupCode->stringValue));

    HAPError err = HAPStringWithFormat(
            passphrase->stringValue,
            sizeof passphrase->stringValue,
            "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            md[0],
            md[1],
            md[2],
            md[3],
            md[4],
            md[5],
            md[6],
            md[7],
            md[8],
            md[9],
            md[10],
            md[11],
            md[12],
            md[13],
            md[14],
            md[15]);
    HAPAssert(!err);
}

// Version 0 of the Setup Payload (No longer produced, but backwards compatibility must be maintained)
//
// UL Prefix:       7 ASCII Characters
// Payload Header:  9 Base 36 encoded characters representing 46 bits of information
// Setup ID:        4 ASCII Characters
//
//  -----------------------------------------------------------------------------------------
//  |UL Prefix| Ver | Reserved | Category | WAC | BLE | IP | Paired | setup code | Setup ID |
//  -----------------------------------------------------------------------------------------
//  | string  |  3  |   4 bits |   8 bits |  1  |  1  | 1  | 1      |   27 bits  |  string  |
//  |         |bits |          |          | bit | bit | bit| bit    |            |          |
//  .........................................................................................
//  | 7 chars |        46 bits  Encoded As 9 Base 36 encoded characters          |  4 chars |
//  -----------------------------------------------------------------------------------------
//
// Version 1 adds the following:
//
// Optional EUI             64 bits
// Required Product Code    32 bits
//
// If an EUI is included the Setup ID is removed.  Legacy decoders can interpret the 4 characters that follow
// the setup code as a setup ID.  To ensure these characters have as much entropy as possible the EUI will
// be encoded in reverse byte order.
//
// Version 0.1 of the Setup Payload
//
// With EUI
//   - UL Prefix:       7 ASCII Characters
//   - Payload Header:  9 Base 36 encoded characters representing 46 bits of information
//   - Suffix Field:   22 Base 36 encoded characters representing 112 bits of information
//  ------------------------------------------------------------------------------------------------------------
//  |UL Prefix|Ver |Res |Prod Num|Thread|Category|WAC|BLE| IP|Paired|setup code| EUI Rev | Prod Num | Reserved |
//  ------------------------------------------------------------------------------------------------------------
//  | string  |  3 |  2 |  1 bit |   1  | 8 bits | 1 | 1 | 1 |  1   | 27 bits  | 64 bits |  32 bits | 16 bits  |
//  |         |bits|bits|        |  bit |        |bit|bit|bit| bit  |          |         |          |          |
//  ...........................................................................................................|
//  | 7 chars |       46 bits Encoded As 9 Base 36 encoded characters          |112 bits encoded as 22 Base 36 |
//  |         |                                                                |         characters            |
//  ------------------------------------------------------------------------------------------------------------
//
// With Setup ID
//   - UL Prefix:       7 ASCII Characters
//   - Payload Header:  9 Base 36 encoded characters representing 46 bits of information
//   - Setup ID:        4 ASCII Characters
//   - Suffix Field:   18 Base 36 encoded characters representing 88 bits of information
//  ------------------------------------------------------------------------------------------------------------
//  |UL Prefix|Ver |Res |Prod Num|Thread|Category|WAC|BLE| IP|Paired|setup code| Setup ID| Prod Num | Reserved |
//  ------------------------------------------------------------------------------------------------------------
//  | string  |  3 |  2 |  1 bit |   1  | 8 bits | 1 | 1 | 1 | 1    | 27 bits  | string  |  32 bits | 56 bits  |
//  |         |bits|bits|        | bit  |        |bit|bit|bit| bit  |          |         |          |          |
//  ...........................................................................................................|
//  | 7 chars |       46 bits Encoded As 9 Base 36 encoded characters          | 4 Chars | 88 bits encoded as  |
//  |         |                                                                |         | 18 Base 36 chars    |
//  ------------------------------------------------------------------------------------------------------------

/** Current Setup Payload Version   */
#define SETUP_PAYLOAD_VER 0
// Note: To maintain backwards compatibility Payload Ver must be set to 0.
// Controllers will need to infer 0 vs 0.1 based on payload length (20 vs 38)

/** Invalid Setup ID */
#define SETUP_PAYLOAD_SETUP_ID_INVALID "0000"

/** Reserved */
#define SETUP_PAYLOAD_RESERVED 0

/** Prefix of the setup payload. */
#define HAPSetupPayloadPrefix ("X-HM://")

/** The number of b36 chars consumed by the payload header */
#define PAYLOAD_HEADER_CHARS 9

/** Size in bytes of each type of Suffix:   */
#define EUI_SUFFIX_BYTES      14
#define SETUP_ID_SUFFIX_BYTES 11

void HAPAccessorySetupGetSetupPayload(
        HAPSetupPayload* setupPayload,
        const HAPSetupCode* _Nullable setupCode,
        const HAPSetupID* _Nullable setupID,
        const HAPEui64* _Nullable eui,
        const HAPAccessoryProductData* productData,
        HAPAccessorySetupSetupPayloadFlags flags,
        HAPAccessoryCategory category) {
    HAPPrecondition(setupPayload);
    HAPPrecondition(productData);
    HAPPrecondition(!setupCode || !flags.isPaired);
    HAPPrecondition(!setupID || !flags.isPaired);
    HAPPrecondition((setupCode && setupID) || (eui && setupCode) || (!setupCode && !setupID));
    HAPPrecondition((eui && !setupID) || (!eui && setupID) || (!eui && !setupID));
    HAPPrecondition(!flags.wacSupported || flags.ipSupported || flags.threadSupported);
    HAPPrecondition(category > 0);

    /** Product Data is 64 bits wide.  Top 32 = Product Group, bottom 32 = Product Number.
     * QR Code generates product info with the 32 bit Product Number */
    uint32_t productInfo = HAPReadBigUInt32(&productData->bytes[4]);

    // The minimum that will always be added.  Further B36 additions are appended provided there's room.
    HAPPrecondition(
            sizeof(setupPayload->stringValue) >=
            sizeof(HAPSetupPayloadPrefix) + PAYLOAD_HEADER_CHARS + sizeof(HAPSetupID));

    // Zero out the string
    HAPRawBufferZero(setupPayload->stringValue, sizeof setupPayload->stringValue);
    char* pl = setupPayload->stringValue;

    // Add the X-HM: Prefix.
    HAPRawBufferCopyBytes(pl, HAPSetupPayloadPrefix, sizeof HAPSetupPayloadPrefix - 1);
    pl += sizeof HAPSetupPayloadPrefix - 1;

    // Raw VersionCategoryFlagsAndSetupCode.
    uint64_t code = (uint64_t)(
            /* 45-43 - Version     */ ((uint64_t) SETUP_PAYLOAD_VER << 43U) |
            /* 42-41 - Reserved    */ ((uint64_t) SETUP_PAYLOAD_RESERVED << 41U) |
            /*    40 - Product Num */ ((uint64_t) 0x1U << 40U) |
            /*    39 - Thread      */ ((uint64_t)(flags.threadSupported ? 1U : 0U) << 39U) |
            /* 38-31 - Category    */ ((uint64_t)(category & 0xFFU) << 31U) |
            /*    30 - WAC         */ ((uint64_t)(flags.wacSupported ? 1U : 0U) << 30U) |
            /*    29 - BLE         */ ((uint64_t)(flags.bleSupported ? 1U : 0U) << 29U) |
            /*    28 - IP          */ ((uint64_t)(flags.ipSupported ? 1U : 0U) << 28U) |
            /*    27 - Paired      */ ((uint64_t)(flags.isPaired ? 1U : 0U) << 27U));

    if (setupCode) {
        code |= /* 26-00 - Setup code */ (uint64_t)(
                (uint64_t)(setupCode->stringValue[0] - '0') * 10000000U +
                (uint64_t)(setupCode->stringValue[1] - '0') * 1000000U +
                (uint64_t)(setupCode->stringValue[2] - '0') * 100000U +
                (uint64_t)(setupCode->stringValue[4] - '0') * 10000U +
                (uint64_t)(setupCode->stringValue[5] - '0') * 1000U +
                (uint64_t)(setupCode->stringValue[7] - '0') * 100U + (uint64_t)(setupCode->stringValue[8] - '0') * 10U +
                (uint64_t)(setupCode->stringValue[9] - '0') * 1U);
    }

    HAPBase36Encode64(pl, PAYLOAD_HEADER_CHARS + 1, code);
    pl += PAYLOAD_HEADER_CHARS; // We should be pointing to the null terminated character at the end of the encoded
                                // string

    int payloadBytes;
    if (flags.threadSupported) {
        payloadBytes = EUI_SUFFIX_BYTES - 1;
    } else {
        payloadBytes = SETUP_ID_SUFFIX_BYTES - 1;
    }

    // Setup ID
    if (setupID) {
        HAPRawBufferCopyBytes(pl, setupID->stringValue, sizeof setupID->stringValue - 1);
        pl += sizeof setupID->stringValue - 1;
    } else if (!flags.threadSupported) {
        HAPRawBufferCopyBytes(pl, SETUP_PAYLOAD_SETUP_ID_INVALID, sizeof(HAPSetupID) - 1);
        pl += sizeof(HAPSetupID) - 1;
    }

    uint128Struct_t payloadSuffix;
    HAPRawBufferZero(&payloadSuffix, sizeof(payloadSuffix));

    // EUI
    if (eui) {
        // Note: The EUI is stored in reverse byte order.  This is because legacy decoders will
        //    parse the first 4 characters of the suffix as the setupID.  The least significant
        //    bytes of the EUI have higher entropy than the most significant.  Storing them in
        //    the high bytes of the encoded field will increase the randomness of the setupID.
        for (int i = sizeof(HAPEui64) - 1; i >= 0 && payloadBytes >= 0; i--) {
            int suffixIndex = payloadBytes / 4;
            int bytePos = payloadBytes % 4;
            payloadSuffix.m32[suffixIndex] |= (uint32_t) eui->bytes[i] << (bytePos * 8);
            payloadBytes--;
        }
    } else if (flags.threadSupported) {
        for (int i = sizeof(HAPEui64) - 1; i >= 0 && payloadBytes >= 0; i--) {
            int suffixIndex = payloadBytes / 4;
            payloadSuffix.m32[suffixIndex] |= 0;
            payloadBytes--;
        }
    }

    // Product Number
    uint8_t* prodInfoPtr = (uint8_t*) &productInfo;
    for (int i = sizeof(productInfo) - 1; i >= 0 && payloadBytes >= 0; i--) {
        int suffixIndex = payloadBytes / 4;
        int bytePos = payloadBytes % 4;
        payloadSuffix.m32[suffixIndex] |= (uint32_t) prodInfoPtr[i] << (bytePos * 8);
        payloadBytes--;
    }

    unsigned long charsRemaining = (long) sizeof(setupPayload->stringValue) - (pl - setupPayload->stringValue);
    HAPBase36Encode128(pl, charsRemaining, &payloadSuffix);
    pl += (charsRemaining - 1); // We should be on the null terminated 0 after the encoded string

    // Done.
    HAPAssert(!*pl);
    HAPAssert(pl - setupPayload->stringValue <= (long) sizeof setupPayload->stringValue);
}

void HAPAccessorySetupGetSetupHash(
        HAPAccessorySetupSetupHash* setupHash,
        const HAPSetupID* setupID,
        const HAPDeviceIDString* deviceIDString) {
    HAPPrecondition(setupHash);
    HAPPrecondition(setupID);
    HAPPrecondition(deviceIDString);

    // Concatenate setup ID and Device ID.
    uint8_t hash[SHA512_BYTES];
    HAPAssert(sizeof setupID->stringValue - 1 + sizeof deviceIDString->stringValue - 1 <= sizeof hash);
    size_t o = 0;
    HAPRawBufferCopyBytes(&hash[o], setupID->stringValue, sizeof setupID->stringValue - 1);
    o += sizeof setupID->stringValue - 1;
    HAPRawBufferCopyBytes(&hash[o], deviceIDString->stringValue, sizeof deviceIDString->stringValue - 1);
    o += sizeof deviceIDString->stringValue - 1;

    // SHA512.
    HAP_sha512(hash, hash, o);

    // Truncate.
    HAPAssert(sizeof setupHash->bytes <= sizeof hash);
    HAPRawBufferCopyBytes(setupHash->bytes, hash, sizeof setupHash->bytes);
}
