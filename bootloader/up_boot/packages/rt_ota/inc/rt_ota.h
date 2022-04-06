/*
 * File      : rt_ota.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#ifndef _RT_OTA_H_
#define _RT_OTA_H_

#include "rt_ota_cfg.h"
#include "rtconfig.h"
#include "rt_ota_flash_dev.h"
#include "rt_ota_partition.h"

#define RT_OTA_SW_VERSION              "0.1.0"

/* OTA download partition name */
#ifndef RT_OTA_DL_PART_NAME
#define RT_OTA_DL_PART_NAME            "download"
#endif

#ifndef RT_OTA_MALLOC
#define RT_OTA_MALLOC                  malloc
#endif

#ifndef RT_OTA_REALLOC
#define RT_OTA_REALLOC                 realloc
#endif

#ifndef RT_OTA_FREE
#define RT_OTA_FREE                    free
#endif

/* OTA utils fnv-hash http://create.stephan-brumme.com/fnv-hash/ */
#define RT_OTA_HASH_FNV_SEED           0x811C9DC5

/**
 * OTA firmware encryption algorithm and compression algorithm
 */
enum rt_ota_algo
{
    RT_OTA_CRYPT_ALGO_NONE    = 0L,               /**< no encryption algorithm and no compression algorithm */
    RT_OTA_CRYPT_ALGO_XOR     = 1L,               /**< XOR encryption */
    RT_OTA_CRYPT_ALGO_AES256  = 2L,               /**< AES256 encryption */
    RT_OTA_CMPRS_ALGO_GZIP    = 1L << 8,          /**< Gzip: zh.wikipedia.org/wiki/Gzip */
    RT_OTA_CMPRS_ALGO_QUICKLZ = 2L << 8,          /**< QuickLZ: www.quicklz.com */
};
typedef enum rt_ota_algo rt_ota_algo_t;

/**
 * OTA '.rbl' file header, also as firmware information header
 */
struct rt_ota_rbl_hdr
{
    char magic[4];

    rt_ota_algo_t algo;
    uint32_t timestamp;
    char name[16];
    char version[24];

    char sn[24];

    /* crc32(aes(zip(rtt.bin))) */
    uint32_t crc32;
    /* hash(rtt.bin) */
    uint32_t hash;

    /* len(rtt.bin) */
    uint32_t size_raw;
    /* len(aes(zip(rtt.bin))) */
    uint32_t size_package;

    /* crc32(rbl_hdr - info_crc32) */
    uint32_t info_crc32;
};
typedef struct rt_ota_rbl_hdr *rt_ota_rbl_hdr_t;

/* OTA API */
int rt_ota_init(void);
int rt_ota_get_fw_hdr(const char *part_name, struct rt_ota_rbl_hdr *hdr);
uint32_t rt_ota_get_save_rbl_body_addr(void);
int rt_ota_save_fw_hdr(const char *part_name, struct rt_ota_rbl_hdr *hdr);
struct rt_ota_partition *rt_ota_get_dl_part(void);
int rt_ota_part_fw_verify(const struct rt_ota_partition *part);
int rt_ota_part_fw_verify_all(void);
int rt_ota_check_upgrade(void);
int rt_ota_upgrade(void);

/* partition operator for OTA */
const struct rt_ota_partition *rt_ota_partition_find(const char *name);
const struct rt_ota_partition *rt_ota_get_partition_table(size_t *len);
int rt_ota_partition_read(const struct rt_ota_partition *part, uint32_t addr, uint8_t *buf, size_t size);
int rt_ota_partition_write(const struct rt_ota_partition *part, uint32_t addr, const uint8_t *buf, size_t size);
int rt_ota_partition_erase(const struct rt_ota_partition *part, uint32_t addr, size_t size);
int rt_ota_partition_erase_all(const struct rt_ota_partition *part);

/* flash device for OTA */
const struct rt_ota_flash_dev *rt_ota_flash_device_find(const char *name);

/* OTA utils */
uint32_t rt_ota_calc_crc32(uint32_t crc, const void *buf, size_t len);
uint32_t rt_ota_calc_hash(uint32_t hash, const void *buf, size_t len);

#endif /* _RT_OTA_H_ */
