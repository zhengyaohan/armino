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

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)

#include <openthread/thread.h>

#include "HAPPlatformThreadCoAPManager+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "CoAPManager" };

/** Maximum message length to be processed. */
#define kHAPPlatformThreadCoAPManager_MaxMessageBytes ((uint16_t) 2048)

/**
 * Maximum short response length
 *
 * This corresponds to an error PDU (5 bytes) plus CHACHA20_POLY1305_TAG_BYTES (16 bytes)
 */
#define kHAPPlatformThreadCoAPManager_MaxShortResponseBytes ((uint16_t) 21)

/**
 * Retrieves the class number of a CoAP code.
 *
 * @param      code                 CoAP code.
 *
 * @return Class number of given CoAP code.
 */
#define GetClass(code) ((uint8_t)((uint8_t)((code) >> 5u) & 0x07u))

/**
 * Retrieves the detail number of a CoAP code.
 *
 * @param      code                 CoAP code.
 *
 * @return Detail number of given CoAP code.
 */
#define GetDetail(code) ((uint8_t)((uint8_t)((code) >> 0u) & 0x1Fu))

static HAPPlatformThreadWakeLock kCoapManagerWakelock;
#define COAP_WAKELOCK_TIMEOUT_MS 5000

#define MAX_IPV6_STR_LEN 40

void HAPPlatformThreadCoAPManagerCreate(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerOptions* options) {
    HAPPrecondition(coapManager);
    HAPPrecondition(options);
    HAPPrecondition(options->resources);
    HAPPrecondition(options->requests);

    HAPRawBufferZero(coapManager, sizeof *coapManager);
    coapManager->port = options->port;
    HAPRawBufferZero(options->resources, options->numResources * sizeof *options->resources);
    coapManager->resources = options->resources;
    coapManager->numResources = options->numResources;
    HAPRawBufferZero(options->requests, options->numRequests * sizeof *options->requests);
    coapManager->requests = options->requests;
    coapManager->numRequests = options->numRequests;

    coapManager->otInstance = (otInstance*) HAPPlatformThreadGetHandle();
}

void HAPPlatformThreadCoAPManagerSetDelegate(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerDelegate* _Nullable delegate) {
    HAPPrecondition(coapManager);

    if (delegate) {
        HAPRawBufferCopyBytes(&coapManager->delegate, HAPNonnull(delegate), sizeof coapManager->delegate);
    } else {
        HAPRawBufferZero(&coapManager->delegate, sizeof coapManager->delegate);
    }
}

HAP_RESULT_USE_CHECK
static HAPPlatformThreadCoAPManagerResourceRef GetResourceHandle(const HAPPlatformThreadCoAPManagerResource* resource) {
    HAPPrecondition(resource);
    HAPPlatformThreadCoAPManagerRef coapManager = resource->coapManager;

    HAPPlatformThreadCoAPManagerResourceRef coapResource =
            (HAPPlatformThreadCoAPManagerResourceRef)(resource - coapManager->resources) + 1u;
    HAPAssert(coapResource);
    HAPAssert(coapResource <= coapManager->numResources);
    return coapResource;
}

static void HandleCoAPRequestDefault(void* _Nullable aContext, otMessage* aMessage, const otMessageInfo* aMessageInfo) {
    HAPPrecondition(!aContext);
    HAPPrecondition(aMessage);
    HAPPrecondition(aMessageInfo);

    HAPLog(&logObject, "Received CoAP message that does not match any request or resource.");
}

void HAPPlatformThreadCoAPManagerStart(HAPPlatformThreadCoAPManagerRef coapManager) {
    HAPPrecondition(coapManager);
    HAPPrecondition(!coapManager->coapIsActive);

    otError e;

    HAPLogInfo(&logObject, "Starting CoAP manager at port %u.", coapManager->port);
    e = otCoapStart(coapManager->otInstance, coapManager->port);
    HAPAssert(!e);

    otCoapSetDefaultHandler(coapManager->otInstance, HandleCoAPRequestDefault, /* aContext: */ NULL);

    coapManager->coapIsActive = true;
}

void HAPPlatformThreadCoAPManagerStop(HAPPlatformThreadCoAPManagerRef coapManager) {
    HAPPrecondition(coapManager);

    otError e;

    if (!coapManager->coapIsActive) {
        return;
    }

    for (size_t i = 0; i < coapManager->numResources; i++) {
        HAPPlatformThreadCoAPManagerResource* resource = &coapManager->resources[i];
        if (resource->coapManager) {
            HAPPlatformThreadCoAPManagerRemoveResource(coapManager, GetResourceHandle(resource));
        }
    }

    otCoapSetDefaultHandler(coapManager->otInstance, /* aHandler: */ NULL, /* aContext: */ NULL);

    HAPLogInfo(&logObject, "Stopping CoAP manager at port %u.", coapManager->port);
    e = otCoapStop(coapManager->otInstance); // This also aborts all pending requests.
    HAPAssert(!e);

    coapManager->coapIsActive = false;
}

HAP_RESULT_USE_CHECK
HAPNetworkPort HAPPlatformThreadCoAPManagerGetServerPort(HAPPlatformThreadCoAPManagerRef coapManager) {
    HAPPrecondition(coapManager);
    HAPPrecondition(coapManager->coapIsActive);

    return coapManager->port;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerGetPrimaryIPAddress(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPIPAddress* primaryIPAddress) {
    HAPPrecondition(coapManager);
    HAPPrecondition(coapManager->coapIsActive);
    HAPPrecondition(primaryIPAddress);
    HAPRawBufferZero(primaryIPAddress, sizeof *primaryIPAddress);

    const otMeshLocalPrefix* meshPrefix = otThreadGetMeshLocalPrefix(coapManager->otInstance);
    if (!meshPrefix) {
        HAPLogError(&logObject, "%s failed.", "otThreadGetMeshLocalPrefix");
        return kHAPError_Unknown;
    }
    HAPAssert(sizeof meshPrefix->m8 <= sizeof(otIp6Address));

    uint8_t highestPriority = 0;
    for (const otNetifAddress* addr = otIp6GetUnicastAddresses(coapManager->otInstance); addr; addr = addr->mNext) {
        // See https://openthread.io/guides/thread-primer/ipv6-addressing
        uint8_t priority = 0;
        if (addr->mAddress.mFields.m8[0] == 0xFE && addr->mAddress.mFields.m8[1] == 0x80) {
            // Link-local address (not routable).
            priority = 1;
        } else if (addr->mRloc) {
            // Routing locator (not stable).
            priority = 2;
        } else if (
                addr->mAddress.mFields.m8[8] == 0x00 && addr->mAddress.mFields.m8[9] == 0x00 &&
                addr->mAddress.mFields.m8[10] == 0x00 && addr->mAddress.mFields.m8[11] == 0xFF &&
                addr->mAddress.mFields.m8[12] == 0xFE && addr->mAddress.mFields.m8[13] == 0x00 &&
                addr->mAddress.mFields.m8[14] == 0xFC) {
            // Anycast locator (RLOC lookup).
            priority = 3;
        } else if (HAPRawBufferAreEqual(addr->mAddress.mFields.m8, meshPrefix->m8, sizeof meshPrefix->m8)) {
            // Mesh-local EID.
            priority = 4;
        } else if (addr->mAddress.mFields.m8[0] >= 0x20 && addr->mAddress.mFields.m8[0] <= 0x3F) {
            // Global unicast address.
            priority = 5;
        } else {
            // Custom address.
            priority = 6;
        }

        if (priority > highestPriority) {
            primaryIPAddress->version = kHAPIPAddressVersion_IPv6;

            HAPAssert(sizeof primaryIPAddress->_.ipv6.bytes == sizeof addr->mAddress.mFields.m8);
            HAPRawBufferCopyBytes(
                    primaryIPAddress->_.ipv6.bytes, addr->mAddress.mFields.m8, sizeof primaryIPAddress->_.ipv6.bytes);

            highestPriority = priority;
        }
    }
    if (!highestPriority) {
        HAPLogError(&logObject, "%s failed.", "otIp6GetUnicastAddresses");
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static bool IsSupportedRequestCode(otCoapCode requestCode) {
    switch (requestCode) {
        case OT_COAP_CODE_EMPTY:
        case OT_COAP_CODE_GET:
        case OT_COAP_CODE_POST:
        case OT_COAP_CODE_PUT:
        case OT_COAP_CODE_DELETE:
            return true;
        case OT_COAP_CODE_RESPONSE_MIN:
        case OT_COAP_CODE_CREATED:
        case OT_COAP_CODE_DELETED:
        case OT_COAP_CODE_VALID:
        case OT_COAP_CODE_CHANGED:
        case OT_COAP_CODE_CONTENT:
        case OT_COAP_CODE_BAD_REQUEST:
        case OT_COAP_CODE_UNAUTHORIZED:
        case OT_COAP_CODE_BAD_OPTION:
        case OT_COAP_CODE_FORBIDDEN:
        case OT_COAP_CODE_NOT_FOUND:
        case OT_COAP_CODE_METHOD_NOT_ALLOWED:
        case OT_COAP_CODE_NOT_ACCEPTABLE:
        case OT_COAP_CODE_PRECONDITION_FAILED:
        case OT_COAP_CODE_REQUEST_TOO_LARGE:
        case OT_COAP_CODE_UNSUPPORTED_FORMAT:
        case OT_COAP_CODE_INTERNAL_ERROR:
        case OT_COAP_CODE_NOT_IMPLEMENTED:
        case OT_COAP_CODE_BAD_GATEWAY:
        case OT_COAP_CODE_SERVICE_UNAVAILABLE:
        case OT_COAP_CODE_GATEWAY_TIMEOUT:
        case OT_COAP_CODE_PROXY_NOT_SUPPORTED:
        default:
            return false;
    }
}

static void HandleCoAPRequest(void* _Nullable aContext, otMessage* aMessage, const otMessageInfo* aMessageInfo) {
    HAPPrecondition(aContext);
    HAPPlatformThreadCoAPManagerResource* resource = aContext;
    HAPPlatformThreadCoAPManagerRef coapManager = resource->coapManager;
    HAPPrecondition(aMessage);
    HAPPrecondition(aMessageInfo);

    otError e = OT_ERROR_NONE;

    HAPLogDebug(&logObject, "HandleCoAPRequest(s)");

    const char* uriPath = resource->otResource.mUriPath;
    otCoapType requestType = otCoapMessageGetType(aMessage);
    otCoapCode requestCode = otCoapMessageGetCode(aMessage);
    const char* requestCodeDescription = otCoapMessageCodeToString(aMessage);
    uint16_t numRequestBytes = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    uint16_t msgId = otCoapMessageGetMessageId(aMessage);
    uint8_t tokenLen = otCoapMessageGetTokenLength(aMessage);
    HAPAssert(tokenLen <= 8);
    const uint8_t* tokenptr = otCoapMessageGetToken(aMessage);
    char ipv6[MAX_IPV6_STR_LEN + 1];

    uint64_t token = 0;
    for (int i = 0; i < tokenLen; i++) {
        token <<= 8;
        token += *tokenptr++;
    }
    switch (requestType) {
        case OT_COAP_TYPE_CONFIRMABLE: {

            HAPLogDebug(
                    &logObject,
                    "Received CON CoAP request on '%s' (%u bytes - id:%d token 0x%llx %d.%02d %s).",
                    uriPath,
                    numRequestBytes,
                    msgId,
                    token,
                    GetClass(requestCode),
                    GetDetail(requestCode),
                    requestCodeDescription);
        }
            goto handleRequest;
        case OT_COAP_TYPE_NON_CONFIRMABLE: {
            HAPLogDebug(
                    &logObject,
                    "Received NON CoAP request on '%s' (%u bytes - id:%d token 0x%llx %d.%02d %s).",
                    uriPath,
                    numRequestBytes,
                    msgId,
                    token,
                    GetClass(requestCode),
                    GetDetail(requestCode),
                    requestCodeDescription);
        }
            goto handleRequest;
        handleRequest : {
            otCoapCode responseCode;

            uint8_t bytes[HAPMin(kHAPPlatformThreadCoAPManager_MaxMessageBytes, UINT16_MAX)];
            uint16_t numResponseBytes = 0;
            uint8_t responseBytes[HAPMin(kHAPPlatformThreadCoAPManager_MaxMessageBytes, UINT16_MAX)];

            bool doTryShortResponse = false;
            otCoapCode shortResponseCode = 0;
            uint16_t numShortResponseBytes = 0;
            uint8_t shortResponseBytes[kHAPPlatformThreadCoAPManager_MaxShortResponseBytes];

            if (!IsSupportedRequestCode(requestCode)) {
                HAPLogError(
                        &logObject,
                        "Unexpected request code: %d.%02d %s. Rejecting.",
                        GetClass(requestCode),
                        GetDetail(requestCode),
                        requestCodeDescription);
                responseCode = OT_COAP_CODE_NOT_FOUND;
            } else if (numRequestBytes > sizeof bytes) {
                HAPLogError(&logObject, "Request too long: %u / %zu bytes. Rejecting.", numRequestBytes, sizeof bytes);
                responseCode = OT_COAP_CODE_NOT_FOUND;
            } else {
                uint16_t numBytes = otMessageRead(aMessage, otMessageGetOffset(aMessage), bytes, sizeof bytes);
                HAPAssert(numBytes == numRequestBytes);
                HAPPlatformThreadCoAPManagerPeer peer;
                HAPRawBufferZero(&peer, sizeof peer);
                peer.ipAddress.version = kHAPIPAddressVersion_IPv6;
                HAPAssert(sizeof peer.ipAddress._.ipv6.bytes == sizeof aMessageInfo->mPeerAddr.mFields.m8);
                HAPRawBufferCopyBytes(
                        peer.ipAddress._.ipv6.bytes,
                        aMessageInfo->mPeerAddr.mFields.m8,
                        sizeof peer.ipAddress._.ipv6.bytes);

                HAPError hapErr = HAPIPv6AddressGetDescription(&peer.ipAddress._.ipv6, ipv6, MAX_IPV6_STR_LEN);
                HAPAssert(hapErr == kHAPError_None);
                HAPLogDebug(&logObject, "   Coap peer IP address %s", ipv6);

                peer.port = aMessageInfo->mPeerPort;
                HAPPlatformThreadCoAPManagerRequestCode reqCode = (HAPPlatformThreadCoAPManagerRequestCode) requestCode;

                HAPPlatformThreadCoAPManagerResponseCode rspCode;
                size_t numRspBytes;
                HAPPlatformThreadCoAPManagerShortResponse shortResponse;
                shortResponse.doTryShortResponse = false;
                shortResponse.shortResponseBytes = shortResponseBytes;
                shortResponse.numShortResponseBytes = sizeof shortResponseBytes;

                resource->callback(
                        coapManager,
                        &peer,
                        GetResourceHandle(resource),
                        reqCode,
                        bytes,
                        numRequestBytes,
                        &rspCode,
                        responseBytes,
                        sizeof responseBytes,
                        &numRspBytes,
                        &shortResponse,
                        resource->context);
                responseCode = (otCoapCode) rspCode;
                shortResponseCode = (otCoapCode) shortResponse.shortResponseCode;
                HAPAssert(numRspBytes <= sizeof responseBytes);
                HAPAssert(
                        !shortResponse.doTryShortResponse ||
                        shortResponse.numShortResponseBytes <= sizeof shortResponseBytes);
                numResponseBytes = (uint16_t) numRspBytes;
                numShortResponseBytes = (uint16_t) shortResponse.numShortResponseBytes;
                doTryShortResponse = shortResponse.doTryShortResponse;
            }

            if (requestType == OT_COAP_TYPE_NON_CONFIRMABLE) {
                HAPLog(&logObject, "Request was non-confirmable. Discarding response.");
                return;
            }
            HAPAssert(requestType == OT_COAP_TYPE_CONFIRMABLE);

            otCoapCode responseCodesToTry[2] = { responseCode, shortResponseCode };
            uint16_t numResponseBytesToTry[2] = { numResponseBytes, numShortResponseBytes };
            uint8_t* responseBytesToTry[2] = { responseBytes, shortResponseBytes };
            size_t numResponsesToTry = doTryShortResponse ? 2 : 1;

            for (size_t i = 0; i < numResponsesToTry; i++) {
                otMessage* _Nullable response = otCoapNewMessage(coapManager->otInstance, /* aSettings: */ NULL);

                if (!response) {
                    HAPLogError(&logObject, "otCoapNewMessage[%zu] failed.", i);
                    // Retry with a shorter response would not help
                    break;
                } else {
                    otCoapMessageInitResponse(response, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCodesToTry[i]);
                    if (numResponseBytes) {
                        e = otCoapMessageSetPayloadMarker(response);
                    }
                    if (e) {
                        HAPAssert(e == OT_ERROR_NO_BUFS);
                        HAPLogError(&logObject, "otCoapMessageSetPayloadMarker[%zu] failed: %d.", i, e);
                    } else {
                        e = otMessageAppend(response, responseBytesToTry[i], numResponseBytesToTry[i]);
                        if (e) {
                            HAPAssert(e == OT_ERROR_NO_BUFS);
                            HAPLogError(&logObject, "otMessageAppend[%zu] failed: %d.", i, e);
                        } else {

                            HAPLogDebug(&logObject, "Sending Response[%zu] to %s", i, ipv6);

                            msgId = otCoapMessageGetMessageId(response);
                            tokenLen = otCoapMessageGetTokenLength(response);
                            HAPAssert(tokenLen <= 8);
                            tokenptr = otCoapMessageGetToken(response);

                            token = 0;
                            for (int j = 0; j < tokenLen; j++) {
                                token <<= 8;
                                token += *tokenptr++;
                            }
                            // We are replying to a message. Wakelock for 2 seconds in anticipation
                            // of getting another one.
                            HAPPlatformThreadAddWakeLock(&kCoapManagerWakelock, COAP_WAKELOCK_TIMEOUT_MS);
                            e = otCoapSendResponse(coapManager->otInstance, response, aMessageInfo);
                            if (e) {
                                HAPAssert(e == OT_ERROR_NO_BUFS);
                                HAPLogError(
                                        &logObject,
                                        "otCoapSendResponse[%zu] failed: %d for %u bytes.",
                                        i,
                                        e,
                                        numResponseBytes);
                            } else {
                                HAPLogDebug(
                                        &logObject,
                                        "Sent acknowledgement[%zu] (%u bytes - id:%d token 0x%llx %d.%02d).",
                                        i,
                                        numResponseBytes,
                                        msgId,
                                        token,
                                        GetClass(responseCode),
                                        GetDetail(responseCode));
                            }
                        }
                    }
                    if (e) {
                        HAPLog(&logObject, "Failed to send response[%zu].", i);
                        otMessageFreeSafe(response);
                    } else {
                        // Successfully sent. Do not retry with shorter response.
                        break;
                    }
                }
            }
            HAPLogDebug(&logObject, "HandleCoAPRequest(e)");
            return;
        }

        case OT_COAP_TYPE_ACKNOWLEDGMENT: {
            HAPLogDebug(
                    &logObject,
                    "Received CoAP acknowledgment on '%s' (%u bytes - %d.%02d %s).",
                    uriPath,
                    numRequestBytes,
                    GetClass(requestCode),
                    GetDetail(requestCode),
                    requestCodeDescription);
            return;
        }

        case OT_COAP_TYPE_RESET: {
            HAPLogDebug(
                    &logObject,
                    "Received CoAP reset message on '%s' (%u bytes - %d.%02d %s).",
                    uriPath,
                    numRequestBytes,
                    GetClass(requestCode),
                    GetDetail(requestCode),
                    requestCodeDescription);
            return;
        }
    }
    HAPFatalError();
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerAddResource(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const char* uriPath,
        HAPPlatformThreadCoAPManagerRequestCallback callback,
        void* _Nullable context,
        HAPPlatformThreadCoAPManagerResourceRef* coapResource) {
    HAPPrecondition(coapManager);
    HAPPrecondition(coapManager->coapIsActive);
    HAPPrecondition(uriPath);
    HAPPrecondition(callback);
    HAPPrecondition(coapResource);

    otError e;

    HAPLogInfo(&logObject, "Adding CoAP resource '%s'.", uriPath);
    *coapResource = 0;
    for (size_t i = 0; i < coapManager->numResources; i++) {
        HAPPlatformThreadCoAPManagerResource* resource = &coapManager->resources[i];
        if (resource->coapManager) {
            continue;
        }

        HAPRawBufferZero(resource, sizeof *resource);
        resource->coapManager = coapManager;
        resource->callback = callback;
        resource->context = context;
        resource->otResource.mUriPath = uriPath;
        resource->otResource.mHandler = HandleCoAPRequest;
        resource->otResource.mContext = resource;
        resource->otResource.mNext = NULL;
        e = otCoapAddResource(coapManager->otInstance, &resource->otResource);
        if (e) {
            HAPAssert(e == OT_ERROR_ALREADY);
            HAPLogError(&logObject, "otCoapAddResource failed: %d.", e);
            HAPFatalError();
        }
        *coapResource = i + 1;
        HAPAssert(*coapResource);
        return kHAPError_None;
    }
    HAPLogError(&logObject, "Cannot allocate more CoAP resources (%zu available).", coapManager->numResources);
    return kHAPError_OutOfResources;
}

void HAPPlatformThreadCoAPManagerRemoveResource(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPPlatformThreadCoAPManagerResourceRef coapResource) {
    HAPPrecondition(coapManager);
    HAPPrecondition(coapManager->coapIsActive);
    HAPPrecondition(coapResource);
    HAPPrecondition(coapResource <= coapManager->numResources);
    HAPPlatformThreadCoAPManagerResource* resource = &coapManager->resources[coapResource - 1];
    HAPPrecondition(resource->coapManager);

    HAPLogInfo(&logObject, "Removing CoAP resource '%s'.", resource->otResource.mUriPath);
    otCoapRemoveResource(coapManager->otInstance, &resource->otResource);
    HAPRawBufferZero(resource, sizeof *resource);
}

static void
        CoapResponseHandler(void* aContext, otMessage* aMessage, const otMessageInfo* aMessageInfo, otError aResult) {
    HAPPrecondition(aContext);
    HAPPlatformThreadCoAPRequest* request = aContext;
    HAPPlatformThreadCoAPManagerResponseCode responseCode = kHAPPlatformThreadCoAPManagerResponseCode_GatewayTimeout;
    char ipv6[MAX_IPV6_STR_LEN + 1];

    HAPError hapErr = HAPIPv6AddressGetDescription(&(request->peer->ipAddress._.ipv6), ipv6, MAX_IPV6_STR_LEN);
    HAPAssert(hapErr == kHAPError_None);
    HAPLogDebug(&logObject, "Notification sent to %s - %d complete", ipv6, request->peer->port);

    HAPError receiveError = kHAPError_Unknown;
    if (aMessage) {
        responseCode = (HAPPlatformThreadCoAPManagerResponseCode) otCoapMessageGetCode(aMessage);
        receiveError = kHAPError_None;
    } else {
        HAPLogError(&logObject, "   Notification Timed Out");
    }

    HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler = request->completionHandler;
    void* _Nullable clientContext = request->context;
    HAPPlatformThreadCoAPManagerRef coapManager = request->manager;
    HAPPrecondition(completionHandler);
    const HAPPlatformThreadCoAPManagerPeer* coapPeer = request->peer;

    HAPRawBufferZero(request, sizeof *request);

    completionHandler(coapManager, receiveError, coapPeer, responseCode, &aResult, sizeof(aResult), clientContext);

    if (coapManager->delegate.handleReadyToSendRequest) {
        coapManager->delegate.handleReadyToSendRequest(coapManager, coapManager->delegate.context);
    }
}

static _Nullable HAPPlatformThreadCoAPRequest* GetIdleRequest(HAPPlatformThreadCoAPManagerRef coapManager) {
    HAPPlatformThreadCoAPRequest* req = NULL;
    for (size_t i = 0; i < coapManager->numRequests && !req; i++) {
        if (!coapManager->requests[i].inUse) {
            req = &coapManager->requests[i];
        }
    }
    return req;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerSendRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        const char* uriPath,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        const void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
        void* _Nullable context) {
    HAPPrecondition(coapManager);
    HAPPrecondition(coapManager->coapIsActive);
    HAPPrecondition(peer);
    HAPPrecondition(uriPath);
    HAPPrecondition(requestBytes);
    HAPPrecondition(completionHandler);

    otError e = OT_ERROR_NONE;
    HAPLogDebug(&logObject, "HAPPlatformThreadCoAPManagerSendRequest(s).");

    HAPPlatformThreadCoAPRequest* coapRequest = GetIdleRequest(coapManager);

    if (!coapRequest) {
        HAPLogDebug(&logObject, "Waiting for completion of pending request.");
        return kHAPError_Busy;
    }
    if (numRequestBytes > UINT16_MAX) {
        HAPLog(&logObject,
               "Only requests up to %u bytes are supported (requested %zu bytes).",
               UINT16_MAX,
               numRequestBytes);
        return kHAPError_OutOfResources;
    }

    otMessageInfo messageInfo;
    HAPRawBufferZero(&messageInfo, sizeof messageInfo);
    HAPPrecondition(peer->ipAddress.version == kHAPIPAddressVersion_IPv6);
    HAPAssert(sizeof messageInfo.mPeerAddr.mFields.m8 == sizeof peer->ipAddress._.ipv6.bytes);
    HAPRawBufferCopyBytes(
            messageInfo.mPeerAddr.mFields.m8, peer->ipAddress._.ipv6.bytes, sizeof messageInfo.mPeerAddr.mFields.m8);
    messageInfo.mPeerPort = peer->port;

    otCoapType requestType = OT_COAP_TYPE_CONFIRMABLE;

    otMessage* _Nullable request = otCoapNewMessage(coapManager->otInstance, /* aSettings: */ NULL);
    if (!request) {
        HAPLogError(&logObject, "otCoapNewMessage failed.");
    } else {
        otCoapMessageInit(request, requestType, (otCoapCode) requestCode);
        if (!HAPStringAreEqual(uriPath, "")) {
            e = otCoapMessageAppendUriPathOptions(request, uriPath);
            if (e) {
                HAPAssert(e == OT_ERROR_INVALID_ARGS || e == OT_ERROR_NO_BUFS);
                HAPLogError(&logObject, "otCoapMessageAppendUriPathOptions failed: %d.", e);
            }
        }
        if (!e) {
            e = otCoapMessageSetPayloadMarker(request);
            if (e) {
                HAPAssert(e == OT_ERROR_NO_BUFS);
                HAPLogError(&logObject, "otCoapMessageSetPayloadMarker failed: %d.", e);
            } else {
                HAPAssert(numRequestBytes <= UINT16_MAX);
                e = otMessageAppend(request, requestBytes, (uint16_t) numRequestBytes);
                if (e) {
                    HAPAssert(e == OT_ERROR_NO_BUFS);
                    HAPLogError(&logObject, "otMessageAppend failed: %d.", e);
                } else {

                    coapRequest->completionHandler = completionHandler;
                    coapRequest->context = context;
                    coapRequest->peer = peer;
                    coapRequest->manager = coapManager;
                    coapRequest->inUse = true;

                    e = otCoapSendRequest(
                            coapManager->otInstance, request, &messageInfo, CoapResponseHandler, coapRequest);
                    if (e) {
                        HAPAssert(e == OT_ERROR_NO_BUFS);
                        HAPLogError(&logObject, "otCoapSendRequest failed: %d.", e);
                    } else {
                        // Create a wakelock in order to allow our CoAP engine to receive the ACK from the controller
                        // Note:  It is okay to not terminate the wakelock, it will time out on its own.
                        HAPPlatformThreadAddWakeLock(&kCoapManagerWakelock, COAP_WAKELOCK_TIMEOUT_MS);
                    }

                    char ipv6[MAX_IPV6_STR_LEN + 1];
                    HAPError hapErr = HAPIPv6AddressGetDescription(&(peer->ipAddress._.ipv6), ipv6, MAX_IPV6_STR_LEN);
                    HAPAssert(hapErr == kHAPError_None);
                    HAPLogDebug(
                            &logObject,
                            "Sent COAP CON request (%s - %d, %zu bytes, - ID %d"
                            " %d).",
                            ipv6,
                            peer->port,
                            numRequestBytes,
                            otCoapMessageGetMessageId(request),
                            requestCode);
                }
            }
        }
        if (e) {
            HAPLog(&logObject, "Failed to send request.");
            otMessageFreeSafe(request);
            return kHAPError_OutOfResources;
        }
    }

    HAPLogDebug(&logObject, "HAPPlatformThreadCoAPManagerSendRequest(e).");
    return kHAPError_None;
}
#endif
