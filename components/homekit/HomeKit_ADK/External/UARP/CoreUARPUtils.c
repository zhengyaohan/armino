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

#include "CoreUARPUtils.h"

uint32_t uarpPayloadTagPack( uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] )
{
    uint32_t payloadTag = 0;
    
    payloadTag |= payload4cc[3];
    payloadTag <<= 8;
    payloadTag |= payload4cc[2];
    payloadTag <<= 8;
    payloadTag |= payload4cc[1];
    payloadTag <<= 8;
    payloadTag |= payload4cc[0];

    return payloadTag;
}

void uarpPayloadTagUnpack( uint32_t payloadTag, uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] )
{
    uint8_t *pTmp;
    
    pTmp = (uint8_t *)&payloadTag;

    if ( payloadTag == 0 )
    {
        payload4cc[0] = '0';
        payload4cc[1] = '0';
        payload4cc[2] = '0';
        payload4cc[3] = '0';
    }
    else
    {
        payload4cc[0] = pTmp[0];
        payload4cc[1] = pTmp[1];
        payload4cc[2] = pTmp[2];
        payload4cc[3] = pTmp[3];
    }
}

void uarpPayloadTagStructPack( uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength], struct UARP4ccTag *pTag )
{
    pTag->char1 = payload4cc[0];
    pTag->char2 = payload4cc[1];
    pTag->char3 = payload4cc[2];
    pTag->char4 = payload4cc[3];
}

void uarpPayloadTagStructUnpack( struct UARP4ccTag *pTag, uint8_t payload4cc[kUARPSuperBinaryPayloadTagLength] )
{
    payload4cc[0] = pTag->char1;
    payload4cc[1] = pTag->char2;
    payload4cc[2] = pTag->char3;
    payload4cc[3] = pTag->char4;
}

uint32_t uarpTagStructPack32( struct UARP4ccTag *pTag )
{
    uint32_t payloadTag = 0;
    
    payloadTag |= pTag->char4;
    payloadTag <<= 8;
    payloadTag |= pTag->char3;
    payloadTag <<= 8;
    payloadTag |= pTag->char2;
    payloadTag <<= 8;
    payloadTag |= pTag->char1;

    return payloadTag;
}

void uarpTagStructUnpack32( uint32_t payloadTag, struct UARP4ccTag *pTag )
{
    uint8_t *pTmp;
    
    pTmp = (uint8_t *)&payloadTag;

    pTag->char1 = pTmp[0];
    pTag->char2 = pTmp[1];
    pTag->char3 = pTmp[2];
    pTag->char4 = pTmp[3];
}

UARPBool uarp4ccCompare( struct UARP4ccTag *pTag1, struct UARP4ccTag *pTag2 )
{
    UARPBool isEqual;

    if ( ( pTag1->char1 != pTag2->char1 ) || ( pTag1->char2 != pTag2->char2 ) ||
         ( pTag1->char3 != pTag2->char3 ) || ( pTag1->char4 != pTag2->char4 ) )
    {
        isEqual = kUARPNo;
    }
    else
    {
        isEqual = kUARPYes;
    }

    return isEqual;
}

UARPBool uarpOuiCompare( struct UARPOUI *pOui1, struct UARPOUI *pOui2 )
{
    UARPBool isEqual;

    if ( ( pOui1->octet1 != pOui2->octet1 ) ||
         ( pOui1->octet2 != pOui2->octet2 ) ||
         ( pOui1->octet3 != pOui2->octet3 ) )
    {
        isEqual = kUARPNo;
    }
    else
    {
        isEqual = kUARPYes;
    }

    return isEqual;
}

UARPVersionComparisonResult uarpVersionCompare( struct UARPVersion *pExistingVersion,
                                               struct UARPVersion *pProposedVersion )
{
    UARPVersionComparisonResult result;
    
    /* compare major versions */
    if ( pExistingVersion->major > pProposedVersion->major )
    {
        result = kUARPVersionComparisonResultIsOlder;
    }
    else if ( pExistingVersion->major < pProposedVersion->major )
    {
        result = kUARPVersionComparisonResultIsNewer;
    }
    /* compare minor versions if major versions are equal */
    else if ( pExistingVersion->minor > pProposedVersion->minor )
    {
        result = kUARPVersionComparisonResultIsOlder;
    }
    else if ( pExistingVersion->minor < pProposedVersion->minor )
    {
        result = kUARPVersionComparisonResultIsNewer;
    }
    /* compare release versions if minor versions are equal */
    else if ( pExistingVersion->release > pProposedVersion->release )
    {
        result = kUARPVersionComparisonResultIsOlder;
    }
    else if ( pExistingVersion->release < pProposedVersion->release )
    {
        result = kUARPVersionComparisonResultIsNewer;
    }
    /* compare build versions if minor versions are equal */
    else if ( pExistingVersion->build > pProposedVersion->build )
    {
        result = kUARPVersionComparisonResultIsOlder;
    }
    else if ( pExistingVersion->build < pProposedVersion->build )
    {
        result = kUARPVersionComparisonResultIsNewer;
    }
    /* if we got here, version is equivalent */
    else
    {
        result = kUARPVersionComparisonResultIsEqual;
    }

    return result;
}

UARPVersionComparisonResult uarpAssetCoreCompare( struct uarpAssetCoreObj *pExistingCore,
                                                 struct uarpAssetCoreObj *pProposedCore )
{
    UARPVersionComparisonResult result;

    if ( pExistingCore->assetFlags != pProposedCore->assetFlags )
    {
        result = kUARPVersionComparisonResultNotEqual;
    }
    else if ( uarp4ccCompare( &pExistingCore->asset4cc, &pProposedCore->asset4cc ) == kUARPNo )
    {
        result = kUARPVersionComparisonResultNotEqual;
    }
    else if ( pExistingCore->assetTotalLength != pProposedCore->assetTotalLength )
    {
        result = kUARPVersionComparisonResultNotEqual;
    }
    else if ( pExistingCore->assetNumPayloads != pProposedCore->assetNumPayloads )
    {
        result = kUARPVersionComparisonResultNotEqual;
    }
    else
    {
        result = uarpVersionCompare( &(pExistingCore->assetVersion), &(pProposedCore->assetVersion) );
    }

    return result;
}

void uarpVersionEndianSwap( struct UARPVersion *pVersion, struct UARPVersion *pVersionSwapped )
{
    /* network to host, host to network, technically irrelevant */
    pVersionSwapped->major = uarpNtohl( pVersion->major );
    pVersionSwapped->minor = uarpNtohl( pVersion->minor );
    pVersionSwapped->release = uarpNtohl( pVersion->release );
    pVersionSwapped->build = uarpNtohl( pVersion->build );
}

UARPBool uarpAssetIsSuperBinary( struct uarpAssetCoreObj *pCore )
{
    UARPBool isSuperBinary;
    
    if ( pCore->assetFlags & kUARPAssetFlagsAssetTypeSuperBinary )
    {
        isSuperBinary = kUARPYes;
    }
    else
    {
        isSuperBinary = kUARPNo;
    }
    
    return isSuperBinary;
}

UARPBool uarpAssetIsDynamicAsset( struct uarpAssetCoreObj *pCore )
{
    UARPBool isDynamicAsset;
    
    if ( pCore->assetFlags & kUARPAssetFlagsAssetTypeDynamic )
    {
        isDynamicAsset = kUARPYes;
    }
    else
    {
        isDynamicAsset = kUARPNo;
    }
    
    return isDynamicAsset;
}

#if !(UARP_DISABLE_LOGS)
const char * uarpStatusCodeToString( uint32_t status )
{
    const char *statusString;
    switch ( status )
    {
        case kUARPStatusSuccess:
            statusString = "success";
            break;
            
        case kUARPStatusUnknownMessageType:
            statusString = "Unknown Message Type";
            break;
            
        case kUARPStatusIncompatibleProtocolVersion:
            statusString = "Incompatible Protocol Version";
            break;
            
        case kUARPStatusInvalidAssetTag:
            statusString = "Invalid Asset Tag";
            break;
            
        case kUARPStatusInvalidDataRequestOffset:
            statusString = "Invalid Data Request Offset";
            break;
            
        case kUARPStatusInvalidDataRequestLength:
            statusString = "Invalid Data Request Length";
            break;
            
        case kUARPStatusMismatchUUID:
            statusString = "Mismatc hUUID";
            break;
            
        case kUARPStatusAssetDataPaused:
            statusString = "Asset Data Paused";
            break;
            
        case kUARPStatusInvalidMessageLength:
            statusString = "Invalid Message Length";
            break;
            
        case kUARPStatusInvalidMessage:
            statusString = "Invalid Message";
            break;
            
        case kUARPStatusInvalidTLV:
            statusString = "Invalid TLV";
            break;
            
        case kUARPStatusNoResources:
            statusString = "No Resources";
            break;
            
        case kUARPStatusUnknownAccessory:
            statusString = "Unknown Accessory";
            break;
            
        case kUARPStatusUnknownController:
            statusString = "Unknown Controller";
            break;
            
        case kUARPStatusInvalidFunctionPointer:
            statusString = "Invalid Function Pointer";
            break;
            
        case kUARPStatusCorruptSuperBinary:
            statusString = "Corrupt SuperBinary";
            break;
            
        case kUARPStatusAssetInFlight:
            statusString = "Asset In Flight";
            break;
            
        case kUARPStatusAssetIdUnknown:
            statusString = "Asset Id Unknown";
            break;
            
        case kUARPStatusInvalidDataResponseLength:
            statusString = "Invalid Data Response Length";
            break;
            
        case kUARPStatusAssetTransferComplete:
            statusString = "Asset Transfer Complete";
            break;
            
        case kUARPStatusUnknownPersonalizationOption:
            statusString = "Unknown Personalization Option";
            break;
            
        case kUARPStatusMismatchPersonalizationOption:
            statusString = "Mismatch Personalization Option";
            break;
            
        case kUARPStatusInvalidAssetType:
            statusString = "Invalid Asset Type";
            break;
            
        case kUARPStatusUnknownAsset:
            statusString = "Unknown Asset";
            break;
            
        case kUARPStatusNoVersionAgreement:
            statusString = "No Version Agreement";
            break;
            
        case kUARPStatusUnknownDataTransferState:
            statusString = "Unknown Data Transfer State";
            break;
            
        case kUARPStatusUnsupported:
            statusString = "Unsupported";
            break;
            
        case kUARPStatusInvalidObject:
            statusString = "Invalid Object";
            break;
            
        case kUARPStatusUnknownInformationOption:
            statusString = "Unknown Information Option";
            break;
            
        case kUARPStatusInvalidDataResponse:
            statusString = "Invalid Data Response";
            break;
            
        case kUARPStatusInvalidArgument:
            statusString = "Invalid Argument";
            break;
            
        case kUARPStatusDataTransferPaused:
            statusString = "Data Transfer Paused";
            break;
            
        case kUARPStatusUnknownPayload:
            statusString = "Unknown Payload";
            break;
            
        case kUARPStatusInvalidDataTransferNotification:
            statusString = "Invalid Data Transfer Notification";
            break;
            
        case kUARPStatusMetaDataTypeNotFound:
            statusString = "MetaData Type Not Found";
            break;
            
        case kUARPStatusMetaDataCorrupt:
            statusString = "MetaData Corrupt";
            break;
            
        case kUARPStatusOutOfOrderMessageID:
            statusString = "Out Of Order Message ID";
            break;
            
        case kUARPStatusDuplicateMessageID:
            statusString = "Duplicate Message ID";
            break;
            
        case kUARPStatusMismatchDataOffset:
            statusString = "Mismatch Data Offset";
            break;
            
        case kUARPStatusInvalidLength:
            statusString = "Invalid Length";
            break;
            
        case kUARPStatusNoMetaData:
            statusString = "No MetaData";
            break;
            
        case kUARPStatusDuplicateController:
            statusString = "Duplicate Controller";
            break;
            
        case kUARPStatusDuplicateAccessory:
            statusString = "Duplicate Accessory";
            break;
            
        case kUARPStatusInvalidOffset:
            statusString = "Invalid Offset";
            break;
            
        case kUARPStatusInvalidPayload:
            statusString = "Invalid Payload";
            break;
            
        case kUARPStatusProcessingIncomplete:
            statusString = "Processing Incomplete";
            break;
            
        case kUARPStatusInvalidDataRequestType:
            statusString = "Invalid Data Request Type";
            break;
            
        case kUARPStatusInvalidSuperBinaryHeader:
            statusString = "Invalid Super Binary Header";
            break;
            
        case kUARPStatusInvalidPayloadHeader:
            statusString = "Invalid Payload Header";
            break;
            
        case kUARPStatusAssetNoBytesRemaining:
            statusString = "Asset No Bytes Remaining";
            break;
            
        case kUARPStatusAssetInvalidCompression:
            statusString = "Asset Invalid Compression";
            break;
            
        case kUARPStatusAssetDecompressionFailure:
            statusString = "Asset Decompression Failure";
            break;
            
        default:
            statusString = "<unknown>";
            break;
    }
    
    return statusString;
}
#endif
