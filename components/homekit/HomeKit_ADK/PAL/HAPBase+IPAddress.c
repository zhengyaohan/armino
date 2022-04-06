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
bool HAPIPAddressVersionIsValid(HAPIPAddressVersion value) {
    switch (value) {
        case kHAPIPAddressVersion_IPv4:
        case kHAPIPAddressVersion_IPv6: {
            return true;
        }
        default:
            return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPIPv4AddressGetDescription(const HAPIPv4Address* value, char* bytes, size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);

    return HAPStringWithFormat(
            bytes, maxBytes, "%u.%u.%u.%u", value->bytes[0], value->bytes[1], value->bytes[2], value->bytes[3]);
}

HAP_RESULT_USE_CHECK
bool HAPIPv4AddressIsMulticast(const HAPIPv4Address* value) {
    HAPPrecondition(value);

    return (value->bytes[0] & 0xF0U) == 0xE0;
}

HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPv4AddressCompare(const HAPIPv4Address* value, const HAPIPv4Address* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    for (size_t i = 0; i < sizeof value->bytes; i++) {
        if (otherValue->bytes[i] > value->bytes[i]) {
            return kHAPComparisonResult_OrderedAscending;
        }
        if (otherValue->bytes[i] < value->bytes[i]) {
            return kHAPComparisonResult_OrderedDescending;
        }
    }
    return kHAPComparisonResult_OrderedSame;
}

HAP_RESULT_USE_CHECK
bool HAPIPv4AddressAreEqual(const HAPIPv4Address* value, const HAPIPv4Address* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    return HAPRawBufferAreEqual(value->bytes, otherValue->bytes, sizeof value->bytes);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HAPIPv6AddressGetDescription(const HAPIPv6Address* value, char* bytes, size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(bytes);

    return HAPStringWithFormat(
            bytes,
            maxBytes,
            "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X",
            HAPReadBigUInt16(&value->bytes[0]),
            HAPReadBigUInt16(&value->bytes[2]),
            HAPReadBigUInt16(&value->bytes[4]),
            HAPReadBigUInt16(&value->bytes[6]),
            HAPReadBigUInt16(&value->bytes[8]),
            HAPReadBigUInt16(&value->bytes[10]),
            HAPReadBigUInt16(&value->bytes[12]),
            HAPReadBigUInt16(&value->bytes[14]));
}

HAP_RESULT_USE_CHECK
bool HAPIPv6AddressIsMulticast(const HAPIPv6Address* value) {
    HAPPrecondition(value);

    return value->bytes[0] == 0xFF;
}

HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPv6AddressCompare(const HAPIPv6Address* value, const HAPIPv6Address* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    for (size_t i = 0; i < sizeof value->bytes; i++) {
        if (otherValue->bytes[i] > value->bytes[i]) {
            return kHAPComparisonResult_OrderedAscending;
        }
        if (otherValue->bytes[i] < value->bytes[i]) {
            return kHAPComparisonResult_OrderedDescending;
        }
    }
    return kHAPComparisonResult_OrderedSame;
}

HAP_RESULT_USE_CHECK
bool HAPIPv6AddressAreEqual(const HAPIPv6Address* value, const HAPIPv6Address* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    return HAPRawBufferAreEqual(value->bytes, otherValue->bytes, sizeof value->bytes);
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
bool HAPIPAddressIsValid(const HAPIPAddress* value) {
    HAPPrecondition(value);

    return HAPIPAddressVersionIsValid(value->version);
}

HAP_RESULT_USE_CHECK
HAPError HAPIPAddressGetDescription(const HAPIPAddress* value, char* bytes, size_t maxBytes) {
    HAPPrecondition(value);
    HAPPrecondition(HAPIPAddressIsValid(value));
    HAPPrecondition(bytes);

    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            return HAPIPv4AddressGetDescription(&value->_.ipv4, bytes, maxBytes);
        }
        case kHAPIPAddressVersion_IPv6: {
            return HAPIPv6AddressGetDescription(&value->_.ipv6, bytes, maxBytes);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPIPAddressIsMulticast(const HAPIPAddress* value) {
    HAPPrecondition(value);

    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            return HAPIPv4AddressIsMulticast(&value->_.ipv4);
        }
        case kHAPIPAddressVersion_IPv6: {
            return HAPIPv6AddressIsMulticast(&value->_.ipv6);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPComparisonResult HAPIPAddressCompare(const HAPIPAddress* value, const HAPIPAddress* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            switch (otherValue->version) {
                case kHAPIPAddressVersion_IPv4: {
                    return HAPIPv4AddressCompare(&value->_.ipv4, &otherValue->_.ipv4);
                }
                case kHAPIPAddressVersion_IPv6: {
                    return kHAPComparisonResult_OrderedAscending;
                }
                default:
                    HAPFatalError();
            }
        }
            HAPFatalError();
        case kHAPIPAddressVersion_IPv6: {
            switch (otherValue->version) {
                case kHAPIPAddressVersion_IPv4: {
                    return kHAPComparisonResult_OrderedDescending;
                }
                case kHAPIPAddressVersion_IPv6: {
                    return HAPIPv6AddressCompare(&value->_.ipv6, &otherValue->_.ipv6);
                }
                default:
                    HAPFatalError();
            }
        }
            HAPFatalError();
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPIPAddressAreEqual(const HAPIPAddress* value, const HAPIPAddress* otherValue) {
    HAPPrecondition(value);
    HAPPrecondition(otherValue);

    if (value->version != otherValue->version) {
        return false;
    }

    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            return HAPIPv4AddressAreEqual(&value->_.ipv4, &otherValue->_.ipv4);
        }
        case kHAPIPAddressVersion_IPv6: {
            return HAPIPv6AddressAreEqual(&value->_.ipv6, &otherValue->_.ipv6);
        }
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPIPAddressFromString(const char* description, HAPIPAddress* value) {
    HAPPrecondition(description);
    HAPPrecondition(value);

    HAPError err;

    HAPRawBufferZero(value, sizeof *value);

    size_t numDescriptionBytes = HAPStringGetNumBytes(description);

    // Determine IP address version.
    for (size_t i = 0; i < numDescriptionBytes; i++) {
        if (description[i] == '.') {
            value->version = kHAPIPAddressVersion_IPv4;
            break;
        }
        if (description[i] == ':') {
            value->version = kHAPIPAddressVersion_IPv6;
            break;
        }
    }
    if (!value->version) {
        return kHAPError_InvalidData;
    }

    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            size_t groupIndex = 0;
            bool inGroup = false;
            for (size_t i = 0; i < numDescriptionBytes; i++) {
                if (description[i] == '.') {
                    if (!inGroup) {
                        return kHAPError_InvalidData;
                    }
                    inGroup = false;
                    if (groupIndex >= 3) {
                        return kHAPError_InvalidData;
                    }
                    groupIndex++;
                } else if (HAPASCIICharacterIsNumber(description[i])) {
                    HAPAssert(groupIndex < sizeof value->_.ipv4.bytes);
                    inGroup = true;
                    uint8_t digit = (uint8_t)(description[i] - '0');
                    if (value->_.ipv4.bytes[groupIndex] > (UINT8_MAX - digit) / 10) {
                        return kHAPError_InvalidData;
                    }
                    value->_.ipv4.bytes[groupIndex] *= 10;
                    value->_.ipv4.bytes[groupIndex] += digit;
                } else {
                    return kHAPError_InvalidData;
                }
            }
            if (!inGroup) {
                return kHAPError_InvalidData;
            }
            if (groupIndex != 3) {
                return kHAPError_InvalidData;
            }
        }
            return kHAPError_None;
        case kHAPIPAddressVersion_IPv6: {
            size_t groupIndex = 0;
            bool inGroup = false;
            uint16_t group = 0;
            for (size_t i = 0; i < numDescriptionBytes; i++) {
                if (description[i] == ':') {
                    if (!inGroup) {
                        return kHAPError_InvalidData;
                    }
                    inGroup = false;
                    if (groupIndex >= 7) {
                        return kHAPError_InvalidData;
                    }
                    groupIndex++;
                } else {
                    uint8_t digit;
                    err = HAPUInt8FromHexDigit(description[i], &digit);
                    if (err) {
                        HAPAssert(err == kHAPError_InvalidData);
                        return err;
                    }
                    HAPAssert(groupIndex < sizeof value->_.ipv6.bytes / 2);
                    if (!inGroup) {
                        inGroup = true;
                        group = 0;
                    }
                    if (group > (UINT16_MAX - digit) / 16) {
                        return kHAPError_InvalidData;
                    }
                    group *= 16;
                    group += digit;
                    HAPWriteBigUInt16(&value->_.ipv6.bytes[groupIndex * 2], group);
                }
            }
            if (!inGroup) {
                return kHAPError_InvalidData;
            }
            if (groupIndex != 7) {
                return kHAPError_InvalidData;
            }
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPIPAddressRangeCreate(
        HAPIPAddressRange* ipAddressRange,
        const HAPIPAddress* startAddress,
        const HAPIPAddress* endAddress) {
    HAPPrecondition(ipAddressRange);
    HAPPrecondition(startAddress);
    HAPPrecondition(endAddress);

    if (endAddress->version != startAddress->version) {
        return kHAPError_InvalidData;
    }
    if (HAPIPAddressCompare(startAddress, endAddress) == kHAPComparisonResult_OrderedDescending) {
        return kHAPError_InvalidData;
    }

    HAPRawBufferZero(ipAddressRange, sizeof *ipAddressRange);
    switch (startAddress->version) {
        case kHAPIPAddressVersion_IPv4: {
            ipAddressRange->version = kHAPIPAddressVersion_IPv4;
            HAPRawBufferCopyBytes(
                    &ipAddressRange->_.ipv4.startAddress,
                    &startAddress->_.ipv4,
                    sizeof ipAddressRange->_.ipv4.startAddress);
            HAPRawBufferCopyBytes(
                    &ipAddressRange->_.ipv4.endAddress, &endAddress->_.ipv4, sizeof ipAddressRange->_.ipv4.endAddress);
        }
            return kHAPError_None;
        case kHAPIPAddressVersion_IPv6: {
            ipAddressRange->version = kHAPIPAddressVersion_IPv6;
            HAPRawBufferCopyBytes(
                    &ipAddressRange->_.ipv6.startAddress,
                    &startAddress->_.ipv6,
                    sizeof ipAddressRange->_.ipv6.startAddress);
            HAPRawBufferCopyBytes(
                    &ipAddressRange->_.ipv6.endAddress, &endAddress->_.ipv6, sizeof ipAddressRange->_.ipv6.endAddress);
        }
            return kHAPError_None;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPIPAddressRangeIsValid(const HAPIPAddressRange* value) {
    HAPPrecondition(value);

    if (!HAPIPAddressVersionIsValid(value->version)) {
        return false;
    }
    switch (value->version) {
        case kHAPIPAddressVersion_IPv4: {
            bool isAnyStartAddress = HAPIPv4AddressAreEqual(&value->_.ipv4.startAddress, &kHAPIPAddress_IPv4Any._.ipv4);
            bool isAnyEndAddress = HAPIPv4AddressAreEqual(&value->_.ipv4.endAddress, &kHAPIPAddress_IPv4Any._.ipv4);
            HAPComparisonResult comparisonResult =
                    HAPIPv4AddressCompare(&value->_.ipv4.startAddress, &value->_.ipv4.endAddress);

            if (isAnyStartAddress && !isAnyEndAddress) {
                return false;
            }
            if (isAnyEndAddress && !isAnyStartAddress) {
                return false;
            }
            if (comparisonResult == kHAPComparisonResult_OrderedDescending) {
                return false;
            }
        }
            return true;
        case kHAPIPAddressVersion_IPv6: {
            bool isAnyStartAddress = HAPIPv6AddressAreEqual(&value->_.ipv6.startAddress, &kHAPIPAddress_IPv4Any._.ipv6);
            bool isAnyEndAddress = HAPIPv6AddressAreEqual(&value->_.ipv6.endAddress, &kHAPIPAddress_IPv4Any._.ipv6);
            HAPComparisonResult comparisonResult =
                    HAPIPv6AddressCompare(&value->_.ipv6.startAddress, &value->_.ipv6.endAddress);

            if (isAnyStartAddress && !isAnyEndAddress) {
                return false;
            }
            if (isAnyEndAddress && !isAnyStartAddress) {
                return false;
            }
            if (comparisonResult == kHAPComparisonResult_OrderedDescending) {
                return false;
            }
        }
            return true;
        default:
            HAPFatalError();
    }
}

HAP_RESULT_USE_CHECK
bool HAPNetworkPortRangeIsValid(const HAPNetworkPortRange* value) {
    HAPPrecondition(value);

    if (value->startPort > value->endPort) {
        return false;
    }
    return true;
}
