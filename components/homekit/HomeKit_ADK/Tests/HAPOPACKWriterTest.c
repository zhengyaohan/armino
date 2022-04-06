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
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#include "HAPOPACK.h"

#include "HAPPlatform+Init.h"

int main() {
    HAPError err;
    HAPPlatformCreate();

    uint8_t bytes[128 * 1024];
    HAPOPACKWriter writer;

#define VALIDATE_WRITER_BUFFER(writer, expectedBytes, numExpectedBytes) \
    do { \
        void* actualBytes; \
        size_t numActualBytes; \
        HAPOPACKWriterGetBuffer(writer, &actualBytes, &numActualBytes); \
        HAPAssert(numActualBytes == (numExpectedBytes)); \
        HAPAssert(HAPRawBufferAreEqual(actualBytes, expectedBytes, numActualBytes)); \
    } while (0)

    HAPLogDebug(&kHAPLog_Default, "Boolean: true");
    {
        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendBool(&writer, true);
        HAPAssert(!err);

        uint8_t expectedBytes[] = { 0x01 };
        VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
    }

    HAPLogDebug(&kHAPLog_Default, "Boolean: false");
    {
        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendBool(&writer, false);
        HAPAssert(!err);

        uint8_t expectedBytes[] = { 0x02 };
        VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
    }

    HAPLogDebug(&kHAPLog_Default, "Null");
    {
        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendNull(&writer);
        HAPAssert(!err);

        uint8_t expectedBytes[] = { 0x04 };
        VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
    }

    HAPLogDebug(&kHAPLog_Default, "Date: 29 Jun 2007 09:41:00");
    {
        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendDate(&writer, 568028460);
        HAPAssert(!err);

        uint8_t expectedBytes[] = { 0x06, 0x00, 0x00, 0x00, 0x96, 0xB6, 0xED, 0xC0, 0x41 };
        VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
    }

    HAPLogDebug(&kHAPLog_Default, "UUID: 395F76B2-B377-4D8F-85FA-3CA8F32F5F7C");
    {
        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendUUID(
                &writer,
                &(const HAPUUID) { { 0x7C,
                                     0x5F,
                                     0x2F,
                                     0xF3,
                                     0xA8,
                                     0x3C,
                                     0xFA,
                                     0x85,
                                     0x8F,
                                     0x4D,
                                     0x77,
                                     0xB3,
                                     0xB2,
                                     0x76,
                                     0x5F,
                                     0x39 } });
        HAPAssert(!err);

        uint8_t expectedBytes[] = { 0x05, 0x39, 0x5F, 0x76, 0xB2, 0xB3, 0x77, 0x4D, 0x8F,
                                    0x85, 0xFA, 0x3C, 0xA8, 0xF3, 0x2F, 0x5F, 0x7C };
        VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
    }

    /* Number: */ { { int64_t values[] = { -1,
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
    struct {
        size_t numBytes;
        uint8_t* bytes;
    } expected[] = { { 1, (uint8_t[]) { 0x07 } },

                     { 1, (uint8_t[]) { 0x08 } },
                     { 1, (uint8_t[]) { 0x09 } },
                     { 1, (uint8_t[]) { 0x0A } },
                     { 1, (uint8_t[]) { 0x0B } },
                     { 1, (uint8_t[]) { 0x0C } },
                     { 1, (uint8_t[]) { 0x0D } },
                     { 1, (uint8_t[]) { 0x0E } },
                     { 1, (uint8_t[]) { 0x0F } },
                     { 1, (uint8_t[]) { 0x10 } },
                     { 1, (uint8_t[]) { 0x11 } },
                     { 1, (uint8_t[]) { 0x12 } },
                     { 1, (uint8_t[]) { 0x13 } },
                     { 1, (uint8_t[]) { 0x14 } },
                     { 1, (uint8_t[]) { 0x15 } },
                     { 1, (uint8_t[]) { 0x16 } },
                     { 1, (uint8_t[]) { 0x17 } },
                     { 1, (uint8_t[]) { 0x18 } },
                     { 1, (uint8_t[]) { 0x19 } },
                     { 1, (uint8_t[]) { 0x1A } },
                     { 1, (uint8_t[]) { 0x1B } },
                     { 1, (uint8_t[]) { 0x1C } },
                     { 1, (uint8_t[]) { 0x1D } },
                     { 1, (uint8_t[]) { 0x1E } },
                     { 1, (uint8_t[]) { 0x1F } },
                     { 1, (uint8_t[]) { 0x20 } },
                     { 1, (uint8_t[]) { 0x21 } },
                     { 1, (uint8_t[]) { 0x22 } },
                     { 1, (uint8_t[]) { 0x23 } },
                     { 1, (uint8_t[]) { 0x24 } },
                     { 1, (uint8_t[]) { 0x25 } },
                     { 1, (uint8_t[]) { 0x26 } },
                     { 1, (uint8_t[]) { 0x27 } },
                     { 1, (uint8_t[]) { 0x28 } },
                     { 1, (uint8_t[]) { 0x29 } },
                     { 1, (uint8_t[]) { 0x2A } },
                     { 1, (uint8_t[]) { 0x2B } },
                     { 1, (uint8_t[]) { 0x2C } },
                     { 1, (uint8_t[]) { 0x2D } },
                     { 1, (uint8_t[]) { 0x2E } },

                     { 1, (uint8_t[]) { 0x2F } },

                     { 2, (uint8_t[]) { 0x30, (uint8_t) INT8_MIN } },
                     { 2, (uint8_t[]) { 0x30, (uint8_t) -117 } },
                     { 2, (uint8_t[]) { 0x30, (uint8_t) 43 } },
                     { 2, (uint8_t[]) { 0x30, (uint8_t) INT8_MAX } },

                     { 3, (uint8_t[]) { 0x31, HAPExpandLittleInt16(INT16_MIN) } },
                     { 3, (uint8_t[]) { 0x31, HAPExpandLittleInt16(-9306) } },
                     { 3, (uint8_t[]) { 0x31, HAPExpandLittleInt16(14263) } },
                     { 3, (uint8_t[]) { 0x31, HAPExpandLittleInt16(INT16_MAX) } },

                     { 5, (uint8_t[]) { 0x32, HAPExpandLittleInt32(INT32_MIN) } },
                     { 5, (uint8_t[]) { 0x32, HAPExpandLittleInt32(-1591251256) } },
                     { 5, (uint8_t[]) { 0x32, HAPExpandLittleInt32(508454127) } },
                     { 5, (uint8_t[]) { 0x32, HAPExpandLittleInt32(INT32_MAX) } },

                     { 9, (uint8_t[]) { 0x33, HAPExpandLittleInt64(INT64_MIN) } },
                     { 9, (uint8_t[]) { 0x33, HAPExpandLittleInt64(-3850230932105999879) } },
                     { 9, (uint8_t[]) { 0x33, HAPExpandLittleInt64(8378242252125190412) } },
                     { 9, (uint8_t[]) { 0x33, HAPExpandLittleInt64(INT64_MAX) } } };

    for (size_t i = 0; i < HAPArrayCount(values); i++) {
        HAPLogDebug(&kHAPLog_Default, "Number: 0x%016llX", (unsigned long long) values[i]);

        // Int.
        {
            HAPLogDebug(&kHAPLog_Default, "- Int");
            HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
            err = HAPOPACKWriterAppendInt(&writer, values[i]);
            HAPAssert(!err);
            VALIDATE_WRITER_BUFFER(&writer, expected[i].bytes, expected[i].numBytes);
        }

        // Number.
        {
            HAPLogDebug(&kHAPLog_Default, "- Number");
            HAPOPACKNumber number;
            HAPOPACKNumberCreate(&number, kHAPOPACKNumberType_Int64, &values[i]);
            HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
            err = HAPOPACKWriterAppendNumber(&writer, &number);
            HAPAssert(!err);
            VALIDATE_WRITER_BUFFER(&writer, expected[i].bytes, expected[i].numBytes);
        }
    }
}
{
    for (size_t i = 0; i < 10; i++) {
        double value;
        HAPPlatformRandomNumberFill(&value, sizeof value);

        HAPLogDebug(&kHAPLog_Default, "Number: %g", value);

        uint64_t bitPattern = HAPDoubleGetBitPattern(value);
        uint8_t expectedBytes[] = { 0x36, HAPExpandLittleUInt64(bitPattern) };

        // Float.
        {
            HAPLogDebug(&kHAPLog_Default, "- Float");
            HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
            err = HAPOPACKWriterAppendFloat(&writer, value);
            HAPAssert(!err);
            VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
        }

        // Number.
        {
            HAPLogDebug(&kHAPLog_Default, "- Number");
            HAPOPACKNumber number;
            HAPOPACKNumberCreate(&number, kHAPOPACKNumberType_Double, &value);
            HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
            err = HAPOPACKWriterAppendNumber(&writer, &number);
            HAPAssert(!err);
            VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
        }
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
    struct {
        size_t numBytes;
        uint8_t* bytes;
    } expected[] = {
        { 1, (uint8_t[]) { 0x40 } },
        { 2, (uint8_t[]) { 0x41, '1' } },
        { 3, (uint8_t[]) { 0x42, '1', '2' } },
        { 4, (uint8_t[]) { 0x43, '1', '2', '3' } },
        { 5, (uint8_t[]) { 0x44, '1', '2', '3', '4' } },
        { 6, (uint8_t[]) { 0x45, '1', '2', '3', '4', '5' } },
        { 7, (uint8_t[]) { 0x46, '1', '2', '3', '4', '5', '6' } },
        { 8, (uint8_t[]) { 0x47, '1', '2', '3', '4', '5', '6', '7' } },
        { 9, (uint8_t[]) { 0x48, '1', '2', '3', '4', '5', '6', '7', '8' } },
        { 10, (uint8_t[]) { 0x49, '1', '2', '3', '4', '5', '6', '7', '8', '9' } },
        { 11, (uint8_t[]) { 0x4A, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' } },
        { 12, (uint8_t[]) { 0x4B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1' } },
        { 13, (uint8_t[]) { 0x4C, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2' } },
        { 14, (uint8_t[]) { 0x4D, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3' } },
        { 15, (uint8_t[]) { 0x4E, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4' } },
        { 16, (uint8_t[]) { 0x4F, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5' } },
        { 17, (uint8_t[]) { 0x50, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6' } },
        { 18,
          (uint8_t[]) { 0x51, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7' } },
        { 19,
          (uint8_t[]) {
                  0x52, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8' } },
        { 20, (uint8_t[]) { 0x53, '1', '2', '3', '4', '5', '6', '7', '8', '9',
                            '0',  '1', '2', '3', '4', '5', '6', '7', '8', '9' } },
        { 21, (uint8_t[]) { 0x54, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                            '1',  '2', '3', '4', '5', '6', '7', '8', '9', '0' } },
        { 22, (uint8_t[]) { 0x55, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                            '1',  '2', '3', '4', '5', '6', '7', '8', '9', '0', '1' } },
        { 23, (uint8_t[]) { 0x56, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1',
                            '2',  '3', '4', '5', '6', '7', '8', '9', '0', '1', '2' } },
        { 24, (uint8_t[]) { 0x57, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1',
                            '2',  '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3' } },
        { 25, (uint8_t[]) { 0x58, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2',
                            '3',  '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4' } },
        { 26, (uint8_t[]) { 0x59, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2',
                            '3',  '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5' } },
        { 27, (uint8_t[]) { 0x5A, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
                            '4',  '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6' } },
        { 28, (uint8_t[]) { 0x5B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
                            '4',  '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7' } },
        { 29, (uint8_t[]) { 0x5C, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4',
                            '5',  '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8' } },
        { 30, (uint8_t[]) { 0x5D, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4',
                            '5',  '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' } },
        { 31, (uint8_t[]) { 0x5E, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5',
                            '6',  '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' } },
        { 32, (uint8_t[]) { 0x5F, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5',
                            '6',  '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1' } },
        { 33, (uint8_t[]) { 0x60, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
                            '7',  '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2' } },
        { 35, (uint8_t[]) { 0x6F, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7',
                            '8',  '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '\0' } },
        { 82, (uint8_t[]) { 0x6F, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
                            '7',  '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3',
                            '4',  '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
                            '1',  '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7',
                            '8',  '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '\0' } }
    };

    for (size_t i = 0; i < HAPArrayCount(values); i++) {
        HAPLogDebug(&kHAPLog_Default, "String: %s", values[i]);

        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendString(&writer, values[i]);
        HAPAssert(!err);

        VALIDATE_WRITER_BUFFER(&writer, expected[i].bytes, expected[i].numBytes);
    }
}

/* Data: */ {
    struct {
        size_t numBytes;
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
          (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E } },
        { 16,
          (uint8_t[]) {
                  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F } },
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
        { 29, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                            0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C } },
        { 30,
          (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                        0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D } },
        { 31,
          (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E } },
        { 32, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F } },
        { 33, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 } },
        { 255,
          (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
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
        { 256, (uint8_t[]) { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
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
    struct {
        size_t numBytes;
        uint8_t* bytes;
    } expected[] = {
        { 1, (uint8_t[]) { 0x70 } },
        { 2, (uint8_t[]) { 0x71, 0x00 } },
        { 3, (uint8_t[]) { 0x72, 0x00, 0x01 } },
        { 4, (uint8_t[]) { 0x73, 0x00, 0x01, 0x02 } },
        { 5, (uint8_t[]) { 0x74, 0x00, 0x01, 0x02, 0x03 } },
        { 6, (uint8_t[]) { 0x75, 0x00, 0x01, 0x02, 0x03, 0x04 } },
        { 7, (uint8_t[]) { 0x76, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } },
        { 8, (uint8_t[]) { 0x77, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } },
        { 9, (uint8_t[]) { 0x78, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 } },
        { 10, (uint8_t[]) { 0x79, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 } },
        { 11, (uint8_t[]) { 0x7A, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 } },
        { 12, (uint8_t[]) { 0x7B, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A } },
        { 13, (uint8_t[]) { 0x7C, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B } },
        { 14, (uint8_t[]) { 0x7D, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C } },
        { 15,
          (uint8_t[]) { 0x7E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D } },
        { 16,
          (uint8_t[]) {
                  0x7F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E } },
        { 17,
          (uint8_t[]) { 0x80,
                        0x00,
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
        { 18,
          (uint8_t[]) { 0x81,
                        0x00,
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
        { 19,
          (uint8_t[]) { 0x82,
                        0x00,
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
        { 20, (uint8_t[]) { 0x83, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12 } },
        { 21, (uint8_t[]) { 0x84, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 } },
        { 22, (uint8_t[]) { 0x85, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14 } },
        { 23, (uint8_t[]) { 0x86, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 } },
        { 24, (uint8_t[]) { 0x87, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
                            0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } },
        { 25, (uint8_t[]) { 0x88, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                            0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 } },
        { 26, (uint8_t[]) { 0x89, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                            0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18 } },
        { 27, (uint8_t[]) { 0x8A, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                            0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19 } },
        { 28, (uint8_t[]) { 0x8B, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                            0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A } },
        { 29, (uint8_t[]) { 0x8C, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                            0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B } },
        { 30,
          (uint8_t[]) { 0x8D, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                        0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C } },
        { 31,
          (uint8_t[]) { 0x8E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                        0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D } },
        { 32, (uint8_t[]) { 0x8F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
                            0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E } },
        { 33, (uint8_t[]) { 0x90, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
                            0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F } },
        { 35, (uint8_t[]) { 0x91, 33,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                            0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                            0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 } },
        { 257,
          (uint8_t[]) { 0x91, 255,  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
                        0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
                        0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
                        0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
                        0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,
                        0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,
                        0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D,
                        0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D,
                        0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D,
                        0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
                        0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD,
                        0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD,
                        0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD,
                        0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD,
                        0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
                        0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD,
                        0xFE } },
        { 259, (uint8_t[]) { 0x92, HAPExpandLittleUInt16(256),
                             0x00, 0x01,
                             0x02, 0x03,
                             0x04, 0x05,
                             0x06, 0x07,
                             0x08, 0x09,
                             0x0A, 0x0B,
                             0x0C, 0x0D,
                             0x0E, 0x0F,
                             0x10, 0x11,
                             0x12, 0x13,
                             0x14, 0x15,
                             0x16, 0x17,
                             0x18, 0x19,
                             0x1A, 0x1B,
                             0x1C, 0x1D,
                             0x1E, 0x1F,
                             0x20, 0x21,
                             0x22, 0x23,
                             0x24, 0x25,
                             0x26, 0x27,
                             0x28, 0x29,
                             0x2A, 0x2B,
                             0x2C, 0x2D,
                             0x2E, 0x2F,
                             0x30, 0x31,
                             0x32, 0x33,
                             0x34, 0x35,
                             0x36, 0x37,
                             0x38, 0x39,
                             0x3A, 0x3B,
                             0x3C, 0x3D,
                             0x3E, 0x3F,
                             0x40, 0x41,
                             0x42, 0x43,
                             0x44, 0x45,
                             0x46, 0x47,
                             0x48, 0x49,
                             0x4A, 0x4B,
                             0x4C, 0x4D,
                             0x4E, 0x4F,
                             0x50, 0x51,
                             0x52, 0x53,
                             0x54, 0x55,
                             0x56, 0x57,
                             0x58, 0x59,
                             0x5A, 0x5B,
                             0x5C, 0x5D,
                             0x5E, 0x5F,
                             0x60, 0x61,
                             0x62, 0x63,
                             0x64, 0x65,
                             0x66, 0x67,
                             0x68, 0x69,
                             0x6A, 0x6B,
                             0x6C, 0x6D,
                             0x6E, 0x6F,
                             0x70, 0x71,
                             0x72, 0x73,
                             0x74, 0x75,
                             0x76, 0x77,
                             0x78, 0x79,
                             0x7A, 0x7B,
                             0x7C, 0x7D,
                             0x7E, 0x7F,
                             0x80, 0x81,
                             0x82, 0x83,
                             0x84, 0x85,
                             0x86, 0x87,
                             0x88, 0x89,
                             0x8A, 0x8B,
                             0x8C, 0x8D,
                             0x8E, 0x8F,
                             0x90, 0x91,
                             0x92, 0x93,
                             0x94, 0x95,
                             0x96, 0x97,
                             0x98, 0x99,
                             0x9A, 0x9B,
                             0x9C, 0x9D,
                             0x9E, 0x9F,
                             0xA0, 0xA1,
                             0xA2, 0xA3,
                             0xA4, 0xA5,
                             0xA6, 0xA7,
                             0xA8, 0xA9,
                             0xAA, 0xAB,
                             0xAC, 0xAD,
                             0xAE, 0xAF,
                             0xB0, 0xB1,
                             0xB2, 0xB3,
                             0xB4, 0xB5,
                             0xB6, 0xB7,
                             0xB8, 0xB9,
                             0xBA, 0xBB,
                             0xBC, 0xBD,
                             0xBE, 0xBF,
                             0xC0, 0xC1,
                             0xC2, 0xC3,
                             0xC4, 0xC5,
                             0xC6, 0xC7,
                             0xC8, 0xC9,
                             0xCA, 0xCB,
                             0xCC, 0xCD,
                             0xCE, 0xCF,
                             0xD0, 0xD1,
                             0xD2, 0xD3,
                             0xD4, 0xD5,
                             0xD6, 0xD7,
                             0xD8, 0xD9,
                             0xDA, 0xDB,
                             0xDC, 0xDD,
                             0xDE, 0xDF,
                             0xE0, 0xE1,
                             0xE2, 0xE3,
                             0xE4, 0xE5,
                             0xE6, 0xE7,
                             0xE8, 0xE9,
                             0xEA, 0xEB,
                             0xEC, 0xED,
                             0xEE, 0xEF,
                             0xF0, 0xF1,
                             0xF2, 0xF3,
                             0xF4, 0xF5,
                             0xF6, 0xF7,
                             0xF8, 0xF9,
                             0xFA, 0xFB,
                             0xFC, 0xFD,
                             0xFE, 0xFF } }
    };

    for (size_t i = 0; i < HAPArrayCount(values); i++) {
        HAPLogBufferDebug(
                &kHAPLog_Default,
                values[i].bytes,
                values[i].numBytes,
                "Data: %lu bytes.",
                (unsigned long) values[i].numBytes);

        HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
        err = HAPOPACKWriterAppendData(&writer, values[i].bytes, values[i].numBytes);
        HAPAssert(!err);

        VALIDATE_WRITER_BUFFER(&writer, expected[i].bytes, expected[i].numBytes);
    }

    uint8_t longBuffer[UINT16_MAX];
    HAPPlatformRandomNumberFill(longBuffer, sizeof longBuffer);
    HAPLogDebug(&kHAPLog_Default, "Data: %lu bytes.", (unsigned long) sizeof longBuffer);

    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendData(&writer, longBuffer, sizeof longBuffer);
    HAPAssert(!err);

    void* actualBytes_;
    size_t numActualBytes;
    HAPOPACKWriterGetBuffer(&writer, &actualBytes_, &numActualBytes);
    uint8_t* actualBytes = actualBytes_;
    HAPAssert(numActualBytes == 3 + sizeof longBuffer);
    HAPAssert(actualBytes[0] == 0x92);
    HAPAssert(HAPReadLittleUInt16(&actualBytes[1]) == sizeof longBuffer);
    HAPAssert(HAPRawBufferAreEqual(&actualBytes[3], longBuffer, sizeof longBuffer));
}

HAPLogDebug(&kHAPLog_Default, "Array: []");
{
    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendArrayBegin(&writer);
    HAPAssert(!err);
    err = HAPOPACKWriterAppendTerminator(&writer);
    HAPAssert(!err);

    uint8_t expectedBytes[] = { 0xDF, 0x03 };
    VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
}

HAPLogDebug(&kHAPLog_Default, "Array: [42]");
{
    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendArrayBegin(&writer);
    HAPAssert(!err);
    err = HAPOPACKWriterAppendInt(&writer, 42);
    HAPAssert(!err);
    err = HAPOPACKWriterAppendTerminator(&writer);
    HAPAssert(!err);

    uint8_t expectedBytes[] = { 0xDF, 0x30, 42, 0x03 };
    VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
}

HAPLogDebug(&kHAPLog_Default, "Dictionary: [:]");
{
    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendDictionaryBegin(&writer);
    HAPAssert(!err);
    err = HAPOPACKWriterAppendTerminator(&writer);
    HAPAssert(!err);

    uint8_t expectedBytes[] = { 0xEF, 0x03 };
    VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
}

HAPLogDebug(&kHAPLog_Default, "Dictionary: [\"the answer\": 42]");
{
    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendDictionaryBegin(&writer);
    HAPAssert(!err);
    {
        err = HAPOPACKWriterAppendString(&writer, "the answer");
        HAPAssert(!err);
        err = HAPOPACKWriterAppendInt(&writer, 42);
        HAPAssert(!err);
    }
    err = HAPOPACKWriterAppendTerminator(&writer);
    HAPAssert(!err);

    uint8_t expectedBytes[] = { 0xEF, 0x4A, 't', 'h', 'e', ' ', 'a', 'n', 's', 'w', 'e', 'r', 0x30, 42, 0x03 };
    VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
}

HAPLogDebug(&kHAPLog_Default, "[\"a\": \"x\", \"b\": 0, \"c\": \"y\"]");
{
    HAPOPACKWriterCreate(&writer, bytes, sizeof bytes);
    err = HAPOPACKWriterAppendDictionaryBegin(&writer);
    HAPAssert(!err);
    {
        err = HAPOPACKWriterAppendString(&writer, "a");
        HAPAssert(!err);
        err = HAPOPACKWriterAppendString(&writer, "x");
        HAPAssert(!err);
    }
    {
        err = HAPOPACKWriterAppendString(&writer, "b");
        HAPAssert(!err);
        err = HAPOPACKWriterAppendInt(&writer, 0);
        HAPAssert(!err);
    }
    {
        err = HAPOPACKWriterAppendString(&writer, "c");
        HAPAssert(!err);
        err = HAPOPACKWriterAppendString(&writer, "y");
        HAPAssert(!err);
    }
    err = HAPOPACKWriterAppendTerminator(&writer);
    HAPAssert(!err);

    uint8_t expectedBytes[] = { 0xEF, 0x41, 'a', 0x41, 'x', 0x41, 'b', 0x08, 0x41, 'c', 0x41, 'y', 0x03 };
    VALIDATE_WRITER_BUFFER(&writer, expectedBytes, sizeof expectedBytes);
}
}
