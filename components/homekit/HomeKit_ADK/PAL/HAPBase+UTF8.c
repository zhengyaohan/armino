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

HAP_RESULT_USE_CHECK
bool HAPUTF8IsValidData(const void* bytes, size_t numBytes) {
    HAPPrecondition(bytes);

    // See http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - Table 3-7, page 94.

    int error = 0;  // Error state in bit 0.
    int state = 0;  // Number of leading 1 bits == number of outstanding continuation bytes.
    int prefix = 0; // Prefix byte if value is second byte.

    // state     value    -> more  first  second -> error  state'    prefix
    // 0xxxxxxx  0xxxxxxx     0      0      x        0    00000000  00000000
    // 0xxxxxxx  10xxxxxx     0      1      0        1
    // 0xxxxxxx  1110xxxx     0      1      1        0    110xxxxx  1110xxxx
    // 110xxxx0  0xxxxxxx     1      0      x        1
    // 110xxxx0  10xxxxxx     1      1      0        0    10xxxxxx  00000000
    // 110xxxx0  110xxxxx     1      1      1        1

    for (size_t i = 0; i < numBytes; i++) {
        int value = ((const uint8_t*) bytes)[i];
        int more = state >> 7;         // More continuation bytes expected.             NOLINT(hicpp-signed-bitwise)
        int first = value >> 7;        // First bit.                                    NOLINT(hicpp-signed-bitwise)
        int second = (value >> 6) & 1; // Second bit.                                   NOLINT(hicpp-signed-bitwise)

        // Illegal value.
        error |= ((uint8_t)(value - 0xC0) - 2) >> 8U; // value == 1100000x             NOLINT(hicpp-signed-bitwise)
        error |= (0xF4 - value) >> 8U;                // value >= 11110101             NOLINT(hicpp-signed-bitwise)

        // Illegal second byte.
        int bits = value >> 5;                                  // NOLINT(hicpp-signed-bitwise)
        error |= (((uint8_t)(prefix - 0xE0) - 1) >> 8) & ~bits; // 11100000  xx0xxxxx  NOLINT(hicpp-signed-bitwise)
        error |= (((uint8_t)(prefix - 0xED) - 1) >> 8) & bits;  // 11101101  xx1xxxxx  NOLINT(hicpp-signed-bitwise)
        bits |= value >> 4;                                     // NOLINT(hicpp-signed-bitwise)
        error |= (((uint8_t)(prefix - 0xF0) - 1) >> 8) & ~bits; // 11110000  xx00xxxx  NOLINT(hicpp-signed-bitwise)
        error |= (((uint8_t)(prefix - 0xF4) - 1) >> 8) & bits;  // 11110100  xx11xxxx  NOLINT(hicpp-signed-bitwise)

        // Illegal continuation.
        error |= (first & ~second) ^ more; // NOLINT(hicpp-signed-bitwise)

        // New state.
        prefix = -(first & second) & value;                 // NOLINT(hicpp-signed-bitwise)
        state = (uint8_t)((prefix | (-more & state)) << 1); // NOLINT(hicpp-signed-bitwise)
    }

    // Missing continuations.
    error |= state >> 7; // NOLINT(hicpp-signed-bitwise)

    return (bool) (1 & ~error); // NOLINT(hicpp-signed-bitwise)
}
