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

#ifndef HAP_PLATFORM_THREAD_COAP_MANAGER_H
#define HAP_PLATFORM_THREAD_COAP_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * CoAP manager.
 */
typedef struct HAPPlatformThreadCoAPManager HAPPlatformThreadCoAPManager;
typedef struct HAPPlatformThreadCoAPManager* HAPPlatformThreadCoAPManagerRef;
HAP_NONNULL_SUPPORT(HAPPlatformThreadCoAPManager)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates a CoAP code value.
 *
 * @param      codeClass            Class.
 * @param      codeDetail           Detail.
 *
 * @return CoAP code corresponding to the given parameters.
 *
 * @see https://tools.ietf.org/html/rfc7252
 */
#define HAPPlatformThreadCoAPManagerCreateCode(codeClass, codeDetail) \
    ((uint8_t)((uint8_t)((codeClass) << 5u) | (codeDetail)))

/**
 * RFC7252 CoAP request code.
 *
 * @see https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#codes
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformThreadCoAPManagerRequestCode) {
    /** Empty message code. */
    kHAPPlatformThreadCoAPManagerRequestCode_Empty = HAPPlatformThreadCoAPManagerCreateCode(0u, 0u),

    /** GET. */
    kHAPPlatformThreadCoAPManagerRequestCode_GET = HAPPlatformThreadCoAPManagerCreateCode(0u, 1u),

    /** POST. */
    kHAPPlatformThreadCoAPManagerRequestCode_POST = HAPPlatformThreadCoAPManagerCreateCode(0u, 2u),

    /** PUT. */
    kHAPPlatformThreadCoAPManagerRequestCode_PUT = HAPPlatformThreadCoAPManagerCreateCode(0u, 3u),

    /** DELETE. */
    kHAPPlatformThreadCoAPManagerRequestCode_DELETE = HAPPlatformThreadCoAPManagerCreateCode(0u, 4u)
} HAP_ENUM_END(uint8_t, HAPPlatformThreadCoAPManagerRequestCode);

/**
 * RFC7252 CoAP response code.
 *
 * @see https://www.iana.org/assignments/core-parameters/core-parameters.xhtml#codes
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformThreadCoAPManagerResponseCode) {
    /** Created. */
    kHAPPlatformThreadCoAPManagerResponseCode_Created = HAPPlatformThreadCoAPManagerCreateCode(2u, 1u),

    /** Deleted. */
    kHAPPlatformThreadCoAPManagerResponseCode_Deleted = HAPPlatformThreadCoAPManagerCreateCode(2u, 2u),

    /** Valid. */
    kHAPPlatformThreadCoAPManagerResponseCode_Valid = HAPPlatformThreadCoAPManagerCreateCode(2u, 3u),

    /** Changed. */
    kHAPPlatformThreadCoAPManagerResponseCode_Changed = HAPPlatformThreadCoAPManagerCreateCode(2u, 4u),

    /** Content. */
    kHAPPlatformThreadCoAPManagerResponseCode_Content = HAPPlatformThreadCoAPManagerCreateCode(2u, 5u),

    //------------------------------------------------------------------------------------------------------------------

    /** Bad Request. */
    kHAPPlatformThreadCoAPManagerResponseCode_BadRequest = HAPPlatformThreadCoAPManagerCreateCode(4u, 0u),

    /** Unauthorized. */
    kHAPPlatformThreadCoAPManagerResponseCode_Unauthorized = HAPPlatformThreadCoAPManagerCreateCode(4u, 1u),

    /** Bad Option. */
    kHAPPlatformThreadCoAPManagerResponseCode_BadOption = HAPPlatformThreadCoAPManagerCreateCode(4u, 2u),

    /** Forbidden. */
    kHAPPlatformThreadCoAPManagerResponseCode_Forbidden = HAPPlatformThreadCoAPManagerCreateCode(4u, 3u),

    /** Not Found. */
    kHAPPlatformThreadCoAPManagerResponseCode_NotFound = HAPPlatformThreadCoAPManagerCreateCode(4u, 4u),

    /** Method Not Allowed. */
    kHAPPlatformThreadCoAPManagerResponseCode_MethodNotAllowed = HAPPlatformThreadCoAPManagerCreateCode(4u, 5u),

    /** Not Acceptable. */
    kHAPPlatformThreadCoAPManagerResponseCode_NotAcceptable = HAPPlatformThreadCoAPManagerCreateCode(4u, 6u),

    /** Precondition Failed. */
    kHAPPlatformThreadCoAPManagerResponseCode_PreconditionFailed = HAPPlatformThreadCoAPManagerCreateCode(4u, 12u),

    /** Request Entity Too Large. */
    kHAPPlatformThreadCoAPManagerResponseCode_RequestEntityTooLarge = HAPPlatformThreadCoAPManagerCreateCode(4u, 13u),

    /** Unsupported Content-Format. */
    kHAPPlatformThreadCoAPManagerResponseCode_UnsupportedContentFormat =
            HAPPlatformThreadCoAPManagerCreateCode(4u, 15u),

    //------------------------------------------------------------------------------------------------------------------

    /** Internal Server Error. */
    kHAPPlatformThreadCoAPManagerResponseCode_InternalServerError = HAPPlatformThreadCoAPManagerCreateCode(5u, 0u),

    /** Not Implemented. */
    kHAPPlatformThreadCoAPManagerResponseCode_NotImplemented = HAPPlatformThreadCoAPManagerCreateCode(5u, 1u),

    /** Bad Gateway. */
    kHAPPlatformThreadCoAPManagerResponseCode_BadGateway = HAPPlatformThreadCoAPManagerCreateCode(5u, 2u),

    /** Service Unavailable. */
    kHAPPlatformThreadCoAPManagerResponseCode_ServiceUnavailable = HAPPlatformThreadCoAPManagerCreateCode(5u, 3u),

    /** Gateway Timeout. */
    kHAPPlatformThreadCoAPManagerResponseCode_GatewayTimeout = HAPPlatformThreadCoAPManagerCreateCode(5u, 4u),

    /** Proxying Not Supported. */
    kHAPPlatformThreadCoAPManagerResponseCode_ProxyingNotSupported = HAPPlatformThreadCoAPManagerCreateCode(5u, 5u)
} HAP_ENUM_END(uint8_t, HAPPlatformThreadCoAPManagerResponseCode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Peer information.
 */
typedef struct {
    HAPIPAddress ipAddress; /**< Peer IP address. */
    HAPNetworkPort port;    /**< Peer port number. */
} HAPPlatformThreadCoAPManagerPeer;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Delegate to handle CoAP manager events.
 */
typedef struct {
    /**
     * Client context pointer. Will be passed to callbacks.
     */
    void* _Nullable context;

    /**
     * Invoked when the CoAP manager is again ready to send a request.
     *
     * @param      coapManager          CoAP manager.
     * @param      context              The context pointer of the CoAP manager delegate structure.
     */
    void (*_Nullable handleReadyToSendRequest)(HAPPlatformThreadCoAPManagerRef coapManager, void* _Nullable context);
} HAPPlatformThreadCoAPManagerDelegate;
HAP_NONNULL_SUPPORT(HAPPlatformThreadCoAPManagerDelegate)

/**
 * Specifies or clears the delegate for receiving CoAP manager events.
 *
 * - The delegate structure is copied and does not need to remain valid.
 *
 * @param      coapManager          CoAP manager.
 * @param      delegate             The delegate to receive the CoAP manager events. NULL to clear.
 */
void HAPPlatformThreadCoAPManagerSetDelegate(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerDelegate* _Nullable delegate);

/**
 * Starts the CoAP manager, allowing to add CoAP resources and to send CoAP requests.
 *
 * @param      coapManager          CoAP manager.
 */
void HAPPlatformThreadCoAPManagerStart(HAPPlatformThreadCoAPManagerRef coapManager);

/**
 * Stops the CoAP manager, removing all added CoAP resources and cancelling pending requests.
 *
 * @param      coapManager          CoAP manager.
 */
void HAPPlatformThreadCoAPManagerStop(HAPPlatformThreadCoAPManagerRef coapManager);

/**
 * Returns the network port associated with a CoAP manager where CoAP resources are served.
 *
 * @param      coapManager          CoAP manager.
 *
 * @return Port number associated with the CoAP manager.
 */
HAP_RESULT_USE_CHECK
HAPNetworkPort HAPPlatformThreadCoAPManagerGetServerPort(HAPPlatformThreadCoAPManagerRef coapManager);

/**
 * Fetches the primary IP address associated with a CoAP manager.
 *
 * @param      coapManager          CoAP manager.
 * @param[out] primaryIPAddress     Primary IP address.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        Otherwise.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerGetPrimaryIPAddress(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPIPAddress* primaryIPAddress);

/**
 * CoAP resource handle.
 */
typedef uintptr_t HAPPlatformThreadCoAPManagerResourceRef;

/** Data structure for short response argument */
typedef struct {
    /**
     * A flag to indicate if a short response must be tried,
     * in case the response cannot be sent due to insufficient resources.
     * Caller, i.e., CoAP manager will set this to false by default.
     * Callback function must set this field value to true before returning,
     * if the callback function built a short response to try.
     */
    bool doTryShortResponse;

    /**
     * CoAP response code in case the response cannot be sent out due to insufficient resources.
     * Callback function must assign this field value before returning.
     */
    HAPPlatformThreadCoAPManagerResponseCode shortResponseCode;

    /**
     * Buffer to fill a short response into in case the response cannot be sent out due to
     * insufficient resources. May overlap with request buffer.
     * Callback function must fill the buffer before returning.
     */
    void* shortResponseBytes;

    /**
     * Capacity of the short response buffer.
     * Caller assigns this field value.
     */
    size_t maxShortResponseBytes;

    /**
     * Length of data that was filled into short response buffer.
     * Callback function must assign this field value before returning.
     */
    size_t numShortResponseBytes;
} HAPPlatformThreadCoAPManagerShortResponse;

/**
 * CoAP request handler.
 *
 * @param      coapManager           CoAP manager.
 * @param      coapResource          CoAP resource handle.
 * @param      peer                  Peer information.
 * @param      requestCode           CoAP request code.
 * @param      requestBytes          Buffer that contains the request data.
 * @param      numRequestBytes       Length of data in request buffer.
 * @param[out] responseCode          CoAP response code.
 * @param[out] responseBytes         Buffer to fill response into. May overlap with request buffer.
 * @param      maxResponseBytes      Capacity of response buffer.
 * @param[out] numResponseBytes      Length of data that was filled into response buffer.
 * @param      shortResponse         pointer to arguments related to short response which will be tried in case
 *                                   the response cannot be sent due to insufficient resources.
 *                                   The pointer is set to NULL by the caller if the platform does not support
 *                                   fallback short response.
 * @param      context               The context pointer given to the HAPPlatformThreadCoAPManagerAddResource function.
 */
typedef void (*HAPPlatformThreadCoAPManagerRequestCallback)(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        HAPPlatformThreadCoAPManagerResourceRef coapResource,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerResponseCode* responseCode,
        void* responseBytes,
        size_t maxResponseBytes,
        size_t* numResponseBytes,
        HAPPlatformThreadCoAPManagerShortResponse* _Nullable shortResponse,
        void* _Nullable context);

/**
 * Adds a CoAP resource to be served.
 *
 * - This function may only be called after the CoAP manager is started.
 *
 * @param      coapManager          CoAP manager.
 * @param      uriPath              URI path string. Must remain valid while CoAP resource is added.
 * @param      callback             Callback that is invoked when a CoAP request for the URI path is received.
 * @param      context              Client context pointer. Will be passed to the callback.
 * @param[out] coapResource         Non-zero CoAP resource handle, if successful.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If no more CoAP resources can be allocated.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerAddResource(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const char* uriPath,
        HAPPlatformThreadCoAPManagerRequestCallback callback,
        void* _Nullable context,
        HAPPlatformThreadCoAPManagerResourceRef* coapResource);

/**
 * Removes a previously added CoAP resource.
 *
 * - The callback passed to HAPPlatformThreadCoAPManagerAddResource will no longer be invoked.
 *
 * - This function may only be called after the CoAP manager is started.
 *
 * @param      coapManager          CoAP manager.
 * @param      coapResource         CoAP resource handle.
 */
void HAPPlatformThreadCoAPManagerRemoveResource(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPPlatformThreadCoAPManagerResourceRef coapResource);

/**
 * Completion handler of a send request operation.
 *
 * @param      coapManager          CoAP manager.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the request was aborted or timed out.
 * @param      peer                 Peer information. Only defined if successful.
 * @param      responseCode         CoAP response code. Only defined if successful.
 * @param      responseBytes        Response buffer. Only defined if successful.
 * @param      numResponseBytes     Length of data in response buffer. Only defined if successful.
 * @param      context              The context pointer given to the HAPPlatformThreadCoAPManagerSendRequest function.
 */
typedef void (*HAPPlatformThreadCoAPManagerSendRequestCompletionHandler)(
        HAPPlatformThreadCoAPManagerRef coapManager,
        HAPError error,
        const HAPPlatformThreadCoAPManagerPeer* _Nullable peer,
        HAPPlatformThreadCoAPManagerResponseCode responseCode,
        void* _Nullable responseBytes,
        size_t numResponseBytes,
        void* _Nullable context);

/**
 * Sends a CoAP request to a peer.
 *
 * - If the request cannot be sent at this time, kHAPError_InvalidState is returned.
 *   Once another request may be sent the delegate's handleReadyToSendRequest method shall be called.
 *
 * - This function may only be called after the CoAP manager is started.
 *
 * @param      coapManager          CoAP manager.
 * @param      peer                 Peer information.
 * @param      uriPath              URI path string. Must remain valid until completion handler is called.
 * @param      requestCode          CoAP request code.
 * @param      requestBytes         Request buffer.
 * @param      numRequestBytes      Length of data in request buffer.
 * @param      completionHandler    Completion handler to call when the operation is complete.
 * @param      context              Client context pointer. Will be passed to the completion handler.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Busy           If the request could not be sent at this time.
 * @return kHAPError_OutOfResources If the buffer is too large to send.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformThreadCoAPManagerSendRequest(
        HAPPlatformThreadCoAPManagerRef coapManager,
        const HAPPlatformThreadCoAPManagerPeer* peer,
        const char* uriPath,
        HAPPlatformThreadCoAPManagerRequestCode requestCode,
        const void* requestBytes,
        size_t numRequestBytes,
        HAPPlatformThreadCoAPManagerSendRequestCompletionHandler completionHandler,
        void* _Nullable context);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
