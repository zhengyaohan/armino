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
float HAPFloatFromBitPattern(uint32_t bitPattern) {
    float value;
    HAPAssert(sizeof value == sizeof bitPattern);
    HAPRawBufferCopyBytes(&value, &bitPattern, sizeof value);
    return value;
}

HAP_RESULT_USE_CHECK
uint32_t HAPFloatGetBitPattern(float value) {
    uint32_t bitPattern;
    HAPAssert(sizeof bitPattern == sizeof value);
    HAPRawBufferCopyBytes(&bitPattern, &value, sizeof bitPattern);
    return bitPattern;
}

//----------------------------- Bigint Implementation ------------------------------

#define kInt_NumberOfWords (6U)  // Number of words (total 168 bits).
#define kInt_BitsPerWord   (28U) // Bits per word.
#define kInt_BitMask       (((uint32_t) 1U << kInt_BitsPerWord) - 1U)

typedef struct {
    uint32_t w[kInt_NumberOfWords];
    uint32_t len;
} Bigint;

// x = val
static void BigintInit(Bigint* x, uint64_t value) {
    uint32_t n = 0;
    while (value) {
        x->w[n] = (uint32_t) value & kInt_BitMask;
        value >>= kInt_BitsPerWord;
        n++;
    }
    x->len = n;
}

// Returns 0 if x == y, <0 if x < y, >0 if x > y
HAP_RESULT_USE_CHECK
static int32_t BigintComp(const Bigint* x, const Bigint* y) {
    uint32_t nx = x->len, ny = y->len;
    int32_t delta = (int32_t) nx - (int32_t) ny;
    if (delta) {
        return delta;
    }
    while (nx > 0) {
        nx--;
        delta = (int32_t) x->w[nx] - (int32_t) y->w[nx];
        if (delta) {
            return delta;
        }
    }
    return 0;
}

// z = x + y
static void BigintAdd(const Bigint* x, const Bigint* y, Bigint* z) {
    uint32_t c = 0, i = 0, nx = x->len, ny = y->len;
    while (i < nx || i < ny || c != 0) {
        c += (i < nx ? x->w[i] : 0) + (i < ny ? y->w[i] : 0);
        z->w[i] = c & kInt_BitMask;
        c >>= kInt_BitsPerWord;
        i++;
    }
    z->len = i;
}

// x = x * n, 2 <= n <= 10
static void BigintMul(Bigint* x, uint32_t n) {
    uint32_t c = 0, i = 0, nx = x->len;
    while (i < nx) {
        c += x->w[i] * n;
        x->w[i] = c & kInt_BitMask;
        c >>= kInt_BitsPerWord;
        i++;
    }
    if (c) {
        x->w[i] = c;
        x->len = i + 1;
    }
}

// x = x % y; returns x / y
// pre: x < 10 * y
HAP_RESULT_USE_CHECK
static uint32_t BigintDivRem(Bigint* x, const Bigint* y) {
    uint32_t q = 0, ny = y->len;
    while (BigintComp(x, y) >= 0) {
        uint32_t i = 0, nx = x->len, n = 0;
        int32_t c = 0;
        q++;
        while (i < nx) {
            // x = x - y
            c += x->w[i];
            if (i < ny) {
                c -= y->w[i];
            }
            x->w[i] = (uint32_t) c & kInt_BitMask;
            i++;
            if (c != 0) {
                n = i;
            }                       // Remember most significant word.
            c >>= kInt_BitsPerWord; // NOLINT(hicpp-signed-bitwise)
        }
        x->len = n;
    }
    return q;
}

//-----------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPFloatFromString(const char* string, float* value) {
    HAPPrecondition(string);
    HAPPrecondition(value);

    // - We don't want to accept leading or trailing whitespace.
    // - We don't want to accept hexadecimal floats for now.
    // - We don't want to accept infinity / nan for now.
    // - We only want to accept standalone values.
    *value = 0.0F;
    char c = string[0];
    int i = 1;

    // Read sign.
    uint32_t sign = 0;
    if (c == '-') {
        sign = 0x80000000;
        c = string[i++];
    } else if (c == '+') {
        c = string[i++];
    } else {
        // No sign.
    }

    // Read mantissa.
    uint64_t mant = 0;
    int dp = 0;
    int digits = 0;
    int exp10 = 0; // Base 10 exponent.
    for (;;) {
        if (c == '.' && !dp) {
            dp = 1;
        } else if (HAPASCIICharacterIsNumber(c)) {
            if (!dp) {
                exp10++;
            }
            if (mant < 100000000000000000LL) { // 10^17
                mant = mant * 10 + (uint64_t)(c - '0');
                exp10--;
            }
            digits++;
        } else {
            break;
        }
        c = string[i++];
    }
    if (digits == 0) {
        // No mantissa digits.
        return kHAPError_InvalidData;
    }
    /* mantissa == mant * 10^exp10, mant < 10^18 */

    // Read exponent.
    if (c == 'e' || c == 'E') {
        // Scan exponent.
        c = string[i++];
        int expSign = 1;
        if (c == '-') {
            expSign = -1;
            c = string[i++];
        } else if (c == '+') {
            c = string[i++];
        } else {
            // No sign.
        }
        int exp = 0;
        digits = 0;
        while (HAPASCIICharacterIsNumber(c)) {
            if (exp < 1000) {
                exp = exp * 10 + c - '0';
            }
            c = string[i++];
            digits = 1;
        }
        if (digits == 0) {
            // No exponent digits.
            return kHAPError_InvalidData;
        }
        exp10 += exp * expSign;
    }
    if (c != 0) {
        // Illegal characters in string.
        return kHAPError_InvalidData;
    }
    /* |value| == mant * 10^exp10 */

    // Check zero and large exponents to avoid Bigint overflow.
    // Values below 0.7*10-45 are rounded down to zero.
    if (mant == 0 || exp10 < -(45 + 18)) {
        *value = HAPFloatFromBitPattern(sign); // +/-0
        return kHAPError_None;
    }
    // Values above 3.4*10^38 are converted to infinity.
    if (exp10 > 38) {
        *value = HAPFloatFromBitPattern(0x7F800000 + sign); // +/-inf
        return kHAPError_None;
    }
    /* -63 <= exp10 <= 38 */

    // Base change.
    Bigint X, S;
    BigintInit(&X, mant);
    BigintInit(&S, 1);
    int exp2 = 0; // Base 2 exponent.
    /* |value| == X * 10^exp10 */
    while (exp10 > 0) {
        BigintMul(&X, 5); // * 10/2
        exp10--;
        exp2++;
    }
    while (exp10 < 0) {
        BigintMul(&S, 5); // * 10/2
        exp10++;
        exp2--;
    }
    while (BigintComp(&X, &S) >= 0) {
        BigintMul(&S, 2);
        exp2++;
    }
    while (BigintComp(&X, &S) < 0) {
        BigintMul(&X, 2);
        exp2--;
    }
    /* |value| == X/S * 2^exp2, 1 <= X/S < 2, X,S < 2^150 */

    // Assemble float bits.
    uint32_t bits = 0; // Mantissa bits (1.23).
    int numBits = 24;  // Number of mantissa bits.
    if (exp2 >= -150) {
        // No underflow.
        if (exp2 < -126) {
            // Denormalized float.
            numBits = 150 + exp2;
            exp2 = -126;
        }
        for (i = 0; i < numBits; i++) {
            bits = bits * 2 + BigintDivRem(&X, &S);
            BigintMul(&X, 2);
        }
        // Round to even.
        if (BigintComp(&X, &S) + (int32_t)(bits & 1U) > 0) {
            bits++;
        }
        if (bits >= 0x1000000) {
            // Rounding overflow.
            bits >>= 1U;
            exp2++;
        }
        if (exp2 > 127) {
            // Exponent overflow.
            bits = 0x7F800000; // inf
        } else {
            // Include exponent.
            bits += ((uint32_t)(exp2 + 126) << 23U);
        }
    }
    *value = HAPFloatFromBitPattern(bits + sign);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPFloatGetDescription(char* bytes, size_t maxBytes, float value) {
    uint32_t bits = HAPFloatGetBitPattern(value);
    uint32_t mant = bits & 0x7FFFFFU;         // Base 2 mantissa.
    int exp2 = (int) ((bits >> 23U) & 0xFFU); // Base 2 exponent.
    size_t i = 0;
    if ((int32_t) bits < 0) {
        if (i + 1 >= maxBytes) {
            return kHAPError_OutOfResources;
        }
        bytes[i++] = '-';
    }
    if (exp2 == 0xFF) { // inf/nan
        if (i + 3 >= maxBytes) {
            return kHAPError_OutOfResources;
        }
        if (mant) {
            // no sign
            bytes[0] = 'n';
            bytes[1] = 'a';
            bytes[2] = 'n';
            bytes[3] = 0;
        } else {
            bytes[i++] = 'i';
            bytes[i++] = 'n';
            bytes[i++] = 'f';
            bytes[i] = 0;
        }
        return kHAPError_None;
    }
    if (exp2) { // normalized
        mant |= 0x800000U;
    } else { // denormalized
        exp2 = 1;
    }
    if (mant == 0) {
        if (i + 1 >= maxBytes) {
            return kHAPError_OutOfResources;
        }
        bytes[i++] = '0';
        bytes[i] = 0;
        return kHAPError_None;
    }

    // Base change.
    Bigint X, D, S, T;
    BigintInit(&X, mant * 2);
    BigintInit(&D, 1);
    BigintInit(&S, 0x800000 * 2); // Position of decimal point.
    exp2 -= 127;
    /* |value| == X/S * 2^exp2, delta == D/S * 2^exp2, X/S <= 2, 0 < X < 2^25, -127 <= exp2 <= 127 */
    int exp10 = 0;
    while (exp2 < 0) {
        if (BigintComp(&X, &S) <= 0) { // X/S <= 1
            BigintMul(&X, 5);
            BigintMul(&D, 5);
            exp10--;
        } else { // X/S > 1
            BigintMul(&S, 2);
        }
        exp2++;
    }
    while (exp2 > 0) {
        if (BigintComp(&X, &S) <= 0) { // X/S <= 1
            BigintMul(&X, 2);
            BigintMul(&D, 2);
        } else { // X/S > 1
            BigintMul(&S, 5);
            exp10++;
        }
        exp2--;
    }
    /* |value| == X/S * 10^exp10, delta == D/S * 10^exp10, 1/5 < X/S <= 5, X,S < 2^114 */

    // Write digits.
    int32_t odd = bits & 1U; // Original mantissa is odd.
    uint32_t digit;          // Actual digit.
    int32_t low;             // low <= 0 => digit is in range.
    int32_t high;            // high <= 0 => (digit + 1) is in range.
    int dpPos = 0;           // Position of decimal point.
    int numDig = 0;          // Number of written digits.
    for (;;) {
        digit = BigintDivRem(&X, &S);
        /* X/S is difference between generated digits and precise value, X/S < 1 */
        if ((bits & 0x7FFFFFU) == 0) { // Special case:
            BigintAdd(&X, &X, &T);     // Lower delta is delta/2.
            low = BigintComp(&T, &D);  // X/S < D/S/2
        } else {
            low = BigintComp(&X, &D) + odd; // X/S </<= D/S
        }
        BigintAdd(&D, &X, &T);
        high = BigintComp(&S, &T) + odd; // 1 - X/S </<= D/S
        if (numDig == 0 && digit == 0 && high > 0) {
            exp10--; // Suppress leading zero.
        } else {
            if (numDig == 0 && exp10 >= -4 && exp10 <= 5) {
                // Eliminate small exponents.
                dpPos = exp10;
                exp10 = 0;
                if (dpPos < 0) {
                    // Write leading decimal point.
                    if (i + (size_t)(2 - dpPos) >= maxBytes) {
                        return kHAPError_OutOfResources;
                    }
                    bytes[i++] = '0';
                    bytes[i++] = '.';
                    while (dpPos < -1) {
                        bytes[i++] = '0';
                        dpPos++;
                    }
                }
            }
            if ((low <= 0 || high <= 0) && numDig >= dpPos) {
                // No more digits needed.
                break;
            }
            if (i + 2 >= maxBytes) {
                return kHAPError_OutOfResources;
            }
            bytes[i++] = (char) (digit + '0'); // Write digit.
            if (numDig == dpPos) {
                bytes[i++] = '.'; // Write decimal point.
            }
            numDig++;
        }
        BigintMul(&X, 10);
        BigintMul(&D, 10);
    }
    // Handle last digit.
    if (low > 0) {          // Only digit+1 in range.
        digit++;            // Use digit+1.
    } else if (high <= 0) { // digit and digit+1 in range.
        // Round to even.
        BigintAdd(&X, &X, &T);
        if (BigintComp(&T, &S) + (int32_t)(digit & 1U) > 0) { // X/S >=/> 1/2
            digit++;
        }
    } else {
        // No special treatment needed.
    }
    if (i + 1 >= maxBytes) {
        return kHAPError_OutOfResources;
    }
    // Write last digit (no decimal point).
    bytes[i++] = (char) (digit + '0');

    // Write exponent.
    if (exp10) {
        if (i + 4 >= maxBytes) {
            return kHAPError_OutOfResources;
        }
        bytes[i++] = 'e';
        if (exp10 < 0) {
            bytes[i++] = '-';
            exp10 = -exp10;
        } else {
            bytes[i++] = '+';
        }
        bytes[i++] = (char) ('0' + exp10 / 10);
        bytes[i++] = (char) ('0' + exp10 % 10);
    }
    bytes[i] = 0;
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
float HAPFloatGetFraction(float value) {
    uint32_t bits = HAPFloatGetBitPattern(value);
    int exp = (int) ((bits >> 23U) & 0xFFU) - 127;
    if (exp < 0) { // no integer part
        return value;
    }
    if (exp >= 23) { // no fractional part
        return value - value;
    }
    // Remove fractional bits.
    bits &= (0xFFFFFFFFU << (unsigned int) (23 - exp));
    // Subtract integer part.
    return value - HAPFloatFromBitPattern(bits);
}

HAP_RESULT_USE_CHECK
float HAPFloatGetAbsoluteValue(float value) {
    return HAPFloatFromBitPattern(HAPFloatGetBitPattern(value) & 0x7FFFFFFFU);
}

HAP_RESULT_USE_CHECK
bool HAPFloatIsZero(float value) {
    return (HAPFloatGetBitPattern(value) & 0x7FFFFFFFU) == 0;
}

HAP_RESULT_USE_CHECK
bool HAPFloatIsFinite(float value) {
    return (HAPFloatGetBitPattern(value) & 0x7F800000U) != 0x7F800000; // inf exponent
}

HAP_RESULT_USE_CHECK
bool HAPFloatIsInfinite(float value) {
    return (HAPFloatGetBitPattern(value) & 0x7FFFFFFFU) == 0x7F800000; // inf
}
