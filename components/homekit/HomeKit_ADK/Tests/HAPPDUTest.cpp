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
// Copyright (C) 2020 Apple Inc. All Rights Reserved.

#include "HAPPDU.h"
#include "HAP+API.h"
#include "Harness/UnitTestFramework.h"

#define WRITE_PDU_WITH_BAD_OPCODE     0x00, 0xAB, 0x00, 0x00, 0x00
#define WRITE_PDU_WITH_NO_BODY_LENGTH 0x00, 0x02, 0x00, 0x00, 0x00
#define WRITE_PDU_WITH_BODY_LENGTH(length) \
    0x00, 0x02, 0x00, 0x00, 0x00, (uint8_t)(length & 0xFF), (uint8_t)((length >> 8) & 0xFF)
#define WRITE_PDU_WITH_BAD_OPCODE_AND_BODY_LENGTH(length) \
    0x00, 0xAB, 0x00, 0x00, 0x00, (uint8_t)(length & 0xFF), (uint8_t)((length >> 8) & 0xFF)

static int NO_BODY_HEADER_LENGTH = 5;
static int BODY_HEADER_LENGTH = 7;

TEST(SinglePDUNoBodyLengthPasses) {
    static const uint8_t pdu[] = { WRITE_PDU_WITH_NO_BODY_LENGTH };
    HAPError result = HAPPDUVerifyMessage((void*) pdu, NO_BODY_HEADER_LENGTH);
    TEST_ASSERT_EQUAL(result, kHAPError_None);
}

TEST(SinglePDUWithGoodBodyLengthPasses) {
    int pdu1BodyLength = 2;
    int pdu2BodyLength = 5;
    int pdu3BodyLength = 0;

    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;
    int pdu3Length = BODY_HEADER_LENGTH + pdu3BodyLength;

    static const uint8_t pdu1[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength), 0x00, 0x00 };
    static const uint8_t pdu2[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu2BodyLength), 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t pdu3[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu3BodyLength) };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, pdu1Length);
    TEST_ASSERT_EQUAL(result, kHAPError_None);

    result = HAPPDUVerifyMessage((void*) pdu2, pdu2Length);
    TEST_ASSERT_EQUAL(result, kHAPError_None);

    result = HAPPDUVerifyMessage((void*) pdu3, pdu3Length);
    TEST_ASSERT_EQUAL(result, kHAPError_None);
}

TEST(SinglePDUWithBodyOverflowFails) {
    int pdu1Length = NO_BODY_HEADER_LENGTH - 1;
    static const uint8_t pdu1[] = { WRITE_PDU_WITH_NO_BODY_LENGTH };

    int pdu2BodyLength = 0;
    int pdu2ReportedBodyLength = 2;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;
    static const uint8_t pdu2[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedBodyLength) };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, pdu1Length);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);

    result = HAPPDUVerifyMessage((void*) pdu2, pdu2Length);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);
}

TEST(SinglePDUWithBadOpcodeFails) {
    static const uint8_t pdu[] = { WRITE_PDU_WITH_BAD_OPCODE };
    HAPError result = HAPPDUVerifyMessage((void*) pdu, NO_BODY_HEADER_LENGTH);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidState);
}

TEST(MultiPDUWithBadOpcodeFails) {
    int pdu1BodyLength = 0;
    int pdu2BodyLength = 0;
    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    int multiPduLength = pdu1Length + pdu2Length;
    static const uint8_t pdu1[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength),
                                    0x00,
                                    0x00,
                                    WRITE_PDU_WITH_BAD_OPCODE_AND_BODY_LENGTH(pdu2BodyLength) };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);
}

TEST(SinglePDUWithBodyUnderflowFails) {
    int pdu1Length = NO_BODY_HEADER_LENGTH + 1;
    static const uint8_t pdu1[] = { WRITE_PDU_WITH_NO_BODY_LENGTH };

    int pdu2BodyLength = 2;
    int pdu2ReportedBodyLength = 0;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    static const uint8_t pdu2[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedBodyLength), 0x00, 0x00 };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, pdu1Length);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);

    result = HAPPDUVerifyMessage((void*) pdu2, pdu2Length);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);
}

TEST(MultiPDUNoBodyLengthFails) {
    int pdu1BodyLength = 2;
    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = NO_BODY_HEADER_LENGTH;

    int multiPduLength = pdu1Length + pdu2Length;
    static const uint8_t pdu1[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength), 0x00, 0x00, WRITE_PDU_WITH_NO_BODY_LENGTH
    };
    static const uint8_t pdu2[] = {
        WRITE_PDU_WITH_NO_BODY_LENGTH, WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength), 0x00, 0x00
    };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);

    result = HAPPDUVerifyMessage((void*) pdu2, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_InvalidData);
}

TEST(MultiPDUBodyLengthZeroPasses) {
    int pdu1BodyLength = 2;
    int pdu2BodyLength = 0;
    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    int multiPduLength = pdu1Length + pdu2Length;
    static const uint8_t pdu1[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength), 0x00, 0x00, WRITE_PDU_WITH_BODY_LENGTH(pdu2BodyLength)
    };
    static const uint8_t pdu2[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu2BodyLength), WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength), 0x00, 0x00
    };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_None);

    result = HAPPDUVerifyMessage((void*) pdu2, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_None);
}

TEST(MultiPDUWithGoodBodyLengthPasses) {
    int pdu1BodyLength = 2;
    int pdu2BodyLength = 4;
    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    int multiPduLength = pdu1Length + pdu2Length;
    static const uint8_t pdu1[] = { WRITE_PDU_WITH_BODY_LENGTH(pdu1BodyLength),
                                    0x00,
                                    0x00,
                                    WRITE_PDU_WITH_BODY_LENGTH(pdu2BodyLength),
                                    0x00,
                                    0x00,
                                    0x00,
                                    0x00 };

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_EQUAL(result, kHAPError_None);
}

TEST(MultiPDUWithBodyOverflowFails) {
    int pdu1BodyLength = 3;
    int pdu2BodyLength = 5;

    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    int pdu1ReportedLength = 2;
    int pdu2ReportedLength = 5;

    int multiPduLength = pdu1Length + pdu2Length;

    static const uint8_t pdu1[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu1ReportedLength), 0x00, 0x00, 0x00, // Reports 2, actual 3
        WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedLength), 0x00, 0x00, 0x00, 0x00, 0x00
    }; // Reports 5, actual 5

    static const uint8_t pdu2[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedLength), 0x00, 0x00, 0x00, 0x00, 0x00, // Reports 5, actual 5
        WRITE_PDU_WITH_BODY_LENGTH(pdu1ReportedLength), 0x00, 0x00, 0x00
    }; // Reports 2, actual 3

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_NE(result, kHAPError_None);

    result = HAPPDUVerifyMessage((void*) pdu2, multiPduLength);
    TEST_ASSERT_NE(result, kHAPError_None);
}

TEST(MultiPDUWithBodyUnderflowFails) {
    int pdu1BodyLength = 3;
    int pdu2BodyLength = 5;

    int pdu1Length = BODY_HEADER_LENGTH + pdu1BodyLength;
    int pdu2Length = BODY_HEADER_LENGTH + pdu2BodyLength;

    int pdu1ReportedLength = 4;
    int pdu2ReportedLength = 5;

    int multiPduLength = pdu1Length + pdu2Length;

    static const uint8_t pdu1[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu1ReportedLength), 0x00, 0x00, 0x00, // Reports 4, actual 3
        WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedLength), 0x00, 0x00, 0x00, 0x00, 0x00
    }; // Reports 5, actual 5

    static const uint8_t pdu2[] = {
        WRITE_PDU_WITH_BODY_LENGTH(pdu2ReportedLength), 0x00, 0x00, 0x00, 0x00, 0x00, // Reports 5, actual 5
        WRITE_PDU_WITH_BODY_LENGTH(pdu1ReportedLength), 0x00, 0x00, 0x00
    }; // Reports 4, actual 3

    HAPError result = HAPPDUVerifyMessage((void*) pdu1, multiPduLength);
    TEST_ASSERT_NE(result, kHAPError_None);

    result = HAPPDUVerifyMessage((void*) pdu2, multiPduLength);
    TEST_ASSERT_NE(result, kHAPError_None);
}

int main(int argc, char** argv) {
    // HAPPlatformCreate();

    return EXECUTE_TESTS(argc, (const char**) argv);
}

#undef WRITE_PDU_WITH_NO_BODY_LENGTH
#undef WRITE_PDU_WITH_BODY_LENGTH
