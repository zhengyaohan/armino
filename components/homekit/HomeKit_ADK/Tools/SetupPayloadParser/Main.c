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
// Copyright (C) 2020-2021 Apple Inc. All Rights Reserved.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HAP+API.h"
#include "HAPBase36Util.h"
#include "HAPLogSubsystem.h"

// See QR Code documentation or HAPAccessorySetup for Setup payload format

#define PAIRED_OFFSET   27
#define IP_OFFSET       28
#define BLE_OFFSET      29
#define WAC_OFFSET      30
#define CATEGORY_OFFSET 31
#define THREAD_OFFSET   39
#define PROD_NUM_OFFSET 40
#define VER_OFFSET      43

#define SETUP_CODE_MASK (0x7FFFFFF)
#define PAIRED_MASK     (1LLU << PAIRED_OFFSET)
#define IP_MASK         (1LLU << IP_OFFSET)
#define BLE_MASK        (1LLU << BLE_OFFSET)
#define WAC_MASK        (1LLU << WAC_OFFSET)
#define CATEGORY_MASK   (0xFFLLU << CATEGORY_OFFSET)
#define THREAD_MASK     (1LLU << THREAD_OFFSET)
#define PROD_NUM_MASK   (1LLU << PROD_NUM_OFFSET)
#define VER_MASK        (7LLU << VER_OFFSET)

#define LOW(X)        ((X) &0xFFFFFFFFL)
#define HIGH(X)       ((X) >> 32)
#define HIGHLOW(X, Y) (((X) << 32) + (Y))

static void mult64by128(uint64_t* op1hi, uint64_t* op1lo, uint64_t x) {
    uint64_t y0 = LOW(*op1lo);
    uint64_t y1 = HIGH(*op1lo);
    uint64_t x0 = LOW(x);
    uint64_t x1 = HIGH(x);
    uint64_t y12 = HIGHLOW(*op1hi, y1);

    // low 64 bits
    *op1lo = x * *op1lo;

    // The upper 64 bits:
    *op1hi = (*op1hi * x0) + (y12 * x1);
    uint64_t p01 = x0 * y1;
    uint64_t p10 = x1 * y0;
    uint64_t p00 HAP_UNUSED = x0 * y0;
    *op1hi += HIGH(p01);
    *op1hi += HIGH(p10);

    // Process Carry
    uint64_t p2 HAP_UNUSED = HIGH(p00) + LOW(p01) + LOW(p10);
    *op1hi += HIGH(p2);
}

static void add64to128(uint64_t* op1hi, uint64_t* op1lo, uint64_t add) {
    uint64_t lo = *op1lo + add;
    if (lo < *op1lo) {
        (*op1hi)++;
    }
    *op1lo = lo;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage:  SetupPayloadParser <SETUP PAYLOAD STRING>\n");
        return -1;
    }

    // Strip off the UL Prefix if any
    char* payload = strrchr(argv[1], '/') ? strrchr(argv[1], '/') + 1 : argv[1];

    printf("Parsing Setup Payload [%s]\n", payload);
    printf("---------------------------------------------------\n");

    // Every payload must include the header and setup id,
    // while also being 31 characters or less long
    if ((strlen(payload) < 13) || strlen(payload) > 31) {
        printf("Invalid payload length\n");
        return -1;
    }

    // parse the first 9 characters that make up the 'header'
    uint64_t val = 0;
    for (int i = 0; i < 9; i++) {
        val *= 36;
        val += HAPBase36DecodeInt(payload[i]);
    }

    uint32_t setupCode = (uint32_t)(val & SETUP_CODE_MASK);
    int isThread = (int) ((val & THREAD_MASK) >> THREAD_OFFSET);
    int hasProd = (int) ((val & PROD_NUM_MASK) >> PROD_NUM_OFFSET);
    printf("Setup Code \t= %u\n", setupCode);
    printf("Is Paired \t= %s\n", (val & PAIRED_MASK) >> PAIRED_OFFSET ? "TRUE" : "FALSE");
    printf("Is Thread \t= %s\n", isThread ? "TRUE" : "FALSE");
    printf("Is IP \t\t= %s\n", (val & IP_MASK) >> IP_OFFSET ? "TRUE" : "FALSE");
    printf("Is BLE \t\t= %s\n", (val & BLE_MASK) >> BLE_OFFSET ? "TRUE" : "FALSE");
    printf("Is WAC \t\t= %s\n", (val & WAC_MASK) >> WAC_OFFSET ? "TRUE" : "FALSE");
    printf("Has Product Num = %s\n", hasProd ? "TRUE" : "FALSE");
    printf("Category \t= %u\n", (uint32_t)((val & CATEGORY_MASK) >> CATEGORY_OFFSET));
    printf("Version \t= %u\n", (uint32_t)((val & VER_MASK) >> VER_OFFSET));

    // Advance past header
    payload += 9;

    if (!isThread) {
        // Setup ID is only for Extended format payloads. Thread accessories use Extended with EUI format payloads which
        // do not contain a Setup ID.
        printf("Setup ID \t= %.*s\n", 4, payload);
        payload += 4;
    }

    // Decode the rest of the payload data field
    uint64_t high = 0;
    uint64_t low = 0;
    while (*payload) {
        mult64by128(&high, &low, 36);
        add64to128(&high, &low, HAPBase36DecodeInt(*payload));
        payload++;
    }

    // If decoding for a Thread accessory, the payload data field is Extended with EUI format, decode 112 bits as EUI
    // and product number (with 16 reserved bits)
    if (isThread) {
        printf("EUI \t\t= %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
               (uint32_t)((low >> 48) & 0xFF),
               (uint32_t)((low >> 56) & 0xFF),
               (uint32_t)(high & 0xFF),
               (uint32_t)((high >> 8) & 0xFF),
               (uint32_t)((high >> 16) & 0xFF),
               (uint32_t)((high >> 24) & 0xFF),
               (uint32_t)((high >> 32) & 0xFF),
               (uint32_t)((high >> 40) & 0xFF));

        if (hasProd) {
            printf("Product Num \t= %X\n", (uint32_t)((low >> 16) & 0xFFFFFFFF));
        }
    } else if (hasProd) {
        // If decoding for a non-Thread accessory, the payload data is Extended format, decode 88 bits as product number
        // (with 56 reserved bits)
        printf("Product Num: \t= %X\n", (uint32_t)(high << 8 | low >> 56));
    }

    return 0;
}
