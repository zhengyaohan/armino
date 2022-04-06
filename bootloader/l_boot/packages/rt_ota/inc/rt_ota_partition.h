/*
 * File      : rt_ota_partition.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#ifndef _RT_OTA_PARTITION_H_
#define _RT_OTA_PARTITION_H_

#include "rt_ota_cfg.h"
#include "rt_ota_flash_dev.h"

#if RT_OTA_PART_HAS_TABLE_CFG

/* partition table */
#ifndef RT_OTA_PART_TABLE
#define RT_OTA_PART_TABLE              { 0 }
#endif

#endif /* RT_OTA_PART_HAS_TABLE_CFG */

#ifndef RT_OTA_PART_NAME_MAX
#define RT_OTA_PART_NAME_MAX           24
#endif

/* partition magic word */
#define RT_OTA_PART_MAGIC_WROD         0x45503130

/**
 * OTA partition
 */
struct rt_ota_partition
{
    uint32_t magic_word;

    /* partition name */
    char name[RT_OTA_PART_NAME_MAX];
    /* flash device name for partition */
    char flash_name[RT_OTA_FLASH_DEV_NAME_MAX];

    /* partition offset address on flash device */
    uint32_t offset;
    size_t len;

    uint8_t reserved;
};
typedef struct rt_ota_partition *rt_ota_partition_t;

int rt_ota_partition_init(void);

#endif /* _RT_OTA_PARTITION_H_ */
