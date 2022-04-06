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

#ifndef HAP_CHARACTERISTIC_TYPES_TLV_H
#define HAP_CHARACTERISTIC_TYPES_TLV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPCharacteristicTypes.h"
#include "HAPTLV+Internal.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType) {
    kHAPCharacteristicTLVType_Separator = 0
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType);

#define kHAPCharacteristicTLVDescription_Separator "TLV Separator"

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_OperationStatus,
        HAPCharacteristicTLVFormat_WiFiRouter_OperationStatus)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_IPAddress) {
    kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv4Address = 1,
    kHAPCharacteristicTLVType_WiFiRouter_IPAddress_IPv6Address = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_IPAddress);

#define kHAPCharacteristicTLVDescription_WiFiRouter_IPAddress_IPv4Address "IPv4 Address"

#define kHAPCharacteristicTLVDescription_WiFiRouter_IPAddress_IPv6Address "IPv6 Address"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_IPAddress, HAPCharacteristicTLVFormat_WiFiRouter_IPAddress)

#define kHAPCharacteristicValueDescription_WiFiRouter_Credential_MACAddress "MAC Address Credential"

#define kHAPCharacteristicValueDescription_WiFiRouter_Credential_PSK "PSK Credential"

HAP_UNION_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_Credential, HAPCharacteristicTLVFormat_WiFiRouter_Credential)

HAP_ENUM_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_ICMPProtocol, HAPCharacteristicTLVFormat_WiFiRouter_ICMPProtocol)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ICMPType) {
    kHAPCharacteristicTLVType_WiFiRouter_ICMPType_Protocol = 1,
    kHAPCharacteristicTLVType_WiFiRouter_ICMPType_TypeValue = 2,
    kHAPCharacteristicTLVType_WiFiRouter_ICMPType_CodeValue = 3
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ICMPType);

#define kHAPCharacteristicTLVDescription_WiFiRouter_ICMPType_Protocol "ICMP Protocol"

#define kHAPCharacteristicTLVDescription_WiFiRouter_ICMPType_TypeValue "ICMP Type Value"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_ICMPType, HAPCharacteristicTLVFormat_WiFiRouter_ICMPType)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicRawTLVType_WiFiRouter_ICMPList) {
    kHAPCharacteristicRawTLVType_WiFiRouter_ICMPList_ICMPType = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicRawTLVType_WiFiRouter_ICMPList);

#define kHAPCharacteristicRawTLVDescription_WiFiRouter_ICMPList_ICMPType "ICMP Type"

/**
 * Callback that should be invoked for each ICMP Type.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WiFiRouter_ICMPList_ICMPType)(
        void* _Nullable context,
        HAPCharacteristicValue_WiFiRouter_ICMPType* value,
        bool* shouldContinue);

/**
 * ICMP Type List.
 */
typedef struct {
    /**
     * Enumerates all ICMP Types.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WiFiRouter_ICMPList_ICMPType callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    HAPCharacteristicValue_WiFiRouter_ICMPType _;
} HAPCharacteristicRawValue_WiFiRouter_ICMPList;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicRawValue_WiFiRouter_ICMPList,
        HAPCharacteristicTLVFormat_WiFiRouter_ICMPType,
        HAPCharacteristicRawTLVFormat_WiFiRouter_ICMPList)

HAP_VALUE_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_ICMPList, HAPCharacteristicTLVFormat_WiFiRouter_ICMPList)

HAP_ENUM_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_FirewallType, HAPCharacteristicTLVFormat_WiFiRouter_FirewallType)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule_Protocol,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule_Protocol)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule) {
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_Protocol = 1,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostDNSName = 2,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPStart = 3,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostIPEnd = 4,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortStart = 5,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule_HostPortEnd = 6
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall_PortRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_Protocol "Protocol"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostDNSName "Host DNS Name"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostIPStart "Host IP Address Start"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostIPEnd "Host IP Address End"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostPortStart "Host Port Start"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_PortRule_HostPortEnd "Host Port End"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_PortRule,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_PortRule)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule) {
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostDNSName = 1,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostIPStart = 2,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_HostIPEnd = 3,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule_ICMPList = 4
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall_ICMPRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostDNSName "Host DNS Name"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostIPStart "Host IP Address Start"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_HostIPEnd "Host IP Address End"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_ICMPRule_ICMPList "ICMP Type List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_ICMPRule,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_ICMPRule)

#define kHAPCharacteristicValueDescription_WiFiRouter_WANFirewall_Rule_Port "WAN Port Rule"

#define kHAPCharacteristicValueDescription_WiFiRouter_WANFirewall_Rule_ICMP "WAN ICMP Rule"

HAP_UNION_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_Rule,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_Rule)

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_WANFirewall_RuleList,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_Rule,
        HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall_RuleList)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall) {
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_Type = 1,
    kHAPCharacteristicTLVType_WiFiRouter_WANFirewall_RuleList = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_WANFirewall);

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_Type "WAN Firewall Type"

#define kHAPCharacteristicTLVDescription_WiFiRouter_WANFirewall_RuleList "WAN Firewall Rule List"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_WANFirewall, HAPCharacteristicTLVFormat_WiFiRouter_WANFirewall)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_Direction,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_Direction)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicRawTLVType_WiFiRouter_LANFirewall_Rule_EndpointList) {
    kHAPCharacteristicRawTLVType_WiFiRouter_LANFirewall_Rule_EndpointList_Group = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicRawTLVType_WiFiRouter_LANFirewall_Rule_EndpointList);

#define kHAPCharacteristicRawTLVDescription_WiFiRouter_LANFirewall_Rule_EndpointList_Group "Client Group Identifier"

/**
 * Callback that should be invoked for each Client Group Identifier.
 *
 * @param      context              Context.
 * @param      value                Value.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_Rule_EndpointList_Group)(
        void* _Nullable context,
        uint32_t* value,
        bool* shouldContinue);

/**
 * Endpoint List.
 */
typedef struct {
    /**
     * Enumerates all Client Group Identifiers.
     *
     * - Enumeration can only be requested once.
     *
     * - When this callback is produced by a HAPTLVReader the only valid error codes are:
     *   kHAPError_None and kHAPError_InvalidData.
     *
     * - When this callback is passed to a HAPTLVWriter the only valid error codes are:
     *   kHAPError_None, kHAPError_Unknown, kHAPError_InvalidState, kHAPError_OutOfResources, and kHAPError_Busy.
     *   Notably, kHAPError_InvalidData is not allowed.
     *
     * @param      dataSource           Data source.
     * @param      callback             Function to call on each value.
     * @param      context              Context that is passed to the callback.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_Unknown        If an unknown error occurred while serializing.
     * @return kHAPError_InvalidState   If serialization is not possible in the current state.
     * @return kHAPError_InvalidData    If invalid data was encountered while parsing.
     * @return kHAPError_OutOfResources If out of resources while serializing.
     * @return kHAPError_Busy           If serialization is temporarily not possible.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*enumerate)(
            HAPSequenceTLVDataSource* dataSource,
            HAPCharacteristicValueCallback_WiFiRouter_LANFirewall_Rule_EndpointList_Group callback,
            void* _Nullable context);

    /** Data source to pass to the enumerate function of decoded sequence TLV. */
    HAPSequenceTLVDataSource dataSource;

    /** Buffer for internal use by the enumerate function. */
    uint32_t _;
} HAPCharacteristicRawValue_WiFiRouter_LANFirewall_Rule_EndpointList;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicRawValue_WiFiRouter_LANFirewall_Rule_EndpointList,
        HAPUInt32TLVFormat,
        HAPCharacteristicRawTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList)

HAP_VALUE_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule_EndpointList,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule_EndpointList)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule) {
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_Direction = 1,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_EndpointList = 2,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_IPAddress = 3,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule_Port = 4
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_MulticastBridgingRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_Direction "Direction"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_EndpointList "Endpoint List"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_IPAddress "Destination IP Address"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_MulticastBridgingRule_Port "Destination Port"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_MulticastBridgingRule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_MulticastBridgingRule)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule_Protocol,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule_Protocol)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule) {
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_Direction = 1,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_EndpointList = 2,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_Protocol = 3,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_PortStart = 5,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule_PortEnd = 6
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticPortRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_Direction "Direction"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_EndpointList "Endpoint List"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_Protocol "Protocol"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_PortStart "Destination Port Start"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticPortRule_PortEnd "Destination Port End"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticPortRule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticPortRule)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule_Protocol,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule_Protocol)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_AdvertisementProtocol,
        HAPCharacteristicTLVFormat_WiFiRouter_AdvertisementProtocol)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ServiceType) {
    kHAPCharacteristicTLVType_WiFiRouter_ServiceType_Name = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ServiceType);

#define kHAPCharacteristicTLVDescription_WiFiRouter_ServiceType_Name "Name"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_ServiceType, HAPCharacteristicTLVFormat_WiFiRouter_ServiceType)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule) {
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Direction = 1,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_EndpointList = 2,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Protocol = 3,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_AdvertProtocol = 4,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Flags = 5,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule_Service = 6
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_DynamicPortRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Direction "Direction"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_EndpointList "Endpoint List"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Protocol "Protocol"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_AdvertProtocol "Advertisement Protocol"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Flags "Flags"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_DynamicPortRule_Service "Service Type"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_DynamicPortRule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_DynamicPortRule)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule) {
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_Direction = 1,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_EndpointList = 2,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule_ICMPList = 4
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall_StaticICMPRule);

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_Direction "Direction"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_EndpointList "Endpoint List"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_StaticICMPRule_ICMPList "ICMP Type List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_StaticICMPRule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_StaticICMPRule)

#define kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_MulticastBridging "Multicast Bridging Rule"

#define kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_StaticPort "Static Port Rule"

#define kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_DynamicPort "Dynamic Port Rule"

#define kHAPCharacteristicValueDescription_WiFiRouter_LANFirewall_Rule_StaticICMP "Static ICMP Rule"

HAP_UNION_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_Rule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule)

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_LANFirewall_RuleList,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_Rule,
        HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall_RuleList)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall) {
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_Type = 1,
    kHAPCharacteristicTLVType_WiFiRouter_LANFirewall_RuleList = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_LANFirewall);

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_Type "LAN Firewall Type"

#define kHAPCharacteristicTLVDescription_WiFiRouter_LANFirewall_RuleList "LAN Firewall Rule List"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WiFiRouter_LANFirewall, HAPCharacteristicTLVFormat_WiFiRouter_LANFirewall)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Config) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Identifier = 1,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Group = 3,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_Credential = 4,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_WANFirewall = 5,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Config_LANFirewall = 6
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Config);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Identifier \
    "Network Client Profile Identifier"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Group "Client Group Identifier"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_Credential "Credential Data"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_WANFirewall "WAN Firewall Configuration"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Config_LANFirewall "LAN Firewall Configuration"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_Config,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Config)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_Operation_Type,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation_Type)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Operation) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Type = 1,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation_Config = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Operation);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation_Type "Operation Type"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation_Config \
    "Network Client Profile Configuration"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_Operation,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Operation = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Operation \
    "Network Client Profile Control Operation"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Operation,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl)

extern const HAPCharacteristicTLVFormat_NetworkClientProfileControl
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp_Status = 1,
    kHAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp_Config = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_OperationRsp);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_OperationRsp_Status "Status"

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_OperationRsp_Config \
    "Network Client Profile Configuration"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_OperationRsp,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_OperationRsp)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Response) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Response_OperationRsp = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Response);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Response_OperationRsp \
    "Network Client Profile Control Operation Response"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_Response,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_OperationRsp,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Response)

extern const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Response
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Response;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ClientList) {
    kHAPCharacteristicTLVType_WiFiRouter_ClientList_Identifier = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiRouter_ClientList);

#define kHAPCharacteristicTLVDescription_WiFiRouter_ClientList_Identifier "Network Client Profile Identifier"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiRouter_ClientList,
        HAPUInt32TLVFormat,
        HAPCharacteristicTLVFormat_WiFiRouter_ClientList)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Event) {
    kHAPCharacteristicTLVType_NetworkClientProfileControl_Event_ClientList = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientProfileControl_Event);

#define kHAPCharacteristicTLVDescription_NetworkClientProfileControl_Event_ClientList \
    "Network Client Profile Identifier List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientProfileControl_Event,
        HAPCharacteristicTLVFormat_NetworkClientProfileControl_Event)

extern const HAPCharacteristicTLVFormat_NetworkClientProfileControl_Event
        kHAPCharacteristicTLVFormat_NetworkClientProfileControl_Event;

//----------------------------------------------------------------------------------------------------------------------

#define kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_Client \
    "Network Client Profile Identifier"

#define kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_MACAddress "MAC Address"

#define kHAPCharacteristicValueDescription_NetworkClientStatusControl_Identifier_IPAddress "IP Address"

HAP_UNION_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_Identifier,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_Identifier)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList) {
    kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList_Identifier = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList);

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IdentifierList_Identifier \
    "Network Client Status Identifier"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_IdentifierList,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_Identifier,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_IdentifierList)

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_OperationType,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_OperationType)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl) {
    kHAPCharacteristicTLVType_NetworkClientStatusControl_OperationType = 1,
    kHAPCharacteristicTLVType_NetworkClientStatusControl_IdentifierList = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl);

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_OperationType "Operation Type"

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IdentifierList \
    "Network Client Status Identifier List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl)

extern const HAPCharacteristicTLVFormat_NetworkClientStatusControl
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_IPAddressList) {
    kHAPCharacteristicTLVType_NetworkClientStatusControl_IPAddressList_IPAddress = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_IPAddressList);

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IPAddressList_IPAddress "IP Address"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_IPAddressList,
        HAPCharacteristicTLVFormat_WiFiRouter_IPAddress,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_IPAddressList)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_Status) {
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_Client = 1,
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_MACAddress = 2,
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_IPAddressList = 3,
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_Name = 6,
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Status_RSSI = 7
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_Status);

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_Client "Network Client Profile Identifier"

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_MACAddress "MAC Address"

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_IPAddressList "IP Address List"

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_Name "Name"

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Status_RSSI "RSSI"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_Status,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_Status)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_Response) {
    kHAPCharacteristicTLVType_NetworkClientStatusControl_Response_Status = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkClientStatusControl_Response);

#define kHAPCharacteristicTLVDescription_NetworkClientStatusControl_Response_Status "Network Client Status"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkClientStatusControl_Response,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_Status,
        HAPCharacteristicTLVFormat_NetworkClientStatusControl_Response)

extern const HAPCharacteristicTLVFormat_NetworkClientStatusControl_Response
        kHAPCharacteristicTLVFormat_NetworkClientStatusControl_Response;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_SupportedRouterConfiguration) {
    kHAPCharacteristicTLVType_SupportedRouterConfiguration_Flags = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_SupportedRouterConfiguration);

#define kHAPCharacteristicTLVDescription_SupportedRouterConfiguration_Flags "Router Flags"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_SupportedRouterConfiguration,
        HAPCharacteristicTLVFormat_SupportedRouterConfiguration)

extern const HAPCharacteristicTLVFormat_SupportedRouterConfiguration
        kHAPCharacteristicTLVFormat_SupportedRouterConfiguration;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WANConfigurationList_Config_WANType,
        HAPCharacteristicTLVFormat_WANConfigurationList_Config_WANType)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WANConfigurationList_Config) {
    kHAPCharacteristicTLVType_WANConfigurationList_Config_Identifier = 1,
    kHAPCharacteristicTLVType_WANConfigurationList_Config_WANType = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WANConfigurationList_Config);

#define kHAPCharacteristicTLVDescription_WANConfigurationList_Config_Identifier "WAN Identifier"

#define kHAPCharacteristicTLVDescription_WANConfigurationList_Config_WANType "WAN Type"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WANConfigurationList_Config,
        HAPCharacteristicTLVFormat_WANConfigurationList_Config)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WANConfigurationList) {
    kHAPCharacteristicTLVType_WANConfigurationList_Config = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WANConfigurationList);

#define kHAPCharacteristicTLVDescription_WANConfigurationList_Config "WAN Configuration"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_WANConfigurationList,
        HAPCharacteristicTLVFormat_WANConfigurationList_Config,
        HAPCharacteristicTLVFormat_WANConfigurationList)

extern const HAPCharacteristicTLVFormat_WANConfigurationList kHAPCharacteristicTLVFormat_WANConfigurationList;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WANStatusList_Status) {
    kHAPCharacteristicTLVType_WANStatusList_Status_WANIdentifier = 1,
    kHAPCharacteristicTLVType_WANStatusList_Status_LinkStatus = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WANStatusList_Status);

#define kHAPCharacteristicTLVDescription_WANStatusList_Status_WANIdentifier "WAN Identifier"

#define kHAPCharacteristicTLVDescription_WANStatusList_Status_LinkStatus "WAN Link Status"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_WANStatusList_Status, HAPCharacteristicTLVFormat_WANStatusList_Status)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WANStatusList) {
    kHAPCharacteristicTLVType_WANStatusList_Status = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WANStatusList);

#define kHAPCharacteristicTLVDescription_WANStatusList_Status "WAN Status"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_WANStatusList,
        HAPCharacteristicTLVFormat_WANStatusList_Status,
        HAPCharacteristicTLVFormat_WANStatusList)

extern const HAPCharacteristicTLVFormat_WANStatusList kHAPCharacteristicTLVFormat_WANStatusList;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiConfigurationControl) {
    /** Operation Type - Read config, Simple Update or Fail-safe Update */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_OperationType = 1,

    /** Opaque, persistent value used by the controller to identify versions of the Wi-Fi
     * configuration of an accessory */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_Cookie = 2,

    /**
     * Status of the update - success, failed, auth failed, network configured
     */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_UpdateStatus = 3,

    /**
     * Timeout in seconds for the fail safe update operation, between 1 and 60 seconds
     */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_Operation_Timeout = 4,

    /**
     * ISO 3166-1 country code used by the accessory to configure the correct set of radio channels.
     * that are available for Wi-Fi (Optional)
     */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_CountryCode = 10,

    /**
     * The Wi-Fi configuration for this accessory when acting in Station Mode (Optional)
     */
    kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig = 11

} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiConfigurationControl);

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_OperationType "Operation Type"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_Cookie "Cookie"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_UpdateStatus "Update Status"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_Operation_Timeout "Operation Timeout"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_CountryCode "Country Code"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig "Station Configuration"

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiConfigurationControl_OperationType,
        HAPCharacteristicTLVFormat_WiFiConfigurationControl_OperationType)

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiConfigurationControl_StationConfig,
        HAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig) {
    kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SSID = 1,
    kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_SecurityMode = 2,
    kHAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig_PSK = 3
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_WiFiConfigurationControl_StationConfig);

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_SSID "SSID"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_SecurityMode "Security Mode"

#define kHAPCharacteristicTLVDescription_WiFiConfigurationControl_StationConfig_PSK "PSK"

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiConfigurationControl_StationConfig_SecurityMode,
        HAPCharacteristicTLVFormat_WiFiConfigurationControl_StationConfig_SecurityMode)

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_WiFiConfigurationControl,
        HAPCharacteristicTLVFormat_WiFiConfigurationControl)

extern const HAPCharacteristicTLVFormat_WiFiConfigurationControl kHAPCharacteristicTLVFormat_WiFiConfigurationControl;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkAccessViolationControl_OperationType,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl_OperationType)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl) {
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_OperationType = 1,
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_ClientList = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl);

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_OperationType "Operation Type"

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_ClientList \
    "Network Client Profile Identifier List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkAccessViolationControl,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl)

extern const HAPCharacteristicTLVFormat_NetworkAccessViolationControl
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Violation) {
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_Client = 1,
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_LastTimestamp = 2,
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_Violation_ResetTimestamp = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Violation);

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_Client \
    "Network Client Profile Identifier"

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_LastTimestamp \
    "Last Violation Timestamp"

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Violation_ResetTimestamp "Last Reset Timestamp"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkAccessViolationControl_Violation,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Violation)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Response) {
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_Response_Violation = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Response);

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Response_Violation "Network Access Violation"

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkAccessViolationControl_Response,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Violation,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response)

extern const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Response;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Event) {
    kHAPCharacteristicTLVType_NetworkAccessViolationControl_Event_ClientList = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NetworkAccessViolationControl_Event);

#define kHAPCharacteristicTLVDescription_NetworkAccessViolationControl_Event_ClientList \
    "Network Client Profile Identifier List"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NetworkAccessViolationControl_Event,
        HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event)

extern const HAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event
        kHAPCharacteristicTLVFormat_NetworkAccessViolationControl_Event;
/* Lock Event Notification Context TLV */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_LockContextData) {
    kHAPCharacteristicTLVType_LockContextIdentifier = 1,
    kHAPCharacteristicTLVType_LockContextSource = 2,
    kHAPCharacteristicTLVType_LockContextTimestamp = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_LockContextData);
//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPoint) {
    kHAPCharacteristicTLVType_AccessCodeControlPoint_OperationType = 1,
    kHAPCharacteristicTLVType_AccessCodeControlPoint_Request = 2,
    kHAPCharacteristicTLVType_AccessCodeControlPoint_Response = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPoint);

#define kHAPCharacteristicTLVDescription_AccessCodeControlPoint_OperationType "Operation Type"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPoint_Request "Access Code Control Point Request"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPoint_Response "Access Code Control Point Response"

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPointRequest) {
    kHAPCharacteristicTLVType_AccessCodeControlPointRequest_Identifier = 1,
    kHAPCharacteristicTLVType_AccessCodeControlPointRequest_AccessCode = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPointRequest);

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointRequest_Identifier "Identifier"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointRequest_AccessCode "Access Code"

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPointResponse) {
    kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Identifier = 1,
    kHAPCharacteristicTLVType_AccessCodeControlPointResponse_AccessCode = 2,
    kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Flags = 3,
    kHAPCharacteristicTLVType_AccessCodeControlPointResponse_Status = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_AccessCodeControlPointResponse);

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Identifier "Identifier"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_AccessCode "Access Code"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Flags "Flags"

#define kHAPCharacteristicTLVDescription_AccessCodeControlPointResponse_Status "Status"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_AccessCodeControlPointResponse,
        HAPCharacteristicTLVFormat_AccessCodeControlPointResponse)

extern const HAPCharacteristicTLVFormat_AccessCodeControlPointResponse
        kHAPCharacteristicTLVFormat_AccessCodeControlPointResponse;

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_AccessCodeControlPoint, HAPCharacteristicTLVFormat_AccessCodeControlPoint)

extern const HAPCharacteristicTLVFormat_AccessCodeControlPoint kHAPCharacteristicTLVFormat_AccessCodeControlPoint;

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_AccessCodeControlPointRequest,
        HAPCharacteristicTLVFormat_AccessCodeControlPointRequest)

extern const HAPCharacteristicTLVFormat_AccessCodeControlPointRequest
        kHAPCharacteristicTLVFormat_AccessCodeControlPointRequest;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_AccessCodeSupportedConfiguration) {
    kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_CharacterSet = 1,
    kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MinimumLength = 2,
    kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumLength = 3,
    kHAPCharacteristicTLVType_AccessCodeSupportedConfiguration_MaximumAccessCodes = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_AccessCodeSupportedConfiguration);

#define kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_CharacterSet "Character Set"

#define kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MinimumLength "Minimum length"

#define kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MaximumLength "Maximum length"

#define kHAPCharacteristicTLVDescription_AccessCodeSupportedConfiguration_MaximumAccessCodes "Maximum Access Codes"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_AccessCodeSupportedConfiguration,
        HAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration)

extern const HAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration
        kHAPCharacteristicTLVFormat_AccessCodeSupportedConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_OperatingStateResponse) {
    /** State. */
    kHAPCharacteristicTLVType_OperatingStateResponseType_State = 1,

    /** Abnormal Reason. */
    kHAPCharacteristicTLVType_OperatingStateResponseType_AbnormalReasons = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_OperatingStateResponse);

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessSupportedConfiguration) {
    kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumIssuerKeys = 1,
    kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumSuspendedDeviceCredentialKeys = 2,
    kHAPCharacteristicTLVType_NfcAccessSupportedConfiguration_MaximumActiveDeviceCredentialKeys = 3
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessSupportedConfiguration);

#define kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumIssuerKeys "Maximum Issuer Keys"

#define kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumSuspendedDeviceCredentialKeys \
    "Maximum Suspended Device Credential Keys"

#define kHAPCharacteristicTLVDescription_NfcAccessSupportedConfiguration_MaximumActiveDeviceCredentialKeys \
    "Maximum Active Device Credential Keys"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessSupportedConfiguration,
        HAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration)

extern const HAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration
        kHAPCharacteristicTLVFormat_NfcAccessSupportedConfiguration;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessControlPoint) {
    kHAPCharacteristicTLVType_NfcAccessControlPoint_OperationType = 1,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyRequest = 2,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_IssuerKeyResponse = 3,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyRequest = 4,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_DeviceCredentialKeyResponse = 5,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyRequest = 6,
    kHAPCharacteristicTLVType_NfcAccessControlPoint_ReaderKeyResponse = 7,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessControlPoint);

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_OperationType "Operation Type"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_IssuerKeyRequest "NFC Access Issuer Key Request"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_IssuerKeyResponse "NFC Access Issuer Key Response"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_DeviceCredentialKeyRequest \
    "NFC Access Device Credential Key Request"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_DeviceCredentialKeyResponse \
    "NFC Access Device Credential Key Response"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_ReaderKeyRequest "NFC Access Reader Key Request"

#define kHAPCharacteristicTLVDescription_NfcAccessControlPoint_ReaderKeyResponse "NFC Access Reader Key Response"

/* NFC Access Issuer Key */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessIssuerKeyRequest) {
    kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Type = 1,
    kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Key = 2,
    kHAPCharacteristicTLVType_NfcAccessIssuerKeyRequest_Identifier = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessIssuerKeyRequest);

#define kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Type "Type"

#define kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Key "Key"

#define kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyRequest_Identifier "Identifier"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessIssuerKeyRequest,
        HAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest)

extern const HAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyRequest;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessIssuerKeyResponse) {
    kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_Identifier = 1,
    kHAPCharacteristicTLVType_NfcAccessIssuerKeyResponse_StatusCode = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessIssuerKeyResponse);

#define kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyResponse_Identifier "Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessIssuerKeyResponse_StatusCode "Status Code"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessIssuerKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessIssuerKeyResponse;

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessControlPoint_IssuerKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_IssuerKeyResponse;

/* NFC Access Device Credential Key */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest) {
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Type = 1,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Key = 2,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_IssuerKeyIdentifier = 3,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_State = 4,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest_Identifier = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyRequest);

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Type "Type"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Key "Key"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_IssuerKeyIdentifier "Issuer Key Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_State "State"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyRequest_Identifier "Identifier"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessDeviceCredentialKeyRequest,
        HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest)

extern const HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest
        kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyRequest;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse) {
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_Identifier = 1,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_IssuerKeyIdentifier = 2,
    kHAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse_StatusCode = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessDeviceCredentialKeyResponse);

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_Identifier "Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_IssuerKeyIdentifier \
    "Issuer Key Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessDeviceCredentialKeyResponse_StatusCode "Status Code"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessDeviceCredentialKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessDeviceCredentialKeyResponse;

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessControlPoint_DeviceCredentialKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_DeviceCredentialKeyResponse;

/* NFC Access Reader Key */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessReaderKeyRequest) {
    kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Type = 1,
    kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Key = 2,
    kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_ReaderIdentifier = 3,
    kHAPCharacteristicTLVType_NfcAccessReaderKeyRequest_Identifier = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessReaderKeyRequest);

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Type "Type"

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Key "Key"

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_ReaderIdentifier "Reader Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyRequest_Identifier "Identifier"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessReaderKeyRequest,
        HAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest)

extern const HAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest kHAPCharacteristicTLVFormat_NfcAccessReaderKeyRequest;

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_NfcAccessReaderKeyResponse) {
    kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_Identifier = 1,
    kHAPCharacteristicTLVType_NfcAccessReaderKeyResponse_StatusCode = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_NfcAccessReaderKeyResponse);

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyResponse_Identifier "Identifier"

#define kHAPCharacteristicTLVDescription_NfcAccessReaderKeyResponse_StatusCode "Status Code"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessReaderKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessReaderKeyResponse;

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_NfcAccessControlPoint_ReaderKeyResponse,
        HAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse)

extern const HAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse
        kHAPCharacteristicTLVFormat_NfcAccessControlPoint_ReaderKeyResponse;

//----------------------------------------------------------------------------------------------------------------------

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_FirmwareUpdateReadiness) {
    kHAPCharacteristicTLVType_FirmwareUpdateReadiness_StagingNotReadyReason = 1,
    kHAPCharacteristicTLVType_FirmwareUpdateReadiness_UpdateNotReadyReason = 2
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_FirmwareUpdateReadiness);

#define kHAPCharacteristicTLVDescription_FirmwareUpdateReadiness_StagingNotReadyReason "Staging Not Ready Reason"
#define kHAPCharacteristicTLVDescription_FirmwareUpdateReadiness_UpdateNotReadyReason  "Update Not Ready Reason"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_FirmwareUpdateReadiness,
        HAPCharacteristicTLVFormat_FirmwareUpdateReadiness)

extern const HAPCharacteristicTLVFormat_FirmwareUpdateReadiness kHAPCharacteristicTLVFormat_FirmwareUpdateReadiness;

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState,
        HAPCharacteristicTLVFormat_FirmwareUpdateStatus_FirmwareUpdateState)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_FirmwareUpdateStatus) {
    kHAPCharacteristicTLVType_FirmwareUpdateStatus_FirmwareUpdateState = 1,
    kHAPCharacteristicTLVType_FirmwareUpdateStatus_UpdateDuration = 2,
    kHAPCharacteristicTLVType_FirmwareUpdateStatus_StagedFirmwareVersion = 3
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_FirmwareUpdateStatus);

#define kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_FirmwareUpdateState   "Firmware Update State"
#define kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_UpdateDuration        "Update Duration"
#define kHAPCharacteristicTLVDescription_FirmwareUpdateStatus_StagedFirmwareVersion "Staged Firmware Version"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_FirmwareUpdateStatus, HAPCharacteristicTLVFormat_FirmwareUpdateStatus)

extern const HAPCharacteristicTLVFormat_FirmwareUpdateStatus kHAPCharacteristicTLVFormat_FirmwareUpdateStatus;

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_ThreadManagementControl_OperationType,
        HAPCharacteristicTLVFormat_ThreadManagementControl_OperationType)

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_ThreadManagementControl_NetworkCredentials,
        HAPCharacteristicTLVFormat_ThreadManagementControl_NetworkCredentials)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials) {
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_NetworkName = 1,
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_Channel = 2,
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_PanId = 3,
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_ExtPanId = 4,
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials_MasterKey = 5,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials);

#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_NetworkName "Network Name"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_Channel     "Channel"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_PanId       "PAN ID"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_ExtPanId    "Ext PAN ID"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials_MasterKey   "Master Key"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_ThreadManagementControl,
        HAPCharacteristicTLVFormat_ThreadManagementControl)

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_ThreadManagementControl) {
    kHAPCharacteristicTLVType_ThreadManagementControl_OperationType = 1,
    kHAPCharacteristicTLVType_ThreadManagementControl_NetworkCredentials = 2,
    kHAPCharacteristicTLVType_ThreadManagementControl_FormingAllowed = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_ThreadManagementControl);

#define kHAPCharacteristicTLVDescription_ThreadManagementControl_OperationType      "Operation Type"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_NetworkCredentials "Network Credentials"
#define kHAPCharacteristicTLVDescription_ThreadManagementControl_FormingAllowed     "Forming Allowed"

extern const HAPCharacteristicTLVFormat_ThreadManagementControl kHAPCharacteristicTLVFormat_ThreadManagementControl;

//----------------------------------------------------------------------------------------------------------------------

extern const HAPDataTLVFormat kHAPCharacteristicTLVFormat_VariableLengthInteger4;

extern const HAPDataTLVFormat kHAPCharacteristicTLVFormat_VariableLengthInteger8;

/**
 * Supported Transition Configurations
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_SupportedTransitionConfiguration) {
    /** Supported Transitions. */
    kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransitions = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_SupportedTransitionConfiguration);

#define kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransitions \
    "Supported Characteristic Value Transition Configuration"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_SupportedTransition, HAPCharacteristicTLVFormat_SupportedTransition)

extern const HAPCharacteristicTLVFormat_SupportedTransition kHAPCharacteristicTLVFormat_SupportedTransition;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_SupportedTransitionConfigurations,
        HAPCharacteristicTLVFormat_SupportedTransition,
        HAPCharacteristicTLVFormat_SupportedTransitionConfigurations)

extern const HAPCharacteristicTLVFormat_SupportedTransitionConfigurations
        kHAPCharacteristicTLVFormat_SupportedTransitionConfigurations;

/**
 * Supported Transition Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition) {
    /** HAP Instance ID. */
    kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition_HAPID = 1,
    /** Transition Types. */
    kHAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition_Types = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_SupportedTransitionConfiguration_SupportedTransition);

#define kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransition_HAPID "HAP instance ID"
#define kHAPCharacteristicTLVDescription_SupportedTransitionConfiguration_SupportedTransition_Types \
    "Characteristic Value Transition Types"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_TransitionControl, HAPCharacteristicTLVFormat_TransitionControl)

extern const HAPCharacteristicTLVFormat_TransitionControl kHAPCharacteristicTLVFormat_TransitionControl;

/**
 * Transition Control
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl) {
    /** Fetch transitions. */
    kHAPCharacteristicTLVType_TransitionControl_Fetch = 1,
    /** Configure transitions. */
    kHAPCharacteristicTLVType_TransitionControl_Start = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl);

#define kHAPCharacteristicTLVDescription_TransitionControl_Fetch "Transition Fetch"
#define kHAPCharacteristicTLVDescription_TransitionControl_Start "Transition Start"

/**
 * Transition Control- fetch
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_Fetch) {
    /** HAP Instance ID to fetch transitions for */
    kHAPCharacteristicTLVType_TransitionControl_Fetch_HAPID = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_Fetch);

#define kHAPCharacteristicTLVDescription_TransitionControl_Fetch_HAPID "HAP Instance ID"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_Fetch,
        HAPCharacteristicTLVFormat_TransitionControl_Fetch)

extern const HAPCharacteristicTLVFormat_TransitionControl_Fetch kHAPCharacteristicTLVFormat_TransitionControl_Fetch;

/**
 * Transition Control- start
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_Start) {
    /** Transitions */
    kHAPCharacteristicTLVType_TransitionControl_Start_Transitions = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_Start);

#define kHAPCharacteristicTLVDescription_TransitionControl_Start_Transitions "Transitions"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_Transition,
        HAPCharacteristicTLVFormat_TransitionControl_Transition)

extern const HAPCharacteristicTLVFormat_TransitionControl_Transition
        kHAPCharacteristicTLVFormat_TransitionControl_Transition;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_Start,
        HAPCharacteristicTLVFormat_TransitionControl_Transition,
        HAPCharacteristicTLVFormat_TransitionControl_Start)

extern const HAPCharacteristicTLVFormat_TransitionControl_Start kHAPCharacteristicTLVFormat_TransitionControl_Start;

/**
 * Transition Control Response
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControlResponse) {
    /** Response type: Transition. */
    kHAPCharacteristicTLVType_TransitionControlResponse_Transition = 1,
    /** Response type: Transition State. */
    kHAPCharacteristicTLVType_TransitionControlResponse_TransitionState = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControlResponse);

#define kHAPCharacteristicTLVDescription_TransitionControlResponse_Transition "Transition Control Response: Transition"
#define kHAPCharacteristicTLVDescription_TransitionControlResponse_TransitionState \
    "Transition Control Response: Transition State"

/**
 * Transition control- transition
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_Transition) {
    /** HAP Instance ID of the transition */
    kHAPCharacteristicTLVType_TransitionControl_Transition_HAPID = 1,
    /** Controller Context */
    kHAPCharacteristicTLVType_TransitionControl_Transition_ControllerContext = 2,
    /** Transition repetition */
    kHAPCharacteristicTLVType_TransitionControl_Transition_EndBehavior = 3,
    /** Linear Transitions */
    kHAPCharacteristicTLVType_TransitionControl_Transition_Linear = 4,
    /** Linear Derived Transitions */
    kHAPCharacteristicTLVType_TransitionControl_Transition_LinearDerived = 5,
    /** Value Update Time Interval */
    kHAPCharacteristicTLVType_TransitionControl_Transition_ValueUpdateTimeInterval = 6,
    /** Notify Value Change Threshold */
    kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyValueChangeThreshold = 7,
    /** Notify Time Interval Threshold */
    kHAPCharacteristicTLVType_TransitionControl_Transition_NotifyTimeIntervalThreshold = 8,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_Transition);

#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_HAPID             "HAP Instance ID"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_ControllerContext "Controller Context"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_EndBehavior       "End Behavior"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_Linear            "Linear Transition"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_LinearDerived     "Linear Derived Transition"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_ValueUpdateTimeInterval \
    "Value Update Time Interval"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_NotifyValueChangeThreshold \
    "Notify Value Change Threshold"
#define kHAPCharacteristicTLVDescription_TransitionControl_Transition_NotifyTimeIntervalThreshold \
    "Notify Time Interval Threshold"

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_EndBehavior,
        HAPCharacteristicTLVFormat_TransitionControl_Transition_EndBehavior)

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearTransition,
        HAPCharacteristicTLVFormat_TransitionControl_Transition_Linear)

extern const HAPCharacteristicTLVFormat_TransitionControl_Transition_Linear
        kHAPCharacteristicTLVFormat_TransitionControl_Transition_Linear;

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearDerivedTransition,
        HAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived)

extern const HAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived
        kHAPCharacteristicTLVFormat_TransitionControl_Transition_LinearDerived;

/**
 * Transition control- Linear Transitions
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearTransition) {
    /** Linear Transition Points */
    kHAPCharacteristicTLVType_TransitionControl_LinearTransition_Points = 1,
    /** Start Condition type */
    kHAPCharacteristicTLVType_TransitionControl_LinearTransition_StartCondition = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearTransition);

#define kHAPCharacteristicTLVDescription_TransitionControl_LinearTransition_Points         "Linear Transition Points"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearTransition_StartCondition "Start Condition"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearTransition_Point,
        HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points)

extern const HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points
        kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points;

HAP_ENUM_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_StartCondition,
        HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_StartCondition)
/**
 * Linear Transition Point Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint) {
    /** Target value */
    kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_TargetValue = 1,
    /** Time by which target value must be reached */
    kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_TargetCompletionDuration = 2,
    /** Delay to start transition */
    kHAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint_StartDelayDuration = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearTransitionPoint);

#define kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_TargetValue "Target Value"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_TargetCompletionDuration \
    "Target Completion Duration"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearTransitionPoint_StartDelayDuration \
    "Start Delay Duration"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_Point,
        HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points)

extern const HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points
        kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points;

/**
 * Transition control- Derived Linear Transitions
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_SourceValueRange) {
    /** Lower bound */
    kHAPCharacteristicTLVType_TransitionControl_SourceValueRange_Lower = 1,
    /** Upper bound */
    kHAPCharacteristicTLVType_TransitionControl_SourceValueRange_Upper = 2,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_SourceValueRange);

#define kHAPCharacteristicTLVDescription_TransitionControl_SourceRange_Lower "Source Value Lower Bound"
#define kHAPCharacteristicTLVDescription_TransitionControl_SourceRange_Upper "Source Value Upper Bound"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_SourceValueRange,
        HAPCharacteristicTLVFormat_TransitionControl_SourceValueRange)

extern const HAPCharacteristicTLVFormat_TransitionControl_SourceValueRange
        kHAPCharacteristicTLVFormat_TransitionControl_SourceValueRange;

/**
 * Transition control- Derived Linear Transitions
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition) {
    /** Derived Linear Transition Points */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_Points = 1,
    /** HAP Instance ID of the source characteristic */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceID = 2,
    /** Source Value Range */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition_SourceRange = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearDerivedTransition);

#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_Points \
    "Linear Derived Transition Points"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_SourceID    "Source HAP Instance ID"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransition_SourceRange "Source Value Range"

/**
 * Derived Linear Transition Point Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint) {
    /** Curve scale */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_Scale = 1,
    /** Curve offset */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_Offset = 2,
    /** Time by which target value must be reached */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_TargetCompletionDuration = 3,
    /** Delay to start transition */
    kHAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint_StartDelayDuration = 4,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionControl_LinearDerivedTransitionPoint);

#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_Scale  "Scale"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_Offset "Offset"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_TargetCompletionDuration \
    "Target Completion Duration"
#define kHAPCharacteristicTLVDescription_TransitionControl_LinearDerivedTransitionPoint_StartDelayDuration \
    "Start Delay Duration"

HAP_STRUCT_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_TransitionState_ActiveContext,
        HAPCharacteristicTLVFormat_TransitionState_ActiveContext)

extern const HAPCharacteristicTLVFormat_TransitionState_ActiveContext
        kHAPCharacteristicTLVFormat_TransitionState_ActiveContext;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionState,
        HAPCharacteristicTLVFormat_TransitionState_ActiveContext,
        HAPCharacteristicTLVFormat_TransitionState)

extern const HAPCharacteristicTLVFormat_TransitionState kHAPCharacteristicTLVFormat_TransitionState;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearDerivedTransition_PointList,
        HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_Points,
        HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList)

extern const HAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList
        kHAPCharacteristicTLVFormat_TransitionControl_LinearDerivedTransition_PointList;

HAP_SEQUENCE_TLV_SUPPORT(
        HAPCharacteristicValue_TransitionControl_LinearTransition_PointList,
        HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_Points,
        HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList)

extern const HAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList
        kHAPCharacteristicTLVFormat_TransitionControl_LinearTransition_PointList;

/**
 * Transition State
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionState) {
    /** Active Context */
    kHAPCharacteristicTLVType_TransitionState_ActiveContexts = 1,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionState);

#define kHAPCharacteristicTLVDescription_TransitionState_ActiveContexts "Active Contexts"

/**
 * Active Context Description
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_TransitionState_ActiveContext) {
    /** HAP Instance ID of the context */
    kHAPCharacteristicTLVType_TransitionState_ActiveContext_HAPID = 1,
    /** Controller Context */
    kHAPCharacteristicTLVType_TransitionState_ActiveContext_ControllerContext = 2,
    /** TIme elapsed since given transition was set */
    kHAPCharacteristicTLVType_TransitionState_ActiveContext_TimeElapsedSinceStart = 3,
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_TransitionState_ActiveContext);

#define kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_HAPID                 "HAP Instance ID"
#define kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_ControllerContext     "Controller Context"
#define kHAPCharacteristicTLVDescription_TransitionState_ActiveContext_TimeElapsedSinceStart "Time Elapsed Since Start"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicTLVType_HardwareFinish) {
    kHAPCharacteristicTLVType_HardwareFinish_RgbColorValue = 1
} HAP_ENUM_END(uint8_t, HAPCharacteristicTLVType_HardwareFinish);

#define kHAPCharacteristicTLVDescription_HardwareFinish_RgbColorValue "RGB Color Value"

HAP_STRUCT_TLV_SUPPORT(HAPCharacteristicValue_HardwareFinish, HAPCharacteristicTLVFormat_HardwareFinish)

extern const HAPCharacteristicTLVFormat_HardwareFinish kHAPCharacteristicTLVFormat_HardwareFinish;

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
