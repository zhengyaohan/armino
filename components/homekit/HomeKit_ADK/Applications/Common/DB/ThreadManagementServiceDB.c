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

#include "ThreadManagementServiceDB.h"

#if (HAVE_THREAD == 1)

#include "HAPPlatformSetup+Init.h"
#include "HAPServiceTypes.h"

/**
 * The 'Service Signature' characteristic of the Thread Management service.
 */
static const HAPDataCharacteristic threadManagementServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_ThreadManagementServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'OpenThreadVersion" characteristic of the Thread Management service.
 */
static const HAPStringCharacteristic threadManagementOpenThreadVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_ThreadManagementOpenThreadVersion,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementOpenThreadVersion,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementOpenThreadVersion,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .readRequiresAdminPermissions = true,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false, }, },
    .constraints = { .maxLength = 128 },
    .callbacks = { .handleRead = HAPPlatformSetupHandleOpenThreadVersionRead, .handleWrite = NULL, },
};

/**
 * The 'Control' characteristic of the Thread Management service.
 */
static const HAPTLV8Characteristic threadManagementControlCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_ThreadManagementControl,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementControl,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementControl,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = false,
        .hidden = false,
        .requiresTimedWrite = false,
        .readRequiresAdminPermissions = true,
        .writeRequiresAdminPermissions = true,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = true,
            .supportsWriteResponse = true,
        },
        .ble = {
            .supportsBroadcastNotification = false,
            .supportsDisconnectedNotification = false,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false,
        }
    },
    .callbacks = {
        .handleRead = HAPHandleThreadManagementControlRead,
        .handleWrite = HAPHandleThreadManagementControlWrite,
        .handleSubscribe = NULL,
        .handleUnsubscribe = NULL,
    },
};

/**
 * The 'Node Capabilities' characteristic of the Thread Management service.
 */
static const HAPUInt16Characteristic threadManagementNodeCapabilitiesCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt16,
    .iid = kIID_ThreadManagementNodeCapabilities,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementNodeCapabilities,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementNodeCapabilities,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0, .maximumValue = 0x1F, .stepValue = 1, },
    .callbacks = { .handleRead = HAPHandleThreadManagementNodeCapabilitiesRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL, },
};

/**
 * The 'Thread Role' characteristic of the Thread Management service.
 */
const HAPUInt16Characteristic threadManagementStatusCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt16,
    .iid = kIID_ThreadManagementStatus,
    .characteristicType = &kHAPCharacteristicType_ThreadManagementStatus,
    .debugDescription = kHAPCharacteristicDebugDescription_ThreadManagementStatus,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .readRequiresAdminPermissions = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .minimumValue = 0, .maximumValue = 0x7F, .stepValue = 1, },
    .callbacks = { .handleRead = HAPHandleThreadManagementStatusRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL, },
};

/**
 * The 'Thread Current Transport' characteristic of the Thread Management service.
 */
static const HAPBoolCharacteristic threadManagementCurrentTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_ThreadManagementCurrentTransport,
    .characteristicType = &kHAPCharacteristicType_CurrentTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_CurrentTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false, },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleThreadManagementCurrentTransportRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL, },
};

/**
 * Thread Management service that contains the above Thread Management characteristics.
 */
const HAPService threadManagementService = {
    .iid = kIID_ThreadManagement,
    .serviceType = &kHAPServiceType_ThreadManagement,
    .debugDescription = kHAPServiceDebugDescription_ThreadManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &threadManagementServiceSignatureCharacteristic,
                    &threadManagementOpenThreadVersionCharacteristic,
                    &threadManagementNodeCapabilitiesCharacteristic,
                    &threadManagementStatusCharacteristic,
                    &threadManagementCurrentTransportCharacteristic,
                    &threadManagementControlCharacteristic,
                    NULL,
            },
};

#endif // (HAVE_THREAD == 1)
