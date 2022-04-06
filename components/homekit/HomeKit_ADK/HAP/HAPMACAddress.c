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

#include "HAPMACAddress.h"

#include "HAPAccessoryServer+Internal.h"
#include "HAPDeviceID.h"

/**
 * Checks whether a MAC address is valid.
 *
 * - The function may modify the given MAC address candidate.
 *
 * @param      macAddress           MAC address candidate. May be modified.
 *
 * @return true                     If the returned MAC address is valid.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
typedef bool (*HAPMACAddressValidatorCallback)(HAPMACAddress* macAddress);

/**
 * Deterministically derives the MAC address for a given accessory server.
 *
 * @param      server              Accessory server.
 * @param      networkInterface     Interface.
 * @param      macAddress           MAC address.
 * @param      validatorCallback    Validator callback to check whether a MAC address is valid.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
static HAPError HAPMACAddressGet(
        HAPAccessoryServer* server,
        const char* _Nullable const networkInterface,
        HAPMACAddress* macAddress,
        HAPMACAddressValidatorCallback validatorCallback) {
    HAPPrecondition(server);
    HAPPrecondition(macAddress);
    HAPPrecondition(validatorCallback);

    HAPError err;

    // Load Device ID.
    HAPDeviceID deviceID;
    err = HAPDeviceIDGet(server, &deviceID);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    // Load firmware revision.
    // BLE: Accessories supporting random static Bluetooth LE device addresses must use a new
    //      Bluetooth LE device address after a firmware update.
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.8 Firmware Update Requirements
    char salt[64 + 4]; // 64 byte FW rev + 4 bytes counter.
    HAPRawBufferZero(salt, sizeof salt);
    size_t numFirmwareVersionBytes = HAPStringGetNumBytes(server->primaryAccessory->firmwareVersion);
    HAPAssert(numFirmwareVersionBytes >= 1);
    HAPAssert(numFirmwareVersionBytes <= 64);
    HAPRawBufferCopyBytes(salt, server->primaryAccessory->firmwareVersion, numFirmwareVersionBytes);

    // Derive MAC addresses until a valid one is found.
    for (uint32_t i = 0;; i++) {
        HAPRawBufferCopyBytes(&salt[64], &i, sizeof i);

        HAP_hkdf_sha512(
                macAddress->bytes,
                sizeof macAddress->bytes,
                deviceID.bytes,
                sizeof deviceID.bytes,
                (const uint8_t*) salt,
                sizeof salt,
                (const uint8_t*) networkInterface,
                networkInterface ? HAPStringGetNumBytes(networkInterface) : 0);

        if (validatorCallback(macAddress)) {
            return kHAPError_None;
        }
    }
}

HAP_RESULT_USE_CHECK
static bool HAPMACAddressValidateRandomStaticBLEDeviceAddress(HAPMACAddress* macAddress) {
    HAPPrecondition(macAddress);

    // Make random static.
    HAPAssert(sizeof macAddress->bytes == 6);
    macAddress->bytes[0] |= 0xC0U;

    // Check vs invalid BD_ADDR.
    // - The two most significant bits of the address shall be equal to 1.
    // - At least one bit of the random part of the address shall be 0.
    // - At least one bit of the random part of the address shall be 1.
    // See Bluetooth Core Specification Version 5
    // Vol 6 Part B Section 1.3.2.1 Static Device Address
    static const HAPMACAddress invalidMACAddresses[] = {
        { .bytes = { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00 } },
        { .bytes = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } },
    };

    if (HAPRawBufferAreEqual(macAddress->bytes, invalidMACAddresses[0].bytes, sizeof(HAPMACAddress)) ||
        HAPRawBufferAreEqual(macAddress->bytes, invalidMACAddresses[1].bytes, sizeof(HAPMACAddress))) {
        return false;
    }

    return true;
}

HAP_RESULT_USE_CHECK
HAPError HAPMACAddressGetRandomStaticBLEDeviceAddress(
        HAPAccessoryServer* server,
        const char* _Nullable bleInterface,
        HAPMACAddress* macAddress) {
    return HAPMACAddressGet(server, bleInterface, macAddress, HAPMACAddressValidateRandomStaticBLEDeviceAddress);
}
