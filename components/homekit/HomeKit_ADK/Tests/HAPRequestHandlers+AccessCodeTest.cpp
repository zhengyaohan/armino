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
#include "HAP.h"
#include "HAPPlatform+Init.h"

#include "HAP+KeyValueStoreDomains.h"
#include "Harness/HAPAccessoryServerHelper.h"
#include "Harness/HAPPlatformClockHelper.h"
#include "Harness/HAPTestController.h"
#include "Harness/IPTestHelpers.h"
#include "Harness/SecurityTestHelpers.h"
#include "Harness/TemplateDB.h"
#include "Harness/UnitTestFramework.h"

#if (HAVE_ACCESS_CODE == 1)
#include "AccessCodeHelper.h"

#define kAccessCodeKeyStoreDomain ((HAPPlatformKeyValueStoreDomain) 0x00)

static HAPAccessoryServer accessoryServer;
static HAPAccessoryServerCallbacks accessoryServerCallbacks;
static HAPIPSession ipSessions[kHAPIPSessionStorage_DefaultNumElements];
static IPSessionState ipSessionStates[HAPArrayCount(ipSessions)];
static HAPIPReadContext ipReadContexts[kAttributeCount];
static HAPIPWriteContext ipWriteContexts[kAttributeCount];
static uint8_t ipScratchBuffer[kHAPIPSession_DefaultScratchBufferSize];
static HAPIPAccessoryServerStorage ipAccessoryServerStorage;
static HAPSession sessions[3];

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
                    &accessCodeService,
                    NULL,
            },
    .callbacks = { .identify = IdentifyAccessoryHelper },
};

/**
 * Data structure for access code request
 * - The *IsSet flags are used to determine if the corresponding TLV type is set in the request
 */
typedef struct {
    /** The identifier for the access code */
    uint32_t identifier;
    bool identifierIsSet;

    /** The access code */
    const char* accessCode;
    bool accessCodeIsSet;
} AccessCodeControlRequest;

/**
 * Data structure for access code response
 * - The *IsExpected flags are used to determine if the corresponding TLV type is expected in the response
 * - The *IsChecked flags are used to determine if the value of the corresponding TLV type should be verified if the TLV
 * type is expected and exists in the response
 */
typedef struct {
    /** The identifier for the access code */
    uint32_t identifier;
    bool identifierIsExpected;
    bool identifierIsChecked;

    /** The access code */
    const char* accessCode;
    bool accessCodeIsExpected;

    /** Bitmask indicating the restrictions on the access code */
    uint32_t flags;
    bool flagsIsExpected;

    /** The status code */
    uint8_t status;
    bool statusIsExpected;
} AccessCodeControlResponse;

typedef struct {
    /** Contains data for a write request on Access Code Control Point */
    AccessCodeControlRequest request;
    /** Contains data to verify a response to Access Code Control Point */
    AccessCodeControlResponse response;
} AccessCodeEntry;

/**
 * Setup the request and expected response for a read operation
 *
 * @param[out]   entry        access code entry that contains the request and expected response
 * @param        identifier   identifier of an access code to read for the request and verify in the response
 * @param        accessCode   expected access code to be read
 * @param        status       expected status for the operation
 */
static void
        SetupReadOperationEntry(AccessCodeEntry* entry, uint32_t identifier, const char* accessCode, uint8_t status) {
    HAPPrecondition(entry);

    // For read, only Access Code Identifier is present
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;
    entry->request.accessCodeIsSet = false;

    if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist) {
        // For does not exist failure status, only Access Code Identifier and Status are populated
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCodeIsExpected = false;
        entry->response.flagsIsExpected = false;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else {
        // For successful status, all fields are populated
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    }
}

/**
 * Setup the request and expected response for an add operation
 *
 * @param[out]   entry        access code entry that contains the request and expected response
 * @param        duplicateId  for duplicate failure, expected identifier of the duplicate access code
 * @param        accessCode   access code to add for the request and verify in the response
 * @param        status       expected status for the operation
 */
static void
        SetupAddOperationEntry(AccessCodeEntry* entry, uint32_t duplicateId, const char* accessCode, uint8_t status) {
    HAPPrecondition(entry);

    // For add, only Access Code is present
    entry->request.identifierIsSet = false;
    entry->request.accessCode = accessCode;
    entry->request.accessCodeIsSet = true;

    if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success) {
        // For successful status, all fields are populated, identifier is expected but not checked
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = false;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate) {
        // For duplicate status, all fields are populated, identifier is that of the duplicate entry
        entry->response.identifier = duplicateId;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else {
        // For all other failure status, only Access Code and Status are populated
        entry->response.identifierIsExpected = false;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flagsIsExpected = false;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    }
}

/**
 * Setup the request and expected response for an update operation
 *
 * @param[out]   entry        access code entry that contains the request and expected response
 * @param        identifier   identifier of an access code to update for the request and verify in the response
 * @param        duplicateId  for duplicate failure, expected identifier of the duplicate access code
 * @param        accessCode   access code to update for the request and verify in the response
 * @param        status       expected status for the operation
 */
static void SetupUpdateOperationEntry(
        AccessCodeEntry* entry,
        uint32_t identifier,
        uint32_t duplicateId,
        const char* accessCode,
        uint8_t status) {
    HAPPrecondition(entry);

    // For update, all fields are present
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;
    entry->request.accessCode = accessCode;
    entry->request.accessCodeIsSet = true;

    if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success) {
        // For successful status, all fields are populated, identifier is that of the updated entry
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate) {
        // For duplicate status, all fields are populated, identifier is that of the duplicate entry
        entry->response.identifier = duplicateId;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else {
        // For all other failure status, only Access Code Identifier, Access Code, and Status are populated
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flagsIsExpected = false;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    }
}

/**
 * Setup the request and expected response for a remove operation
 *
 * @param[out]   entry        access code entry that contains the request and expected response
 * @param        identifier   identifier of an access code to remove for the request and verify in the response
 * @param        accessCode   expected access code to be removed
 * @param        status       expected status for the operation
 */
static void
        SetupRemoveOperationEntry(AccessCodeEntry* entry, uint32_t identifier, const char* accessCode, uint8_t status) {
    HAPPrecondition(entry);

    // For remove, only Access Code Identifier is present
    entry->request.identifier = identifier;
    entry->request.identifierIsSet = true;
    entry->request.accessCodeIsSet = false;

    if (status == kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist) {
        // For does not exist failure status, only Access Code Identifier and Status are populated
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCodeIsExpected = false;
        entry->response.flagsIsExpected = false;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    } else {
        // For successful status, all fields are populated
        entry->response.identifier = identifier;
        entry->response.identifierIsExpected = true;
        entry->response.identifierIsChecked = true;
        entry->response.accessCode = accessCode;
        entry->response.accessCodeIsExpected = true;
        entry->response.flags = 0; // no restrictions
        entry->response.flagsIsExpected = true;
        entry->response.status = status;
        entry->response.statusIsExpected = true;
    }
}

/**
 * Calculate the number of TLV types/fields are expected for a response
 */
static uint8_t CalculateNumExpectedTLVs(const AccessCodeControlResponse* response) {
    HAPPrecondition(response);

    uint8_t numExpectedTLVs = 0;

    if (response->identifierIsExpected) {
        numExpectedTLVs++;
    }
    if (response->accessCodeIsExpected) {
        numExpectedTLVs++;
    }
    if (response->flagsIsExpected) {
        numExpectedTLVs++;
    }
    if (response->statusIsExpected) {
        numExpectedTLVs++;
    }

    return numExpectedTLVs;
}

/**
 * Emulate controller sending a write request to operate on for one or more access codes
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   numEntries   number of access codes to include in the write request
 * @param   entries      array of access code data for the write request
 */
static void SendWriteRequestMultipleAccessCodes(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        size_t numEntries,
        const AccessCodeEntry* entries) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(numEntries == 0 || entries);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Writing Access Code Control Point for operation %d", operation);
    {
        const HAPTLV8CharacteristicWriteRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &accessCodeControlPointCharacteristic,
            .service = &accessCodeService,
            .accessory = &accessory,
        };

        uint8_t requestBuffer[1024];
        HAPTLVWriter requestWriter;
        HAPTLVWriterCreate(&requestWriter, requestBuffer, sizeof requestBuffer);
        uint8_t operationType[] = { operation };
        HAPTLV tlv = {
            .type = kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType,
            .value = { .bytes = operationType, .numBytes = sizeof operationType },
        };
        err = HAPTLVWriterAppend(&requestWriter, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);

        for (size_t i = 0; i < numEntries; i++) {
            void* controlPointRequestBytes;
            size_t numControlPointRequestBytes;
            uint8_t controlPointRequestBuffer[256];
            {
                HAPTLVWriter controlPointRequestWriter;
                HAPTLVWriterCreate(
                        &controlPointRequestWriter, controlPointRequestBuffer, sizeof controlPointRequestBuffer);

                if (entries[i].request.identifierIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_AccessCodeControlPointRequest_Identifier,
                        .value = { .bytes = &entries[i].request.identifier,
                                   .numBytes = sizeof entries[i].request.identifier },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                if (entries[i].request.accessCodeIsSet) {
                    tlv = {
                        .type = kHAPCharacteristicTLVType_AccessCodeControlPointRequest_AccessCode,
                        .value = { .bytes = entries[i].request.accessCode,
                                   .numBytes = HAPStringGetNumBytes(entries[i].request.accessCode) },
                    };
                    err = HAPTLVWriterAppend(&controlPointRequestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }

                HAPTLVWriterGetBuffer(
                        &controlPointRequestWriter, &controlPointRequestBytes, &numControlPointRequestBytes);

                tlv = {
                    .type = kHAPCharacteristicTLVType_AccessCodeControlPoint_Request,
                    .value = { .bytes = controlPointRequestBytes, .numBytes = numControlPointRequestBytes },
                };
                err = HAPTLVWriterAppend(&requestWriter, &tlv);
                TEST_ASSERT_EQUAL(err, kHAPError_None);

                if (i + 1 < numEntries) {
                    // Add TLV separator
                    tlv = { .type = 0, .value = { .bytes = NULL, .numBytes = 0 } };
                    err = HAPTLVWriterAppend(&requestWriter, &tlv);
                    TEST_ASSERT_EQUAL(err, kHAPError_None);
                }
            }
        }

        void* requestBytes;
        size_t numRequestBytes;
        HAPTLVWriterGetBuffer(&requestWriter, &requestBytes, &numRequestBytes);

        HAPTLVReader requestReader;
        HAPTLVReaderCreate(&requestReader, requestBytes, numRequestBytes);

        err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

/**
 * Verify the write response is as expected for operation on one or more access codes
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   numEntries   expected number of access codes to verify
 * @param   entries      array of expected values
 */
static void VerifyWriteResponse(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        size_t numEntries,
        const AccessCodeEntry* entries) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(numEntries == 0 || entries);

    HAPError err;

    HAPLog(&kHAPLog_Default, "Reading Access Code Control Point");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &accessCodeControlPointCharacteristic,
            .service = &accessCodeService,
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
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT(valid);
        TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType);
        TEST_ASSERT_EQUAL(HAPReadUInt8(tlv.value.bytes), operation);

        uint8_t numFoundResponses = 0;

        // Read each Access Code Control Point Response
        for (;;) {
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_AccessCodeControlPoint_Response);
            TEST_ASSERT(numFoundResponses < numEntries);

            HAPTLVReader nestedReader;
            HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);

            uint8_t numExpectedTLVs = CalculateNumExpectedTLVs(&entries[numFoundResponses].response);
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
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Identifier: {
                        if (entries[numFoundResponses].response.identifierIsExpected) {
                            TEST_ASSERT_EQUAL(
                                    nestedTLV.value.numBytes, sizeof entries[numFoundResponses].response.identifier);
                            if (entries[numFoundResponses].response.identifierIsChecked) {
                                TEST_ASSERT_EQUAL(
                                        HAPReadLittleUInt32(nestedTLV.value.bytes),
                                        entries[numFoundResponses].response.identifier);
                            }
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_AccessCode: {
                        if (entries[numFoundResponses].response.accessCodeIsExpected) {
                            TEST_ASSERT_EQUAL(
                                    nestedTLV.value.numBytes,
                                    HAPStringGetNumBytes(entries[numFoundResponses].response.accessCode));
                            TEST_ASSERT(HAPRawBufferAreEqual(
                                    nestedTLV.value.bytes,
                                    entries[numFoundResponses].response.accessCode,
                                    nestedTLV.value.numBytes));
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Flags: {
                        if (entries[numFoundResponses].response.flagsIsExpected) {
                            TEST_ASSERT_EQUAL(
                                    nestedTLV.value.numBytes, sizeof entries[numFoundResponses].response.flags);
                            TEST_ASSERT_EQUAL(
                                    HAPReadLittleUInt32(nestedTLV.value.bytes),
                                    entries[numFoundResponses].response.flags);
                        } else {
                            // TLV type not expected
                            TEST_ASSERT(false);
                        }
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Status: {
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

    HAPLog(&kHAPLog_Default, "Reading Access Code Configuration State");
    {
        const HAPUInt16CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &accessCodeConfigurationStateCharacteristic,
            .service = &accessCodeService,
            .accessory = &accessory,
        };
        err = request.characteristic->callbacks.handleRead(server, &request, configurationState, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
    }
}

/**
 * Emulate controller listing all access codes. The identifiers from the list operation are returned.
 *
 * @param        session       session
 * @param        server        accessory server
 * @param        numEntries    expected number of entries
 * @param[out]   identifiers   array storage for identifiers listed
 */
static void ListExpectingSuccess(
        HAPSession* session,
        HAPAccessoryServer* server,
        size_t numEntries,
        uint32_t* identifiers) {
    HAPPrecondition(session);
    HAPPrecondition(server);
    HAPPrecondition(numEntries == 0 || identifiers);

    SendWriteRequestMultipleAccessCodes(
            session, server, kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List, 0, NULL);

    HAPLog(&kHAPLog_Default, "Reading Access Code Control Point");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = session,
            .characteristic = &accessCodeControlPointCharacteristic,
            .service = &accessCodeService,
            .accessory = &accessory,
        };
        HAPError err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        HAPTLV tlv;
        bool valid;
        err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        TEST_ASSERT(valid);
        TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType);
        TEST_ASSERT_EQUAL(
                HAPReadUInt8(tlv.value.bytes), kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_List);

        uint8_t numFoundResponses = 0;

        // Read each Access Code Control Point Response
        for (;;) {
            err = HAPTLVReaderGetNext(&responseReader, &valid, &tlv);
            TEST_ASSERT_EQUAL(err, kHAPError_None);
            if (!valid) {
                break;
            }

            TEST_ASSERT_EQUAL(tlv.type, kHAPCharacteristicTLVType_AccessCodeControlPoint_Response);
            TEST_ASSERT(numFoundResponses < numEntries);

            HAPTLVReader nestedReader;
            HAPTLVReaderCreate(&nestedReader, (void*) tlv.value.bytes, tlv.value.numBytes);

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
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Identifier: {
                        TEST_ASSERT_EQUAL(nestedTLV.value.numBytes, sizeof identifiers[numFoundResponses]);
                        identifiers[numFoundResponses] = HAPReadLittleUInt32(nestedTLV.value.bytes);
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_AccessCode: {
                        // TLV type not expected
                        TEST_ASSERT(false);
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Flags: {
                        // TLV type not expected
                        TEST_ASSERT(false);
                        break;
                    }
                    case kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Status: {
                        // TLV type not expected
                        TEST_ASSERT(false);
                        break;
                    }
                    default:
                        // Unknown TLV type
                        TEST_ASSERT(false);
                        break;
                }

                numFoundTLVs++;
            }

            TEST_ASSERT_EQUAL(numFoundTLVs, 1);

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
 * Emulate controller operating on multiple access codes
 *
 * @param   session      session
 * @param   server       accessory server
 * @param   operation    operation type
 * @param   numEntries   number of access codes to operate on and verify
 * @param   entries      array of access codes to operate on and verify
 */
static void OperateOnMultipleAccessCodes(
        HAPSession* session,
        HAPAccessoryServer* server,
        uint8_t operation,
        size_t numEntries,
        const AccessCodeEntry* entries) {
    SendWriteRequestMultipleAccessCodes(session, server, operation, numEntries, entries);
    VerifyWriteResponse(session, server, operation, numEntries, entries);
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

    HAPHandleAccessCodeSessionInvalidate(server, session);
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
 * Tests Access Code Supported Configuration read handler
 */
TEST(TestAccessCodeSupportedConfiguration) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    HAPLog(&kHAPLog_Default, "Reading Access Code Supported Configuration.");
    {
        uint8_t bytes[1024];
        HAPTLVWriter responseWriter;
        HAPTLVWriterCreate(&responseWriter, bytes, sizeof bytes);
        const HAPTLV8CharacteristicReadRequest request = {
            .transportType = kHAPTransportType_IP,
            .session = &sessions[0],
            .characteristic = &accessCodeSupportedConfigurationCharacteristic,
            .service = &accessCodeService,
            .accessory = &accessory,
        };
        HAPError err = request.characteristic->callbacks.handleRead(&accessoryServer, &request, &responseWriter, NULL);
        TEST_ASSERT_EQUAL(err, kHAPError_None);
        void* responseBytes;
        size_t numResponseBytes;
        HAPTLVWriterGetBuffer(&responseWriter, &responseBytes, &numResponseBytes);
        HAPTLVReader responseReader;
        HAPTLVReaderCreate(&responseReader, responseBytes, numResponseBytes);

        bool foundTLVs[4];
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
                case kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_CharacterSet: {
                    uint8_t value = HAPReadUInt8(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint8_t));
                    TEST_ASSERT_EQUAL(value, kHAPCharacteristicValue_AccessCode_CharacterSet_ArabicNumerals);
                    break;
                }
                case kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MinimumLength: {
                    uint8_t value = HAPReadUInt8(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint8_t));
                    TEST_ASSERT_EQUAL(value, kAccessCodeMinimumLength);
                    break;
                }
                case kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumLength: {
                    uint8_t value = HAPReadUInt8(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint8_t));
                    TEST_ASSERT_EQUAL(value, kAccessCodeMaximumLength);
                    break;
                }
                case kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumAccessCodes: {
                    uint16_t value = HAPReadLittleUInt16(tlv.value.bytes);
                    TEST_ASSERT_EQUAL(tlv.value.numBytes, sizeof(uint16_t));
                    TEST_ASSERT_EQUAL(value, kAccessCodeListSize);
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
 * Tests controller performing all operations for one access code, expecting success
 * - Add one access code
 * - List to verify there is only one identifier retrieved
 * - Read for the identifier to verify the access code matches the added access code
 * - Update the access code using identifier retrieved
 * - Read for the identifier to verify the access code matches the updated value
 * - Remove the access code using identifier retrieved
 * - List to verify there are no identifiers retrieved
 */
TEST(TestOperationsOneSuccessful) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Add one access code
    AccessCodeEntry entries[1];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0], 0, "1234567", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifier of the added access code
    uint32_t identifiers[1];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Read to verify the access code for identifier retrieved matches the access code that was added
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0],
            identifiers[0],
            "1234567",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            HAPArrayCount(entries),
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Update access code for the identifier
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[0],
            0,
            "98765",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            HAPArrayCount(entries),
            entries);

    // Update operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Read to verify the access code has been updated
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0], identifiers[0], "98765", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            HAPArrayCount(entries),
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove access code
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationEntry(
            &entries[0], identifiers[0], "98765", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            HAPArrayCount(entries),
            entries);

    // Verify there are no identifiers
    ListExpectingSuccess(&sessions[0], &accessoryServer, 0, NULL);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing all operations for multiple access codes, expecting success
 * - Add multiple access codes
 * - List to verify there are correct number of identifiers retrieved
 * - Read for all identifiers to verify the access codes match the added access codes
 * - Update some access codes using identifiers retrieved
 * - Read for the identifiers to verify the access codes match the updated values
 * - Remove some access codes using identifiers retrieved
 * - List to verify there are correct number of identifiers left
 */
TEST(TestOperationsBulkSuccessful) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Bulk add multiple access codes
    const char* accessCodes[] = { "11111111", "22222222", "33333333", "44444444", "55555555" };
    AccessCodeEntry entries[5];
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPArrayCount(entries); i++) {
        SetupAddOperationEntry(
                &entries[i], 0, accessCodes[i], kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Bulk add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the added access codes
    uint32_t identifiers[5];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Read to verify the access codes for identifiers retrieved match the access codes that were added
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPArrayCount(entries); i++) {
        SetupReadOperationEntry(
                &entries[i],
                identifiers[i],
                accessCodes[i],
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            HAPArrayCount(entries),
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Bulk update access codes for some identifiers
    HAPRawBufferZero(&entries, sizeof entries);
    accessCodes[1] = "1234";
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            0,
            accessCodes[1],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    accessCodes[4] = "5678";
    SetupUpdateOperationEntry(
            &entries[1],
            identifiers[4],
            0,
            accessCodes[4],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            2,
            entries);

    // Update operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Read to verify the access codes have been updated
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPArrayCount(entries); i++) {
        SetupReadOperationEntry(
                &entries[i],
                identifiers[i],
                accessCodes[i],
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            HAPArrayCount(entries),
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Bulk remove some access codes
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationEntry(
            &entries[0],
            identifiers[0],
            accessCodes[0],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[1],
            identifiers[2],
            accessCodes[2],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[2],
            identifiers[4],
            accessCodes[4],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            3,
            entries);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Verify the expected identifiers are left
    uint32_t identifiersAfterRemove[2];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiersAfterRemove), identifiersAfterRemove);
    TEST_ASSERT_EQUAL(identifiersAfterRemove[0], identifiers[1]);
    TEST_ASSERT_EQUAL(identifiersAfterRemove[1], identifiers[3]);
}

/**
 * Tests controller performing various failing read operations
 */
TEST(TestReadFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    AccessCodeEntry entries[2];
    SetupAddOperationEntry(
            &entries[0], 0, "376616", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifier of the added access code
    uint32_t identifiers[1];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Remove the only access code added
    SetupRemoveOperationEntry(
            &entries[0],
            identifiers[0],
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            1,
            entries);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Read an already removed entry
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0],
            identifiers[0],
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            1,
            entries);

    // Read a non-existent entry
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0],
            identifiers[0] + 1,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            1,
            entries);

    // Read multiple non-existent entries
    SetupReadOperationEntry(
            &entries[0],
            identifiers[0] + 8,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[1],
            identifiers[0] + 6,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            HAPArrayCount(entries),
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing add operations
 */
TEST(TestAddFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    const char* accessCodes[] = { "1111", "2222", "3333", "4444", "5555", "6666", "7777", "8888" };
    AccessCodeEntry entries[kAccessCodeListSize];
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPArrayCount(entries); i++) {
        SetupAddOperationEntry(
                &entries[i], 0, accessCodes[i], kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the added access codes
    uint32_t identifiers[kAccessCodeListSize];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Duplicate
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0],
            identifiers[6],
            accessCodes[6],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Exceeded maximum allowed access codes
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0],
            0,
            "9999",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Access code too short
    char tooShortAccessCode[kAccessCodeMinimumLength];
    for (size_t i = 0; i < kAccessCodeMinimumLength - 1; i++) {
        tooShortAccessCode[i] = '0';
    }
    tooShortAccessCode[kAccessCodeMinimumLength - 1] = 0;
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0],
            0,
            tooShortAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Access code too long
    char tooLongAccessCode[kAccessCodeMaximumLength + 2];
    for (size_t i = 0; i < kAccessCodeMaximumLength + 1; i++) {
        tooLongAccessCode[i] = '0';
    }
    tooLongAccessCode[kAccessCodeMaximumLength + 1] = 0;
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0],
            0,
            tooLongAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Invalid character
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0],
            0,
            "1234e",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing update operations
 */
TEST(TestUpdateFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    const char* accessCodes[] = { "12345678", "87654321", "1111" };
    AccessCodeEntry entries[3];
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < HAPArrayCount(entries); i++) {
        SetupAddOperationEntry(
                &entries[i], 0, accessCodes[i], kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the added access codes
    uint32_t identifiers[3];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Duplicate
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            identifiers[2],
            accessCodes[2],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);

    // Access code too short
    char tooShortAccessCode[kAccessCodeMinimumLength];
    for (size_t i = 0; i < kAccessCodeMinimumLength - 1; i++) {
        tooShortAccessCode[i] = '0';
    }
    tooShortAccessCode[kAccessCodeMinimumLength - 1] = 0;
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            0,
            tooShortAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);

    // Access code too long
    char tooLongAccessCode[kAccessCodeMaximumLength + 2];
    for (size_t i = 0; i < kAccessCodeMaximumLength + 1; i++) {
        tooLongAccessCode[i] = '0';
    }
    tooLongAccessCode[kAccessCodeMaximumLength + 1] = 0;
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            0,
            tooLongAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);

    // Invalid character
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            0,
            "ffff",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);

    // Does not exist
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[2] + 65500,
            0,
            "1234",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing various failing remove operations
 */
TEST(TestRemoveFailure) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    AccessCodeEntry entries[2];
    SetupAddOperationEntry(
            &entries[0], 0, "376616", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            1,
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifier of the added access code
    uint32_t identifiers[1];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Remove the only access code added
    SetupRemoveOperationEntry(
            &entries[0],
            identifiers[0],
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            1,
            entries);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Remove an already removed entry
    SetupRemoveOperationEntry(
            &entries[0],
            identifiers[0],
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            1,
            entries);

    // Remove a non-existent entry
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationEntry(
            &entries[0],
            identifiers[0] + 1,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            1,
            entries);

    // Remove multiple non-existent entries
    SetupReadOperationEntry(
            &entries[0],
            identifiers[0] + 16,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[1],
            identifiers[0] + 3,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            HAPArrayCount(entries),
            entries);

    // All operations have failed so configuration state value should not have changed
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
}

/**
 * Tests controller performing all operations for multiple access codes, expecting mixed responses
 * - Add mixture of valid and invalid access codes
 * - List to verify there are correct number of identifiers retrieved for only the valid access codes
 * - Read for all identifiers to verify the access codes match the successfully added access codes
 * - Update with a mixture of valid and invalid access codes for some identifiers retrieved
 * - Read for the identifiers to verify the access codes match the updated values
 * - Remove some access codes using identifiers retrieved
 * - List to verify there are correct number of identifiers left
 */
TEST(TestOperationsBulkMixed) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    // Bulk add a mixture of valid and invalid access codes
    char tooShortAccessCode[kAccessCodeMinimumLength];
    for (size_t i = 0; i < kAccessCodeMinimumLength - 1; i++) {
        tooShortAccessCode[i] = '0';
    }
    tooShortAccessCode[kAccessCodeMinimumLength - 1] = 0;
    char tooLongAccessCode[kAccessCodeMaximumLength + 2];
    for (size_t i = 0; i < kAccessCodeMaximumLength + 1; i++) {
        tooLongAccessCode[i] = '0';
    }
    tooLongAccessCode[kAccessCodeMaximumLength + 1] = 0;
    AccessCodeEntry entries[13];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0], 0, "1111", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[1],
            0,
            tooShortAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength);
    SetupAddOperationEntry(
            &entries[2], 0, "22222", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[3], 0, "333333", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[4], 0, "4444444", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[5],
            0,
            tooLongAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength);
    SetupAddOperationEntry(
            &entries[6], 0, "55555555", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[7],
            0,
            "135T9",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter);
    SetupAddOperationEntry(
            &entries[8], 0, "6666666", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[9], 0, "777777", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[10], 1, "22222", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate);
    SetupAddOperationEntry(
            &entries[11], 0, "88888", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[12],
            0,
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Bulk add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the successfully added access codes
    uint32_t identifiers[8];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Read to verify the access codes for identifiers retrieved match the access codes that were added successfully and
    // attempt to read non-existent entries fails
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0],
            identifiers[7] + 1,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[1], identifiers[0], "1111", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[2], identifiers[1], "22222", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[3],
            identifiers[2],
            "333333",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[4],
            identifiers[7] + 12,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[5],
            identifiers[7] + 3,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[6],
            identifiers[3],
            "4444444",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[7],
            identifiers[4],
            "55555555",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[8],
            identifiers[5],
            "6666666",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[9],
            identifiers[7] + 5,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[10],
            identifiers[6],
            "777777",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[11],
            identifiers[7],
            "88888",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            12,
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Bulk update a mixture of valid and invalid access codes for some identifiers
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[1],
            0,
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupUpdateOperationEntry(
            &entries[1],
            identifiers[5],
            identifiers[7],
            "88888",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDuplicate);
    SetupUpdateOperationEntry(
            &entries[2],
            identifiers[6],
            0,
            tooShortAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorSmallerThanMinimumLength);
    SetupUpdateOperationEntry(
            &entries[3],
            identifiers[0],
            0,
            "1359",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupUpdateOperationEntry(
            &entries[4],
            identifiers[7],
            0,
            tooLongAccessCode,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorLargerThanMaximumLength);
    SetupUpdateOperationEntry(
            &entries[5],
            identifiers[2],
            0,
            "h3110",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorInvalidCharacter);
    SetupUpdateOperationEntry(
            &entries[6],
            identifiers[7] + 8,
            0,
            "3110",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupUpdateOperationEntry(
            &entries[7],
            identifiers[3],
            0,
            "3110",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            8,
            entries);

    // Update operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Read to verify some access codes have been updated and attempt to read non-existent entries fails
    HAPRawBufferZero(&entries, sizeof entries);
    SetupReadOperationEntry(
            &entries[0], identifiers[0], "1359", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[1],
            identifiers[1],
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[2],
            identifiers[2],
            "333333",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[3],
            identifiers[7] + 12,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[4], identifiers[3], "3110", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[5],
            identifiers[4],
            "55555555",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[6],
            identifiers[7] + 2,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupReadOperationEntry(
            &entries[7],
            identifiers[5],
            "6666666",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[8],
            identifiers[6],
            "777777",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[9], identifiers[7], "88888", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupReadOperationEntry(
            &entries[10],
            identifiers[7] + 8,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            11,
            entries);

    // Read operation does not result in an increment to the configuration state value
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Bulk remove a mixture of valid and invalid identifiers
    HAPRawBufferZero(&entries, sizeof entries);
    SetupRemoveOperationEntry(
            &entries[0], identifiers[0], "1359", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[1],
            identifiers[7] + 6,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    SetupRemoveOperationEntry(
            &entries[2],
            identifiers[5],
            "6666666",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[3], identifiers[7], "88888", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[4],
            identifiers[2],
            "333333",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[5],
            identifiers[6],
            "777777",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupRemoveOperationEntry(
            &entries[6],
            identifiers[7] + 20,
            NULL,
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ErrorDoesNotExist);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Remove,
            7,
            entries);

    // Remove operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }
    // Verify the expected identifiers are left
    uint32_t identifiersAfterRemove[3];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiersAfterRemove), identifiersAfterRemove);
    TEST_ASSERT_EQUAL(identifiersAfterRemove[0], identifiers[1]);
    TEST_ASSERT_EQUAL(identifiersAfterRemove[1], identifiers[3]);
    TEST_ASSERT_EQUAL(identifiersAfterRemove[2], identifiers[4]);
}

/**
 * Tests controller bulk add more than the maximum allowed access codes, each with the maximum length allowed. The
 * expectation is that a success response is returned for each access code with the exception of the first response
 * after maximum is reached. This last response should be an error response indicating maximum allowed has been reached.
 *
 * For example:
 * - Maximum allowed access code is 8
 * - Bulk add 10 access codes
 * - Expected result is receiving 9 responses, with the 9th response indicating maximum allowed reached
 */
TEST(TestMaxResponseSize) {
    HAPPrecondition(HAPArrayCount(sessions) > 0);

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    char accessCodes[kAccessCodeListSize + 2][kAccessCodeMaximumLength + 1];
    HAPRawBufferZero(&accessCodes, sizeof accessCodes);
    for (size_t i = 0; i < kAccessCodeListSize + 2; i++) {
        for (size_t j = 0; j < kAccessCodeMaximumLength; j++) {
            accessCodes[i][j] = '0' + i;
        }
    }

    // Bulk add maximum allowed, each with maximum length
    AccessCodeEntry entries[kAccessCodeListSize + 2];
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < kAccessCodeListSize; i++) {
        SetupAddOperationEntry(
                &entries[i], 0, accessCodes[i], kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    SetupAddOperationEntry(
            &entries[kAccessCodeListSize],
            0,
            accessCodes[kAccessCodeListSize],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes);
    SetupAddOperationEntry(
            &entries[kAccessCodeListSize + 1],
            0,
            accessCodes[kAccessCodeListSize + 1],
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_ExceededMaximumAllowedAccessCodes);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            kAccessCodeListSize + 1,
            entries);

    // Bulk add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the added access codes
    uint32_t identifiers[8];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Read to verify the access codes for identifiers retrieved match the access codes that were added
    HAPRawBufferZero(&entries, sizeof entries);
    for (size_t i = 0; i < kAccessCodeListSize; i++) {
        SetupReadOperationEntry(
                &entries[i],
                identifiers[i],
                accessCodes[i],
                kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    }
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Read,
            kAccessCodeListSize,
            entries);
}

/**
 * Tests race conditions related to notifications
 *
 * Note that the latest specification does not have any relevance in notification as to the race conditions
 * of access code modifying operations but this test remained to check out normal behaviors.
 */
TEST(TestNotificationsRaceConditions) {
    HAPPrecondition(HAPArrayCount(sessions) > 1); // This test requires at least two sessions

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    AccessCodeEntry entries[2];
    SetupAddOperationEntry(
            &entries[0], 0, "1234567", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[1], 0, "9876543", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Perform list operation to retrieve the identifiers of the added access codes
    uint32_t identifiers[2];
    ListExpectingSuccess(&sessions[0], &accessoryServer, HAPArrayCount(identifiers), identifiers);

    // Emulate a controller sending write request to update access codes
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[0],
            0,
            "0123456",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupUpdateOperationEntry(
            &entries[1],
            identifiers[1],
            0,
            "3110",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SendWriteRequestMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            HAPArrayCount(entries),
            entries);

    // Emulate the same controller losing connections before write response could be read
    DisconnectController(&accessoryServer, &sessions[0]);

    // Emulate another controller being attached
    ConnectController(&accessoryServer, &sessions[0]);

    // All controllers including the new one should receive notifications
    configurationState++;
    for (size_t i = 0; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Emulate a controller sending write request to update an access code
    HAPRawBufferZero(&entries, sizeof entries);
    SetupUpdateOperationEntry(
            &entries[0],
            identifiers[0],
            0,
            "0023456",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            entries);
    configurationState++;

    // Emulate another controller sending write request to update a different access code
    SetupUpdateOperationEntry(
            &entries[1],
            identifiers[1],
            0,
            "376616",
            kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[1],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Update,
            1,
            &entries[1]);
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

    // Clear access code list
    HAPLog(&kHAPLog_Default, "Clearing Access Codes");
    HAPError err = AccessCodeClear();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    uint16_t configurationState;
    ReadConfigurationState(&sessions[0], &accessoryServer, &configurationState);

    AccessCodeEntry entries[2];
    HAPRawBufferZero(&entries, sizeof entries);
    SetupAddOperationEntry(
            &entries[0], 0, "1234567", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    SetupAddOperationEntry(
            &entries[1], 0, "9876543", kHAPCharacteristicValue_AccessCodeControlPointResponseStatus_Success);
    OperateOnMultipleAccessCodes(
            &sessions[0],
            &accessoryServer,
            kHAPCharacteristicValue_AccessCodeControlPoint_Operation_Type_Add,
            HAPArrayCount(entries),
            entries);

    // Add operation results in an increment to the configuration state value
    configurationState++;
    for (size_t i = 1; i < HAPArrayCount(sessions); i++) {
        ExpectNotification(&sessions[i], &accessoryServer, configurationState);
    }

    // Emulate factory reset by purging key value store
    err = HAPPlatformKeyValueStorePurgeDomain(platform.keyValueStore, kAccessCodeKeyStoreDomain);
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    err = AccessCodeRestart();
    TEST_ASSERT_EQUAL(err, kHAPError_None);

    // List operation should result in empty list
    ListExpectingSuccess(&sessions[0], &accessoryServer, 0, NULL);
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
    err = AccessCodeCreate(platform.keyValueStore, kAccessCodeKeyStoreDomain, NULL, &accessoryServerOptions);
    TEST_ASSERT_EQUAL(err, kHAPError_None);
    accessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    accessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    accessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;
    accessoryServerCallbacks = { .handleUpdatedState = HAPAccessoryServerHelperHandleUpdatedAccessoryServerState };

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
#if (HAVE_ACCESS_CODE == 1)
    TEST_ASSERT_EQUAL(HAPGetCompatibilityVersion(), HAP_COMPATIBILITY_VERSION);

    TestSuiteSetup();
    int execute_result = EXECUTE_TESTS(argc, (const char**) argv);
    TestSuiteTeardown();

    return execute_result;
#else
    HAPLogInfo(&kHAPLog_Default, "This test is not enabled. Please enable HAVE_ACCESS_CODE to run this test.");
    return 0;
#endif
}
