#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

#define PKG_USING_RT_OTA
#define RT_OTA_USING_CRYPT
#define RT_OTA_CRYPT_ALGO_USING_AES256
#define RT_OTA_USING_CMPRS
#define RT_OTA_CMPRS_ALGO_USING_QUICKLZ
#define PKG_USING_RT_OTA_LATEST_VERSION

#define QLZ_COMPRESSION_LEVEL 3
#define TINY_CRYPT_AES
#define FAL_PART_TABLE_END_OFFSET (64 * 1024UL) //speed up if need change to 64k
#define FAL_PART_TABLE_FLASH_DEV_NAME "beken_onchip_crc"
#endif
// eof

