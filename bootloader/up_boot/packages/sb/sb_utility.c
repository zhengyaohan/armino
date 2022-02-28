/*************************************************************
 * @file        sb_utility.c
 * @brief       code of secure function driver of BK7271
 * @author      Hsu Pochin
 * @version     V1.0
 * @date        2021-10-12
 * @par
 * @attention
 *
 * @history     2021-10-12 hpc    create this file
 */
#include "BK_config.h"

#include "sb_utility.h"

//#define SB_FUNCTION_TEST_ENABLE	1
#if(SB_FUNCTION_TEST_ENABLE == 1)
	#include <stdio.h>
	#include <string.h>
	
	#define uint8_t unsigned char
	#define uint32_t unsigned int
	
	extern void bk_printf(const char *fmt, ...);
	
	static const uint8_t
	SHA256_64_bytes_msg [] = {
		0x35, 0x92, 0xec, 0xfd, 0x1e, 0xac, 0x61, 0x8f, 0xd3, 0x90, 0xe7, 0xa9,
		0xc2, 0x4b, 0x65, 0x65, 0x32, 0x50, 0x93, 0x67, 0xc2, 0x1a, 0x0e, 0xac,
		0x12, 0x12, 0xac, 0x83, 0xc0, 0xb2, 0x0c, 0xd8, 0x96, 0xeb, 0x72, 0xb8,
		0x01, 0xc4, 0xd2, 0x12, 0xc5, 0x45, 0x2b, 0xbb, 0xf0, 0x93, 0x17, 0xb5,
		0x0c, 0x5c, 0x9f, 0xb1, 0x99, 0x75, 0x53, 0xd2, 0xbb, 0xc2, 0x9b, 0xb4,
		0x2f, 0x57, 0x48, 0xad
	};
	
	static const uint8_t
	SHA256_64_bytes_digest [] = {
		0x10, 0x5a, 0x60, 0x86, 0x58, 0x30, 0xac, 0x3a, 0x37, 0x1d, 0x38, 0x43,
		0x32, 0x4d, 0x4b, 0xb5, 0xfa, 0x8e, 0xc0, 0xe0, 0x2d, 0xda, 0xa3, 0x89,
		0xad, 0x8d, 0xa4, 0xf1, 0x02, 0x15, 0xc4, 0x54
	};

	unsigned int sb_vault_header_data[66]=
	{
		0x02704c42,0x03000000,0xea826e12,0x0b609eea,
		0x98aa4659,0x4997734b,0xa4f719e2,0x87110438,
		0x4ad3a354,0x5db26bae,0xc95720b6,0xe95e5461,
		0x68f32411,0x7559cd60,0x83afe066,0x70a97724,
		0x538cdd82,0xc4826788,
		////Pubkey present 
			                  0xbce4f9bc,0x76979838,
		0xedc80be0,0xea050f59,0x7e382b92,0x290c2c2b,
		0xc02250b6,0x249e2865,0x2543b188,0xf12a2d6c,
		0x9f40377e,0x24a9d5d3,0xf8e3fc88,0x17311e85,
		0x4c146c24,0x995af150,
		////
							  0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,

		//0x40000000,0x01000000,0x00000000,0x00000000,
		0x00000000,
	#if 1	////No Attr
				   0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,
	#else
				   0x01000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,
		0x00000000,
	#endif	
				   0x00000000
	};
	
	unsigned int sb_img_data[128]=
	{
		0x01000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x64636261, 0x68676665, 0x6C6B6A69, 0x706F6E6D,
		0x74737271, 0x78777675, 0x32317A79, 0x36353433,
		0x30393837, 0x44434241, 0x48474645, 0x4C4B4A49,
		0x504F4E4D, 0x54535251, 0x58575655, 0x62615A59
	};
#endif

void app_sb_function_test(void)
{
#if(SB_FUNCTION_TEST_ENABLE == 1)	
	SB_ECDSA_Certificate_t sb_ecdsa_data;
	int tst_res = 0xFF;
	uint8_t SHA_Digest[32] = {0};
	uint32_t version = 0xFF;
	uint8_t tmpCnt = 0;
	
bk_printf("%s \r\n", __FUNCTION__);
 #if 0    ////Do ECDSA Demo
	////ECDSA Verify
	sb_ecdsa_data.pData = sb_img_data;
	sb_ecdsa_data.wDataLen = 128;
	memcpy(&sb_ecdsa_data.Signature.r, (uint8_t *)(&sb_vault_header_data[2]), 64);
	memcpy(&sb_ecdsa_data.PublicKey.Qx, (uint8_t *)(&sb_vault_header_data[18]), 64);

	#if 0	////Debug info
	uint32_t *ptr;
	bk_printf("ImageData: \r\n    ");
	ptr = sb_img_data;
	for(tmpCnt=0; tmpCnt<32; )
	{
		bk_printf("0x%8X ", ptr[tmpCnt++]);
		if((tmpCnt%4) == 0)
			bk_printf("\r\n    ");
	}
	bk_printf("Signature: \r\n    ");
	ptr = (uint32_t *)sb_ecdsa_data.Signature.r;
	for(tmpCnt=0; tmpCnt<16; )
	{
		bk_printf("0x%8X ", ptr[tmpCnt++]);
		if((tmpCnt%4) == 0)
			bk_printf("\r\n    ");
	}
	bk_printf("Pubkey: \r\n    ");
	ptr = (uint32_t *)sb_ecdsa_data.PublicKey.Qx;
	for(tmpCnt=0; tmpCnt<16; )
	{
		bk_printf("0x%8X ", ptr[tmpCnt++]);
		if((tmpCnt%4) == 0)
			bk_printf("\r\n    ");
	}
	#endif

	tst_res = ap_sb_ecdsa256_verify(&sb_ecdsa_data);
	bk_printf("ap_sb_ecdsa256_verifys() = %d \r\n", tst_res);
#endif    ////Do ECDSA Demo

 #if 1   ////Do SHA-256 Demo
	////SHA-256
  #if 1	////4MB
	////calculate 4MB(all 0xFF, asume flash 0x0x100000 - 0x200000 are all 0xFF, SHA this area 4 times)
    ////Result:CD3517473707D59C3D915B52A3E16213CADCE80D9FFB2B4371958FB7ACB51A08
	tst_res = AP_SB_SHA256_CAL_START((uint32_t *)0x100000, 0x100000, SHA_Digest);
	bk_printf("ap_sb_sha256_cal_Start() = %d\r\nDigest:\r\n", tst_res);
	tst_res = AP_SB_SHA256_CAL_CONTINUE((uint32_t *)0x100000, 0x100000, SHA_Digest);
	bk_printf("ap_sb_sha256_cal_Continue(1) = %d\r\nDigest:\r\n", tst_res);
	tst_res = AP_SB_SHA256_CAL_CONTINUE((uint32_t *)0x100000, 0x100000, SHA_Digest);
	bk_printf("ap_sb_sha256_cal_Continue(2) = %d\r\nDigest:\r\n", tst_res);
	tst_res = AP_SB_SHA256_CAL_STOP((uint32_t *)0x100000, 0x100000, SHA_Digest);
	bk_printf("ap_sb_sha256_cal_Stop() = %d\r\nDigest:\r\n", tst_res);
  #else ////64Bytes
	tst_res = ap_sb_sha256_cal((uint32_t *)SHA256_64_bytes_msg, 64, SHA_Digest);
	bk_printf("ap_sb_sha256_cal() = %d - %d\r\nDigest:\r\n", tst_res, 
						memcmp(SHA256_64_bytes_digest, SHA_Digest, 32));
  #endif
	for(tmpCnt=0; tmpCnt<32; tmpCnt+=8)
	{	
		bk_printf("  %2X %2X %2X %2X %2X %2X %2X %2X \r\n", 
		SHA_Digest[tmpCnt], SHA_Digest[tmpCnt+1], SHA_Digest[tmpCnt+2], SHA_Digest[tmpCnt+3], 
		SHA_Digest[tmpCnt+4], SHA_Digest[tmpCnt+5], SHA_Digest[tmpCnt+6], SHA_Digest[tmpCnt+7] );
	}
 #endif   ////Do SHA-256 Demo
 #if 0    ////Do Version Demo
	////Version test
	tst_res = ap_sb_version_read(&version);
	bk_printf("ap_sb_version_read() = %d , Version = %d\r\n", tst_res, version);
	
	tst_res = ap_sb_version_increase();
	bk_printf("ap_sb_version_increase() = %d \r\n", tst_res);
	
	tst_res = ap_sb_version_read(&version);
	bk_printf("ap_sb_version_read() = %d , Version = %d\r\n", tst_res, version);
 #endif    ////Do Version Demo
#endif	
}


