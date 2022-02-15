/* aessiv.c
 *
 * AES-SIV implementation using the VAL API.
 */

/*****************************************************************************
* Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
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
#include "aessiv.h"                     // typedefs

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */
#include "api_val.h"                    // val_*
#define LOG_SEVERITY_MAX  LOG_SEVERITY_CRIT
#include "log.h"                        // Log_HexDump

#include <stdlib.h>
#define da_malloc(s)    malloc(s)
#define da_free(s)      free(s)


static void
AESSIV_Xor(
        uint8_t * buffer,
        const uint8_t * XorValue)
{
    int i;

    for (i = 0; i < (128/8); i++)
    {
        buffer[i] ^= XorValue[i];
    }
}

static void
AESSIV_BitShiftLeft(
        uint8_t * buffer)
{
    int i;

    for (i = 0; i < ((128/8) -1); i++)
    {
        buffer[i] = (uint8_t)((buffer[i] << 1) | ((buffer[i + 1] >> 7) & 1));
    }
    buffer[((128/8) -1)] = (uint8_t)(buffer[((128/8) -1)] << 1);
}

// dbl(S)
//   is the multiplication of S and 0...010 in the finite field
//   represented using the primitive polynomial x^128 + x^7 + x^2 + x + 1.
static void
AESSIV_Dbl(
        uint8_t * buffer)
{
    const uint8_t AESSIV_XorBlock[(128/8)] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
    };
    bool XorNeeded = ((buffer[0] >> 7) == 1);

    AESSIV_BitShiftLeft(buffer);
    if (XorNeeded)
    {
        AESSIV_Xor(buffer, AESSIV_XorBlock);
    }
}

static int
AESSIV_S2V(
        AESSIV_Context * Context_p,
        const uint8_t * Data_p,
        const size_t DataSize,
        uint8_t * Mac_p)
{
    const uint8_t zero_block[(128/8)] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t * DataBuf_p = NULL;
    size_t DataBufSize = 0;
    ValSymContextPtr_t SymContext_p = NULL;
    ValSize_t MacSize = (128/8);
    ValStatus_t Status;
    unsigned int i;

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_AES_CMAC, VAL_SYM_MODE_NONE, false,
                          &SymContext_p);
    if (Status != VAL_SUCCESS)
    {
        return -1;
    }

    if (Context_p->Verbose)
    {
        Log_HexDump("CMAC Key", 0, Context_p->Key, (Context_p->KeySize / 2));
    }
    Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                            Context_p->Key, (ValSize_t)(Context_p->KeySize / 2));
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }

    Status = val_SymMacGenerate(SymContext_p,
                                zero_block, sizeof(zero_block),
                                Mac_p, &MacSize);
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }
    if (Context_p->Verbose)
    {
        Log_HexDump("MAC(0)", 0, Mac_p, (unsigned int)MacSize);
    }

    for (i = 0; i < Context_p->AD_ListCount; i++)
    {
        uint8_t LocalMac[(128/8)];
        ValSize_t ADsize = 0;

        if (Context_p->Verbose)
        {
            Log_HexDump("MAC(..)", 0, Mac_p, (unsigned int)MacSize);
        }
        AESSIV_Dbl(Mac_p);
        if (Context_p->Verbose)
        {
            Log_HexDump("MACdbl(..)", 0, Mac_p, (unsigned int)MacSize);
        }

        Status = val_SymAlloc(VAL_SYM_ALGO_MAC_AES_CMAC, VAL_SYM_MODE_NONE, false,
                              &SymContext_p);
        if (Status != VAL_SUCCESS)
        {
            return -1;
        }

        Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                                Context_p->Key, (ValSize_t)(Context_p->KeySize/2));
        if (Status != VAL_SUCCESS)
        {
            val_SymRelease(SymContext_p);
            return -1;
        }

        ADsize = (ValSize_t)(Context_p->AD_List[i+1] - Context_p->AD_List[i]);
        MacSize = (128/8);
        Status = val_SymMacGenerate(SymContext_p,
                                    Context_p->AD_List[i], ADsize,
                                    LocalMac, &MacSize);
        if (Status != VAL_SUCCESS)
        {
            val_SymRelease(SymContext_p);
            return -1;
        }

        AESSIV_Xor(Mac_p, LocalMac);
        if (Context_p->Verbose)
        {
            Log_HexDump("MACxor(..)", 0, Mac_p, (unsigned int)MacSize);
        }
    }

    if (DataSize >= (128/8))
    {
        DataBufSize = DataSize;
        DataBuf_p = da_malloc(DataBufSize);
        if (DataBuf_p == NULL)
        {
            return -1;
        }

        memcpy(DataBuf_p, Data_p, DataSize);
        AESSIV_Xor(&DataBuf_p[DataBufSize - 16], Mac_p);
    }
    else
    {
        DataBufSize = (128/8);
        DataBuf_p = da_malloc(DataBufSize);
        if (DataBuf_p == NULL)
        {
            return -1;
        }

        AESSIV_Dbl(Mac_p);
        memcpy(DataBuf_p, Data_p, DataSize);
        DataBuf_p[DataSize] = 0x80;           // Pad
        for (i = (unsigned int)(DataSize + 1); i < (128/8); i++)
        {
            DataBuf_p[i] = 0x00;
        }
        AESSIV_Xor(DataBuf_p, Mac_p);
    }
    if (Context_p->Verbose)
    {
        Log_HexDump("CMAC Data", 0, DataBuf_p, (unsigned int)DataBufSize);
    }

    Status = val_SymAlloc(VAL_SYM_ALGO_MAC_AES_CMAC, VAL_SYM_MODE_NONE, false,
                          &SymContext_p);
    if (Status != VAL_SUCCESS)
    {
        da_free(DataBuf_p);
        return -1;
    }

    Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                            Context_p->Key, (ValSize_t)(Context_p->KeySize/2));
    if (Status != VAL_SUCCESS)
    {
        da_free(DataBuf_p);
        val_SymRelease(SymContext_p);
        return -1;
    }

    MacSize = (128/8);
    Status = val_SymMacGenerate(SymContext_p,
                                DataBuf_p, DataBufSize,
                                Mac_p, &MacSize);
    da_free(DataBuf_p);
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }
    if (Context_p->Verbose)
    {
        Log_HexDump("V", 0, Mac_p, (unsigned int)MacSize);
    }

    return 0;
}

static int
AESSIV_AESCTR(
        AESSIV_Context * Context_p,
        bool fEncrypt,
        uint8_t * IV_p,
        const uint8_t * InData_p,
        const size_t InDataSize,
        uint8_t * OutData_p,
        size_t OutDataSize)
{
    ValSymContextPtr_t SymContext_p = NULL;
    ValStatus_t Status;
    size_t InSize;
    size_t OutSize = OutDataSize;
    uint8_t tmpIV[(128/8)];

    Status = val_SymAlloc(VAL_SYM_ALGO_CIPHER_AES, VAL_SYM_MODE_CIPHER_CTR, false,
                          &SymContext_p);
    if (Status != VAL_SUCCESS)
    {
        return -1;
    }

    if (fEncrypt)
    {
        Status =  val_SymCipherInitEncrypt(SymContext_p);
        if (Status != VAL_SUCCESS)
        {
            val_SymRelease(SymContext_p);
            return -1;
        }
    }

    if (Context_p->Verbose)
    {
        Log_HexDump("CTR Key", 0,
                    (Context_p->Key + (Context_p->KeySize/2)),
                    (Context_p->KeySize / 2));
    }
    Status = val_SymInitKey(SymContext_p, VAL_ASSETID_INVALID,
                            (Context_p->Key + (Context_p->KeySize/2)),
                            (ValSize_t)(Context_p->KeySize/2));
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }

    memcpy(tmpIV, IV_p, sizeof(tmpIV));
    tmpIV[8]  &= 0x7F;
    tmpIV[12] &= 0x7F;
    if (Context_p->Verbose)
    {
        Log_HexDump("CTR IV", 0, tmpIV, sizeof(tmpIV));
    }
    Status = val_SymInitIV(SymContext_p, tmpIV, sizeof(tmpIV));
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }

    InSize = (InDataSize + ((128/8) - 1)) & (size_t)~((128/8) - 1);
    if (InSize == InDataSize)
    {
        if (Context_p->Verbose)
        {
            Log_HexDump("CTR DataIn", 0, InData_p, (unsigned int)InSize);
        }
        Status = val_SymCipherFinal(SymContext_p,
                                    InData_p, InSize,
                                    OutData_p, &OutSize);
    }
    else
    {
        uint8_t * InDataBuf_p;
        uint8_t * OutDataBuf_p;

        InDataBuf_p = da_malloc(InSize);
        if (InDataBuf_p == NULL)
        {
            val_SymRelease(SymContext_p);
            return -1;
        }
        OutDataBuf_p = da_malloc(InSize);
        if (OutDataBuf_p == NULL)
        {
            da_free(InDataBuf_p);
            val_SymRelease(SymContext_p);
            return -1;
        }

        memcpy(InDataBuf_p, InData_p, InDataSize);
        if ((InSize - InDataSize) > 0)
        {
            memset(&InDataBuf_p[InDataSize], 0, (InSize - InDataSize));
        }

        if (Context_p->Verbose)
        {
            Log_HexDump("CTR DataIn", 0, InDataBuf_p, (unsigned int)InSize);
        }
        OutSize = InSize;
        Status = val_SymCipherFinal(SymContext_p,
                                    InDataBuf_p, InSize,
                                    OutDataBuf_p, &OutSize);
        if (Status == VAL_SUCCESS)
        {
            memcpy(OutData_p, OutDataBuf_p, InDataSize);
        }

        da_free(InDataBuf_p);
        da_free(OutDataBuf_p);
    }
    if (Status != VAL_SUCCESS)
    {
        val_SymRelease(SymContext_p);
        return -1;
    }

    if (Context_p->Verbose)
    {
        Log_HexDump("CTR DataOut", 0, OutData_p, (unsigned int)OutSize);
    }

    return 0;
}

int
AESSIV_Init(
        AESSIV_Context * Context_p,
        const bool Verbose)
{
    memset(Context_p, 0, sizeof(AESSIV_Context));
    Context_p->Verbose = Verbose;
    return 0;
}

int
AESSIV_SetKey(
        AESSIV_Context * Context_p,
        const uint8_t * Key_p,
        const size_t KeySize)
{
    if (KeySize > sizeof(Context_p->Key))
    {
        return -1;
    }

    memcpy(Context_p->Key, Key_p, KeySize);
    Context_p->KeySize = (unsigned int)KeySize;
    if (Context_p->Verbose)
    {
        Log_HexDump("Set Key", 0, Context_p->Key, Context_p->KeySize);
    }
    return 0;
}

int
AESSIV_SetAD(
        AESSIV_Context * Context_p,
        const uint8_t * AD_p,
        const size_t ADSize)
{
    uint8_t * Begin_p = NULL;

    if (Context_p->AD_ListCount == 0)
    {
        if (ADSize > sizeof(Context_p->AD_Buffer))
        {
            return -1;
        }

        Begin_p = Context_p->AD_Buffer;
        Context_p->AD_List[Context_p->AD_ListCount] = Begin_p;
    }
    else
    {
        Begin_p = Context_p->AD_List[Context_p->AD_ListCount];
        if (ADSize > (sizeof(Context_p->AD_Buffer) - (unsigned int)(Begin_p - Context_p->AD_Buffer)))
        {
            return -1;
        }
        if (Context_p->AD_ListCount == ((sizeof(Context_p->AD_List) / sizeof(Context_p->AD_List[0])) -1))
        {
            return -1;
        }
    }
    memcpy(Begin_p, AD_p, ADSize);
    if (Context_p->Verbose)
    {
        Log_HexDump("Set AD", 0, Context_p->AD_List[Context_p->AD_ListCount], (unsigned int)ADSize);
    }
    Context_p->AD_ListCount++;
    Context_p->AD_List[Context_p->AD_ListCount] = Begin_p + ADSize;

    return 0;
}

int
AESSIV_Encrypt(
        AESSIV_Context * Context_p,
        const uint8_t * InData_p,
        const size_t InDataSize,
        uint8_t * OutData_p,
        size_t * OutDataSize_p)
{
    uint8_t V[(128/8)];

    if ((InDataSize + (128/8)) > *OutDataSize_p)
    {
        return -1;
    }

    if (AESSIV_S2V(Context_p, InData_p, InDataSize, V) < 0)
    {
        return -1;
    }

    if (AESSIV_AESCTR(Context_p, true, V,
                      InData_p, InDataSize,
                      &OutData_p[(128/8)], (*OutDataSize_p - (128/8))) < 0)
    {
        return -1;
    }

    memcpy(OutData_p, V, (128/8));
    *OutDataSize_p = InDataSize + (128/8);

    if (Context_p->Verbose)
    {
        Log_HexDump("KeyBlob Data", 0, OutData_p, (unsigned int)(*OutDataSize_p));
    }
    return 0;
}

/* end of file aessiv.c */
