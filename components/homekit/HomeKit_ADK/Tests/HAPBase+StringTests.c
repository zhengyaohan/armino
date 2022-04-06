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

#include "HAPPlatform.h"

#define TEST(expectedString, format, ...) \
    do { \
        HAPError err; \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s", format); \
        char actualString[sizeof expectedString] = { 0 }; \
        err = HAPStringWithFormat(actualString, sizeof actualString, format, ##__VA_ARGS__); \
        HAPAssert(!err); \
        HAPAssert(HAPStringGetNumBytes(actualString) == sizeof expectedString - 1); \
        HAPAssert(HAPStringAreEqual(actualString, expectedString)); \
    } while (0)

#define TEST_STRING_HAS_PREFIX(string, prefix, expectedResult) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %s", prefix); \
        HAPAssert(expectedResult == HAPStringHasPrefix(string, prefix)); \
    } while (0)

static const char* null() {
    return NULL;
}

int main() {
    TEST("value: [77%] blabla", "value: [%d%%] blabla", 77);
    TEST("12:34:56:78:9A:BC", "%02X:%02X:%02X:%02X:%02X:%02X", 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);

    TEST("77", "%d", 77);
    TEST("-77", "%d", -77);
    TEST("  77", "%4d", 77);
    TEST(" -77", "%4d", -77);
    TEST("77", "%02d", 77);
    TEST("-77", "%02d", -77);
    TEST("0077", "%04d", 77);
    TEST("-077", "%04d", -77);
    TEST(" +77", "%+4d", 77);
    TEST(" -77", "%+4d", -77);
    TEST("+77", "%+02d", 77);
    TEST("-77", "%+02d", -77);
    TEST("+077", "%+04d", 77);
    TEST("-077", "%+04d", -77);
    TEST("  77", "% 4d", 77);
    TEST(" -77", "% 4d", -77);
    TEST(" 77", "% 02d", 77);
    TEST("-77", "% 02d", -77);
    TEST(" 077", "% 04d", 77);
    TEST("-077", "% 04d", -77);

    TEST("77", "%ld", 77l);
    TEST("-77", "%ld", -77l);
    TEST("  77", "%4ld", 77l);
    TEST(" -77", "%4ld", -77l);
    TEST("77", "%02ld", 77l);
    TEST("-77", "%02ld", -77l);
    TEST("0077", "%04ld", 77l);
    TEST("-077", "%04ld", -77l);
    TEST(" +77", "%+4ld", 77l);
    TEST(" -77", "%+4ld", -77l);
    TEST("+77", "%+02ld", 77l);
    TEST("-77", "%+02ld", -77l);
    TEST("+077", "%+04ld", 77l);
    TEST("-077", "%+04ld", -77l);
    TEST("  77", "% 4ld", 77l);
    TEST(" -77", "% 4ld", -77l);
    TEST(" 77", "% 02ld", 77l);
    TEST("-77", "% 02ld", -77l);
    TEST(" 077", "% 04ld", 77l);
    TEST("-077", "% 04ld", -77l);

    TEST("7777777777777777", "%lld", 7777777777777777ll);
    TEST("-7777777777777777", "%lld", -7777777777777777ll);
    TEST("    7777777777777777", "%20lld", 7777777777777777ll);
    TEST("   -7777777777777777", "%20lld", -7777777777777777ll);
    TEST("7777777777777777", "%016lld", 7777777777777777ll);
    TEST("-7777777777777777", "%016lld", -7777777777777777ll);
    TEST("00007777777777777777", "%020lld", 7777777777777777ll);
    TEST("-0007777777777777777", "%020lld", -7777777777777777ll);
    TEST("   +7777777777777777", "%+20lld", 7777777777777777ll);
    TEST("   -7777777777777777", "%+20lld", -7777777777777777ll);
    TEST("+7777777777777777", "%+016lld", 7777777777777777ll);
    TEST("-7777777777777777", "%+016lld", -7777777777777777ll);
    TEST("+0007777777777777777", "%+020lld", 7777777777777777ll);
    TEST("-0007777777777777777", "%+020lld", -7777777777777777ll);
    TEST("    7777777777777777", "% 20lld", 7777777777777777ll);
    TEST("   -7777777777777777", "% 20lld", -7777777777777777ll);
    TEST(" 7777777777777777", "% 016lld", 7777777777777777ll);
    TEST("-7777777777777777", "% 016lld", -7777777777777777ll);
    TEST(" 0007777777777777777", "% 020lld", 7777777777777777ll);
    TEST("-0007777777777777777", "% 020lld", -7777777777777777ll);

    TEST("77", "%u", 77);
    TEST("  77", "%4u", 77);
    TEST("77", "%02u", 77);
    TEST("0077", "%04u", 77);

    TEST("77", "%lu", 77l);
    TEST("  77", "%4lu", 77l);
    TEST("77", "%02lu", 77l);
    TEST("0077", "%04lu", 77l);

    TEST("17777777777777777777", "%llu", 17777777777777777777llu);
    TEST("  17777777777777777777", "%22llu", 17777777777777777777llu);
    TEST("0017777777777777777777", "%022llu", 17777777777777777777llu);

    TEST("4d", "%x", 77);
    TEST("4d", "%02x", 77);
    TEST("  4d", "%4x", 77);
    TEST("004d", "%04x", 77);
    TEST("4D", "%X", 77);
    TEST("4D", "%02X", 77);
    TEST("  4D", "%4X", 77);
    TEST("004D", "%04X", 77);

    TEST("4d", "%lx", 77l);
    TEST("4d", "%02lx", 77l);
    TEST("  4d", "%4lx", 77l);
    TEST("004d", "%04lx", 77l);
    TEST("4D", "%lX", 77l);
    TEST("4D", "%02lX", 77l);
    TEST("  4D", "%4lX", 77l);
    TEST("004D", "%04lX", 77l);

    TEST("1234567890abcdef", "%llx", 0x1234567890ABCDEFll);
    TEST("    1234567890abcdef", "%20llx", 0x1234567890ABCDEFll);
    TEST("00001234567890abcdef", "%020llx", 0x1234567890ABCDEFll);
    TEST("1234567890ABCDEF", "%llX", 0x1234567890ABCDEFll);
    TEST("    1234567890ABCDEF", "%20llX", 0x1234567890ABCDEFll);
    TEST("00001234567890ABCDEF", "%020llX", 0x1234567890ABCDEFll);

    TEST("   123456789", "%12zu", (size_t) 123456789);
    TEST("  0x12345678", "%12p", (void*) 0x12345678);
    TEST("  0xbeeffeed", "%12p", (void*) 0xBeefFeed);

    TEST("$", "%c", '$');
    TEST("  $", "%3c", '$');

    do {
        HAPError err;
        HAPLogInfo(&kHAPLog_Default, "Testing >%%c<");
        char actualString[4] = { 0 };
        err = HAPStringWithFormat(actualString, sizeof actualString, ">%c<", 0);
        HAPAssert(!err);
        HAPAssert(actualString[0] == '>');
        HAPAssert(actualString[1] == 0);
        HAPAssert(actualString[2] == '<');
        HAPAssert(actualString[3] == 0);
    } while (0);

    TEST("(null)", "%s", null());
    TEST("abcdefg", "%s", "abcdefg");
    TEST("   abcdefg", "%10s", "abcdefg");

    TEST_STRING_HAS_PREFIX("Greetings", "Greet", true);
    TEST_STRING_HAS_PREFIX("Greetings", "greet", false);
    TEST_STRING_HAS_PREFIX("A", "A", true);
    TEST_STRING_HAS_PREFIX("A", "Apple", false);
    TEST_STRING_HAS_PREFIX("Apple", "Apple", true);
    TEST_STRING_HAS_PREFIX("-NETWORK-", "-", true);
    TEST_STRING_HAS_PREFIX("-NETWORK-", "*", false);
    TEST_STRING_HAS_PREFIX("", "", true);

    return 0;
}
