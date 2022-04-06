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

#include "HAPPlatform+Init.h"
#include "HAPPlatformAccessorySetup+Init.h"
#include "HAPPlatformKeyValueStore+SDKDomains.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "AccessorySetup" };

void HAPPlatformAccessorySetupCreate(
		HAPPlatformAccessorySetupRef _Nonnull accessorySetup,
		const HAPPlatformAccessorySetupOptions* _Nonnull options)
{
	HAPPrecondition(accessorySetup);
	HAPPrecondition(options);
	HAPPrecondition(options->keyValueStore);

	accessorySetup->keyValueStore = options->keyValueStore;
}

void HAPPlatformAccessorySetupLoadSetupInfo(
		HAPPlatformAccessorySetupRef _Nonnull accessorySetup,
		HAPSetupInfo* _Nonnull setupInfo)
{
	HAPPrecondition(accessorySetup);
	HAPPrecondition(setupInfo);

	HAPError err;

	bool found;
	size_t numBytes;
	err = HAPPlatformKeyValueStoreGet(
			accessorySetup->keyValueStore,
			kSDKKeyValueStoreDomain_Provisioning,
			kSDKKeyValueStoreKey_Provisioning_SetupInfo,
			setupInfo,
			sizeof *setupInfo,
			&numBytes,
			&found);
	if (err) {
		HAPAssert(err == kHAPError_Unknown);
		HAPFatalError();
	}
	if (!found) {
		HAPLogError(&logObject, "No setup info found in key-value store.");
		HAPFatalError();
	}
	if (numBytes != sizeof *setupInfo) {
		HAPLogError(&logObject, "Invalid setup info size %zu.", numBytes);
		HAPFatalError();
	}
}

void HAPPlatformAccessorySetupLoadSetupCode(
		HAPPlatformAccessorySetupRef _Nonnull accessorySetup,
		HAPSetupCode* _Nonnull setupCode)
{
	HAPPrecondition(accessorySetup);
	HAPPrecondition(setupCode);

	HAPError err;

	bool found;
	size_t numBytes;
	err = HAPPlatformKeyValueStoreGet(
			accessorySetup->keyValueStore,
			kSDKKeyValueStoreDomain_Provisioning,
			kSDKKeyValueStoreKey_Provisioning_SetupCode,
			setupCode,
			sizeof *setupCode,
			&numBytes,
			&found);
	if (err) {
		HAPAssert(err == kHAPError_Unknown);
		HAPFatalError();
	}
	if (!found) {
		HAPLogError(&logObject, "No setup code found in key-value store.");
		HAPFatalError();
	}
	if (numBytes != sizeof *setupCode) {
		HAPLogError(&logObject, "Invalid setup code size %zu.", numBytes);
		HAPFatalError();
	}
}

void HAPPlatformAccessorySetupLoadSetupID(
		HAPPlatformAccessorySetupRef _Nonnull accessorySetup,
		bool* _Nonnull valid,
		HAPSetupID* _Nonnull setupID)
{
	HAPPrecondition(accessorySetup);
	HAPPrecondition(valid);
	HAPPrecondition(setupID);

	HAPError err;

	size_t numBytes;
	err = HAPPlatformKeyValueStoreGet(
			accessorySetup->keyValueStore,
			kSDKKeyValueStoreDomain_Provisioning,
			kSDKKeyValueStoreKey_Provisioning_SetupID,
			setupID,
			sizeof *setupID,
			&numBytes,
			valid);
	if (err) {
		HAPAssert(err == kHAPError_Unknown);
		HAPFatalError();
	}
	if (!*valid) {
		HAPLog(&logObject, "No setup ID found. QR codes and NFC require provisioning a setup ID.");
		return;
	}
	if (numBytes != sizeof *setupID) {
		HAPLogError(&logObject, "Invalid setup ID size %zu.", numBytes);
		HAPFatalError();
	}
}


//----------------------------------------------------------------------------------------------------------------------
// Deprecated APIs.
#if 0
HAP_DEPRECATED_MSG("Return false and use HAPPlatformAccessorySetupDisplay / HAPPlatformAccessorySetupNFC instead.")
HAP_RESULT_USE_CHECK
HAPPlatformAccessorySetupCapabilities
		HAPPlatformAccessorySetupGetCapabilities(HAPPlatformAccessorySetupRef _Nonnull accessorySetup)
{
	HAPPrecondition(accessorySetup);

	// Deprecated. Return false and use HAPPlatformAccessorySetupDisplay / HAPPlatformAccessorySetupNFC instead.
	return (HAPPlatformAccessorySetupCapabilities){.supportsDisplay = false, .supportsProgrammableNFC = false};
}

void HAPPlatformAccessorySetupUpdateSetupPayload(
		HAPPlatformAccessorySetupRef _Nonnull accessorySetup,
		const HAPSetupPayload* _Nullable setupPayload,
		const HAPSetupCode* _Nullable setupCode)
{
	HAPPrecondition(accessorySetup);

	(void) setupPayload;
	(void) setupCode;
	HAPLogError(&logObject, "[NYI] %s.", __func__);
	HAPFatalError();
}
#endif        

