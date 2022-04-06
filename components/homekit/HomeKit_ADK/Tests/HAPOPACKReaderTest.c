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

#include "HAPOPACK.h"

#include "HAPPlatform+Init.h"

HAP_DIAGNOSTIC_IGNORED_CLANG("-Wfloat-equal")

int main() {
    HAPError err;
    HAPPlatformCreate();

    HAPOPACKReader reader;

#define CREATE_READER(reader, bytes, numBytes) \
    do { \
        HAPOPACKReaderCreate(reader, bytes, numBytes); \
\
        /* Check that skipping really does not modify reader contents. */ \
        bool found; \
        do { \
            err = HAPOPACKReaderPeekNextType(reader, &found, NULL); \
            HAPAssert(!err); \
\
            if (found) { \
                err = HAPOPACKReaderGetNext(reader, NULL); \
                HAPAssert(!err); \
            } \
        } while (found); \
\
        HAPOPACKReaderCreate(reader, bytes, numBytes); \
    } while (0)

#define CHECK_TYPE(reader, t) \
    do { \
        bool found; \
        HAPOPACKItemType type; \
        err = HAPOPACKReaderPeekNextType(reader, &found, &type); \
        HAPAssert(!err); \
        HAPAssert(found); \
        HAPAssert(type == (t)); \
    } while (0)

    HAPLogDebug(&kHAPLog_Default, "Unexpected input");
    {
        uint8_t tags[] = { 0x00,
                           /*                   */ 0x34,
                           /*                                     */ 0x37,
                           0x38,
                           0x39,
                           0x3A,
                           0x3B,
                           0x3C,
                           0x3D,
                           0x3E,
                           0x3F,
                           /*                         */ 0x65,
                           0x66,
                           0x67,
                           0x68,
                           0x69,
                           0x6A,
                           0x6B,
                           0x6C,
                           0x6D,
                           0x6E,
                           /*                         */ 0x95,
                           0x96,
                           0x97,
                           0x98,
                           0x99,
                           0x9A,
                           0x9B,
                           0x9C,
                           0x9D,
                           0x9E,
                           0xA0,
                           0xA1,
                           0xA2,
                           0xA3,
                           0xA4,
                           0xA5,
                           0xA6,
                           0xA7,
                           0xA8,
                           0xA9,
                           0xAA,
                           0xAB,
                           0xAC,
                           0xAD,
                           0xAE,
                           0xAF,
                           0xB0,
                           0xB1,
                           0xB2,
                           0xB3,
                           0xB4,
                           0xB5,
                           0xB6,
                           0xB7,
                           0xB8,
                           0xB9,
                           0xBA,
                           0xBB,
                           0xBC,
                           0xBD,
                           0xBE,
                           0xBF,
                           0xC0,
                           0xC1,
                           0xC2,
                           0xC3,
                           0xC4,
                           0xC5,
                           0xC6,
                           0xC7,
                           0xC8,
                           0xC9,
                           0xCA,
                           0xCB,
                           0xCC,
                           0xCD,
                           0xCE,
                           0xCF,
                           0xF0,
                           0xF1,
                           0xF2,
                           0xF3,
                           0xF4,
                           0xF5,
                           0xF6,
                           0xF7,
                           0xF8,
                           0xF9,
                           0xFA,
                           0xFB,
                           0xFC,
                           0xFD,
                           0xFE,
                           0xFF };
        for (size_t i = 0; i < sizeof tags; i++) {
            HAPLogDebug(&kHAPLog_Default, "- 0x%02X", tags[i]);

            uint8_t bytes[] = { tags[i] };

            HAPOPACKReaderCreate(&reader, bytes, sizeof bytes);
            bool found;
            err = HAPOPACKReaderPeekNextType(&reader, &found, NULL);
            HAPAssert(err == kHAPError_InvalidData);
        }
    }

    HAPLogDebug(&kHAPLog_Default, "Boolean: true");
    {
        uint8_t bytes[] = { 0x01 };

        CREATE_READER(&reader, bytes, sizeof bytes);
        CHECK_TYPE(&reader, kHAPOPACKItemType_Bool);
        bool actualValue;
        err = HAPOPACKReaderGetNextBool(&reader, &actualValue);
        HAPAssert(!err);

        bool expectedValue = true;
        HAPAssert(actualValue == expectedValue);
    }

    HAPLogDebug(&kHAPLog_Default, "Boolean: false");
    {
        uint8_t bytes[] = { 0x02 };

        CREATE_READER(&reader, bytes, sizeof bytes);
        CHECK_TYPE(&reader, kHAPOPACKItemType_Bool);
        bool actualValue;
        err = HAPOPACKReaderGetNextBool(&reader, &actualValue);
        HAPAssert(!err);

        bool expectedValue = false;
        HAPAssert(actualValue == expectedValue);
    }

    HAPLogDebug(&kHAPLog_Default, "Null");
    {
        uint8_t bytes[] = { 0x04 };

        CREATE_READER(&reader, bytes, sizeof bytes);
        CHECK_TYPE(&reader, kHAPOPACKItemType_Null);
        err = HAPOPACKReaderGetNextNull(&reader);
        HAPAssert(!err);
    }

    HAPLogDebug(&kHAPLog_Default, "UUID: 395F76B2-B377-4D8F-85FA-3CA8F32F5F7C");
    {
        uint8_t bytes[] = { 0x05, 0x39, 0x5F, 0x76, 0xB2, 0xB3, 0x77, 0x4D, 0x8F,
                            0x85, 0xFA, 0x3C, 0xA8, 0xF3, 0x2F, 0x5F, 0x7C };

        CREATE_READER(&reader, bytes, sizeof bytes);
        CHECK_TYPE(&reader, kHAPOPACKItemType_UUID);
        HAPUUID actualValue;
        err = HAPOPACKReaderGetNextUUID(&reader, &actualValue);
        HAPAssert(!err);

        HAPUUID expectedValue = {
            { 0x7C, 0x5F, 0x2F, 0xF3, 0xA8, 0x3C, 0xFA, 0x85, 0x8F, 0x4D, 0x77, 0xB3, 0xB2, 0x76, 0x5F, 0x39 }
        };
        HAPAssert(HAPUUIDAreEqual(&actualValue, &expectedValue));
    }

    HAPLogDebug(&kHAPLog_Default, "Date: 29 Jun 2007 09:41:00");
    {
        uint8_t bytes[] = { 0x06, 0x00, 0x00, 0x00, 0x96, 0xB6, 0xED, 0xC0, 0x41 };

        CREATE_READER(&reader, bytes, sizeof bytes);
        CHECK_TYPE(&reader, kHAPOPACKItemType_Date);
        HAPOPACKDate actualValue;
        err = HAPOPACKReaderGetNextDate(&reader, &actualValue);
        HAPAssert(!err);

        HAPOPACKDate expectedValue = 568028460;
        HAPAssert(actualValue == expectedValue);
    }

    /* Number: */ {
        int64_t values[] = { -1,
                             0,
                             1,
                             2,
                             3,
                             4,
                             5,
                             6,
                             7,
                             8,
                             9,
                             10,
                             11,
                             12,
                             13,
                             14,
                             15,
                             16,
                             17,
                             18,
                             19,
                             20,
                             21,
                             22,
                             23,
                             24,
                             25,
                             26,
                             27,
                             28,
                             29,
                             30,
                             31,
                             32,
                             33,
                             34,
                             35,
                             36,
                             37,
                             38,
                             39,
                             INT8_MIN,
                             -117,
                             43,
                             INT8_MAX,
                             INT16_MIN,
                             -9306,
                             14263,
                             INT16_MAX,
                             INT32_MIN,
                             -1591251256,
                             508454127,
                             INT32_MAX,
                             INT64_MIN,
                             -3850230932105999879,
                             8378242252125190412,
                             INT64_MAX };

        for (size_t i = 0; i < HAPArrayCount(values); i++) {
            HAPLogDebug(&kHAPLog_Default, "Number: 0x%016llX", (unsigned long long) values[i]);

            // Compact encoding.
            if (values[i] >= -1 && values[i] <= 39) {
                HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
                uint8_t bytes[] = { (uint8_t)(kHAPOPACKTag_Negative1 + values[i] - -1) };
                HAPAssert(bytes[0] >= kHAPOPACKTag_Negative1 && bytes[0] <= kHAPOPACKTag_39);

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Int8 encoding.
            if (values[i] >= INT8_MIN && values[i] <= INT8_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- Int8 encoding");
                uint8_t bytes[] = { kHAPOPACKTag_Int8, (uint8_t) values[i] };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Int16 encoding.
            if (values[i] >= INT16_MIN && values[i] <= INT16_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- Int16 encoding");
                uint8_t bytes[] = { kHAPOPACKTag_Int16, HAPExpandLittleInt16(values[i]) };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Int32 encoding.
            if (values[i] >= INT32_MIN && values[i] <= INT32_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- Int32 encoding");
                uint8_t bytes[] = { kHAPOPACKTag_Int32, HAPExpandLittleInt32(values[i]) };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Int64 encoding.
            {
                HAPLogDebug(&kHAPLog_Default, "- Int64 encoding");
                uint8_t bytes[] = { kHAPOPACKTag_Int64, HAPExpandLittleInt64(values[i]) };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Float encoding.
            if (values[i] == (int64_t)(float) values[i]) {
                HAPLogDebug(&kHAPLog_Default, "- Float encoding");
                float val = values[i];
                uint32_t bitPattern = HAPFloatGetBitPattern(val);
                uint8_t bytes[] = { kHAPOPACKTag_Float, HAPExpandLittleUInt32(bitPattern) };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }

            // Double encoding.
            if (values[i] == (int64_t)(double) values[i]) {
                HAPLogDebug(&kHAPLog_Default, "- Double encoding");
                double val = values[i];
                uint64_t bitPattern = HAPDoubleGetBitPattern(val);
                uint8_t bytes[] = { kHAPOPACKTag_Double, HAPExpandLittleUInt64(bitPattern) };

                CREATE_READER(&reader, bytes, sizeof bytes);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == values[i]);
            }
        }
    }

    /* String: */ {
        const char* values[] = { "",
                                 "1",
                                 "12",
                                 "123",
                                 "1234",
                                 "12345",
                                 "123456",
                                 "1234567",
                                 "12345678",
                                 "123456789",
                                 "1234567890",
                                 "12345678901",
                                 "123456789012",
                                 "1234567890123",
                                 "12345678901234",
                                 "123456789012345",
                                 "1234567890123456",
                                 "12345678901234567",
                                 "123456789012345678",
                                 "1234567890123456789",
                                 "12345678901234567890",
                                 "123456789012345678901",
                                 "1234567890123456789012",
                                 "12345678901234567890123",
                                 "123456789012345678901234",
                                 "1234567890123456789012345",
                                 "12345678901234567890123456",
                                 "123456789012345678901234567",
                                 "1234567890123456789012345678",
                                 "12345678901234567890123456789",
                                 "123456789012345678901234567890",
                                 "1234567890123456789012345678901",
                                 "12345678901234567890123456789012",
                                 "123456789012345678901234567890123",
                                 "12345678901234567890123456789012345678901234567890123456789012345678901234567890" };

        for (uint64_t i = 0; i < HAPArrayCount(values); i++) {
            HAPLogDebug(&kHAPLog_Default, "String: %s", values[i]);

            uint64_t numValueBytes = HAPStringGetNumBytes(values[i]);

            // Compact encoding.
            if (numValueBytes <= 32) {
                HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = (uint8_t)(kHAPOPACKTag_String0 + numValueBytes);
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }

            // UInt8 encoding.
            if (numValueBytes <= UINT8_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt8 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_StringUInt8;
                bytes[o++] = (uint8_t) numValueBytes;
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }

            // UInt16 encoding.
            if (numValueBytes <= UINT16_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt16 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_StringUInt16;
                HAPWriteLittleUInt16(&bytes[o], numValueBytes);
                o += sizeof(uint16_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }

            // UInt32 encoding.
            if (numValueBytes <= UINT32_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt16 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_StringUInt32;
                HAPWriteLittleUInt32(&bytes[o], numValueBytes);
                o += sizeof(uint32_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }

            // UInt64 encoding.
            {
                HAPLogDebug(&kHAPLog_Default, "- UInt64 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_StringUInt64;
                HAPWriteLittleUInt64(&bytes[o], numValueBytes);
                o += sizeof(uint64_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }

            // Implicit length encoding.
            {
                HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_String;
                HAPRawBufferCopyBytes(&bytes[o], values[i], numValueBytes);
                o += numValueBytes;
                bytes[o++] = '\0';
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, values[i]));
            }
        }
    }

    /* Data: */ {
        struct {
            uint64_t numBytes;
            uint8_t* bytes;
        } values[] = {
            { 0, (uint8_t[]) { 0x00 } },
            { 1, (uint8_t[]) { 0x00 } },
            { 2, (uint8_t[]) { 0x00, 0x01 } },
            { 3, (uint8_t[]) { 0x00, 0x01, 0x02 } },
            { 4, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03 } },
            { 5, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04 } },
            { 6, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } },
            { 7, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
            { 8, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } },
            { 9, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 } },
            { 10, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 } },
            { 11, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A } },
            { 12, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B } },
            { 13, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C } },
            { 14, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D } },
            { 15,
              (uint8_t[]) {
                      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E } },
            { 16,
              (uint8_t[]) { 0x00,
                            0x01,
                            0x02,
                            0x03,
                            0x04,
                            0x05,
                            0x06,
                            0x07,
                            0x08,
                            0x09,
                            0x0A,
                            0x0B,
                            0x0C,
                            0x0D,
                            0x0E,
                            0x0F } },
            { 17,
              (uint8_t[]) { 0x00,
                            0x01,
                            0x02,
                            0x03,
                            0x04,
                            0x05,
                            0x06,
                            0x07,
                            0x08,
                            0x09,
                            0x0A,
                            0x0B,
                            0x0C,
                            0x0D,
                            0x0E,
                            0x0F,
                            0x10 } },
            { 18,
              (uint8_t[]) { 0x00,
                            0x01,
                            0x02,
                            0x03,
                            0x04,
                            0x05,
                            0x06,
                            0x07,
                            0x08,
                            0x09,
                            0x0A,
                            0x0B,
                            0x0C,
                            0x0D,
                            0x0E,
                            0x0F,
                            0x10,
                            0x11 } },
            { 19,
              (uint8_t[]) { 0x00,
                            0x01,
                            0x02,
                            0x03,
                            0x04,
                            0x05,
                            0x06,
                            0x07,
                            0x08,
                            0x09,
                            0x0A,
                            0x0B,
                            0x0C,
                            0x0D,
                            0x0E,
                            0x0F,
                            0x10,
                            0x11,
                            0x12 } },
            { 20, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                                0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 } },
            { 21, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14 } },
            { 22, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 } },
            { 23, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } },
            { 24, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 } },
            { 25, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 } },
            { 26, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19 } },
            { 27, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                                0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A } },
            { 28, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                                0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B } },
            { 29,
              (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                            0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C } },
            { 30, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                                0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
                                0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D } },
            { 31, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E } },
            { 32, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F } },
            { 33, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                                0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 } },
            { 255,
              (uint8_t[]) {
                      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
                      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
                      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
                      0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
                      0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
                      0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
                      0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
                      0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
                      0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                      0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
                      0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
                      0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
                      0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
                      0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE } },
            { 256,
              (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                            0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
                            0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
                            0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
                            0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
                            0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
                            0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                            0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                            0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86,
                            0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
                            0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4,
                            0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
                            0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2,
                            0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1,
                            0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0,
                            0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
                            0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE,
                            0xFF } },
        };

        for (size_t i = 0; i < HAPArrayCount(values); i++) {
            HAPLogBufferDebug(
                    &kHAPLog_Default,
                    values[i].bytes,
                    values[i].numBytes,
                    "Data: %lu bytes.",
                    (unsigned long) values[i].numBytes);

            // Compact encoding.
            if (values[i].numBytes <= 32) {
                HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = (uint8_t)(kHAPOPACKTag_Data0 + values[i].numBytes);
                HAPRawBufferCopyBytes(&bytes[o], values[i].bytes, values[i].numBytes);
                o += values[i].numBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }

            // UInt8 encoding.
            if (values[i].numBytes <= UINT8_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt8 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_DataUInt8;
                bytes[o++] = (uint8_t) values[i].numBytes;
                HAPRawBufferCopyBytes(&bytes[o], values[i].bytes, values[i].numBytes);
                o += values[i].numBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }

            // UInt8 encoding.
            if (values[i].numBytes <= UINT16_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt16 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_DataUInt16;
                HAPWriteLittleUInt16(&bytes[o], values[i].numBytes);
                o += sizeof(uint16_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i].bytes, values[i].numBytes);
                o += values[i].numBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }

            // UInt32 encoding.
            if (values[i].numBytes <= UINT32_MAX) {
                HAPLogDebug(&kHAPLog_Default, "- UInt32 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_DataUInt32;
                HAPWriteLittleUInt32(&bytes[o], values[i].numBytes);
                o += sizeof(uint32_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i].bytes, values[i].numBytes);
                o += values[i].numBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }

            // UInt64 encoding.
            {
                HAPLogDebug(&kHAPLog_Default, "- UInt64 encoding");
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_DataUInt64;
                HAPWriteLittleUInt64(&bytes[o], values[i].numBytes);
                o += sizeof(uint64_t);
                HAPRawBufferCopyBytes(&bytes[o], values[i].bytes, values[i].numBytes);
                o += values[i].numBytes;
                HAPAssert(o <= sizeof bytes);

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }

            // Implicit length encoding.
            {
                uint8_t bytes[1024];
                size_t o = 0;
                bytes[o++] = kHAPOPACKTag_Data;
                size_t remainingBytes = values[i].numBytes;
                size_t level = 0;
                while (remainingBytes || level) {
                    uint8_t n;
                    HAPPlatformRandomNumberFill(&n, sizeof n);
                    n %= 16;
                    if (values[i].numBytes - remainingBytes < 128) {
                        n = 15;
                    }
                    if (remainingBytes < n) {
                        n = (uint8_t) remainingBytes;
                    }

                    uint8_t kind;
                    HAPPlatformRandomNumberFill(&kind, sizeof kind);
                    if (sizeof bytes - o < 128 && level) {
                        kind = 1;
                    }
                    switch (kind % 7) {
                        case 0: {
                            if (level < 16) {
                                level++;
                                bytes[o++] = kHAPOPACKTag_Data;
                            }
                            break;
                        }
                        case 1: {
                            if (level) {
                                bytes[o++] = kHAPOPACKTag_Terminator;
                                level--;
                            }
                            break;
                        }
                        case 2: {
                            HAPAssert(n <= 32);
                            bytes[o++] = (uint8_t)(kHAPOPACKTag_Data0 + n);
                            HAPRawBufferCopyBytes(&bytes[o], &values[i].bytes[values[i].numBytes - remainingBytes], n);
                            o += n;
                            remainingBytes -= n;
                            break;
                        }
                        case 3: {
                            HAPAssert(n <= UINT8_MAX);
                            bytes[o++] = kHAPOPACKTag_DataUInt8;
                            bytes[o++] = (uint8_t) n;
                            HAPRawBufferCopyBytes(&bytes[o], &values[i].bytes[values[i].numBytes - remainingBytes], n);
                            o += n;
                            remainingBytes -= n;
                            break;
                        }
                        case 4: {
                            bytes[o++] = kHAPOPACKTag_DataUInt16;
                            HAPWriteLittleUInt16(&bytes[o], (uint16_t) n);
                            o += sizeof(uint16_t);
                            HAPRawBufferCopyBytes(&bytes[o], &values[i].bytes[values[i].numBytes - remainingBytes], n);
                            o += n;
                            remainingBytes -= n;
                            break;
                        }
                        case 5: {
                            bytes[o++] = kHAPOPACKTag_DataUInt32;
                            HAPWriteLittleUInt32(&bytes[o], (uint32_t) n);
                            o += sizeof(uint32_t);
                            HAPRawBufferCopyBytes(&bytes[o], &values[i].bytes[values[i].numBytes - remainingBytes], n);
                            o += n;
                            remainingBytes -= n;
                            break;
                        }
                        case 6: {
                            bytes[o++] = kHAPOPACKTag_DataUInt64;
                            HAPWriteLittleUInt64(&bytes[o], (uint64_t) n);
                            o += sizeof(uint64_t);
                            HAPRawBufferCopyBytes(&bytes[o], &values[i].bytes[values[i].numBytes - remainingBytes], n);
                            o += n;
                            remainingBytes -= n;
                            break;
                        }
                        default:
                            HAPFatalError();
                    }
                }
                bytes[o++] = kHAPOPACKTag_Terminator;
                HAPAssert(o <= sizeof bytes);
                HAPLogBufferDebug(&kHAPLog_Default, bytes, o, "- Implicit length encoding");

                CREATE_READER(&reader, bytes, o);
                CHECK_TYPE(&reader, kHAPOPACKItemType_Data);
                void* actualValueBytes;
                size_t numActualValueBytes;
                err = HAPOPACKReaderGetNextData(&reader, &actualValueBytes, &numActualValueBytes);
                HAPAssert(!err);
                HAPAssert(numActualValueBytes == values[i].numBytes);
                HAPAssert(HAPRawBufferAreEqual(actualValueBytes, values[i].bytes, values[i].numBytes));
            }
        }
    }

    HAPLogDebug(&kHAPLog_Default, "Array: []");
    {
        // Compact encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
            uint8_t bytes[] = { 0xD0 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Array);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextArray(&reader, &subReader);
            HAPAssert(!err);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Implicit length encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
            uint8_t bytes[] = { 0xDF, 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Array);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextArray(&reader, &subReader);
            HAPAssert(!err);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
    }

    HAPLogDebug(&kHAPLog_Default, "Array: [42]");
    {
        // Compact encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
            uint8_t bytes[] = { 0xD1, 0x30, 42 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Array);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextArray(&reader, &subReader);
            HAPAssert(!err);

            CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
            int64_t actualValue;
            err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
            HAPAssert(!err);
            HAPAssert(actualValue == 42);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Implicit length encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
            uint8_t bytes[] = { 0xDF, 0x30, 42, 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Array);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextArray(&reader, &subReader);
            HAPAssert(!err);

            CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
            int64_t actualValue;
            err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
            HAPAssert(!err);
            HAPAssert(actualValue == 42);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
    }

    HAPLogDebug(&kHAPLog_Default, "Dictionary: [:]");
    {
        // Compact encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
            uint8_t bytes[] = { 0xE0 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Implicit length encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
            uint8_t bytes[] = { 0xEF, 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
    }

    HAPLogDebug(&kHAPLog_Default, "Dictionary: [\"the answer\": 42]");
    {
        // Compact encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
            uint8_t bytes[] = { 0xE1, 0x4A, 't', 'h', 'e', ' ', 'a', 'n', 's', 'w', 'e', 'r', 0x30, 42 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "the answer"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == 42);
            }

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Implicit length encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
            uint8_t bytes[] = { 0xEF, 0x4A, 't', 'h', 'e', ' ', 'a', 'n', 's', 'w', 'e', 'r', 0x30, 42, 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "the answer"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == 42);
            }

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
    }

    HAPLogDebug(&kHAPLog_Default, "[\"a\": \"x\", \"b\": 0, \"c\": \"y\"]");
    {
        // Compact encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Compact encoding");
            uint8_t bytes[] = { 0xE3, 0x41, 'a', 0x41, 'x', 0x41, 'b', 0x08, 0x41, 'c', 0x41, 'y' };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "a"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "x"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "b"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == 0);
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "c"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "y"));
            }

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Implicit length encoding.
        {
            HAPLogDebug(&kHAPLog_Default, "- Implicit length encoding");
            uint8_t bytes[] = { 0xEF, 0x41, 'a', 0x41, 'x', 0x41, 'b', 0x08, 0x41, 'c', 0x41, 'y', 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKReader subReader;
            err = HAPOPACKReaderGetNextDictionary(&reader, &subReader);
            HAPAssert(!err);

            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "a"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "x"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "b"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == 0);
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "c"));
            }
            {
                CHECK_TYPE(&subReader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&subReader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "y"));
            }

            bool found;
            err = HAPOPACKReaderPeekNextType(&subReader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Get all.
        {
            HAPLogDebug(&kHAPLog_Default, "- Get all");
            uint8_t bytes[] = { 0xEF, 0x41, 'a', 0x41, 'x', 0x41, 'b', 0x08, 0x41, 'c', 0x41, 'y', 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKStringDictionaryElement aElement, bElement, cElement, dElement;
            aElement.key = "a";
            bElement.key = "b";
            cElement.key = "c";
            dElement.key = "d";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &reader,
                    (HAPOPACKStringDictionaryElement* const[]) { &aElement, &dElement, &bElement, &cElement, NULL });
            HAPAssert(!err);

            {
                HAPAssert(aElement.value.exists);
                CHECK_TYPE(&aElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&aElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "x"));
            }
            {
                HAPAssert(bElement.value.exists);
                CHECK_TYPE(&bElement.value.reader, kHAPOPACKItemType_Number);
                int64_t actualValue;
                err = HAPOPACKReaderGetNextInt(&bElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(actualValue == 0);
            }
            {
                HAPAssert(cElement.value.exists);
                CHECK_TYPE(&cElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&cElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "y"));
            }
            { HAPAssert(!dElement.value.exists); }

            bool found;
            err = HAPOPACKReaderPeekNextType(&reader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }

        // Get some.
        {
            HAPLogDebug(&kHAPLog_Default, "- Get some");
            // {'a': 'x', 'b': 0, 'c': 'y'}
            uint8_t bytes[] = { 0xEF, 0x41, 'a', 0x41, 'x', 0x41, 'b', 0x08, 0x41, 'c', 0x41, 'y', 0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKStringDictionaryElement aElement, cElement;
            aElement.key = "a";
            cElement.key = "c";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &reader, (HAPOPACKStringDictionaryElement* const[]) { &aElement, &cElement, NULL });
            HAPAssert(!err);

            {
                HAPAssert(aElement.value.exists);
                CHECK_TYPE(&aElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&aElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "x"));
            }
            {
                HAPAssert(cElement.value.exists);
                CHECK_TYPE(&cElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&cElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "y"));
            }

            bool found = false;
            err = HAPOPACKReaderPeekNextType(&reader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
        // Get some, nested skip.
        {
            HAPLogDebug(&kHAPLog_Default, "- Get some, nested skip");
            // {'a': 'x', 'b': ['b1','b2',0], 'c': 'y'}
            uint8_t bytes[] = { 0xEF, 0x41, 'a', 0x41, 'x',  0x41, 'b', 0xD3, 0x42, 'b',
                                '1',  0x42, 'b', '2',  0x08, 0x41, 'c', 0x41, 'y',  0x03 };

            CREATE_READER(&reader, bytes, sizeof bytes);
            CHECK_TYPE(&reader, kHAPOPACKItemType_Dictionary);
            HAPOPACKStringDictionaryElement aElement, cElement;
            aElement.key = "a";
            cElement.key = "c";
            err = HAPOPACKStringDictionaryReaderGetAll(
                    &reader, (HAPOPACKStringDictionaryElement* const[]) { &aElement, &cElement, NULL });
            HAPAssert(!err);

            {
                HAPAssert(aElement.value.exists);
                CHECK_TYPE(&aElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&aElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "x"));
            }
            {
                HAPAssert(cElement.value.exists);
                CHECK_TYPE(&cElement.value.reader, kHAPOPACKItemType_String);
                char* actualValue;
                err = HAPOPACKReaderGetNextString(&cElement.value.reader, &actualValue);
                HAPAssert(!err);
                HAPAssert(HAPStringAreEqual(actualValue, "y"));
            }

            bool found = false;
            err = HAPOPACKReaderPeekNextType(&reader, &found, NULL);
            HAPAssert(!err);
            HAPAssert(!found);
        }
    }

} // main()
