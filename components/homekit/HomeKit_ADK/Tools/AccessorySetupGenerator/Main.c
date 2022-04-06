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

#include <stdio.h>
#include <stdlib.h>

#include "HAPAccessory.h"
#include "HAPAccessorySetup.h"
#include "HAPCrypto.h"

int main(int argc, char* argv[]) {
    HAPError err;
    if (argc <= 1) {

#ifndef _WIN32
#define B(stringLiteral) "\x1B[1m" stringLiteral "\x1B[0m"
#define U(stringLiteral) "\x1B[4m" stringLiteral "\x1B[0m"
#else
#define B(stringLiteral) stringLiteral
#define U(stringLiteral) stringLiteral
#endif

        const char* categoryDescriptions =
                "           1  Other.\n"
                "           2  Bridges.\n"
                "           3  Fans.\n"
                "           4  Garage Door Openers.\n"
                "           5  Lighting.\n"
                "           6  Locks.\n"
                "           7  Outlets.\n"
                "           8  Switches.\n"
                "           9  Thermostats.\n"
                "          10  Sensors.\n"
                "          11  Security Systems.\n"
                "          12  Doors.\n"
                "          13  Windows.\n"
                "          14  Window Coverings.\n"
                "          15  Programmable Switches.\n"
                "          16  Range Extenders.\n"
                "          17  IP Cameras.\n"
                "          18  Video Doorbells.\n"
                "          19  Air Purifiers.\n"
                "          20  Heaters.\n"
                "          21  Air Conditioners.\n"
                "          22  Humidifiers.\n"
                "          23  Dehumidifiers.\n"
                "          24  Apple TV.\n"
                "          28  Sprinklers.\n"
                "          29  Faucets.\n"
                "          30  Shower Systems.\n"
                "          32  Remotes.\n"
                "          33  Wi-Fi Routers.\n";

        const char* exampleOutput =
                "     1\n"
                "     518-08-582\n"
                "     263FEA64889756A8E25FD53DD5FA1022\n"
                "     D0BE3DFCC3B28A4D612943215AD71005CA4E240A5672EFF427F30EEAC173756167AC4D73779\n"
                "        3AF18937B1770E173ED346AB790E428B2771ACA62FE11C1A0FC8E01169824632BB914863\n"
                "        760918841CB3F263D5D71C431A2141C51797A91022C5BCD30D7BC9259A2037C4BDEE8F74\n"
                "        8D65B15AEA33DF2F00193FBAAC603C921820D2E4FE5747F965F31F3DD16D8A7228FE8FC8\n"
                "        5AD70138C797CB91B47488283C568D1CDAFCF6E950A1D117BD4E42FB0B90FF97992BCCE0\n"
                "        C86F62F866489BC2F556D342F4C20AC26B12A48299C642BE86270F0D3F1E6E86E84115A7\n"
                "        12931F7FE1D53E6230FB14C29AD2E23B16E0B8F6AFD4D709B562DC4921F550450AC8FD09\n"
                "        73DD80DAE629CB399DD6E3E96695E2E8060196D5FFFD292A1246AD76219E998FDD0E690B\n"
                "        405A0D2AD9C9CADF905520C4E6B66952E0DA27E523060DE310A539F6BF30E48B69A5F26D\n"
                "        5E283DE6EE8F51AFB920E00D1B1AE3BA423041A63BA788B6F6BCBA2AD7C89946EEE79D72\n"
                "        6649BCEAB43BB920F11260F8017C9921A60C169B28569\n"
                "     7OSX\n"
                "     X-HM://007JNU5AE7OSX"
                "\n"
                "     5A74B23CEX47J0N3"
                "\n";
        HAP_DIAGNOSTIC_PUSH
        HAP_DIAGNOSTIC_IGNORED_CLANG("-Woverlength-strings")
        HAP_DIAGNOSTIC_IGNORED_GCC("-Woverlength-strings")
        printf(
            B("HomeKit Accessory Setup Generator")" - Version %s (%s)\n"
            "\n"
            B("USAGE")"\n"
            "     "B("AccessorySetupGenerator")" [OPTION]...\n"
            "\n"
            B("DESCRIPTION")"\n"
            "     This tool generates information for provisioning of a HomeKit accessory,\n"
            "     namely a setup code, a corresponding SRP salt and verifier, and a setup ID.\n"
            "     The setup code is used by the controller to set up an encrypted link with\n"
            "     the accessory during HomeKit pairing. The setup ID is used to identify\n"
            "     the accessory to which a scanned label belongs.\n"
            "     \n"
            "     "B("Each accessory needs to be provisioned with unique accessory setup")"\n"
            "     "B("information before it may be used.")"\n"
            "\n"
            B("OPTIONS")"\n"
            "     The following options are available:\n"
            "     \n"
            "     "B("--ip")"\n"
            "        Accessory supports HAP over IP transport; MUST be ON if WAC is ON.\n"
            "     \n"
            "     "B("--ble")"\n"
            "        Accessory supports HAP over BLE transport.\n"
            "     \n"
            "     "B("--wac")"\n"
            "        Accessory supports Wi-Fi Accessory Configuration (WAC)\n"
            "        for configuring Wi-Fi credentials.\n"
            "     \n"
            "     "B("--thread")"\n"
            "        Accessory supports HAP over Thread transport. EUI must be included.\n"
            "     \n"
            "     "B("--category")" "U("Category")"\n"
            "        The accessory category.\n"
            "        \n"
            "        An accessory with support for multiple categories should advertise the\n"
            "        primary category. An accessory for which a primary category cannot be\n"
            "        determined or the primary category isn't among the well defined\n"
            "        categories falls in the `Other` category.\n"
            "        \n"
            "        Well defined categories:\n"
            "%s\n"
            "     "B("--setup-code")" "U("Setup code")"\n"
            "        Generates accessory setup information that allows pairing using the\n"
            "        specified setup code (e.g. for development).\n"
            "        Format is `XXX-XX-XXX` with X being a digit from 0-9.\n"
            "        - Setup codes that only consist of a repeating digit are not allowed.\n"
            "        - `123-45-678` and `876-54-321` are not allowed.\n"
            "        If this option is not present, a random setup code is generated.\n"
            "     \n"
            "     "B("--setup-id")" "U("Setup ID")"\n"
            "        Provisions accessory setup information using a specific setup ID.\n"
            "        Format is `XXXX` with X being a digit from 0-9 or a character from A-Z.\n"
            "        - Lowercase characters are not allowed.\n"
            "        Setup ID and EUI are mutually exclusive: Accessories may be provisioned with\n"
            "        one or the other, but not both\n"
            "        If this option is not present, and no EUI is provided,\n"
            "        a random setup ID is generated.\n"
            "     "B("--eui")" "U("EUI 64")"\n"
            "        Provisions accessory setup information with a specific EUI64.\n"
            "        EUI and Setup ID are mutually exclusive: Accessories may be provisioned with\n"
            "        one or the other, but not both\n"
            "        - Lowercase characters are not allowed.\n"
            "        - Required if device supports HAP over Thread\n"
            "        - Format is the 64 bit value represented in hexidecimal.\n"
            "     "B("--product-data")" "U("Product Data")"\n"
            "        Provisions accessory setup information with a specific Product Data.\n"
            "        - Format is the 64 bit value represented in hexidecimal.\n"
            "        - Top 32 bits are the Product Group.  Bottom 32 bits are the Product Number"
            "        - Lowercase characters are not allowed.\n"
            "\n"
            B("OUTPUT")"\n"
            "     Output consists of a series of lines in a machine-readable format.\n"
            "     Lines are terminated with a \\n character.\n"
            "     \n"
            "     1. "B("Output format version")" which is `1` for this version.\n"
            "     \n"
            "     2. "B("Setup code")" in format `XXX-XX-XXX` with X being a digit from 0-9.\n"
            "        - Must be deployed to the accessory if it has a programmable NFC tag but\n"
            "          is not connected to a display.\n"
            "        - Must be printed on labels affixed to the accessory and its packaging\n"
            "          if the accessory is not connected to a display.\n"
            "     \n"
            "     3. "B("SRP salt")" as a hexadecimal string.\n"
            "        - Must be deployed to the accessory if it is not connected to a display.\n"
            "     \n"
            "     4. "B("SRP verifier")" as a hexadecimal string.\n"
            "        - Must be deployed to the accessory if it is not connected to a display.\n"
            "     \n"
            "     5. "B("Setup ID")" in format `XXX` with X being a digit from 0-9 or a\n"
            "        character from A-Z.\n"
            "        - Must be deployed to the accessory.\n"
            "     \n"
            "     6. "B("Setup payload")" as a string.\n"
            "        - Must be printed on labels affixed to the accessory and its packaging\n"
            "          if the accessory is not connected to a display.\n"
            "     \n"
            "7. "B("Joiner Passphrase")" as a string.\n"
            "        - Must be printed deployed to the accessory if it supports HAP over Thread.\n"
            "\n"
            B("EXAMPLE")"\n"
            "     Example output for an "B("Outlet")" (category identifier "B("7")") accessory supporting\n"
            "     "B("HAP over IP")" and "B("Wi-Fi Accessory Configuration")" with setup code `"B("518-08-582")"`\n"
            "     and setup ID `"B("7OSX")"`.\n"
            "     \n"
            "%s",
            HAPGetVersion(), HAPGetBuild(), categoryDescriptions, exampleOutput);
        HAP_DIAGNOSTIC_POP
#undef B
#undef U
        exit(EXIT_SUCCESS);
    }

    // Parse arguments.
    const char* fixedSetupCode = NULL;
    const char* fixedSetupID = NULL;
    HAPAccessorySetupSetupPayloadFlags flags = {
        .threadSupported = false,
        .isPaired = false,
        .ipSupported = false,
        .bleSupported = false,
        .wacSupported = false,
    };
    HAPAccessoryCategory category = (HAPAccessoryCategory) 0;
    HAPEui64 eui;
    HAPEui64* euiPtr = NULL;
    HAPAccessoryProductData prodData;
    HAPAccessoryProductData* prodDataPtr = NULL;

    for (int i = 1; i < argc; i++) {
        if (HAPStringAreEqual(argv[i], "--ip")) {
            if (flags.ipSupported) {
                fprintf(stderr, "--ip specified multiple times.");
                exit(EXIT_FAILURE);
            }
            flags.ipSupported = true;
        } else if (HAPStringAreEqual(argv[i], "--ble")) {
            if (flags.bleSupported) {
                fprintf(stderr, "--ble specified multiple times.");
                exit(EXIT_FAILURE);
            }
            flags.bleSupported = true;
        } else if (HAPStringAreEqual(argv[i], "--wac")) {
            if (flags.wacSupported) {
                fprintf(stderr, "--wac specified multiple times.");
                exit(EXIT_FAILURE);
            }
            flags.wacSupported = true;
        } else if (HAPStringAreEqual(argv[i], "--thread")) {
            if (flags.threadSupported) {
                fprintf(stderr, "--thread specified multiple times.\n");
                exit(EXIT_FAILURE);
            }
            flags.threadSupported = true;
        } else if (HAPStringAreEqual(argv[i], "--category")) {
            if (i == argc) {
                fprintf(stderr, "--category specified without accessory category identifier.");
                exit(EXIT_FAILURE);
            }
            i++;
            uint64_t categoryID;
            err = HAPUInt64FromString(argv[i], &categoryID);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                fprintf(stderr, "--category specified with malformed accessory category identifier.");
                exit(EXIT_FAILURE);
            }
            if (!categoryID || categoryID > UINT8_MAX) {
                fprintf(stderr, "--category specified with out-of-range accessory category identifier.");
                exit(EXIT_FAILURE);
            }
            if (category) {
                fprintf(stderr, "--category specified multiple times.");
                exit(EXIT_FAILURE);
            }
            category = (HAPAccessoryCategory) categoryID;
        } else if (HAPStringAreEqual(argv[i], "--setup-code")) {
            if (i == argc) {
                fprintf(stderr, "--setup-code specified without setup code.");
                exit(EXIT_FAILURE);
            }
            i++;
            if (!HAPAccessorySetupIsValidSetupCode(argv[i])) {
                fprintf(stderr, "--setup-code specified with invalid setup code.");
                exit(EXIT_FAILURE);
            }
            if (fixedSetupCode) {
                fprintf(stderr, "--setup-code specified multiple times.");
                exit(EXIT_FAILURE);
            }
            fixedSetupCode = argv[i];
        } else if (HAPStringAreEqual(argv[i], "--setup-id")) {
            if (i == argc) {
                fprintf(stderr, "--setup-id specified without setup ID.");
                exit(EXIT_FAILURE);
            }
            i++;
            if (!HAPAccessorySetupIsValidSetupID(argv[i])) {
                fprintf(stderr, "--setup-id specified with invalid setup ID.");
                exit(EXIT_FAILURE);
            }
            if (fixedSetupID) {
                fprintf(stderr, "--setup-id specified multiple times.");
                exit(EXIT_FAILURE);
            }
            fixedSetupID = argv[i];
        } else if (HAPStringAreEqual(argv[i], "--eui")) {
            if (i == argc) {
                fprintf(stderr, "--eui specified without EUI\n");
                exit(EXIT_FAILURE);
            }
            if (euiPtr) {
                fprintf(stderr, "--eui specified multiple times.\n");
                exit(EXIT_FAILURE);
            }
            i++;
            uint64_t euiNum;
            err = HAPUInt64FromHexString(argv[i], &euiNum);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                fprintf(stderr, "--eui specified with malformed eui value.\n");
                exit(EXIT_FAILURE);
            }
            for (int j = sizeof(eui) - 1; j >= 0; j--) {
                eui.bytes[j] = euiNum & 0xFF;
                euiNum >>= 8;
            }
            euiPtr = &eui;
        } else if (HAPStringAreEqual(argv[i], "--product-data")) {
            if (i == argc) {
                fprintf(stderr, "--product-data specified without Product Data\n");
                exit(EXIT_FAILURE);
            }
            if (prodDataPtr) {
                fprintf(stderr, "--product-data specified multiple times.\n");
                exit(EXIT_FAILURE);
            }
            i++;
            uint64_t prodDataVal;
            err = HAPUInt64FromHexString(argv[i], &prodDataVal);
            if (err) {
                HAPAssert(err == kHAPError_InvalidData);
                fprintf(stderr, "--prodDataVal specified with malformed product data value.\n");
                exit(EXIT_FAILURE);
            }
            for (int j = sizeof(prodData) - 1; j >= 0; j--) {
                prodData.bytes[j] = prodDataVal & 0xFF;
                prodDataVal >>= 8;
            }
            prodDataPtr = &prodData;
        } else {
            fprintf(stderr, "Too many arguments specified.\n");
            exit(EXIT_FAILURE);
        }
    }
    if (!category) {
        fprintf(stderr, "No accessory category identifier specified.\n");
        exit(EXIT_FAILURE);
    }
    if (flags.threadSupported && !euiPtr) {
        fprintf(stderr, "The HAP over Thread feature requires an EUI\n");
        exit(EXIT_FAILURE);
    }
    if (!flags.ipSupported && !flags.bleSupported && !flags.threadSupported) {
        fprintf(stderr, "No transport specified.\n");
        exit(EXIT_FAILURE);
    }
    if (flags.wacSupported && !flags.ipSupported) {
        fprintf(stderr, "-wac specified but -ip not specified.\n");
        exit(EXIT_FAILURE);
    }
    if (!prodDataPtr) {
        fprintf(stderr, "No product data specified\n");
        exit(EXIT_FAILURE);
    }
    if (fixedSetupID && euiPtr) {
        fprintf(stderr, "EUI and Setup ID are mutually exclusive\n");
        exit(EXIT_FAILURE);
    }

    // Setup code.
    HAPSetupCode setupCode;
    if (fixedSetupCode) {
        HAPRawBufferCopyBytes(setupCode.stringValue, fixedSetupCode, sizeof setupCode.stringValue);
    } else {
        HAPAccessorySetupGenerateRandomSetupCode(&setupCode);
    }

    // Setup info.
    HAPSetupInfo setupInfo;
    HAPPlatformRandomNumberFill(setupInfo.salt, sizeof setupInfo.salt);

    const uint8_t srpUserName[] = "Pair-Setup";
    HAP_srp_verifier(
            setupInfo.verifier,
            setupInfo.salt,
            srpUserName,
            sizeof srpUserName - 1,
            (const uint8_t*) setupCode.stringValue,
            sizeof setupCode.stringValue - 1);

    // Setup ID.
    HAPSetupID setupID;

    HAPSetupID* setupIDptr = NULL;
    if (fixedSetupID) {
        HAPRawBufferCopyBytes(setupID.stringValue, fixedSetupID, sizeof setupID.stringValue);
        setupIDptr = &setupID;
    } else if (!euiPtr) {
        HAPAccessorySetupGenerateRandomSetupID(&setupID);
        fixedSetupID = setupID.stringValue;
        setupIDptr = &setupID;
    }

    // Setup payload.
    HAPSetupPayload setupPayload;
    HAPAccessorySetupGetSetupPayload(&setupPayload, &setupCode, setupIDptr, euiPtr, prodDataPtr, flags, category);

    char setupIdFromEuiChars[5];
    if (euiPtr) {
        HAPRawBufferZero(setupIdFromEuiChars, sizeof(setupIdFromEuiChars));
        HAPRawBufferCopyBytes(setupIdFromEuiChars, &setupPayload.stringValue[9 + 7], 4);
        fixedSetupID = setupIdFromEuiChars;
    }

    // Output.
    printf("1\n");
    printf("%s\n", setupCode.stringValue);
    for (size_t i = 0; i < sizeof setupInfo.salt; i++) {
        printf("%02X", setupInfo.salt[i]);
    }
    printf("\n");
    for (size_t i = 0; i < sizeof setupInfo.verifier; i++) {
        printf("%02X", setupInfo.verifier[i]);
    }
    printf("\n");
    printf("%s\n", fixedSetupID);
    printf("%s\n", setupPayload.stringValue);

    if (flags.threadSupported) {
        // Generate joiner passphrase
        HAPJoinerPassphrase passphrase;
        HAPAccessorySetupGenerateJoinerPassphrase(&passphrase, &setupCode);
        printf("%s\n", passphrase.stringValue);
    }

    return 0;
}
