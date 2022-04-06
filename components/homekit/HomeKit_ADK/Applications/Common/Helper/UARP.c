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

#include "UARP.h"

#include <string.h>

#include "HAP.h"

#include "ApplicationFeatures.h"

#if (HAVE_UARP_SUPPORT == 1)

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

#define kUARPDataResponseTimeout ((HAPTime)(1 * HAPMinute))

//----------------------------------------------------------------------------------------------------------------------

static void UARPReturnBuffer(void* accessoryDelegate, uint8_t* buffer);
static void UARPAssetMetadataComplete(void* accessoryDelegate, void* assetDelegate);
static void UARPPayloadMetadataComplete(void* accessoryDelegate, void* assetDelegate);

void UARPHandleAccept(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context);
void UARPHandleData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context,
        void* dataBytes,
        size_t numDataBytes);
void UARPHandleInvalidate(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request,
        HAPDataStreamHandle dataStream,
        void* _Nullable context);

//----------------------------------------------------------------------------------------------------------------------

static const HAPLogObject uarpLogObject = { .subsystem = "com.apple.mfi.HomeKit.Core", .category = "UARP" };

// UARP accessory object and buffers.
UARPAccessory uarpAccessory;
static uint8_t uarpRxBuffer[kUARPRxBufferSize][kUARPNumRxBuffers];
static uint8_t uarpTxBuffers[kUARPTxBufferSize][kUARPNumTxBuffers];
static uint8_t uarpTxScratchBuffers[kUARPTxScratchBufferSize][kUARPNumTxBuffers];
static UARPInternalStorage uarpInternalBuffers;

// HDS UARP stream application protocol instantiation.
static HAPStreamDataStreamApplicationProtocolContext uarpProtocolContexts[kApp_NumDataStreams];
static HAPStreamDataStreamApplicationProtocolSendContext uarpProtocolSendContexts[kApp_NumDataStreams];
HAPStreamApplicationProtocol streamProtocolUARP = {
    .name = kHAPStreamAppProtocolName_UARP,
    .context = (void*) &uarpAccessory,
    .storage = { .streamAppProtocolContexts = uarpProtocolContexts,
                 .streamAppProtocolSendContexts = uarpProtocolSendContexts,
                 .numSendContexts = HAPArrayCount(uarpProtocolSendContexts),
                 .rxBuffers = uarpRxBuffer,
                 .rxBufferSize = kUARPRxBufferSize,
                 .numRxBuffers = kUARPNumRxBuffers },
    .config = { .maxSendContextsPerDataStream = kUARPMaxTxBuffersPerController, .requiresAdminPermissions = true },
    .callbacks = { .handleAccept = UARPHandleAccept,
                   .handleData = UARPHandleData,
                   .handleInvalidate = UARPHandleInvalidate }
};

//----------------------------------------------------------------------------------------------------------------------

static bool UARPIsFirmwareAssetActive(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return (accessory->activeFirmwareAsset.uarpPlatformAsset != NULL) ? true : false;
}

static bool UARPIsFirmwareAssetLinkedToController(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return (accessory->activeFirmwareAsset.controller != NULL) ? true : false;
}

static bool UARPIsFirmwareAssetPausedByController(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return accessory->activeFirmwareAsset.isPausedByController;
}

static bool UARPIsFirmwareAssetStalled(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return accessory->activeFirmwareAsset.dataResponseTimedOut;
}

static bool UARPIsFirmwareAssetPendingRelease(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return accessory->activeFirmwareAsset.isPendingRelease;
}

static bool UARPIsFirmwareAssetPendingAccept(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    return (accessory->pendingFirmwareAsset.uarpPlatformAsset != NULL) ? true : false;
}

HAP_RESULT_USE_CHECK
static HAPError
        UARPControllerRequestScratchBuffer(UARPAccessory* accessory, UARPController* controller, void** scratchBuffer) {
    HAPPrecondition(accessory);
    HAPPrecondition(controller);
    HAPPrecondition(scratchBuffer);

    HAPLogDebug(&uarpLogObject, "[%s] UARP controller (%p) requesting scratch buffer", __func__, (void*) controller);

    *scratchBuffer = NULL;

    // Find a free controller buffer slot.
    int bufferSlot;
    for (bufferSlot = 0; bufferSlot < kUARPMaxTxBuffersPerController; bufferSlot++) {
        if (controller->streamTxScratchBuffer[bufferSlot] == NULL) {
            break;
        }
    }
    if (controller->streamTxScratchBuffer[bufferSlot] != NULL) {
        HAPLogError(
                &uarpLogObject,
                "[%s] Controller (%p) requested scratch buffer with max pending sends (%d).",
                __func__,
                (void*) controller,
                kUARPMaxTxBuffersPerController);
        return kHAPError_OutOfResources;
    }

    size_t bufferIdx = 0;
    if (UARPIsFirmwareAssetActive(accessory) && UARPIsFirmwareAssetLinkedToController(accessory) &&
        (accessory->activeFirmwareAsset.controller != controller)) {
        // Reserve the first kUARPMaxTxBuffersPerController buffers for the active staging session.
        bufferIdx = kUARPMaxTxBuffersPerController;
    }
    for (uint8_t* streamTxScratchBuffer =
                 &accessory->streamTxScratchBuffers[bufferIdx * accessory->config.txScratchBufferSize];
         bufferIdx < accessory->config.numTxBuffers;
         bufferIdx++, streamTxScratchBuffer += accessory->config.txScratchBufferSize) {
        if (!(accessory->streamTxScratchBufferIsAllocated & (1 << bufferIdx))) {
            *scratchBuffer = streamTxScratchBuffer;
            accessory->streamTxScratchBufferIsAllocated |= (1 << bufferIdx);
            break;
        }
    }

    if (*scratchBuffer == NULL) {
        HAPLogError(&uarpLogObject, "[%s] Unable to satisfy scratch buffer request", __func__);
        return kHAPError_OutOfResources;
    }

    controller->streamTxScratchBuffer[bufferSlot] = *scratchBuffer;
    HAPLogDebug(&uarpLogObject, "[%s] Provided scratch buffer (%p)", __func__, (void*) *scratchBuffer);

    return kHAPError_None;
}

static void
        UARPControllerReturnScratchBuffer(UARPAccessory* accessory, UARPController* controller, void* scratchBuffer) {
    HAPPrecondition(accessory);
    HAPPrecondition(controller);
    HAPPrecondition(scratchBuffer);

    HAPLogDebug(
            &uarpLogObject,
            "[%s] Controller (%p) returning scratch buffer (%p).",
            __func__,
            (void*) controller,
            (void*) scratchBuffer);

    // Find the buffer slot.
    int bufferSlot;
    for (bufferSlot = 0; bufferSlot < kUARPMaxTxBuffersPerController; bufferSlot++) {
        if (controller->streamTxScratchBuffer[bufferSlot] == scratchBuffer) {
            break;
        }
    }
    if (controller->streamTxScratchBuffer[bufferSlot] != scratchBuffer) {
        HAPLogError(&uarpLogObject, "[%s] Unrecognized scratch buffer (%p)", __func__, scratchBuffer);
        HAPFatalError();
    }

    int bufferIdx = 0;
    uint32_t allocatedBuffers = accessory->streamTxScratchBufferIsAllocated;
    uint8_t* streamTxScratchBuffer = accessory->streamTxScratchBuffers;
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && (streamTxScratchBuffer == scratchBuffer)) {
            accessory->streamTxScratchBufferIsAllocated &= ~(1 << bufferIdx);
            controller->streamTxScratchBuffer[bufferSlot] = NULL;
            return;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        streamTxScratchBuffer += accessory->config.txScratchBufferSize;
    }
    HAPLogError(&uarpLogObject, "[%s] Unable to return scratch buffer (%p)", __func__, scratchBuffer);
    HAPFatalError();
}

static void UARPDataSendCompletionHandler(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPDataStreamDispatcher* dispatcher HAP_UNUSED,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPDataStreamRequest* request HAP_UNUSED,
        HAPDataStreamHandle dataStream,
        HAPError error HAP_UNUSED,
        void* scratchBytes,
        size_t numScratchBytes HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(HAPStringAreEqual(streamAppProtocol->name, kHAPStreamAppProtocolName_UARP));
    HAPPrecondition(dataStream < kUARPNumMaxControllers);

    UARPAccessory* accessory = (UARPAccessory*) streamAppProtocol->context;
    HAPAssert(accessory);
    UARPController* controller = &accessory->controllers[dataStream];

    UARPControllerReturnScratchBuffer(accessory, controller, scratchBytes);
}

static void UARPVersionFromAcessoryFirmwareVersionString(UARPVersion* version, const char* firmwareString) {
    HAPPrecondition(version);
    HAPPrecondition(firmwareString);

    unsigned long major = 0;
    unsigned long minor = 0;
    unsigned long revision = 0;

    HAPRawBufferZero(version, sizeof(*version));

    int fields = sscanf(firmwareString, "%lu.%lu.%lu", &major, &minor, &revision);
    if ((fields < 1) || (major > UINT32_MAX) || (minor > UINT32_MAX) || (revision > UINT32_MAX)) {
        HAPLogError(&uarpLogObject, "[%s] Invalid firmware string %s", __func__, firmwareString);
        return;
    }

    version->major = (uint32_t) major;
    version->minor = (uint32_t) minor;
    version->release = (uint32_t) revision;
}

static void UARPRemoveController(UARPAccessory* accessory, UARPController* controller) {
    HAPPrecondition(accessory);
    HAPPrecondition(controller);

    // Remove controller from accessory object.
    controller->isAllocated = false;

    // Notify libUARP of the controller removal.
    HAPLogInfo(&uarpLogObject, "%s: Removing UARP remote controller (%p).", __func__, (void*) controller);
    uint32_t status =
            uarpPlatformControllerRemove(&accessory->uarpPlatformAccessory, controller->uarpPlatformController);
    HAPAssert(status == kUARPStatusSuccess);

    // Return the platform controller object buffer.
    UARPReturnBuffer(accessory, (uint8_t*) controller->uarpPlatformController);
}

static void UARPCancelDataResponseTimer(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    if (accessory->activeFirmwareAsset.timeoutTimer) {
        HAPPlatformTimerDeregister(accessory->activeFirmwareAsset.timeoutTimer);
        accessory->activeFirmwareAsset.timeoutTimer = 0;
    }
}

static void UARPHandleDataResponseTimeout(HAPPlatformTimerRef timer, void* _Nullable context) {
    HAPPrecondition(context);
    UARPAccessory* accessory = (UARPAccessory*) context;

    HAPAssert(timer == accessory->activeFirmwareAsset.timeoutTimer);
    accessory->activeFirmwareAsset.timeoutTimer = 0;

    UARPController* controller = accessory->activeFirmwareAsset.controller;

    if (controller) {
        HAPLogError(&uarpLogObject, "[%s] Controller (%p) timed out.", __func__, (void*) controller);
        accessory->activeFirmwareAsset.dataResponseTimedOut = true;
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingPaused);
    } else {
        HAPFatalError();
    }
}

static void UARPClearStagedFirmwareVersion(UARPAccessory* accessory) {
    HAPPrecondition(accessory);

    HAPRawBufferZero(&(accessory->stagedFirmwareVersion), sizeof(accessory->stagedFirmwareVersion));
}

static uint32_t
        UARPAcceptFirmwareAsset(UARPAccessory* accessory, UARPController* controller, struct uarpPlatformAsset* asset) {
    HAPPrecondition(accessory);
    HAPPrecondition(controller);
    HAPPrecondition(asset);

    uint32_t status;

    UARPClearStagedFirmwareVersion(accessory);

    HAPLogDebug(&uarpLogObject, "[%s] Accepting asset <0x%08X>", __func__, (unsigned int) asset->core.assetTag);
    // Set up active asset and accept the offer from the controller.
    accessory->activeFirmwareAsset.controller = controller;
    accessory->activeFirmwareAsset.uarpPlatformAsset = asset;
    accessory->activeFirmwareAsset.dataResponseTimedOut = false;
    accessory->activeFirmwareAsset.isPausedByController = false;
    accessory->activeFirmwareAsset.isPendingRelease = false;
    status = uarpPlatformAccessoryAssetAccept2(
            &(accessory->uarpPlatformAccessory),
            controller->uarpPlatformController,
            asset,
            (void*) &accessory->activeFirmwareAsset,
            NULL);

    return status;
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t UARPRequestBuffer(void* accessoryDelegate, uint8_t* _Nonnull* _Nonnull buffer, uint32_t bufferLength) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    HAPLogDebug(
            &uarpLogObject,
            "[%s] UARP accessory (%p) requesting buffer of %zu bytes",
            __func__,
            (void*) &(accessory->uarpPlatformAccessory),
            (size_t) bufferLength);

    *buffer = NULL;

    if (bufferLength == sizeof(struct uarpPlatformController)) {
        size_t bufferIdx;
        struct uarpPlatformController* uarpPlatformControllerBuffer;
        for (bufferIdx = 0, uarpPlatformControllerBuffer = &(accessory->uarpBuffers->uarpPlatformController[0]);
             bufferIdx < kUARPNumMaxControllers;
             bufferIdx++, uarpPlatformControllerBuffer++) {
            if (!(accessory->uarpPlatformControllerBufferIsAllocated & (1 << bufferIdx))) {
                *buffer = (uint8_t*) uarpPlatformControllerBuffer;
                accessory->uarpPlatformControllerBufferIsAllocated |= (1 << bufferIdx);
                break;
            }
        }
    } else if (bufferLength == sizeof(struct uarpPlatformAsset)) {
        int bufferIdx;
        struct uarpPlatformAsset* uarpPlatformAssetBuffer;
        for (bufferIdx = 0, uarpPlatformAssetBuffer = &(accessory->uarpBuffers->uarpPlatformAsset[0]);
             bufferIdx < kUARPNumAssetBuffers;
             bufferIdx++, uarpPlatformAssetBuffer++) {
            if (!(accessory->uarpPlatformAssetBufferIsAllocated & (1 << bufferIdx))) {
                *buffer = (uint8_t*) uarpPlatformAssetBuffer;
                accessory->uarpPlatformAssetBufferIsAllocated |= (1 << bufferIdx);
                break;
            }
        }
    } else if (bufferLength == kUARPDataChunkSize) {
        int bufferIdx;
        uint8_t* uarpDataWindowBuffer;
        for (bufferIdx = 0, uarpDataWindowBuffer = &(accessory->uarpBuffers->uarpDataWindow[0][0]);
             bufferIdx < kUARPNumDataWindowBuffers;
             bufferIdx++, uarpDataWindowBuffer += kUARPDataChunkSize) {
            if (!(accessory->uarpDataWindowBufferIsAllocated & (1 << bufferIdx))) {
                *buffer = uarpDataWindowBuffer;
                accessory->uarpDataWindowBufferIsAllocated |= (1 << bufferIdx);
                break;
            }
        }
    }

    if (*buffer == NULL) {
        HAPLogError(&uarpLogObject, "[%s] Unable to satisfy buffer request", __func__);
        HAPAssert(false);
        return kUARPStatusNoResources;
    }

    HAPRawBufferZero(*buffer, bufferLength);
    HAPLogDebug(&uarpLogObject, "[%s] Provided buffer (%p)", __func__, (void*) *buffer);
    HAPLogDebug(
            &uarpLogObject,
            "[%s] Buffer allocations (0x%08lX) (0x%08lX) (0x%08lX)",
            __func__,
            (long unsigned int) accessory->uarpPlatformControllerBufferIsAllocated,
            (long unsigned int) accessory->uarpPlatformAssetBufferIsAllocated,
            (long unsigned int) accessory->uarpDataWindowBufferIsAllocated);
    return kUARPStatusSuccess;
}

static void UARPReturnBuffer(void* accessoryDelegate, uint8_t* buffer) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    HAPLogDebug(
            &uarpLogObject,
            "[%s] UARP accessory (%p) returning buffer (%p)",
            __func__,
            (void*) &(accessory->uarpPlatformAccessory),
            (void*) buffer);

    int bufferIdx = 0;
    uint32_t allocatedBuffers = accessory->uarpDataWindowBufferIsAllocated;
    uint8_t* uarpDataWindowBuffer = &(accessory->uarpBuffers->uarpDataWindow[0][0]);
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && (uarpDataWindowBuffer == buffer)) {
            accessory->uarpDataWindowBufferIsAllocated &= ~(1 << bufferIdx);
            goto exit;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        uarpDataWindowBuffer += kUARPDataChunkSize;
    }

    bufferIdx = 0;
    allocatedBuffers = accessory->uarpPlatformAssetBufferIsAllocated;
    struct uarpPlatformAsset* uarpPlatformAssetBuffer = &(accessory->uarpBuffers->uarpPlatformAsset[0]);
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && ((uint8_t*) uarpPlatformAssetBuffer == buffer)) {
            accessory->uarpPlatformAssetBufferIsAllocated &= ~(1 << bufferIdx);
            goto exit;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        uarpPlatformAssetBuffer++;
    }

    bufferIdx = 0;
    allocatedBuffers = accessory->uarpPlatformControllerBufferIsAllocated;
    struct uarpPlatformController* uarpPlatformControllerBuffer = &(accessory->uarpBuffers->uarpPlatformController[0]);
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && ((uint8_t*) uarpPlatformControllerBuffer == buffer)) {
            accessory->uarpPlatformControllerBufferIsAllocated &= ~(1 << bufferIdx);
            goto exit;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        uarpPlatformControllerBuffer++;
    }

    HAPLogError(&uarpLogObject, "[%s] Unrecognized buffer (%p)", __func__, (void*) buffer);
    HAPFatalError();

exit:
    HAPLogDebug(
            &uarpLogObject,
            "[%s] Buffer allocations (0x%08lX) (0x%08lX) (0x%08lX)",
            __func__,
            (long unsigned int) accessory->uarpPlatformControllerBufferIsAllocated,
            (long unsigned int) accessory->uarpPlatformAssetBufferIsAllocated,
            (long unsigned int) accessory->uarpDataWindowBufferIsAllocated);
    return;
}

static uint32_t UARPRequestTransmitMsgBuffer(
        void* accessoryDelegate,
        void* controllerDelegate,
        uint8_t* _Nonnull* _Nonnull buffer,
        uint32_t* length) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(buffer);
    HAPPrecondition(length);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    HAPLogDebug(
            &uarpLogObject,
            "[%s] UARP accessory (%p) controller (%p) requesting transmit buffer",
            __func__,
            (void*) &(accessory->uarpPlatformAccessory),
            (void*) controller);

    *buffer = NULL;

    // Find a free controller buffer slot.
    int bufferSlot;
    for (bufferSlot = 0; bufferSlot < kUARPMaxTxBuffersPerController; bufferSlot++) {
        if (controller->uarpTxBuffer[bufferSlot] == NULL) {
            break;
        }
    }
    if (controller->uarpTxBuffer[bufferSlot] != NULL) {
        HAPLogError(
                &uarpLogObject,
                "[%s] Controller (%p) requested transmit buffer with max pending sends (%d).",
                __func__,
                controllerDelegate,
                kUARPMaxTxBuffersPerController);
        return kUARPStatusNoResources;
    }

    size_t bufferIdx = 0;
    if (UARPIsFirmwareAssetActive(accessory) && UARPIsFirmwareAssetLinkedToController(accessory) &&
        (accessory->activeFirmwareAsset.controller != controller)) {
        // Reserve the first kUARPMaxTxBuffersPerController buffers for the active staging session.
        bufferIdx = kUARPMaxTxBuffersPerController;
    }
    for (uint8_t* uarpTxBuffer = &accessory->uarpTxBuffers[bufferIdx * accessory->config.txBufferSize];
         bufferIdx < accessory->config.numTxBuffers;
         bufferIdx++, uarpTxBuffer += accessory->config.txBufferSize) {
        if (!(accessory->uarpTxBufferIsAllocated & (1 << bufferIdx))) {
            *buffer = uarpTxBuffer;
            accessory->uarpTxBufferIsAllocated |= (1 << bufferIdx);
            break;
        }
    }

    if (*buffer == NULL) {
        HAPLogError(&uarpLogObject, "[%s] Unable to satisfy transmit buffer request", __func__);
        return kUARPStatusNoResources;
    }

    *length = accessory->config.txBufferSize;
    controller->uarpTxBuffer[bufferSlot] = *buffer;

    HAPRawBufferZero(*buffer, *length);
    HAPLogDebug(&uarpLogObject, "[%s] Provided transmit buffer (%p)", __func__, (void*) *buffer);

    return kUARPStatusSuccess;
}

static void UARPReturnTransmitMsgBuffer(void* accessoryDelegate, void* controllerDelegate, uint8_t* buffer) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(buffer);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    HAPLogDebug(
            &uarpLogObject,
            "[%s] UARP accessory (%p) controller (%p) returning transmit buffer (%p)",
            __func__,
            (void*) &(accessory->uarpPlatformAccessory),
            (void*) controller,
            (void*) buffer);

    // Find the buffer slot.
    int bufferSlot;
    for (bufferSlot = 0; bufferSlot < kUARPMaxTxBuffersPerController; bufferSlot++) {
        if (controller->uarpTxBuffer[bufferSlot] == buffer) {
            break;
        }
    }
    if (controller->uarpTxBuffer[bufferSlot] != buffer) {
        HAPLogError(&uarpLogObject, "[%s] Unrecognized transmit buffer (%p)", __func__, (void*) buffer);
        HAPFatalError();
    }

    int bufferIdx = 0;
    uint32_t allocatedBuffers = accessory->uarpTxBufferIsAllocated;
    uint8_t* uarpTxBuffer = accessory->uarpTxBuffers;
    while (allocatedBuffers) {
        if ((allocatedBuffers & 1) && (uarpTxBuffer == buffer)) {
            accessory->uarpTxBufferIsAllocated &= ~(1 << bufferIdx);
            controller->uarpTxBuffer[bufferSlot] = NULL;
            return;
        }
        allocatedBuffers >>= 1;
        bufferIdx++;
        uarpTxBuffer += accessory->config.txBufferSize;
    }
    HAPLogError(&uarpLogObject, "[%s] Unable to return transmit buffer (%p)", __func__, (void*) buffer);
    HAPFatalError();
}

static uint32_t
        UARPSendMessage(void* accessoryDelegate, void* controllerDelegate, uint8_t* buffer, uint32_t bufferLength) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(buffer);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    if (!controller->isAllocated) {
        HAPLogError(
                &uarpLogObject,
                "[%s] Dropping message (%zu bytes) for invalid controller %p.",
                __func__,
                (size_t) bufferLength,
                controllerDelegate);
        return kUARPStatusUnknownController;
    }

    HAPLogInfo(
            &uarpLogObject,
            "[%s] Sending message (%zu bytes) to controller %p.",
            __func__,
            (size_t) bufferLength,
            controllerDelegate);

    void* scratchBuffer;
    HAPError err = UARPControllerRequestScratchBuffer(accessory, controller, &scratchBuffer);
    if (err) {
        return kUARPStatusNoResources;
    }

    struct UARPMsgHeader* msg = (struct UARPMsgHeader*) buffer;
    if ((controller == accessory->activeFirmwareAsset.controller) &&
        (uarpNtohs(msg->msgType) == kUARPMsgAssetDataRequest)) {
        HAPAssert(!accessory->activeFirmwareAsset.timeoutTimer);

        err = HAPPlatformTimerRegister(
                &accessory->activeFirmwareAsset.timeoutTimer,
                HAPPlatformClockGetCurrent() + kUARPDataResponseTimeout,
                UARPHandleDataResponseTimeout,
                accessory);
        if (err) {
            HAPAssert(err == kHAPError_OutOfResources);
            HAPLog(&kHAPLog_Default, "Not enough resources to start data response timeout timer");
        }
    }
    err = HAPStreamDataStreamProtocolSendData(
            controller->dataStreamContext.server,
            controller->dataStreamContext.dispatcher,
            controller->dataStreamContext.streamAppProtocol,
            controller->dataStreamContext.dataStream,
            scratchBuffer,
            accessory->config.txScratchBufferSize,
            buffer,
            bufferLength,
            UARPDataSendCompletionHandler);

    // The contents of the tx buffer has been transferred to the scratch buffer for HDS transmit
    // This callback indicates the buffer is available, but the message itself may not have been sent yet
    uarpPlatformAccessorySendMessageComplete(
            &(accessory->uarpPlatformAccessory), controller->uarpPlatformController, buffer);

    return (err == kHAPError_None) ? kUARPStatusSuccess : kUARPStatusNoResources;
}

static uint32_t UARPDataTransferPause(void* accessoryDelegate HAP_UNUSED, void* controllerDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    if (controller && (accessory->activeFirmwareAsset.controller == controller)) {
        accessory->activeFirmwareAsset.isPausedByController = true;
        UARPCancelDataResponseTimer(accessory);
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingPaused);
    }
    return kUARPStatusSuccess;
}

static uint32_t UARPDataTransferResume(void* accessoryDelegate HAP_UNUSED, void* controllerDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    if (controller && (accessory->activeFirmwareAsset.controller == controller)) {
        accessory->activeFirmwareAsset.isPausedByController = false;
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingResumed);
    }
    return kUARPStatusSuccess;
}

static void UARPEvaluateAssetOfferWithOrphanedAsset(
        UARPAccessory* accessory,
        struct uarpPlatformAsset** asset,
        bool* shouldOfferAccessory,
        bool* shouldAccept) {
    HAPPrecondition(accessory);
    HAPPrecondition(asset);
    HAPPrecondition(*asset);
    HAPPrecondition(shouldOfferAccessory);
    HAPPrecondition(shouldAccept);

    HAPLogDebug(&uarpLogObject, "[%s]", __func__);

    // Compare the offered asset to our orphaned asset.
    // Offered Asset == Orphaned Asset: Merge
    // Offered Version < Orphaned Version: Deny
    // Offered Version > Orphaned Version: Offer
    UARPVersionComparisonResult compareResult =
            uarpAssetCoreCompare(&(accessory->activeFirmwareAsset.uarpPlatformAsset->core), &((*asset)->core));
    if (compareResult == kUARPVersionComparisonResultIsEqual) {
        // Offered asset is a match. Merge and continue staging.
        HAPLogDebug(&uarpLogObject, "Merging offered and orphaned assets");
        uarpPlatformAccessorySuperBinaryMerge(
                &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset, *asset);
        // The offered asset has been merged into the orphaned asset.
        // We proceed with the previously-orphaned asset object, so re-direct asset pointer accordingly.
        *asset = accessory->activeFirmwareAsset.uarpPlatformAsset;
        *shouldAccept = true;
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingResumed);
    } else {
        // Compare the version of our orphaned asset to that of the offered asset.
        UARPVersionComparisonResult compareResult = uarpVersionCompare(
                &(accessory->activeFirmwareAsset.uarpPlatformAsset->core.assetVersion), &((*asset)->core.assetVersion));
        if (compareResult == kUARPVersionComparisonResultIsNewer) {
            // Offer asset to accessory if it is a newer version than that currently being staged.
            HAPLogDebug(&uarpLogObject, "Offered asset is newer than orphaned asset");
            *shouldOfferAccessory = true;
        }
    }
    HAPAssert(*asset);
}

static void UARPEvaluateCompetingAssetOffer(
        UARPAccessory* accessory,
        struct uarpPlatformAsset** asset,
        bool* shouldOfferAccessory,
        bool* shouldAccept) {
    HAPPrecondition(accessory);
    HAPPrecondition(asset);
    HAPPrecondition(*asset);
    HAPPrecondition(shouldOfferAccessory);
    HAPPrecondition(shouldAccept);

    bool compareAssetVersions = true;

    HAPLogDebug(&uarpLogObject, "[%s]", __func__);

    // Compare the offered asset to our current asset.
    // Offered Asset == Paused Asset: Abandon/Merge
    // Offered Version < Paused Version: Deny
    // Offered Version > Paused Version: Offer
    // Offered Version <= In-Progress Version: Deny
    // Offered Version > In-Progress Version: Offer
    if (UARPIsFirmwareAssetPausedByController(accessory) || UARPIsFirmwareAssetStalled(accessory)) {
        // If our current asset is paused by the corresponding controller or our data requests stalled
        // (controller likely disconnected silently), see if this offer is the same asset.
        UARPVersionComparisonResult compareResult =
                uarpAssetCoreCompare(&(accessory->activeFirmwareAsset.uarpPlatformAsset->core), &((*asset)->core));
        if (compareResult == kUARPVersionComparisonResultIsEqual) {
            // Offered asset is a match.
            // Abandon transfer with paused/stalled controller.
            compareAssetVersions = false;
            HAPLogDebug(&uarpLogObject, "Abandoning paused/stalled asset and merging with offered asset");
            uarpPlatformAccessoryAssetAbandon(
                    &(accessory->uarpPlatformAccessory),
                    accessory->activeFirmwareAsset.controller->uarpPlatformController,
                    accessory->activeFirmwareAsset.uarpPlatformAsset);
            // Merge with our asset from the new controller and pick up where we left off.
            uarpPlatformAccessorySuperBinaryMerge(
                    &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset, *asset);
            // The offered asset has been merged into the abandoned asset.
            // We proceed with the previously-abandoned asset object, so re-direct asset pointer accordingly.
            *asset = accessory->activeFirmwareAsset.uarpPlatformAsset;
            *shouldAccept = true;
            accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                    accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingResumed);
        }
    }

    if (compareAssetVersions) {
        // Compare the version of our current asset to that of the offered asset.
        UARPVersionComparisonResult compareResult = uarpVersionCompare(
                &(accessory->activeFirmwareAsset.uarpPlatformAsset->core.assetVersion), &((*asset)->core.assetVersion));
        if (compareResult == kUARPVersionComparisonResultIsNewer) {
            // Offer asset to accessory if it is a newer version than that currently being staged.
            HAPLogDebug(&uarpLogObject, "Offered asset is newer than current asset");
            *shouldOfferAccessory = true;
        }
    }
    HAPAssert(*asset);
}

static void UARPSuperBinaryOffered(
        void* accessoryDelegate HAP_UNUSED,
        void* controllerDelegate HAP_UNUSED,
        struct uarpPlatformAsset* asset HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(asset);

    uint32_t status;
    uint8_t versionIsAcceptable;
    uint16_t denyReason = 0;
    bool shouldOfferAccessory = false;
    bool shouldAccept = false;

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    HAPLogInfo(
            &uarpLogObject,
            "[%s] UARP controller (%p) offered SuperBinary <Tag: 0x%08X> <Version: %u.%u.%u.%u> <Length: %u> "
            "<Payloads: %u>",
            __func__,
            (void*) controller,
            (unsigned int) asset->core.assetTag,
            (int) asset->core.assetVersion.major,
            (int) asset->core.assetVersion.minor,
            (int) asset->core.assetVersion.release,
            (int) asset->core.assetVersion.build,
            (int) asset->core.assetTotalLength,
            (int) asset->core.assetNumPayloads);

    // Initial check to confirm the offered version is newer than the running version and neither the offered version
    // nor a newer version is already staged.
    status = uarpPlatformAccessoryAssetIsAcceptable2(
            &(accessory->uarpPlatformAccessory), asset, &versionIsAcceptable, &denyReason);
    HAPAssert(status == kUARPStatusSuccess);

    if (versionIsAcceptable == kUARPNo) {
        HAPLogDebug(&uarpLogObject, "[%s] Asset is not acceptable (%u)", __func__, denyReason);
    } else if (UARPIsFirmwareAssetActive(accessory) && !UARPIsFirmwareAssetLinkedToController(accessory)) {
        // Handle orphaned asset.
        UARPEvaluateAssetOfferWithOrphanedAsset(accessory, &asset, &shouldOfferAccessory, &shouldAccept);
    } else if (UARPIsFirmwareAssetActive(accessory) && UARPIsFirmwareAssetLinkedToController(accessory)) {
        // Handle competing asset.
        UARPEvaluateCompetingAssetOffer(accessory, &asset, &shouldOfferAccessory, &shouldAccept);
    } else {
        // No active asset. Always offer if the version check indicates it is acceptable.
        shouldOfferAccessory = true;
    }

    if (shouldOfferAccessory == true) {
        HAPLogDebug(&uarpLogObject, "[%s] Offering asset to accessory", __func__);
        accessory->accessoryFirmwareAssetCallbacks->assetOffered(
                accessory->hapAccessoryServer,
                accessory->hapAccessory,
                &(asset->core.assetVersion),
                asset->core.assetTag,
                asset->core.assetTotalLength,
                asset->core.assetNumPayloads,
                &shouldAccept);

        // If the accessory is accepting the offer, there should not be an active asset or the active asset
        // should have been abandoned as a precursor to accepting the competing asset offer.
        if (shouldAccept == true) {
            HAPAssert(!UARPIsFirmwareAssetActive(accessory) || UARPIsFirmwareAssetPendingRelease(accessory));
            if (UARPIsFirmwareAssetActive(accessory) && !UARPIsFirmwareAssetPendingRelease(accessory)) {
                // Not much to do other than force a deny.
                shouldAccept = false;
            }
        }
    }

    if (shouldAccept == true) {
        if (UARPIsFirmwareAssetPendingRelease(accessory)) {
            // Asset and associated resources (data window buffer) must be released before we can accept the offer.
            HAPLogDebug(
                    &uarpLogObject,
                    "[%s] Pending offer for acceptance (%p) (%p)",
                    __func__,
                    (void*) controller,
                    (void*) asset);
            accessory->pendingFirmwareAsset.controller = controller;
            accessory->pendingFirmwareAsset.uarpPlatformAsset = asset;
        } else {
            status = UARPAcceptFirmwareAsset(accessory, controller, asset);
            HAPAssert(status == kUARPStatusSuccess);
        }
    } else {
        // Deny the offer from the controller.
        HAPLogDebug(&uarpLogObject, "[%s] Denying asset <0x%08X>", __func__, (unsigned int) asset->core.assetTag);
        status = uarpPlatformAccessoryAssetDeny3(
                &(accessory->uarpPlatformAccessory), controller->uarpPlatformController, asset, denyReason, NULL, NULL);
        HAPAssert(status == kUARPStatusSuccess);
    }
}

static void UARPDynamicAssetOffered(
        void* accessoryDelegate HAP_UNUSED,
        void* controllerDelegate HAP_UNUSED,
        struct uarpPlatformAsset* asset HAP_UNUSED) {
}

static void UARPAssetRescinded(
        void* accessoryDelegate HAP_UNUSED,
        void* controllerDelegate HAP_UNUSED,
        void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    if (asset == &(accessory->activeFirmwareAsset)) {
        HAPLogInfo(&uarpLogObject, "[%s] Asset rescinded by controller %p.", __func__, (void*) controller);
        UARPCancelDataResponseTimer(accessory);
        UARPClearStagedFirmwareVersion(accessory);
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_AssetRescinded);
    }
}

static void UARPRescindAllAssets(
        void* accessoryDelegate HAP_UNUSED,
        void* controllerDelegate HAP_UNUSED,
        void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPController* controller = (UARPController*) controllerDelegate;

    HAPLogInfo(&uarpLogObject, "[%s] Rescind all assets request from controller %p.", __func__, (void*) controller);

    UARPCancelDataResponseTimer(accessory);
    UARPClearStagedFirmwareVersion(accessory);
    accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
            accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_AssetRescinded);
}

static void UARPAssetCorrupt(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    if (asset == &(accessory->activeFirmwareAsset)) {
        HAPLogInfo(&uarpLogObject, "[%s] Asset detected as corrupt.", __func__);
        UARPCancelDataResponseTimer(accessory);
        // If the UARP stack detects a corrupt asset, we are still required to abandon the asset to mark it for
        // cleanup.
        uarpPlatformAccessoryAssetAbandon(&(accessory->uarpPlatformAccessory), NULL, asset->uarpPlatformAsset);
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_AssetCorrupt);
    }
}

static void UARPAssetOrphaned(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    if ((asset == &(accessory->activeFirmwareAsset)) && UARPIsFirmwareAssetLinkedToController(accessory)) {
        HAPLogInfo(&uarpLogObject, "[%s] Asset %p orphaned.", __func__, (void*) asset);
        UARPCancelDataResponseTimer(accessory);
        asset->controller = NULL;
        accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingPaused);
    }
}

static void UARPAssetReleased(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    if (asset == &(accessory->activeFirmwareAsset)) {
        HAPLogInfo(&uarpLogObject, "[%s] Asset %p released.", __func__, (void*) asset);
        UARPCancelDataResponseTimer(accessory);
        HAPRawBufferZero(&(accessory->activeFirmwareAsset), sizeof(accessory->activeFirmwareAsset));
    }
}

static void UARPAssetReady(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    uint32_t status =
            uarpPlatformAccessoryAssetRequestMetaData(&(accessory->uarpPlatformAccessory), asset->uarpPlatformAsset);

    if (status == kUARPStatusNoMetaData) {
        UARPAssetMetadataComplete(accessory, assetDelegate);
        status = kUARPStatusSuccess;
    }

    HAPAssert(status == kUARPStatusSuccess);
}

static void UARPAssetMetadataTLV(
        void* accessoryDelegate HAP_UNUSED,
        void* assetDelegate HAP_UNUSED,
        uint32_t tlvType HAP_UNUSED,
        uint32_t tlvLength HAP_UNUSED,
        uint8_t* _Nullable tlvValue HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    accessory->accessoryFirmwareAssetCallbacks->assetMetadataTLV(
            accessory->hapAccessoryServer, accessory->hapAccessory, tlvType, tlvLength, tlvValue);
}

static void UARPAssetMetadataComplete(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    accessory->accessoryFirmwareAssetCallbacks->assetMetadataComplete(
            accessory->hapAccessoryServer, accessory->hapAccessory);
}

static void UARPPayloadReady(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    // Save off the payload index for which this callback was invoked.
    int payloadIndex = asset->uarpPlatformAsset->selectedPayloadIndex;

    accessory->accessoryFirmwareAssetCallbacks->payloadReady(
            accessory->hapAccessoryServer,
            accessory->hapAccessory,
            &(asset->uarpPlatformAsset->payload.plHdr.payloadVersion),
            asset->uarpPlatformAsset->payload.plHdr.payloadTag,
            asset->uarpPlatformAsset->payload.plHdr.payloadLength);

    if (!UARPIsFirmwareAssetActive(accessory)) {
        // Accessory abandoned asset.
        return;
    }

    if (payloadIndex != asset->uarpPlatformAsset->selectedPayloadIndex) {
        // Accessory called UARPRequestPayloadByIndex() in callback to request a different payload.
        // We should not proceed. This callback will be invoked again when the new payload header is received.
        return;
    }

    uint32_t status =
            uarpPlatformAccessoryPayloadRequestMetaData(&(accessory->uarpPlatformAccessory), asset->uarpPlatformAsset);

    if (status == kUARPStatusNoMetaData) {
        UARPPayloadMetadataComplete(accessory, assetDelegate);
        status = kUARPStatusSuccess;
    }

    HAPAssert(status == kUARPStatusSuccess);
}

static void UARPPayloadMetadataTLV(
        void* accessoryDelegate HAP_UNUSED,
        void* assetDelegate HAP_UNUSED,
        uint32_t tlvType HAP_UNUSED,
        uint32_t tlvLength HAP_UNUSED,
        uint8_t* _Nullable tlvValue HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    accessory->accessoryFirmwareAssetCallbacks->payloadMetadataTLV(
            accessory->hapAccessoryServer, accessory->hapAccessory, tlvType, tlvLength, tlvValue);
}

static void UARPPayloadMetadataComplete(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPFirmwareAsset* asset = (UARPFirmwareAsset*) assetDelegate;

    // Save off the payload index for which this callback was invoked.
    int payloadIndex = asset->uarpPlatformAsset->selectedPayloadIndex;

    accessory->accessoryFirmwareAssetCallbacks->payloadMetadataComplete(
            accessory->hapAccessoryServer, accessory->hapAccessory);

    if (!UARPIsFirmwareAssetActive(accessory)) {
        // Accessory abandoned asset.
        return;
    }

    if (payloadIndex != asset->uarpPlatformAsset->selectedPayloadIndex) {
        // Accessory called UARPRequestPayloadByIndex() in callback to request a different payload.
        // We should not proceed. The payloadReady callback will be invoked again when the new payload header is
        // received.
        return;
    }

    uint32_t status =
            uarpPlatformAccessoryPayloadRequestData(&(accessory->uarpPlatformAccessory), asset->uarpPlatformAsset);
    HAPAssert(status == kUARPStatusSuccess);
}

static void UARPPayloadData(
        void* accessoryDelegate HAP_UNUSED,
        void* assetDelegate HAP_UNUSED,
        uint8_t* buffer HAP_UNUSED,
        uint32_t bufferLength HAP_UNUSED,
        uint32_t offset HAP_UNUSED,
        uint8_t* assetState HAP_UNUSED,
        uint32_t assetStateLength HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);
    HAPPrecondition(buffer);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    accessory->accessoryFirmwareAssetCallbacks->payloadData(
            accessory->hapAccessoryServer,
            accessory->hapAccessory,
            accessory->activeFirmwareAsset.uarpPlatformAsset->payload.plHdr.payloadTag,
            offset,
            buffer,
            bufferLength);
}

static void UARPPayloadDataComplete(void* accessoryDelegate HAP_UNUSED, void* assetDelegate HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(assetDelegate);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    accessory->accessoryFirmwareAssetCallbacks->payloadDataComplete(
            accessory->hapAccessoryServer, accessory->hapAccessory);
}

static uint32_t UARPApplyStagedAssets(
        void* accessoryDelegate HAP_UNUSED,
        void* controllerDelegate HAP_UNUSED,
        uint16_t* flags HAP_UNUSED) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(flags);

    bool requestRefused;
    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    HAPLogInfo(&uarpLogObject, "[%s] Apply staged assets for controller %p.", __func__, controllerDelegate);

    if (UARPIsVersionZero(&(accessory->stagedFirmwareVersion))) {
        // No staged version.
        if (UARPIsFirmwareAssetActive(accessory) && UARPIsFirmwareAssetLinkedToController(accessory)) {
            *flags = kUARPApplyStagedAssetsFlagsMidUpload;
        } else {
            *flags = kUARPApplyStagedAssetsFlagsNothingStaged;
        }
    } else {
        // Notify the accessory of the apply request.
        accessory->accessoryFirmwareAssetCallbacks->applyStagedAssets(
                accessory->hapAccessoryServer, accessory->hapAccessory, &requestRefused);
        if (requestRefused == true) {
            *flags = kUARPApplyStagedAssetsFlagsInUse;
        } else {
            // Success flag indicates the apply request was accepted by the accessory.
            // HomeKit uses the accessory's firmware version to determine actual apply success/failure.
            *flags = kUARPApplyStagedAssetsFlagsSuccess;
            UARPClearStagedFirmwareVersion(accessory);
        }
    }

    HAPLogInfo(&uarpLogObject, "[%s] Processed apply request. Flags = %u.", __func__, (unsigned int) *flags);
    return kUARPStatusSuccess;
}

static uint32_t UARPQueryManufacturerName(void* accessoryDelegate, uint8_t* buffer, uint32_t* length) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);
    HAPPrecondition(length);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    size_t stringLen = HAPStringGetNumBytes(accessory->hapAccessory->manufacturer);
    if (stringLen > *length) {
        return kUARPStatusNoResources;
    }

    HAPRawBufferCopyBytes(buffer, accessory->hapAccessory->manufacturer, stringLen);
    *length = (uint32_t) stringLen;

    return kUARPStatusSuccess;
}

static uint32_t UARPQueryModelName(void* accessoryDelegate, uint8_t* buffer, uint32_t* length) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);
    HAPPrecondition(length);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    size_t stringLen = HAPStringGetNumBytes(accessory->hapAccessory->model);
    if (stringLen > *length) {
        return kUARPStatusNoResources;
    }

    HAPRawBufferCopyBytes(buffer, accessory->hapAccessory->model, stringLen);
    *length = (uint32_t) stringLen;

    return kUARPStatusSuccess;
}

static uint32_t UARPQuerySerialNumber(void* accessoryDelegate, uint8_t* buffer, uint32_t* length) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);
    HAPPrecondition(length);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    size_t stringLen = HAPStringGetNumBytes(accessory->hapAccessory->serialNumber);
    if (stringLen > *length) {
        return kUARPStatusNoResources;
    }

    HAPRawBufferCopyBytes(buffer, accessory->hapAccessory->serialNumber, stringLen);
    *length = (uint32_t) stringLen;

    return kUARPStatusSuccess;
}

static uint32_t UARPQueryHardwareVersion(void* accessoryDelegate, uint8_t* buffer, uint32_t* length) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(buffer);
    HAPPrecondition(length);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;

    size_t stringLen = HAPStringGetNumBytes(accessory->hapAccessory->hardwareVersion);
    if (stringLen > *length) {
        return kUARPStatusNoResources;
    }

    HAPRawBufferCopyBytes(buffer, accessory->hapAccessory->hardwareVersion, stringLen);
    *length = (uint32_t) stringLen;

    return kUARPStatusSuccess;
}

static uint32_t
        UARPQueryActiveFirmwareVersion(void* accessoryDelegate, uint32_t assetTag HAP_UNUSED, UARPVersion* version) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(version);

    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    UARPVersionFromAcessoryFirmwareVersionString(version, accessory->hapAccessory->firmwareVersion);

    return kUARPStatusSuccess;
}

static uint32_t UARPQueryStagedFirmwareVersion(
        void* accessoryDelegate,
        uint32_t assetTag HAP_UNUSED,
        struct UARPVersion* version) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(version);

    HAPRawBufferZero(version, sizeof(*version));
    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    HAPRawBufferCopyBytes(version, &accessory->stagedFirmwareVersion, sizeof(*version));

    return kUARPStatusSuccess;
}

static uint32_t UARPQueryLastError(void* accessoryDelegate, struct UARPLastErrorAction* lastErrorAction) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(lastErrorAction);

    HAPRawBufferZero(lastErrorAction, sizeof(*lastErrorAction));
    UARPAccessory* accessory = (UARPAccessory*) accessoryDelegate;
    accessory->accessoryFirmwareAssetCallbacks->retrieveLastError(
            accessory->hapAccessoryServer, accessory->hapAccessory, lastErrorAction);

    return kUARPStatusSuccess;
}

static uint32_t UARPAssetSolicited(void* accessoryDelegate, void* controllerDelegate, struct UARP4ccTag* assetTag) {
    HAPPrecondition(accessoryDelegate);
    HAPPrecondition(controllerDelegate);
    HAPPrecondition(assetTag);

    uint32_t status = kUARPStatusUnsupported;
    return status;
}

static const struct uarpPlatformAccessoryCallbacks UARPCallbacks = {
    .fRequestBuffer = UARPRequestBuffer,
    .fReturnBuffer = UARPReturnBuffer,
    .fRequestTransmitMsgBuffer = UARPRequestTransmitMsgBuffer,
    .fReturnTransmitMsgBuffer = UARPReturnTransmitMsgBuffer,
    .fSendMessage = UARPSendMessage,
    .fDataTransferPause = UARPDataTransferPause,
    .fDataTransferResume = UARPDataTransferResume,
    .fSuperBinaryOffered = UARPSuperBinaryOffered,
    .fDynamicAssetOffered = UARPDynamicAssetOffered,
    .fAssetRescinded = UARPAssetRescinded,
    .fAssetCorrupt = UARPAssetCorrupt,
    .fAssetOrphaned = UARPAssetOrphaned,
    .fAssetReleased = UARPAssetReleased,
    .fAssetReady = UARPAssetReady,
    .fAssetMetaDataTLV = UARPAssetMetadataTLV,
    .fAssetMetaDataComplete = UARPAssetMetadataComplete,
    .fPayloadReady = UARPPayloadReady,
    .fPayloadMetaDataTLV = UARPPayloadMetadataTLV,
    .fPayloadMetaDataComplete = UARPPayloadMetadataComplete,
    .fPayloadData = UARPPayloadData,
    .fPayloadDataComplete = UARPPayloadDataComplete,
    .fApplyStagedAssets = UARPApplyStagedAssets,
    .fManufacturerName = UARPQueryManufacturerName,
    .fModelName = UARPQueryModelName,
    .fSerialNumber = UARPQuerySerialNumber,
    .fHardwareVersion = UARPQueryHardwareVersion,
    .fActiveFirmwareVersion = UARPQueryActiveFirmwareVersion,
    .fStagedFirmwareVersion = UARPQueryStagedFirmwareVersion,
    .fLastError = UARPQueryLastError,
    .fAssetSolicitation = UARPAssetSolicited,
    .fRescindAllAssets = UARPRescindAllAssets,
};

//----------------------------------------------------------------------------------------------------------------------

void UARPHandleData(
        HAPAccessoryServer* server,
        HAPDataStreamDispatcher* dispatcher,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request HAP_UNUSED,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED,
        void* dataBytes,
        size_t numDataBytes) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(HAPStringAreEqual(streamAppProtocol->name, kHAPStreamAppProtocolName_UARP));
    HAPPrecondition(dataStream < kUARPNumMaxControllers);
    HAPPrecondition(dataBytes);

    uint32_t status;
    UARPAccessory* accessory = (UARPAccessory*) streamAppProtocol->context;
    HAPAssert(accessory);
    UARPController* controller = &accessory->controllers[dataStream];

    // Check whether a UARP controller has been registered for this data stream.
    if (!controller->isAllocated) {
        // Get a buffer for the UARP controller object.
        HAPLogInfo(&uarpLogObject, "%s: Adding UARP remote controller (%p).", __func__, (void*) controller);
        status = UARPRequestBuffer(
                accessory, (uint8_t**) &(controller->uarpPlatformController), sizeof(struct uarpPlatformController));
        if (status != kUARPStatusSuccess) {
            HAPLogError(&uarpLogObject, "[%s] No free controller buffers.", __func__);
            return;
        }

        // Store state for the data stream.
        controller->dataStreamContext.server = server;
        controller->dataStreamContext.dispatcher = dispatcher;
        controller->dataStreamContext.streamAppProtocol = streamAppProtocol;
        controller->dataStreamContext.dataStream = dataStream;
        controller->isAllocated = true;

        // Notify libUARP of the new controller.
        status = uarpPlatformControllerAdd(
                &accessory->uarpPlatformAccessory, controller->uarpPlatformController, controller);
        if (status != kUARPStatusSuccess) {
            HAPLogError(&uarpLogObject, "[%s] Failed to register controller.", __func__);
            return;
        }
    }

    HAPLogInfo(
            &uarpLogObject,
            "[%s] Received message (%zu bytes) from controller %p.",
            __func__,
            numDataBytes,
            (void*) controller);

    // If the controller is staging the active asset, handle data response tracking.
    struct UARPMsgHeader* msg = (struct UARPMsgHeader*) dataBytes;
    if ((controller == accessory->activeFirmwareAsset.controller) &&
        (uarpNtohs(msg->msgType) == kUARPMsgAssetDataResponse)) {
        if (accessory->activeFirmwareAsset.timeoutTimer) {
            // De-register pending response timer.
            HAPPlatformTimerDeregister(accessory->activeFirmwareAsset.timeoutTimer);
            accessory->activeFirmwareAsset.timeoutTimer = 0;
        } else if (accessory->activeFirmwareAsset.dataResponseTimedOut) {
            // Received the data response. Resume staging.
            accessory->activeFirmwareAsset.dataResponseTimedOut = false;
            accessory->accessoryFirmwareAssetCallbacks->assetStateChange(
                    accessory->hapAccessoryServer, accessory->hapAccessory, kUARPAssetStateChange_StagingResumed);
        }
    }

    // Pass the message to libUARP for processing.
    accessory->isProcessingRxMsg = true;
    status = uarpPlatformAccessoryRecvMessage(
            &(accessory->uarpPlatformAccessory), controller->uarpPlatformController, dataBytes, numDataBytes);
    if (status != kUARPStatusSuccess) {
        HAPLog(&uarpLogObject, "[%s] UARP receive returned %u.", __func__, (int) status);
    }
    accessory->isProcessingRxMsg = false;

    // Cleanup should be a NOP here as libUARP runs it upon rx message processing completion.
    // But to future proof our dependencies, let's ensure cleanup is complete.
    uarpPlatformCleanupAssets(&(accessory->uarpPlatformAccessory));

    // If a firmware asset is pending, run the accept flow.
    if (UARPIsFirmwareAssetPendingAccept(accessory)) {
        HAPAssert(!UARPIsFirmwareAssetActive(accessory));
        if (!UARPIsFirmwareAssetActive(accessory)) {
            HAPLogDebug(
                    &uarpLogObject,
                    "[%s] Accepting pending offer (%p) (%p)",
                    __func__,
                    (void*) accessory->pendingFirmwareAsset.controller,
                    (void*) accessory->pendingFirmwareAsset.uarpPlatformAsset);
            status = UARPAcceptFirmwareAsset(
                    accessory,
                    accessory->pendingFirmwareAsset.controller,
                    accessory->pendingFirmwareAsset.uarpPlatformAsset);
            HAPAssert(status == kUARPStatusSuccess);
            if (status == kUARPStatusSuccess) {
                HAPRawBufferZero(&(accessory->pendingFirmwareAsset), sizeof(accessory->pendingFirmwareAsset));
            }
        }
    }
}

void UARPHandleAccept(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPDataStreamDispatcher* dispatcher HAP_UNUSED,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request HAP_UNUSED,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(HAPStringAreEqual(streamAppProtocol->name, kHAPStreamAppProtocolName_UARP));
    HAPPrecondition(dataStream < kUARPNumMaxControllers);

    HAPLogDebug(&uarpLogObject, "%s: UARP transport opened (%d).", __func__, dataStream);
}

void UARPHandleInvalidate(
        HAPAccessoryServer* server HAP_UNUSED,
        HAPDataStreamDispatcher* dispatcher HAP_UNUSED,
        HAPStreamApplicationProtocol* streamAppProtocol,
        const HAPServiceRequest* request HAP_UNUSED,
        HAPDataStreamHandle dataStream,
        void* _Nullable context HAP_UNUSED) {
    HAPPrecondition(streamAppProtocol);
    HAPPrecondition(HAPStringAreEqual(streamAppProtocol->name, kHAPStreamAppProtocolName_UARP));
    HAPPrecondition(dataStream < kUARPNumMaxControllers);

    HAPLogDebug(&uarpLogObject, "%s: UARP transport closed (%d).", __func__, dataStream);

    UARPAccessory* accessory = (UARPAccessory*) streamAppProtocol->context;
    HAPAssert(accessory);
    UARPController* controller = &accessory->controllers[dataStream];
    if (!controller->isAllocated) {
        HAPLogDebug(
                &uarpLogObject,
                "[%s] Invalidate received for un-registered controller (%p).",
                __func__,
                (void*) controller);
        return;
    }

    UARPRemoveController(accessory, controller);
}

//----------------------------------------------------------------------------------------------------------------------

void UARPInitialize(HAPAccessoryServer* server, const HAPAccessory* accessory) {
    HAPPrecondition(server);
    HAPPrecondition(accessory);

    void* vendorExtension = NULL;
    fcnUarpVendorSpecific vendorSpecificCallback = NULL;
    HAPRawBufferZero(&uarpAccessory, sizeof(uarpAccessory));

    // Initialize accessory object.
    uarpAccessory.hapAccessory = accessory;
    uarpAccessory.hapAccessoryServer = server;
    uarpAccessory.uarpOptions.maxRxPayloadLength = kUARPDataChunkSize;
    uarpAccessory.uarpOptions.maxTxPayloadLength = kUARPTxBufferSize - sizeof(union UARPMessages);
    uarpAccessory.uarpOptions.payloadWindowLength = kUARPDataChunkSize;
    uarpAccessory.config.numTxBuffers = kUARPNumTxBuffers;
    uarpAccessory.config.txBufferSize = kUARPTxBufferSize;
    uarpAccessory.config.txScratchBufferSize = kUARPTxScratchBufferSize;
    uarpAccessory.uarpTxBuffers = &uarpTxBuffers[0][0];
    uarpAccessory.streamTxScratchBuffers = &uarpTxScratchBuffers[0][0];

    HAPLogDebug(
            &uarpLogObject,
            "[%s] Tx buffers (%p) (%p).",
            __func__,
            (void*) uarpAccessory.uarpTxBuffers,
            (void*) uarpAccessory.streamTxScratchBuffers);

    uarpAccessory.uarpCallbacks = &UARPCallbacks;
    uarpAccessory.uarpBuffers = &uarpInternalBuffers;

    HAPLogDebug(
            &uarpLogObject,
            "[%s] Internal buffers (%p) (%p) (%p).",
            __func__,
            (void*) &(uarpAccessory.uarpBuffers->uarpPlatformController[0]),
            (void*) &(uarpAccessory.uarpBuffers->uarpPlatformAsset[0]),
            (void*) uarpAccessory.uarpBuffers->uarpDataWindow);

    HAPLogDebug(
            &uarpLogObject,
            "[%s] Expected buffer request sizes: %zu, %zu, %zu.",
            __func__,
            sizeof(struct uarpPlatformController),
            sizeof(struct uarpPlatformAsset),
            (size_t) kUARPDataChunkSize);

    HAPLogInfo(&uarpLogObject, "%s: Initializing libUARP.", __func__);
    uint32_t status = uarpPlatformAccessoryInit(
            &(uarpAccessory.uarpPlatformAccessory),
            &(uarpAccessory.uarpOptions),
            (struct uarpPlatformAccessoryCallbacks*) uarpAccessory.uarpCallbacks,
            vendorExtension,
            vendorSpecificCallback,
            &uarpAccessory);
    if (status != kUARPStatusSuccess) {
        HAPLogError(&uarpLogObject, "%s: UARP Initialization failed", __func__);
        HAPFatalError();
    }

    uarpAccessory.isInitialized = true;
}

HAP_RESULT_USE_CHECK
HAPError UARPRequestFirmwareAssetPayloadByIndex(uint32_t payloadIndex) {
    UARPAccessory* accessory = &uarpAccessory;

    if (!UARPIsFirmwareAssetActive(accessory)) {
        return kHAPError_InvalidState;
    } else if (payloadIndex >= accessory->activeFirmwareAsset.uarpPlatformAsset->core.assetNumPayloads) {
        return kHAPError_InvalidData;
    }

    uint32_t status = uarpPlatformAssetSetPayloadIndex(
            &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset, payloadIndex);
    HAPAssert(status == kUARPStatusSuccess);

    return (status == kUARPStatusSuccess) ? kHAPError_None : kHAPError_Unknown;
}

HAP_RESULT_USE_CHECK
HAPError UARPSetFirmwareAssetPayloadDataOffset(uint32_t payloadOffset) {
    UARPAccessory* accessory = &uarpAccessory;

    // Make sure there is an asset in which to set the payload offset.
    if (!UARPIsFirmwareAssetActive(accessory)) {
        return kHAPError_InvalidState;
    }

    uint32_t status = uarpPlatformAssetSetPayloadOffset(
            &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset, payloadOffset);
    HAPAssert(status == kUARPStatusSuccess);

    switch (status) {
        case kUARPStatusSuccess: {
            return kHAPError_None;
        }

        // If the selected payload is invalid, or the offset is out of bounds.
        case kUARPStatusInvalidPayload:
        case kUARPStatusInvalidOffset: {
            return kHAPError_InvalidData;
        }

        default: {
            return kHAPError_Unknown;
        }
    }
}

HAP_RESULT_USE_CHECK
HAPError UARPFirmwareAssetFullyStaged(UARPVersion* stagedVersion) {
    HAPPrecondition(stagedVersion);
    UARPAccessory* accessory = &uarpAccessory;

    if (!UARPIsFirmwareAssetActive(accessory)) {
        return kHAPError_InvalidState;
    }

    HAPRawBufferCopyBytes(&(accessory->stagedFirmwareVersion), stagedVersion, sizeof(accessory->stagedFirmwareVersion));
    HAPLogDebug(
            &kHAPLog_Default,
            "%s: staged version = %d.%d.%d.%d",
            __func__,
            (int) accessory->stagedFirmwareVersion.major,
            (int) accessory->stagedFirmwareVersion.minor,
            (int) accessory->stagedFirmwareVersion.release,
            (int) accessory->stagedFirmwareVersion.build);

    uint32_t status = uarpPlatformAccessoryAssetFullyStaged(
            &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset);
    HAPAssert(status == kUARPStatusSuccess);

    return (status == kUARPStatusSuccess) ? kHAPError_None : kHAPError_Unknown;
}

HAP_RESULT_USE_CHECK
HAPError UARPRequestFirmwareAssetStagingStateChange(UARPAssetStagingStateChangeRequest state) {
    UARPAccessory* accessory = &uarpAccessory;

    uint32_t status = kUARPStatusSuccess;

    switch (state) {
        case kUARPAssetStagingStateChangeRequest_Pause: {
            HAPLogDebug(&uarpLogObject, "%s: Asset staging pause requested", __func__);
            if (!UARPIsFirmwareAssetActive(accessory)) {
                HAPLogError(&uarpLogObject, "%s: No active asset", __func__);
                return kHAPError_InvalidState;
            }
            if (!UARPIsFirmwareAssetLinkedToController(accessory)) {
                HAPLogError(&uarpLogObject, "%s: Asset is orphaned", __func__);
                return kHAPError_InvalidState;
            }
            status = uarpPlatformAccessoryPayloadRequestDataPause(
                    &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset);
            UARPCancelDataResponseTimer(accessory);
            break;
        }
        case kUARPAssetStagingStateChangeRequest_Resume: {
            HAPLogDebug(&uarpLogObject, "%s: Asset staging resume requested", __func__);
            if (!UARPIsFirmwareAssetActive(accessory)) {
                HAPLogError(&uarpLogObject, "%s: No active asset", __func__);
                return kHAPError_InvalidState;
            }
            if (!UARPIsFirmwareAssetLinkedToController(accessory)) {
                HAPLogInfo(&uarpLogObject, "%s: Asset is orphaned", __func__);
                // An asset may become orphaned between an accessory pause request and a subsequent resume
                // request. This does not indicate an error scenario.
                return kHAPError_None;
            }
            status = uarpPlatformAccessoryPayloadRequestDataResume(
                    &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset);
            break;
        }
        case kUARPAssetStagingStateChangeRequest_Abandon: {
            HAPLogDebug(&uarpLogObject, "%s: Asset staging abandon requested", __func__);
            if (UARPIsFirmwareAssetActive(accessory) && UARPIsFirmwareAssetLinkedToController(accessory)) {
                // Abandoning an asset which is being actively staged requires pausing the data transfer.
                status = uarpPlatformAccessoryPayloadRequestDataPause(
                        &(accessory->uarpPlatformAccessory), accessory->activeFirmwareAsset.uarpPlatformAsset);
                if (status == kUARPStatusSuccess) {
                    status = uarpPlatformAccessoryAssetAbandon(
                            &(accessory->uarpPlatformAccessory),
                            accessory->activeFirmwareAsset.controller->uarpPlatformController,
                            accessory->activeFirmwareAsset.uarpPlatformAsset);
                }
            } else if (UARPIsFirmwareAssetActive(accessory)) {
                // Abandon an orphaned asset, which does not send an asset processing notification.
                status = uarpPlatformAccessoryAssetAbandon(
                        &(accessory->uarpPlatformAccessory), NULL, accessory->activeFirmwareAsset.uarpPlatformAsset);
            }
            if (status == kUARPStatusSuccess) {
                UARPClearStagedFirmwareVersion(accessory);
                if (UARPIsFirmwareAssetActive(accessory)) {
                    // If the asset object hasn't been cleaned up (i.e. it has not be fully staged), further usage
                    // will be deferred until its release is complete.
                    accessory->activeFirmwareAsset.isPendingRelease = true;
                    UARPCancelDataResponseTimer(accessory);
                    if (!accessory->isProcessingRxMsg) {
                        // If an abandon occurs outside the context of a callback invoked via rx message processing,
                        // cleanup needs to be triggered.
                        uarpPlatformCleanupAssets(&(accessory->uarpPlatformAccessory));
                    }
                }
            }
            break;
        }
        default:
            HAPFatalError();
    }
    HAPAssert(status == kUARPStatusSuccess);

    return (status == kUARPStatusSuccess) ? kHAPError_None : kHAPError_Unknown;
}

bool UARPIsVersionZero(const UARPVersion* version) {
    HAPPrecondition(version);

    if ((version->major == 0) && (version->minor == 0) && (version->release == 0) && (version->build == 0)) {
        return true;
    } else {
        return false;
    }
}

bool UARPAreVersionsEqual(const UARPVersion* version1, const UARPVersion* version2) {
    HAPPrecondition(version1);
    HAPPrecondition(version2);

    if ((version1->major == version2->major) && (version1->minor == version2->minor) &&
        (version1->release == version2->release) && (version1->build == version2->build)) {
        return true;
    } else {
        return false;
    }
}

HAP_RESULT_USE_CHECK
HAPError UARPRegisterFirmwareAsset(
        const UARPAccessoryFirmwareAssetCallbacks* callbacks,
        UARPVersion* stagedFirmwareVersion) {
    HAPPrecondition(callbacks);
    HAPPrecondition(uarpAccessory.isInitialized);

    // Verify the accessory provided required callbacks.
    uarpAccessory.accessoryFirmwareAssetCallbacks = callbacks;
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->assetOffered);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->assetMetadataTLV);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->assetMetadataComplete);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->payloadReady);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->payloadMetadataTLV);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->payloadMetadataComplete);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->payloadData);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->payloadDataComplete);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->assetStateChange);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->applyStagedAssets);
    HAPAssert(uarpAccessory.accessoryFirmwareAssetCallbacks->retrieveLastError);

    // Store the staged firmware version (if any) provided by the accessory.
    if (stagedFirmwareVersion) {
        HAPRawBufferCopyBytes(
                &(uarpAccessory.stagedFirmwareVersion),
                stagedFirmwareVersion,
                sizeof(uarpAccessory.stagedFirmwareVersion));
    }

    HAPLogInfo(
            &kHAPLog_Default,
            "%s: staged firmware version = %d.%d.%d.%d",
            __func__,
            (int) uarpAccessory.stagedFirmwareVersion.major,
            (int) uarpAccessory.stagedFirmwareVersion.minor,
            (int) uarpAccessory.stagedFirmwareVersion.release,
            (int) uarpAccessory.stagedFirmwareVersion.build);

    return kHAPError_None;
}

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#endif // (HAVE_UARP_SUPPORT == 1)
