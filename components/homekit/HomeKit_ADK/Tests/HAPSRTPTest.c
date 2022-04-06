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

#include "HAP+API.h"

#include "HAPPlatform+Init.h"
#include "Harness/HAPTestController.c"

/**
 * Test HAPSRTPSetupContext() function potential leaks
 */
static void TestHAPSRTPSetupContext(void) {
    HAPSRTPContext srtpContext;
    HAPSRTPContext srtcpContext;
    const uint8_t key[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    uint8_t salt[kHAPSRTP_SaltSize];
    uint32_t ssrc = 0xfedcba98; // synchronization source
    uint32_t tagSize = 8;

    for (size_t i = 0; i < kHAPSRTP_SaltSize; i++) {
        salt[i] = 'a' + 'b' * i;
    }

    HAPSRTPSetupContext(&srtpContext, &srtcpContext, key, sizeof key, salt, tagSize, ssrc);

    // Note that there shouldn't be any heap memory leaks at this point.

    // The purpose of this test is to call the above function, which isn't supposed to leak memory.
    // If there were any leaks, the address sanitizer would detect it when the test app exits.
}

/**
 * Test HAPSRTPEncrypt() function potential leaks
 */
static void TestHAPSRTPEncrypt(void) {
    HAPSRTPContext srtpContext;
    HAPSRTPContext srtcpContext;
    const uint8_t key[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    uint8_t salt[kHAPSRTP_SaltSize];
    uint32_t ssrc = 0xfedcba98; // synchronization source
    uint32_t tagSize = 8;

    for (size_t i = 0; i < kHAPSRTP_SaltSize; i++) {
        salt[i] = 'a' + 'b' * i;
    }

    HAPSRTPSetupContext(&srtpContext, &srtcpContext, key, sizeof key, salt, tagSize, ssrc);

    // Try encrypt
    uint8_t buffer[256];
    uint8_t srcData[64];
    for (size_t i = 0; i < sizeof srcData; i++) {
        srcData[i] = 'z' * i;
    }

    HAPSRTPEncrypt(&srtpContext, buffer, srcData, 8, sizeof srcData - 8, 0);

    // Note that there shouldn't be any heap memory leaks at this point.

    // The purpose of this test is to call the above function, which isn't supposed to leak memory.
    // If there were any leaks, the address sanitizer would detect it when the test app exits.
}

/**
 * Test HAPSRTPDecrypt() function potential leaks
 */
static void TestHAPSRTPDecrypt(void) {
    HAPSRTPContext srtpContext;
    HAPSRTPContext srtcpContext;
    const uint8_t key[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
    uint8_t salt[kHAPSRTP_SaltSize];
    uint32_t ssrc = 0xfedcba98; // synchronization source
    uint32_t tagSize = 8;

    for (size_t i = 0; i < kHAPSRTP_SaltSize; i++) {
        salt[i] = 'a' + 'b' * i;
    }

    HAPSRTPSetupContext(&srtpContext, &srtcpContext, key, sizeof key, salt, tagSize, ssrc);

    // Try encrypt
    uint8_t buffer[256];
    uint8_t srcData[64];
    for (size_t i = 0; i < sizeof srcData; i++) {
        srcData[i] = 'z' * i;
    }

    HAPSRTPDecrypt(&srtpContext, buffer, srcData, sizeof srcData, 0);

    // Note that there shouldn't be any heap memory leaks at this point.

    // The purpose of this test is to call the above function, which isn't supposed to leak memory.
    // If there were any leaks, the address sanitizer would detect it when the test app exits.
}

int main() {
    HAPPlatformCreate();

    TestHAPSRTPSetupContext();
    TestHAPSRTPEncrypt();
    TestHAPSRTPDecrypt();

    return 0;
}
