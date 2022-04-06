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

#include "HAPIPAccessoryProtocol.h"

int main() {
    HAPError err;

    HAPIPWriteContext writeContexts[128];

    {
        // Test vector.
        // See HomeKit Accessory Protocol Specification R17
        // Section 6.7.2.4 Timed Write Procedures
        char request[] =
                "{\n"
                "\"characteristics\": [{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 6,\n"
                "\"value\" : 1\n"
                "\n"
                "},\n"
                "{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 7,\n"
                "\"value\" : 3\n"
                "\n"
                "},\n"
                "{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 8,\n"
                "\"value\" : 4\n"
                "\n"
                "}],\n"
                "\"pid\" : 11122333\n"
                "}\n";
        uint64_t pid;
        bool pid_valid;
        size_t contexts_count;
        err = HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
                request,
                sizeof request - 1,
                writeContexts,
                HAPArrayCount(writeContexts),
                &contexts_count,
                &pid_valid,
                &pid);
        HAPAssert(!err);
        HAPAssert(contexts_count == 3);
        HAPIPWriteContext* writeContext = &writeContexts[0];
        HAPAssert(writeContext->aid == 2);
        HAPAssert(writeContext->iid == 6);
        HAPAssert(writeContext->status == 0);
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 1);
        HAPAssert(writeContext->ev == kHAPIPEventNotificationState_Undefined);
        HAPAssert(writeContext->authorizationData.bytes == NULL);
        HAPAssert(writeContext->authorizationData.numBytes == 0);
        HAPAssert(writeContext->remote == false);
        writeContext = &writeContexts[1];
        HAPAssert(writeContext->aid == 2);
        HAPAssert(writeContext->iid == 7);
        HAPAssert(writeContext->status == 0);
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 3);
        HAPAssert(writeContext->ev == kHAPIPEventNotificationState_Undefined);
        HAPAssert(writeContext->authorizationData.bytes == NULL);
        HAPAssert(writeContext->authorizationData.numBytes == 0);
        HAPAssert(writeContext->remote == false);
        writeContext = &writeContexts[2];
        HAPAssert(writeContext->aid == 2);
        HAPAssert(writeContext->iid == 8);
        HAPAssert(writeContext->status == 0);
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 4);
        HAPAssert(writeContext->ev == kHAPIPEventNotificationState_Undefined);
        HAPAssert(writeContext->authorizationData.bytes == NULL);
        HAPAssert(writeContext->authorizationData.numBytes == 0);
        HAPAssert(writeContext->remote == false);
        HAPAssert(pid_valid);
        HAPAssert(pid == 11122333);
    }
    {
        // Detect duplicate PID.
        char request[] =
                "{\n"
                "\"characteristics\": [{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 6,\n"
                "\"value\" : 1\n"
                "\n"
                "},\n"
                "{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 7,\n"
                "\"value\" : 3\n"
                "\n"
                "},\n"
                "{\n"
                "\"aid\" : 2,\n"
                "\"iid\" : 8,\n"
                "\"value\" : 4\n"
                "\n"
                "}],\n"
                "\"pid\" : 11122333,\n"
                "\"pid\" : 11122333,\n"
                "}\n";
        uint64_t pid;
        bool pid_valid;
        size_t contexts_count;
        err = HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
                request,
                sizeof request - 1,
                writeContexts,
                HAPArrayCount(writeContexts),
                &contexts_count,
                &pid_valid,
                &pid);
        HAPAssert(err == kHAPError_InvalidData);
    }
    {
        char request[] =
                "{\"characteristics\":["
                "{\"aid\":1,\"iid\":1,\"value\":-2147483648},"
                "{\"aid\":1,\"iid\":2,\"value\":-1},"
                "{\"aid\":1,\"iid\":3,\"value\":0},"
                "{\"aid\":1,\"iid\":4,\"value\":1},"
                "{\"aid\":1,\"iid\":5,\"value\":2147483648},"
                "{\"aid\":1,\"iid\":6,\"value\":4294967296},"
                "{\"aid\":1,\"iid\":7,\"value\":9223372036854775808},"
                "{\"aid\":1,\"iid\":8,\"value\":18446744073709551615}]}";
        uint64_t pid;
        bool pid_valid;
        size_t contexts_count;
        err = HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
                request,
                sizeof request - 1,
                writeContexts,
                HAPArrayCount(writeContexts),
                &contexts_count,
                &pid_valid,
                &pid);
        HAPAssert(!err);
        HAPAssert(contexts_count == 8);
        HAPIPWriteContext* writeContext = &writeContexts[0];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_Int);
        HAPAssert(writeContext->value.intValue == -2147483648);
        writeContext = &writeContexts[1];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_Int);
        HAPAssert(writeContext->value.intValue == -1);
        writeContext = &writeContexts[2];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 0);
        writeContext = &writeContexts[3];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 1);
        writeContext = &writeContexts[4];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 2147483648);
        writeContext = &writeContexts[5];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 4294967296);
        writeContext = &writeContexts[6];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 9223372036854775808ULL);
        writeContext = &writeContexts[7];
        HAPAssert(writeContext->type == kHAPIPWriteValueType_UInt);
        HAPAssert(writeContext->value.unsignedIntValue == 18446744073709551615ULL);
    }
    {
        // Test correctly-formatted but empty write request.
        char request[] = "{\"characteristics\":[]}";
        uint64_t pid;
        bool pid_valid;
        size_t contexts_count;
        err = HAPIPAccessoryProtocolGetCharacteristicWriteRequests(
                request,
                sizeof request - 1,
                writeContexts,
                HAPArrayCount(writeContexts),
                &contexts_count,
                &pid_valid,
                &pid);
        HAPAssert(!err);
        HAPAssert(contexts_count == 0);
        HAPAssert(!pid_valid);
    }

    return 0;
}
