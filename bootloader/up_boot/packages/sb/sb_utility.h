/*************************************************************
 * @file        sb_utility.h
 * @brief       code of secure function driver of BK7271
 * @author      Hsu Pochin
 * @version     V1.0
 * @date        2021-10-12
 * @par
 * @attention
 *
 * @history     2021-10-12 hpc    create this file
 */

#ifndef __SB_UTILITY_H__
#define __SB_UTILITY_H__



#define SB_ECDSA_BYTES		32			////256bit

/** ECDSA signature. */
typedef struct
{
    unsigned char r[SB_ECDSA_BYTES];    /** r. */
    unsigned char s[SB_ECDSA_BYTES];    /** s. */
}SB_ECDSA_Signature_t;


/** ECDSA public key. */
typedef struct
{
    unsigned char Qx[SB_ECDSA_BYTES];    /** Qx. */
    unsigned char Qy[SB_ECDSA_BYTES];    /** Qy. */
}SB_ECDSA_PublicKey_t;


/** ECDSA certificate. */
typedef struct
{
    SB_ECDSA_PublicKey_t PublicKey;    /** Public key. */
    SB_ECDSA_Signature_t Signature;    /** Signature. */
	unsigned int *pData;				/* Data */
	unsigned int wDataLen;				/* Data length*/
}SB_ECDSA_Certificate_t;


/*----------------------------------------------------------------------------
 * ap_sb_ecdsa256_verify
 *
 * This function do the ECDSA verify. Data lenth must be multiples of 64.
 *
 * *pPtr
 *     A pointer that point to the struct of SB_ECDSA_Certificate_t.
 *
 * Return Value
 *     0           Verify Pass.
 *     other value Verify Fail.
 */
int ap_sb_ecdsa256_verify(SB_ECDSA_Certificate_t *pPtr);


/*----------------------------------------------------------------------------
 * ap_sb_sha256_cal
 *
 * This function do the calculation of SHA-256. Data lenth must be multiples of 64.
 *
 * *ptr
 *     A pointer that point to start address of data.  
 *
 * len
 *     Length of data
 *
 * *pDig
 *     A pointer that point to start address of saving result(the array size must be 32Bytes)
 *
 * Return Value
 *     0           Verify Pass.
 *     other value Verify Fail.
 */
int ap_sb_sha256_cal(unsigned int *ptr, unsigned int len, unsigned char *pDig);

#if 1    ////V0.1.1,
/*----------------------------------------------------------------------------
 * ap_sb_sha256_cal_new
 *
 * This function do the calculation of SHA-256. Data lenth must be multiples of 64.
 *
 * *ptr
 *     A pointer that point to start address of data.
 *
 * len
 *     Length of data
 *
 * *pDig
 *     A pointer that point to start address of saving result(the array size must be 32Bytes)
 *
 * mode
 *	  sha256 portion calculation mode(0:Start, 1:Continue, 2:Stop)
 *
 * Return Value
 *     0           Verify Pass.
 *     other value Verify Fail.
 */

int ap_sb_sha256_cal_new(unsigned int *ptr, unsigned int len, unsigned char *pDig, unsigned char mode);

#define AP_SB_SHA256_CAL_START(ptr, len, pDig)		ap_sb_sha256_cal_new(ptr, len, pDig, 0)
#define AP_SB_SHA256_CAL_CONTINUE(ptr, len, pDig)	ap_sb_sha256_cal_new(ptr, len, pDig, 1)
#define AP_SB_SHA256_CAL_STOP(ptr, len, pDig)		ap_sb_sha256_cal_new(ptr, len, pDig, 2)

#endif

/*----------------------------------------------------------------------------
 * ap_sb_version_read
 *
 * This function read the value of latest version in the IC.
 *
 * *Version
 *     A pointer that point to start address of saving result.  
 *
 * Return Value
 *     0           Verify Pass.
 *     other value Verify Fail.
 */
int ap_sb_version_read(unsigned int *Version);


/*----------------------------------------------------------------------------
 * ap_sb_version_increase
 *
 * This function increse the value of version by 1 and save in the IC.
 *
 * Return Value
 *     0           Verify Pass.
 *     other value Verify Fail.
 */
int ap_sb_version_increase(void);

#endif	////__SB_UTILITY_H__
