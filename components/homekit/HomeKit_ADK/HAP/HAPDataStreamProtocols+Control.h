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

#ifndef HAP_DATA_STREAM_PROTOCOLS_CONTROL_H
#define HAP_DATA_STREAM_PROTOCOLS_CONTROL_H

#include "HAP+API.h"
#include "HAPLogSubsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)

/**
 * "control" HomeKit Data Stream protocol name.
 */
#define kHAPControlDataStreamProtocol_Name "control"

/**
 * Topic of a control.hello message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.3.6 Hello
 */
#define kHAPControlDataStreamProtocol_HelloTopic "hello"

/**
 * Topic of a control.version message.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.3.7 Protocol Versioning
 */
#define kHAPControlDataStreamProtocol_VersionTopic "version"

/**
 * HomeKit Data Stream Protocol version of the accessory.
 */
#define kHAPControlDataStreamProtocol_AccessoryHdsVersion ((double) 1.1)

/**
 * Default HomeKit Data Stream Protocol version of the connected peer.
 */
#define kHAPControlDataStreamProtocol_DefaultPeerHdsVersion ((double) 1.0)

/**
 * Minimum HomeKit Data Stream Protocol version of the connected peer needed for hello pings.
 */
#define kHAPControlDataStreamProtocol_MinPeerHdsVersionForPing ((double) 1.1)

/**
 * "control" HomeKit Data Stream protocol handler state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPControlDataStreamProtocolState) {
    /** Waiting for control.hello request. */
    kHAPControlDataStreamProtocolState_WaitingForHelloRequest = 1,

    /** Sending control.hello response. */
    kHAPControlDataStreamProtocolState_SendingHelloResponse,

    /** HomeKit Data Stream is connected. */
    kHAPControlDataStreamProtocolState_Connected,
} HAP_ENUM_END(uint8_t, HAPControlDataStreamProtocolState);

/**
 * "control" HomeKit Data Stream protocol handler base.
 */
extern const HAPDataStreamProtocolBase kHAPControlDataStreamProtocol_Base;

/**
 * "control" HomeKit Data Stream protocol handler.
 */
typedef struct {
    /**
     * HomeKit Data Stream protocol handler base. Must refer to kHAPControlDataStreamProtocol_Base.
     */
    const HAPDataStreamProtocolBase* base;
} HAPControlDataStreamProtocol;
HAP_STATIC_ASSERT(HAP_OFFSETOF(HAPControlDataStreamProtocol, base) == 0, HAPControlDataStreamProtocol_base);

/**
 * Sends a hello ping request over "control" protocol.
 *
 * @param      server               Accessory server.
 * @param      dispatcher           HomeKit Data Stream dispatcher.
 * @param      dataStream           HomeKit Data Stream.
 */
void HAPControlDataStreamProtocolSendHelloPing(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPDataStreamHandle dataStream);

#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
