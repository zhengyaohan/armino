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

#include "HAP.h"
#include "HAPPlatformKeyValueStore+Init.h"

#include <mbedtls/platform.h>
#include "bk_api_mem.h"
#include "bk_api_rtos.h"
#include "sys_config.h"


#ifndef CONFIG_MBEDTLS
#error "homekit need MBEDTLS."
#endif

#ifndef CONFIG_MDNS
#error "homekit need MDNS."
#endif

#ifndef CONFIG_AUTOIP
#error "homekit need AUTOIP."
#endif

#if 0
#if defined(MBEDTLS_CONFIG_FILE)
        #error "Invalid MBEDTLS_CONFIG_FILE."
#endif

#if !defined(MBEDTLS_PLATFORM_C)
        #error "need MBEDTLS_PLATFORM_C."
#endif

#if !defined(MBEDTLS_PLATFORM_MEMORY)
        #error "need MBEDTLS_PLATFORM_MEMORY."
#endif
#endif

void HAPPlatformSetupDrivers(void) {
	//Tell MbedTLS to use buffer allocator functions available on Nordic
	//  If MbedTLS crypto is being used then tell MbedTLS to use buffer allocator functions
#if 0
    int ret = mbedtls_platform_set_calloc_free(os_calloc_, os_free);
    HAPAssert(ret == 0);
    ret = mbedtls_platform_setup(NULL);
    HAPAssert(ret == 0);
#endif
}

void HAPPlatformSetupInitBLE(
		HAPPlatformBLEPeripheralManager* blePeripheralManager,
		HAPPlatformKeyValueStoreRef keyValueStore)
{
	// Do nothing
}

void HAPPlatformSetupDeinitBLE(void)
{
	// Do nothing
}

void HAPPlatformSetupInitKeyValueStore(HAPPlatformKeyValueStoreRef keyValueStoreRef)
{
	// Create key-value store
    HAPPlatformKeyValueStoreCreate(
            keyValueStoreRef, &(const HAPPlatformKeyValueStoreOptions) { .flash_base_addr = 0x0180000 });
}

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC)
void HAPPlatformSetupInitAccessorySetupNFC(HAPPlatformAccessorySetupNFCRef setupNFC)
{
	// Do nothing
}
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_MFI_HW_AUTH)
void HAPPlatformSetupInitMFiHWAuth(HAPPlatformMFiHWAuthRef mfiHWAuth)
{
	// Do nothing
}
#endif
