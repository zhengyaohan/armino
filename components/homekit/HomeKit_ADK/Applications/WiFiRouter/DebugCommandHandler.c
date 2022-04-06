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

// This debug command line serves development purposes only.
// It must not be included in production accessories and can be safely compiled out for production builds once no longer
// required for testing.

#include "HAP.h"
HAP_DIAGNOSTIC_PUSH
HAP_DIAGNOSTIC_IGNORED_GCC("-Wpedantic") // ISO C forbids an empty translation unit

#if (HAP_TESTING == 1)
#include "HAPPlatformWiFiRouter+Init.h"

#include "App.h"
#include "DB.h"
#include "DebugCommandHandler.h"

DebugCommandLineContext debugCommandLineContext;

static void PrintUsage(void) {
    HAPLogInfo(
            &kHAPLog_Default,
            "Usage (Legend: [] optional argument, ... repeatable argument):\n"
            "ownership-proof-token\n"
            "    generate\n"
            "    disabled\n"
            "    enabled\n" kHAPCharacteristicDebugDescription_NetworkClientProfileControl
            "\n"
            "    list\n"
            "    next-identifier <networkClientIdentifier : decimal number>\n"
            "    add --group <groupIdentifier : decimal number>\n"
            "        [--credential mac <macAddress : AA:BB:CC:DD:EE:FF>]\n"
            "        [--credential psk <pskCredential : passphrase (8-63 bytes string) / psk (64 hex digits)>]\n"
            "    update <networkClientIdentifier : decimal number>\n"
            "        [--group <groupIdentifier : decimal number>\n"
            "        [--credential mac <macAddress : AA:BB:CC:DD:EE:FF>]\n"
            "        [--credential psk <pskCredential : passphrase (8-63 bytes string) / psk (64 hex digits)>]\n"
            "    remove <networkClientIdentifier : decimal "
            "number>\n" kHAPCharacteristicDebugDescription_NetworkClientStatusControl
            "\n"
            "    list\n"
            "    connect <clientMAC : AA:BB:CC:DD:EE:FF>\n"
            "        [--credential <pskCredential : passphrase (8-63 bytes string) / psk (64 hex digits)>]\n"
            "        [--name <clientName : UTF-8 string>]\n"
            "    disconnect <clientMAC : AA:BB:CC:DD:EE:FF>\n"
            "    add [--client <networkClientIdentifier : decimal number>]\n"
            "        --mac <macAddress : AA:BB:CC:DD:EE:FF>\n"
            "        [--ip <ipAddress : xxx.xxx.xxx.xxx / XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX>...]\n"
            "        --group <groupIdentifier : decimal number>\n"
            "        [--name <clientName : UTF-8 string>]\n" kHAPCharacteristicDebugDescription_RouterStatus
            "\n"
            "    ready\n"
            "    not-ready\n" kHAPCharacteristicDebugDescription_WANConfigurationList
            "\n"
            "    unconfigured <wanIdentifier : decimal number>\n"
            "    other <wanIdentifier : decimal number>\n"
            "    dhcp <wanIdentifier : decimal number>\n"
            "    bridge <wanIdentifier : decimal number>\n" kHAPCharacteristicDebugDescription_WANStatusList
            "\n"
            "    update <wanIdentifier : decimal number> <wanStatus : decimal "
            "number>\n" kHAPCharacteristicDebugDescription_ManagedNetworkEnable
            "\n"
            "    disabled\n"
            "    enabled\n" kHAPCharacteristicDebugDescription_NetworkAccessViolationControl
            "\n"
            "    add <networkClientIdentifier : decimal number> <timestamp : decimal "
            "number>\n" kHAPCharacteristicDebugDescription_WiFiSatelliteStatus
            "\n"
            "    unknown <satelliteIndex : decimal number>\n"
            "    connected <satelliteIndex : decimal number>\n"
            "    not-connected <satelliteIndex : decimal number>");
}

HAP_RESULT_USE_CHECK
HAPError ProcessCommandLine(
        HAPAccessoryServer* server,
        HAPSession* session,
        const HAPAccessory* accessory,
        int argc,
        char** argv) {
    HAPPrecondition(argv);

    HAPError err;

    // Command
    const char* command = argv[0];
    if (HAPStringAreEqual(command, "ownership-proof-token")) {
        // Operation.
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        bool tokenRequired = false;
        if (HAPStringAreEqual(operation, "generate")) {
            GenerateOwnershipProofToken();
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "disabled")) {
            tokenRequired = false;
        } else if (HAPStringAreEqual(operation, "enabled")) {
            tokenRequired = true;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
        for (int i = 2; i < argc;) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
            return kHAPError_InvalidData;
        }

        // Perform operation.
        HAPAccessoryServerSetOwnershipProofTokenRequired(server, tokenRequired);
        return kHAPError_None;
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_NetworkClientProfileControl)) {
        // Operation.
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];
        if (HAPStringAreEqual(operation, "list")) {
            for (int i = 2; i < argc;) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                return kHAPError_InvalidData;
            }

            HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;

            int errCode;

            sqlite3_stmt* stmt;
            errCode = sqlite3_prepare_v2(
                    wiFiRouter->db,
                    "SELECT\n"
                    "    `id`, -- 0\n"
                    "    `group`, -- 1\n"
                    "    `credential_mac`, -- 2\n"
                    "    `credential_passphrase`, -- 3\n"
                    "    `credential_psk` -- 4\n"
                    "FROM `hap_clients`\n"
                    "LEFT JOIN `hap_client_groups`\n"
                    "    ON `hap_clients`.`id` = `hap_client_groups`.`client`\n"
                    "ORDER BY `id`",
                    /* nByte: */ -1,
                    &stmt,
                    /* pzTail: */ NULL);
            if (errCode) {
                return kHAPError_Unknown;
            }

            bool isValid = true;
            {
                bool added = false;
                while ((errCode = sqlite3_step(stmt)) == SQLITE_ROW) {
                    sqlite3_int64 clientIdentifier = sqlite3_column_int64(stmt, /* iCol: */ 0);
                    sqlite3_int64 groupIdentifier = sqlite3_column_int64(stmt, /* iCol: */ 1);

                    bool hasCredentialMAC = sqlite3_column_type(stmt, /* iCol: */ 2) != SQLITE_NULL;
                    const void* _Nullable credentialMAC = sqlite3_column_blob(stmt, /* iCol: */ 2);
                    int numCredentialMACBytes = sqlite3_column_bytes(stmt, /* iCol: */ 2);

                    bool hasCredentialPassphrase = sqlite3_column_type(stmt, /* iCol: */ 3) != SQLITE_NULL;
                    const unsigned char* _Nullable credentialPassphrase = sqlite3_column_text(stmt, /* iCol: */ 3);
                    int numCredentialPassphraseBytes HAP_UNUSED = sqlite3_column_bytes(stmt, /* iCol: */ 3);

                    bool hasCredentialPSK = sqlite3_column_type(stmt, /* iCol: */ 4) != SQLITE_NULL;
                    const void* _Nullable credentialPSK = sqlite3_column_blob(stmt, /* iCol: */ 4);
                    int numCredentialPSKBytes = sqlite3_column_bytes(stmt, /* iCol: */ 4);

                    if ((hasCredentialMAC && !credentialMAC) || (hasCredentialPassphrase && !credentialPassphrase) ||
                        (hasCredentialPSK && !credentialPSK)) {
                        isValid = false;
                        break;
                    }

                    HAPLog(&kHAPLog_Default,
                           "--------------------------------------------------------------------------------");
                    HAPLog(&kHAPLog_Default,
                           "%lu: Group %lu",
                           (unsigned long) clientIdentifier,
                           (unsigned long) groupIdentifier);

                    if (credentialMAC) {
                        if (numCredentialMACBytes != 6) {
                            isValid = false;
                            break;
                        }
                        HAPLog(&kHAPLog_Default,
                               "- MAC Address Credential: %02X:%02X:%02X:%02X:%02X:%02X",
                               ((const uint8_t*) credentialMAC)[0],
                               ((const uint8_t*) credentialMAC)[1],
                               ((const uint8_t*) credentialMAC)[2],
                               ((const uint8_t*) credentialMAC)[3],
                               ((const uint8_t*) credentialMAC)[4],
                               ((const uint8_t*) credentialMAC)[5]);
                    } else if (credentialPassphrase) {
                        HAPLog(&kHAPLog_Default, "- PSK Credential: Passphrase: %s", credentialPassphrase);
                    } else if (credentialPSK) {
                        if (numCredentialPSKBytes != 32) {
                            isValid = false;
                            break;
                        }
                        HAPLog(&kHAPLog_Default,
                               "- PSK Credential: PSK: "
                               "<%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
                               "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X>",
                               ((const uint8_t*) credentialPSK)[0],
                               ((const uint8_t*) credentialPSK)[1],
                               ((const uint8_t*) credentialPSK)[2],
                               ((const uint8_t*) credentialPSK)[3],
                               ((const uint8_t*) credentialPSK)[4],
                               ((const uint8_t*) credentialPSK)[5],
                               ((const uint8_t*) credentialPSK)[6],
                               ((const uint8_t*) credentialPSK)[7],
                               ((const uint8_t*) credentialPSK)[8],
                               ((const uint8_t*) credentialPSK)[9],
                               ((const uint8_t*) credentialPSK)[10],
                               ((const uint8_t*) credentialPSK)[11],
                               ((const uint8_t*) credentialPSK)[12],
                               ((const uint8_t*) credentialPSK)[13],
                               ((const uint8_t*) credentialPSK)[14],
                               ((const uint8_t*) credentialPSK)[15],
                               ((const uint8_t*) credentialPSK)[16],
                               ((const uint8_t*) credentialPSK)[17],
                               ((const uint8_t*) credentialPSK)[18],
                               ((const uint8_t*) credentialPSK)[19],
                               ((const uint8_t*) credentialPSK)[20],
                               ((const uint8_t*) credentialPSK)[21],
                               ((const uint8_t*) credentialPSK)[22],
                               ((const uint8_t*) credentialPSK)[23],
                               ((const uint8_t*) credentialPSK)[24],
                               ((const uint8_t*) credentialPSK)[25],
                               ((const uint8_t*) credentialPSK)[26],
                               ((const uint8_t*) credentialPSK)[27],
                               ((const uint8_t*) credentialPSK)[28],
                               ((const uint8_t*) credentialPSK)[29],
                               ((const uint8_t*) credentialPSK)[30],
                               ((const uint8_t*) credentialPSK)[31]);
                    }
                    added = true;
                }
                if (added) {
                    HAPLog(&kHAPLog_Default,
                           "--------------------------------------------------------------------------------");
                }
                if (isValid && (errCode != SQLITE_ROW && errCode != SQLITE_DONE)) {
                    isValid = false;
                }
            }
            sqlite3_finalize_safe(stmt);
            if (!isValid) {
                return kHAPError_Unknown;
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "next-identifier")) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;

            for (int i = 2; i < argc; i++) {
                if (!clientIdentifier) {
                    uint64_t rawClientIdentifier;
                    err = HAPUInt64FromString(argv[2], &rawClientIdentifier);
                    if (err || !rawClientIdentifier ||
                        rawClientIdentifier > (HAPPlatformWiFiRouterClientIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client profile identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!clientIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client profile identifier missing.");
                return kHAPError_InvalidData;
            }

            HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;

            int errCode;

            {
                sqlite3_stmt* stmt;
                errCode = sqlite3_prepare_v2(
                        wiFiRouter->db,
                        "INSERT INTO `sqlite_sequence` (\n"
                        "    `name`,\n"
                        "    `seq`\n"
                        ") SELECT\n"
                        "    'hap_clients',\n"
                        "    :seq\n"
                        "WHERE NOT EXISTS(SELECT 1 FROM `sqlite_sequence` WHERE `name` = 'hap_clients')",
                        /* nByte: */ -1,
                        &stmt,
                        /* pzTail: */ NULL);
                if (errCode) {
                    return kHAPError_Unknown;
                }

                err = kHAPError_Unknown;
                {
                    errCode =
                            sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":seq"), clientIdentifier - 1);
                    if (!errCode) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            err = kHAPError_None;
                        }
                    }
                }
                sqlite3_finalize_safe(stmt);
                if (err) {
                    return err;
                }
            }
            {
                sqlite3_stmt* stmt;
                errCode = sqlite3_prepare_v2(
                        wiFiRouter->db,
                        "UPDATE `sqlite_sequence` SET\n"
                        "    `seq` = :seq\n"
                        "WHERE `name` = 'hap_clients'",
                        /* nByte: */ -1,
                        &stmt,
                        /* pzTail: */ NULL);
                if (errCode) {
                    return kHAPError_Unknown;
                }

                err = kHAPError_Unknown;
                {
                    errCode =
                            sqlite3_bind_int64(stmt, sqlite3_bind_parameter_index(stmt, ":seq"), clientIdentifier - 1);
                    if (!errCode) {
                        errCode = sqlite3_step(stmt);
                        if (errCode == SQLITE_DONE) {
                            err = kHAPError_None;
                        }
                    }
                }
                sqlite3_finalize_safe(stmt);
                if (err) {
                    return err;
                }
            }
            return err;
        } else if (HAPStringAreEqual(operation, "add")) {
            HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = 0;
            HAPPlatformWiFiRouterClientCredential credential;
            HAPRawBufferZero(&credential, sizeof credential);
            for (int i = 2; i < argc; i++) {
                if (HAPStringAreEqual(argv[i], "--group")) {
                    if (groupIdentifier) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    uint64_t rawGroupIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawGroupIdentifier);
                    if (err || !rawGroupIdentifier || rawGroupIdentifier > (HAPPlatformWiFiRouterGroupIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client group identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    groupIdentifier = (HAPPlatformWiFiRouterGroupIdentifier) rawGroupIdentifier;
                } else if (HAPStringAreEqual(argv[i], "--credential")) {
                    if (credential.type) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without type.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    if (HAPStringAreEqual(argv[i], "mac")) {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_MACAddress;
                        if (i >= argc - 1) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "%s credential specified without value.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                        i++;
                        err = HAPMACAddressFromString(argv[i], &credential._.macAddress);
                        if (err) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "Invalid MAC address: %s.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                    } else if (HAPStringAreEqual(argv[i], "psk")) {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_PSK;
                        if (i >= argc - 1) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "%s credential specified without value.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                        i++;
                        const char* rawCredential = argv[i];

                        // Validate characters.
                        bool isValidHexKey = true;
                        const char* c;
                        for (c = rawCredential; *c; c++) {
                            if (*c < 32 || *c > 126) {
                                PrintUsage();
                                HAPLog(&kHAPLog_Default, "Wi-Fi passphrase contains invalid character: 0x%02X.", *c);
                                return kHAPError_InvalidData;
                            }
                            if (!HAPASCIICharacterIsHexDigit(*c)) {
                                isValidHexKey = false;
                            }
                        }

                        // Validate length.
                        // 8-63 printable ASCII characters or 64 hexadecimal digits.
                        size_t numBytes = (size_t)(c - rawCredential);
                        if (numBytes == 64 && isValidHexKey) {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_PSK;
                            for (size_t j = 0; j < kHAPWiFiWPAPSK_NumBytes; j++) {
                                uint8_t rawCredentialOffset0;
                                uint8_t rawCredentialOffset1;
                                err = HAPUInt8FromHexDigit(rawCredential[2 * j + 0], &rawCredentialOffset0);
                                HAPAssert(!err);
                                err = HAPUInt8FromHexDigit(rawCredential[2 * j + 1], &rawCredentialOffset1);
                                HAPAssert(!err);
                                credential._.psk._.psk.bytes[j] =
                                        (uint8_t)(rawCredentialOffset0 << 4) | (uint8_t)(rawCredentialOffset1 << 0);
                            }
                        } else if (numBytes >= 8 && numBytes <= 63) {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_Passphrase;
                            HAPAssert(numBytes < sizeof credential._.psk._.passphrase.stringValue);
                            HAPRawBufferCopyBytes(
                                    credential._.psk._.passphrase.stringValue, rawCredential, numBytes + 1);
                        } else {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default,
                                   "Wi-Fi passphrase has invalid length: %lu.",
                                   (unsigned long) numBytes);
                            return kHAPError_InvalidData;
                        }
                    } else {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Unknown --credential type: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!groupIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client group identifier missing.");
                return kHAPError_InvalidData;
            }
            if (!credential.type) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Credential missing.");
                return kHAPError_InvalidData;
            }

            // Generate TLV.
            uint8_t requestBytes[1024];
            HAPTLVWriter requestWriter;
            HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

            // Network Client Profile Control Operation.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                // Operation Type.
                uint8_t operationTypeBytes[] = {
                    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Add
                };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = 1,
                                .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Network Client Profile Configuration.
                {
                    HAPTLVWriter sub2Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                    }

                    // Client Group Identifier.
                    uint8_t groupIdentifierBytes[] = { HAPExpandLittleUInt32(groupIdentifier) };
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) { .type = 3,
                                              .value = { .bytes = groupIdentifierBytes,
                                                         .numBytes = sizeof groupIdentifierBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }

                    // Credential Data.
                    {
                        HAPTLVWriter sub3Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                        }

                        switch (credential.type) {
                            case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                                // MAC Address Credential.
                                err = HAPTLVWriterAppend(
                                        &sub3Writer,
                                        &(const HAPTLV) {
                                                .type = 1,
                                                .value = { .bytes = credential._.macAddress.bytes,
                                                           .numBytes = sizeof credential._.macAddress.bytes } });
                                if (err) {
                                    HAPAssert(err == kHAPError_OutOfResources);
                                    return err;
                                }
                                break;
                            }
                            case kHAPPlatformWiFiRouterCredentialType_PSK: {
                                switch (credential._.psk.type) {
                                    case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                                        // PSK Credential.
                                        err = HAPTLVWriterAppend(
                                                &sub3Writer,
                                                &(const HAPTLV) {
                                                        .type = 2,
                                                        .value = {
                                                                .bytes = credential._.psk._.passphrase.stringValue,
                                                                .numBytes = HAPStringGetNumBytes(
                                                                        credential._.psk._.passphrase.stringValue) } });
                                        if (err) {
                                            HAPAssert(err == kHAPError_OutOfResources);
                                            return err;
                                        }
                                        break;
                                    }
                                    case kHAPWiFiWPAPersonalCredentialType_PSK: {
                                        // PSK Credential.
                                        char pskCredentialBytes[2 * kHAPWiFiWPAPSK_NumBytes + 1] = { 0 };
                                        for (size_t i = 0; i < kHAPWiFiWPAPSK_NumBytes; i++) {
                                            err = HAPStringWithFormat(
                                                    &pskCredentialBytes[2 * i],
                                                    sizeof pskCredentialBytes - 2 * i,
                                                    "%02X",
                                                    credential._.psk._.psk.bytes[i]);
                                            HAPAssert(!err);
                                        }
                                        err = HAPTLVWriterAppend(
                                                &sub3Writer,
                                                &(const HAPTLV) { .type = 2,
                                                                  .value = { .bytes = pskCredentialBytes,
                                                                             .numBytes = HAPStringGetNumBytes(
                                                                                     pskCredentialBytes) } });
                                        if (err) {
                                            HAPAssert(err == kHAPError_OutOfResources);
                                            return err;
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub2Writer,
                                &(const HAPTLV) { .type = 4, .value = { .bytes = bytes, .numBytes = numBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }

                    // WAN Firewall Configuration.
                    {
                        HAPTLVWriter sub3Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                        }

                        // WAN Firewall Type.
                        uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = 1,
                                                  .value = { .bytes = wanFirewallTypeBytes,
                                                             .numBytes = sizeof wanFirewallTypeBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }

                        // WAN Firewall Rule List.
                        {
                            HAPTLVWriter sub4Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                            }

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub3Writer,
                                    &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                return err;
                            }
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub2Writer,
                                &(const HAPTLV) { .type = 5, .value = { .bytes = bytes, .numBytes = numBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }

                    // LAN Firewall Configuration.
                    {
                        HAPTLVWriter sub3Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                        }

                        // LAN Firewall Type.
                        uint8_t wanFirewallTypeBytes[] = { kHAPCharacteristicValue_WiFiRouter_FirewallType_Allowlist };
                        err = HAPTLVWriterAppend(
                                &sub3Writer,
                                &(const HAPTLV) { .type = 1,
                                                  .value = { .bytes = wanFirewallTypeBytes,
                                                             .numBytes = sizeof wanFirewallTypeBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }

                        // LAN Firewall Rule List.
                        {
                            HAPTLVWriter sub4Writer;
                            {
                                void* bytes;
                                size_t maxBytes;
                                HAPTLVWriterGetScratchBytes(&sub3Writer, &bytes, &maxBytes);
                                HAPTLVWriterCreate(&sub4Writer, bytes, maxBytes);
                            }

                            // Finalize.
                            void* bytes;
                            size_t numBytes;
                            HAPTLVWriterGetBuffer(&sub4Writer, &bytes, &numBytes);
                            err = HAPTLVWriterAppend(
                                    &sub3Writer,
                                    &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                            if (err) {
                                HAPAssert(err == kHAPError_OutOfResources);
                                return err;
                            }
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub2Writer,
                                &(const HAPTLV) { .type = 6, .value = { .bytes = bytes, .numBytes = numBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &requestWriter,
                        &(const HAPTLV) { .type = 1, .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Perform operation.
            {
                const HAPTLV8CharacteristicWriteRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory,
                    .remote = false,
                    .authorizationData = { .bytes = NULL, .numBytes = 0 }
                };
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
                HAPLogBuffer(&kHAPLog_Default, bytes, numBytes, "Synthesized %s TLV.", command);
                HAPTLVReader requestReader;
                HAPTLVReaderCreate(&requestReader, bytes, numBytes);
                err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                            err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            {
                const HAPTLV8CharacteristicReadRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory
                };
                HAPTLVWriter responseWriter;
                HAPTLVWriterCreate(&responseWriter, requestBytes, sizeof requestBytes);
                err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "update")) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;
            HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = 0;
            HAPPlatformWiFiRouterClientCredential credential;
            HAPRawBufferZero(&credential, sizeof credential);
            for (int i = 2; i < argc; i++) {
                if (HAPStringAreEqual(argv[i], "--group")) {
                    if (groupIdentifier) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    uint64_t rawGroupIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawGroupIdentifier);
                    if (err || !rawGroupIdentifier || rawGroupIdentifier > (HAPPlatformWiFiRouterGroupIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client group identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    groupIdentifier = (HAPPlatformWiFiRouterGroupIdentifier) rawGroupIdentifier;
                } else if (HAPStringAreEqual(argv[i], "--credential")) {
                    if (credential.type) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without type.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    if (HAPStringAreEqual(argv[i], "mac")) {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_MACAddress;
                        if (i >= argc - 1) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "%s credential specified without value.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                        i++;
                        err = HAPMACAddressFromString(argv[i], &credential._.macAddress);
                        if (err) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "Invalid MAC address: %s.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                    } else if (HAPStringAreEqual(argv[i], "psk")) {
                        credential.type = kHAPPlatformWiFiRouterCredentialType_PSK;
                        if (i >= argc - 1) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "%s credential specified without value.", argv[i]);
                            return kHAPError_InvalidData;
                        }
                        i++;
                        const char* rawCredential = argv[i];

                        // Validate characters.
                        bool isValidHexKey = true;
                        const char* c;
                        for (c = rawCredential; *c; c++) {
                            if (*c < 32 || *c > 126) {
                                PrintUsage();
                                HAPLog(&kHAPLog_Default, "Wi-Fi passphrase contains invalid character: 0x%02X.", *c);
                                return kHAPError_InvalidData;
                            }
                            if (!HAPASCIICharacterIsHexDigit(*c)) {
                                isValidHexKey = false;
                            }
                        }

                        // Validate length.
                        // 8-63 printable ASCII characters or 64 hexadecimal digits.
                        size_t numBytes = (size_t)(c - rawCredential);
                        if (numBytes == 64 && isValidHexKey) {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_PSK;
                            for (size_t j = 0; j < kHAPWiFiWPAPSK_NumBytes; j++) {
                                uint8_t rawCredentialOffset0;
                                uint8_t rawCredentialOffset1;
                                err = HAPUInt8FromHexDigit(rawCredential[2 * j + 0], &rawCredentialOffset0);
                                HAPAssert(!err);
                                err = HAPUInt8FromHexDigit(rawCredential[2 * j + 1], &rawCredentialOffset1);
                                HAPAssert(!err);
                                credential._.psk._.psk.bytes[j] =
                                        (uint8_t)(rawCredentialOffset0 << 4) | (uint8_t)(rawCredentialOffset1 << 0);
                            }
                        } else if (numBytes >= 8 && numBytes <= 63) {
                            credential._.psk.type = kHAPWiFiWPAPersonalCredentialType_Passphrase;
                            HAPAssert(numBytes < sizeof credential._.psk._.passphrase.stringValue);
                            HAPRawBufferCopyBytes(
                                    credential._.psk._.passphrase.stringValue, rawCredential, numBytes + 1);
                        } else {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default,
                                   "Wi-Fi passphrase has invalid length: %lu.",
                                   (unsigned long) numBytes);
                            return kHAPError_InvalidData;
                        }
                    } else {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Unknown --credential type: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                } else if (!clientIdentifier) {
                    uint64_t rawClientIdentifier;
                    err = HAPUInt64FromString(argv[2], &rawClientIdentifier);
                    if (err || !rawClientIdentifier ||
                        rawClientIdentifier > (HAPPlatformWiFiRouterClientIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client profile identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!clientIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client profile identifier missing.");
                return kHAPError_InvalidData;
            }

            // Generate TLV.
            uint8_t requestBytes[1024];
            HAPTLVWriter requestWriter;
            HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

            // Network Client Profile Control Operation.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                // Operation Type.
                uint8_t operationTypeBytes[] = {
                    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Update
                };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = 1,
                                .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Network Client Profile Configuration.
                {
                    HAPTLVWriter sub2Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                    }

                    // Network Client Profile Identifier.
                    uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clientIdentifier) };
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) { .type = 1,
                                              .value = { .bytes = networkClientIdentifierBytes,
                                                         .numBytes = sizeof networkClientIdentifierBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }

                    // Client Group Identifier.
                    if (groupIdentifier) {
                        uint8_t groupIdentifierBytes[] = { HAPExpandLittleUInt32(groupIdentifier) };
                        err = HAPTLVWriterAppend(
                                &sub2Writer,
                                &(const HAPTLV) { .type = 3,
                                                  .value = { .bytes = groupIdentifierBytes,
                                                             .numBytes = sizeof groupIdentifierBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }

                    // Credential Data.
                    if (credential.type) {
                        HAPTLVWriter sub3Writer;
                        {
                            void* bytes;
                            size_t maxBytes;
                            HAPTLVWriterGetScratchBytes(&sub2Writer, &bytes, &maxBytes);
                            HAPTLVWriterCreate(&sub3Writer, bytes, maxBytes);
                        }

                        switch (credential.type) {
                            case kHAPPlatformWiFiRouterCredentialType_MACAddress: {
                                // MAC Address Credential.
                                err = HAPTLVWriterAppend(
                                        &sub3Writer,
                                        &(const HAPTLV) {
                                                .type = 1,
                                                .value = { .bytes = credential._.macAddress.bytes,
                                                           .numBytes = sizeof credential._.macAddress.bytes } });
                                if (err) {
                                    HAPAssert(err == kHAPError_OutOfResources);
                                    return err;
                                }
                                break;
                            }
                            case kHAPPlatformWiFiRouterCredentialType_PSK: {
                                switch (credential._.psk.type) {
                                    case kHAPWiFiWPAPersonalCredentialType_Passphrase: {
                                        // PSK Credential.
                                        err = HAPTLVWriterAppend(
                                                &sub3Writer,
                                                &(const HAPTLV) {
                                                        .type = 2,
                                                        .value = {
                                                                .bytes = credential._.psk._.passphrase.stringValue,
                                                                .numBytes = HAPStringGetNumBytes(
                                                                        credential._.psk._.passphrase.stringValue) } });
                                        if (err) {
                                            HAPAssert(err == kHAPError_OutOfResources);
                                            return err;
                                        }
                                        break;
                                    }
                                    case kHAPWiFiWPAPersonalCredentialType_PSK: {
                                        // PSK Credential.
                                        char pskCredentialBytes[2 * kHAPWiFiWPAPSK_NumBytes + 1] = { 0 };
                                        for (size_t i = 0; i < kHAPWiFiWPAPSK_NumBytes; i++) {
                                            err = HAPStringWithFormat(
                                                    &pskCredentialBytes[2 * i],
                                                    sizeof pskCredentialBytes - 2 * i,
                                                    "%02X",
                                                    credential._.psk._.psk.bytes[i]);
                                            HAPAssert(!err);
                                        }
                                        err = HAPTLVWriterAppend(
                                                &sub3Writer,
                                                &(const HAPTLV) { .type = 2,
                                                                  .value = { .bytes = pskCredentialBytes,
                                                                             .numBytes = HAPStringGetNumBytes(
                                                                                     pskCredentialBytes) } });
                                        if (err) {
                                            HAPAssert(err == kHAPError_OutOfResources);
                                            return err;
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        // Finalize.
                        void* bytes;
                        size_t numBytes;
                        HAPTLVWriterGetBuffer(&sub3Writer, &bytes, &numBytes);
                        err = HAPTLVWriterAppend(
                                &sub2Writer,
                                &(const HAPTLV) { .type = 4, .value = { .bytes = bytes, .numBytes = numBytes } });
                        if (err) {
                            HAPAssert(err == kHAPError_OutOfResources);
                            return err;
                        }
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &requestWriter,
                        &(const HAPTLV) { .type = 1, .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Perform operation.
            {
                const HAPTLV8CharacteristicWriteRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory,
                    .remote = false,
                    .authorizationData = { .bytes = NULL, .numBytes = 0 }
                };
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
                HAPLogBuffer(&kHAPLog_Default, bytes, numBytes, "Synthesized %s TLV.", command);
                HAPTLVReader requestReader;
                HAPTLVReaderCreate(&requestReader, bytes, numBytes);
                err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                            err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            {
                const HAPTLV8CharacteristicReadRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory
                };
                HAPTLVWriter responseWriter;
                HAPTLVWriterCreate(&responseWriter, requestBytes, sizeof requestBytes);
                err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "remove")) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;

            for (int i = 2; i < argc; i++) {
                if (!clientIdentifier) {
                    uint64_t rawClientIdentifier;
                    err = HAPUInt64FromString(argv[2], &rawClientIdentifier);
                    if (err || !rawClientIdentifier ||
                        rawClientIdentifier > (HAPPlatformWiFiRouterClientIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client profile identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!clientIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client profile identifier missing.");
                return kHAPError_InvalidData;
            }

            // Generate TLV.
            uint8_t requestBytes[1024];
            HAPTLVWriter requestWriter;
            HAPTLVWriterCreate(&requestWriter, requestBytes, sizeof requestBytes);

            // Network Client Profile Control Operation.
            {
                HAPTLVWriter subWriter;
                {
                    void* bytes;
                    size_t maxBytes;
                    HAPTLVWriterGetScratchBytes(&requestWriter, &bytes, &maxBytes);
                    HAPTLVWriterCreate(&subWriter, bytes, maxBytes);
                }

                // Operation Type.
                uint8_t operationTypeBytes[] = {
                    kHAPCharacteristicValue_NetworkClientProfileControl_Operation_Type_Remove
                };
                err = HAPTLVWriterAppend(
                        &subWriter,
                        &(const HAPTLV) {
                                .type = 1,
                                .value = { .bytes = operationTypeBytes, .numBytes = sizeof operationTypeBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }

                // Network Client Profile Configuration.
                {
                    HAPTLVWriter sub2Writer;
                    {
                        void* bytes;
                        size_t maxBytes;
                        HAPTLVWriterGetScratchBytes(&subWriter, &bytes, &maxBytes);
                        HAPTLVWriterCreate(&sub2Writer, bytes, maxBytes);
                    }

                    // Network Client Profile Identifier.
                    uint8_t networkClientIdentifierBytes[] = { HAPExpandLittleUInt32(clientIdentifier) };
                    err = HAPTLVWriterAppend(
                            &sub2Writer,
                            &(const HAPTLV) { .type = 1,
                                              .value = { .bytes = networkClientIdentifierBytes,
                                                         .numBytes = sizeof networkClientIdentifierBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }

                    // Finalize.
                    void* bytes;
                    size_t numBytes;
                    HAPTLVWriterGetBuffer(&sub2Writer, &bytes, &numBytes);
                    err = HAPTLVWriterAppend(
                            &subWriter,
                            &(const HAPTLV) { .type = 2, .value = { .bytes = bytes, .numBytes = numBytes } });
                    if (err) {
                        HAPAssert(err == kHAPError_OutOfResources);
                        return err;
                    }
                }

                // Finalize.
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&subWriter, &bytes, &numBytes);
                err = HAPTLVWriterAppend(
                        &requestWriter,
                        &(const HAPTLV) { .type = 1, .value = { .bytes = bytes, .numBytes = numBytes } });
                if (err) {
                    HAPAssert(err == kHAPError_OutOfResources);
                    return err;
                }
            }

            // Perform operation.
            {
                const HAPTLV8CharacteristicWriteRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory,
                    .remote = false,
                    .authorizationData = { .bytes = NULL, .numBytes = 0 }
                };
                void* bytes;
                size_t numBytes;
                HAPTLVWriterGetBuffer(&requestWriter, &bytes, &numBytes);
                HAPLogBuffer(&kHAPLog_Default, bytes, numBytes, "Synthesized %s TLV.", command);
                HAPTLVReader requestReader;
                HAPTLVReaderCreate(&requestReader, bytes, numBytes);
                err = request.characteristic->callbacks.handleWrite(server, &request, &requestReader, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                            err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            {
                const HAPTLV8CharacteristicReadRequest request = {
                    .transportType = kHAPTransportType_IP,
                    .session = session,
                    .characteristic = &wiFiRouterNetworkClientProfileControlCharacteristic,
                    .service = &wiFiRouterService,
                    .accessory = accessory
                };
                HAPTLVWriter responseWriter;
                HAPTLVWriterCreate(&responseWriter, requestBytes, sizeof requestBytes);
                err = request.characteristic->callbacks.handleRead(server, &request, &responseWriter, NULL);
                if (err) {
                    HAPAssert(
                            err == kHAPError_Unknown || err == kHAPError_InvalidState ||
                            err == kHAPError_OutOfResources || err == kHAPError_Busy);
                    return kHAPError_Unknown;
                }
            }
            return kHAPError_None;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_NetworkClientStatusControl)) {
        // Operation.
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];
        if (HAPStringAreEqual(operation, "list")) {
            for (int i = 2; i < argc;) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                return kHAPError_InvalidData;
            }

            HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;
            bool added = false;
            for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
                if (!wiFiRouter->connections[i].isActive) {
                    continue;
                }
                added = true;

                HAPLog(&kHAPLog_Default,
                       "--------------------------------------------------------------------------------");
                HAPLog(&kHAPLog_Default,
                       "%lu: MAC %02X:%02X:%02X:%02X:%02X:%02X",
                       (unsigned long) wiFiRouter->connections[i].clientIdentifier,
                       wiFiRouter->connections[i].macAddress.bytes[0],
                       wiFiRouter->connections[i].macAddress.bytes[1],
                       wiFiRouter->connections[i].macAddress.bytes[2],
                       wiFiRouter->connections[i].macAddress.bytes[3],
                       wiFiRouter->connections[i].macAddress.bytes[4],
                       wiFiRouter->connections[i].macAddress.bytes[5]);
                for (size_t j = 0; j < wiFiRouter->connections[i].numIPAddresses; j++) {
                    switch (wiFiRouter->connections[i].ipAddresses[j].version) {
                        case kHAPIPAddressVersion_IPv4: {
                            HAPLog(&kHAPLog_Default,
                                   "- IPv4 Address: %u.%u.%u.%u",
                                   wiFiRouter->connections[i].ipAddresses[j]._.ipv4.bytes[0],
                                   wiFiRouter->connections[i].ipAddresses[j]._.ipv4.bytes[1],
                                   wiFiRouter->connections[i].ipAddresses[j]._.ipv4.bytes[2],
                                   wiFiRouter->connections[i].ipAddresses[j]._.ipv4.bytes[3]);
                            break;
                        }
                        case kHAPIPAddressVersion_IPv6: {
                            HAPLog(&kHAPLog_Default,
                                   "- IPv6 Address: %04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[0]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[2]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[4]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[6]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[8]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[10]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[12]),
                                   HAPReadBigUInt16(&wiFiRouter->connections[i].ipAddresses[j]._.ipv6.bytes[14]));
                            break;
                        }
                    }
                }
                if (HAPStringGetNumBytes(wiFiRouter->connections[i].name)) {
                    HAPLog(&kHAPLog_Default, "- Name: %s", wiFiRouter->connections[i].name);
                }
            }
            if (added) {
                HAPLog(&kHAPLog_Default,
                       "--------------------------------------------------------------------------------");
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "connect")) {
            // MAC address.
            if (argc < 3) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "MAC address not specified.");
                return kHAPError_InvalidData;
            }
            HAPMACAddress macAddress;
            err = HAPMACAddressFromString(argv[2], &macAddress);
            if (err) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Invalid MAC address: %s.", argv[2]);
                return kHAPError_InvalidData;
            }

            // Optional arguments.
            HAPWiFiWPAPersonalCredential credential;
            bool credentialFound = false;
            const char* name = NULL;
            for (int i = 3; i < argc; i++) {
                if (HAPStringAreEqual(argv[i], "--credential")) {
                    if (credentialFound) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    credentialFound = true;
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    const char* rawCredential = argv[i];

                    // Validate characters.
                    bool isValidHexKey = true;
                    const char* c;
                    for (c = rawCredential; *c; c++) {
                        if (*c < 32 || *c > 126) {
                            PrintUsage();
                            HAPLog(&kHAPLog_Default, "Wi-Fi passphrase contains invalid character: 0x%02X.", *c);
                            return kHAPError_InvalidData;
                        }
                        if (!HAPASCIICharacterIsHexDigit(*c)) {
                            isValidHexKey = false;
                        }
                    }

                    // Validate length.
                    // 8-63 printable ASCII characters or 64 hexadecimal digits.
                    size_t numBytes = (size_t)(c - rawCredential);
                    if (numBytes == 64 && isValidHexKey) {
                        credential.type = kHAPWiFiWPAPersonalCredentialType_PSK;
                        for (size_t j = 0; j < kHAPWiFiWPAPSK_NumBytes; j++) {
                            uint8_t rawCredentialOffset0;
                            uint8_t rawCredentialOffset1;
                            err = HAPUInt8FromHexDigit(rawCredential[2 * j + 0], &rawCredentialOffset0);
                            HAPAssert(!err);
                            err = HAPUInt8FromHexDigit(rawCredential[2 * j + 1], &rawCredentialOffset1);
                            HAPAssert(!err);
                            credential._.psk.bytes[j] =
                                    (uint8_t)(rawCredentialOffset0 << 4) | (uint8_t)(rawCredentialOffset1 << 0);
                        }
                    } else if (numBytes >= 8 && numBytes <= 63) {
                        credential.type = kHAPWiFiWPAPersonalCredentialType_Passphrase;
                        HAPAssert(numBytes < sizeof credential._.passphrase.stringValue);
                        HAPRawBufferCopyBytes(credential._.passphrase.stringValue, rawCredential, numBytes + 1);
                    } else {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Wi-Fi passphrase has invalid length: %lu.", (unsigned long) numBytes);
                        return kHAPError_InvalidData;
                    }
                } else if (HAPStringAreEqual(argv[i], "--name")) {
                    if (name) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    name = argv[i];
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }

            // Perform operation.
            err = HAPPlatformWiFiRouterConnectClient(
                    debugCommandLineContext.wiFiRouter,
                    credentialFound ? &credential : NULL,
                    &macAddress,
                    name,
                    /* ipAddress: */ NULL);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized);
                HAPLog(&kHAPLog_Default, "Connecting network client failed: %u.", err);
                return kHAPError_Unknown;
            }
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "disconnect")) {
            // MAC address.
            if (argc < 3) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "MAC address not specified.");
                return kHAPError_InvalidData;
            }
            HAPMACAddress macAddress;
            err = HAPMACAddressFromString(argv[2], &macAddress);
            if (err) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Invalid MAC address: %s.", argv[2]);
                return kHAPError_InvalidData;
            }

            // Optional arguments.
            for (int i = 3; i < argc;) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                return kHAPError_InvalidData;
            }

            // Perform operation.
            HAPPlatformWiFiRouterDisconnectClient(debugCommandLineContext.wiFiRouter, &macAddress);
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "add")) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;
            HAPMACAddress macAddress;
            HAPRawBufferZero(&macAddress, sizeof macAddress);
            bool macAddressFound = false;
            HAPIPAddress ipAddresses[kHAPPlatformWiFiRouterConnection_MaxIPAddresses];
            size_t numIPAddresses = 0;
            HAPPlatformWiFiRouterGroupIdentifier groupIdentifier = 0;
            const char* _Nullable name = NULL;
            for (int i = 2; i < argc; i++) {
                if (HAPStringAreEqual(argv[i], "--client")) {
                    if (clientIdentifier) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    uint64_t rawClientIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawClientIdentifier);
                    if (err || !rawClientIdentifier ||
                        rawClientIdentifier > (HAPPlatformWiFiRouterClientIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client profile identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                } else if (HAPStringAreEqual(argv[i], "--mac")) {
                    if (macAddressFound) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    err = HAPMACAddressFromString(argv[i], &macAddress);
                    if (err) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid MAC address: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    macAddressFound = true;
                } else if (HAPStringAreEqual(argv[i], "--ip")) {
                    if (numIPAddresses >= HAPArrayCount(ipAddresses)) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Too many arguments of kind %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    err = HAPIPAddressFromString(argv[i], &ipAddresses[numIPAddresses]);
                    if (err) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid IP address: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    numIPAddresses++;
                } else if (HAPStringAreEqual(argv[i], "--group")) {
                    if (groupIdentifier) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    uint64_t rawGroupIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawGroupIdentifier);
                    if (err || !rawGroupIdentifier || rawGroupIdentifier > (HAPPlatformWiFiRouterGroupIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client group identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    groupIdentifier = (HAPPlatformWiFiRouterGroupIdentifier) rawGroupIdentifier;
                } else if (HAPStringAreEqual(argv[i], "--name")) {
                    if (name) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Duplicate argument: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    if (i >= argc - 1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "%s specified without value.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    i++;
                    name = argv[i];
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!macAddressFound) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "MAC address missing.");
                return kHAPError_InvalidData;
            }
            if (!groupIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client group identifier missing.");
                return kHAPError_InvalidData;
            }

            // Perform operation.
            HAPLogInfo(&kHAPLog_Default, "Adding simulated network client status.");
            HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;
            for (size_t i = 0; i < HAPArrayCount(wiFiRouter->connections); i++) {
                if (wiFiRouter->connections[i].isActive) {
                    if (HAPRawBufferAreEqual(
                                wiFiRouter->connections[i].macAddress.bytes,
                                macAddress.bytes,
                                sizeof macAddress.bytes)) {
                        HAPLog(&kHAPLog_Default, "Network client with the given MAC address is already connected.");
                        return kHAPError_InvalidState;
                    }

                    continue;
                }

                // Create fake network client status.
                wiFiRouter->connections[i].isActive = true;
                wiFiRouter->connections[i].clientIdentifier = clientIdentifier;
                wiFiRouter->connections[i].macAddress = macAddress;
                HAPAssert(numIPAddresses <= HAPArrayCount(wiFiRouter->connections[i].ipAddresses));
                wiFiRouter->connections[i].numIPAddresses = numIPAddresses;
                for (size_t j = 0; j < numIPAddresses; j++) {
                    wiFiRouter->connections[i].ipAddresses[j] = ipAddresses[j];
                }
                if (name) {
                    size_t numNameBytes = HAPStringGetNumBytes(HAPNonnull(name));
                    if (numNameBytes >= sizeof wiFiRouter->connections[i].name) {
                        HAPLog(&kHAPLog_Default, "Network client has too long name. Not keeping track of name.");
                    } else {
                        HAPRawBufferCopyBytes(wiFiRouter->connections[i].name, HAPNonnull(name), numNameBytes + 1);
                    }
                }
                HAPLogInfo(
                        &kHAPLog_Default,
                        "Network client %02X:%02X:%02X:%02X:%02X:%02X connected (network client identifier %lu).",
                        macAddress.bytes[0],
                        macAddress.bytes[1],
                        macAddress.bytes[2],
                        macAddress.bytes[3],
                        macAddress.bytes[4],
                        macAddress.bytes[5],
                        (unsigned long) clientIdentifier);
                return kHAPError_None;
            }
            HAPLog(&kHAPLog_Default, "Reached limit of concurrently connected clients.");
            return kHAPError_OutOfResources;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_RouterStatus)) {
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        if (HAPStringAreEqual(operation, "ready")) {
            HAPPlatformWiFiRouterSetReady(debugCommandLineContext.wiFiRouter, true);
            return kHAPError_None;
        } else if (HAPStringAreEqual(operation, "not-ready")) {
            HAPPlatformWiFiRouterSetReady(debugCommandLineContext.wiFiRouter, false);
            return kHAPError_None;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_WANConfigurationList)) {
        if (argc < 3) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation or WAN identifier not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        HAPPlatformWiFiRouterWANType wanType;

        if (HAPStringAreEqual(operation, "unconfigured")) {
            wanType = kHAPPlatformWiFiRouterWANType_Unconfigured;
        } else if (HAPStringAreEqual(operation, "other")) {
            wanType = kHAPPlatformWiFiRouterWANType_Other;
        } else if (HAPStringAreEqual(operation, "dhcp")) {
            wanType = kHAPPlatformWiFiRouterWANType_DHCP;
        } else if (HAPStringAreEqual(operation, "bridge")) {
            wanType = kHAPPlatformWiFiRouterWANType_BridgeMode;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown WAN type: %s.", argv[1]);
            return kHAPError_InvalidData;
        }

        HAPPlatformWiFiRouterWANIdentifier wanIdentifier;
        uint64_t rawWANIdentifier;
        err = HAPUInt64FromString(argv[2], &rawWANIdentifier);
        if (err || !rawWANIdentifier || rawWANIdentifier > (HAPPlatformWiFiRouterWANIdentifier) -1) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Invalid WAN identifier: %s.", argv[2]);
            return kHAPError_InvalidData;
        }
        wanIdentifier = (HAPPlatformWiFiRouterWANIdentifier) rawWANIdentifier;

        err = HAPPlatformWiFiRouterAcquireExclusiveConfigurationAccess(debugCommandLineContext.wiFiRouter);
        if (err) {
            HAPAssert(err == kHAPError_Busy);
        } else {
            err = HAPPlatformWiFiRouterWANSetType(debugCommandLineContext.wiFiRouter, wanIdentifier, wanType);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
            }
            HAPPlatformWiFiRouterReleaseExclusiveConfigurationAccess(debugCommandLineContext.wiFiRouter);
        }
        return err;
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_WANStatusList)) {
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        if (HAPStringAreEqual(operation, "update")) {
            HAPPlatformWiFiRouterWANIdentifier wanIdentifier = 0;
            HAPPlatformWiFiRouterWANStatus wanStatus = 0;
            bool wanStatusFound = false;
            for (int i = 2; i < argc; i++) {
                if (!wanIdentifier) {
                    uint64_t rawWANIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawWANIdentifier);
                    if (err || !rawWANIdentifier || rawWANIdentifier > (HAPPlatformWiFiRouterWANIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid WAN identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    wanIdentifier = (HAPPlatformWiFiRouterWANIdentifier) rawWANIdentifier;
                } else if (!wanStatusFound) {
                    uint64_t rawWANStatus;
                    err = HAPUInt64FromString(argv[i], &rawWANStatus);
                    if (err || rawWANStatus > (HAPPlatformWiFiRouterWANStatus) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid WAN link status: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    wanStatus = (HAPPlatformWiFiRouterWANStatus) rawWANStatus;
                    wanStatusFound = true;
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!wanIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "WAN identifier missing.");
                return kHAPError_InvalidData;
            }
            if (!wanStatusFound) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "WAN link status missing.");
                return kHAPError_InvalidData;
            }

            err = HAPPlatformWiFiRouterWANSetStatus(debugCommandLineContext.wiFiRouter, wanIdentifier, wanStatus);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                return err;
            }
            return kHAPError_None;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_ManagedNetworkEnable)) {
        // Operation.
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        HAPCharacteristicValue_ManagedNetworkEnable value = 0;
        if (HAPStringAreEqual(operation, "disabled")) {
            value = kHAPCharacteristicValue_ManagedNetworkEnable_Disabled;
        } else if (HAPStringAreEqual(operation, "enabled")) {
            value = kHAPCharacteristicValue_ManagedNetworkEnable_Enabled;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
        for (int i = 2; i < argc;) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
            return kHAPError_InvalidData;
        }

        // Perform operation.
        {
            const HAPUInt8CharacteristicWriteRequest request = {
                .transportType = kHAPTransportType_IP,
                .session = session,
                .characteristic = &wiFiRouterManagedNetworkEnableCharacteristic,
                .service = &wiFiRouterService,
                .accessory = accessory,
                .remote = false,
                .authorizationData = { .bytes = NULL, .numBytes = 0 }
            };
            err = request.characteristic->callbacks.handleWrite(server, &request, value, NULL);
            if (err) {
                HAPAssert(
                        err == kHAPError_Unknown || err == kHAPError_InvalidState || err == kHAPError_InvalidData ||
                        err == kHAPError_OutOfResources || err == kHAPError_NotAuthorized || err == kHAPError_Busy);
                return kHAPError_Unknown;
            }
        }
        return kHAPError_None;
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_NetworkAccessViolationControl)) {
        if (argc < 2) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Operation not specified.");
            return kHAPError_InvalidData;
        }
        const char* operation = argv[1];

        if (HAPStringAreEqual(operation, "add")) {
            HAPPlatformWiFiRouterClientIdentifier clientIdentifier = 0;
            HAPPlatformWiFiRouterTimestamp timestamp = 0;
            bool timestampFound = false;
            for (int i = 2; i < argc; i++) {
                if (!clientIdentifier) {
                    uint64_t rawClientIdentifier;
                    err = HAPUInt64FromString(argv[i], &rawClientIdentifier);
                    if (err || !rawClientIdentifier ||
                        rawClientIdentifier > (HAPPlatformWiFiRouterClientIdentifier) -1) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid network client profile identifier: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    clientIdentifier = (HAPPlatformWiFiRouterClientIdentifier) rawClientIdentifier;
                } else if (!timestampFound) {
                    err = HAPUInt64FromString(argv[i], &timestamp);
                    if (err) {
                        PrintUsage();
                        HAPLog(&kHAPLog_Default, "Invalid timestamp: %s.", argv[i]);
                        return kHAPError_InvalidData;
                    }
                    timestampFound = true;
                } else {
                    PrintUsage();
                    HAPLog(&kHAPLog_Default, "Unknown argument: %s.", argv[i]);
                    return kHAPError_InvalidData;
                }
            }
            if (!clientIdentifier) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Network client profile identifier missing.");
                return kHAPError_InvalidData;
            }
            if (!timestampFound) {
                PrintUsage();
                HAPLog(&kHAPLog_Default, "Timestamp missing.");
                return kHAPError_InvalidData;
            }

            HAPLogInfo(&kHAPLog_Default, "Adding simulated network access violation.");
            HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;
            err = HAPPlatformWiFiRouterClientRecordAccessViolation(wiFiRouter, clientIdentifier, timestamp);
            if (err) {
                HAPAssert(err == kHAPError_Unknown || err == kHAPError_InvalidState);
                return err;
            }
            return kHAPError_None;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown %s operation: %s", command, operation);
            return kHAPError_InvalidData;
        }
    } else if (HAPStringAreEqual(command, kHAPCharacteristicDebugDescription_WiFiSatelliteStatus)) {
        if (argc < 3) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Satellite status or satellite index not specified.");
            return kHAPError_InvalidData;
        }

        HAPPlatformWiFiRouterSatelliteStatus satelliteStatus;

        if (HAPStringAreEqual(argv[1], "unknown")) {
            satelliteStatus = kHAPPlatformWiFiRouterSatelliteStatus_Unknown;
        } else if (HAPStringAreEqual(argv[1], "connected")) {
            satelliteStatus = kHAPPlatformWiFiRouterSatelliteStatus_Connected;
        } else if (HAPStringAreEqual(argv[1], "not-connected")) {
            satelliteStatus = kHAPPlatformWiFiRouterSatelliteStatus_NotConnected;
        } else {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Unknown satellite status: %s.", argv[1]);
            return kHAPError_InvalidData;
        }

        size_t satelliteIndex;

        uint64_t rawSatelliteIndex;
        err = HAPUInt64FromString(argv[2], &rawSatelliteIndex);
        if (err || rawSatelliteIndex > SIZE_MAX) {
            PrintUsage();
            HAPLog(&kHAPLog_Default, "Invalid satellite index: %s.", argv[2]);
            return kHAPError_InvalidData;
        }
        satelliteIndex = (size_t) rawSatelliteIndex;

        HAPLogInfo(&kHAPLog_Default, "Setting status of simulated Wi-Fi satellite accessory.");
        HAPPlatformWiFiRouterRef wiFiRouter = debugCommandLineContext.wiFiRouter;
        err = HAPPlatformWiFiRouterSetSatelliteStatus(wiFiRouter, satelliteIndex, satelliteStatus);
        if (err) {
            HAPAssert(err == kHAPError_InvalidState);
            return err;
        }
        return kHAPError_None;
    } else {
        PrintUsage();
        HAPLog(&kHAPLog_Default, "Unknown command: %s", command);
        return kHAPError_InvalidData;
    }
}

#endif

HAP_DIAGNOSTIC_POP
