

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
//#include <arpa/inet.h>
//#include <unistd.h>
#include "HAPPlatform+Init.h"
#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformTimer.h"
#include <string.h>
#include "bk_api_mem.h"
#include "bk_api_rtos.h"
#include "bk_api_str.h"



static const HAPLogObject logObject =
{
    .subsystem = kHAPPlatform_LogSubsystem, .category = "ServiceDiscovery"
};


#define TTL_DEFAULT             90

// TODO Add support for re-registering service discovery in case of error while app is running.
#if 0


static void HandleFileHandleCallback(HAPPlatformFileHandleRef fileHandle, 
    HAPPlatformFileHandleEvent fileHandleEvents, 
    void * _Nullable context)
{
    HAPAssert(fileHandle);
    HAPAssert(fileHandleEvents.isReadyForReading);
    HAPAssert(context);

    HAPPlatformServiceDiscoveryRef serviceDiscovery = context;

    HAPAssert(serviceDiscovery->fileHandle == fileHandle);

    DNSServiceErrorType errorCode = DNSServiceProcessResult(serviceDiscovery->dnsService);

    if (errorCode != kDNSServiceErr_NoError)
    {
        HAPLogError(&logObject, "%s: Service discovery results processing failed: %ld.", __func__, (long) errorCode);
        HAPFatalError();
    }
}


static void HandleServiceRegisterReply(DNSServiceRef service HAP_UNUSED, 
    DNSServiceFlags flags HAP_UNUSED, 
    DNSServiceErrorType errorCode, 
    const char * name HAP_UNUSED, 
    const char * regtype HAP_UNUSED, 
    const char * domain HAP_UNUSED, 
    void * context_ HAP_UNUSED)
{
    if (errorCode != kDNSServiceErr_NoError)
    {
        HAPLogError(&logObject, "%s: Service discovery registration failed: %ld.", __func__, (long) errorCode);
        HAPFatalError();
    }
}


#endif




void make_txt_data(struct mdns_service * service, void * txt_userdata)
{
    mdns_resp_add_service_txtall(service, (char *) TXTRecordGetBytesPtr((TXTRecordRef *) txt_userdata), 
        TXTRecordGetLength((TXTRecordRef *) txt_userdata));
}


void HAPPlatformServiceDiscoveryCreate(HAPPlatformServiceDiscoveryRef serviceDiscovery, 
    const HAPPlatformServiceDiscoveryOptions * options)
{
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(options);

    HAPLogDebug(&logObject, "Storage configuration: serviceDiscovery = %lu", 
        (unsigned long) sizeof * serviceDiscovery);

    HAPRawBufferZero(serviceDiscovery, sizeof * serviceDiscovery);

    if (options->interfaceName)
    {
        size_t numInterfaceNameBytes = HAPStringGetNumBytes(HAPNonnull(options->interfaceName));

        if ((numInterfaceNameBytes == 0) || (numInterfaceNameBytes >= sizeof serviceDiscovery->interfaceName))
        {
            HAPLogError(&logObject, "Invalid local network interface name.");
            HAPFatalError();
        }
        HAPRawBufferCopyBytes(serviceDiscovery->interfaceName, HAPNonnull(options->interfaceName), 
            numInterfaceNameBytes);
    }
    mdns_resp_init();
#if 0    
    if ((infc = netif_find(serviceDiscovery->interfaceName)) != NULL)
    {
        os_printf("netif_find ok \r\n");
    }
    else 
    {
        os_printf("!!! not find netinfc:%s \r\n", serviceDiscovery->interfaceName);
    }
#endif

    serviceDiscovery->dnsService = NULL;
}


void HAPPlatformServiceDiscoveryRegister(HAPPlatformServiceDiscoveryRef serviceDiscovery, 
    const char * name, 
    const char * protocol, 
    HAPNetworkPort port, 
    HAPPlatformServiceDiscoveryTXTRecord * txtRecords, 
    size_t numTXTRecords)
{
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(!serviceDiscovery->dnsService);
    HAPPrecondition(name);
    HAPPrecondition(protocol);
    HAPPrecondition(txtRecords);

    //HAPError err;
    err_t errorCode;

    uint32_t interfaceIndex=1;
    char tmp_proto[64];
    strncpy(tmp_proto, protocol, sizeof(tmp_proto));
    char * srv_type = strtok(tmp_proto, ".");
    char * proto = strtok(NULL, ".");
    HAPLogDebug(&logObject, "srv_type: %s ", srv_type);
    HAPLogDebug(&logObject, "proto: %s ", proto);

    UINT8 proto_val = DNSSD_PROTO_TCP;
    if (os_memcmp(proto, "_udp", 4) == 0)
    {
        proto_val = DNSSD_PROTO_UDP;
    }

    if (serviceDiscovery->netif)
    {
    mdns_resp_remove_netif(serviceDiscovery->netif);
                serviceDiscovery->netif = NULL;
                serviceDiscovery->dnsService = NULL;
      }
    
    if( (serviceDiscovery->netif = (void *)netif_find_support_igmp()) != NULL)
    {
            errorCode = mdns_resp_add_netif(serviceDiscovery->netif , "beken", TTL_DEFAULT);

            if (errorCode != ERR_OK)
            {
                HAPLogError(&logObject, "%s: mdns_resp_add_netif failed: %ld.", __func__, (long) errorCode);
                HAPFatalError();
            }        
    }
    else
        {
        HAPLogError(&logObject, "%s: netif_find_support_igmp  failed.", __func__);
                HAPFatalError();
        }

    if (HAPStringAreEqual(protocol, "_mfi-config._tcp") && interfaceIndex && HAP_FEATURE_ENABLED(HAP_FEATURE_WAC))
    {
        // We noticed that service registration has no effect immediately after a change in the network interface
        // configuration, e.g., when switching in and out of a software access point for WAC. A delay of about 15
        // seconds seems to circumvent this problem.
        HAPLogInfo(&logObject, "Delaying service registration ...");
        unsigned int delay = 15 /* seconds */;
        rtos_thread_sleep(delay);

    }

    HAPLogDebug(&logObject, "interfaceIndex: %lu", (unsigned long) interfaceIndex);
    HAPLogDebug(&logObject, "name: \"%s\"", name);
    HAPLogDebug(&logObject, "protocol: \"%s\"", protocol);

    HAPLogDebug(&logObject, "port: %u", port);

    TXTRecordCreate(&serviceDiscovery->txtRecord, 
        sizeof serviceDiscovery->txtRecordBuffer, 
        &serviceDiscovery->txtRecordBuffer[0]);
    for (size_t i = 0; i < numTXTRecords; i++)
    {
        HAPPrecondition(!txtRecords[i].value.numBytes || txtRecords[i].value.bytes);
        HAPPrecondition(txtRecords[i].value.numBytes <= UINT8_MAX);
        if (txtRecords[i].value.bytes)
        {
            HAPLogBufferDebug(&logObject, 
                txtRecords[i].value.bytes, 
                txtRecords[i].value.numBytes, 
                "txtRecord[%lu]: \"%s\"", 
                (unsigned long) i, 
                txtRecords[i].key);
        }
        else 
        {
            HAPLogDebug(&logObject, "txtRecord[%lu]: \"%s\"", (unsigned long) i, txtRecords[i].key);
        }
        errorCode = TXTRecordSetValue(&serviceDiscovery->txtRecord, 
            txtRecords[i].key, 
            (uint8_t) txtRecords[i].value.numBytes, 
            txtRecords[i].value.bytes);
        if (errorCode != kDNSServiceErr_NoError)
        {
            HAPLogError(&logObject, "%s: TXTRecordSetValue failed: %ld.", __func__, (long) errorCode);
            HAPFatalError();
        }
    }

    errorCode = mdns_resp_add_service(serviceDiscovery->netif, name, srv_type, proto_val, port, TTL_DEFAULT, &make_txt_data, 
        &serviceDiscovery->txtRecord, &serviceDiscovery->dnsService);

    if (errorCode != ERR_OK)
    {
        HAPLogError(&logObject, "%s: mdns_resp_add_service failed: %ld.", __func__, (long) errorCode);
        HAPFatalError();
    }

#if 0
    err = HAPPlatformFileHandleRegister(&serviceDiscovery->fileHandle, 
        DNSServiceRefSockFD(serviceDiscovery->dnsService), 
        (HAPPlatformFileHandleEvent) {.isReadyForReading = true, .isReadyForWriting = false, 
        .hasErrorConditionPending = false }, 
        HandleFileHandleCallback, 
        serviceDiscovery);
    if (err)
    {
        HAPLogError(&logObject, "%s: HAPPlatformFileHandleRegister failed: %u.", __func__, err);
        HAPFatalError();
    }
    HAPAssert(serviceDiscovery->fileHandle);
#endif
}


void HAPPlatformServiceDiscoveryUpdateTXTRecords(HAPPlatformServiceDiscoveryRef serviceDiscovery, 
    HAPPlatformServiceDiscoveryTXTRecord * txtRecords, 
    size_t numTXTRecords)
{
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(serviceDiscovery->dnsService);
    HAPPrecondition(txtRecords);

    DNSServiceErrorType errorCode;


    TXTRecordDeallocate(&serviceDiscovery->txtRecord);
    TXTRecordCreate(&serviceDiscovery->txtRecord, 
        sizeof serviceDiscovery->txtRecordBuffer, 
        &serviceDiscovery->txtRecordBuffer[0]);
    for (size_t i = 0; i < numTXTRecords; i++)
    {
        HAPPrecondition(!txtRecords[i].value.numBytes || txtRecords[i].value.bytes);
        HAPPrecondition(txtRecords[i].value.numBytes <= UINT8_MAX);
        if (txtRecords[i].value.bytes)
        {
            HAPLogBufferDebug(&logObject, 
                txtRecords[i].value.bytes, 
                txtRecords[i].value.numBytes, 
                "txtRecord[%lu]: \"%s\"", 
                (unsigned long) i, 
                txtRecords[i].key);
        }
        else 
        {
            HAPLogDebug(&logObject, "txtRecord[%lu]: \"%s\"", (unsigned long) i, txtRecords[i].key);
        }
        errorCode = TXTRecordSetValue(&serviceDiscovery->txtRecord, 
            txtRecords[i].key, 
            (uint8_t) txtRecords[i].value.numBytes, 
            txtRecords[i].value.bytes);
        if (errorCode != kDNSServiceErr_NoError)
        {
            HAPLogError(&logObject, "%s: TXTRecordSetValue failed: %ld.", __func__, (long) errorCode);
            HAPFatalError();
        }
    }

    mdns_resp_update_service_txt_userdata((struct mdns_service *) (serviceDiscovery->dnsService), 
        (void *) &serviceDiscovery->txtRecord);

    errorCode = kDNSServiceErr_NoError;
    mdns_resp_netif_settings_changed(serviceDiscovery->netif);

    if (errorCode != kDNSServiceErr_NoError)
    {
        HAPLogError(&logObject, "%s: DNSServiceUpdateRecord failed: %ld.", __func__, (long) errorCode);
        HAPFatalError();
    }
}


void HAPPlatformServiceDiscoveryStop(HAPPlatformServiceDiscoveryRef serviceDiscovery)
{
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(serviceDiscovery->dnsService);

#if 0
    HAPPlatformFileHandleDeregister(serviceDiscovery->fileHandle);

    DNSServiceRefDeallocate(serviceDiscovery->dnsService);
#endif
    if(serviceDiscovery->netif)
    {
        mdns_resp_remove_netif(serviceDiscovery->netif);
        serviceDiscovery->netif = NULL;
        serviceDiscovery->dnsService = NULL;
    }

    TXTRecordDeallocate(&serviceDiscovery->txtRecord);
}


