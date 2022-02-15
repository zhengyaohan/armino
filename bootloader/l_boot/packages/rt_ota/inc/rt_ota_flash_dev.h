/*
 * File      : rt_ota_flash_dev.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#ifndef _RT_OTA_FLASH_DEV_H_
#define _RT_OTA_FLASH_DEV_H_

#include "rt_ota_cfg.h"
#include <stdio.h>

#ifndef RT_OTA_FLASH_DEV_NAME_MAX
#define RT_OTA_FLASH_DEV_NAME_MAX 24
#endif

#ifndef RT_OTA_FLASH_DEV_TABLE
#define RT_OTA_FLASH_DEV_TABLE { 0 }
#endif

struct rt_ota_flash_dev
{
    char name[RT_OTA_FLASH_DEV_NAME_MAX];

    /* flash device start address and len  */
    uint32_t addr;
    size_t len;

    struct
    {
        int (*read)(uint32_t offset, uint8_t *buf, size_t size);
        int (*write)(uint32_t offset, const uint8_t *buf, size_t size);
        int (*erase)(uint32_t offset, size_t size);
    } ops;
};
typedef struct rt_ota_flash_dev *rt_ota_flash_dev_t;

int rt_ota_flash_device_init(void);

#endif /* _RT_OTA_FLASH_DEV_H_ */
