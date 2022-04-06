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

#ifndef CoreUARPUtils_h
#define CoreUARPUtils_h

#include "CoreUARPPlatform.h"
#include "CoreUARPProtocolDefines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    kUARPVersionComparisonResultIsEqual,
    kUARPVersionComparisonResultIsNewer,
    kUARPVersionComparisonResultIsOlder,
    kUARPVersionComparisonResultNotEqual,
    kUARPVersionComparisonResultMax
}
UARPVersionComparisonResult;

typedef enum
{
    kUARPAssetSubrangeSuperBinaryHeader,
    kUARPAssetSubrangeSuperBinaryMetaData,
    kUARPAssetSubrangeSuperBinaryPayloadHeader,
    kUARPAssetSubrangeSuperBinaryPayloadMetaData,
    kUARPAssetSubrangeSuperBinaryPayloadData,
    kUARPAssetSubrangeMax,
}
UARPAssetSubrange;

struct uarpAssetCoreObj
{
    uint16_t assetID;
    uint16_t assetFlags;
    uint32_t assetTag; /* DEPRECATED, please use asset4cc */
    struct UARP4ccTag asset4cc;
    struct UARPVersion assetVersion;
    uint32_t assetTotalLength;
    uint16_t assetNumPayloads;
};

struct uarpDataRequestObj
{
    uint8_t requestType;

    uint32_t payloadTag;
    uint32_t relativeOffset;    // offset to be added to the current offset when filling buffer with chunks
    uint32_t bytesRequested;

    uint8_t *bytes;             // allocated by platform app
    uint32_t bytesResponded;

    // until bytesResponded is equal to bytesRequested
    //      or bytesResponded + absoluteOffset == full length
    uint32_t absoluteOffset;    // offset into the superbinary
    uint32_t currentOffset;     // offset into the payload tag's metadata/payload
    uint32_t bytesRemaining;
};

struct uarpPayloadObj
{
    struct UARPPayloadHeader plHdr;

    uint8_t internalFlags;
    uint8_t vendorFlags;

    uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength];
};

struct uarpPlatformOptionsObj
{
    uint32_t maxTxPayloadLength;
    uint32_t maxRxPayloadLength;
    uint32_t payloadWindowLength;
    uint16_t protocolVersion;
};

struct uarpPlatformTxMsgQueueEntry
{
    void *pMsg;
    uint32_t msgLength;
    struct uarpPlatformTxMsgQueueEntry *pNext;
};

/* These prototypes can be used in verification purposes */

/* -------------------------------------------------------------------------------- */
/*! @brief open a file for superbinary asset transfer verification
    @discussion this routine can be called by an acessory's layer 3 to open a file for transcribing an asset being transferred
    @param pFilename pointer representing the name of the file to be used
    @return context pointer for subsequent calls to the corresponding append / close
 */
/* -------------------------------------------------------------------------------- */
typedef void * (* fcnUarpAssetVerificationSuperBinaryOpen)( char *pFilename );

/* -------------------------------------------------------------------------------- */
/*! @brief append to asset transfer verification file
    @discussion this routine can be called by an acessory's layer 3 to append to a file for transcribing an asset being transferred
    @param pContext pointer to the context returned from fcnUarpAssetVerificationSuperBinaryOpen
    @param subrange clue as to what portion of the asset this buffer represents
    @param pBuffer pointer to the buffer
    @param length length of data in the buffer
    @param offset offset of the pBuffer
 */
/* -------------------------------------------------------------------------------- */
typedef void (* fcnUarpAssetVerificationSuperBinaryAppend)( void *pContext, UARPAssetSubrange subrange,
                                                           uint8_t *pBuffer, uint32_t length, uint32_t offset );

/* -------------------------------------------------------------------------------- */
/*! @brief close the asset transfer verification file
    @discussion this routine can be called by an acessory's layer 3 to close the file used to transcribe an asset being transferred
    @param pContext pointer to the firmware updater delegate's accessory context pointer
 */
/* -------------------------------------------------------------------------------- */
typedef void (* fcnUarpAssetVerificationSuperBinaryClose)( void *pContext );



uint32_t uarpPayloadTagPack( uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] );
void uarpPayloadTagUnpack( uint32_t payloadTag, uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] );

void uarpPayloadTagStructPack( uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength], struct UARP4ccTag *pTag );
void uarpPayloadTagStructUnpack( struct UARP4ccTag *pTag, uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] );

uint32_t uarpTagStructPack32( struct UARP4ccTag *pTag );
void uarpTagStructUnpack32( uint32_t payload4cc, struct UARP4ccTag *pTag );

UARPBool uarp4ccCompare( struct UARP4ccTag *pTag1, struct UARP4ccTag *pTag2 );

UARPBool uarpOuiCompare( struct UARPOUI *pOui1, struct UARPOUI *pOui2 );

/* response is relative to "is the proposed version ... than the existing version?"

    TODO: Description for what to do with an offered Superbinary is coming from a different controller
 
    If offered superbinary is newer than the transferring one...
        - abandon the transferring one
        - accept the new one

    If offered superbinary is older than the transferring one...
        - deny the new one
 */
UARPVersionComparisonResult uarpVersionCompare( struct UARPVersion *pExistingVersion,
                                               struct UARPVersion *pProposedVersion );

UARPVersionComparisonResult uarpAssetCoreCompare( struct uarpAssetCoreObj *pExistingCore,
                                                 struct uarpAssetCoreObj *pProposedCore );

void uarpVersionEndianSwap( struct UARPVersion *pVersion, struct UARPVersion *pVersionSwapped );

UARPBool uarpAssetIsSuperBinary( struct uarpAssetCoreObj *pCore );

UARPBool uarpAssetIsDynamicAsset( struct uarpAssetCoreObj *pCore );

#if !(UARP_DISABLE_LOGS)
const char * uarpStatusCodeToString( uint32_t status );
#endif

#ifdef __cplusplus
}
#endif

#endif /* CoreUARPUtils_h */
