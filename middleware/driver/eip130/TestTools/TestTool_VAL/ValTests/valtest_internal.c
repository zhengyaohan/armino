/* valtest_internal.c
 *
 * Description: VAL Test Suite internal helper functions.
 */

/*****************************************************************************
* Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
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

#include "valtest_internal.h"

/*----------------------------------------------------------------------------
 * valtest_StrictArgsCheck
 *
 * This function checks once if the VAL API is with restirct arguments
 * checking enabled.
 */
bool
valtest_StrictArgsCheck(void)
{
    static ValStatus_t gl_StrictArgs = VAL_SUCCESS;

    if (gl_StrictArgs == VAL_SUCCESS)
    {
        ValSymContextPtr_t SymContext_p;
        ValStatus_t Status;
        uint8_t * buffer_p;
        ValSize_t outDataSize = 20;

        buffer_p = SFZUTF_MALLOC(outDataSize);
        if (buffer_p == NULL)
        {
            // Assume no strict argument checking
            return false;
        }

        Status = val_SymAlloc(VAL_SYM_ALGO_CIPHER_AES,
                              VAL_SYM_MODE_CIPHER_ECB,
                              false,
                              &SymContext_p);
        if (Status == VAL_SUCCESS)
        {
            gl_StrictArgs = val_SymCipherUpdate(SymContext_p,
                                                buffer_p, 14,
                                                buffer_p, &outDataSize);
            (void)val_SymRelease(SymContext_p);
        }

        SFZUTF_FREE(buffer_p);
    }

    // Return true only in case of VAL_BAD_ARGUMENT, otherwise assume no
    // strict argument checking
    return (gl_StrictArgs == VAL_BAD_ARGUMENT);
}


/*----------------------------------------------------------------------------
 * valtest_DefaultTrngConfig
 *
 * This function set the default TRNG configuartion and actives the TRNG.
 */
ValStatus_t
valtest_DefaultTrngConfig(void)
{
    ////return val_TrngConfig(0, g_TrngSampleCycles, 1, 2, 1);
    return val_TrngConfig(0, g_TrngSampleCycles, 1, 2, 0, 1);	////V3.0.2
}


/*----------------------------------------------------------------------------
 * valtest_IsTrngActive
 *
 * This function checks if TRNG has been activated and on request it tries
 * to activate the TRNG if it was not activated.
 */
bool
valtest_IsTrngActive(
        const bool fActivate)
{
    ValStatus_t Status;
    uint8_t * buffer_p;

    buffer_p = SFZUTF_MALLOC(20);
    if (buffer_p == NULL)
    {
        // Assume TRNG is not active
        return false;
    }

    // Get a random number
    Status = val_RandomData(16, buffer_p);
    SFZUTF_FREE(buffer_p);

    if ((Status != VAL_SUCCESS) && fActivate &&
        val_IsAccessSecure() && valtest_IsCOIDAvailable())
    {
        Status = valtest_DefaultTrngConfig();
    }

    return (Status == VAL_SUCCESS);
}


/*----------------------------------------------------------------------------
 * valtest_IsCOIDAvailable
 *
 * This function checks if the Crypto Officer ID (COID) is available.
 */
bool
valtest_IsCOIDAvailable(void)
{
    ValStatus_t FuncRes;
    uint8_t CryptoOfficer = 0;

    FuncRes = val_SystemGetState(NULL, NULL, NULL, NULL,
                                 &CryptoOfficer,
                                 NULL, NULL, NULL);
    if((FuncRes == VAL_SUCCESS) && CryptoOfficer)
    {
        // COID available
        return true;
    }

    // COID not available
    return false;
}


/*----------------------------------------------------------------------------
 * asn1get
 *
 * Simple ASN.1 decoder. Reads the next item (Tag-Length-Value triplet) from
 * 'octets_p'. The actual Tag must match 'tag'. Length must be in 1..0xFFFF.
 * Returns a pointer to the item's Value.
 * Integer values may start with 0x00 to avoid being interpreted as a negative
 * value, but that interpretation is up to the caller.
 */
#if (TEST_VAL_NOT_USED)
uint8_t *
asn1get(
        const uint8_t * octets_p,
        size_t * itemlen_p,
        uint8_t tag)
{
    size_t len = octets_p[1];

    if(tag != octets_p[0])
    {
        LOG_CRIT("%s: tag mismatch (actual %d != %d)\n",
                 __func__, octets_p[0], tag);
        return NULL;
    }
    if(len == 0 || len == 0x80 || len > 0x82)
    {
        LOG_CRIT("%s: bad len[size] (%u)\n",
                 __func__, (unsigned int)len);
        return NULL;
    }

    if (len < 0x80)
    {
        *itemlen_p = len;
        return (uint8_t *)octets_p + 2;
    }

    if (len == 0x81)
    {
        *itemlen_p = octets_p[2];
        return (uint8_t *)octets_p + 3;
    }

    *itemlen_p = (octets_p[2] << 8) + octets_p[3];
    return (uint8_t *)octets_p + 4;
}
#endif


/*----------------------------------------------------------------------------
 * asn1put
 *
 * Simple ASN.1 encoder. Stores the next item (Tag-Length-Value triplet) at
 * 'octets_p'.  Tag must be 0x30 (Sequence) or 0x02 (Integer). Length must be
 * in 0..0xFFFF. Uses 'memmove' to copy the item, unless item_p is NULL, e.g
 * for a Sequence.
 * Returns a pointer to the location right after the stored item ('item_p != NULL)
 * or to the start of the Value ('item_p' == NULL).
 */
#if (TEST_VAL_NOT_USED)
uint8_t *
asn1put(
        uint8_t * octets_p,
        uint8_t * item_p,
        size_t itemlen,
        uint8_t tag)
{
    uint8_t * u8_p = octets_p + 2;

    if(tag != 0x30 && tag != 0x02)
    {
        LOG_CRIT("%s: bad tag (%d)\n",
                 __func__, tag);
        return NULL;
    }
    if(itemlen > 0xFFFF)
    {
        LOG_CRIT("%s: bad length (%u)\n",
                 __func__, (unsigned int)itemlen);
        return NULL;
    }

    octets_p[0] = tag;
    if (itemlen < 0x80)
    {
        octets_p[1] = (uint8_t)itemlen;
    }
    else if (itemlen < 0x100)
    {
        octets_p[1] = 0x81;
        octets_p[2] = (uint8_t)itemlen;
        u8_p += 1;
    }
    else
    {
        octets_p[1] = 0x82;
        octets_p[2] = (uint8_t)(itemlen >> 8);
        octets_p[3] = (uint8_t)(itemlen & 0xFF);
        u8_p += 2;
    }

    if (item_p != NULL)
    {
        memmove(u8_p, item_p, itemlen);
        return u8_p + itemlen;
    }

    return u8_p;
}
#endif

/* end of file valtest_internal.c */
