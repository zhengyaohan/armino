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

#include "HAPAccessory.h"
#include "HAPAccessorySetup.h"

const char* TestArgs[][12] = {
    { NULL,
      "518-08-582",
      "7OSX",
      "0",
      "0",
      "1",
      "0",
      "1",
      "7",
      "E3644573",
      "0",
      "X-HM://0E8NKO6YU7OSX0YJH2CJFPA4DMRWGSG" },
    { NULL,
      "000-00-000",
      "0000",
      "0",
      "0",
      "1",
      "0",
      "1",
      "7",
      "E3644573",
      "0",
      "X-HM://0E8MPTR7K00000YJH2CJFPA4DMRWGSG" },
    { NULL,
      "000-00-000",
      "0000",
      "1",
      "0",
      "1",
      "0",
      "1",
      "7",
      "E3644573",
      "0",
      "X-HM://0E8OXQI9S00000YJH2CJFPA4DMRWGSG" },
    { NULL,
      "518-08-582",
      "7OSX",
      "0",
      "0",
      "0",
      "1",
      "0",
      "7",
      "E3644573",
      "0",
      "X-HM://0E8A97OLI7OSX0YJH2CJFPA4DMRWGSG" },
    { NULL,
      "518-08-582",
      "0000",
      "0",
      "1",
      "1",
      "0",
      "1",
      "7",
      "E3644573",
      "F4CE366B04F7D4DF",
      "X-HM://0L97J37T29FM9QQ4PVEDPDP7NHJGTFK" },
    { NULL,
      "518-08-582",
      "0000",
      "0",
      "1",
      "0",
      "1",
      "0",
      "7",
      "E3644573",
      "F4CE366B04F7D4DF",
      "X-HM://0L8U7MPFQ9FM9QQ4PVEDPDP7NHJGTFK" },

};

int Test(int argc, const char* argv[]) {
    // Input arguments:
    // argv[1] - Setup code. Format XXX-XX-XXX. 000-00-000 for NULL.
    // argv[2] - Setup ID. Format XXXX. 0000 together with NULL setup code for NULL.
    // argv[3] - Paired (1 or 0).
    // argv[4] - Supports HAP over Thread (1 or 0).
    // argv[5] - Supports HAP over IP (1 or 0).
    // argv[6] - Supports HAP over BLE (1 or 0).
    // argv[7] - Supports WAC (1 or 0).
    // argv[8] - Category.
    // argv[9] - product num
    // argv[10] - EUI
    // argv[11] - Expected setup payload.
    HAPPrecondition(argc == 12);

    HAPError err;

    // Process arguments.
    const HAPSetupCode* setupCode = NULL;
    if (!HAPStringAreEqual(argv[1], "000-00-000")) {
        HAPPrecondition(HAPAccessorySetupIsValidSetupCode(argv[1]));
        setupCode = (const HAPSetupCode*) argv[1];
    }

    uint64_t euiVal;
    err = HAPUInt64FromHexString(argv[10], &euiVal);
    HAPPrecondition(!err);

    HAPEui64 eui;
    const HAPEui64* euiPtr = euiVal == 0 ? NULL : &eui;
    for (int j = sizeof(HAPEui64) - 1; j >= 0; j--) {
        eui.bytes[j] = euiVal & 0xFF;
        euiVal >>= 8;
    }

    const HAPSetupID* setupID = NULL;
    if (!setupCode || euiPtr) {
        HAPPrecondition(HAPStringAreEqual(argv[2], "0000"));
    } else {
        HAPPrecondition(HAPAccessorySetupIsValidSetupID(argv[2]));
        setupID = (const HAPSetupID*) argv[2];
    }
    uint64_t isPaired;
    err = HAPUInt64FromString(argv[3], &isPaired);
    HAPPrecondition(!err);
    HAPPrecondition(isPaired == false || isPaired == true);
    uint64_t threadSupported;
    err = HAPUInt64FromString(argv[4], &threadSupported);
    HAPPrecondition(!err);
    uint64_t ipSupported;
    err = HAPUInt64FromString(argv[5], &ipSupported);
    HAPPrecondition(!err);
    uint64_t bleSupported;
    err = HAPUInt64FromString(argv[6], &bleSupported);
    HAPPrecondition(!err);
    HAPPrecondition(bleSupported == false || bleSupported == true);
    uint64_t wacSupported;
    err = HAPUInt64FromString(argv[7], &wacSupported);
    HAPPrecondition(!err);
    HAPPrecondition(wacSupported == false || wacSupported == true);
    uint64_t category;
    err = HAPUInt64FromString(argv[8], &category);
    HAPPrecondition(!err);
    HAPPrecondition(category > 0 && category <= UINT16_MAX);

    uint64_t productDataVal;
    err = HAPUInt64FromHexString(argv[9], &productDataVal);
    HAPPrecondition(!err);
    HAPPrecondition(productDataVal > 0 && productDataVal <= UINT64_MAX);

    HAPAccessoryProductData prodData;
    for (int j = sizeof(prodData) - 1; j >= 0; j--) {
        prodData.bytes[j] = productDataVal & 0xFF;
        productDataVal >>= 8;
    }

    // Derive setup payload.
    HAPSetupPayload setupPayload;
    HAPAccessorySetupSetupPayloadFlags flags = { .isPaired = (bool) isPaired,
                                                 .ipSupported = (bool) ipSupported,
                                                 .bleSupported = (bool) bleSupported,
                                                 .wacSupported = (bool) wacSupported,
                                                 .threadSupported = (bool) threadSupported };
    HAPAccessorySetupGetSetupPayload(
            &setupPayload, setupCode, setupID, euiPtr, &prodData, flags, (HAPAccessoryCategory) category);

    // Compare with expectation.
    HAPAssert(HAPStringAreEqual(setupPayload.stringValue, argv[11]));

    return 0;
}

int main(int argc, char* argv[]) {
    for (size_t i = 0; i < HAPArrayCount(TestArgs); ++i) {
        HAPAssert(Test(12, TestArgs[i]) == 0);
    }

    return 0;
}
