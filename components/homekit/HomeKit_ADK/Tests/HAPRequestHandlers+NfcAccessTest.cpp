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
// Copyright (C) 2021 Apple Inc. All Rights Reserved.
#include "HAP.h"
#include "HAPPlatform+Init.h"

#include "HAP+KeyValueStoreDomains.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

#if (HAVE_NFC_ACCESS == 1)
#include "HAPPlatformNfcAccess.h"

#define kNfcAccessKeyStoreDomain ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * NFC Access write response TLV storage buffer size in bytes
 *   - The following multiplied by max number of suspended device credential keys supported:
 *     - Number of bytes for Identifier field
 *     - Number of bytes for Issuer Key Identifier field
 *     - Number of bytes for Status Code field
 *     - 2 bytes for type/length of Device Credential Key Response
 *     - 6 bytes for type/length of each field in Device Credential Key Response
 *   - 38 bytes for max separators 0000 between each Device Credential Key Response
 */
#define kAppNfcAccessResponseBufferBytes (20 * (8 + 8 + sizeof(uint8_t) + 2 + 6) + 38)

static HAPAccessoryServer accessoryServer;
static HAPAccessoryServerCallbacks accessoryServerCallbacks;
static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
static IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
static HAPIPReadContext ipReadContexts[kAttributeCount];
static HAPIPWriteContext ipWriteContexts[kAttributeCount];
static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
static HAPIPAccessoryServerStorage ipAccessoryServerStorage;
static HAPSession sessions[3];

static uint8_t nfcAccessResponseBuffer[kAppNfcAccessResponseBufferBytes];
static HAPNfcAccessResponseStorage nfcAccessResponseStorage = { .bytes = nfcAccessResponseBuffer,
                                                                .maxBytes = sizeof nfcAccessResponseBuffer };

/**
 * Fake keys for test cases
 */
static const uint8_t ISSUER_KEY_1[NFC_ACCESS_ISSUER_KEY_BYTES] = { 0xE9, 0x6C, 0x73, 0xFB, 0x4A, 0xC1, 0xDF, 0x7F,
                                                                   0x29, 0xD6, 0x36, 0x0D, 0x66, 0x7B, 0x1E, 0x37,
                                                                   0x6D, 0xF2, 0x16, 0x2C, 0x34, 0xA4, 0x43, 0x19,
                                                                   0x19, 0xAF, 0x6D, 0xFE, 0xC8, 0xF2, 0x5A, 0x3A };
static const uint8_t ISSUER_KEY_IDENTIFIER_1[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0xE2, 0xC4, 0x5B, 0x64,
                                                                                  0x83, 0xE9, 0xC8, 0xFD };
static const uint8_t ISSUER_KEY_2[NFC_ACCESS_ISSUER_KEY_BYTES] = { 0xE9, 0x2F, 0xCC, 0xB4, 0x8F, 0x69, 0xA4, 0x1F,
                                                                   0x7D, 0xEE, 0x37, 0x4F, 0x29, 0xE7, 0x4D, 0x1D,
                                                                   0xF0, 0xAD, 0x4C, 0x59, 0x35, 0x48, 0x4D, 0x03,
                                                                   0x8C, 0xF9, 0x77, 0x57, 0x32, 0x4B, 0x11, 0xAC };
static const uint8_t ISSUER_KEY_IDENTIFIER_2[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0x98, 0x5C, 0x6C, 0x62,
                                                                                  0x1A, 0x70, 0x6F, 0x8D };
static const uint8_t DEVICE_CREDENTIAL_KEY_1[NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES] = {
    0xA3, 0x1E, 0x01, 0xCE, 0xE1, 0x5A, 0x4B, 0x71, 0xF6, 0xB9, 0xEC, 0x55, 0x22, 0xB3, 0x4C, 0x0D,
    0x96, 0x08, 0xBF, 0x13, 0xC1, 0x76, 0xAD, 0x12, 0x0E, 0x8C, 0x08, 0x2A, 0xEB, 0xDF, 0x3A, 0xDB,
    0xCF, 0x4E, 0x1D, 0x66, 0x2D, 0x3F, 0x30, 0x28, 0x6E, 0x84, 0x53, 0x49, 0xB7, 0xAB, 0x06, 0xC2,
    0xFB, 0x50, 0x7D, 0x19, 0x81, 0x86, 0xBC, 0xA4, 0x8B, 0xA8, 0x75, 0x20, 0x14, 0xF9, 0x77, 0x0C
};
static const uint8_t DEVICE_CREDENTIAL_KEY_IDENTIFIER_1[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0x9D, 0x32, 0x4A, 0xE7,
                                                                                             0x5E, 0x2D, 0xAE, 0x2F };
static const uint8_t DEVICE_CREDENTIAL_KEY_2[NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES] = {
    0xEF, 0x17, 0xDC, 0xD0, 0x41, 0xB5, 0x09, 0x71, 0x75, 0xA8, 0x01, 0x00, 0x7E, 0xAC, 0xEC, 0xFD,
    0xE1, 0xFB, 0xF8, 0x51, 0x64, 0x6F, 0x13, 0x57, 0x59, 0x9D, 0x62, 0x5D, 0xA6, 0xE9, 0x1C, 0xAF,
    0x12, 0x26, 0xFD, 0xD1, 0xB1, 0xD8, 0x24, 0x99, 0x56, 0x8D, 0xD5, 0xBA, 0x91, 0x5D, 0xEB, 0x87,
    0xC1, 0x83, 0x76, 0x52, 0x7F, 0x31, 0x46, 0xD7, 0x45, 0xA1, 0x15, 0xD4, 0x31, 0xC3, 0x13, 0x5F
};
static const uint8_t DEVICE_CREDENTIAL_KEY_IDENTIFIER_2[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0xE7, 0x58, 0x04, 0x54,
                                                                                             0xD2, 0x72, 0xD9, 0xA9 };
static const uint8_t DEVICE_CREDENTIAL_KEY_3[NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES] = {
    0x97, 0xC5, 0x1E, 0x2F, 0x7F, 0xA1, 0x3E, 0x42, 0xFA, 0x31, 0xFF, 0x67, 0x06, 0x91, 0xC0, 0x29,
    0x22, 0xD9, 0x77, 0x1C, 0x8F, 0xE4, 0x32, 0xFB, 0xA8, 0x44, 0x23, 0x42, 0x57, 0x8F, 0xF2, 0x82,
    0xA7, 0x6D, 0x57, 0x57, 0x2A, 0x0B, 0x6B, 0x22, 0x40, 0x86, 0x08, 0xDE, 0xA8, 0xB0, 0x6E, 0xE9,
    0xF9, 0xA2, 0xDD, 0x0F, 0xEB, 0xF0, 0x4B, 0x8F, 0xB4, 0x2A, 0xC9, 0x6C, 0xEE, 0x22, 0x79, 0x2B
};
static const uint8_t DEVICE_CREDENTIAL_KEY_IDENTIFIER_3[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0x9B, 0x98, 0x6F, 0x18,
                                                                                             0x98, 0x60, 0x0D, 0xA9 };
static const uint8_t READER_KEY_1[NFC_ACCESS_READER_KEY_BYTES] = { 0x3A, 0x37, 0xCA, 0x76, 0x87, 0x0C, 0x3F, 0x68,
                                                                   0xFD, 0x3F, 0x0F, 0x41, 0xE5, 0x52, 0x8A, 0x2F,
                                                                   0x8A, 0xBD, 0x85, 0x4F, 0x75, 0x64, 0xA7, 0x49,
                                                                   0xC8, 0xF6, 0x10, 0x5D, 0x99, 0x7D, 0x63, 0xD4 };
static const uint8_t READER_KEY_IDENTIFIER_1[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0x95, 0xE7, 0x45, 0x95,
                                                                                  0x95, 0x6B, 0x44, 0xBB };
static const uint8_t READER_KEY_2[NFC_ACCESS_READER_KEY_BYTES] = { 0xFB, 0x8F, 0xE7, 0xBF, 0xBC, 0xE0, 0x5F, 0x4C,
                                                                   0x10, 0x10, 0xB6, 0x8F, 0x1D, 0x2A, 0xDD, 0xFE,
                                                                   0x7B, 0xF5, 0x96, 0x83, 0x06, 0x5B, 0xF2, 0x79,
                                                                   0x80, 0xA0, 0xF6, 0x64, 0x61, 0xBF, 0x10, 0x15 };
static const uint8_t READER_KEY_IDENTIFIER_2[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0xBE, 0x41, 0xAD, 0x55,
                                                                                  0xE5, 0x9B, 0x5B, 0xB3 };
static const uint8_t READER_IDENTIFIER[NFC_ACCESS_KEY_IDENTIFIER_BYTES] = { 0xFE, 0xEE, 0x97, 0x9F,
                                                                            0x1B, 0xD2, 0x5E, 0xA5 };

HAP_RESULT_USE_CHECK
static HAPError IdentifyAccessory(
        HAPAccessoryServer* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPFatalError();
}

static void HandleUpdatedAccessoryServerState(HAPAccessoryServer* server, void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(server);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory server state: %s.", "Idle");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory server state: %s.", "Running");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory server state: %s.", "Stopping");
            return;
        }
        default:
            HAPFatalError();
    }
}

static const HAPAccessory accessory = {
    .aid = 1,
    .category = kHAPAccessoryCategory_Locks,
    .name = "Acme Test",
    .productData = "03d8a775e3644573",
    .manufacturer = "Acme",
    .model = "Test1,1",
    .serialNumber = "099DB48E9E28",
    .firmwareVersion = "1",
    .hardwareVersion = "1",
    .services =
            (const HAPService* const[]) {
                    &accessoryInformationService,
                    &hapProtocolInformationService,
                    &pairingService,
                    &nfcAccessService,
                    NULL,
            },
    .callbacks = { .identify = IdentifyAccessory },
};

/**
 * Data structure for NFC access issuer key request
 * - The *IsSet flags are used to determine if the corresponding TLV type is set in the request
 */
typedef struct {
    /** Type of the issuer key */
    uint8_t type;
    bool typeIsSet;

    /**
     * Public key of the asymmetric key pair
     */
    const uint8_t* key;
    bool keyIsSet;

    /** The identifier that uniquely identifies the issuer key */
    const uint8_t* identifier;
    bool identifierIsSet;
} NfcAccessIssuerKeyRequest;

/**
 * Data structure for NFC access issuer key response
 * - The *IsExpected flags are used to determine if the corresponding TLV type is expected in the response
 */
typedef struct {
    /** The identifier that uniquely identifies the issuer key */
    const uint8_t* identifier;
    bool identifierIsExpected;

    /** The status code */
    uint8_t status;
    bool statusIsExpected;
} NfcAccessIssuerKeyResponse;

typedef struct {
    /** Contains data for an Issuer Key write request on NFC Access Control Point */
    NfcAccessIssuerKeyRequest request;
    /** Contains data to verify an Issuer Key response to NFC Access Control Point */
    NfcAccessIssuerKeyResponse response;
} NfcAccessIssuerKeyEntry;

/**
 * Data structure for NFC access device credential key request
 * - The *IsSet flags are used to determine if the corresponding TLV type is set in the request
 */
typedef struct {
    /** Type of the device credential key */
    uint8_t type;
    bool typeIsSet;

    /**
     * Public key of the asymmetric key pair
     */
    const uint8_t* key;
    bool keyIsSet;

    /** The issuer key identifier */
    const uint8_t* issuerKeyIdentifier;
    bool issuerKeyIdentifierIsSet;

    /** Key state (active/suspended) */
    uint8_t state;
    bool stateIsSet;

    /** The identifier that uniquely identifies the device credential key */
    const uint8_t* identifier;
    bool identifierIsSet;
} NfcAccessDeviceCredentialKeyRequest;

/**
 * Data structure for NFC access device credential key response
 * - The *IsExpected flags are used to determine if the corresponding TLV type is expected in the response
 * - The *IsChecked flags are used to determine if the value of the corresponding TLV type should be verified if the TLV
 * type is expected and exists in the response
 */
typedef struct {
    /** The identifier that uniquely identifies the device credential key */
    const uint8_t* identifier;
    bool identifierIsExpected;
    bool identifierIsChecked;

    /** The issuer key identifier */
    const uint8_t* issuerKeyIdentifier;
    bool issuerKeyIdentifierIsExpected;

    /** The status code */
    uint8_t status;
    bool statusIsExpected;
} NfcAccessDeviceCredentialKeyResponse;

typedef struct {
    /** Contains data for a Device Credential Key write request on NFC Access Control Point */
    NfcAccessDeviceCredentialKeyRequest request;
    /** Contains data to verify a Device Credential Key response to NFC Access Control Point */
    NfcAccessDeviceCredentialKeyResponse response;
} NfcAccessDeviceCredentialKeyEntry;

/**
 * Data structure for NFC access reader key request
 * - The *IsSet flags are used to determine if the corresponding TLV type is set in the request
 */
typedef struct {
    /** Type of the reader key */
    uint8_t type;
    bool typeIsSet;

    /**
     * Private key of the asymmetric key pair
     */
    const uint8_t* key;
    bool keyIsSet;

    /** The identifier that uniquely identifies a reader */
    const uint8_t* readerIdentifier;
    bool readerIdentifierIsExpected;

    /** The identifier that uniquely identifies the reader key */
    const uint8_t* identifier;
    bool identifierIsSet;
} NfcAccessReaderKeyRequest;

/**
 * Data structure for NFC access reader key response
 * - The *IsExpected flags are used to determine if the corresponding TLV type is expected in the response
 */
typedef struct {
    /** The identifier that uniquely identifies the reader key */
    const uint8_t* identifier;
    bool identifierIsExpected;

    /** The status code */
    uint8_t status;
    bool statusIsExpected;
} NfcAccessReaderKeyResponse;

typedef struct {
    /** Contains data for a Reader Key write request on NFC Access Control Point */
    NfcAccessReaderKeyRequest request;
    /** Contains data to verify a Reader Key response to NFC Access Control Point */
    NfcAccessReaderKeyResponse response;
} NfcAccessReaderKeyEntry;

/**
 * Setup the request and expected response for a list operation for NFC access issuer key
 *
 * @param[out]   entry        NFC access issuer key entry that contains the request and expected response
 * @param        identifier   expected identifier for the operation
 * @param        status       expected status for the operation
 */
static void
        SetupListOperationIssuerKeyEntry(NfcAccessIssuerKeyEntry* entry, const uint8_t* identifier, uint8_t status) {
    HAPPrecondition(entry);

    // For list request, no fields are populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.identifierIsSet = false;

    // For list response, all fields are expected
    entry->response.identifier = identifier;
    entry->response.identifierIsExpected = true;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for an add operation for NFC access issuer key
 *
 * @param[out]   entry   NFC access issuer key entry that contains the request and expected response
 * @param        type    key type to add for the request
 * @param        key     key to add for the request
 * @param        status  expected status for the operation
 */
static void SetupAddOperationIssuerKeyEntry(
        NfcAccessIssuerKeyEntry* entry,
        uint8_t type,
        const uint8_t* key,
        uint8_t status) {
    HAPPrecondition(entry);

    // For add request, only Type and Key are populated
    entry->request.type = type;
    entry->request.typeIsSet = true;
    entry->request.key = key;
    entry->request.keyIsSet = true;
    entry->request.identifierIsSet = false;

    // For add response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for a remove operation for NFC access issuer key
 *
 * @param[out]   entry        NFC access issuer key entry that contains the request and expected response
 * @param        identifier   identifier to remove for the request
 * @param        status       expected status for the operation
 */
static void
        SetupRemoveOperationIssuerKeyEntry(NfcAccessIssuerKeyEntry* entry, const uint8_t* identifier, uint8_t status) {
    HAPPrecondition(entry);

    // For remove request, only Identifier field is populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;

    // For remove response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for a list operation for NFC access device credential key
 *
 * @param[out]   entry                 NFC access device credential key entry that contains the request and expected
 * response
 * @param        state                 state of key to list for in the request
 * @param        issuerKeyIdentifier   expected issuer key identifier associated with this device credential key
 * @param        identifier            expected identifier for the operation
 * @param        status                expected status for the operation
 */
static void SetupListOperationDeviceCredentialKeyEntry(
        NfcAccessDeviceCredentialKeyEntry* entry,
        uint8_t state,
        const uint8_t* issuerKeyIdentifier,
        const uint8_t* identifier,
        uint8_t status) {
    HAPPrecondition(entry);

    // For list request, only state field is populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.issuerKeyIdentifierIsSet = false;
    entry->request.state = state;
    entry->request.stateIsSet = true;
    entry->request.identifierIsSet = false;

    // For list response, all fields are expected
    entry->response.identifier = identifier;
    entry->response.identifierIsExpected = true;
    entry->response.identifierIsChecked = true;
    entry->response.issuerKeyIdentifier = issuerKeyIdentifier;
    entry->response.issuerKeyIdentifierIsExpected = true;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for an add operation for NFC access device credential key
 *
 * @param[out]   entry                 NFC access device credential key entry that contains the request and expected
 * response
 * @param        type                  key type to add for the request
 * @param        key                   key to add for the request
 * @param        issuerKeyIdentifier   issuer key identifier associated with this device credential key to add for the
 * request
 * @param        state                 state of key to add in the request
 * @param        status                expected status for the operation
 */
static void SetupAddOperationDeviceCredentialKeyEntry(
        NfcAccessDeviceCredentialKeyEntry* entry,
        uint8_t type,
        const uint8_t* key,
        const uint8_t* issuerKeyIdentifier,
        uint8_t state,
        uint8_t status) {
    HAPPrecondition(entry);

    // For add request, only Type, Key, Issuer Key Identifier, and State are populated
    entry->request.type = type;
    entry->request.typeIsSet = true;
    entry->request.key = key;
    entry->request.keyIsSet = true;
    entry->request.issuerKeyIdentifier = issuerKeyIdentifier;
    entry->request.issuerKeyIdentifierIsSet = true;
    entry->request.state = state;
    entry->request.stateIsSet = true;
    entry->request.identifierIsSet = false;

    // For add response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.issuerKeyIdentifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for a remove operation for NFC access device credential key
 *
 * @param[out]   entry        NFC access device credential key entry that contains the request and expected response
 * @param        identifier   identifier to remove for the request
 * @param        status       expected status for the operation
 */
static void SetupRemoveOperationDeviceCredentialKeyEntry(
        NfcAccessDeviceCredentialKeyEntry* entry,
        const uint8_t* identifier,
        uint8_t status) {
    HAPPrecondition(entry);

    // For remove request, only Identifier field is populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.issuerKeyIdentifierIsSet = false;
    entry->request.stateIsSet = false;
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;

    // For remove response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.issuerKeyIdentifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for a list operation for NFC access reader key
 *
 * @param[out]   entry        NFC access reader key entry that contains the request and expected response
 * @param        identifier   expected identifier for the operation
 * @param        status       expected status for the operation
 */
static void
        SetupListOperationReaderKeyEntry(NfcAccessReaderKeyEntry* entry, const uint8_t* identifier, uint8_t status) {
    HAPPrecondition(entry);

    // For list request, no fields are populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.readerIdentifierIsExpected = false;
    entry->request.identifierIsSet = false;

    // For list response, all fields are expected
    entry->response.identifier = identifier;
    entry->response.identifierIsExpected = true;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for an add operation for NFC access reader key
 *
 * @param[out]   entry              NFC access reader key entry that contains the request and expected response
 * @param        type               key type to add for the request
 * @param        key                key to add for the request
 * @param        readerIdentifier   reader identifier to add for the request
 * @param        status             expected status for the operation
 */
static void SetupAddOperationReaderKeyEntry(
        NfcAccessReaderKeyEntry* entry,
        uint8_t type,
        const uint8_t* key,
        const uint8_t* readerIdentifier,
        uint8_t status) {
    HAPPrecondition(entry);

    // For add request, only Type, Key, and Reader Identifier are populated
    entry->request.type = type;
    entry->request.typeIsSet = true;
    entry->request.key = key;
    entry->request.keyIsSet = true;
    entry->request.readerIdentifier = readerIdentifier;
    entry->request.readerIdentifierIsExpected = true;
    entry->request.identifierIsSet = false;

    // For add response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Setup the request and expected response for a remove operation for NFC access reader key
 *
 * @param[out]   entry        NFC access reader key entry that contains the request and expected response
 * @param        identifier   identifier to remove for the request
 * @param        status       expected status for the operation
 */
static void
        SetupRemoveOperationReaderKeyEntry(NfcAccessReaderKeyEntry* entry, const uint8_t* identifier, uint8_t status) {
    HAPPrecondition(entry);

    // For remove request, only Identifier field is populated
    entry->request.typeIsSet = false;
    entry->request.keyIsSet = false;
    entry->request.readerIdentifierIsExpected = false;
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;

    // For remove response, only Status Code field is expected
    entry->response.identifierIsExpected = false;
    entry->response.status = status;
    entry->response.statusIsExpected = true;
}

/**
 * Calculate the number of TLV types/fields expected for an NFC access issuer key response
 */
static uint8_t CalculateIssuerKeyNumExpectedTLVs(const NfcAccessIssuerKeyResponse* response) {
    HAPPrecondition(response);

    uint8_t numExpectedTLVs = 0;

    if (response->identifierIsExpected) {
        numExpectedTLVs++;
    }
    if (response->statusIsExpected) {
        numExpectedTLVs++;
    }

    return numExpectedTLVs;
}

/**
 * Calculate the number of TLV types/fields expected for an NFC access device credential key response
 */
static uint8_t CalculateDeviceCredentialKeyNumExpectedTLVs(const NfcAccessDeviceCredentialKeyResponse* response) {
    HAPPrecondition(response);

    uint8_t numExpectedTLVs = 0;

    if (response->identifierIsExpected) {
        numExpectedTLVs++;
    }
    if (response->issuerKeyIdentifierIsExpected) {
        numExpectedTLVs++;
    }
    if (response->statusIsExpected) {
        numExpectedTLVs++;
    }

    return numExpectedTLVs;
}

/**
 * Calculate the number of TLV types/fields expected for an NFC access reader key response
 */
static uint8_t CalculateReaderKeyNumExpectedTLVs(const NfcAccessReaderKeyResponse* response) {
    HAPPrecondition(response);

    uint8_t numExpectedTLVs = 0;

    if (response->identifierIsExpected) {
        numExpectedTLVs++;
    }
    if (response->statusIsExpected) {
        numExpectedTLVs++;
    }

    return numExpectedTLVs;
}

/**
 * Emulate controller sending a write request to operate on an NFC access issuer key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   entry        data for the write request
 */
static HAPError SendIssuerKeyWriteRequest(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        const NfcAccessIssuerKeyEntry* entry) {
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Writing NFC Access Issuer Key Request for operation %d", operation);
    {
        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };

        uint8_t requestBuffer[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
        uint8_t operationType[] = { operation };
        HAPTLV tlv = {
            .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType,
            .value = { .bytes = operationType, .numBytes = sizeof operationType },
        };
        err = HAPTLVWriterAppend(&requestWriter, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);

        void* controlPointRequestBytes;
        size_t numControlPointRequestBytes;
        uint8_t controlPointRequestBuffer[256];
        {
            HAPTLVWriter controlPointRequestWriter;
            HAPTLVWriterCreate(&controlPointRequestWriter, controlPointRequestBuffer, sizeof controlPointRequestBuffer);

            if (entry) {
                if (entry->request.typeIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Type,
                        .value = { .bytes = &entry->request.type, .numBytes = sizeof entry->request.type },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entry->request.keyIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Key,
                        .value = { .bytes = entry->request.key, .numBytes = NFC_ACCESS_ISSUER_KEY_BYTES },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entry->request.identifierIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Identifier,
                        .value = { .bytes = entry->request.identifier, .numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }
            }

            HAPTLVWriterGetBuffer(&controlPointRequestWriter, &controlPointRequestBytes, &numControlPointRequestBytes);

            tlv = {
                .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyRequest,
                .value = { .bytes = controlPointRequestBytes, .numBytes = numControlPointRequestBytes },
            };
            err = HAPTLVWriterAppend(&requestWriter, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
        }

        void* requestBytes;
        size_t numRequestBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        return err;
    }
}

/**
 * Emulate controller sending a write request to operate on an NFC access device credential key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   entry        data for the write request
 */
static HAPError SendDeviceCredentialKeyWriteRequest(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        const NfcAccessDeviceCredentialKeyEntry* entry) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(entry);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Writing NFC Access Device Credential Key Request for operation %d", operation);
    {
        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };

        uint8_t requestBuffer[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
        uint8_t operationType[] = { operation };
        HAPTLV tlv = {
            .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType,
            .value = { .bytes = operationType, .numBytes = sizeof operationType },
        };
        err = HAPTLVWriterAppend(&requestWriter, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);

        void* controlPointRequestBytes;
        size_t numControlPointRequestBytes;
        uint8_t controlPointRequestBuffer[256];
        {
            HAPTLVWriter controlPointRequestWriter;
            HAPTLVWriterCreate(&controlPointRequestWriter, controlPointRequestBuffer, sizeof controlPointRequestBuffer);

            if (entry->request.typeIsSet) {
                tlv = {
                    .type = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Type,
                    .value = { .bytes = &entry->request.type, .numBytes = sizeof entry->request.type },
                };
                err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            }

            if (entry->request.keyIsSet) {
                tlv = {
                    .type = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Key,
                    .value = { .bytes = entry->request.key, .numBytes = NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES },
                };
                err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            }

            if (entry->request.issuerKeyIdentifierIsSet) {
                tlv = {
                    .type = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_IssuerKeyIdentifier,
                    .value = { .bytes = entry->request.issuerKeyIdentifier,
                               .numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES },
                };
                err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            }

            if (entry->request.stateIsSet) {
                tlv = {
                    .type = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_State,
                    .value = { .bytes = &entry->request.state, .numBytes = sizeof entry->request.state },
                };
                err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            }

            if (entry->request.identifierIsSet) {
                tlv = {
                    .type = kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Identifier,
                    .value = { .bytes = entry->request.identifier, .numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES },
                };
                err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
            }

            HAPTLVWriterGetBuffer(&controlPointRequestWriter, &controlPointRequestBytes, &numControlPointRequestBytes);

            tlv = {
                .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyRequest,
                .value = { .bytes = controlPointRequestBytes, .numBytes = numControlPointRequestBytes },
            };
            err = HAPTLVWriterAppend(&requestWriter, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
        }

        void* requestBytes;
        size_t numRequestBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        return err;
    }
}

/**
 * Emulate controller sending a write request to operate on an NFC access reader key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   entry        data for the write request
 */
static HAPError SendReaderKeyWriteRequest(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        const NfcAccessReaderKeyEntry* entry) {
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Writing NFC Access Reader Key Request for operation %d", operation);
    {
        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };

        uint8_t requestBuffer[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
        uint8_t operationType[] = { operation };
        HAPTLV tlv = {
            .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType,
            .value = { .bytes = operationType, .numBytes = sizeof operationType },
        };
        err = HAPTLVWriterAppend(&requestWriter, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);

        void* controlPointRequestBytes;
        size_t numControlPointRequestBytes;
        uint8_t controlPointRequestBuffer[256];
        {
            HAPTLVWriter controlPointRequestWriter;
            HAPTLVWriterCreate(&controlPointRequestWriter, controlPointRequestBuffer, sizeof controlPointRequestBuffer);

            if (entry) {
                if (entry->request.typeIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Type,
                        .value = { .bytes = &entry->request.type, .numBytes = sizeof entry->request.type },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entry->request.keyIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Key,
                        .value = { .bytes = entry->request.key, .numBytes = NFC_ACCESS_READER_KEY_BYTES },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entry->request.readerIdentifierIsExpected) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_ReaderIdentifier,
                        .value = { .bytes = entry->request.readerIdentifier,
                                   .numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entry->request.identifierIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Identifier,
                        .value = { .bytes = entry->request.identifier, .numBytes = NFC_ACCESS_KEY_IDENTIFIER_BYTES },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }
            }

            HAPTLVWriterGetBuffer(&controlPointRequestWriter, &controlPointRequestBytes, &numControlPointRequestBytes);

            tlv = {
                .type = kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyRequest,
                .value = { .bytes = controlPointRequestBytes, .numBytes = numControlPointRequestBytes },
            };
            err = HAPTLVWriterAppend(&requestWriter, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
        }

        void* requestBytes;
        size_t numRequestBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        return err;
    }
}

/**
 * Verify the write response is as expected for operation on NFC access issuer key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   numEntries   expected number of NFC access issuer key responses to verify
 * @param   entries      data containing expected values
 */
static void VerifyIssuerKeyWriteResponse(
        HAPSession* session,
        HAPAccessoryServer* server,
        size_t numEntries,
        const NfcAccessIssuerKeyEntry* entries) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(numEntries == 0 || entries);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading NFC Access Issuer Key Response");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPTLV tlv;
        bool valid;
        uint8_t numFoundResponses = 0;

        // Read each NFC Access Issuer Key Response
        for (;;) {
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyResponse);
            TEST_ASSERT(numFoundResponses < numEntries);

            HAPTLVReader nestedReader;
            HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);

            uint8_t numExpectedTLVs = CalculateIssuerKeyNumExpectedTLVs(&entries[numFoundResponses].response);
            uint8_t numFoundTLVs = 0;

            // Read all TLV types for a response
            for (;;) {
                HAPTLV nestedTLV;
                bool nestedValid;
                err = HAPTLVReaderGetNext(&nestedReader, &nestedValid, &nestedTLV);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
                if (!nestedValid) {
                    break;
                }

                switch (nestedTLV.type) {
                    case kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_Identifier: {
                        if (entries[numFoundResponses].response.identifierIsExpected) {
                            TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, NFC_ACCESS_KEY_IDENTIFIER_BYTES);
                            TEST_ASSERT(HAPRawBufferAreEqual(
                                    nestedTLV.value.bytes,
                                    entries[numFoundResponses].response.identifier,
                                    nestedTLV.value.numBytes));
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_StatusCode: {
                        if (entries[numFoundResponses].response.statusIsExpected) {
                            TEST_ASSERT_EQUAL(
                                    nestedTLV.value.numBytes, sizeof entries[numFoundResponses].response.status);
                            TEST_ASSERT_EQUAL(
                                    HAPReadUInt8(nestedTLV.value.bytes), entries[numFoundResponses].response.status);
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    default:
                        // Unknown TLV type
                        TEST_ASSERT(false);
                        break;
                }

                numFoundTLVs++;
            }

            TEST_ASSERT_EQUAL(numExpectedTLVs, numFoundTLVs);

            if (numFoundResponses + 1 < numEntries) {
                // TLV separator expected
                err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
                TEST_ASSERT(valid);
                TEST_ASSERT_EQUAL(tlv.type, 0);
            }

            numFoundResponses++;
        }

        TEST_ASSERT_EQUAL(numEntries, numFoundResponses);
    }
}

/**
 * Verify the write response is as expected for operation on NFC access device credential key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   numEntries   expected number of NFC access device credential key responses to verify
 * @param   entries      data containing expected values
 */
static void VerifyDeviceCredentialKeyWriteResponse(
        HAPSession* session,
        HAPAccessoryServer* server,
        size_t numEntries,
        const NfcAccessDeviceCredentialKeyEntry* entries) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(numEntries == 0 || entries);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading NFC Access Device Credential Key Response");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPTLV tlv;
        bool valid;
        uint8_t numFoundResponses = 0;

        // Read each NFC Access Issuer Key Response
        for (;;) {
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyResponse);
            TEST_ASSERT(numFoundResponses < numEntries);

            HAPTLVReader nestedReader;
            HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);

            uint8_t numExpectedTLVs = CalculateDeviceCredentialKeyNumExpectedTLVs(&entries[numFoundResponses].response);
            uint8_t numFoundTLVs = 0;

            // Read all TLV types for a response
            for (;;) {
                HAPTLV nestedTLV;
                bool nestedValid;
                err = HAPTLVReaderGetNext(&nestedReader, &nestedValid, &nestedTLV);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
                if (!nestedValid) {
                    break;
                }

                switch (nestedTLV.type) {
                    case kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_Identifier: {
                        if (entries[numFoundResponses].response.identifierIsExpected) {
                            TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, NFC_ACCESS_KEY_IDENTIFIER_BYTES);
                            if (entries[numFoundResponses].response.identifierIsChecked) {
                                TEST_ASSERT(HAPRawBufferAreEqual(
                                        nestedTLV.value.bytes,
                                        entries[numFoundResponses].response.identifier,
                                        nestedTLV.value.numBytes));
                            }
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_IssuerKeyIdentifier: {
                        if (entries[numFoundResponses].response.issuerKeyIdentifierIsExpected) {
                            TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, NFC_ACCESS_KEY_IDENTIFIER_BYTES);
                            TEST_ASSERT(HAPRawBufferAreEqual(
                                    nestedTLV.value.bytes,
                                    entries[numFoundResponses].response.issuerKeyIdentifier,
                                    nestedTLV.value.numBytes));
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_StatusCode: {
                        if (entries[numFoundResponses].response.statusIsExpected) {
                            TEST_ASSERT_EQUAL(
                                    nestedTLV.value.numBytes, sizeof entries[numFoundResponses].response.status);
                            TEST_ASSERT_EQUAL(
                                    HAPReadUInt8(nestedTLV.value.bytes), entries[numFoundResponses].response.status);
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    default:
                        // Unknown TLV type
                        TEST_ASSERT(false);
                        break;
                }

                numFoundTLVs++;
            }

            TEST_ASSERT_EQUAL(numExpectedTLVs, numFoundTLVs);

            if (numFoundResponses + 1 < numEntries) {
                // TLV separator expected
                err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
                TEST_ASSERT(valid);
                TEST_ASSERT_EQUAL(tlv.type, 0);
            }

            numFoundResponses++;
        }

        TEST_ASSERT_EQUAL(numEntries, numFoundResponses);
    }
}

/**
 * Verify the write response is as expected for operation on NFC access reader key
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   entry        data containing expected values
 */
static void VerifyReaderKeyWriteResponse(
        HAPSession* session,
        HAPAccessoryServer* server,
        const NfcAccessReaderKeyEntry* entry) {
    HAPPrecondition(session);
    HAPPrecondition(server);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading NFC Access Reader Key Response");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessControlPointCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };
        err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPTLV tlv;
        bool valid;
        uint8_t numFoundResponses = 0;

        // Read the NFC Access Reader Key Response
        for (;;) {
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyResponse);

            HAPTLVReader nestedReader;
            HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);

            uint8_t numExpectedTLVs = CalculateReaderKeyNumExpectedTLVs(&entry->response);
            uint8_t numFoundTLVs = 0;

            // Read all TLV types for a response
            for (;;) {
                HAPTLV nestedTLV;
                bool nestedValid;
                err = HAPTLVReaderGetNext(&nestedReader, &nestedValid, &nestedTLV);
                TEST_ASSERT_EQUAL(err, kHAPError_None);
                if (!nestedValid) {
                    break;
                }

                switch (nestedTLV.type) {
                    case kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_Identifier: {
                        if (entry->response.identifierIsExpected) {
                            TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, NFC_ACCESS_KEY_IDENTIFIER_BYTES);
                            TEST_ASSERT(HAPRawBufferAreEqual(
                                    nestedTLV.value.bytes, entry->response.identifier, nestedTLV.value.numBytes));
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_StatusCode: {
                        if (entry->response.statusIsExpected) {
                            TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, sizeof entry->response.status);
                            TEST_ASSERT_EQUAL(HAPReadUInt8(nestedTLV.value.bytes), entry->response.status);
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    default:
                        // Unknown TLV type
                        TEST_ASSERT(false);
                        break;
                }

                numFoundTLVs++;
            }

            TEST_ASSERT_EQUAL(numExpectedTLVs, numFoundTLVs);

            numFoundResponses++;
        }

        if (!entry) {
            TEST_ASSERT_EQUAL(numFoundResponses, 0);
        } else {
            TEST_ASSERT_EQUAL(numFoundResponses, 1);
        }
    }
}

/**
 * Emulate controller reading the configuration state value
 *
 * @param        session              session
 * @param        server               server
 * @param[out]   configurationState   the configuration state value
 */
static void ReadConfigurationState(HAPSession* session, HAPAccessoryServer* server, uint16_t* configurationState) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(configurationState);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading NFC Access Configuration State");
    {
        const HAPUInt16CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &nfcAccessConfigurationStateCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };
        err = request.characteristic->callbacks.handleRead(server, &request, configurationState, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

/**
 * Emulate controller operating on an NFC access issuer key. For all operation types, only one request per write. For
 * List operation type, there may be zero, one, or more more than one responses for one write request. For all other
 * operation types, there is only one response for one write request.
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   numEntries   expected number of NFC access issuer key responses to verify
 * @param   entries      array of NFC access issuer keys to operate on and verify
 */
static void OperateOnIssuerKey(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        size_t numEntries,
        const NfcAccessIssuerKeyEntry* entries) {
    HAPError err = SendIssuerKeyWriteRequest(session, server, operation, entries);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    VerifyIssuerKeyWriteResponse(session, server, numEntries, entries);
}

/**
 * Emulate controller operating on an NFC access device credential key. For all operation types, only one request per
 * write. For List operation type, there may be zero, one, or more more than one responses for one write request. For
 * all other operation types, there is only one response for one write request.
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   numEntries   expected number of NFC access device credential key responses to verify
 * @param   entries      array of NFC access device credential keys to operate on and verify
 */
static void OperateOnDeviceCredentialKey(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        size_t numEntries,
        const NfcAccessDeviceCredentialKeyEntry* entries) {
    HAPError err = SendDeviceCredentialKeyWriteRequest(session, server, operation, &entries[0]);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    VerifyDeviceCredentialKeyWriteResponse(session, server, numEntries, entries);
}

/**
 * Emulate controller operating on an NFC access reader key. For all operation types, only one request per write. For
 * all operation types, there is only one response for one write request.
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   entry        NFC access reader key to operate on and verify
 */
static void OperateOnReaderKey(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        const NfcAccessReaderKeyEntry* entry) {
    HAPError err = SendReaderKeyWriteRequest(session, server, operation, entry);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    VerifyReaderKeyWriteResponse(session, server, entry);
}

/**
 * Emulate accessory server notifications to a session. Accessory server reads characteristics when notification is to
 * be sent out.
 * @param   session                      session
 * @param   server                       accessory server
 * @param   expectedConfigurationState   expected configuration state value during notification
 */
static void ExpectNotification(HAPSession* session, HAPAccessoryServer* server, uint8_t expectedConfigurationState) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    uint16_t configurationState;
    ReadConfigurationState(session, server, &configurationState);
    TEST_ASSERT_EQUAL(configurationState, expectedConfigurationState);
}

/**
 * Emulate controller disconnecting
 *
 * @param   server    accessory server
 * @param   session   session relevant to the controller emulated to be disconnecting
 */
static void DisconnectController(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    HAPHandleNfcAccessSessionInvalidate(server, session);
}

/**
 * Emulate controller connecting
 *
 * @param   server    accessory server
 * @param   session   session relevant to the controller emulated to be connecting
 */
static void ConnectController(HAPAccessoryServer* server, HAPSession* session) {
    HAPPrecondition(server);
    HAPPrecondition(session);

    // Nothing to do in terms of handlers
}

/**
 * Setup step before a test
 */
TEST_SETUP() {
    // Clear all keys
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kNfcAccessKeyStoreDomain);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Make sure all cached lists are reloaded
    err = HAPPlatformNfcAccessCreate(platform.keyValueStore, kNfcAccessKeyStoreDomain, NULL, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Tests NFC Access Supported Configuration read handler
 */
TEST(TestNfcAccessSupportedConfiguration) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    HAPLog(&kHAPLog_Default, "Reading Access Code Supported Configuration.");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &sessions[0],
            .characteristic = &nfcAccessSupportedConfigurationCharacteristic,
            .service = &nfcAccessService,
            .accessory = &accessory,
        };
        HAPError err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        bool foundTLVs[3];
        HAPRawBufferZero(foundTLVs, sizeof foundTLVs);
        for (;;) {
            HAPTLV tlv;
            bool valid;
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT(tlv.type != 0 && tlv.type <= sizeof foundTLVs / sizeof foundTLVs[0]);
            TEST_ASSERT(!foundTLVs[tlv.type - 1]);
            foundTLVs[tlv.type - 1] = true;
            switch (tlv.type) {
                case kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumIssuerKeys: {
                    uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                    TEST_ASSERT_EQUAL(value, HAPPlatformNfcAccessGetMaximumIssuerKeys());
                    break;
                }
                case kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumSuspendedDeviceCredentialKeys: {
                    uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                    TEST_ASSERT_EQUAL(value, HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys());
                    break;
                }
                case kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumActiveDeviceCredentialKeys: {
                    uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                    TEST_ASSERT_EQUAL(value, HAPPlatformNfcAccessGetMaximumActiveDeviceCredentialKeys());
                    break;
                }
            }
        }

        for (size_t i = 0; i < HAPArrayCount(foundTLVs); i++) {
            TEST_ASSERT(foundTLVs[i]);
        }
    }
}

/**
 * Tests controller performing all operations for NFC access issuer key, expecting success
 * - Add an issuer key
 * - List to verify there is only one response and the identifier is as expected
 * - Add a second issuer key
 * - List to verify there are two responses and the identifiers are as expected
 * - Remove the first issuer key
 * - List to verify the second issuer key identifier is retrieved
 * - Remove the second issuer key
 * - List to verify no issuer keys are retrieved
 * - Add the first issuer key again to verify it can be added (not a duplicate case)
 * - List to verify there is only one response and the identifier is as expected
 */
TEST(TestIssuerKeySuccessful) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key
    NfcAccessIssuerKeyEntry entries[2];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationIssuerKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a second issuer key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationIssuerKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifiers of both keys
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    SetupListOperationIssuerKeyEntry(
            &entries[1], ISSUER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            2,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove first issuer key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entries[0]);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of second key still exists
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove second issuer key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entries[0]);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no identifiers exist
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, 0, NULL);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add the first issuer key again and verify it can be added (not a duplicate)
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationIssuerKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationIssuerKeyEntry(
            &entries[0], ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing add operations for NFC access issuer key
 */
TEST(TestIssuerKeyAddFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key
    NfcAccessIssuerKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Duplicate
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Not supported (bad key type NIST256)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            ISSUER_KEY_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Set the maximum allowed issuer keys to 1
    uint16_t maxIssuerKeys = HAPPlatformNfcAccessGetMaximumIssuerKeys();
    HAPError err = HAPPlatformNfcAccessSetMaximumIssuerKeys(1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Out of resources
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorOutOfResources);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Reset the maximum allowed issuer keys
    err = HAPPlatformNfcAccessSetMaximumIssuerKeys(maxIssuerKeys);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing remove operations for NFC access issuer key
 */
TEST(TestIssuerKeyRemoveFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key
    NfcAccessIssuerKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove a non-existent entry
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationIssuerKeyEntry(
            &entry, ISSUER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove the only issuer key added
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationIssuerKeyEntry(
            &entry, ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove an already removed entry
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationIssuerKeyEntry(
            &entry, ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove operation have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various invalid NFC access issuer key requests
 */
TEST(TestIssuerKeyInvalidRequests) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    /**
     * For list operation, no fields are required fields, any extra fields populated should be ignored
     */

    // All fields set
    NfcAccessIssuerKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519;
    entry.request.typeIsSet = true;
    entry.request.key = ISSUER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.identifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    HAPError err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    /**
     * For add operation, Type and Key are required fields
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Type field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.key = ISSUER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.identifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Key field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519;
    entry.request.typeIsSet = true;
    entry.request.identifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    /**
     * For remove operation, Identifier is a required field
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Identifier field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519;
    entry.request.typeIsSet = true;
    entry.request.key = ISSUER_KEY_1;
    entry.request.keyIsSet = true;
    err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing all operations for NFC access issuer key, expecting success
 * - Add an active device credential key
 * - List to verify there is only one response for active keys and the identifier is as expected
 * - List to verify there are no responses for suspended keys
 * - Add a second active device credential key
 * - List to verify there are two responses for active keys and the identifiers are as expected
 * - List to verify there are no responses for suspended keys
 * - Update the first key to suspended state
 * - List to verify there is only one active key
 * - List to verify there is only one suspended key
 * - Remove the first key (suspended)
 * - List to verify there is only one active key
 * - List to verify there are no suspended keys
 * - Remove the second key (active)
 * - List to verify there are no active keys
 * - List to verify there are no suspended keys
 * - Add the first device credential key again to verify it can be added (not a duplicate case)
 */
TEST(TestDeviceCredentialKeySuccessful) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key to be associated with the device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key
    NfcAccessDeviceCredentialKeyEntry entries[2];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added active key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no suspended keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a second active device credential key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifiers of both active keys
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[1],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            2,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no suspended keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Update the first key to suspended
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify there is one active key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify there is one suspended key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove first device credential key (suspended)
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationDeviceCredentialKeyEntry(
            &entries[0],
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entries[0]);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify there is one active key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no suspended keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove second device credential key (active)
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationDeviceCredentialKeyEntry(
            &entries[0],
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entries[0]);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no active keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no suspended keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add the first device credential key (active) again and verify it can be added (not a duplicate)
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added active key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no suspended keys exist
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller evicting the least recently used active key when the active key list is full
 */
TEST(TestDeviceCredentialKeyEviction) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Set the maximum allowed active device credential keys to 2
    uint16_t maxActiveKeys = HAPPlatformNfcAccessGetMaximumActiveDeviceCredentialKeys();
    HAPError err = HAPPlatformNfcAccessSetMaximumActiveDeviceCredentialKeys(2);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Add an issuer key to be associated with the device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key
    NfcAccessDeviceCredentialKeyEntry entries[2];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a second active device credential key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifiers of both active keys
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[1],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            2,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a third active device credential key (expect least recently used to be evicted)
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_3,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify the first device credential key has been evicted
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_3,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[1],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            2,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Reset the maximum allowed active device credential keys
    err = HAPPlatformNfcAccessSetMaximumActiveDeviceCredentialKeys(maxActiveKeys);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
}

/**
 * Tests controller properly removing all NFC access device credential keys corresponding to a removed issuer key
 */
TEST(TestDeviceCredentialKeyRemoveAssociatedKeys) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key to be associated with some device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a second issuer key to be associated with some device credential keys
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key associated with the second issuer key
    NfcAccessDeviceCredentialKeyEntry entries[2];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a suspended device credential key associated with the second issuer key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key associated with the first issuer key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_3,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &entries[0]);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify both identifiers of the added active keys
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_2,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[1],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_3,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            2,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added suspended key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            ISSUER_KEY_IDENTIFIER_2,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove second issuer key which is associated with one active and one suspended device credential key
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupRemoveOperationIssuerKeyEntry(
            &issuerKeyEntry, ISSUER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &issuerKeyEntry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify there is only one active key
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_3,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify there are no suspended keys
    HAPRawBufferZero(&entries, sizeof entries);
    SetupListOperationDeviceCredentialKeyEntry(
            &entries[0],
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing add operations for NFC access device credential key
 */
TEST(TestDeviceCredentialKeyAddFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key to be associated with the device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key
    NfcAccessDeviceCredentialKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a suspended device credential key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Duplicate (active key)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Duplicate (suspended key)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Does not exist (issuer key 2 has not been added)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_3,
            ISSUER_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Not supported (bad key type Ed25519)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            DEVICE_CREDENTIAL_KEY_3,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Set the maximum allowed suspended device credential keys to 1
    uint16_t maxSuspendedKeys = HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys();
    HAPError err = HAPPlatformNfcAccessSetMaximumSuspendedDeviceCredentialKeys(1);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Out of resources (for suspended keys)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_3,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorOutOfResources);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Reset the maximum allowed suspended device credential keys
    err = HAPPlatformNfcAccessSetMaximumSuspendedDeviceCredentialKeys(maxSuspendedKeys);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing remove operations for NFC access device credential key
 */
TEST(TestDeviceCredentialKeyRemoveFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key to be associated with the device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key
    NfcAccessDeviceCredentialKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationDeviceCredentialKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove a non-existent entry
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationDeviceCredentialKeyEntry(
            &entry,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_3,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove the only device credential key added
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationDeviceCredentialKeyEntry(
            &entry, DEVICE_CREDENTIAL_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove an already removed entry
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationDeviceCredentialKeyEntry(
            &entry,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove,
            1,
            &entry);

    // Remove operation have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various invalid NFC access device credential key requests
 */
TEST(TestDeviceCredentialKeyInvalidRequests) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    /**
     * For list operation, State is a required field, any extra fields populated should be ignored
     */

    // No fields set
    NfcAccessDeviceCredentialKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    HAPError err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except State field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = DEVICE_CREDENTIAL_KEY_3;
    entry.request.keyIsSet = true;
    entry.request.issuerKeyIdentifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.issuerKeyIdentifierIsSet = true;
    entry.request.identifier = DEVICE_CREDENTIAL_KEY_IDENTIFIER_3;
    entry.request.identifierIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    /**
     * For add operation, Type, Key, Issuer Key Identifier, and State are required fields
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Type field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.key = DEVICE_CREDENTIAL_KEY_2;
    entry.request.keyIsSet = true;
    entry.request.issuerKeyIdentifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.issuerKeyIdentifierIsSet = true;
    entry.request.state = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active;
    entry.request.stateIsSet = true;
    entry.request.identifier = DEVICE_CREDENTIAL_KEY_IDENTIFIER_2;
    entry.request.identifierIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Key field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.issuerKeyIdentifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.issuerKeyIdentifierIsSet = true;
    entry.request.state = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active;
    entry.request.stateIsSet = true;
    entry.request.identifier = DEVICE_CREDENTIAL_KEY_IDENTIFIER_2;
    entry.request.identifierIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Issuer Key Identifier field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = DEVICE_CREDENTIAL_KEY_2;
    entry.request.keyIsSet = true;
    entry.request.state = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active;
    entry.request.stateIsSet = true;
    entry.request.identifier = DEVICE_CREDENTIAL_KEY_IDENTIFIER_2;
    entry.request.identifierIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except State field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = DEVICE_CREDENTIAL_KEY_2;
    entry.request.keyIsSet = true;
    entry.request.issuerKeyIdentifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.issuerKeyIdentifierIsSet = true;
    entry.request.identifier = DEVICE_CREDENTIAL_KEY_IDENTIFIER_2;
    entry.request.identifierIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    /**
     * For remove operation, Identifier is a required field
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Identifier field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = DEVICE_CREDENTIAL_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.issuerKeyIdentifier = ISSUER_KEY_IDENTIFIER_1;
    entry.request.issuerKeyIdentifierIsSet = true;
    entry.request.state = kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active;
    entry.request.stateIsSet = true;
    err = SendDeviceCredentialKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing all operations for NFC access reader key, expecting success
 * - Add a reader key
 * - List to verify there is only one response and the identifier is as expected
 * - Remove the reader key
 * - List to verify no reader keys are retrieved
 * - Add the same reader key again to verify it can be added (not a duplicate case)
 * - List to verify there is only one response and the identifier is as expected
 * - Remove the reader key
 * - List to verify no reader keys are retrieved
 * - Add a different reader key to verify it can be added
 * - List to verify there is only one response and the identifier is as expected
 */
TEST(TestReaderKeySuccessful) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add a reader key
    NfcAccessReaderKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupListOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove the reader key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no identifiers exist
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, NULL);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add the same reader key again to and verify it can be added (not a duplicate)
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupListOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove the reader key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify no identifiers exist
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, NULL);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add the a different reader key and verify it can be added
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_2,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupListOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing add operations for NFC access reader key
 */
TEST(TestReaderKeyAddFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Not supported (bad key type Ed25519)
    NfcAccessReaderKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorNotSupported);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add a reader key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Duplicate
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDuplicate);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Out of resources
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_2,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorOutOfResources);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing remove operations for NFC access reader key
 */
TEST(TestReaderKeyRemoveFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add a reader key
    NfcAccessReaderKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationReaderKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove a non-existent entry
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_2, kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);

    // Remove the reader key added
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove the already removed key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupRemoveOperationReaderKeyEntry(
            &entry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_ErrorDoesNotExist);
    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);

    // Remove operation have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various invalid NFC access reader key requests
 */
TEST(TestReaderKeyInvalidRequests) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    /**
     * For list operation, no fields are required fields, any extra fields populated should be ignored
     */

    // All fields set
    NfcAccessReaderKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = READER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.readerIdentifier = READER_IDENTIFIER;
    entry.request.readerIdentifierIsExpected = true;
    entry.request.identifier = READER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    HAPError err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    /**
     * For add operation, Type, Key, and Reader Identifier are required fields
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Type field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.key = READER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.readerIdentifier = READER_IDENTIFIER;
    entry.request.readerIdentifierIsExpected = true;
    entry.request.identifier = READER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Key field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.readerIdentifier = READER_IDENTIFIER;
    entry.request.readerIdentifierIsExpected = true;
    entry.request.identifier = READER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Reader Identifier field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = READER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.identifier = READER_KEY_IDENTIFIER_1;
    entry.request.identifierIsSet = true;
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    /**
     * For remove operation, Identifier is a required field
     */

    // No fields set
    HAPRawBufferZero(&entry, sizeof entry);
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All fields set except Identifier field
    HAPRawBufferZero(&entry, sizeof entry);
    entry.request.type = kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256;
    entry.request.typeIsSet = true;
    entry.request.key = READER_KEY_1;
    entry.request.keyIsSet = true;
    entry.request.readerIdentifier = READER_IDENTIFIER;
    entry.request.readerIdentifierIsExpected = true;
    err = SendReaderKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Remove, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_InvalidData);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller receiving the maximum response size by listing the maximum amount of suspended device credential
 * keys
 */
TEST(TestMaxResponseSize) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key to be associated with the device credential keys
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    NfcAccessDeviceCredentialKeyEntry entries[HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys()];
    uint8_t deviceCredentialKeys[HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys()]
                                [NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES];

    // Add maximum suspended device credential keys allowed
    for (size_t i = 0; i < HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys(); i++) {
        // Generate a suspended device credential key
        HAPRawBufferCopyBytes(deviceCredentialKeys[i], DEVICE_CREDENTIAL_KEY_1, NFC_ACCESS_DEVICE_CREDENTIAL_KEY_BYTES);
        deviceCredentialKeys[i][i] = 0xAA;

        HAPRawBufferZero(&entries, sizeof entries);
        SetupAddOperationDeviceCredentialKeyEntry(
                &entries[0],
                kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
                deviceCredentialKeys[i],
                ISSUER_KEY_IDENTIFIER_1,
                kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
                kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
        OperateOnDeviceCredentialKey(
                &sessions[0],
                &accessoryServer,
                kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
                1,
                &entries[0]);

        // Add operation results in an increment to the configuration state value
        configurationState++;
        for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
            ExpectNotification(&sessions[i], &accessoryServer, configurationState);
        }
    }

    // Perform list operation to verify that maximum suspended device credential keys are returned
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys(); i++) {
        SetupListOperationDeviceCredentialKeyEntry(
                &entries[i],
                kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
                ISSUER_KEY_IDENTIFIER_1,
                NULL,
                kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
        // Identifier is irrelevant so do not check the value
        entries[i].response.identifierIsChecked = false;
    }
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            HAPPlatformNfcAccessGetMaximumSuspendedDeviceCredentialKeys(),
            entries);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests race conditions related to notifications
 *
 * Note that the latest specification does not have any relevance in notification as to the race conditions
 * of NFC access keys modifying operations but this test remained to check out normal behaviors.
 */
TEST(TestNotificationsRaceConditions) {
    HAPPrecondition(HAPArrayCount(sessions) > 1); // This test requires at least two sessions

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Emulate a controller sending write request to add an issuer key
    NfcAccessIssuerKeyEntry entry;
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    HAPError err = SendIssuerKeyWriteRequest(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, &entry);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Emulate the same controller losing connections before write response could be read
    DisconnectController(&accessoryServer, &sessions[0]);

    // Emulate another controller being attached
    ConnectController(&accessoryServer, &sessions[1]);

    // All controllers including the new one should receive notifications
    configurationState++;
    for (size_t i = 0; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Emulate a controller sending write request to add another issuer key
    HAPRawBufferZero(&entry, sizeof entry);
    SetupAddOperationIssuerKeyEntry(
            &entry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add, 1, &entry);
    configurationState++;

    // All controllers should receive notifications
    for (size_t i = 0; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests restart scenarios
 */
TEST(TestRestarts) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add an issuer key
    NfcAccessIssuerKeyEntry issuerKeyEntry;
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupAddOperationIssuerKeyEntry(
            &issuerKeyEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_Ed25519,
            ISSUER_KEY_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &issuerKeyEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added issuer key
    HAPRawBufferZero(&issuerKeyEntry, sizeof issuerKeyEntry);
    SetupListOperationIssuerKeyEntry(
            &issuerKeyEntry, ISSUER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnIssuerKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            &issuerKeyEntry);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add an active device credential key
    NfcAccessDeviceCredentialKeyEntry deviceKey;
    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupAddOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_1,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &deviceKey);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a suspended device credential key
    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupAddOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            DEVICE_CREDENTIAL_KEY_2,
            ISSUER_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            1,
            &deviceKey);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added active device credential key
    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupListOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_1,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            &deviceKey);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added suspended device credential key
    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupListOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            ISSUER_KEY_IDENTIFIER_1,
            DEVICE_CREDENTIAL_KEY_IDENTIFIER_2,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            1,
            &deviceKey);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Add a reader key
    NfcAccessReaderKeyEntry readerEntry;
    HAPRawBufferZero(&readerEntry, sizeof readerEntry);
    SetupAddOperationReaderKeyEntry(
            &readerEntry,
            kHAPCharacteristicValue_NfcAccessControlPoint_KeyType_NIST256,
            READER_KEY_1,
            READER_IDENTIFIER,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_Add,
            &readerEntry);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to verify identifier of the added reader key
    HAPRawBufferZero(&readerEntry, sizeof readerEntry);
    SetupListOperationReaderKeyEntry(
            &readerEntry, READER_KEY_IDENTIFIER_1, kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnReaderKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            &readerEntry);

    // List operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Emulate factory reset by purging key value store
    HAPError err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kNfcAccessKeyStoreDomain);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Make sure all cached lists are reloaded
    err = HAPPlatformNfcAccessCreate(platform.keyValueStore, kNfcAccessKeyStoreDomain, NULL, NULL);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // Purging key value store and re-caching lists results in configuration state reset to zero
    configurationState = 0;

    // List operation should result in all empty lists
    OperateOnIssuerKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, 0, NULL);

    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupListOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Active,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            &deviceKey);

    HAPRawBufferZero(&deviceKey, sizeof deviceKey);
    SetupListOperationDeviceCredentialKeyEntry(
            &deviceKey,
            kHAPCharacteristicValue_NfcAccessDeviceCredentialKey_State_Suspended,
            NULL,
            NULL,
            kHAPCharacteristicValue_NfcAccessResponseStatusCode_Success);
    OperateOnDeviceCredentialKey(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List,
            0,
            &deviceKey);

    OperateOnReaderKey(
            &sessions[0], &accessoryServer, kHAPCharacteristicValue_NfcAccessControlPoint_OperationType_List, NULL);

    // Verify configuration state is reset
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

static void TestSuiteSetup() {
    HAPError err;
    HAPPlatformCreate();

    // Setup key value store
    HAPPlatformKeyValueStoreKey controllerPairingID = 0;
    HAPControllerPublicKey controllerPublicKey;
    TestPrepareKeyValueStore(controllerPairingID, &controllerPublicKey);

    // Prepare accessory server storage
    InitializeIPSessions(ipSessions, ipSessionStates, HAPArrayCount(ipSessions));
    ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer },
    };

    // Initialize accessory server
    HAPAccessoryServerOptions accessoryServerOptions;
    HAPRawBufferZero(&accessoryServerOptions, sizeof accessoryServerOptions);
    accessoryServerOptions.nfcAccess.responseStorage = &nfcAccessResponseStorage;
    accessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    accessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    accessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;
    accessoryServerCallbacks = { .handleUpdatedState = HandleUpdatedAccessoryServerState };

    HAPAccessoryServerCreate(&accessoryServer, &accessoryServerOptions, &platform, &accessoryServerCallbacks, NULL);

    // Start accessory server.
    HAPAccessoryServerStart(&accessoryServer, &accessory);
    HAPPlatformClockAdvance(0);
    TEST_ASSERT_EQUAL(HAPAccessoryServerGetState(&accessoryServer), kHAPAccessoryServerState_Running);

    // Discover IP accessory server
    HAPAccessoryServerInfo serverInfo;
    HAPNetworkPort serverPort;
    err = HAPDiscoverIPAccessoryServer(HAPNonnull(platform.ip.serviceDiscovery), &serverInfo, &serverPort);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    TEST_ASSERT(!serverInfo.statusFlags.isNotPaired);

    // Create fake security sessions
    for (size_t i = 0; i < HAPArrayCount(sessions); i++) {
        TestCreateFakeSecuritySession(&sessions[i], &accessoryServer, controllerPairingID);
    }
}

static void TestSuiteTeardown() {
    accessoryServer.ip.state = kHAPIPAccessoryServerState_Idle;
    HAPAccessoryServerRelease(&accessoryServer);
}
#endif

int main(int argc, char** argv) {
#if (HAVE_NFC_ACCESS == 1)
    TEST_ASSERT_EQUAL(HAPGetCompatibilityVersion(), HAP_COMPATIBILITY_VERSION);

    TestSuiteSetup();
    int execute_result = EXECUTE_TESTS(argc, (const char**) argv);
    TestSuiteTeardown();

    return execute_result;
#else
    HAPLogInfo(&kHAPLog_Default, "This test is not enabled. Please enable HAVE_NFC_ACCESS to run this test.");
    return 0;
#endif
}
