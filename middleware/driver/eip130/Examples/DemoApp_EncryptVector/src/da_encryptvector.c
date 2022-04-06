/* da_encryptvector.c
 *
 * Demo Application for the Encrypt Vector for PKI via the VAL API
 */

/*****************************************************************************
* Copyright (c) 2016-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "da_encryptvector.h"           // da_encryptvector_main


/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

#include "c_da_encryptvector.h"         // configuration

#include "log.h"
#include "api_driver_init.h"
#include "api_val.h"

#include <stdlib.h>

#ifndef DA_ENCRYPTVECTOR_FS_REMOVE
#include <stdio.h>      // fopen, fclose, fread
#endif


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

#define DA_PREFIX   "DA_ENC_VEC: "

#ifdef DA_ENCRYPTVECTOR_ENABLE
typedef enum {
    ASSET_TYPE_NONE = 0,
    ASSET_TYPE_ECDSA,
    ASSET_TYPE_RSA,
    ASSET_TYPE_PRIVATE_DATA,
} AssetType_t;
#endif


/*----------------------------------------------------------------------------
 * Local variables
 */

#ifdef DA_ENCRYPTVECTOR_ENABLE
static uint8_t gl_KDK_AssetNumber = 37;
static uint8_t gl_KEK_AssetNumber = 255;
static char * gl_pAssetFile = NULL;
static AssetType_t gl_AssetType = ASSET_TYPE_NONE;
static int gl_DataSize = 0;
static int gl_AssetDataSize = 0;
static uint8_t gl_AssetData[1024];
static uint8_t gl_EncryptedVector[512];
#endif


/*----------------------------------------------------------------------------
 * da_encryptvector_main
 */
int
da_encryptvector_main(
        int argc,
        char **argv)
{
    int i;
    bool fEncryptVector = true;

#ifdef DA_ENCRYPTVECTOR_ENABLE
    ValSize_t ModulusSizeBits = 0;

    if (Driver130_Init() < 0)
    {
        LOG_CRIT(DA_PREFIX "FAILED: Driver130_Init()\n");
        return -1;
    }
#endif

    // Process option arguments
    i = 1;
    while (i < argc)
    {
#ifdef DA_ENCRYPTVECTOR_ENABLE
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help"))
#endif
        {
            LOG_CRIT(DA_PREFIX "Example application for an Encrypted Vector for PKI via the VAL API.\n\n");
            LOG_CRIT("Syntax     : %s [<option> ...]\n", argv[0]);
            LOG_CRIT("Description: Executes a Demo Application that encrypts the Asset data to get an\n");
            LOG_CRIT("             Encrypted Vector that can be used in a PKI command.\n");
#ifdef DA_ENCRYPTVECTOR_ENABLE
            LOG_CRIT("Options    : -help                Display this information.\n");
            LOG_CRIT("             -ecdsa192 <file>     Plaintext or AES wrapped ECDSA private key\n");
            LOG_CRIT("                                  to load.\n");
            LOG_CRIT("             -ecdsa224 <file>     ...\n");
            LOG_CRIT("             -ecdsa256 <file>     ...\n");
            LOG_CRIT("             -ecdsa384 <file>     ...\n");
            LOG_CRIT("             -ecdsa521 <file>     ...\n");
            LOG_CRIT("             -rsa1024 <file>      Plaintext or AES wrapped RSA private key\n");
            LOG_CRIT("                                  (exponent part only) to load.\n");
            LOG_CRIT("             -rsa2048 <file>      ...\n");
            LOG_CRIT("             -rsa3072 <file>      ...\n");
            LOG_CRIT("             -rsa4096 <file>      ...\n");
            LOG_CRIT("             -prvdata <file>      Plaintext or AES wrapped private data to load.\n");
            LOG_CRIT("             -rndprvdata <size>   Random private data to load.\n");
            LOG_CRIT("             -kdk <number>        KDK Asset number for Encrypted Vector (Def. 37).\n");
            LOG_CRIT("             -kek <number>        HSM KEK Asset number.\n");
            LOG_CRIT("                                  When given the <file> is assumed to be an AES\n");
            LOG_CRIT("                                  key wrap blob.\n\n");
            LOG_CRIT("Note: <file> must be a binary file.\n");
#else
            LOG_CRIT("Note: The driver does not support this functionality.\n");
#endif
            fEncryptVector = false;
            break;
        }

#ifdef DA_ENCRYPTVECTOR_ENABLE
        if ((i + 1) >= argc)
        {
            LOG_CRIT(DA_PREFIX "Wrong number of arguments. Please use -help for the correct argument use.\n");
            fEncryptVector = false;
            break;
        }

        if (strncmp(argv[i], "-ecdsa", 6) == 0)
        {
            gl_AssetType = ASSET_TYPE_ECDSA;
            ModulusSizeBits = atoi(&argv[i][6]);
            gl_pAssetFile = argv[i+1];
        }
        else if (strncmp(argv[i], "-rsa", 4) == 0)
        {
            gl_AssetType = ASSET_TYPE_RSA;
            ModulusSizeBits = atoi(&argv[i][4]);
            gl_pAssetFile = argv[i+1];
        }
        else if (strcmp(argv[i], "-prvdata") == 0)
        {
            gl_AssetType = ASSET_TYPE_PRIVATE_DATA;
            gl_pAssetFile = argv[i+1];
        }
        else if (strcmp(argv[i], "-rndprvdata") == 0)
        {
            gl_AssetType = ASSET_TYPE_PRIVATE_DATA;
            gl_pAssetFile = NULL;
            gl_AssetDataSize = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-kdk") == 0)
        {
            gl_KDK_AssetNumber = (uint8_t)atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-kek") == 0)
        {
            gl_KEK_AssetNumber = (uint8_t)atoi(argv[i+1]);
        }
        else
        {
            LOG_CRIT(DA_PREFIX "Invalid argument specified. Please use -help for the correct arguments.\n");
            fEncryptVector = false;
            break;
        }
#endif

        i += 2;
    }

    if (fEncryptVector)
    {
#ifdef DA_ENCRYPTVECTOR_ENABLE
        ValStatus_t FuncRes = VAL_SUCCESS;
        ValAssetId_t AssetId = VAL_ASSETID_INVALID;

        if(gl_pAssetFile != NULL)
        {
#ifdef DA_ENCRYPTVECTOR_FS_REMOVE
            extern uint8_t _binary_da_encryptvector_keyblob_bin_start;
            extern uint8_t _binary_da_encryptvector_keyblob_bin_end;
            extern unsigned int _binary_da_encryptvector_keyblob_bin_size;

            uint8_t * start = (uint8_t*)&_binary_da_encryptvector_keyblob_bin_start;
            uint8_t * end = (uint8_t*)&_binary_da_encryptvector_keyblob_bin_end;
            uint32_t * size = (uint32_t*)&_binary_da_encryptvector_keyblob_bin_size;
            uint8_t * p = gl_AssetData;

            gl_DataSize = (int)size;

            // Read key blob
            while(start < end)
            {
                *p = *start;
                start++;
                p++;
            }
#else // DA_ENCRYPTVECTOR_FS_REMOVE
            FILE *fp;

            fp = fopen(gl_pAssetFile, "rb");
            if (fp == NULL)
            {
                LOG_CRIT(DA_PREFIX "FAILED: Could not open the file\n");
                FuncRes = VAL_INVALID_PARAMETER;
            }
            else
            {
                gl_DataSize = fread(gl_AssetData, 1, sizeof(gl_AssetData), fp);
                fclose(fp);
            }
#endif // !DA_ENCRYPTVECTOR_FS_REMOVE

            if (gl_DataSize <= 0)
            {
                LOG_CRIT(DA_PREFIX "FAILED: Could not read the file\n");
                FuncRes = VAL_INVALID_LENGTH;
            }

            // Determine Asset size
            gl_AssetDataSize = gl_DataSize;
            if (gl_KEK_AssetNumber != 255)
            {
                gl_AssetDataSize -= (64/8);
            }
        }

        if (FuncRes == VAL_SUCCESS)
        {
            ValPolicyMask_t AssetPolicy;

            // Create Asset
            switch (gl_AssetType)
            {
            default:
                gl_pAssetFile = "Dummy";
                gl_AssetDataSize = 64;
                memcpy(gl_AssetData,
                       "Is data to initialize the data area of the Asset to export with.",
                       gl_AssetDataSize);
            case ASSET_TYPE_PRIVATE_DATA:
                AssetPolicy = VAL_POLICY_EXPORT | VAL_POLICY_PRIVATE_DATA;
                if (!val_IsAccessSecure())
                {
                    AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                }
                FuncRes = val_AssetAlloc(AssetPolicy, gl_AssetDataSize,
                                         false, false,
                                         VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                         &AssetId);
                break;

            case ASSET_TYPE_ECDSA:
                switch (ModulusSizeBits)
                {
                case 192:
                    gl_AssetDataSize = (192/8);
                    break;
                case 224:
                    gl_AssetDataSize = (224/8);
                    break;
                case 256:
                    gl_AssetDataSize = (256/8);
                    break;
                case 384:
                    gl_AssetDataSize = (384/8);
                    break;
                case 521:
                    gl_AssetDataSize = ((521+7)/8);
                    break;
                default:
                    gl_AssetDataSize = 0;
                    break;
                }
                if (gl_AssetDataSize)
                {
                    if (gl_KEK_AssetNumber != 255)
                    {
                        AssetPolicy = VAL_POLICY_EXPORT | VAL_POLICY_PRIVATE_DATA;
                        if (!val_IsAccessSecure())
                        {
                            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                        }
                        FuncRes = val_AssetAlloc(AssetPolicy, gl_AssetDataSize,
                                                 false, false,
                                                 VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                                 &AssetId);
                    }
                    else
                    {
                        FuncRes = val_AsymEcdsaAllocPrivateKeyAsset(ModulusSizeBits,
                                                                    VAL_SYM_ALGO_HASH_SHA1,
                                                                    false, true,
                                                                    &AssetId);
                    }
                }
                else
                {
                    FuncRes = VAL_INVALID_KEYSIZE;
                }
                break;

            case ASSET_TYPE_RSA:
                switch (ModulusSizeBits)
                {
                case 1024:
                    gl_AssetDataSize = (1024/8);
                    break;
                case 2048:
                    gl_AssetDataSize = (2048/8);
                    break;
                case 3072:
                    gl_AssetDataSize = (3072/8);
                    break;
                case 4096:
                    gl_AssetDataSize = (4096/8);
                    break;
                default:
                    gl_AssetDataSize = 0;
                    break;
                }
                if (gl_AssetDataSize)
                {
                    if (gl_KEK_AssetNumber != 255)
                    {
                        AssetPolicy = VAL_POLICY_EXPORT | VAL_POLICY_PRIVATE_DATA;
                        if (!val_IsAccessSecure())
                        {
                            AssetPolicy |= VAL_POLICY_SOURCE_NON_SECURE;
                        }
                        FuncRes = val_AssetAlloc(AssetPolicy, gl_AssetDataSize,
                                                 false, false,
                                                 VAL_ASSET_LIFETIME_INFINITE, false, false, 0,
                                                 &AssetId);
                    }
                    else
                    {
                        FuncRes = val_AsymRsaPkcs1v15AllocPrivateKeyAsset(ModulusSizeBits,
                                                                          (gl_AssetDataSize * 8),
                                                                          VAL_SYM_ALGO_HASH_SHA1,
                                                                          false, true,
                                                                          &AssetId);
                    }
                }
                else
                {
                    FuncRes = VAL_INVALID_KEYSIZE;
                }
                break;
            }
            if (FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: AssetAllocation()=%d\n", FuncRes);
            }
        }

        if (FuncRes == VAL_SUCCESS)
        {
            // Load Asset
            if (gl_KEK_AssetNumber != 255)
            {
                // Via AES Key Unwrap
                ValAssetId_t KEKAssetId = VAL_ASSETID_INVALID;

                FuncRes = val_AssetSearch(gl_KEK_AssetNumber, &KEKAssetId, NULL);
                if (FuncRes == VAL_SUCCESS)
                {
                    FuncRes = val_AssetLoadAesunwrap(KEKAssetId, AssetId,
                                                     gl_AssetData, gl_DataSize);
                }
            }
            else
            {
                // Via Plaintext
                ValAsymBigInt_t AssetData;

                switch (gl_AssetType)
                {
                default:
                case ASSET_TYPE_PRIVATE_DATA:
                    if (gl_pAssetFile == NULL)
                    {
                        FuncRes = val_AssetLoadRandom(AssetId);
                    }
                    else
                    {
                        FuncRes = val_AssetLoadPlaintext(AssetId,
                                                         gl_AssetData, gl_AssetDataSize);
                    }
                    break;

                case ASSET_TYPE_ECDSA:
                    AssetData.Data_p = gl_AssetData;
                    AssetData.ByteDataSize = gl_AssetDataSize;
                    // Note that the domain parameters (curve) is not used because
                    // it is not relevant for this example.
                    FuncRes = val_AsymEccLoadPrivateKeyAssetPlaintext(&AssetData,
                                                                      ModulusSizeBits,
                                                                      AssetId,
                                                                      VAL_ASSETID_INVALID,
                                                                      NULL, 0, NULL, NULL);
                    break;

                case ASSET_TYPE_RSA:
                    AssetData.Data_p = gl_AssetData;
                    AssetData.ByteDataSize = gl_AssetDataSize;
                    // Note that modulus is fake because it is not relevant for this
                    // example.
                    FuncRes = val_AsymRsaLoadKeyAssetPlaintext(&AssetData, ModulusSizeBits,
                                                               &AssetData, ModulusSizeBits,
                                                               AssetId,
                                                               VAL_ASSETID_INVALID,
                                                               NULL, 0, NULL, NULL);
                    break;
                }
            }
            if (FuncRes != VAL_SUCCESS)
            {
                LOG_CRIT(DA_PREFIX "FAILED: AssetLoad()=%d\n", FuncRes);
            }
        }

        if (FuncRes == VAL_SUCCESS)
        {
            ValSize_t Size = 0;

            // Retrieve the Encrypt Vector size
            FuncRes = val_EncryptVectorForPKI(AssetId, gl_KDK_AssetNumber,
                                              gl_EncryptedVector, &Size);
            if (FuncRes == VAL_INVALID_LENGTH)
            {
                LOG_CRIT(DA_PREFIX "PASSED: Encrypted Vector size = %d\n",
                         (int)Size);

                // Perform Encrypt Vector for PKI
                FuncRes = val_EncryptVectorForPKI(AssetId, gl_KDK_AssetNumber,
                                                  gl_EncryptedVector, &Size);
                if (FuncRes == VAL_SUCCESS)
                {
                    Log_HexDump(DA_PREFIX "Encrypted Vector",
                                0, gl_EncryptedVector, Size);
                }
                else
                {
                    LOG_CRIT(DA_PREFIX "FAILED: val_EncryptVectorForPKI(Vector)=%d\n",
                             FuncRes);
                }
            }
            else
            {
                LOG_CRIT(DA_PREFIX "FAILED: val_EncryptVectorForPKI(Size)=%d\n",
                         FuncRes);
            }
        }

        // Clean up
        if (AssetId != VAL_ASSETID_INVALID)
        {
            (void)val_AssetFree(AssetId);
        }
#else
        LOG_CRIT(DA_PREFIX "Driver does not encrypt vector functionality\n");
#endif
    } // while

#ifdef DA_ENCRYPTVECTOR_ENABLE
    Driver130_Exit();
#endif

    return 0;
}


#ifndef DA_ENCRYPTVECTOR_MAIN_REMOVE
/*----------------------------------------------------------------------------
 * main
 *
 * Program entry.
 */
int
main(
        int argc,
        char ** argv)
{
    return da_encryptvector_main(argc, argv);
}
#endif


/* end of file da_encryptvector.c */


