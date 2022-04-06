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

/* srp-adk.c
 *
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * srp host API implementation for Thread accessories using OpenThread.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <openthread/ip6.h>
#include <openthread/instance.h>
#include <openthread/thread.h>
#include <openthread/joiner.h>
#include <openthread/message.h>
#include <openthread/udp.h>
#include <openthread/platform/time.h>
#include <openthread/platform/settings.h>
#include "srp.h"
#include "srp-adk.h"
#include "srp-api.h"
#include "dns_sd.h"
#include "HAPPlatformRandomNumber.h"
#include "dns-msg.h"
#include "dns_sd.h"
#define SRP_CRYPTO_MBEDTLS_INTERNAL 1
#include "srp-crypto.h"
#include "HAPPlatform.h"
#include "HAP+KeyValueStoreDomains.h"

HAPPlatformTimerRef m_srp_timer = 0;

const char* key_filename = "srp.key";

#define SRP_IO_CONTEXT_MAGIC 0xFEEDFACEFADEBEEFULL // BEES!   Everybody gets BEES!
#define MAX_BIND_ATTEMPTS 16

typedef struct io_context io_context_t;

struct io_context {
    uint64_t magic_cookie1;
    io_context_t* next;
    HAPTime wakeup_time;
    void* NONNULL srp_context;
    otSockAddr sockaddr;
    otUdpSocket sock;
    srp_wakeup_callback_t wakeup_callback;
    srp_datagram_callback_t datagram_callback;
    bool sock_active;
    uint64_t magic_cookie2;
} * io_contexts;

typedef struct{
    HAPIPAddress address;
    HAPNetworkPort port;
} SrpServerData;

static otInstance* otThreadInstance;
static HAPPlatformKeyValueStoreRef keyValueStore;

static int validate_io_context(io_context_t** dest, void* src) {
    io_context_t* context = src;
    if (context->magic_cookie1 == SRP_IO_CONTEXT_MAGIC && context->magic_cookie2 == SRP_IO_CONTEXT_MAGIC) {
        *dest = context;
        return kDNSServiceErr_NoError;
    }
    return kDNSServiceErr_BadState;
}

void datagram_callback(void* context, otMessage* message, const otMessageInfo* messageInfo) {
    static uint8_t* buf;
    const int buf_len = 1500;
    int length;
    io_context_t* io_context;
    if (validate_io_context(&io_context, context) == kDNSServiceErr_NoError) {
        if (buf == NULL) {
            buf = malloc(buf_len);
            if (buf == NULL) {
                INFO("No memory for read buffer");
                return;
            }
        }

        DEBUG("%d bytes received ", otMessageGetLength(message) - otMessageGetOffset(message));
        length = otMessageRead(message, otMessageGetOffset(message), buf, buf_len - 1);
        io_context->datagram_callback(io_context->srp_context, buf, length);
    }
}

static void wakeup_callback(HAPPlatformTimerRef timer, void* _Nullable context);

static void note_wakeup(const char* what, void* at, uint64_t when) {
#ifdef VERBOSE_DEBUG_MESSAGES
    int milliseconds HAP_UNUSED = (int) (when % HAPSecond);
    HAPTime seconds = when / HAPSecond;
    int minute HAP_UNUSED = (int) ((seconds / 60) % 60);
    int hour HAP_UNUSED = (int) ((seconds / 3600) % (7 * 24));
    int second HAP_UNUSED = (int) (seconds % 60);

    DEBUG("%s %p at %llu  %d:%d:%d.%d", what, at, when, hour, minute, second, milliseconds);
#endif
}

static void compute_wakeup_time(HAPTime now) {
    io_context_t* io_context;
    HAPTime next = 0;

    for (io_context = io_contexts; io_context; io_context = io_context->next) {
        if (next == 0 || (io_context->wakeup_time != 0 && io_context->wakeup_time < next)) {
            next = io_context->wakeup_time;
        }
    }

    // If we don't have a wakeup to schedule, wake up anyway in ten seconds.
    if (next == 0) {
        next = now + 10 * HAPSecond;
    }

    HAPTime minNext = HAPPlatformClockGetCurrent() + (50*HAPMillisecond);    
    if ( next < minNext ){
        // ensure next is always in the future
        next = minNext;
    }

    note_wakeup("next wakeup", NULL, next);
    if (next != 0) {
        if (m_srp_timer) {
            // Terminate existing timer
            HAPPlatformTimerDeregister(m_srp_timer);
            m_srp_timer = 0;
        }
        HAPError err = HAPPlatformTimerRegister(&m_srp_timer, next, wakeup_callback, NULL);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            ERROR("Timer Register returned %d", err);
            HAPFatalError();
        }
    }
}

static void wakeup_callback(HAPPlatformTimerRef timer, void* _Nullable context) {

    io_context_t* io_context;
    HAPTime now = HAPPlatformClockGetCurrent();
    bool more;
    m_srp_timer = 0;
    note_wakeup("     wakeup", NULL, now);
    do {
        more = false;
        for (io_context = io_contexts; io_context; io_context = io_context->next) {
            if (io_context->wakeup_time != 0 && io_context->wakeup_time <= now) {
                more = true;
                note_wakeup("io wakeup", io_context, io_context->wakeup_time);
                io_context->wakeup_time = 0;
                io_context->wakeup_callback(io_context->srp_context);
                break;
            }
            note_wakeup("no wakeup", io_context, io_context->wakeup_time);
        }
    } while (more);

    compute_wakeup_time(now);
}

int srp_deactivate_udp_context(void* host_context, void* in_context) {
    io_context_t *io_context, **p_io_contexts;
    int err;

    err = validate_io_context(&io_context, in_context);
    if (err == kDNSServiceErr_NoError) {
        for (p_io_contexts = &io_contexts; *p_io_contexts; p_io_contexts = &(*p_io_contexts)->next) {
            if (*p_io_contexts == io_context) {
                break;
            }
        }
        // If we don't find it on the list, something is wrong.
        if (*p_io_contexts == NULL) {
            return kDNSServiceErr_Invalid;
        }
        *p_io_contexts = io_context->next;
        io_context->wakeup_time = 0;
        if (io_context->sock_active) {
            otUdpClose(&io_context->sock);
        }
        free(io_context);
    }
    return err;
}

int srp_connect_udp(
        void* context,
        const uint8_t* port,
        uint16_t address_type,
        const uint8_t* address,
        uint16_t addrlen) {
    io_context_t* io_context;
    int err, oterr;

    err = validate_io_context(&io_context, context);

    if (err == kDNSServiceErr_NoError) {
        if (address_type != dns_rrtype_aaaa || addrlen != 16) {
            ERROR("srp_make_udp_context: invalid address");
            return kDNSServiceErr_Invalid;
        }
        memcpy(&io_context->sockaddr.mAddress, address, 16);
        memcpy(&io_context->sockaddr.mPort, port, 2);
#ifdef OT_NETIF_INTERFACE_ID_THREAD
        io_context->sockaddr.mScopeId = OT_NETIF_INTERFACE_ID_THREAD;
#endif

        oterr = otUdpOpen(otThreadInstance, &io_context->sock, datagram_callback, io_context);
        if (oterr != OT_ERROR_NONE) {
            ERROR("srp_make_udp_context: otUdpOpen returned %d", oterr);
            return kDNSServiceErr_Unknown;
        }

        otSockAddr  sockaddr;
        uint16_t bindAttempts = 0;
        do
        {
            HAPRawBufferZero(&sockaddr, sizeof(sockaddr));
            HAPPlatformRandomNumberFill(&sockaddr.mPort, sizeof(sockaddr.mPort));
            oterr = otUdpBind(&io_context->sock, &sockaddr);
            bindAttempts++;

        } while (oterr != OT_ERROR_NONE && bindAttempts < MAX_BIND_ATTEMPTS);
        
        if (oterr != OT_ERROR_NONE) {
            otUdpClose(&io_context->sock);
            ERROR("srp_make_udp_context: otUdpBind returned %d", oterr);
            return kDNSServiceErr_Unknown;
        }

        oterr = otUdpConnect(&io_context->sock, &io_context->sockaddr);
        if (oterr != OT_ERROR_NONE) {
            otUdpClose(&io_context->sock);
            ERROR("srp_make_udp_context: otUdpConnect returned %d", oterr);
            return kDNSServiceErr_Unknown;
        }
        io_context->sock_active = true;
        err = kDNSServiceErr_NoError;
    }
    return err;
}

int srp_disconnect_udp(void* context) {
    io_context_t* io_context;
    int err;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError && io_context->sock_active) {
        otUdpClose(&io_context->sock);
        io_context->sock_active = false;
    }
    return err;
}

int srp_make_udp_context(void* host_context, void** p_context, srp_datagram_callback_t callback, void* context) {
    io_context_t* io_context = calloc(1, sizeof *io_context);
    if (io_context == NULL) {
        ERROR("srp_make_udp_context: no memory");
        return kDNSServiceErr_NoMemory;
    }
    io_context->magic_cookie1 = io_context->magic_cookie2 = SRP_IO_CONTEXT_MAGIC;
    io_context->datagram_callback = callback;
    io_context->srp_context = context;

    *p_context = io_context;
    io_context->next = io_contexts;
    io_contexts = io_context;
    return kDNSServiceErr_NoError;
}

int srp_set_wakeup(void* host_context, void* context, int milliseconds, srp_wakeup_callback_t callback) {
    int err;
    io_context_t* io_context;
    HAPTime now;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        now = HAPPlatformClockGetCurrent();
        io_context->wakeup_time = now + milliseconds * (HAPSecond / 1000);
        io_context->wakeup_callback = callback;
        INFO("srp_set_wakeup: %llu (%llu + %dms)", io_context->wakeup_time, now, milliseconds);
        compute_wakeup_time(now);
    }
    return err;
}

int srp_cancel_wakeup(void* host_context, void* context) {
    int err;
    io_context_t* io_context;

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        io_context->wakeup_time = 0;
    }
    return err;
}

int srp_send_datagram(void* host_context, void* context, void* payload, size_t message_length) {
    int err;
    io_context_t* io_context;
    otError error;
    otMessageInfo messageInfo;
    otMessage* message = NULL;
    uint8_t* ap;

#ifdef VERBOSE_DEBUG_MESSAGES
    int i, j;
    char buf[80], *bufp;
    char* hexdigits = "01234567689abcdef";
    uint8_t* msg = payload;
#endif // VERBOSE_DEBUG_MESSAGES

    err = validate_io_context(&io_context, context);
    if (err == kDNSServiceErr_NoError) {
        memset(&messageInfo, 0, sizeof(messageInfo));
#ifdef OT_NETIF_INTERFACE_ID_THREAD
        messageInfo.mInterfaceId = OT_NETIF_INTERFACE_ID_THREAD;
#endif
        messageInfo.mPeerPort = io_context->sockaddr.mPort;
        messageInfo.mPeerAddr = io_context->sockaddr.mAddress;
        ap = (uint8_t*) &io_context->sockaddr.mAddress;
        INFO("Sending to %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x port %d",
             ap[0],
             ap[1],
             ap[2],
             ap[3],
             ap[4],
             ap[5],
             ap[6],
             ap[7],
             ap[8],
             ap[9],
             ap[10],
             ap[11],
             ap[12],
             ap[13],
             ap[14],
             ap[15],
             io_context->sockaddr.mPort);
#ifdef VERBOSE_DEBUG_MESSAGES
        for (i = 0; i < message_length; i += 32) {
            bufp = buf;
            for (j = 0; bufp < buf + sizeof buf && i + j < message_length; j++) {
                *bufp++ = hexdigits[msg[i + j] >> 4];
                if (bufp < buf + sizeof buf) {
                    *bufp++ = hexdigits[msg[i + j] % 15];
                }
                if (bufp < buf + sizeof buf && (j & 1) == 1) {
                    *bufp++ = ' ';
                }
            }
            *bufp = 0;
            DEBUG("%s", buf);
        }
#endif

        message = otUdpNewMessage(otThreadInstance, NULL);
        if (message == NULL) {
            ERROR("srp_send_datagram: otUdpNewMessage returned NULL");
            return kDNSServiceErr_NoMemory;
        }

        error = otMessageAppend(message, payload, message_length);
        if (error != OT_ERROR_NONE) {
            ERROR("srp_send_datagram: otMessageAppend returned %d", error);
            otMessageFree(message);
            return kDNSServiceErr_NoMemory;
        }

        // if successful, send consumes the message.
        error = otUdpSend(&io_context->sock, message, &messageInfo);
        if (error != OT_ERROR_NONE) {
            otMessageFree(message);
            ERROR("srp_send_datagram: otUdpSend returned %d", error);
            return kDNSServiceErr_Unknown;
        }
    }
    return err;
}

int srp_load_key_data(
        void* host_context,
        const char* key_name,
        uint8_t* buffer,
        uint16_t* length,
        uint16_t buffer_size) {
#ifndef DEBUG_CONFLICTS
    
    // Note that at present we ignore the key name: we are only going to have one host key on an
    // accessory.    
    size_t maxSize = buffer_size;
    size_t sizeFound = 0;
    bool found = false;
    HAPError err = HAPPlatformKeyValueStoreGet(
        keyValueStore,
        kHAPKeyValueStoreDomain_Configuration,
        kHAPKeyValueStoreKey_Configuration_SrpKey,
        buffer, 
        maxSize,
        &sizeFound,
        &found);
    if (err != kHAPError_None || !found) {
        *length = 0;
        return kDNSServiceErr_NoSuchKey;
    }
    *length = (uint16_t)sizeFound;
    return kDNSServiceErr_NoError;
#else
    return kDNSServiceErr_NoSuchKey;
#endif
}

int srp_store_key_data(void* host_context, const char* name, uint8_t* buffer, uint16_t length) {
    HAPError err = HAPPlatformKeyValueStoreSet(
        keyValueStore,
        kHAPKeyValueStoreDomain_Configuration,
        kHAPKeyValueStoreKey_Configuration_SrpKey,
        buffer,
        (size_t)length);
    
    if (err != kHAPError_None) {
        ERROR("Unable to store key (length %d): %d", length, err);
        return kDNSServiceErr_Unknown;
    }
    return kDNSServiceErr_NoError;
}

int srp_reset_key(const char *NONNULL key_name, void *NULLABLE host_context)
{
    HAPError err = HAPPlatformKeyValueStoreRemove(
        keyValueStore,
        kHAPKeyValueStoreDomain_Configuration,
        kHAPKeyValueStoreKey_Configuration_SrpKey);
    if (err != kHAPError_None) {
        ERROR("Unable to clear key %s", key_name );
        return kDNSServiceErr_Unknown;
    }
    return kDNSServiceErr_NoError;
}

bool
srp_get_last_server(uint16_t *NONNULL rrtype, uint8_t *NONNULL rdata, uint16_t rdlim,
                    uint8_t *NONNULL port, void *NULLABLE host_context)
{
    SrpServerData data;
    size_t sizeFound = 0;
    bool found = false;
    HAPError err = HAPPlatformKeyValueStoreGet(
        keyValueStore,
        kHAPKeyValueStoreDomain_Configuration,
        kHAPKeyValueStoreKey_Configuration_SrpServer,
        &data, 
        sizeof(data),
        &sizeFound,
        &found);
    if (err != kHAPError_None || !found) {        
        return false;
    }

    uint8_t* address = NULL;
    uint16_t addrSize = 0;

    switch (data.address.version)
    {
        case kHAPIPAddressVersion_IPv4:
            *rrtype = dns_rrtype_a;
            address = data.address._.ipv4.bytes;
            addrSize = sizeof(data.address._.ipv4.bytes);
            break;
        case kHAPIPAddressVersion_IPv6:
            *rrtype = dns_rrtype_aaaa;
            address = data.address._.ipv6.bytes;
            addrSize = sizeof(data.address._.ipv6.bytes);
            break;
        default:
            ERROR("Invalid address type %d", data.address.version );
            return false;
    }

    if ( addrSize > rdlim )
    {
        ERROR("Buffer len %d not large enough for version %s", 
              rdlim, data.address.version == kHAPIPAddressVersion_IPv4 ? "A" : "AAAA");
        return false;
    }

    HAPRawBufferCopyBytes(rdata, address, addrSize);
    HAPRawBufferCopyBytes(port, &data.port, 2);
    return true;
}

bool
srp_save_last_server(uint16_t rrtype, uint8_t *NONNULL rdata, uint16_t rdlength,
                     uint8_t *NONNULL port, void *NULLABLE host_context)
{
    SrpServerData data;
    switch (rrtype)
    {
        case dns_rrtype_a:
            HAPAssert(rdlength >= sizeof(data.address._.ipv4.bytes));
            data.address.version = kHAPIPAddressVersion_IPv4;
            HAPRawBufferCopyBytes(data.address._.ipv4.bytes, rdata, sizeof(data.address._.ipv4.bytes));
            break;
        case dns_rrtype_aaaa:
            
            HAPAssert(rdlength >= sizeof(data.address._.ipv6.bytes));
            data.address.version = kHAPIPAddressVersion_IPv6;
            HAPRawBufferCopyBytes(data.address._.ipv6.bytes, rdata, sizeof(data.address._.ipv6.bytes));
            break;
        default:
            ERROR("invalid rrtype %d", rrtype);
            return false;
    }

    HAPRawBufferCopyBytes(&data.port, port, sizeof(data.port));

    HAPError err = HAPPlatformKeyValueStoreSet(
        keyValueStore,
        kHAPKeyValueStoreDomain_Configuration,
        kHAPKeyValueStoreKey_Configuration_SrpServer,
        &data,
        sizeof(data));

    if (err != kHAPError_None) {
        ERROR("Unable to store server %d", err);
        return false;
    }
    return true;
}


void register_callback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        DNSServiceErrorType errorCode,
        const char* name,
        const char* regtype,
        const char* domain,
        void* context) {
    INFO("Register Reply: %ld %s %s %s\n",
         errorCode,
         name == NULL ? "<NULL>" : name,
         regtype == NULL ? "<NULL>" : regtype,
         domain == NULL ? "<NULL>" : domain);
}

void conflict_callback(const char* hostname) {
    ERROR("Host name conflict: %s", hostname);
}

int srp_thread_init(otInstance* instance, HAPPlatformKeyValueStoreRef kvs) {
    otThreadInstance = instance;
    keyValueStore = kvs;
    static bool srpInitialized = false;
    if ( !srpInitialized )
    {
        int err = srp_host_init(otThreadInstance);
        HAPAssert(err == kDNSServiceErr_NoError);
        srpInitialized = true;
    }
    return kDNSServiceErr_NoError;
}

int srp_thread_shutdown(otInstance* instance) {
    INFO("In srp_thread_shutdown().");
    if (m_srp_timer) {
        HAPPlatformTimerDeregister(m_srp_timer);
    }
    m_srp_timer = 0;
    int srp_err = srp_deregister(otThreadInstance);
    HAPAssert(srp_err == kDNSServiceErr_NoError ||
              srp_err == kDNSServiceErr_NoSuchRecord);
    return kDNSServiceErr_NoError;
}

void srp_thread_reset_key(){
    int err = srp_host_key_reset();
    HAPAssert(err == kDNSServiceErr_NoError);
}

void srp_openlog(int option) {
}
void srp_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    HAPVLogInfo(&kHAPLog_Default, fmt, args);
    va_end(args);
}

void srp_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    HAPVLogDebug(&kHAPLog_Default, fmt, args);
    va_end(args);
}

void srp_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    HAPVLogError(&kHAPLog_Default, fmt, args);
    va_end(args);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
