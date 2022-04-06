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

#include "DataStreamTransportManagementServiceDB.h"
#include "ApplicationFeatures.h"

#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
#if (HAVE_BLE == 1) || (HAVE_THREAD == 1)
#include "HAPRequestHandlers+DataStreamHAPTransport.h"
#endif // (HAVE_BLE == 1) || (HAVE_THREAD == 1)
#endif // (HAVE_HDS_TRANSPORT_OVER_HAP == 1)

/**
 * The 'Service Signature' characteristic of the Data Stream Transport Management service.
 */
static const HAPDataCharacteristic dataStreamTransportManagementServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_DataStreamTransportManagementServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Supported Data Stream Transport Configuration' characteristic of the Data Stream Transport Management service.
 */
static const HAPTLV8Characteristic
        dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic = {
            .format = kHAPCharacteristicFormat_TLV8,
            .iid = kIID_DataStreamTransportManagementSupportedDataStreamTransportConfiguration,
            .characteristicType = &kHAPCharacteristicType_SupportedDataStreamTransportConfiguration,
            .debugDescription = kHAPCharacteristicDebugDescription_SupportedDataStreamTransportConfiguration,
            .manufacturerDescription = NULL,
            .properties = { .readable = true,
                            .writable = false,
                            .supportsEventNotification = false,
                            .hidden = false,
                            .readRequiresAdminPermissions = false,
                            .writeRequiresAdminPermissions = false,
                            .requiresTimedWrite = false,
                            .supportsAuthorizationData = false,
                            .ip = { .controlPoint = false, .supportsWriteResponse = false },
                            .ble = { .supportsBroadcastNotification = false,
                                     .supportsDisconnectedNotification = false,
                                     .readableWithoutSecurity = false,
                                     .writableWithoutSecurity = false } },
            .callbacks = { .handleRead =
                                   HAPHandleDataStreamTransportManagementSupportedDataStreamTransportConfigurationRead,
                           .handleWrite = NULL,
                           .handleSubscribe = NULL,
                           .handleUnsubscribe = NULL }
        };

/**
 * The 'Setup Data Stream Transport' characteristic of the Data Stream Transport Management service.
 */
static const HAPTLV8Characteristic dataStreamTransportManagementSetupDataStreamTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementSetupDataStreamTransport,
    .characteristicType = &kHAPCharacteristicType_SetupDataStreamTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_SetupDataStreamTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementSetupDataStreamTransportRead,
                   .handleWrite = HAPHandleDataStreamTransportManagementSetupDataStreamTransportWrite,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

/**
 * The 'Version' characteristic of the Data Stream Transport Management service.
 */
static const HAPStringCharacteristic dataStreamTransportManagementVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_DataStreamTransportManagementVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .readRequiresAdminPermissions = false,
                    .writeRequiresAdminPermissions = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementVersionRead,
                   .handleWrite = NULL,
                   .handleSubscribe = NULL,
                   .handleUnsubscribe = NULL }
};

#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
#if (HAVE_BLE == 1) || (HAVE_THREAD == 1)
static const HAPTLV8Characteristic dataStreamHAPTransportCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementDataStreamHAPTransport,
    .characteristicType = &kHAPCharacteristicType_DataStreamHAPTransport,
    .debugDescription = kHAPCharacteristicDebugDescription_DataStreamHAPTransport,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true, .supportsWriteResponse = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementDataStreamHAPTransportRead,
                   .handleWrite = HAPHandleDataStreamTransportManagementDataStreamHAPTransportWrite }
};

static const HAPTLV8Characteristic dataStreamHAPTransportInterruptCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_DataStreamTransportManagementDataStreamHAPTransportInterrupt,
    .characteristicType = &kHAPCharacteristicType_DataStreamHAPTransportInterrupt,
    .debugDescription = kHAPCharacteristicDebugDescription_DataStreamHAPTransportInterrupt,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandleDataStreamTransportManagementDataStreamHAPTransportInterruptRead }
};
#endif // (HAVE_BLE == 1) || (HAVE_THREAD == 1)
#endif // (HAVE_HDS_TRANSPORT_OVER_HAP == 1)

const HAPService dataStreamTransportManagementService = {
    .iid = kIID_DataStreamTransportManagement,
    .serviceType = &kHAPServiceType_DataStreamTransportManagement,
    .debugDescription = kHAPServiceDebugDescription_DataStreamTransportManagement,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics =
            (const HAPCharacteristic* const[]) {
                    &dataStreamTransportManagementServiceSignatureCharacteristic,
                    &dataStreamTransportManagementSupportedDataStreamTransportConfigurationCharacteristic,
                    &dataStreamTransportManagementSetupDataStreamTransportCharacteristic,
                    &dataStreamTransportManagementVersionCharacteristic,
#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
#if (HAVE_BLE == 1) || (HAVE_THREAD == 1)
                    &dataStreamHAPTransportCharacteristic,
                    &dataStreamHAPTransportInterruptCharacteristic,
#endif // (HAVE_BLE == 1) || (HAVE_THREAD == 1)
#endif // (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
                    NULL }
};