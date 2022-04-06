/*
 * Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
 * capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
 * Apple software is governed by and subject to the terms and conditions of your MFi License,
 * including, but not limited to, the restrictions specified in the provision entitled "Public
 * Software", and is further subject to your agreement to the following additional terms, and your
 * agreement that the use, installation, modification or redistribution of this Apple software
 * constitutes acceptance of these additional terms. If you do not agree with these additional terms,
 * you may not use, install, modify or redistribute this Apple software.
 *
 * Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
 * you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
 * license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
 * reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
 * redistribute the Apple Software, with or without modifications, in binary form, in each of the
 * foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
 * "Licensed Products" in accordance with the terms of your MFi License. While you may not
 * redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
 * form, you must retain this notice and the following text and disclaimers in all such redistributions
 * of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
 * used to endorse or promote products derived from the Apple Software without specific prior written
 * permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
 * express or implied, are granted by Apple herein, including but not limited to any patent rights that
 * may be infringed by your derivative works or by other works in which the Apple Software may be
 * incorporated. Apple may terminate this license to the Apple Software by removing it from the list
 * of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
 *
 * Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
 * fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
 * Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
 * reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
 * distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
 * and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
 * acknowledge and agree that Apple may exercise the license granted above without the payment of
 * royalties or further consideration to Participant.
 * The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
 * IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
 * IN COMBINATION WITH YOUR PRODUCTS.
 *
 * IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
 * AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2020 Apple Inc. All Rights Reserved.
 */

#include "CoreUARPPlatformAccessory.h"
#if !(UARP_DISABLE_CONTROLLER)
#include "CoreUARPSuperBinary.h"
#endif

/* MARK: INTERNAL DEFINES */

#define kUARPDataRequestTypeInvalid                         0x00
#define kUARPDataRequestTypeSuperBinaryHeader               0x01
#define kUARPDataRequestTypeSuperBinaryPayloadHeader        0x02
#define kUARPDataRequestTypeSuperBinaryMetadata             0x04
#define kUARPDataRequestTypePayloadMetadata                 0x10
#define kUARPDataRequestTypePayloadPayload                  0x20
#define kUARPDataRequestTypeOutstanding                     0x80

#define kUARPAssetHasHeader                                 0x01
#define kUARPAssetHasPayloadHeader                          0x02
#define kUARPAssetNeedsMetadata                             0x04
#define kUARPAssetHasMetadata                               0x08
#define kUARPAssetHasPayload                                0x10
#define kUARPAssetFullyStaged                               0x40
#define kUARPAssetMarkForCleanup                            0x80

typedef uint32_t (* fcnUarpPlatformAssetDataRequestComplete)( struct uarpPlatformAccessory *pAccessory,
                                                             struct uarpPlatformAsset *pAsset,
                                                             uint8_t reqType, uint32_t payloadTag,
                                                             uint32_t offset, uint8_t * pBuffer, uint32_t length );

/* MARK: INTERNAL PROTOTYPES */

static uint32_t uarpPlatformControllerAddInternal( struct uarpPlatformAccessory *pAccessory,
                                                  struct uarpPlatformController *pController,
                                                  struct uarpPlatformOptionsObj *pControllerOptions,
                                                  void *pControllerDelegate );

static struct uarpPlatformAsset *
    uarpPlatformAssetFindByAssetID( struct uarpPlatformAccessory *pAccessory,
                                   struct uarpPlatformController *pController,
                                   struct uarpPlatformAsset *pAssetList, uint16_t assetID );

static uint32_t uarpPlatformUpdateSuperBinaryMetaData( struct uarpPlatformAccessory *pAccessory,
                                                      struct uarpPlatformAsset *pAsset,
                                                      void *pBuffer, uint32_t lengthBuffer );

static uint32_t uarpPlatformUpdatePayloadMetaData( struct uarpPlatformAccessory *pAccessory,
                                                  struct uarpPlatformAsset *pAsset,
                                                  void *pBuffer, uint32_t lengthBuffer );

static uint32_t uarpPlatformUpdatePayloadPayload( struct uarpPlatformAccessory *pAccessory,
                                                 struct uarpPlatformAsset *pAsset,
                                                 void *pBuffer, uint32_t lengthBuffer );

static uint32_t uarpPlatformUpdateMetaData( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformAsset *pAsset,
                                           void *pBuffer, uint32_t lengthBuffer,
                                           fcnUarpPlatformAccessoryMetaDataTLV fMetaDataTLV,
                                           fcnUarpPlatformAccessoryMetaDataComplete fMetaDataComplete );

static void uarpPlatformCleanupAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                                   struct uarpPlatformController *pController );

static uint32_t uarpPlatformAccessoryAssetSuperBinaryPullHeader( struct uarpPlatformAccessory *pAccessory,
                                                                struct uarpPlatformAsset *pAsset );

static UARPBool uarpPlatformAccessoryShouldRequestMetadata( uint8_t flags );

static uint32_t uarpPlatformAccessoryAssetAbandonInternal( struct uarpPlatformAccessory *pAccessory,
                                                          struct uarpPlatformController *pController,
                                                          struct uarpPlatformAsset *pAsset,
                                                          uint16_t abandonReason,
                                                          UARPBool notifyController );

static uint32_t uarpPlatformDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                                struct uarpPlatformAsset *pAsset,
                                                uint8_t reqType, uint32_t payloadTag,
                                                uint32_t offset, uint8_t * pBuffer, uint32_t length );

static uint32_t uarpPlatformSuperBinaryHeaderDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                                                 struct uarpPlatformAsset *pAsset,
                                                                 uint8_t reqType, uint32_t payloadTag,
                                                                 uint32_t offset, uint8_t * pBuffer, uint32_t length );

static uint32_t uarpPlatformAssetPayloadHeaderDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                                                  struct uarpPlatformAsset *pAsset,
                                                                  uint8_t reqType, uint32_t payloadTag,
                                                                  uint32_t offset, uint8_t * pBuffer, uint32_t length );

static uint32_t uarpPlatformAssetRequestDataContinue( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformController *pController,
                                                     struct uarpPlatformAsset *pAsset );

static uint32_t uarpPlatformAssetRequestData( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset,
                                             uint8_t requestType, uint32_t relativeOffset, uint32_t lengthNeeded );

static void uarpPlatformAssetCleanup( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset );

static void uarpPlatformAssetRelease( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset );

static void uarpPlatformAssetOrphan( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset );

static void uarpPlatformAssetRescind( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformController *pController,
                                     struct uarpPlatformAsset *pAsset );

#if !(UARP_DISABLE_CONTROLLER)
static UARPAssetSubrange uarpPlatformDataRequestSubtype( uint8_t reqType );

static uint32_t uarpPlatformPrepareAsset( struct uarpPlatformAccessory *pAccessory,
                                         struct uarpPlatformController *pController,
                                         void *pAssetCtx, struct UARP4ccTag *pTag,
                                         struct uarpPlatformAssetCallbacks *pCallbacks );

static uint32_t uarpPlatformAssetProcessingNotification( void *pAccessoryDelegate, void *pControllerDelegate,
                                                        uint16_t assetID, uint16_t assetProcessingFlags );
#endif

/* MARK: CALLBACK PROTOTYPES */

static uint32_t uarpPlatformRequestTransmitMsgBuffer( void *pAccessoryDelegate, void *pControllerDelegate,
                                                     uint8_t **ppBuffer, uint32_t *pLength );

static void uarpPlatformReturnTransmitMsgBuffer( void *pAccessoryDelegate, void *pControllerDelegate, uint8_t *pBuffer );

static uint32_t uarpPlatformSendMessage( void *pAccessoryDelegate, void *pControllerDelegate,
                                        uint8_t *pBuffer, uint32_t length );

static uint32_t uarpPlatformDataTransferPause( void *pAccessoryDelegate, void *pControllerDelegate );

static uint32_t uarpPlatformDataTransferResume( void *pAccessoryDelegate, void *pControllerDelegate );

static uint32_t uarpPlatformQueryAccessoryInfo( void *pAccessoryDelegate, uint32_t infoType, void *pBuffer,
                                               uint32_t lengthBuffer, uint32_t *pLengthNeeded );

static uint32_t uarpPlatformApplyStagedAssets( void *pAccessoryDelegate, void *pControllerDelegate, uint16_t *pFlags );

static void uarpPlatformAssetRescinded( void *pAccessoryDelegate, void *pControllerDelegate,
                                       uint16_t assetID );

static uint32_t uarpPlatformAssetDataResponse( void *pAccessoryDelegate, void *pControllerDelegate, uint16_t assetID,
                                              uint8_t *pBuffer, uint32_t length, uint32_t offset );

static uint32_t uarpPlatformAssetOffered( void *pAccessoryDelegate, void *pControllerDelegate,
                                         struct uarpAssetCoreObj *pAssetCore );

static void uarpPlatformProtocolVersion( void *pAccessoryDelegate, uint16_t *pProtocolVersion );

static void uarpPlatformGarbageCollection( void *pAccessoryDelegate );

#if !(UARP_DISABLE_CONTROLLER)
static uint32_t uarpPlatformAssetSolicited( void *pAccessoryDelegate, void *pControllerDelegate,
                                           struct UARP4ccTag *pAssetTag );
#endif

#if !(UARP_DISABLE_CONTROLLER)
static uint32_t uarpPlatformAssetDataResponseMaxPayload( void *pAccessoryDelegate, void *pControllerDelegate,
                                                        uint16_t numBytesRequested, uint16_t *pMaxBytesResponded );
#endif

#if !(UARP_DISABLE_CONTROLLER)
static uint32_t uarpPlatformAssetDataRequest( void *pAccessoryDelegate, void *pControllerDelegate,
                                             uint16_t assetID, uint16_t numBytesRequested, uint32_t dataOffset,
                                             uint8_t *pBuffer, uint16_t *pNumBytesResponded );
#endif

static void uarpPlatformCleanupRxAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformController *pController );

#if !(UARP_DISABLE_CONTROLLER)
static void uarpPlatformCleanupTxAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformController *pController );
#endif

/* MARK: CONTROL ROUTINES */

uint32_t uarpPlatformAccessoryInit( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformOptionsObj *pOptions,
                                   struct uarpPlatformAccessoryCallbacks *pCallbacks,
                                   void *pVendorExtension, fcnUarpVendorSpecific fVendorSpecific, void *pDelegate )
{
    uint32_t status;
    struct uarpAccessoryCallbacksObj callbacks;

    /* verifiers */
    __UARP_Verify_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pOptions, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pCallbacks, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pDelegate, exit, status = kUARPStatusInvalidArgument );

    __UARP_Verify_Action( pCallbacks->fRequestBuffer, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fReturnBuffer, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fRequestTransmitMsgBuffer, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fReturnTransmitMsgBuffer, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fSendMessage, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fDataTransferPause, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fDataTransferResume, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fSuperBinaryOffered, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fDynamicAssetOffered, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fApplyStagedAssets, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fManufacturerName, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fModelName, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fSerialNumber, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fHardwareVersion, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fActiveFirmwareVersion, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fStagedFirmwareVersion, exit, status = kUARPStatusInvalidFunctionPointer );
    __UARP_Verify_Action( pCallbacks->fLastError, exit, status = kUARPStatusInvalidFunctionPointer );

    /* clear everything up */
    memset( pAccessory, 0, sizeof( struct uarpPlatformAccessory ) );

    /* store the delegate */
    pAccessory->callbacks = *pCallbacks;
    pAccessory->pDelegate = pDelegate;
    pAccessory->pVendorExtension = pVendorExtension;

    /* setup callbacks */
    memset( pAccessory, 0, sizeof( struct uarpAccessoryCallbacksObj ) );
    callbacks.fRequestTransmitMsgBuffer = uarpPlatformRequestTransmitMsgBuffer;
    callbacks.fReturnTransmitMsgBuffer = uarpPlatformReturnTransmitMsgBuffer;
    callbacks.fSendMessage = uarpPlatformSendMessage;
    callbacks.fAccessoryQueryAccessoryInfo = uarpPlatformQueryAccessoryInfo;
    callbacks.fAccessoryAssetOffered = uarpPlatformAssetOffered;
    callbacks.fAssetRescinded = uarpPlatformAssetRescinded;
    callbacks.fAccessoryAssetDataResponse = uarpPlatformAssetDataResponse;
    callbacks.fUpdateDataTransferPause = uarpPlatformDataTransferPause;
    callbacks.fUpdateDataTransferResume = uarpPlatformDataTransferResume;
    callbacks.fApplyStagedAssets = uarpPlatformApplyStagedAssets;
    callbacks.fVendorSpecific = fVendorSpecific;
    callbacks.fProtocolVersion = uarpPlatformProtocolVersion;
    callbacks.fGarbageCollection = uarpPlatformGarbageCollection;
#if !(UARP_DISABLE_CONTROLLER)
    callbacks.fAssetSolicitation = uarpPlatformAssetSolicited;
    callbacks.fAssetDataResponseMaxPayload = uarpPlatformAssetDataResponseMaxPayload;
    callbacks.fAssetDataRequest = uarpPlatformAssetDataRequest;
    callbacks.fAssetProcessingNotification = uarpPlatformAssetProcessingNotification;
#endif

    pAccessory->nextTxAssetID = kUARPAssetIDInvalid + 1;

    /* tell the lower edge we are here */
    pAccessory->_options = *pOptions;

    status = uarpAccessoryInit( &(pAccessory->_accessory), &callbacks, (void *)pAccessory );

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

uint32_t uarpPlatformControllerAdd( struct uarpPlatformAccessory *pAccessory,
                                   struct uarpPlatformController *pController,
                                   void *pControllerDelegate )
{
    return uarpPlatformControllerAddInternal( pAccessory, pController, NULL, pControllerDelegate );
}

uint32_t uarpPlatformControllerAddWithOptions( struct uarpPlatformAccessory *pAccessory,
                                              struct uarpPlatformController *pController,
                                              struct uarpPlatformOptionsObj *pControllerOptions,
                                              void *pControllerDelegate )
{
    return uarpPlatformControllerAddInternal( pAccessory, pController, pControllerOptions, pControllerDelegate );
}

uint32_t uarpPlatformControllerAddInternal( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformController *pController,
                                           struct uarpPlatformOptionsObj *pControllerOptions,
                                           void *pControllerDelegate )
{
    uint32_t status;

    /* uarpAccessoryRemoteControllerAdd() needs this set before it is called */
    pController->pDelegate = pControllerDelegate;

    /* set the options for this controller */
    if ( pControllerOptions )
    {
        pController->_options = *pControllerOptions;
    }
    else
    {
        pController->_options = pAccessory->_options;
    }
    __UARP_Require_Action( ( pController->_options.maxRxPayloadLength > 0 ), exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( ( pController->_options.maxTxPayloadLength > 0 ), exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( ( pController->_options.payloadWindowLength >= pController->_options.maxRxPayloadLength ),
                          exit, status = kUARPStatusInvalidArgument );

    if ( pController->_options.protocolVersion == 0 )
    {
        pController->_options.protocolVersion = pAccessory->_options.protocolVersion;
    }

    /* tell the lower layer about the controller */
    status = uarpAccessoryRemoteControllerAdd( &(pAccessory->_accessory),
                                              &(pController->_controller),
                                              (void *)pController );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Add Remote UARP Controller %d",
                pController->_controller.remoteControllerID );

exit:
    return status;
}

uint32_t uarpPlatformControllerRemove( struct uarpPlatformAccessory *pAccessory,
                                      struct uarpPlatformController *pController )
{
    uint32_t status;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Remove Remote UARP Controller %d",
                pController->_controller.remoteControllerID );

    /* tell the lower layer we are removing the contoller */
    status = uarpAccessoryRemoteControllerRemove( &(pAccessory->_accessory),
                                                 &(pController->_controller) );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    uarpPlatformCleanupAssetsForController( pAccessory, pController );

    /* done */
    status = kUARPStatusSuccess;

exit:
    return status;
}

uint32_t uarpPlatformAccessoryRecvMessage( struct uarpPlatformAccessory *pAccessory,
                                          struct uarpPlatformController *pController,
                                          uint8_t *pBuffer, uint32_t length )
{
    uint32_t status;

    /* Verifiers */
    __UARP_Verify_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "RECV %u bytes from Remote UARP Controller %d",
                 length, pController->_controller.remoteControllerID );

    /* let the lower layer know we got a message */
    status = uarpAccessoryRecvMessage( &(pAccessory->_accessory), &(pController->_controller), pBuffer, length );

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

uint32_t uarpPlatformAccessorySendSyncMsg( struct uarpPlatformAccessory *pAccessory,
                                          struct uarpPlatformController *pController )
{
    uint32_t status;

    status = uarpAccessorySendSyncMsg( &(pAccessory->_accessory), &(pController->_controller) );

    return status;
}

uint32_t uarpPlatformAccessoryAssetIsAcceptable( struct uarpPlatformAccessory *pAccessory,
                                                struct uarpPlatformAsset *pAsset,
                                                UARPBool *pIsAcceptable )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetIsAcceptable2( pAccessory, pAsset, pIsAcceptable, NULL );

    return status;
}

uint32_t uarpPlatformAccessoryAssetIsAcceptable2( struct uarpPlatformAccessory *pAccessory,
                                                 struct uarpPlatformAsset *pAsset,
                                                 UARPBool *pIsAcceptable, uint16_t *pReason )
{
    uint32_t status;
    uint32_t tag;
    struct UARPVersion activeFwVersion;
    struct UARPVersion stagedFwVersion;
    UARPVersionComparisonResult compareResult;

    /* assume the asset is acceptable until proven otherwise */
    *pIsAcceptable = kUARPYes;
    if ( pReason )
    {
        *pReason = 0;
    }

    /* make sure the offer is a newer version than we are running */
    tag = uarpTagStructPack32( &pAsset->core.asset4cc );
    status = pAccessory->callbacks.fActiveFirmwareVersion( pAccessory->pDelegate, tag, &activeFwVersion );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    compareResult = uarpVersionCompare( &activeFwVersion, &(pAsset->core.assetVersion) );

    if ( compareResult != kUARPVersionComparisonResultIsNewer )
    {
        uarpLogInfo( kUARPLoggingCategoryPlatform, "%s", "Active Firmware version is newer than the offered asset" );

        *pIsAcceptable = kUARPNo;

        if ( pReason )
        {
            if ( compareResult == kUARPVersionComparisonResultIsEqual )
            {
                *pReason = kUARPAssetProcessingFlagsReasonSameVersionActive;
            }
            else
            {
                *pReason = kUARPAssetProcessingFlagsReasonHigherVersionActive;
            }
        }
    }
    __UARP_Require_Action_Quiet( ( *pIsAcceptable == kUARPYes ), exit, status = kUARPStatusSuccess );

    /* make sure we don't have this version or newer staged */
    tag = uarpTagStructPack32( &pAsset->core.asset4cc );
    status = pAccessory->callbacks.fStagedFirmwareVersion( pAccessory->pDelegate, tag, &stagedFwVersion );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    compareResult = uarpVersionCompare( &stagedFwVersion, &(pAsset->core.assetVersion) );

    if ( compareResult != kUARPVersionComparisonResultIsNewer )
    {
        uarpLogInfo( kUARPLoggingCategoryPlatform, "%s", "Staged Firmware version is newer than the offered asset" );

        *pIsAcceptable = kUARPNo;
        if ( pReason )
        {
            if ( compareResult == kUARPVersionComparisonResultIsEqual )
            {
                *pReason = kUARPAssetProcessingFlagsReasonSameVersionStaged;
            }
            else
            {
                *pReason = kUARPAssetProcessingFlagsReasonHigherVersionStaged;
            }
        }
    }
    __UARP_Require_Action_Quiet( ( *pIsAcceptable == kUARPYes ), exit, status = kUARPStatusSuccess );

    status = kUARPStatusSuccess;

exit:
    if ( status != kUARPStatusSuccess )
    {
        *pIsAcceptable = kUARPNo;
    }

    return status;
}

uint32_t uarpPlatformAccessoryAssetCookieIsAcceptable( struct uarpPlatformAccessory *pAccessory,
                                                      struct uarpPlatformAsset *pAsset,
                                                      struct uarpPlatformAssetCookie *pCookie,
                                                      UARPBool *pIsAcceptable )
{
    uint32_t tag;
    UARPVersionComparisonResult versionResult;
    (void) pAccessory;

    versionResult = uarpVersionCompare( &(pAsset->core.assetVersion), &(pCookie->assetVersion) );

    tag = uarpTagStructPack32( &pAsset->core.asset4cc );
    if ( tag != pCookie->assetTag )
    {
        *pIsAcceptable = kUARPNo;
    }
    else if ( versionResult != kUARPVersionComparisonResultIsEqual )
    {
        *pIsAcceptable = kUARPNo;
    }
    else if ( pAsset->core.assetTotalLength != pCookie->assetTotalLength )
    {
        *pIsAcceptable = kUARPNo;
    }
    else if ( pAsset->core.assetNumPayloads != pCookie->assetNumPayloads )
    {
        *pIsAcceptable = kUARPNo;
    }
    else
    {
        *pIsAcceptable = kUARPYes;
    }

    return kUARPStatusSuccess;
}

uint32_t uarpPlatformAccessoryAssetAccept( struct uarpPlatformAccessory *pAccessory,
                                          struct uarpPlatformController *pController,
                                          struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetAccept2( pAccessory, pController, pAsset, NULL, NULL );

    return status;
}

uint32_t uarpPlatformAccessoryAssetAccept2( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformController *pController,
                                           struct uarpPlatformAsset *pAsset,
                                           void *pDelegate,
                                           struct uarpPlatformAssetCallbacks *pCallbacks )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetAccept2WithPayloadWindow( pAccessory, pController, pAsset, pDelegate,
                                                                pCallbacks, NULL, 0 );

    return status;
}

uint32_t uarpPlatformAccessoryAssetAccept2WithPayloadWindow( struct uarpPlatformAccessory *pAccessory,
                                                            struct uarpPlatformController *pController,
                                                            struct uarpPlatformAsset *pAsset,
                                                            void *pDelegate,
                                                            struct uarpPlatformAssetCallbacks *pCallbacks,
                                                            uint8_t *pPayloadWindow,
                                                            uint32_t payloadWindowLength )
{
    UARPBool isPresent;
    uint8_t flags;
    uint32_t status;
    struct uarpPlatformAsset *pAssetTmp;

    /* verifiers */
    __UARP_Verify_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    /* add to our list, after making sure we aren't already on it.
     We could be on it because we are merging / resuming */
    isPresent = kUARPNo;

    for ( pAssetTmp = pAccessory->pRxAssetList; pAssetTmp; pAssetTmp = pAssetTmp->pNext )
    {
        if ( pAssetTmp == pAsset )
        {
            isPresent = kUARPYes;
            break;
        }
    }
    if ( isPresent == kUARPNo )
    {
        pAsset->pNext = pAccessory->pRxAssetList;
        pAccessory->pRxAssetList = pAsset;
    }

    /* Do not set the delegate if it is NULL, older implementations may be setting it manually
        and calling uarpPlatformAccessoryAssetAccept() */
    if ( pDelegate )
    {
        pAsset->pDelegate = pDelegate;
    }
    pAsset->pausedByAccessory = kUARPNo;
    pAsset->internalFlags &= ~kUARPAssetMarkForCleanup;

    status = uarpPlatformAssetSetCallbacks( pAccessory, pAsset, pCallbacks );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    /* setup the scratch buffers / payload window for this asset,
        we may be merging an asset that already has a payload window */
    if ( ( pPayloadWindow != NULL ) && ( payloadWindowLength == 0 ) )
    {
        status = kUARPStatusInvalidArgument;
    }
    else if ( ( payloadWindowLength > 0 ) && ( payloadWindowLength != pController->_options.payloadWindowLength ) )
    {
        status = kUARPStatusInvalidArgument;
    }
    else
    {
        status = kUARPStatusSuccess;
    }
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    if ( pAsset->pScratchBuffer != NULL )
    {
        pAccessory->callbacks.fReturnBuffer( pAccessory->pDelegate, pAsset->pScratchBuffer );
        pAsset->pScratchBuffer = NULL;
    }

    if ( pPayloadWindow != NULL )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Accept Asset Offer Asset ID %u, payload window assigned",
                     pAsset->core.assetID );

        pAsset->pScratchBuffer = pPayloadWindow;
        pAsset->lengthScratchBuffer = payloadWindowLength;
    }
    else
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Accept Asset Offer Asset ID %u, payload window allocated",
                     pAsset->core.assetID );

        pAsset->lengthScratchBuffer = pController->_options.payloadWindowLength;
        status = pAccessory->callbacks.fRequestBuffer( pAccessory->pDelegate, &(pAsset->pScratchBuffer),
                                                      pAsset->lengthScratchBuffer );
        __UARP_Require( ( status == kUARPStatusSuccess ), exit );
    }

    /* read the SuperBinary Header */
    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Flags <%02x>", pAsset->internalFlags );
    uarpLogInfo( kUARPLoggingCategoryPlatform, "Selected Payload %d, Flags <%02x>",
                pAsset->selectedPayloadIndex, pAsset->payload.internalFlags );

    /* determine what we need to do for pulling the asset, this is because of resuming an orphaned asset */
    flags = kUARPAssetHasHeader | kUARPAssetHasPayloadHeader;

    if ( ( pAsset->internalFlags & flags ) == 0 )
    {
        status = uarpPlatformAccessoryAssetSuperBinaryPullHeader( pAccessory, pAsset );
    }
    else if ( uarpPlatformAccessoryShouldRequestMetadata( pAsset->internalFlags ) == kUARPYes )
    {
        status = uarpPlatformAccessoryAssetRequestMetaData( pAccessory, pAsset );
    }
    else if ( pAsset->selectedPayloadIndex == (int) kUARPPayloadIndexInvalid )
    {
        pAsset->callbacks.fAssetMetaDataComplete( pAccessory->pDelegate, pAsset->pDelegate );

        status = kUARPStatusSuccess;
    }
    else if ( uarpPlatformAccessoryShouldRequestMetadata( pAsset->payload.internalFlags ) == kUARPYes )
    {
        pAsset->callbacks.fPayloadReady( pAccessory->pDelegate, pAsset->pDelegate );

        status = kUARPStatusSuccess;
    }
    else
    {
        status = uarpPlatformAccessoryPayloadRequestData( pAccessory, pAsset );
    }

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetDeny( struct uarpPlatformAccessory *pAccessory,
                                        struct uarpPlatformController *pController,
                                        struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetDeny2( pAccessory, pController, pAsset, NULL, NULL );

    return status;
}

uint32_t uarpPlatformAccessoryAssetDeny2( struct uarpPlatformAccessory *pAccessory,
                                         struct uarpPlatformController *pController,
                                         struct uarpPlatformAsset *pAsset,
                                         void *pDelegate,
                                         struct uarpPlatformAssetCallbacks *pCallbacks )
{
    uint32_t status;
    (void) pDelegate;
    (void) pCallbacks;

    status = uarpPlatformAccessoryAssetDeny3( pAccessory, pController, pAsset, 0, NULL, NULL );

    return status;
}

uint32_t uarpPlatformAccessoryAssetDeny3( struct uarpPlatformAccessory *pAccessory,
                                         struct uarpPlatformController *pController,
                                         struct uarpPlatformAsset *pAsset,
                                         uint16_t denyReason,
                                         void *pDelegate,
                                         struct uarpPlatformAssetCallbacks *pCallbacks )
{
    uint32_t status;

    /* verifiers */
    __UARP_Verify_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pController, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Deny Asset ID <%u> for Controller <%d>",
                 pAsset->core.assetID, pController->_controller.remoteControllerID );

    /* Do not set the delegate if it is NULL, older implementations may be setting it manually
        and calling uarpPlatformAccessoryAssetDeny() */
    if ( pDelegate )
    {
        pAsset->pDelegate = pDelegate;
    }

    status = uarpPlatformAssetSetCallbacks( pAccessory, pAsset, pCallbacks );
    __UARP_Require( ( status == kUARPStatusSuccess ), exit );

    status = uarpAccessoryAssetDeny2( &(pAccessory->_accessory), &(pController->_controller),
                                     pAsset->core.assetID, denyReason );
    if ( status == kUARPStatusSuccess )
    {
        pAsset->internalFlags |= kUARPAssetMarkForCleanup;
    }

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetAbandon( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformController *pController,
                                           struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetAbandon2( pAccessory, pController, pAsset, 0 );

    return status;
}

uint32_t uarpPlatformAccessoryAssetAbandon2( struct uarpPlatformAccessory *pAccessory,
                                            struct uarpPlatformController *pController,
                                            struct uarpPlatformAsset *pAsset,
                                            uint16_t abandonReason )
{
    uint32_t status;
    UARPBool isNotifyController;

    if ( pController == NULL )
    {
        isNotifyController = kUARPNo;
    }
    else
    {
        isNotifyController = kUARPYes;
    }
    status = uarpPlatformAccessoryAssetAbandonInternal( pAccessory, pController, pAsset,
                                                       abandonReason, isNotifyController );

    return status;
}

uint32_t uarpPlatformAccessoryAssetAbandonByDelegate( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformController *pController,
                                                     void *pAssetDelegate )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;

    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );

    pAsset = uarpPlatformAssetFindByAssetContext( pAccessory, pAccessory->pRxAssetList, pAssetDelegate );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    status = uarpPlatformAccessoryAssetAbandon( pAccessory, pController, pAsset );

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetCorrupt( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset->pController, exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Corrupt Asset ID <%u> for Controller <%d>",
                 pAsset->core.assetID, pAsset->pController->_controller.remoteControllerID );

    pAsset->dataReq.requestType &= ~kUARPDataRequestTypeOutstanding;
    pAsset->internalFlags |= kUARPAssetMarkForCleanup;

    status = uarpAccessoryAssetCorrupt( &(pAccessory->_accessory),
                                       &(pAsset->pController->_controller),
                                       pAsset->core.assetID );

    pAsset->pController = NULL;

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetCorruptByDelegate( struct uarpPlatformAccessory *pAccessory,
                                                     void *pAssetDelegate )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;

    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );

    pAsset = uarpPlatformAssetFindByAssetContext( pAccessory, pAccessory->pRxAssetList, pAssetDelegate );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    status = uarpPlatformAccessoryAssetCorrupt( pAccessory, pAsset );

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetRelease( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformController *pController,
                                           struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    status = uarpPlatformAccessoryAssetAbandonInternal( pAccessory, pController, pAsset, 0, kUARPNo );

    return status;
}

uint32_t uarpPlatformAccessoryAssetReleaseByDelegate( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformController *pController,
                                                     void *pAssetDelegate )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;

    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );

    pAsset = uarpPlatformAssetFindByAssetContext( pAccessory, pAccessory->pTxAssetList, pAssetDelegate );
    if ( pAsset == NULL )
    {
        pAsset = uarpPlatformAssetFindByAssetContext( pAccessory, pAccessory->pRxAssetList, pAssetDelegate );
    }
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    status = uarpPlatformAccessoryAssetRelease( pAccessory, pController, pAsset );

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetRequestMetaData( struct uarpPlatformAccessory *pAccessory,
                                                   struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    if ( pAsset && ( pAsset->sbHdr.superBinaryMetadataLength > 0 ) )
    {
        status = uarpPlatformAssetRequestData( pAccessory, pAsset,
                                              kUARPDataRequestTypeSuperBinaryMetadata,
                                              0,
                                              pAsset->sbHdr.superBinaryMetadataLength );
    }
    else
    {
        status = kUARPStatusNoMetaData;
    }

    return status;
}

uint32_t uarpPlatformAssetQueryNumberOfPayloads( struct uarpPlatformAccessory *pAccessory,
                                                struct uarpPlatformAsset *pAsset,
                                                uint16_t *pNumPayloads )
{
    uint32_t status;

    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    *pNumPayloads = pAsset->core.assetNumPayloads;

    status = kUARPStatusSuccess;

exit:
    return status;
}

uint32_t uarpPlatformAssetSetPayloadIndex( struct uarpPlatformAccessory *pAccessory,
                                          struct uarpPlatformAsset *pAsset, int payloadIdx )
{
    return uarpPlatformAssetSetPayloadIndexWithCookie( pAccessory, pAsset, payloadIdx, NULL );
}

uint32_t uarpPlatformAssetSetPayloadIndexWithCookie( struct uarpPlatformAccessory *pAccessory,
                                                    struct uarpPlatformAsset *pAsset, int payloadIdx,
                                                    struct uarpPlatformAssetCookie *pCookie )
{
    uint32_t offset;
    uint32_t status;
    UARPBool isValidCookie;

    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( ( payloadIdx < pAsset->core.assetNumPayloads ), exit,
                          status = kUARPStatusInvalidPayload );

    isValidCookie = uarpPlatformAssetIsCookieValid( pAccessory, pAsset, pCookie );

    if ( isValidCookie )
    {
        payloadIdx = pCookie->selectedPayloadIndex;
    }

    pAsset->lengthPayloadRecvd = 0;

    /* save off the index and clear the fact that we have the paylaod header */
    pAsset->selectedPayloadIndex = payloadIdx;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Set Active Payload Index <%d>", pAsset->selectedPayloadIndex );

    pAsset->internalFlags = pAsset->internalFlags & ~kUARPAssetHasPayloadHeader;
    pAsset->payload.internalFlags = 0;

    /* determine offset based on selected index payloadIdx */
    offset = pAsset->selectedPayloadIndex * sizeof( struct UARPPayloadHeader );

    /* call the lower edge */
    status = uarpPlatformAssetRequestData( pAccessory, pAsset,
                                          kUARPDataRequestTypeSuperBinaryPayloadHeader,
                                          offset,
                                          sizeof( struct UARPPayloadHeader ) );

exit:
    return status;
}

uint32_t uarpPlatformAccessoryPayloadRequestMetaData( struct uarpPlatformAccessory *pAccessory,
                                                     struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    if ( pAsset && ( pAsset->payload.plHdr.payloadMetadataLength > 0 ) )
    {
        status = uarpPlatformAssetRequestData( pAccessory, pAsset,
                                              kUARPDataRequestTypePayloadMetadata,
                                              0,
                                              pAsset->payload.plHdr.payloadMetadataLength );
    }
    else
    {
        status = kUARPStatusNoMetaData;
    }

    return status;
}

uint32_t uarpPlatformAssetSetPayloadOffset( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformAsset *pAsset,
                                           uint32_t payloadOffset )
{
    uint32_t status;
    (void) pAccessory;

    /* ensure that...
        - we have selected a payload
        - the offset is within bounds
        - there is not an outstanding data request
     */
    if ( pAsset == NULL )
    {
        status = kUARPStatusInvalidArgument;
    }
    else if ( pAsset->selectedPayloadIndex == (int) kUARPPayloadIndexInvalid )
    {
        status = kUARPStatusInvalidPayload;
    }
    else if ( payloadOffset >= pAsset->payload.plHdr.payloadLength )
    {
        status = kUARPStatusInvalidOffset;
    }
    else if ( pAsset->dataReq.requestType & kUARPDataRequestTypeOutstanding )
    {
        status = kUARPStatusAssetInFlight;
    }
    else
    {
        pAsset->lengthPayloadRecvd = payloadOffset;

        status = kUARPStatusSuccess;
    }

    return status;
}

uint32_t uarpPlatformAccessoryPayloadRequestData( struct uarpPlatformAccessory *pAccessory,
                                                 struct uarpPlatformAsset *pAsset )
{
    return uarpPlatformAccessoryPayloadRequestDataWithCookie( pAccessory, pAsset, NULL );
}

uint32_t uarpPlatformAccessoryPayloadRequestDataWithCookie( struct uarpPlatformAccessory *pAccessory,
                                                           struct uarpPlatformAsset *pAsset,
                                                           struct uarpPlatformAssetCookie *pCookie )
{
    UARPBool isValidCookie;
    uint32_t length;
    uint32_t status;

    /* verifiers */
    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    /* if we have a valid cookie and try to update the payload offset */
    isValidCookie = uarpPlatformAssetIsCookieValid( pAccessory, pAsset, pCookie );

    if ( isValidCookie == kUARPYes )
    {
        status = uarpPlatformAssetSetPayloadOffset( pAccessory, pAsset, pCookie->lengthPayloadRecvd );
        __UARP_Check( status == kUARPStatusSuccess );
    }

    /* payload window unless less is left */
    if ( pAsset->payload.plHdr.payloadLength == 0 )
    {
        length = 0;
    }
    else
    {
        length = pAsset->payload.plHdr.payloadLength - pAsset->lengthPayloadRecvd;
    }

    if ( length > pAsset->lengthScratchBuffer )
    {
        length = pAsset->lengthScratchBuffer;
    }

    /* call the lower edge */
    if ( length == 0 )
    {
        pAsset->callbacks.fPayloadDataComplete( pAccessory->pDelegate, pAsset->pDelegate );
        status = kUARPStatusSuccess;
    }
    else
    {
        status = uarpPlatformAssetRequestData( pAccessory, pAsset,
                                              kUARPDataRequestTypePayloadPayload,
                                              pAsset->lengthPayloadRecvd,
                                              length );
    }

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

uint32_t uarpPlatformAccessoryPayloadRequestDataPause( struct uarpPlatformAccessory *pAccessory,
                                                      struct uarpPlatformAsset *pAsset )
{
    uint32_t status;
    (void) pAccessory;

    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    __UARP_Require_Action( ( pAsset->pausedByAccessory == kUARPNo ), exit, status = kUARPStatusSuccess );

    pAsset->pausedByAccessory = kUARPYes;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Transfer paused on Asset ID %u, by accessory request",
                 pAsset->core.assetID );

    /* NOTE: Intentionally do not try to pause an outstanding data request.  Let it complete. */
    status = kUARPStatusSuccess;

exit:
    return status;
}

uint32_t uarpPlatformAccessoryPayloadRequestDataResume( struct uarpPlatformAccessory *pAccessory,
                                                       struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    __UARP_Require_Action( ( pAsset->pausedByAccessory == kUARPYes ), exit, status = kUARPStatusSuccess );

    pAsset->pausedByAccessory = kUARPNo;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Transfer resumed on Asset ID %u, by accessory request",
                 pAsset->core.assetID );

    __UARP_Require_Action( ( pAsset->pController != NULL ), exit, status = kUARPStatusSuccess );

    if ( ( pAsset->dataReq.requestType & kUARPDataRequestTypeOutstanding ) == 0 )
    {
        status = uarpPlatformAccessoryPayloadRequestData( pAccessory, pAsset );
    }
    else
    {
        status = kUARPStatusSuccess;
    }

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetFullyStaged( struct uarpPlatformAccessory *pAccessory,
                                               struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    /* verifiers */
    __UARP_Verify_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset->pController, exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Staged Asset ID <%u> for Controller <%d>",
                 pAsset->core.assetID,
                 pAsset->pController->_controller.remoteControllerID );

    pAsset->internalFlags |= kUARPAssetFullyStaged;

    status = uarpAccessoryAssetStaged( &(pAccessory->_accessory),
                                      &(pAsset->pController->_controller),
                                      pAsset->core.assetID );

exit:
    return status;
}

uint32_t uarpPlatformAccessorySuperBinaryMerge( struct uarpPlatformAccessory *pAccessory,
                                               struct uarpPlatformAsset *pAssetOrphaned,
                                               struct uarpPlatformAsset *pAssetOffered )
{
    uint32_t status;
    (void) pAccessory;

    /* verifiers */
    __UARP_Verify_Action( pAssetOrphaned, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pAssetOffered, exit, status = kUARPStatusInvalidArgument );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Merging Assets <%u> -> <%u>",
                pAssetOffered->core.assetID, pAssetOrphaned->core.assetID );

    pAssetOrphaned->core.assetID = pAssetOffered->core.assetID;

    pAssetOrphaned->dataReq.requestType &= ~kUARPDataRequestTypeOutstanding;

    pAssetOffered->internalFlags |= kUARPAssetMarkForCleanup;

    pAssetOrphaned->pController = pAssetOffered->pController;
    pAssetOffered->pController = NULL;

    /* set the delegate for the platform */
    pAssetOrphaned->pDelegate = pAssetOrphaned;

    /* done */
    status = kUARPStatusSuccess;

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

uint32_t uarpPlatformAccessorySuperBinaryMerge2( struct uarpPlatformAccessory *pAccessory,
                                                struct uarpPlatformAsset *pMergeFrom,
                                                struct uarpPlatformAsset *pMergeTo )
{
    uint32_t status;
    (void) pAccessory;

    /* verifiers */
    __UARP_Verify_Action( pMergeFrom, exit, status = kUARPStatusInvalidArgument );
    __UARP_Verify_Action( pMergeTo, exit, status = kUARPStatusInvalidArgument );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Merging Assets <%u> -> <%u>",
                pMergeFrom->core.assetID, pMergeTo->core.assetID );

    pMergeTo->sbHdr = pMergeFrom->sbHdr;

    /* core should not be copied */

    /* mark the old asset for cleanup and ensure the new one is not */
    pMergeFrom->internalFlags |= kUARPAssetMarkForCleanup;
    pMergeTo->internalFlags = ( pMergeFrom->internalFlags & ~kUARPAssetMarkForCleanup);

    /* make an outstanding data request invalid */
    pMergeFrom->dataReq.requestType &= ~kUARPDataRequestTypeOutstanding;
    pMergeTo->dataReq = pMergeFrom->dataReq;

    pMergeTo->pausedByAccessory = pMergeFrom->pausedByAccessory;

    pMergeTo->selectedPayloadIndex = pMergeFrom->selectedPayloadIndex;
    pMergeTo->payload = pMergeFrom->payload;
    pMergeTo->lengthPayloadRecvd = pMergeFrom->lengthPayloadRecvd;

    /* move the scratch buffer from the old asset to the new asset */
    pMergeTo->pScratchBuffer = pMergeFrom->pScratchBuffer;
    pMergeTo->lengthScratchBuffer = pMergeFrom->lengthScratchBuffer;

    pMergeFrom->pScratchBuffer = NULL;
    pMergeFrom->lengthScratchBuffer = 0;

    /* pController and pDelegate are set at offer time */

    /* done */
    status = kUARPStatusSuccess;

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

void uarpPlatformAccessorySendMessageComplete( struct uarpPlatformAccessory *pAccessory,
                                              struct uarpPlatformController *pController,
                                              uint8_t *pBuffer )
{
    if ( pAccessory )
    {
        uarpPlatformReturnTransmitMsgBuffer( pAccessory, pController, pBuffer );
    }

    return;
}

/* MARK: HELPERS */

struct uarpPlatformAsset *
    uarpPlatformAssetFindByAssetID( struct uarpPlatformAccessory *pAccessory,
                                   struct uarpPlatformController *pController,
                                   struct uarpPlatformAsset *pAssetList,
                                   uint16_t assetID )
{
    struct uarpPlatformAsset *pTmp;
    struct uarpPlatformAsset *pAsset;
    (void) pAccessory;

    pAsset = NULL;

    /* look for the remote accessory in the superbinary list */
    for ( pTmp = pAssetList; pTmp; pTmp = pTmp->pNext  )
    {
        if ( ( pTmp->pController == pController ) && ( pTmp->core.assetID == assetID ) )
        {
            pAsset = pTmp;
            break;
        }
    }

    return pAsset;
}

struct uarpPlatformAsset *
    uarpPlatformAssetFindByAssetContext( struct uarpPlatformAccessory *pAccessory,
                                        struct uarpPlatformAsset *pAssetList,
                                        void *pAssetCtx )
{
    struct uarpPlatformAsset *pTmp;
    struct uarpPlatformAsset *pAsset;
    (void) pAccessory;

    pAsset = NULL;

    /* look for the remote accessory in the superbinary list */
    for ( pTmp = pAssetList; pTmp; pTmp = pTmp->pNext  )
    {
        if ( pTmp->pDelegate == pAssetCtx )
        {
            pAsset = pTmp;
            break;
        }
    }

    return pAsset;
}

uint32_t uarpPlatformUpdateSuperBinaryMetaData( struct uarpPlatformAccessory *pAccessory,
                                               struct uarpPlatformAsset *pAsset,
                                               void *pBuffer, uint32_t lengthBuffer )
{
    uint32_t status;
    (void) pAccessory;

    /* mark as having the metadata */
    pAsset->internalFlags |= kUARPAssetHasMetadata;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "%s", "SuperBinary MetaData Rx COMPLETE" );

    status = uarpPlatformUpdateMetaData( pAccessory, pAsset, pBuffer, lengthBuffer,
                                        pAsset->callbacks.fAssetMetaDataTLV,
                                        pAsset->callbacks.fAssetMetaDataComplete );

    return status;
}

uint32_t uarpPlatformUpdatePayloadMetaData( struct uarpPlatformAccessory *pAccessory,
                                           struct uarpPlatformAsset *pAsset,
                                           void *pBuffer, uint32_t lengthBuffer )
{
    uint32_t status;
    struct uarpPayloadObj *pPayload;

    /* mark as having the metadata */
    pPayload = &(pAsset->payload);
    pPayload->internalFlags |= kUARPAssetHasMetadata;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset <%d> Payload <%c%c%c%c> MetaData Rx COMPLETE",
                pAsset->core.assetID,
                pPayload->payload4cc[0], pPayload->payload4cc[1],
                pPayload->payload4cc[2], pPayload->payload4cc[3] );

    status = uarpPlatformUpdateMetaData( pAccessory, pAsset, pBuffer, lengthBuffer,
                                        pAsset->callbacks.fPayloadMetaDataTLV,
                                        pAsset->callbacks.fPayloadMetaDataComplete );

    return status;
}

uint32_t uarpPlatformUpdatePayloadPayload( struct uarpPlatformAccessory *pAccessory,
                                          struct uarpPlatformAsset *pAsset,
                                          void *pBuffer, uint32_t lengthBuffer )
{
    uint32_t status;
    struct uarpPayloadObj *pPayload;
    struct uarpPlatformAssetCookie cookie;

    /* need to update what was coming from controller */
    pPayload = &(pAsset->payload);

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Payload <%c%c%c%c> Payload Rx Window Complete %u bytes from offset %u",
                pPayload->payload4cc[0], pPayload->payload4cc[1],
                pPayload->payload4cc[2], pPayload->payload4cc[3],
                lengthBuffer, pAsset->lengthPayloadRecvd );

    cookie.assetTag = uarpTagStructPack32( &pAsset->core.asset4cc );
    cookie.assetVersion = pAsset->core.assetVersion;
    cookie.assetTotalLength = pAsset->core.assetTotalLength;
    cookie.assetNumPayloads = pAsset->core.assetNumPayloads;
    cookie.selectedPayloadIndex = pAsset->selectedPayloadIndex;
    cookie.lengthPayloadRecvd = pAsset->lengthPayloadRecvd;

    /* Assume the entire payload window would be consumed, but if compressed,
        it will be adjusted in the scope of fPayloadData() */
    pAsset->lengthScratchBufferConsumed = lengthBuffer;

    pAsset->callbacks.fPayloadData( pAccessory->pDelegate, pAsset->pDelegate, pBuffer, lengthBuffer,
                                   pAsset->lengthPayloadRecvd, (void *)&cookie, sizeof( cookie ) );

    pAsset->lengthPayloadRecvd += pAsset->lengthScratchBufferConsumed;

    /* if we consumed less than the payload buffer, we must be dealing with compression.  Therefore, adjust the payload
     window to slide up the unconsumed bytes to index 0 and request more bytes to fill the next window without
     re-requesting already transferreed bytes.  The firmware was likely compressed due to a slow transport, so requesting
     bytes would be silly */
    if ( pAsset->lengthScratchBufferConsumed < lengthBuffer )
    {
        pAsset->lengthScratchBufferUnconsumed = lengthBuffer - pAsset->lengthScratchBufferConsumed;

        uint8_t *pSourceBuffer = (uint8_t *)pBuffer + pAsset->lengthScratchBufferConsumed;
        memcpy( pBuffer, pSourceBuffer, pAsset->lengthScratchBufferUnconsumed );
    }

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Payload <%c%c%c%c> Payload RX %u bytes of %u",
                pPayload->payload4cc[0], pPayload->payload4cc[1],
                pPayload->payload4cc[2], pPayload->payload4cc[3],
                pAsset->lengthPayloadRecvd, pPayload->plHdr.payloadLength );

    /* either pull the next window or indicate this is completed */
    if ( pAsset->internalFlags & kUARPAssetMarkForCleanup )
    {
        /* The callback fPayloadData may have determined the asset was corrupt or decided to abandon it */
        status = kUARPStatusSuccess;
    }
    else if ( pAsset->lengthPayloadRecvd == pPayload->plHdr.payloadLength )
    {
        pPayload->internalFlags |= kUARPAssetHasPayload;

        uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Payload <%c%c%c%c> Payload Rx COMPLETE",
                    pPayload->payload4cc[0], pPayload->payload4cc[1],
                    pPayload->payload4cc[2], pPayload->payload4cc[3] );

        pAsset->callbacks.fPayloadDataComplete( pAccessory->pDelegate, pAsset->pDelegate );

        status = kUARPStatusSuccess;
    }
    else if ( pAsset->pausedByAccessory == kUARPNo )
    {
        status = uarpPlatformAccessoryPayloadRequestData( pAccessory, pAsset );
    }
    else
    {
        status = kUARPStatusSuccess;
    }

    return status;
}

void uarpPlatformCleanupAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                            struct uarpPlatformController *pController )
{
    uarpPlatformCleanupRxAssetsForController( pAccessory, pController );
#if !(UARP_DISABLE_CONTROLLER)
    uarpPlatformCleanupTxAssetsForController( pAccessory, pController );
#endif
}

void uarpPlatformCleanupRxAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                              struct uarpPlatformController *pController )
{
    struct uarpPlatformAsset *pAssets;
    struct uarpPlatformAsset *pTmpAsset;

    __UARP_Require_Quiet( ( pAccessory->_accessory.rxLock == kUARPNo ), exit );

    /* run through the assets, orphaning superbinaries from this controller. */
    pAssets = pAccessory->pRxAssetList;
    pAccessory->pRxAssetList = NULL;

    while ( pAssets )
    {
        pTmpAsset = pAssets;
        pAssets = pAssets->pNext;

        /* Are we cleaning up for all controllers(=NULL)? Or a particular one ? */
        if ( ( pController != NULL ) && ( pTmpAsset->pController != pController ) )
        {
            pTmpAsset->pNext = pAccessory->pRxAssetList;
            pAccessory->pRxAssetList = pTmpAsset;
            continue;
        }

        /* If we're a dynamic asset, and not doing an "all controller" cleanup, we need to be released */
        if ( ( pController != NULL ) && ( uarpAssetIsDynamicAsset( &pTmpAsset->core ) ) )
        {
            pTmpAsset->internalFlags |= kUARPAssetMarkForCleanup;
        }

        if ( pTmpAsset->internalFlags & ( kUARPAssetMarkForCleanup | kUARPAssetFullyStaged ) )
        {
            uarpPlatformAssetRelease( pAccessory, pTmpAsset );
            uarpPlatformAssetCleanup( pAccessory, pTmpAsset );
            continue;
        }

        /* if we're a superbinary that wasn't marked for cleanup, let the upper layer know we were orphaned and keep it around */
        if ( pController != NULL )
        {
            uarpPlatformAssetOrphan( pAccessory, pTmpAsset );
        }

        pTmpAsset->pNext = pAccessory->pRxAssetList;
        pAccessory->pRxAssetList = pTmpAsset;
    }

exit:
    return;
}

#if !(UARP_DISABLE_CONTROLLER)
void uarpPlatformCleanupTxAssetsForController( struct uarpPlatformAccessory *pAccessory,
                                              struct uarpPlatformController *pController )
{
    struct uarpPlatformAsset *pAssets;
    struct uarpPlatformAsset *pTmpAsset;

    __UARP_Require_Quiet( ( pAccessory->_accessory.rxLock == kUARPNo ), exit );

    /* run through the assets, orphaning superbinaries from this controller. */
    pAssets = pAccessory->pTxAssetList;
    pAccessory->pTxAssetList = NULL;

    while ( pAssets )
    {
        pTmpAsset = pAssets;
        pAssets = pAssets->pNext;

        /* Are we cleaning up for all controllers(=NULL)? Or a particular one ? */
        if ( ( pController != NULL ) && ( pTmpAsset->pController != pController ) )
        {
            pTmpAsset->pNext = pAccessory->pTxAssetList;
            pAccessory->pTxAssetList = pTmpAsset;
            continue;
        }

        /* If we're a dynamic asset, and not doing an "all controller" cleanup, we need to be released */
        if ( ( pController != NULL ) && ( uarpAssetIsDynamicAsset( &pTmpAsset->core ) ) )
        {
            pTmpAsset->internalFlags |= kUARPAssetMarkForCleanup;
        }

        if ( pTmpAsset->internalFlags & ( kUARPAssetMarkForCleanup | kUARPAssetFullyStaged ) )
        {
            uarpPlatformAssetRelease( pAccessory, pTmpAsset );
            uarpPlatformAssetCleanup( pAccessory, pTmpAsset );
            continue;
        }

        /* if we're a superbinary that wasn't marked for cleanup, let the upper layer know we were orphaned and keep it around */
        if ( pController != NULL )
        {
            uarpPlatformAssetOrphan( pAccessory, pTmpAsset );
        }

        pTmpAsset->pNext = pAccessory->pTxAssetList;
        pAccessory->pTxAssetList = pTmpAsset;
    }

exit:
    return;
}
#endif

void uarpPlatformCleanupAssets( struct uarpPlatformAccessory *pAccessory )
{
    uarpPlatformCleanupAssetsForController( pAccessory, NULL );

    return;
}

uint32_t uarpPlatformAssetSetCallbacks( struct uarpPlatformAccessory *pAccessory,
                                       struct uarpPlatformAsset *pAsset,
                                       struct uarpPlatformAssetCallbacks *pCallbacks )
{
    uint32_t status;

    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    /* Setup the callbacks pointers.  This seems tedious but it will be helpful
     in future uses of the actual cases */
    if ( pCallbacks )
    {
        pAsset->callbacks = *pCallbacks;
    }
    if ( pAsset->callbacks.fAssetReady == NULL )
    {
        pAsset->callbacks.fAssetReady = pAccessory->callbacks.fAssetReady;
    }
    if ( ( pAsset->callbacks.fAssetStore == NULL ) && ( kUARPNo == uarpAssetIsDynamicAsset( &(pAsset->core) ) ) )
    {
        pAsset->callbacks.fAssetStore = pAccessory->callbacks.fAssetStore;
    }
    if ( pAsset->callbacks.fAssetMetaDataTLV == NULL )
    {
        pAsset->callbacks.fAssetMetaDataTLV = pAccessory->callbacks.fAssetMetaDataTLV;
    }
    if ( pAsset->callbacks.fAssetMetaDataComplete == NULL )
    {
        pAsset->callbacks.fAssetMetaDataComplete = pAccessory->callbacks.fAssetMetaDataComplete;
    }
    if ( pAsset->callbacks.fPayloadReady == NULL )
    {
        pAsset->callbacks.fPayloadReady = pAccessory->callbacks.fPayloadReady;
    }
    if ( pAsset->callbacks.fPayloadMetaDataTLV == NULL )
    {
        pAsset->callbacks.fPayloadMetaDataTLV = pAccessory->callbacks.fPayloadMetaDataTLV;
    }
    if ( pAsset->callbacks.fPayloadMetaDataComplete == NULL )
    {
        pAsset->callbacks.fPayloadMetaDataComplete = pAccessory->callbacks.fPayloadMetaDataComplete;
    }
    if ( pAsset->callbacks.fPayloadData == NULL )
    {
        pAsset->callbacks.fPayloadData = pAccessory->callbacks.fPayloadData;
    }
    if ( pAsset->callbacks.fPayloadDataComplete == NULL )
    {
        pAsset->callbacks.fPayloadDataComplete = pAccessory->callbacks.fPayloadDataComplete;
    }
    if ( pAsset->callbacks.fAssetRescinded == NULL )
    {
        pAsset->callbacks.fAssetRescinded = pAccessory->callbacks.fAssetRescinded;
    }
    if ( pAsset->callbacks.fAssetCorrupt == NULL )
    {
        pAsset->callbacks.fAssetCorrupt = pAccessory->callbacks.fAssetCorrupt;
    }
    if ( pAsset->callbacks.fAssetOrphaned == NULL )
    {
        pAsset->callbacks.fAssetOrphaned = pAccessory->callbacks.fAssetOrphaned;
    }
    if ( pAsset->callbacks.fAssetReleased == NULL )
    {
        pAsset->callbacks.fAssetReleased = pAccessory->callbacks.fAssetReleased;
    }

    status = kUARPStatusSuccess;

exit:
    return status;
}

#if !(UARP_DISABLE_CONTROLLER)
uint32_t uarpPlatformPrepareSuperBinary( struct uarpPlatformAccessory *pAccessory,
                                        struct uarpPlatformController *pController,
                                        void *pAssetCtx,
                                        struct uarpPlatformAssetCallbacks *pCallbacks )
{
    return uarpPlatformPrepareAsset( pAccessory, pController, pAssetCtx, NULL, pCallbacks );
}
#endif

#if !(UARP_DISABLE_CONTROLLER)
uint32_t uarpPlatformPrepareDynamicAsset( struct uarpPlatformAccessory *pAccessory,
                                         struct uarpPlatformController *pController,
                                         void *pAssetCtx, struct UARP4ccTag *pTag,
                                         struct uarpPlatformAssetCallbacks *pCallbacks )
{
    return uarpPlatformPrepareAsset( pAccessory, pController, pAssetCtx, pTag, pCallbacks );
}
#endif

#if !(UARP_DISABLE_CONTROLLER)
uint32_t uarpPlatformPrepareAsset( struct uarpPlatformAccessory *pAccessory,
                                  struct uarpPlatformController *pController,
                                  void *pAssetCtx, struct UARP4ccTag *pTag,
                                  struct uarpPlatformAssetCallbacks *pCallbacks )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;

    __UARP_Require_Action( pController, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAssetCtx, exit, status = kUARPStatusInvalidArgument );

    status = pAccessory->callbacks.fRequestBuffer( pAccessory->pDelegate, (uint8_t **)&pAsset,
                                                  sizeof( struct uarpPlatformAsset ) );
    __UARP_Require( status == kUARPStatusSuccess, exit );
    pAsset->pDelegate = pAssetCtx;
    pAsset->pController = pController;

    /* put it on the list */
    pAsset->pNext = pAccessory->pTxAssetList;
    pAccessory->pTxAssetList = pAsset;

    /* Setup the callbacks pointers.  This seems tedious but it will be helpful
     in future uses of the actual cases */
    status = uarpPlatformAssetSetCallbacks( pAccessory, pAsset, pCallbacks );
    __UARP_Require( status == kUARPStatusSuccess, exit );

    /* assign the asset id */
    if ( ( pAccessory->nextTxAssetID + 1 ) == kUARPAssetIDAllAssets )
    {
        pAccessory->nextTxAssetID = kUARPAssetIDInvalid + 1;
    }
    pAsset->core.assetID = pAccessory->nextTxAssetID++;

    if ( pTag )
    {
        pAsset->core.assetFlags = kUARPAssetFlagsAssetTypeDynamic;
        pAsset->core.asset4cc = *pTag;
        pAsset->core.assetTag = uarpTagStructPack32( pTag );
    }
    else
    {
        pAsset->core.assetFlags = kUARPAssetFlagsAssetTypeSuperBinary;
    }

exit:
    return status;
}
#endif

#if !(UARP_DISABLE_CONTROLLER)
uint32_t uarpPlatformOfferAsset( struct uarpPlatformAccessory *pAccessory,
                                struct uarpPlatformController *pController,
                                void *pAssetCtx )
{
    uint32_t status;
    uint16_t length;
    uint16_t lengthResponded;
    struct uarpPlatformAsset *pAsset;
    struct UARPSuperBinaryHeader sbHdr;

    __UARP_Require_Action( pController, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAccessory, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAssetCtx, exit, status = kUARPStatusInvalidArgument );

    pAsset = uarpPlatformAssetFindByAssetContext( pAccessory, pAccessory->pTxAssetList, pAssetCtx );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset->callbacks.fAssetGetBytesAtOffset, exit,
                          status = kUARPStatusInvalidFunctionPointer );

    /* read superbinary header and set up some core values */
    length = (uint16_t)sizeof( struct UARPSuperBinaryHeader );
    status = pAsset->callbacks.fAssetGetBytesAtOffset( pAccessory->pDelegate, pAsset->pDelegate,
                                                      &sbHdr, length, 0, &lengthResponded );
    __UARP_Require( status == kUARPStatusSuccess, exit );
    uarpSuperBinaryHeaderEndianSwap( &sbHdr, &pAsset->sbHdr );

    /* grab the final pieces from the superbinary header for the asset core */
    pAsset->core.assetVersion = pAsset->sbHdr.superBinaryVersion;
    pAsset->core.assetTotalLength = pAsset->sbHdr.superBinaryLength;
    pAsset->core.assetNumPayloads = (uint16_t)pAsset->sbHdr.payloadHeadersLength / (uint16_t)sizeof( struct UARPPayloadHeader );

    status = uarpAccessoryAssetOffer( &(pAccessory->_accessory), &(pController->_controller), &(pAsset->core) );

exit:
    return status;
}
#endif

uint32_t uarpPlatformUpdateMetaData( struct uarpPlatformAccessory *pAccessory,
                                    struct uarpPlatformAsset *pAsset,
                                    void *pBuffer, uint32_t lengthBuffer,
                                    fcnUarpPlatformAccessoryMetaDataTLV fMetaDataTLV,
                                    fcnUarpPlatformAccessoryMetaDataComplete fMetaDataComplete )
{
    uint8_t *pTmp;
    uint32_t tlvType;
    uint32_t tlvLength;
    uint32_t status;
    uint32_t remainingLength;
    struct UARPTLVHeader *pTlv;

    /* mark as having the metadata */
    pAsset->internalFlags |= kUARPAssetHasMetadata;

    /* iterate */
    pTmp = pBuffer;
    remainingLength = lengthBuffer;

    while ( remainingLength >= sizeof( struct UARPTLVHeader ) )
    {
        pTlv = (struct UARPTLVHeader *)pTmp;

        tlvType = uarpNtohl( pTlv->tlvType );
        tlvLength = uarpNtohl( pTlv->tlvLength );

        pTmp += sizeof( struct UARPTLVHeader );
        remainingLength -= sizeof( struct UARPTLVHeader );

        __UARP_Require_Action( ( remainingLength >= tlvLength ), exit, status = kUARPStatusMetaDataCorrupt );

        fMetaDataTLV( pAccessory->pDelegate, pAsset->pDelegate, tlvType, tlvLength, pTmp );

        pTmp += tlvLength;
        remainingLength -= tlvLength;
    }

    fMetaDataComplete( pAccessory->pDelegate, pAsset->pDelegate );

    /* done */
    status = kUARPStatusSuccess;

exit:
    return status;
}

uint32_t uarpPlatformAccessoryAssetSuperBinaryPullHeader( struct uarpPlatformAccessory *pAccessory,
                                                         struct uarpPlatformAsset *pAsset )
{
    uint32_t status;

    /* verifiers */
    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );

    /* call the lower edge */
    status = uarpPlatformAssetRequestData( pAccessory, pAsset,
                                          kUARPDataRequestTypeSuperBinaryHeader,
                                          0,
                                          sizeof( struct UARPSuperBinaryHeader ) );

__UARP_Verify_exit /* This resolves "exit:" if you have chosen to compile in __UARP_Verify_Action */
    return status;
}

UARPBool uarpPlatformAccessoryShouldRequestMetadata( uint8_t flags )
{
    UARPBool status;

    if ( ( flags & kUARPAssetNeedsMetadata ) && ( ( flags & kUARPAssetHasMetadata ) == 0 ) )
    {
        status = kUARPYes;
    }
    else
    {
        status = kUARPNo;
    }

    return status;
}

uint32_t uarpPlatformAccessoryAssetAbandonInternal( struct uarpPlatformAccessory *pAccessory,
                                                   struct uarpPlatformController *pController,
                                                   struct uarpPlatformAsset *pAsset, uint16_t abandonReason,
                                                   UARPBool notifyController )
{
    uint32_t status;

    if ( pAsset )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Abandon Asset ID <%u> for Controller <%d>",
                     pAsset->core.assetID,
                     pController ? pController->_controller.remoteControllerID : -1 );

        if ( notifyController == kUARPYes )
        {
            status = uarpAccessoryAssetAbandon2( &(pAccessory->_accessory), &(pController->_controller),
                                                pAsset->core.assetID, abandonReason );
        }
        else
        {
            status = kUARPStatusSuccess;
        }

        pAsset->dataReq.requestType &= ~kUARPDataRequestTypeOutstanding;
        pAsset->internalFlags |= kUARPAssetMarkForCleanup;

        pAsset->pController = NULL;
    }
    else
    {
        status = kUARPStatusSuccess;
    }

    return status;
}

uint32_t uarpPlatformSuperBinaryHeaderDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                                          struct uarpPlatformAsset *pAsset,
                                                           uint8_t reqType, uint32_t payloadTag,
                                                           uint32_t offset, uint8_t * pBuffer, uint32_t length )
{
    uint32_t status;
    struct UARPSuperBinaryHeader *pSbHdr;

    /* alias the request buffer and superbinary header */
    pSbHdr = (struct UARPSuperBinaryHeader *)pBuffer;

    pAsset->sbHdr.superBinaryFormatVersion = uarpNtohl( pSbHdr->superBinaryFormatVersion );
    pAsset->sbHdr.superBinaryHeaderLength = uarpNtohl( pSbHdr->superBinaryHeaderLength );
    pAsset->sbHdr.superBinaryLength = uarpNtohl( pSbHdr->superBinaryLength );

    uarpVersionEndianSwap( &(pSbHdr->superBinaryVersion), &(pAsset->sbHdr.superBinaryVersion) );

    pAsset->sbHdr.superBinaryMetadataOffset = uarpNtohl( pSbHdr->superBinaryMetadataOffset );
    pAsset->sbHdr.superBinaryMetadataLength = uarpNtohl( pSbHdr->superBinaryMetadataLength );
    pAsset->sbHdr.payloadHeadersOffset = uarpNtohl( pSbHdr->payloadHeadersOffset );
    pAsset->sbHdr.payloadHeadersLength = uarpNtohl( pSbHdr->payloadHeadersLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Offered (asset id %u)",
                pAsset->core.assetID );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Format Version %08x",
                pAsset->sbHdr.superBinaryFormatVersion );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Header Length %u",
                pAsset->sbHdr.superBinaryHeaderLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Length %u",
                pAsset->sbHdr.superBinaryLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Version (%u.%u.%u.%u)",
                pAsset->sbHdr.superBinaryVersion.major,
                pAsset->sbHdr.superBinaryVersion.minor,
                pAsset->sbHdr.superBinaryVersion.release,
                pAsset->sbHdr.superBinaryVersion.build );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Metadata Offset %u",
                pAsset->sbHdr.superBinaryMetadataOffset );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Metadata Length %u",
                pAsset->sbHdr.superBinaryMetadataLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Headers Offset %u",
                pAsset->sbHdr.payloadHeadersOffset );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Headers Length %u",
                pAsset->sbHdr.payloadHeadersLength );

    /* Verify lengths and offset */
    if ( pAsset->sbHdr.superBinaryFormatVersion != kUARPSuperBinaryFormatVersion )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else if ( pAsset->sbHdr.superBinaryHeaderLength != sizeof( struct UARPSuperBinaryHeader ) )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else if ( ( pAsset->sbHdr.superBinaryMetadataOffset + pAsset->sbHdr.superBinaryMetadataLength ) >
             pAsset->core.assetTotalLength )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else if ( ( pAsset->sbHdr.payloadHeadersOffset + pAsset->sbHdr.payloadHeadersLength ) >
             pAsset->core.assetTotalLength )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else
    {
        status = kUARPStatusSuccess;
    }

    if ( status != kUARPStatusSuccess )
    {
        status = uarpAccessoryAssetCorrupt( &(pAccessory->_accessory),
                                           &(pAsset->pController->_controller),
                                           pAsset->core.assetID );

        if ( pAsset->callbacks.fAssetCorrupt )
        {
            pAsset->callbacks.fAssetCorrupt( pAccessory->pDelegate, pAsset->pDelegate );
        }
    }
    else
    {
        if ( pAsset->sbHdr.superBinaryMetadataLength > 0 )
        {
            pAsset->internalFlags |= kUARPAssetNeedsMetadata;
        }

        pAsset->internalFlags |= kUARPAssetHasHeader;

        status = uarpPlatformDataRequestComplete( pAccessory, pAsset, reqType, payloadTag, offset, pBuffer, length );
    }

    return status;
}

uint32_t uarpPlatformAssetPayloadHeaderDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                                           struct uarpPlatformAsset *pAsset,
                                                           uint8_t reqType, uint32_t payloadTag,
                                                           uint32_t offset, uint8_t * pBuffer, uint32_t length )
{
    uint32_t status;
    struct UARPPayloadHeader *pPlHdr;

    /* TODO: double check reqType */

    /* alias the request buffer and superbinary header */
    pPlHdr = (struct UARPPayloadHeader *)pBuffer;

    pAsset->payload.plHdr.payloadHeaderLength = uarpNtohl( pPlHdr->payloadHeaderLength );
    pAsset->payload.plHdr.payloadTag = pPlHdr->payloadTag; /* no endian intentionally */

    uarpVersionEndianSwap( &(pPlHdr->payloadVersion), &(pAsset->payload.plHdr.payloadVersion) );

    pAsset->payload.plHdr.payloadMetadataOffset = uarpNtohl( pPlHdr->payloadMetadataOffset );
    pAsset->payload.plHdr.payloadMetadataLength = uarpNtohl( pPlHdr->payloadMetadataLength );
    pAsset->payload.plHdr.payloadOffset = uarpNtohl( pPlHdr->payloadOffset );
    pAsset->payload.plHdr.payloadLength = uarpNtohl( pPlHdr->payloadLength );

    pAsset->payload.internalFlags |= kUARPAssetHasPayloadHeader;

    if ( pAsset->payload.plHdr.payloadMetadataLength > 0 )
    {
        pAsset->payload.internalFlags |= kUARPAssetNeedsMetadata;
    }

    uarpPayloadTagUnpack( pAsset->payload.plHdr.payloadTag, pAsset->payload.payload4cc );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset Offered (asset id %u), Payload %d",
                pAsset->core.assetID, pAsset->selectedPayloadIndex );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Header Length %u",
                pAsset->payload.plHdr.payloadHeaderLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Tag 0x%08x <%c%c%c%c>",
                pAsset->payload.plHdr.payloadTag,
                pAsset->payload.payload4cc[0],
                pAsset->payload.payload4cc[1],
                pAsset->payload.payload4cc[2],
                pAsset->payload.payload4cc[3]);

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Version (%u.%u.%u.%u)",
                pAsset->payload.plHdr.payloadVersion.major,
                pAsset->payload.plHdr.payloadVersion.minor,
                pAsset->payload.plHdr.payloadVersion.release,
                pAsset->payload.plHdr.payloadVersion.build );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Metadata Offset %u",
                pAsset->payload.plHdr.payloadMetadataOffset );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Metadata Length %u",
                pAsset->payload.plHdr.payloadMetadataLength );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Offset %u",
                pAsset->payload.plHdr.payloadOffset );

    uarpLogInfo( kUARPLoggingCategoryPlatform, "- Payload Length %u",
                pAsset->payload.plHdr.payloadLength );

    /* Verify lengths and offset */
    if ( ( pAsset->payload.plHdr.payloadMetadataOffset + pAsset->payload.plHdr.payloadMetadataLength ) >
             pAsset->core.assetTotalLength )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else if ( ( pAsset->payload.plHdr.payloadOffset + pAsset->payload.plHdr.payloadLength ) >
             pAsset->core.assetTotalLength )
    {
        status = kUARPStatusInvalidSuperBinaryHeader;
    }
    else
    {
        status = kUARPStatusSuccess;
    }

    if ( status != kUARPStatusSuccess )
    {
        status = uarpAccessoryAssetCorrupt( &(pAccessory->_accessory),
                                           &(pAsset->pController->_controller),
                                           pAsset->core.assetID );

        if ( pAsset->callbacks.fAssetCorrupt )
        {
            pAsset->callbacks.fAssetCorrupt( pAccessory->pDelegate, pAsset->pDelegate );
        }
    }
    else
    {
        pAsset->internalFlags |= kUARPAssetHasPayloadHeader;

        status = uarpPlatformDataRequestComplete( pAccessory, pAsset, reqType, payloadTag, offset, pBuffer, length );
    }

    return status;
}

uint32_t uarpPlatformAssetRequestDataContinue( struct uarpPlatformAccessory *pAccessory,
                                              struct uarpPlatformController *pController,
                                              struct uarpPlatformAsset *pAsset )
{
    uint32_t status;
    uint32_t bytesToRequest;

    /* TODO: make quiet */
    __UARP_Require_Action( ( pAsset->pausedByAccessory == kUARPNo ), exit, status = kUARPStatusDataTransferPaused );
    __UARP_Require_Action( ( pAsset->dataReq.bytesRemaining > 0 ), exit, status = kUARPStatusAssetNoBytesRemaining );

    /* adjust bytes to request */
    bytesToRequest = pAsset->dataReq.bytesRemaining;

    if ( bytesToRequest > pController->_options.maxRxPayloadLength )
    {
        bytesToRequest = pController->_options.maxRxPayloadLength;
    }

    uarpLogDebug( kUARPLoggingCategoryPlatform, "REQ BYTES - Asset <%u> <%c%c%c%c> Request Type <0x%x> "
                "Relative Offset <%u> Absolute Offset <%u> Current Offset <%u> "
                "Bytes Requested <%u> Bytes Responded <%u> Bytes Remaining <%u> Bytes to Request <%u>",
                pAsset->core.assetID,
                pAsset->payload.payload4cc[0] ? pAsset->payload.payload4cc[0] : '0',
                pAsset->payload.payload4cc[1] ? pAsset->payload.payload4cc[1] : '0',
                pAsset->payload.payload4cc[2] ? pAsset->payload.payload4cc[2] : '0',
                pAsset->payload.payload4cc[3] ? pAsset->payload.payload4cc[3] : '0',
                pAsset->dataReq.requestType,
                pAsset->dataReq.relativeOffset,
                pAsset->dataReq.absoluteOffset,
                pAsset->dataReq.currentOffset,
                pAsset->dataReq.bytesRequested,
                pAsset->dataReq.bytesResponded,
                pAsset->dataReq.bytesRemaining,
                bytesToRequest );

    status = uarpAccessoryAssetRequestData( &(pAccessory->_accessory),
                                           &(pController->_controller),
                                           pAsset->core.assetID,
                                           pAsset->dataReq.currentOffset,
                                           bytesToRequest );

exit:
    /* TODO: is this appropriate here, given the require macros?? */
    if ( status == kUARPStatusSuccess )
    {
        pAsset->dataReq.requestType |= kUARPDataRequestTypeOutstanding;
    }
    else if ( status == kUARPStatusDataTransferPaused )
    {
        status = kUARPStatusSuccess;
    }
    else if ( status == kUARPStatusAssetNoBytesRemaining )
    {
        status = kUARPStatusSuccess;
    }

    return status;
}

/* pAsset will be released when returning from this routine */
void uarpPlatformAssetCleanup( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset )
{
    uarpLogInfo( kUARPLoggingCategoryPlatform, "Asset cleaned up (asset id %u)",
                pAsset->core.assetID );

    if ( pAsset->pScratchBuffer )
    {
        pAccessory->callbacks.fReturnBuffer( pAccessory->pDelegate, (uint8_t *)pAsset->pScratchBuffer );
        pAsset->pScratchBuffer = NULL;
    }
    pAsset->lengthScratchBuffer = 0;

    pAccessory->callbacks.fReturnBuffer( pAccessory->pDelegate, (uint8_t *)pAsset );

    return;
}

uint32_t uarpPlatformAssetOffered( void *pAccessoryDelegate, void *pControllerDelegate,
                                  struct uarpAssetCoreObj *pAssetCore )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* initializers */
    pAsset = NULL;

    /* alias our context pointer */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    status = pAccessory->callbacks.fRequestBuffer( pAccessory->pDelegate, (uint8_t **)&pAsset,
                                                  sizeof( struct uarpPlatformAsset ) );
    __UARP_Require( status == kUARPStatusSuccess, exit );

    pAsset->pController = pController;
    pAsset->core = *pAssetCore;
    pAsset->selectedPayloadIndex = kUARPPayloadIndexInvalid;

    /* forward assign accessory callbacks to the new asset */
    pAsset->callbacks.fAssetReady = pAccessory->callbacks.fAssetReady;
    pAsset->callbacks.fAssetStore = pAccessory->callbacks.fAssetStore;
    pAsset->callbacks.fAssetMetaDataTLV = pAccessory->callbacks.fAssetMetaDataTLV;
    pAsset->callbacks.fAssetMetaDataComplete = pAccessory->callbacks.fAssetMetaDataComplete;
    pAsset->callbacks.fPayloadReady = pAccessory->callbacks.fPayloadReady;
    pAsset->callbacks.fPayloadMetaDataTLV = pAccessory->callbacks.fPayloadMetaDataTLV;
    pAsset->callbacks.fPayloadMetaDataComplete = pAccessory->callbacks.fPayloadMetaDataComplete;
    pAsset->callbacks.fPayloadData = pAccessory->callbacks.fPayloadData;
    pAsset->callbacks.fPayloadDataComplete = pAccessory->callbacks.fPayloadDataComplete;
    pAsset->callbacks.fAssetRescinded = pAccessory->callbacks.fAssetRescinded;
    pAsset->callbacks.fAssetCorrupt = pAccessory->callbacks.fAssetCorrupt;
    pAsset->callbacks.fAssetOrphaned = pAccessory->callbacks.fAssetOrphaned;
    pAsset->callbacks.fAssetReleased = pAccessory->callbacks.fAssetReleased;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Offered from UARP Controller %d <Asset ID %u>",
                 pController->_controller.remoteControllerID,
                 pAsset->core.assetID );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "- Version <%u.%u.%u.%u>",
                 pAsset->core.assetVersion.major,
                 pAsset->core.assetVersion.minor,
                 pAsset->core.assetVersion.release,
                 pAsset->core.assetVersion.build );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "- Flags <0x%08x>", pAsset->core.assetFlags );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "- Tag <0x%08x>", pAsset->core.assetTag );

    /* put this on the right asset list and indicate to the upper layer */
    pAsset->pNext = pAccessory->pRxAssetList;
    pAccessory->pRxAssetList = pAsset;

    /* Tell the upper layer about this asset being offered */
    if ( uarpAssetIsSuperBinary( &(pAsset->core) ) )
    {
        pAccessory->callbacks.fSuperBinaryOffered( pAccessory->pDelegate, pController->pDelegate, pAsset );

        status = kUARPStatusSuccess;
    }
    else if ( uarpAssetIsDynamicAsset( &(pAsset->core) ) )
    {
        pAccessory->callbacks.fDynamicAssetOffered( pAccessory->pDelegate, pController->pDelegate, pAsset );

        status = kUARPStatusSuccess;
    }
    else
    {
        status = kUARPStatusInvalidAssetType;
    }

exit:
    /* TODO: if failed, free asset and payloads */

    return status;
}

uint32_t uarpPlatformAssetDataResponse( void *pAccessoryDelegate, void *pControllerDelegate, uint16_t assetID,
                                        uint8_t *pBuffer, uint32_t length, uint32_t offset )
{
    uint8_t *pResponseBuffer;
    uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength];
    uint32_t status;
    struct uarpPlatformAsset *pAsset;
    struct uarpDataRequestObj *pRequest;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;
    fcnUarpPlatformAssetDataRequestComplete fRequestComplete;

    /* alias our context pointers */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    pAsset = uarpPlatformAssetFindByAssetID( pAccessory, pController, pAccessory->pRxAssetList, assetID );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusNoResources );

    /* if the asset has been abandoned or rescinded, we should ignore this response */
    if ( pAsset->internalFlags & kUARPAssetMarkForCleanup )
    {
        pAsset = NULL;
    }
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusUnknownAsset );

    /* copy into the data response buffer, if everything checks out */
    pRequest = &(pAsset->dataReq);
    __UARP_Require_Action( pAsset->pScratchBuffer, exit, status = kUARPStatusNoResources );
    __UARP_Require_Action( ( pAsset->pScratchBuffer == pRequest->bytes ), exit, status = kUARPStatusNoResources );
    __UARP_Require_Action( ( pRequest->currentOffset == offset ), exit, status = kUARPStatusMismatchDataOffset );
    __UARP_Require_Action( ( pRequest->bytesRequested >= length ), exit,
                          status = kUARPStatusInvalidDataResponseLength );
    __UARP_Require_Action( ( pRequest->requestType | kUARPDataRequestTypeOutstanding ), exit,
                          status = kUARPStatusInvalidDataResponse );

    uarpPayloadTagUnpack( pRequest->payloadTag, payload4cc );

    pResponseBuffer = pRequest->bytes;
    pResponseBuffer += pRequest->bytesResponded;

    /* copy buffers */
    memcpy( pResponseBuffer, pBuffer, length );

    pRequest->bytesResponded += length;

    pRequest->requestType = pRequest->requestType & ~kUARPDataRequestTypeOutstanding;

    /* some requests are internal to the Firmware Updater, so we will hijack the completion routine */
    if ( pRequest->requestType == kUARPDataRequestTypeSuperBinaryHeader )
    {
        fRequestComplete = uarpPlatformSuperBinaryHeaderDataRequestComplete;
    }
    else if ( pRequest->requestType == kUARPDataRequestTypeSuperBinaryPayloadHeader )
    {
        fRequestComplete = uarpPlatformAssetPayloadHeaderDataRequestComplete;
    }
    else
    {
        fRequestComplete = uarpPlatformDataRequestComplete;
    }

#if !(UARP_DISABLE_CONTROLLER)
    {
        UARPAssetSubrange subrange = uarpPlatformDataRequestSubtype( pRequest->requestType );

        if ( ( pAsset->callbacks.fAssetStore ) && ( subrange < kUARPAssetSubrangeMax ) )
        {
            pAsset->callbacks.fAssetStore( pAccessory->pDelegate, pAsset->pDelegate, subrange,
                                          pBuffer, length, offset );
        }
    }
#endif

    /* inform the delegate when the request is full */
    /* TODO: Handle when we simply won't fill the requested buffer; or does bytesRequested handle that? */
    pRequest->bytesRemaining = pRequest->bytesRequested - pRequest->bytesResponded;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "RSP BYTES - Asset <%u> <%c%c%c%c> Request Type <0x%x> "
                "Relative Offset <%u> Absolute Offset <%u> Current Offset <%u> "
                "Bytes Requested <%u> Bytes Responded <%u> Total Bytes Responded <%u> Bytes Remaining <%u> ",
                pAsset->core.assetID,
                payload4cc[0], payload4cc[1], payload4cc[2], payload4cc[3], pRequest->requestType,
                pRequest->relativeOffset, pRequest->absoluteOffset, pRequest->currentOffset,
                pRequest->bytesRequested, length, pRequest->bytesResponded, pRequest->bytesRemaining );

    pRequest->currentOffset = pRequest->absoluteOffset + pRequest->bytesResponded;

    /* TODO: do we want to hold onto a full payload if paused, until resume? */
    if ( pRequest->bytesResponded == pRequest->bytesRequested )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Data Response from Controller ID <%d> - All Bytes Requested",
                     pController->_controller.remoteControllerID );

        status = fRequestComplete( pAccessory, pAsset, pRequest->requestType,
                                  pRequest->payloadTag, pRequest->relativeOffset,
                                  pRequest->bytes, pRequest->bytesResponded );
    }
    else if ( pController->_controller.dataTransferAllowed == kUARPNo )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Data Response from Controller ID <%d>"
                     "Transfer Paused by controller, wait for resume",
                     pController->_controller.remoteControllerID );

        status = kUARPStatusSuccess;
    }
    else if ( pAsset->pausedByAccessory == kUARPYes )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Data Response from Controller ID <%d>"
                     "Transfer Paused by accessory, wait for resume",
                     pController->_controller.remoteControllerID );

        status = kUARPStatusSuccess;
    }
    else
    {
        status = uarpPlatformAssetRequestDataContinue( pAccessory, pAsset->pController, pAsset );
    }

exit:
    return status;
}

uint32_t uarpPlatformAssetRequestData( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset,
                                      uint8_t requestType, uint32_t relativeOffset, uint32_t lengthNeeded )

{
    uint32_t status;
    uint32_t tmp32;
    uint32_t startOffset;
    uint32_t maxLength;
    struct uarpPayloadObj *pPayload;
    struct uarpDataRequestObj *pRequest;

    __UARP_Verify_Action( pAsset, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pAsset->pScratchBuffer, exit, status = kUARPStatusNoResources );
    __UARP_Require_Action( ( pAsset->lengthScratchBuffer >= lengthNeeded ), exit,
                          status = kUARPStatusNoResources );

    pRequest = &(pAsset->dataReq);
    __UARP_Require_Action( ( ( pRequest->requestType & kUARPDataRequestTypeOutstanding ) == 0 ), exit,
                          status = kUARPStatusAssetInFlight );

    pRequest->requestType = requestType;
    pRequest->relativeOffset = relativeOffset;
    pRequest->bytesRequested = lengthNeeded;
    pRequest->bytes = pAsset->pScratchBuffer;

    /* adjust bytes for unconsumed bytes from previous round, this would only happen if the payload is compressed */
    pRequest->bytesResponded = pAsset->lengthScratchBufferUnconsumed;
    pAsset->lengthScratchBufferUnconsumed = 0;

    /* Only one outstanding request per asset */
    if ( pRequest->requestType == kUARPDataRequestTypeSuperBinaryHeader )
    {
        startOffset = 0;
        maxLength = sizeof( struct UARPSuperBinaryHeader );
    }
    else if ( pRequest->requestType == kUARPDataRequestTypeSuperBinaryPayloadHeader )
    {
        startOffset = pAsset->sbHdr.payloadHeadersOffset;
        maxLength = pAsset->sbHdr.payloadHeadersLength;
    }
    else if ( pRequest->requestType == kUARPDataRequestTypeSuperBinaryMetadata )
    {
        startOffset = pAsset->sbHdr.superBinaryMetadataOffset;
        maxLength = pAsset->sbHdr.superBinaryMetadataLength;
    }
    else if ( pRequest->requestType == kUARPDataRequestTypePayloadMetadata )
    {
        pPayload = &(pAsset->payload);

        pRequest->payloadTag = pAsset->payload.plHdr.payloadTag;

        startOffset = pPayload->plHdr.payloadMetadataOffset;
        maxLength = pPayload->plHdr.payloadMetadataLength;
    }
    else if ( pRequest->requestType == kUARPDataRequestTypePayloadPayload )
    {
        pPayload = &(pAsset->payload);

        pRequest->payloadTag = pAsset->payload.plHdr.payloadTag;

        startOffset = pPayload->plHdr.payloadOffset;
        maxLength = pPayload->plHdr.payloadLength;
    }
    else
    {
        startOffset = 0;
        maxLength = 0;
    }

    /* validate the request  */
    __UARP_Require_Action( ( maxLength > 0 ), exit, status = kUARPStatusInvalidDataRequestLength );
    __UARP_Require_Action( ( pRequest->bytesRequested <= maxLength ), exit,
                          status = kUARPStatusInvalidDataRequestLength );

    tmp32 = pRequest->relativeOffset + pRequest->bytesRequested;
    __UARP_Require_Action( ( tmp32 <= maxLength ), exit, status = kUARPStatusInvalidDataRequestOffset );

    /* make the call */
    pRequest->absoluteOffset = startOffset + pRequest->relativeOffset;
    pRequest->currentOffset = pRequest->absoluteOffset + pRequest->bytesResponded;
    pRequest->bytesRemaining = pRequest->bytesRequested - pRequest->bytesResponded;

    status = uarpPlatformAssetRequestDataContinue( pAccessory, pAsset->pController, pAsset );

exit:
    return status;
}

/* MARK: CALLBACKS */

uint32_t uarpPlatformRequestTransmitMsgBuffer( void *pAccessoryDelegate, void *pControllerDelegate,
                                              uint8_t **ppBuffer, uint32_t *pLength )
{
    uint32_t status;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    status = pAccessory->callbacks.fRequestTransmitMsgBuffer( pAccessory->pDelegate, pController->pDelegate,
                                                             ppBuffer, pLength );

    return status;
}

void uarpPlatformReturnTransmitMsgBuffer( void *pAccessoryDelegate, void *pControllerDelegate, uint8_t *pBuffer )
{
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    pAccessory->callbacks.fReturnTransmitMsgBuffer( pAccessory->pDelegate, pController->pDelegate, pBuffer );

    return;
}

uint32_t uarpPlatformSendMessage( void *pAccessoryDelegate, void *pControllerDelegate,
                                 uint8_t *pBuffer, uint32_t length )
{
    uint32_t status;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "SEND %u bytes to Remote UARP Controller %d",
                 length, pController->_controller.remoteControllerID );

    status = pAccessory->callbacks.fSendMessage( pAccessory->pDelegate, pController->pDelegate, pBuffer, length );

    return status;
}

uint32_t uarpPlatformDataTransferPause( void *pAccessoryDelegate, void *pControllerDelegate )
{
    uint32_t status;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Transfers PAUSED from Remote Controller %u",
                pController->_controller.remoteControllerID );

    status = pAccessory->callbacks.fDataTransferPause( pAccessory->pDelegate, pController->pDelegate );

    return status;
}

uint32_t uarpPlatformDataTransferResume( void *pAccessoryDelegate, void *pControllerDelegate )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    uarpLogInfo( kUARPLoggingCategoryPlatform, "Transfers RESUMED from Remote Controller %u",
                pController->_controller.remoteControllerID );

    /* look through the assets related to this controller and see if there are any outstanding data requests
        if there are, it was likely dropped by the controller so we need to re-request  */
    for ( pAsset = pAccessory->pRxAssetList; pAsset != NULL; pAsset = pAsset->pNext )
    {
        if ( pAsset->pController != pController )
        {
            continue;
        }

        status = uarpPlatformAssetRequestDataContinue( pAccessory, pController, pAsset );
        __UARP_Check( status == kUARPStatusSuccess );
    }

    status = pAccessory->callbacks.fDataTransferResume( pAccessory->pDelegate, pController->pDelegate );

    return status;
}

uint32_t uarpPlatformDataRequestComplete( struct uarpPlatformAccessory *pAccessory,
                                         struct uarpPlatformAsset *pAsset,
                                         uint8_t reqType, uint32_t payloadTag,
                                         uint32_t offset, uint8_t *pBuffer, uint32_t length )
{
    uint32_t status;
    (void) payloadTag;
    (void) offset;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Data Request Complete; %u bytes at offset %u", length, offset );

    if ( reqType == kUARPDataRequestTypeSuperBinaryHeader )
    {
        pAsset->callbacks.fAssetReady( pAccessory->pDelegate, pAsset->pDelegate );

        status = kUARPStatusSuccess;
    }
    else if ( reqType == kUARPDataRequestTypeSuperBinaryMetadata )
    {
        status = uarpPlatformUpdateSuperBinaryMetaData( pAccessory, pAsset, pBuffer, length );
    }
    else if ( reqType == kUARPDataRequestTypeSuperBinaryPayloadHeader )
    {
        pAsset->callbacks.fPayloadReady( pAccessory->pDelegate, pAsset->pDelegate );

        status = kUARPStatusSuccess;
    }
    else if ( reqType == kUARPDataRequestTypePayloadMetadata )
    {
        status = uarpPlatformUpdatePayloadMetaData( pAccessory, pAsset, pBuffer, length );
    }
    else if ( reqType == kUARPDataRequestTypePayloadPayload )
    {
        status = uarpPlatformUpdatePayloadPayload( pAccessory, pAsset, pBuffer, length );
    }
    else
    {
        status = kUARPStatusInvalidDataRequestType;
    }

    return status;
}

#if !(UARP_DISABLE_CONTROLLER)

UARPAssetSubrange uarpPlatformDataRequestSubtype( uint8_t reqType )
{
    UARPAssetSubrange subrange;

    if ( reqType == kUARPDataRequestTypeSuperBinaryHeader )
    {
        subrange = kUARPAssetSubrangeSuperBinaryHeader;
    }
    else if ( reqType == kUARPDataRequestTypeSuperBinaryMetadata )
    {
        subrange = kUARPAssetSubrangeSuperBinaryMetaData;
    }
    else if ( reqType == kUARPDataRequestTypeSuperBinaryPayloadHeader )
    {
        subrange = kUARPAssetSubrangeSuperBinaryPayloadHeader;
    }
    else if ( reqType == kUARPDataRequestTypePayloadMetadata )
    {
        subrange = kUARPAssetSubrangeSuperBinaryPayloadMetaData;
    }
    else if ( reqType == kUARPDataRequestTypePayloadPayload )
    {
        subrange = kUARPAssetSubrangeSuperBinaryPayloadData;
    }
    else
    {
        subrange = kUARPAssetSubrangeMax;
    }

    return subrange;
}

#endif

uint32_t uarpPlatformQueryAccessoryInfo( void *pAccessoryDelegate, uint32_t infoType, void *pBuffer,
                                        uint32_t lengthBuffer, uint32_t *pLengthNeeded )
{
    uint32_t status;
    struct UARPVersion *pVersion;
    struct UARPLastErrorAction *pLastAction;
    struct uarpPlatformAccessory *pAccessory;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;

    *pLengthNeeded = lengthBuffer;

    switch ( infoType )
    {
        case kUARPTLVAccessoryInformationManufacturerName:

            status = pAccessory->callbacks.fManufacturerName( pAccessory->pDelegate, pBuffer, pLengthNeeded );

            break;

        case kUARPTLVAccessoryInformationModelName:

            status = pAccessory->callbacks.fModelName( pAccessory->pDelegate, pBuffer, pLengthNeeded );

            break;

        case kUARPTLVAccessoryInformationHardwareVersion:

            status = pAccessory->callbacks.fHardwareVersion( pAccessory->pDelegate, pBuffer, pLengthNeeded );

            break;

        case kUARPTLVAccessoryInformationSerialNumber:

            status = pAccessory->callbacks.fSerialNumber( pAccessory->pDelegate, pBuffer, pLengthNeeded );

            break;

        case kUARPTLVAccessoryInformationFirmwareVersion:
        case kUARPTLVAccessoryInformationStagedFirmwareVersion:

            *pLengthNeeded = sizeof( struct UARPVersion );
            if ( *pLengthNeeded > lengthBuffer )
            {
                status = kUARPStatusNoResources;
                break;
            }

            pVersion = (struct UARPVersion *)pBuffer;

            if ( infoType == kUARPTLVAccessoryInformationFirmwareVersion )
            {
                status = pAccessory->callbacks.fActiveFirmwareVersion( pAccessory->pDelegate, 0, pVersion );
            }
            else
            {
                status = pAccessory->callbacks.fStagedFirmwareVersion( pAccessory->pDelegate, 0, pVersion );
            }

            pVersion->major = uarpHtonl( pVersion->major );
            pVersion->minor = uarpHtonl( pVersion->minor );
            pVersion->release = uarpHtonl( pVersion->release );
            pVersion->build = uarpHtonl( pVersion->build );

            break;

        case kUARPTLVAccessoryInformationLastError:

            *pLengthNeeded = (uint32_t)sizeof( struct UARPLastErrorAction );
            if ( *pLengthNeeded > lengthBuffer )
            {
                status = kUARPStatusNoResources;
                break;
            }

            pLastAction = (struct UARPLastErrorAction *)pBuffer;

            status = pAccessory->callbacks.fLastError( pAccessory->pDelegate, pLastAction );

            pLastAction->lastAction = uarpHtonl( pLastAction->lastAction );
            pLastAction->lastError = uarpHtonl( pLastAction->lastError );

            status = kUARPStatusSuccess;
            break;

        default:
            *pLengthNeeded = 0;
            status = kUARPStatusUnknownInformationOption;
            break;
    }

    return status;
}

uint32_t uarpPlatformApplyStagedAssets( void *pAccessoryDelegate, void *pControllerDelegate, uint16_t *pFlags )
{
    uint32_t status;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    status = pAccessory->callbacks.fApplyStagedAssets( pAccessory->pDelegate, pController->pDelegate, pFlags );

    return status;
}

void uarpPlatformAssetRelease( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset )
{
    uarpLogInfo( kUARPLoggingCategoryPlatform, "%s", "Asset Released" );

    if ( pAsset->callbacks.fAssetReleased )
    {
        pAsset->callbacks.fAssetReleased( pAccessory->pDelegate, pAsset->pDelegate );
    }
    pAsset->pDelegate = NULL;

    return;
}

void uarpPlatformAssetOrphan( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset )
{
    uarpLogInfo( kUARPLoggingCategoryPlatform, "Orphan %s Asset <%u>",
                uarpAssetIsSuperBinary( &pAsset->core ) ? "SuperBinary" : "Dynamic ",
                pAsset->core.assetID );

    pAsset->pController = NULL;

    if ( pAsset->callbacks.fAssetOrphaned )
    {
        pAsset->callbacks.fAssetOrphaned( pAccessory->pDelegate, pAsset->pDelegate );
    }

    return;
}

void uarpPlatformAssetRescinded( void *pAccessoryDelegate, void *pControllerDelegate, uint16_t assetID )
{
    struct uarpPlatformAsset *pAsset;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Asset Rescinded from UARP Controller %d <Asset ID %u>",
                 pController->_controller.remoteControllerID, assetID );

    if ( assetID == kUARPAssetIDAllAssets )
    {
        pAsset = NULL;
    }
    else
    {
        pAsset = uarpPlatformAssetFindByAssetID( pAccessory, pController, pAccessory->pRxAssetList, assetID );
        __UARP_Require( pAsset, exit );
    }
        uarpPlatformAssetRescind( pAccessory, pController, pAsset );

exit:
    return;
}

void uarpPlatformAssetRescind( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformController *pController,
                              struct uarpPlatformAsset *pAsset )
{
    void *pAssetDelegate;
    void *pControllerDelegate;

    if ( pAsset )
    {
        pAsset->dataReq.requestType &= ~kUARPDataRequestTypeOutstanding;
        pAsset->internalFlags |= kUARPAssetMarkForCleanup;
        pAssetDelegate = pAsset->pDelegate;
    }
    else
    {
        pAssetDelegate = NULL;
    }

    if ( pController )
    {
        pControllerDelegate = pController->pDelegate;
    }
    else
    {
        pControllerDelegate = NULL;
    }

    if ( pAsset && pAsset->callbacks.fAssetRescinded )
    {
        pAsset->callbacks.fAssetRescinded( pAccessory->pDelegate, pControllerDelegate, pAssetDelegate );
    }
    else if ( pAccessory && pAccessory->callbacks.fRescindAllAssets )
    {
        pAccessory->callbacks.fRescindAllAssets( pAccessory->pDelegate, pControllerDelegate, NULL );
    }
}

UARPBool uarpPlatformAssetIsCookieValid( struct uarpPlatformAccessory *pAccessory, struct uarpPlatformAsset *pAsset,
                                        struct uarpPlatformAssetCookie *pCookie )
{
    uint32_t tag;
    UARPBool isValid;
    UARPVersionComparisonResult versionResult;
    (void) pAccessory;

    tag = uarpTagStructPack32( &pAsset->core.asset4cc );
    if ( pCookie == NULL )
    {
        isValid = kUARPNo;
    }
    else if ( pCookie->assetTag != tag )
    {
        isValid = kUARPNo;
    }
    else if ( pCookie->assetTotalLength != pAsset->core.assetTotalLength )
    {
        isValid = kUARPNo;
    }
    else if ( pCookie->assetNumPayloads != pAsset->core.assetNumPayloads )
    {
        isValid = kUARPNo;
    }
    else if ( pCookie->selectedPayloadIndex >= pAsset->core.assetNumPayloads )
    {
        isValid = kUARPNo;
    }
    else
    {
        versionResult = uarpVersionCompare( &(pCookie->assetVersion), &(pAsset->core.assetVersion) );

        if ( versionResult != kUARPVersionComparisonResultIsEqual )
        {
            isValid = kUARPNo;
        }
        else
        {
            isValid = kUARPYes;
        }
    }

    return isValid;
}

void uarpPlatformProtocolVersion( void *pAccessoryDelegate, uint16_t *pProtocolVersion )
{
    struct uarpPlatformAccessory *pAccessory;

    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;

    if ( pAccessory->_options.protocolVersion == 0 )
    {
        *pProtocolVersion = UARP_PROTOCOL_VERSION;
    }
    else if ( pAccessory->_options.protocolVersion > UARP_PROTOCOL_VERSION )
    {
        *pProtocolVersion = UARP_PROTOCOL_VERSION;
    }
    else
    {
        *pProtocolVersion = pAccessory->_options.protocolVersion;
    }
}

void uarpPlatformGarbageCollection( void *pAccessoryDelegate )
{
    struct uarpPlatformAccessory *pAccessory;

    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;

    uarpPlatformCleanupAssets( pAccessory );
}

uint32_t uarpPlatformAccessoryRemoveAssetPayloadWindow( struct uarpPlatformAccessory *pAccessory,
                                                       struct uarpPlatformAsset *pAsset,
                                                       uint8_t **ppPayloadWindow,
                                                       uint32_t *pPayloadWindowLength )
{
    uint32_t status;
    (void) pAccessory;

    __UARP_Require_Action( ppPayloadWindow, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( pPayloadWindowLength, exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Take Payload Window from Asset ID %u",
                 pAsset->core.assetID );

    *ppPayloadWindow = pAsset->pScratchBuffer;
    pAsset->pScratchBuffer = NULL;

    *pPayloadWindowLength = pAsset->lengthScratchBuffer;
    pAsset->lengthScratchBuffer = 0;

    status = kUARPStatusSuccess;

exit:
    return status;
}

uint32_t uarpPlatformAccessoryProvideAssetPayloadWindow( struct uarpPlatformAccessory *pAccessory,
                                                        struct uarpPlatformAsset *pAsset,
                                                        uint8_t *pPayloadWindow,
                                                        uint32_t payloadWindowLength )
{
    uint32_t status;

    __UARP_Require_Action( pPayloadWindow, exit, status = kUARPStatusInvalidArgument );
    __UARP_Require_Action( ( payloadWindowLength > 0 ), exit, status = kUARPStatusInvalidArgument );

    uarpLogDebug( kUARPLoggingCategoryPlatform, "Give Payload Window to Asset ID %u",
                 pAsset->core.assetID );

    if ( pAsset->pScratchBuffer )
    {
        uarpLogDebug( kUARPLoggingCategoryPlatform, "Resume Asset Transfer, Asset ID %u releasing old payload window",
                     pAsset->core.assetID );

        pAccessory->callbacks.fReturnBuffer( pAccessory->pDelegate, (uint8_t *)pAsset->pScratchBuffer );
    }

    pAsset->pScratchBuffer = pPayloadWindow;
    pAsset->lengthScratchBuffer = payloadWindowLength;

    status = kUARPStatusSuccess;

exit:
    return status;
}

/* MARK: Controller functionality */

#if !(UARP_DISABLE_CONTROLLER)

uint32_t uarpPlatformAssetSolicited( void *pAccessoryDelegate, void *pControllerDelegate, struct UARP4ccTag *pAssetTag )
{
    uint32_t status;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    if ( pAccessory->callbacks.fAssetSolicitation )
    {
        status = pAccessory->callbacks.fAssetSolicitation( pAccessory->pDelegate, pController->pDelegate, pAssetTag );
    }
    else
    {
        status = kUARPStatusInvalidAssetTag;
    }

    return status;
}

static uint32_t uarpPlatformAssetDataResponseMaxPayload( void *pAccessoryDelegate, void *pControllerDelegate,
                                                        uint16_t numBytesRequested, uint16_t *pMaxBytesResponded )
{
    uint32_t status;
    struct uarpPlatformController *pController;
    (void) pAccessoryDelegate;

    /* alias delegates */
    pController = (struct uarpPlatformController *)pControllerDelegate;

    /* determine how much we will maximumly reply */
    if ( numBytesRequested > pController->_options.maxTxPayloadLength )
    {
        *pMaxBytesResponded = pController->_options.maxTxPayloadLength;
    }
    else
    {
        *pMaxBytesResponded = numBytesRequested;
    }

    status = kUARPStatusSuccess;

    return status;
}

uint32_t uarpPlatformAssetDataRequest( void *pAccessoryDelegate, void *pControllerDelegate,
                                      uint16_t assetID, uint16_t numBytesRequested, uint32_t dataOffset,
                                      uint8_t *pBuffer, uint16_t *pNumBytesResponded )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    /* find asset by id */
    pAsset = uarpPlatformAssetFindByAssetID( pAccessory, pController, pAccessory->pTxAssetList, assetID );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusNoResources );

    /* query the layer above for the data buffer */
    if ( pAsset->core.assetTotalLength < dataOffset )
    {
        numBytesRequested = 0;
    }
    else if ( pAsset->core.assetTotalLength < ( dataOffset + (uint32_t)numBytesRequested ) )
    {
        numBytesRequested = (uint16_t)(pAsset->core.assetTotalLength - dataOffset);
    }

    if ( pAsset->callbacks.fAssetGetBytesAtOffset )
    {
        status = pAsset->callbacks.fAssetGetBytesAtOffset( pAccessory->pDelegate, pAsset->pDelegate,
                                                          pBuffer, numBytesRequested, dataOffset,
                                                          pNumBytesResponded );
    }
    else
    {
        status = kUARPStatusUnsupported;
    }

exit:
    return status;
}

uint32_t uarpPlatformAssetProcessingNotification( void *pAccessoryDelegate, void *pControllerDelegate,
                                                 uint16_t assetID, uint16_t assetProcessingFlags )
{
    uint32_t status;
    struct uarpPlatformAsset *pAsset;
    struct uarpPlatformAccessory *pAccessory;
    struct uarpPlatformController *pController;

    /* alias delegates */
    pAccessory = (struct uarpPlatformAccessory *)pAccessoryDelegate;
    pController = (struct uarpPlatformController *)pControllerDelegate;

    /* find asset by id */
    pAsset = uarpPlatformAssetFindByAssetID( pAccessory, pController, pAccessory->pTxAssetList, assetID );
    __UARP_Require_Action( pAsset, exit, status = kUARPStatusNoResources );

    if ( pAsset->callbacks.fAssetProcessingNotification )
    {
        status = pAsset->callbacks.fAssetProcessingNotification( pAccessory->pDelegate, pAsset->pDelegate,
                                                                assetProcessingFlags );
    }
    else
    {
        status = kUARPStatusUnsupported;
    }

exit:
    return status;
}

#endif
