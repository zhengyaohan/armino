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

#include <stdlib.h>

#include "HAPBLETransaction.h"

const char* TestArgs[][7] = {
    { NULL, "2048", "25", "0x01", "0x42", "0x0001", NULL },   { NULL, "2048", "25", "0x01", "0x42", "0x0001", "8" },
    { NULL, "2048", "25", "0x01", "0x42", "0x0001", "64" },   { NULL, "2048", "25", "0x01", "0x42", "0x0001", "1024" },
    { NULL, "2048", "25", "0x01", "0x42", "0x0001", "2048" }, { NULL, "2048", "25", "0x01", "0x42", "0x0001", "2049" },
    { NULL, "2048", "25", "0x01", "0x42", "0x0001", "4096" },
};

static unsigned long ParseInt(const char* stringValue, int base) {
    HAPPrecondition(stringValue);

    char* end;
    unsigned long intValue = strtoul(stringValue, &end, base);
    HAPAssert(end == &stringValue[HAPStringGetNumBytes(stringValue)]);
    return intValue;
}

static int Test(int argc, const char* argv[]) {
    // Input arguments:
    // argv[1] - Transaction body buffer size. 0 for a NULL buffer.
    // argv[2] - MTU. Must be >= 7.
    // argv[3] - HAP Opcode (hex).
    // argv[4] - TID (hex).
    // argv[5] - IID (hex).
    // argv[6] - Body length. If no body should be included, omit this arg.
    HAPPrecondition(argc == 6 || argc == 7);

    HAPError err;

    // Process arguments.
    uint64_t maxBodyBytes;
    err = HAPUInt64FromString(argv[1], &maxBodyBytes);
    HAPPrecondition(!err);
    uint64_t mtu;
    err = HAPUInt64FromString(argv[2], &mtu);
    HAPPrecondition(!err);
    HAPPrecondition(mtu >= 7);
    unsigned long opcode = ParseInt(argv[3], 16);
    HAPPrecondition(opcode <= UINT8_MAX);
    unsigned long tid = ParseInt(argv[4], 16);
    HAPPrecondition(tid <= UINT8_MAX);
    unsigned long iid = ParseInt(argv[5], 16);
    HAPPrecondition(iid <= UINT16_MAX);
    HAPPrecondition(iid);
    bool hasBody = argc == 7;
    uint64_t numBodyBytes = 0;
    if (hasBody) {
        err = HAPUInt64FromString(argv[6], &mtu);
        HAPPrecondition(!err);
        HAPPrecondition(numBodyBytes <= UINT16_MAX);
    }

    // Allocate body buffer
    static uint8_t bodyBytes[4096];
    HAPAssert(maxBodyBytes <= sizeof bodyBytes);

    // Initialize tx.
    HAPBLETransaction transaction;
    HAPBLETransactionCreate(&transaction, bodyBytes, maxBodyBytes);

    // Write data.
    static uint8_t fragmentBytes[4096];
    HAPAssert(mtu <= sizeof fragmentBytes);
    bool first = true;
    for (size_t remainingBodyBytes = numBodyBytes; first || remainingBodyBytes;) {
        size_t o = 0;

        // Write header.
        if (first) {
            first = false;
            fragmentBytes[o++] = 0x00; // First Fragment, Request, 1 Byte Control Field.
            fragmentBytes[o++] = (uint8_t) opcode;
            fragmentBytes[o++] = (uint8_t) tid;
            HAPWriteLittleUInt16(&fragmentBytes[o], iid);
            o += 2;
            if (hasBody) {
                HAPWriteLittleUInt16(&fragmentBytes[o], numBodyBytes);
                o += 2;
            }
            HAPAssert(o <= mtu);
        } else {
            fragmentBytes[o++] = 0x80; // Continuation, Request, 1 Byte Control Field.
            fragmentBytes[o++] = (uint8_t) tid;
            HAPAssert(o <= mtu);
        }

        // Synthesize body.
        while (o < mtu && remainingBodyBytes) {
            fragmentBytes[o++] = (uint8_t)((numBodyBytes - remainingBodyBytes) & 0xFF);
            remainingBodyBytes--;
        }
        HAPAssert(o <= mtu);

        // Process fragment.
        HAPAssert(!HAPBLETransactionIsRequestAvailable(&transaction));
        err = HAPBLETransactionHandleWrite(&transaction, fragmentBytes, o);
        HAPAssert(!err);
    }

    // Get request.
    HAPAssert(HAPBLETransactionIsRequestAvailable(&transaction));
    HAPBLETransactionRequest request;
    err = HAPBLETransactionGetRequest(&transaction, &request);
    if (numBodyBytes > maxBodyBytes) {
        HAPAssert(err == kHAPError_OutOfResources);
        return 0;
    }
    HAPAssert(!err);

    // Verify request.
    HAPAssert(request.opcode == opcode);
    HAPAssert(request.iid == iid);
    for (size_t i = 0; i < request.bodyReader.numBytes; i++) {
        uint8_t* b = request.bodyReader.bytes;
        HAPAssert(b[i] == (i & 0xFF));
    }

    return 0;
}

int main(int argc, char* argv[]) {
    for (size_t i = 0; i < HAPArrayCount(TestArgs); ++i) {
        HAPAssert(Test(TestArgs[i][6] ? 7 : 6, TestArgs[i]) == 0);
    }

    return 0;
}
