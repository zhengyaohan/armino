/* da_val_specials.c
 *
 * Demo Application for the VAL API Specials that cannot be tested via the
 * test program
 */

/*****************************************************************************
* Copyright (c) 2016-2019 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

/*----------------------------------------------------------------------------
 * This module implements (provides) the following interface(s):
 */

#include "da_val_specials.h"            // da_val_specials_main()


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_da_val_specials.h"          // configuration

#include "log.h"
#include "api_driver_init.h"
#include "api_val.h"

#include <stdlib.h>


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define DA_PREFIX   "DA_VAL_SPEC: "

// eMMC Message types
typedef enum
{
    // Requests
    EMMC_MSGTYP_REQ_AUTH_KEY_PROG = 1,
    EMMC_MSGTYP_REQ_RD_WCNTR      = 2,
    EMMC_MSGTYP_REQ_AUTH_DATA_WR  = 3,
    EMMC_MSGTYP_REQ_AUTH_DATA_RD  = 4,
    EMMC_MSGTYP_REQ_RESULT_RD     = 5,

    // Responses
    EMMC_MSGTYP_RSP_AUTH_KEY_PROG = 0x100,
    EMMC_MSGTYP_RSP_RD_WCNTR      = 0x200,
    EMMC_MSGTYP_RSP_AUTH_DATA_WR  = 0x300,
    EMMC_MSGTYP_RSP_AUTH_DATA_RD  = 0x400,
} eMMC_MsgTypes;

typedef struct
{
    // Param 0: Decryption counter
    uint8_t DecCounterLen;
    uint8_t DecCounterZero1;
    uint8_t DecCounterPolicy[4];
    uint8_t DecCounterZero2[4];
    uint8_t DecCounterData[4];
    // Param 1: Encryption counter
    uint8_t EncCounterLen;
    uint8_t EncCounterZero1;
    uint8_t EncCounterPolicy[4];
    uint8_t EncCounterZero2[4];
    uint8_t EncCounterData[4];
    // Param 2: eMMC address
    uint8_t eMmcAddrLen;
    uint8_t eMmcAddrZero1;
    uint8_t eMmcAddrPolicy[4];
    uint8_t eMmcAddrZero2[4];
    uint8_t eMmcAddrData[2];
    // Param 3: Associated Data Tail
    uint8_t ADTailLen;
    uint8_t ADTailZero1;
    uint8_t ADTailPolicy[4];
    uint8_t ADTailZero2[4];
    uint8_t ADTailData[48];
    uint8_t EndMarker;
} eMMC_HibernationData;


/*----------------------------------------------------------------------------
 * Local variables
 */

// Hardware Unique Key (HUK)
static const uint8_t gl_ADLabel[] = "SomeAssociatedDataForProvisioningWithKeyBlob";
static const uint8_t gl_HUK_OTPKeyBlob[] =
{
    0x18, 0x6E, 0x2E, 0xA2, 0x32, 0x09, 0x5B, 0x4A, 0x17, 0xCD, 0xA0, 0xDA, 0x8C, 0xB8, 0x88, 0xED,
    0x2B, 0x33, 0x8A, 0x33, 0xE6, 0x35, 0xC9, 0x8B, 0x20, 0x24, 0x3B, 0x44, 0x5B, 0x39, 0x4B, 0xDD,
    0x98, 0x11, 0x37, 0xF0, 0x96, 0x20, 0xB3, 0x34, 0x6E, 0xC4, 0xDE, 0xCB, 0xC4, 0x34, 0x53, 0x63,
};

// eMMC Authentication Key (AuthKey)
static const uint8_t gl_eMMCAuthKey[] =
{
    0xC8, 0x4D, 0x7A, 0x95, 0xE8, 0x81, 0x48, 0xC1, 0x9E, 0x40, 0xE8, 0xFB, 0xCF, 0xE6, 0x4F, 0xBA,
    0xE6, 0xAF, 0x78, 0x19, 0x6F, 0x9C, 0xE9, 0xF7, 0x7A, 0xDD, 0x42, 0xCE, 0x8C, 0x03, 0xB8, 0x66
};
static const uint8_t gl_eMMCAuthKey_OTPKeyBlob[] =
{
    0x64, 0x0E, 0xCA, 0xDD, 0xCD, 0x59, 0xD1, 0x30, 0x6C, 0x1A, 0x9F, 0x06, 0x16, 0x73, 0x04, 0x71,
    0x24, 0x48, 0x24, 0x1F, 0x90, 0xE9, 0x1C, 0x8C, 0x40, 0xCD, 0xBC, 0x67, 0xE8, 0x11, 0xF6, 0xC0,
    0x59, 0x45, 0x7B, 0x1E, 0xCA, 0xA3, 0xB3, 0x5B, 0x72, 0x84, 0xE8, 0x5A, 0x65, 0x02, 0xAD, 0x81
};
static const uint8_t gl_eMMCAuthKey_AssetNumber = 12;

// eMMC Policies
//static const uint8_t gl_eMMC_PolicyZero[4]      = { 0, 0, 0, 0 };    // empty
static const uint8_t gl_eMMC_PolicyConstant[4]  = { 1, 2, 2, 0 };    // public, service, constant
static const uint8_t gl_eMMC_PolicyMonCntBI[4]  = { 1, 1, 0x09, 0 }; // Monotonic counter, binary inc, without max
//static const uint8_t gl_eMMC_PolicyMonCntBIM[4] = { 1, 1, 0x19, 0 }; // Monotonic counter, binary inc, with max

// eMMC addresses
static const uint8_t gl_eMMCAddr_SequenceChk = 0;
static const uint8_t gl_eMMCAddr_Hibernation = 5;

// eMMC Memory Model
static uint32_t gl_eMMCModelWriteCounter = 0;
static uint8_t gl_eMMCModelNonce[16] = { 0 };
#define DA_EMMC_MODEL_MEMORY_ENTRIES  6
static uint8_t gl_eMMCModelMemory[DA_EMMC_MODEL_MEMORY_ENTRIES][256];

static uint8_t gl_EmptyMac[1] = { 0 };

static uint16_t gl_SampleCycles = 3072;

static bool gl_HibernationDone = false;
static uint8_t gl_DataBlobBuffer[(128 / 8) + 1024];
static ValSize_t gl_DataBlobSize;

// Milenage Related information
static const uint8_t gl_Milenage_AssetNumber = 14;

/* Test Set1 */
static const uint8_t gl_Milenage_TS1_OTPBlob[] = {
    0xA0, 0xF1, 0xC6, 0x1D, 0x26, 0x86, 0x33, 0x7A, 0xE3, 0xF4, 0x6D, 0x6E, 0xBB, 0x4C, 0xEB, 0x7F,
    0xBD, 0x66, 0xDA, 0x81, 0x38, 0xA1, 0x46, 0x01, 0x97, 0x0C, 0xAC, 0xAC, 0x9C, 0x67, 0x90, 0x98,
    0x25, 0xC6, 0x98, 0x15, 0xD9, 0xDA, 0x4D, 0x09, 0x50, 0x3D, 0xE6, 0x7C, 0x08, 0x33, 0x39, 0xDE
};
static const uint8_t gl_Milenage_TS1_RAND[] = {
    0x23, 0x55, 0x3C, 0xBE, 0x96, 0x37, 0xA8, 0x9D, 0x21, 0x8A, 0xE6, 0x4D, 0xAE, 0x47, 0xBF, 0x35
};
static const uint8_t gl_Milenage_TS1_SQN[] = {
    0xFF, 0x9B, 0xB4, 0xD0, 0xB6, 0x07
};
static const uint8_t gl_Milenage_TS1_AMF[] = {
    0xB9, 0xB9
};
static const uint8_t gl_Milenage_TS1_f1[] = {
    0x4A, 0x9F, 0xFA, 0xC3, 0x54, 0xDF, 0xAF, 0xB3
};
static const uint8_t gl_Milenage_TS1_f1star[] = {
    0x01, 0xCF, 0xAF, 0x9E, 0xC4, 0xE8, 0x71, 0xE9
};
static const uint8_t gl_Milenage_TS1_f2[] = {
    0xA5, 0x42, 0x11, 0xD5, 0xE3, 0xBA, 0x50, 0xBF
};
static const uint8_t gl_Milenage_TS1_f3[] = {
    0xB4, 0x0B, 0xA9, 0xA3, 0xC5, 0x8B, 0x2A, 0x05, 0xBB, 0xF0, 0xD9, 0x87, 0xB2, 0x1B, 0xF8, 0xCB
};
static const uint8_t gl_Milenage_TS1_f4[] = {
    0xF7, 0x69, 0xBC, 0xD7, 0x51, 0x04, 0x46, 0x04, 0x12, 0x76, 0x72, 0x71, 0x1C, 0x6D, 0x34, 0x41
};
static const uint8_t gl_Milenage_TS1_f5[] = {
    0xAA, 0x68, 0x9C, 0x64, 0x83, 0x70
};
static const uint8_t gl_Milenage_TS1_f5star[] = {
    0x45, 0x1E, 0x8B, 0xEC, 0xA4, 0x3B
};


/*----------------------------------------------------------------------------
 * DoReset
 *
 * This function performs a firmware based reset.
 */
static ValStatus_t
DoReset(void)
{
    ValStatus_t FuncRes;

    FuncRes = val_SystemReset();
    if (FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "PASSED: Reset system.\n");
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SystemReset()=%d.\n", (int)FuncRes);
    }
    return FuncRes;
}


/*----------------------------------------------------------------------------
 * WriteOTP
 *
 * This function writes an item (KeyBlob) to OTP.
 */
static ValStatus_t
WriteOTP(
        const ValAssetNumber_t StaticAssetNumber,
        const ValAssetNumber_t AssetPolicyNumber,
        const bool fAddCRC,
        ValOctetsIn_t * const AssociatedData_p,
        const ValSize_t AssociatedDataSize,
        ValOctetsIn_t * const KeyBlob_p,
        const ValSize_t KeyBlobSize)
{
    ValStatus_t FuncRes;

    FuncRes = val_OTPDataWrite(StaticAssetNumber, AssetPolicyNumber, fAddCRC,
                               AssociatedData_p, AssociatedDataSize,
                               KeyBlob_p, KeyBlobSize);
    if (FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "PASSED: OTP data written.\n");
        LOG_CRIT(DA_PREFIX "Reset system to enable the information stored in OTP\n");
        FuncRes = DoReset();
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_OTPDataWrite()=%d.\n", (int)FuncRes);
    }
    return FuncRes;
}


/*----------------------------------------------------------------------------
 * WriteRandomHuk
 *
 * This function writes a randomly generated HUK and default COID to OTP.
 */
static ValStatus_t
WriteRandomHuk(
        const ValAssetNumber_t StaticAssetNumber,
        const bool f128bit,
        const bool fAddCRC,
        char * const SampleCyclesInfo)
{
    ValStatus_t FuncRes;
    ValSize_t KeyBlobSize = 0;
    uint8_t OTPKeyBlobBuffer[(128+256)/8];
    uint8_t * KeyBlob_p = NULL;

    if (SampleCyclesInfo[0] == '=')
    {
        gl_SampleCycles = (uint16_t)atoi(&SampleCyclesInfo[1]);
        if (gl_SampleCycles == 0)
        {
            gl_SampleCycles = 2;
        }
    }

    if (!f128bit)
    {
        KeyBlob_p = OTPKeyBlobBuffer;
        KeyBlobSize = sizeof(OTPKeyBlobBuffer);
        memset(OTPKeyBlobBuffer, 0, KeyBlobSize);
    }

    FuncRes = val_ProvisionRandomRootKey(0x4F5A3647, StaticAssetNumber,
                                         f128bit, fAddCRC,
                                         0, gl_SampleCycles, 1, 1,
                                         gl_ADLabel, sizeof(gl_ADLabel)-1,
                                         KeyBlob_p, &KeyBlobSize);
    if (FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "PASSED: Random HUK written.\n");

        if (KeyBlob_p != NULL)
        {
            Log_HexDump("OTPKeyBlob(HUK)", 0, KeyBlob_p, (unsigned int)KeyBlobSize);
        }

        LOG_CRIT(DA_PREFIX "Reset system to enable the information stored in OTP\n");
        FuncRes = DoReset();
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_ProvisionRandomRootKey()=%d.\n", (int)FuncRes);
    }
    return FuncRes;
}


/*----------------------------------------------------------------------------
 * ConfigureTrng
 *
 * This function configures and actives the TRNG.
 */
static ValStatus_t
ConfigureTrng(void)
{
    ValStatus_t FuncRes;

    LOG_CRIT(DA_PREFIX "Configure TRNG (%u)\n", gl_SampleCycles);
    FuncRes = val_TrngConfig(0, gl_SampleCycles, 1, 2, 1);
    if(FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "PASSED: Configure TRNG\n");
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_TrngConfig()=%d.\n", (int)FuncRes);
    }
    return FuncRes;
}


/*----------------------------------------------------------------------------
 * GetVersion
 *
 * This function configures and actives the TRNG.
 */
static ValStatus_t
GetVersion(void)
{
    ValOctetsOut_t DataOut_p[100];
    ValSize_t DataOutByteCount = 100;
    ValStatus_t FuncRes;

    LOG_CRIT(DA_PREFIX "Get version information as a sanity check\n");
    FuncRes = val_SystemGetVersion(DataOut_p, &DataOutByteCount);
    if(FuncRes == VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "PASSED: Version: %s\n", DataOut_p);
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SystemGetVersion()=%d\n", FuncRes);
    }
    return FuncRes;
}


/*----------------------------------------------------------------------------
 * eMMCModelCreatePacket
 *
 * This function creates an eMMC packet based on the eMMC Model.
 */
static int
eMMCModelCreatePacket(
        uint16_t aMsgType,
        uint8_t * aPktBuffer_p,
        uint32_t aPktBufferSize,
        uint16_t aResult,
        uint16_t aBlockCount,
        uint16_t aAddress,
        uint8_t * aData_p,
        uint32_t aDataSize)
{
    uint8_t * Packet_p;
    uint16_t Result = 0;
    uint16_t BlockCount = 0;
    uint16_t Address = 0;
    uint32_t WriteCounter = 0;
    uint8_t * Nonce_p = NULL;
    uint8_t * Data_p = NULL;
    ValSize_t DataSize = 0;
    uint8_t * Mac_p = NULL;
    ValSize_t MacSize = 0;

    if ((aPktBuffer_p == NULL) || (aPktBufferSize < 512))
    {
        LOG_CRIT(DA_PREFIX "FAILED: Packet buffer too small\n");
        return -1;
    }

    switch (aMsgType)
    {
    default:
    case EMMC_MSGTYP_REQ_AUTH_KEY_PROG:
    case EMMC_MSGTYP_RSP_AUTH_KEY_PROG:
        LOG_CRIT(DA_PREFIX "FAILED: Operation not defined at the moment\n");
        return -2;

    case EMMC_MSGTYP_REQ_RD_WCNTR:
        Nonce_p = gl_eMMCModelNonce;
        Mac_p = gl_EmptyMac;
        MacSize = sizeof(gl_EmptyMac);
        break;

    case EMMC_MSGTYP_RSP_RD_WCNTR:
        Result = aResult;
        WriteCounter = gl_eMMCModelWriteCounter;
        Nonce_p = gl_eMMCModelNonce;
        break;

    case EMMC_MSGTYP_REQ_AUTH_DATA_WR:
        BlockCount = aBlockCount;
        Address = aAddress;
        WriteCounter = gl_eMMCModelWriteCounter;
        Data_p = aData_p;
        DataSize = aDataSize;
        break;

    case EMMC_MSGTYP_RSP_AUTH_DATA_WR:
        Result = aResult;
        Address = aAddress;
        WriteCounter = gl_eMMCModelWriteCounter;
        break;

    case EMMC_MSGTYP_REQ_AUTH_DATA_RD:
        Address = aAddress;
        Nonce_p = gl_eMMCModelNonce;
        Mac_p = gl_EmptyMac;
        MacSize = sizeof(gl_EmptyMac);
        break;

    case EMMC_MSGTYP_RSP_AUTH_DATA_RD:
        Result = aResult;
        BlockCount = aBlockCount;
        Address = aAddress;
        Nonce_p = gl_eMMCModelNonce;
        Data_p = aData_p;
        DataSize = aDataSize;
        break;

    case EMMC_MSGTYP_REQ_RESULT_RD:
        Mac_p = gl_EmptyMac;
        MacSize = sizeof(gl_EmptyMac);
        break;
    }

    // Assembly packet
    Packet_p = aPktBuffer_p;

    // + Message type
    *Packet_p++ = (uint8_t)(aMsgType >> 0);
    *Packet_p++ = (uint8_t)(aMsgType >> 8);

    // + Result
    *Packet_p++ = (uint8_t)(Result >> 0);
    *Packet_p++ = (uint8_t)(Result >> 8);

    // + BlockCount
    *Packet_p++ = (uint8_t)(BlockCount >> 0);
    *Packet_p++ = (uint8_t)(BlockCount >> 8);

    // + Address
    *Packet_p++ = (uint8_t)(Address >> 0);
    *Packet_p++ = (uint8_t)(Address >> 8);

    // + WriteCounter
    *Packet_p++ = (uint8_t)(WriteCounter >> 0);
    *Packet_p++ = (uint8_t)(WriteCounter >> 8);
    *Packet_p++ = (uint8_t)(WriteCounter >> 16);
    *Packet_p++ = (uint8_t)(WriteCounter >> 24);

    // + Nonce
    if (Nonce_p == NULL)
    {
        memset(Packet_p, 0, 16);
    }
    else
    {
        memcpy(Packet_p, Nonce_p, 16);
    }
    Packet_p += 16;

    // + Data
    if (Data_p == NULL)
    {
        memset(Packet_p, 0, 256);
    }
    else
    {
        memcpy(Packet_p, Data_p, DataSize);
        if (DataSize != 256)
        {
            memset((Packet_p + DataSize), 0, (256 - DataSize));
        }
    }
    Packet_p += 256;

    // + MAC
    if (Mac_p == NULL)
    {
        ValSymContextPtr_t SymContext_p = NULL;
        ValStatus_t Status;

        Status = val_SymAlloc(VAL_SYM_ALGO_MAC_HMAC_SHA256, VAL_SYM_MODE_NONE,
                              false, &SymContext_p);
        if (Status != VAL_SUCCESS)
        {
            LOG_CRIT(DA_PREFIX "FAILED: val_SymAlloc()=%d.\n", (int)Status);
            return -3;
        }

        Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                                gl_eMMCAuthKey, sizeof(gl_eMMCAuthKey));
        if (Status != VAL_SUCCESS)
        {
            (void)val_SymRelease(SymContext_p);
            LOG_CRIT(DA_PREFIX "FAILED: val_SymInitKey()=%d.\n", (int)Status);
            return -3;
        }

        MacSize = (256 / 8);
        Status = val_SymMacGenerate(SymContext_p,
                                    aPktBuffer_p,(ValSize_t)(Packet_p - aPktBuffer_p),
                                    Packet_p, &MacSize);
        if (Status != VAL_SUCCESS)
        {
            (void)val_SymRelease(SymContext_p);
            LOG_CRIT(DA_PREFIX "FAILED: val_SymMacGenerate()=%d.\n", (int)Status);
            return -3;
        }
    }
    else
    {
        memcpy(Packet_p, Mac_p, MacSize);
    }
    if (MacSize != 32)
    {
        memset((Packet_p + MacSize), 0, (32 - MacSize));
    }
    Packet_p += 32;

    // Pad/Stuff remaing packet
    memset(Packet_p, 0, 196);

    return 0;
}


/*----------------------------------------------------------------------------
 * eMMCModelHandleRequest
 *
 * This function handles an eMMC request packet and generates an response
 * based on the eMMC Model.
 */
static int
eMMCModelHandleRequest(
        uint8_t * aPktBuffer_p,
        uint32_t aPktBufferSize,
        uint8_t * aResponseBuffer_p,
        uint32_t aResponseBufferSize,
        uint16_t aResult)
{
    int rc = 0;
    uint16_t MsgType = 0;
    uint16_t Result = 0;
    uint16_t Address;
    uint32_t WriteCounter = 0;
    uint8_t * Data_p = NULL;
    uint32_t DataSize = 0;

    if ((aPktBuffer_p == NULL) || (aPktBufferSize < 512) ||
        (aResponseBuffer_p == NULL) || (aResponseBufferSize < 512))
    {
        LOG_CRIT(DA_PREFIX "FAILED: Packet/Response buffer too small\n");
        return -11;
    }

    if (aResult != 0xFFFF)
    {
        Result = aResult;
    }

    MsgType = (uint16_t)((aPktBuffer_p[1] << 8) + aPktBuffer_p[0]);
    switch (MsgType)
    {
    default:
        LOG_CRIT(DA_PREFIX "FAILED: Invalid request (%d).\n", (int)MsgType);
        return -12;

    case EMMC_MSGTYP_REQ_RD_WCNTR:
        memcpy(gl_eMMCModelNonce, &aPktBuffer_p[12], 16);
        rc = eMMCModelCreatePacket(EMMC_MSGTYP_RSP_RD_WCNTR,
                                   aResponseBuffer_p, aResponseBufferSize,
                                   Result, 1, 0,
                                   NULL, 0);
        break;

    case EMMC_MSGTYP_REQ_AUTH_DATA_WR:
        Address = (uint16_t)((aPktBuffer_p[7] << 8) + (aPktBuffer_p[6] << 0));
        WriteCounter = (uint32_t)((aPktBuffer_p[11] << 24) +
                                  (aPktBuffer_p[10] << 16) +
                                  (aPktBuffer_p[9]  <<  8) +
                                  (aPktBuffer_p[8]  <<  0));
        if (gl_eMMCModelWriteCounter == 0xFFFFFFFF)
        {
            LOG_CRIT(DA_PREFIX "WARNING: eMMC Model - Write counter expired\n");
            if (aResult == 0xFFFF)
            {
                Result = 0x85;          // Write failure
            }
        }
        else if (WriteCounter == gl_eMMCModelWriteCounter)
        {
            if ((aResult == 0) || (aResult == 0xFFFF))
            {
                if (Address < DA_EMMC_MODEL_MEMORY_ENTRIES)
                {
                    memcpy(gl_eMMCModelMemory[Address], &aPktBuffer_p[28], 256);
                }
                else
                {
                    Result = 0x4;           // Address failure
                }
            }
            gl_eMMCModelWriteCounter += 1;
        }
        else
        {
            LOG_CRIT(DA_PREFIX "WARNING: eMMC Model - Write counter expired\n");
            if (aResult != 0x3)
            {
                LOG_CRIT(DA_PREFIX "FAILED: eMMC Model - Write counter mismatch (%d != %d)\n",
                         WriteCounter, gl_eMMCModelWriteCounter);
            }
            else
            {
                LOG_CRIT(DA_PREFIX "WARNING: eMMC Model - Write counter does not match (%d != %d)\n",
                         WriteCounter, gl_eMMCModelWriteCounter);
            }
            if (aResult == 0xFFFF)
            {
                Result = 0x3;           // Write failure
            }
        }
        rc = eMMCModelCreatePacket(EMMC_MSGTYP_RSP_AUTH_DATA_WR,
                                   aResponseBuffer_p, aResponseBufferSize,
                                   Result, 0, Address,
                                   NULL, 0);
        break;

    case EMMC_MSGTYP_REQ_AUTH_DATA_RD:
        Address = (uint16_t)((aPktBuffer_p[7] << 8) + aPktBuffer_p[6]);
        if ((aResult == 0) || (aResult == 0xFFFF))
        {
            if (Address < DA_EMMC_MODEL_MEMORY_ENTRIES)
            {
                Data_p = gl_eMMCModelMemory[Address];
                DataSize = 256;
            }
        }
        rc = eMMCModelCreatePacket(EMMC_MSGTYP_RSP_AUTH_DATA_RD,
                                   aResponseBuffer_p, aResponseBufferSize,
                                   Result, 1, Address,
                                   Data_p, DataSize);
        break;
    }

    return rc;
}

/*----------------------------------------------------------------------------
 * IncrementCounter
 *
 * This function increments a counter.
 */
static void
IncrementCounter(
        uint8_t * CounterData_p,
        ValSize_t Size)
{
    unsigned int i;

    for (i = 0; i < Size; i++)
    {
        CounterData_p[i] = (uint8_t)(CounterData_p[i] + 1);
        if (CounterData_p[i] != 0)
        {
            break;
        }
    }
}


/*----------------------------------------------------------------------------
 * eMMCCheckSequence
 *
 * This function checks the eMMC sequence and uses the eMMC Model as reference.
 */
static ValStatus_t
eMMCCheckSequence(void)
{
    ValStatus_t FuncRes;
    ValAssetId_t AuthKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t StateAssetId = VAL_ASSETID_INVALID;
    uint8_t PacketBuffer[512];
    uint8_t ResponseBuffer[512];
    uint8_t * Data_p;
    ValSize_t Size;
    int rc;

    LOG_CRIT(DA_PREFIX "eMMC functions/sequence check\n");

    // Make sure that the TRNG is configured
    FuncRes = ConfigureTrng();
    if(FuncRes != VAL_SUCCESS)
    {
        return FuncRes;
    }

    // Get Authentication Key Asset ID
    FuncRes = val_AssetSearch(gl_eMMCAuthKey_AssetNumber, &AuthKeyAssetId, NULL);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetSearch(Authentication Key)=%d\n", (int)FuncRes);
        LOG_CRIT(DA_PREFIX "        Use '-emmcauthkey' to write it to OTP.\n");
        return FuncRes;
    }

    // eMMC read - get State Asset ID and Nonce
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadRequest(AuthKeyAssetId, &StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read verify - Get/Initialize eMMC item data
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_RD,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_SequenceChk, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadVerify(StateAssetId,
                                 ResponseBuffer, (28 + 256),
                                 &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read write counter request
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadWriteCounterRequest(StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read write counter verify - Get/Initialize write counter value
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_RD_WCNTR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 0, 0, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadWriteCounterVerify(StateAssetId,
                                             ResponseBuffer, (28 + 256),
                                             &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC write request
    Data_p = gl_eMMCModelMemory[0];
    if (*Data_p)                    // Check parameter length
    {
        // Increment four byte counter
        IncrementCounter(&Data_p[10], 4);
    }
    else
    {
        // Set parameter length to 4 (four byte counter) and initialize it to 0
        memset(Data_p, 0, 256);
        *Data_p = 4;
    }
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_WR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_SequenceChk, Data_p, 256);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    Size = (256 / 8);
    FuncRes = val_eMMCWriteRequest(StateAssetId,
                                   PacketBuffer, (28 + 256),
                                   &PacketBuffer[(28 + 256)], &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC write verify
    FuncRes = val_eMMCWriteVerify(StateAssetId,
                                  ResponseBuffer, (28 + 256),
                                  &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // State Asset is not valid anymore, so delete it
    FuncRes = val_AssetFree(StateAssetId);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetFree(StateAssetId)=%d\n", (int)FuncRes);
        return FuncRes;
    }
    StateAssetId =  VAL_ASSETID_INVALID;

    // eMMC read - get State Asset ID and Nonce
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadRequest(AuthKeyAssetId, &StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadRequest(2)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read verify
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_RD,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 0, gl_eMMCAddr_SequenceChk, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadVerify(StateAssetId,
                                 ResponseBuffer, (28 + 256),
                                 &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadVerify(2)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // State Asset is not needed anymore, so delete it
    FuncRes = val_AssetFree(StateAssetId);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetFree(StateAssetId2)=%d\n", (int)FuncRes);
    }

    return FuncRes;
}


/*----------------------------------------------------------------------------
 * Hibernation
 *
 * This function performs the sequence to go to hibernation mode.
 */
static ValStatus_t
Hibernation(void)
{
    ValStatus_t FuncRes = VAL_SUCCESS;
    ValAssetId_t AuthKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t StateAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KekAssetId = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    eMMC_HibernationData HiberationData;
    uint8_t PacketBuffer[512];
    uint8_t ResponseBuffer[512];
    ValSize_t Size;
    int rc;
    eMMC_HibernationData * HiberData_p =
           (eMMC_HibernationData *)gl_eMMCModelMemory[gl_eMMCAddr_Hibernation];

    LOG_CRIT(DA_PREFIX "Hibernation sequence check\n");

    if (gl_HibernationDone)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Please, perform Resume from Hibernation first\n");
        return VAL_INVALID_STATE;
    }

    if (HiberData_p->DecCounterLen == 0)
    {
        // Initialize hibernation data
        HiberData_p->DecCounterLen = 4;
        memcpy(HiberData_p->DecCounterPolicy, gl_eMMC_PolicyMonCntBI, sizeof(HiberData_p->DecCounterPolicy));
        HiberData_p->EncCounterLen = 4;
        memcpy(HiberData_p->EncCounterPolicy, gl_eMMC_PolicyMonCntBI, sizeof(HiberData_p->EncCounterPolicy));
        HiberData_p->eMmcAddrLen = 2;
        memcpy(HiberData_p->eMmcAddrPolicy, gl_eMMC_PolicyConstant, sizeof(HiberData_p->eMmcAddrPolicy));
        HiberData_p->eMmcAddrData[0] = gl_eMMCAddr_Hibernation;
        HiberData_p->ADTailLen = 48;
        memcpy(HiberData_p->ADTailPolicy, gl_eMMC_PolicyConstant, sizeof(HiberData_p->ADTailPolicy));
        memset(HiberData_p->ADTailData, 'a', sizeof(HiberData_p->ADTailData)); // some data
        HiberData_p->EndMarker = 0xFF;
    }

    // Make sure that the TRNG is configured and active.
    FuncRes = ConfigureTrng();
    if(FuncRes != VAL_SUCCESS)
    {
        return FuncRes;
    }

    // Get Authentication Key Asset ID
    FuncRes = val_AssetSearch(gl_eMMCAuthKey_AssetNumber, &AuthKeyAssetId, NULL);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetSearch(Authentication Key)=%d\n", (int)FuncRes);
        LOG_CRIT(DA_PREFIX "        Use '-emmcauthkey' to write it to OTP.\n");
        return FuncRes;
    }

    // eMMC read - get State Asset ID and Nonce
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadRequest(AuthKeyAssetId, &StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read verify - Get/Initialize eMMC item data
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_RD,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_Hibernation, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadVerify(StateAssetId,
                                 ResponseBuffer, (28 + 256),
                                 &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Copy eMMC hibernation data
    memcpy(&HiberationData, &ResponseBuffer[28], sizeof(HiberationData));

    // eMMC Read write counter request
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadWriteCounterRequest(StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read write counter verify - Get/Initialize write counter value
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_RD_WCNTR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 0, 0, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadWriteCounterVerify(StateAssetId,
                                             ResponseBuffer, (28 + 256),
                                             &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Increment the encryption monotonic counter and write it
    IncrementCounter(HiberationData.EncCounterData, HiberationData.EncCounterLen);

    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_WR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_Hibernation,
                               (uint8_t *)&HiberationData, sizeof(HiberationData));
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    Size = (256 / 8);
    FuncRes = val_eMMCWriteRequest(StateAssetId,
                                   PacketBuffer, (28 + 256),
                                   &PacketBuffer[(28 + 256)], &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC write verify
    FuncRes = val_eMMCWriteVerify(StateAssetId,
                                  ResponseBuffer, (28 + 256),
                                  &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Create Trusted Key Encryption Key (KEK) Asset
    AssetPolicy = VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT | VAL_POLICY_TRUSTED_WRAP;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    FuncRes = val_AssetAlloc(AssetPolicy, (512 / 8),
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             &KekAssetId);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetAlloc(KEK)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Derive KEK from the HUK
    memcpy(PacketBuffer,
           HiberationData.EncCounterData, HiberationData.EncCounterLen);
    Size = HiberationData.EncCounterLen;
    memcpy(&PacketBuffer[Size],
           HiberationData.eMmcAddrData, HiberationData.eMmcAddrLen);
    Size += HiberationData.eMmcAddrLen;
    memcpy(&PacketBuffer[Size],
           HiberationData.ADTailData, HiberationData.ADTailLen);
    Size += HiberationData.ADTailLen;
    FuncRes = val_AssetLoadDerive(KekAssetId, val_AssetGetRootKey(),
                                  PacketBuffer, Size,
                                  false, false, NULL, 0, NULL, 0);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetLoadDerive(KEK)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Goto hibernation mode
    gl_DataBlobSize = sizeof(gl_DataBlobBuffer);
    FuncRes = val_SystemHibernation(StateAssetId, KekAssetId,
                                    PacketBuffer, Size,
                                    gl_DataBlobBuffer, &gl_DataBlobSize);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SystemHibernation()=%d\n", (int)FuncRes);
    }
    else
    {
        gl_HibernationDone = true;
        LOG_CRIT(DA_PREFIX "PASSED: Hardware is in Hibernation mode\n");

        LOG_CRIT(DA_PREFIX "Reset system to ensure hardware is in an initial state\n");
        FuncRes = DoReset();
    }

    return FuncRes;
}


/*----------------------------------------------------------------------------
 * ResumeFromHibernation
 *
 * This function performs the sequence to resume from hibernation mode.
 */
static ValStatus_t
ResumeFromHibernation(void)
{
    ValStatus_t FuncRes = VAL_SUCCESS;
    ValAssetId_t AuthKeyAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t StateAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KekAssetId = VAL_ASSETID_INVALID;
    ValPolicyMask_t AssetPolicy;
    eMMC_HibernationData HiberationData;
    uint8_t PacketBuffer[512];
    uint8_t ResponseBuffer[512];
    ValSize_t Size;
    int rc;

    LOG_CRIT(DA_PREFIX "Resume From Hibernation sequence check\n");

    if (!gl_HibernationDone)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Please, perform Hibernation first\n");
        return VAL_INVALID_STATE;
    }

    // Make sure that the TRNG is configured and active.
    FuncRes = ConfigureTrng();
    if(FuncRes != VAL_SUCCESS)
    {
        return FuncRes;
    }

    // Get Authentication Key Asset ID
    FuncRes = val_AssetSearch(gl_eMMCAuthKey_AssetNumber, &AuthKeyAssetId, NULL);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetSearch(Authentication Key)=%d\n", (int)FuncRes);
        LOG_CRIT(DA_PREFIX "        Use '-emmcauthkey' to write it to OTP.\n");
        return FuncRes;
    }

    // eMMC read - get State Asset ID and Nonce
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadRequest(AuthKeyAssetId, &StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read verify - Get/Initialize eMMC item data
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_RD,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_Hibernation, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadVerify(StateAssetId,
                                 ResponseBuffer, (28 + 256),
                                 &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Copy eMMC hibernation data
    memcpy(&HiberationData, &ResponseBuffer[28], sizeof(HiberationData));

    // eMMC Read write counter request
    Size = sizeof(gl_eMMCModelNonce);
    FuncRes = val_eMMCReadWriteCounterRequest(StateAssetId, gl_eMMCModelNonce, &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC Read write counter verify - Get/Initialize write counter value
    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_RD_WCNTR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 0, 0, NULL, 0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    FuncRes = val_eMMCReadWriteCounterVerify(StateAssetId,
                                             ResponseBuffer, (28 + 256),
                                             &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCReadWriteCounterVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Increment the decryption monotonic counter (must be equal to encryption
    // monotonic counter) and write it
    IncrementCounter(HiberationData.DecCounterData, HiberationData.DecCounterLen);

    rc = eMMCModelCreatePacket(EMMC_MSGTYP_REQ_AUTH_DATA_WR,
                               PacketBuffer, sizeof(PacketBuffer),
                               0, 1, gl_eMMCAddr_Hibernation,
                               (uint8_t *)&HiberationData, sizeof(HiberationData));
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    rc = eMMCModelHandleRequest(PacketBuffer, sizeof(PacketBuffer),
                                ResponseBuffer, sizeof(ResponseBuffer),
                                0);
    if (rc != 0)
    {
        return VAL_OPERATION_FAILED;
    }

    Size = (256 / 8);
    FuncRes = val_eMMCWriteRequest(StateAssetId,
                                   PacketBuffer, (28 + 256),
                                   &PacketBuffer[(28 + 256)], &Size);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteRequest()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // eMMC write verify
    FuncRes = val_eMMCWriteVerify(StateAssetId,
                                  ResponseBuffer, (28 + 256),
                                  &ResponseBuffer[(28 + 256)], (256 / 8));
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_eMMCWriteVerify()=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Create Trusted Key Encryption Key (KEK) Asset
    AssetPolicy = VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT | VAL_POLICY_TRUSTED_WRAP;
    if (!val_IsAccessSecure())
    {
        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
    }
    FuncRes = val_AssetAlloc(AssetPolicy, (512 / 8),
                             false, false,
                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                             &KekAssetId);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetAlloc(KEK)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Derive KEK from the HUK
    memcpy(PacketBuffer,
           HiberationData.DecCounterData, HiberationData.DecCounterLen);
    Size = HiberationData.DecCounterLen;
    memcpy(&PacketBuffer[Size],
           HiberationData.eMmcAddrData, HiberationData.eMmcAddrLen);
    Size += HiberationData.eMmcAddrLen;
    memcpy(&PacketBuffer[Size],
           HiberationData.ADTailData, HiberationData.ADTailLen);
    Size += HiberationData.ADTailLen;
    FuncRes = val_AssetLoadDerive(KekAssetId, val_AssetGetRootKey(),
                                  PacketBuffer, Size,
                                  false, false, NULL, 0, NULL, 0);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_AssetLoadDerive(KEK)=%d\n", (int)FuncRes);
        return FuncRes;
    }

    // Resume from hibernation mode
    FuncRes = val_SystemResumeFromHibernation(StateAssetId, KekAssetId,
                                              PacketBuffer, Size,
                                              gl_DataBlobBuffer, gl_DataBlobSize);
    if(FuncRes != VAL_SUCCESS)
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SystemResumeFromHibernation()=%d\n", (int)FuncRes);
    }
    else
    {
        gl_HibernationDone = false;
        LOG_CRIT(DA_PREFIX "PASSED: Resume hardware from Hibernation mode\n");

        FuncRes = GetVersion();
    }

    return FuncRes;
}

/*----------------------------------------------------------------------------
 * MilenageXor
 *
 * This function performs a XOR of two values and returns the result.
 */
static void
MilenageXor(
        uint8_t * aValueResult,
        const uint8_t * aValue1,
        const uint8_t * aValue2,
        const uint32_t Size)
{
    uint32_t i;

    for (i = 0; i < Size; i++)
    {
        aValueResult[i] = aValue1[i] ^ aValue2[i];
    }
}

/*----------------------------------------------------------------------------
 * MilenageAutnCheck
 *
 * This function performs the Milenage AUTN verification related operations.
 */
static ValStatus_t
MilenageAutnCheck(void)
{
    uint8_t AUTN[sizeof(gl_Milenage_TS1_SQN) + sizeof(gl_Milenage_TS1_AMF) + sizeof(gl_Milenage_TS1_f1)];
    uint8_t RES[sizeof(gl_Milenage_TS1_f2)];
    uint8_t CK[sizeof(gl_Milenage_TS1_f3)];
    uint8_t IK[sizeof(gl_Milenage_TS1_f4)];
    ValStatus_t FuncRes = VAL_SUCCESS;
    ValAssetId_t SqnAdminAssetId = VAL_ASSETID_INVALID;
    ValAssetId_t KekAssetId = VAL_ASSETID_INVALID;

    LOG_CRIT(DA_PREFIX "Milenage AUTN verification check\n");

    // Format AUTN
    MilenageXor(AUTN,
                gl_Milenage_TS1_SQN, gl_Milenage_TS1_f5,
                sizeof(gl_Milenage_TS1_SQN));
    memcpy(&AUTN[sizeof(gl_Milenage_TS1_SQN)],
           gl_Milenage_TS1_AMF, sizeof(gl_Milenage_TS1_AMF));
    memcpy(&AUTN[sizeof(gl_Milenage_TS1_SQN) + sizeof(gl_Milenage_TS1_AMF)],
           gl_Milenage_TS1_f1, sizeof(gl_Milenage_TS1_f1));

    {
        uint8_t SQN[sizeof(gl_Milenage_TS1_SQN)];
        uint8_t AMF[sizeof(gl_Milenage_TS1_AMF)];

        FuncRes = val_SFMilenageAutnVerification(gl_Milenage_AssetNumber,
                                                 gl_Milenage_TS1_RAND, AUTN,
                                                 RES, CK, IK, SQN, AMF);
        if(FuncRes == VAL_SUCCESS)
        {
            if ((memcmp(RES, gl_Milenage_TS1_f2, sizeof(gl_Milenage_TS1_f2)) == 0) &&
                (memcmp(CK, gl_Milenage_TS1_f3, sizeof(gl_Milenage_TS1_f3)) == 0) &&
                (memcmp(IK, gl_Milenage_TS1_f4, sizeof(gl_Milenage_TS1_f4)) == 0) &&
                (memcmp(SQN, gl_Milenage_TS1_SQN, sizeof(gl_Milenage_TS1_SQN)) == 0) &&
                (memcmp(AMF, gl_Milenage_TS1_AMF, sizeof(gl_Milenage_TS1_AMF)) == 0))
            {
                LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageAutnVerification()=OK\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutnVerification()=Bad Result\n");
                return VAL_DATA_OVERRUN_ERROR;
            }
        }
        else
        {
            LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutsGeneration()=%d\n", (int)FuncRes);
        }
    }

    FuncRes = val_SFMilenageSqnAdminCreate(gl_Milenage_AssetNumber, true,
                                           &SqnAdminAssetId);
    if(FuncRes == VAL_SUCCESS)
    {
        uint8_t AUTS[sizeof(gl_Milenage_TS1_SQN) + sizeof(gl_Milenage_TS1_f1star)];
        uint32_t EMMCause = 0;

        LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageSqnAdminCreate()=OK\n");

        FuncRes = val_SFMilenageAutnVerificationSqn(SqnAdminAssetId,
                                                    gl_Milenage_TS1_RAND, AUTN,
                                                    &EMMCause, RES, CK, IK, AUTS);
        if(FuncRes == VAL_SUCCESS)
        {
            int result1 = memcmp(RES, gl_Milenage_TS1_f2, sizeof(gl_Milenage_TS1_f2));
            int result2 = memcmp(CK, gl_Milenage_TS1_f3, sizeof(gl_Milenage_TS1_f3));
            int result3 = memcmp(IK, gl_Milenage_TS1_f4, sizeof(gl_Milenage_TS1_f4));
            if ((result1 == 0) && (result2 == 0) && (result3 == 0))
            {
                LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageAutnVerificationSqn()=OK\n");

                FuncRes = val_SFMilenageAutnVerificationSqn(SqnAdminAssetId,
                                                            gl_Milenage_TS1_RAND, AUTN,
                                                            &EMMCause, RES, CK, IK, AUTS);
                if((FuncRes == VAL_VERIFY_ERROR) && (EMMCause == 21))
                {
                    ValPolicyMask_t AssetPolicy;

                    LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageAutnVerificationSqn(EMMCause)=OK\n");

                    // Create Trusted Key Encryption Key (KEK) Asset
                    AssetPolicy = VAL_POLICY_ENCRYPT | VAL_POLICY_DECRYPT | VAL_POLICY_TRUSTED_WRAP;
                    if (!val_IsAccessSecure())
                    {
                        AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                    }
                    FuncRes = val_AssetAlloc(AssetPolicy, (512 / 8),
                                             false, false,
                                             VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                             &KekAssetId);
                    if(FuncRes == VAL_SUCCESS)
                    {
                        static const uint8_t gl_AssocData[] = "Just some associated data for key derivation/datablob generation";

                        FuncRes = val_AssetLoadDerive(KekAssetId, val_AssetGetRootKey(),
                                                      gl_AssocData, sizeof(gl_AssocData)-1,
                                                      false, false, NULL, 0, NULL, 0);
                        if(FuncRes == VAL_SUCCESS)
                        {
                            gl_DataBlobSize = 300;
                            FuncRes = val_SFMilenageSqnAdminExport(SqnAdminAssetId, KekAssetId,
                                                                   gl_AssocData, sizeof(gl_AssocData)-1,
                                                                   gl_DataBlobBuffer, &gl_DataBlobSize);
                            if(FuncRes == VAL_SUCCESS)
                            {
                                if (gl_DataBlobSize == ((128/8) + 200))
                                {
                                    LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageSqnAdminExport()=OK\n");
                                }
                                else
                                {
                                    LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageSqnAdminExport() wrong blob size (%d != %d)\n",
                                             (int)gl_DataBlobSize, ((128/8) + 200));
                                }
                                FuncRes = val_SFMilenageSqnAdminReset(SqnAdminAssetId);
                                if(FuncRes == VAL_SUCCESS)
                                {
                                    LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageSqnAdminReset()=OK\n");
                                }
                                else
                                {
                                    LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageSqnAdminReset()=%d\n", (int)FuncRes);
                                }
                            }
                            else
                            {
                                LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageSqnAdminExport()=%d\n", (int)FuncRes);
                            }
                        }
                        else
                        {
                            LOG_CRIT(DA_PREFIX "FAILED: val_AssetLoadRandom(KEK)=%d\n", (int)FuncRes);
                        }
                    }
                    else
                    {
                        LOG_CRIT(DA_PREFIX "FAILED: val_AssetAlloc(KEK)=%d\n", (int)FuncRes);
                    }
                }
                else
                {
                    LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutnVerificationSqn(EMMCause)=%d (%d)\n",
                             (int)FuncRes, (int)EMMCause);
                }
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutnVerificationSqn()=Bad Result (%d,%d,%d)\n",
                         result1, result2, result3);
                FuncRes = VAL_DATA_OVERRUN_ERROR;
            }
        }
        else
        {
            LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutnVerificationSqn()=%d\n", (int)FuncRes);
        }
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageSqnAdminCreate()=%d\n", (int)FuncRes);
    }

    if (SqnAdminAssetId != VAL_ASSETID_INVALID)
    {
        (void)val_SFMilenageSqnAdminRelease(SqnAdminAssetId); // Note: = val_AssetFree
    }
    if (KekAssetId != VAL_ASSETID_INVALID)
    {
        (void)val_AssetFree(KekAssetId);
    }

    return FuncRes;
}


/*----------------------------------------------------------------------------
 * MilenageAutsCheck
 *
 * This function performs the Milenage AUTS Generation.
 */
static ValStatus_t
MilenageAutsCheck(void)
{
    ValStatus_t FuncRes = VAL_SUCCESS;
    uint8_t AUTS[sizeof(gl_Milenage_TS1_SQN) + sizeof(gl_Milenage_TS1_f1star)];
    uint8_t SQNmsAK[sizeof(gl_Milenage_TS1_SQN)];

    FuncRes = val_SFMilenageAutsGeneration(gl_Milenage_AssetNumber,
                                           gl_Milenage_TS1_RAND,
                                           gl_Milenage_TS1_SQN, gl_Milenage_TS1_AMF,
                                           AUTS);
    if(FuncRes == VAL_SUCCESS)
    {
        int result1;
        int result2;

        MilenageXor(SQNmsAK,
                    gl_Milenage_TS1_SQN, gl_Milenage_TS1_f5star,
                    sizeof(gl_Milenage_TS1_SQN));
        result1 = memcmp(AUTS, SQNmsAK, sizeof(gl_Milenage_TS1_SQN));
        result2 = memcmp(&AUTS[sizeof(gl_Milenage_TS1_SQN)],
                         gl_Milenage_TS1_f1star, sizeof(gl_Milenage_TS1_f1star));
        if ((result1 == 0) && (result2 == 0))
        {
            LOG_CRIT(DA_PREFIX "PASSED: val_SFMilenageAutsGeneration=OK\n");
        }
        else
        {
            LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutsGeneration=Bad Result (%d,%d)\n",
                     result1, result2);
            FuncRes = VAL_DATA_OVERRUN_ERROR;
        }
    }
    else
    {
        LOG_CRIT(DA_PREFIX "FAILED: val_SFMilenageAutsGeneration()=%d\n", (int)FuncRes);
    }

    return FuncRes;
}


/*----------------------------------------------------------------------------
 * da_val_specials_main
 */
int
da_val_specials_main(
        int argc,
        char **argv)
{
    int i;

    if (Driver130_Init() < 0)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Driver130_Init()\n");
        return -1;
    }

    // Initialize eMMC model
    for (i = 0; i < DA_EMMC_MODEL_MEMORY_ENTRIES; i++)
    {
        memset(gl_eMMCModelMemory[i], 0, 256);
    }

    // Process option arguments
    for (i = 1; argc > i; i++)
    {
        ValStatus_t FuncRes = VAL_SUCCESS;

        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help"))
        {
            LOG_CRIT(DA_PREFIX "Example application for special functionality provided by the VAL API.\n\n");
            LOG_CRIT("Syntax     : %s [<option> ...]\n", argv[0]);
            LOG_CRIT("Description: Executes a Demo Application that can perform special VAL API\n");
            LOG_CRIT("             operations. Note that you can use the options to perform an\n");
            LOG_CRIT("             operation to set the hardware in a certain state and the provided\n");
            LOG_CRIT("             operations (options) are executed in the provided order.\n");
            LOG_CRIT("Options    : -help              Display this information.\n");
            LOG_CRIT("             -emmcauthkey       Write eMMC authentication key to OTP. Note that\n");
            LOG_CRIT("                                the OTP must be initialized with HUK and COID.\n");
            LOG_CRIT("             -emmccheck         Check the eMMC read and write functions based\n");
            LOG_CRIT("                                on the eMMC authentication key.\n");
            LOG_CRIT("             -milenageotp       Write Milenage K and OPc to OTP. Note that\n");
            LOG_CRIT("                                the OTP must be initialized with HUK and COID.\n");
            LOG_CRIT("             -milenageautn      Check the Milenage AUTN verification sequence based\n");
            LOG_CRIT("                                on the Milenage K and OPc.\n");
            LOG_CRIT("             -milenageauts      Check the Milenage AUTS generation sequence based\n");
            LOG_CRIT("                                on the Milenage K and OPc.\n");
            LOG_CRIT("             -inccounter        Increment the 96-bit monotonic counter.\n");
            LOG_CRIT("             -initotp           Initialize the OTP with a default COID and HUK.\n");
            LOG_CRIT("             -provrndhuk128[=<Samples>]\n");
            LOG_CRIT("                                Initialize the OTP with a random 128-bit HUK\n");
            LOG_CRIT("                                without CRC and default COID. There is no OTP\n");
            LOG_CRIT("                                key blob generated. Optionally <Samples> specify\n");
            LOG_CRIT("                                the TRNG cycle samples for the TRNG Configure/\n");
            LOG_CRIT("                                Start (default=3072).\n");
            LOG_CRIT("             -provrndhuk256[=<Samples>]\n");
            LOG_CRIT("                                Initialize the OTP with a random 256-bit HUK\n");
            LOG_CRIT("                                with CRC and default COID. An OTP key blob with\n");
            LOG_CRIT("                                HUK is generated. The <Samples> specified the\n");
            LOG_CRIT("                                TRNG cycle samples for the TRNG Configure/Start\n");
            LOG_CRIT("                                (default=3072).\n");
            LOG_CRIT("             -sleep             Set hardware in Sleep mode.\n");
            LOG_CRIT("             -stopsleep         Resume hardware from Sleep mode.\n");
            LOG_CRIT("             -hibernation       Set hardware in Hibernation mode.\n");
            LOG_CRIT("             -stophibernation   Resume hardware from hibernation mode.\n");
            LOG_CRIT("                                Note that this option can only be used in the\n");
            LOG_CRIT("                                combination '-hibernation -stophibernation'.\n");
            LOG_CRIT("             -hwmodulestatus    Get module status from hardware.\n");
            LOG_CRIT("             -hwoptions         Get hardware options.\n");
            LOG_CRIT("             -hwversion         Get hardware version information.\n");
            LOG_CRIT("             -selftest          Run self-test.\n");
            LOG_CRIT("             -system            System information.\n");
            LOG_CRIT("             -reset             Reset hardware.\n");
            LOG_CRIT("             -trng[=<Samples>]  Configure/Start the TRNG. Optionally <Samples>\n");
            LOG_CRIT("                                specify the TRNG cycle samples (default=3072).\n");
            LOG_CRIT("             -zerootp           Zeroize (destroy) OTP.\n");
            continue;
        }

        if (strcmp(argv[i], "-reset") == 0)
        {
            // Reset system to have known starting state
            LOG_CRIT(DA_PREFIX "Reset system\n");
            FuncRes = DoReset();
        }
        else if (strncmp(argv[i], "-trng", 5) == 0)
        {
            if (argv[i][5] == '=')
            {
                gl_SampleCycles = (uint16_t)atoi(&argv[i][6]);
                if (gl_SampleCycles == 0)
                {
                    gl_SampleCycles = 2;
                }
            }
            FuncRes = ConfigureTrng();
        }
        else if (strcmp(argv[i], "-initotp") == 0)
        {
            LOG_CRIT(DA_PREFIX "Program default HUK and Crypto Officer ID in OTP\n");
            FuncRes = WriteOTP(0, 1, true,
                               gl_ADLabel, sizeof(gl_ADLabel)-1,
                               gl_HUK_OTPKeyBlob, sizeof(gl_HUK_OTPKeyBlob));
        }
        else if (strncmp(argv[i], "-provrndhuk128", 14) == 0)
        {
            LOG_CRIT(DA_PREFIX "Program random 128-bit HUK (no CRC&KeyBlob) and default Crypto Officer ID in OTP\n");
            FuncRes = WriteRandomHuk(0, true, true, &argv[i][14]);
        }
        else if (strncmp(argv[i], "-provrndhuk256", 14) == 0)
        {
            LOG_CRIT(DA_PREFIX "Program random 256-bit HUK (CRC&KeyBlob) and default Crypto Officer ID in OTP\n");
            FuncRes = WriteRandomHuk(0, false, true, &argv[i][14]);
        }
        else if (strcmp(argv[i], "-inccounter") == 0)
        {
            ValAssetId_t AssetId = VAL_ASSETID_INVALID;
            uint8_t DataBuffer1[16];
            uint8_t DataBuffer2[16];
            ValSize_t DataSize;

            LOG_CRIT(DA_PREFIX "96-bit Monotonic Counter increment\n");

            // Get Asset ID of the 96-bit Monotonic Counter
            FuncRes = val_AssetSearch(33, &AssetId, &DataSize);
            if(FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_AssetSearch(AssetNummber=33)=%d\n", (int)FuncRes);
                break;
            }

            FuncRes = val_MonotonicCounterRead(AssetId, DataBuffer1, DataSize);
            if(FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_MonotonicCounterRead(1)=%d\n", (int)FuncRes);
                break;
            }

            FuncRes = val_MonotonicCounterIncrement(AssetId);
            if(FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_MonotonicCounterIncrement()=%d\n", (int)FuncRes);
                break;
            }

            FuncRes = val_MonotonicCounterRead(AssetId, DataBuffer2, DataSize);
            if(FuncRes == VAL_SUCCESS)
            {
                Log_HexDump("Before increment:", 0, DataBuffer1, (unsigned int)DataSize);
                Log_HexDump("After increment :", 0, DataBuffer2, (unsigned int)DataSize);
                if (memcmp(DataBuffer1, DataBuffer2, DataSize) != 0)
                {
                    LOG_CRIT(DA_PREFIX "PASSED: 96-bit Monotonic Counter increment\n");
                }
                else
                {
                    LOG_CRIT(DA_PREFIX "FAILED: 96-bit Monotonic Counter increment\n");
                    break;
                }
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_MonotonicCounterRead(2)=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-zerootp") == 0)
        {
            LOG_CRIT(DA_PREFIX "Zeroize (destroy) OTP\n");
            FuncRes = val_ServiceSelectOTPZeroize();
            if(FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_ServiceSelectOTPZeroize()=%d\n", (int)FuncRes);
                break;
            }

            FuncRes = val_ServiceZeroizeOTP();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: OTP Zeroize\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_ServiceZeroizeOTP()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-emmcauthkey") == 0)
        {
            ValAssetId_t AuthKeyAssetId = VAL_ASSETID_INVALID;

            LOG_CRIT(DA_PREFIX "Program eMMC Authentication Key in OTP\n");

            // Check if Authentication Key is already written
            FuncRes = val_AssetSearch(gl_eMMCAuthKey_AssetNumber, &AuthKeyAssetId, NULL);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "SKIPPED: eMMC Authentication Key already exists\n");
            }
            else
            {
                FuncRes = WriteOTP(gl_eMMCAuthKey_AssetNumber, 6, true,
                                   gl_ADLabel, sizeof(gl_ADLabel)-1,
                                   gl_eMMCAuthKey_OTPKeyBlob, sizeof(gl_eMMCAuthKey_OTPKeyBlob));
            }
        }
        else if (strcmp(argv[i], "-emmccheck") == 0)
        {
            FuncRes = eMMCCheckSequence();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: eMMC functions/sequence check\n");
            }
        }
        else if (strcmp(argv[i], "-sleep") == 0)
        {
            FuncRes = val_SystemSleep();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: hardware is in Sleep mode\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SystemSleep()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-stopsleep") == 0)
        {
            FuncRes = val_SystemResumeFromSleep();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: Resume hardware from Sleep mode\n");

                FuncRes = GetVersion();
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SystemResumeFromSleep()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-hibernation") == 0)
        {
            FuncRes = Hibernation();
        }
        else if (strcmp(argv[i], "-stophibernation") == 0)
        {
            FuncRes = ResumeFromHibernation();
        }
        else if (strcmp(argv[i], "-selftest") == 0)
        {
            LOG_CRIT(DA_PREFIX "Running self-test\n");
            FuncRes = val_SystemSelfTest();
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: self-test Passed\n");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SystemSelfTest()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-system") == 0)
        {
            uint8_t OtpErrorCode = 0;
            uint16_t OtpErrorLocation = 0;
            uint8_t Mode = 0;
            uint8_t ErrorTest = 0;
            uint8_t CryptoOfficer = 0;
            uint8_t HostID = 0;
            uint8_t NonSecure = 0;
            uint32_t Identity = 0;

            FuncRes = val_SystemGetState(&OtpErrorCode, &OtpErrorLocation,
                                         &Mode, &ErrorTest, &CryptoOfficer,
                                         &HostID, &NonSecure, &Identity);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: System State read\n");
                LOG_CRIT(DA_PREFIX "OTP State      : %u (%u)\n", OtpErrorCode, OtpErrorLocation);
                LOG_CRIT(DA_PREFIX "Mode           : %u (0x%02X)\n", Mode, ErrorTest);
                LOG_CRIT(DA_PREFIX "CryptoOfficerId: %s\n", CryptoOfficer ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "HostId         : %u\n", HostID);
                LOG_CRIT(DA_PREFIX "Secure         : %s\n", NonSecure ? "No" : "Yes");
                LOG_CRIT(DA_PREFIX "Identity       : 0x%X\n", Identity);
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_SystemGetState()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-hwmodulestatus") == 0)
        {
            uint8_t FIPSmode;
            uint8_t NonFIPSmode;
            uint8_t CRC24Ok;
            uint8_t CRC24Busy;
            uint8_t CRC24Error;
            uint8_t FwImageWritten;
            uint8_t FwImageCheckDone;
            uint8_t FwImageAccepted;
            uint8_t FatalError;

            FuncRes = val_HW_ModuleStatus(&FIPSmode, &NonFIPSmode, &FatalError,
                                          &CRC24Ok, &CRC24Busy, &CRC24Error,
                                          &FwImageWritten, &FwImageCheckDone, &FwImageAccepted);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: Hardware Module Status read\n");
                LOG_CRIT(DA_PREFIX "FIPS mode                : %sctive\n", FIPSmode ? "A" : "Ina");
                LOG_CRIT(DA_PREFIX "non-FIPS mode            : %sctive\n", NonFIPSmode ? "A" : "Ina");
                LOG_CRIT(DA_PREFIX "Fatal error              : %s\n", FatalError ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "CRC24 Ok                 : %s\n", CRC24Ok ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "CRC24 busy               : %s\n", CRC24Busy ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "CRC24 error              : %s\n", CRC24Error ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "Firmware image written   : %s\n", FwImageWritten ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "Firmware image check done: %s\n", FwImageCheckDone ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "Firmware image accepted  : %s\n", FwImageAccepted ? "Yes" : "No");
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_HW_ModuleStatus()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-hwoptions") == 0)
        {
            uint8_t nMailboxes;
            uint16_t MailboxSize;
            uint8_t HostId;
            uint8_t SecureHostId;
            uint8_t MasterId;
            uint8_t MyHostId;
            uint8_t ProtectionAvailable;
            uint8_t Protection;
            uint16_t StandardEngines;
            uint16_t CustomEngines;

            FuncRes = val_HW_EIP_Options(&nMailboxes, &MailboxSize,
                                         &HostId, &SecureHostId, &MasterId, &MyHostId,
                                         &ProtectionAvailable, &Protection,
                                         &StandardEngines, &CustomEngines);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: Hardware EIP Options read\n");
                LOG_CRIT(DA_PREFIX "Number of mailboxes   : %d\n", nMailboxes);
                LOG_CRIT(DA_PREFIX "Mailbox size in bytes : %d\n", MailboxSize);
                LOG_CRIT(DA_PREFIX "HostId                : 0x%X\n", HostId);
                LOG_CRIT(DA_PREFIX "SecureHostId          : 0x%X\n", SecureHostId);
                LOG_CRIT(DA_PREFIX "MasterId              : %d\n", MasterId);
                LOG_CRIT(DA_PREFIX "MyHostId              : %d\n", MyHostId);
                LOG_CRIT(DA_PREFIX "Protection available  : %s\n", ProtectionAvailable ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "Protection set        : %s\n", Protection ? "Yes" : "No");
                LOG_CRIT(DA_PREFIX "Standard Engines      : 0x%04X\n", StandardEngines);
                LOG_CRIT(DA_PREFIX "Custom Engines        : 0x%04X\n", CustomEngines);
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_HW_EIP_Options()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-hwversion") == 0)
        {
            uint8_t EipNumber;
            uint8_t MajorVersion;
            uint8_t MinorVersion;
            uint8_t PatchLevel;

            FuncRes = val_HW_EIP_Version(&MajorVersion, &MinorVersion,
                                         &PatchLevel, &EipNumber);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "PASSED: Hardware EIP version read\n");
                LOG_CRIT(DA_PREFIX "EIP HW number    : 0x%X (%d)\n", EipNumber, EipNumber);
                LOG_CRIT(DA_PREFIX "HW major version : %d\n", MajorVersion);
                LOG_CRIT(DA_PREFIX "HW minor version : %d\n", MinorVersion);
                LOG_CRIT(DA_PREFIX "HW patch level   : %d\n", PatchLevel);
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_HW_EIP_Version()=%d\n", (int)FuncRes);
            }
        }
        else if (strcmp(argv[i], "-milenageotp") == 0)
        {
            ValAssetId_t AssetId = VAL_ASSETID_INVALID;

            LOG_CRIT(DA_PREFIX "Program Milenage K and OPc in OTP\n");

            // Check if Authentication Key is already written
            FuncRes = val_AssetSearch(gl_Milenage_AssetNumber, &AssetId, NULL);
            if(FuncRes == VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "SKIPPED: Milenage K and OPc already exists\n");
            }
            else
            {
                FuncRes = WriteOTP(gl_Milenage_AssetNumber, 7, true,
                                   gl_ADLabel, sizeof(gl_ADLabel)-1,
                                   gl_Milenage_TS1_OTPBlob, sizeof(gl_Milenage_TS1_OTPBlob));
            }
        }
        else if (strcmp(argv[i], "-milenageautn") == 0)
        {
            FuncRes = MilenageAutnCheck();
        }
        else if (strcmp(argv[i], "-milenageauts") == 0)
        {
            FuncRes = MilenageAutsCheck();
        }
        if(FuncRes != VAL_SUCCESS)
        {
            // Always exit on an error
            break;
        }
    }

    Driver130_Exit();

    return 0;
}


#ifndef DA_VAL_MAIN_REMOVE
/*----------------------------------------------------------------------------
 * main
 *
 * Program entry.
 */
int
main(
        int argc,
        char **argv)
{
    return da_val_specials_main(argc, argv);
}
#endif // !DA_VAL_MAIN_REMOVE


/* end of file da_val_specials.c */


