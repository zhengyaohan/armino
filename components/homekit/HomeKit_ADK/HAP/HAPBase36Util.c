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

#include "HAPBase36Util.h"
#include "HAPBase.h"

static uint32_t divWithRemainder(uint32_t dividend, uint32_t divisor, uint32_t* remainder) {
    uint64_t mDividend = (((uint64_t) *remainder) << 32) + dividend;
    *remainder = (uint32_t)(mDividend % divisor);
    return (uint32_t)(mDividend / divisor);
}

// Divide a 128 bit value by a 32 bit value.
static uint32_t div128(uint128Struct_t* dividend, uint32_t divisor, uint128Struct_t* result) {
    uint32_t remainder = 0;
    result->m32[3] = divWithRemainder(dividend->m32[3], divisor, &remainder);
    result->m32[2] = divWithRemainder(dividend->m32[2], divisor, &remainder);
    result->m32[1] = divWithRemainder(dividend->m32[1], divisor, &remainder);
    result->m32[0] = divWithRemainder(dividend->m32[0], divisor, &remainder);
    return remainder;
}

static uint8_t b36Table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

bool HAPBase36Encode128(char* str, uint8_t numChars, uint128Struct_t* val) {
    uint32_t code = 0;
    for (int i = numChars - 2; i >= 0; i--) {
        code = div128(val, 36, val);
        HAPAssert(code < 36U);
        str[i] = b36Table[code];
    }
    str[numChars - 1] = '\0';
    return !val->m32[0] && !val->m32[1] && !val->m32[2] && !val->m32[3];
}

bool HAPBase36Encode64(char* str, uint8_t numChars, uint64_t val) {
    uint32_t code = 0;
    for (int i = numChars - 2; i >= 0; i--) {
        code = val % 36;
        val = val / 36;
        HAPAssert(code < 36U);
        str[i] = b36Table[code];
    }
    str[numChars - 1] = '\0';
    return val == 0;
}

uint32_t HAPBase36DecodeInt(char c) {
    uint32_t v = 0;
    if (HAPASCIICharacterIsNumber(c)) {
        v = c - '0';
    } else if (HAPASCIICharacterIsUppercaseLetter(c)) {
        v = c - 'A' + 10;
    }
    return v;
}
