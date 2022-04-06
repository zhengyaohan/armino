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

#define TEST_ASCII_IS_LETTER(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsLetter(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_UPPERCASE_LETTER(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsUppercaseLetter(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_NUMBER(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsNumber(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_ALPHANUMERIC(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsAlphanumeric(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsUppercaseAlphanumeric(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_HEX_DIGIT(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsHexDigit(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_LOWERCASE_HEX_DIGIT(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsLowercaseHexDigit(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_UPPERCASE_HEX_DIGIT(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsUppercaseHexDigit(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_LOWERCASE_HEX_LETTER(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsLowercaseHexLetter(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

#define TEST_ASCII_IS_UPPERCASE_HEX_LETTER(input, expectedValue) \
    do { \
\
        HAPLogInfo(&kHAPLog_Default, "Testing %c", input); \
        bool actualValue = HAPASCIICharacterIsUppercaseHexLetter(input); \
        HAPAssert(actualValue == expectedValue); \
    } while (0)

int main() {
    // Character must be valid for uppercase and lowercase
    // (but not ASCII character above or below it)
    TEST_ASCII_IS_LETTER('\'', false);
    TEST_ASCII_IS_LETTER('a', true);
    TEST_ASCII_IS_LETTER('z', true);
    TEST_ASCII_IS_LETTER('{', false);
    TEST_ASCII_IS_LETTER('@', false);
    TEST_ASCII_IS_LETTER('A', true);
    TEST_ASCII_IS_LETTER('Z', true);
    TEST_ASCII_IS_LETTER('[', false);
    // Character is invalid if number, other characters
    TEST_ASCII_IS_LETTER('/', false);
    TEST_ASCII_IS_LETTER('0', false);
    TEST_ASCII_IS_LETTER('9', false);
    TEST_ASCII_IS_LETTER(':', false);

    // Character must be valid for uppercase
    // (but not ASCII character above or below it)
    TEST_ASCII_IS_UPPERCASE_LETTER('@', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('A', true);
    TEST_ASCII_IS_UPPERCASE_LETTER('Z', true);
    TEST_ASCII_IS_UPPERCASE_LETTER('[', false);
    // Character is invalid if lowercase, number, other characters
    TEST_ASCII_IS_UPPERCASE_LETTER('/', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('0', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('9', false);
    TEST_ASCII_IS_UPPERCASE_LETTER(':', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('\'', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('a', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('z', false);
    TEST_ASCII_IS_UPPERCASE_LETTER('{', false);

    // Character must be valid for number
    // (but not for ASCII character above or below it)
    TEST_ASCII_IS_NUMBER('/', false);
    TEST_ASCII_IS_NUMBER('0', true);
    TEST_ASCII_IS_NUMBER('9', true);
    TEST_ASCII_IS_NUMBER(':', false);
    // Character must be invalid for letters, other characters
    TEST_ASCII_IS_LETTER('\'', false);
    TEST_ASCII_IS_NUMBER('a', false);
    TEST_ASCII_IS_NUMBER('z', false);
    TEST_ASCII_IS_LETTER('{', false);
    TEST_ASCII_IS_LETTER('@', false);
    TEST_ASCII_IS_NUMBER('A', false);
    TEST_ASCII_IS_NUMBER('Z', false);

    // Character must be valid for letters and numbers
    // (but not for ASCII character above or below it)
    TEST_ASCII_IS_ALPHANUMERIC('@', false);
    TEST_ASCII_IS_ALPHANUMERIC('A', true);
    TEST_ASCII_IS_ALPHANUMERIC('Z', true);
    TEST_ASCII_IS_ALPHANUMERIC('[', false);
    TEST_ASCII_IS_ALPHANUMERIC('\'', false);
    TEST_ASCII_IS_ALPHANUMERIC('a', true);
    TEST_ASCII_IS_ALPHANUMERIC('z', true);
    TEST_ASCII_IS_ALPHANUMERIC('{', false);
    TEST_ASCII_IS_ALPHANUMERIC('/', false);
    TEST_ASCII_IS_ALPHANUMERIC('0', true);
    TEST_ASCII_IS_ALPHANUMERIC('9', true);
    TEST_ASCII_IS_ALPHANUMERIC(':', false);

    // Character must be valid for uppercase letters, and numbers
    // (but not for ASCII character above or below it)
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('@', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('A', true);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('Z', true);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('[', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('/', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('0', true);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('9', true);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC(':', false);
    // Character is invalid for lowercase, other characters
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('\'', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('a', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('z', false);
    TEST_ASCII_IS_UPPERCASE_ALPHANUMERIC('{', false);

    // Character must be valid for a-f
    // (but not for ASCII characters above or below it)
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('\'', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('a', true);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('f', true);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('g', false);
    // Character invalid for A-F, numbers, other characters
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('@', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('A', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('F', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('/', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('0', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER('9', false);
    TEST_ASCII_IS_LOWERCASE_HEX_LETTER(':', false);

    // Character must be valid for A-F
    // (but not for ASCII characters above or below it)
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('@', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('A', true);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('F', true);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('G', false);
    // Character invalid for a-f, other characters
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('a', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('f', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('/', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('0', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER('9', false);
    TEST_ASCII_IS_UPPERCASE_HEX_LETTER(':', false);

    // Character must be valid for A-F, a-f, and numbers
    // (but not for ASCII characters above or below it)
    TEST_ASCII_IS_HEX_DIGIT('@', false);
    TEST_ASCII_IS_HEX_DIGIT('A', true);
    TEST_ASCII_IS_HEX_DIGIT('F', true);
    TEST_ASCII_IS_HEX_DIGIT('G', false);
    TEST_ASCII_IS_HEX_DIGIT('\'', false);
    TEST_ASCII_IS_HEX_DIGIT('a', true);
    TEST_ASCII_IS_HEX_DIGIT('f', true);
    TEST_ASCII_IS_HEX_DIGIT('g', false);
    TEST_ASCII_IS_HEX_DIGIT('/', false);
    TEST_ASCII_IS_HEX_DIGIT('0', true);
    TEST_ASCII_IS_HEX_DIGIT('9', true);
    TEST_ASCII_IS_HEX_DIGIT(':', false);

    // Character must be valid for a-f, and numbers
    // (but not for ASCII characters above or below it)
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('\'', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('a', true);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('f', true);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('g', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('/', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('0', true);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('9', true);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT(':', false);
    // Character invalid for A-F, other characters
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('@', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('A', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('F', false);
    TEST_ASCII_IS_LOWERCASE_HEX_DIGIT('G', false);

    // Character must be valid for A-F, and numbers
    // (but not for ASCII characters above or below it)
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('@', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('A', true);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('F', true);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('G', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('/', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('0', true);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('9', true);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT(':', false);
    // Character invalid for a-f, other characters
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('\'', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('a', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('f', false);
    TEST_ASCII_IS_UPPERCASE_HEX_DIGIT('g', false);

    return 0;
}
