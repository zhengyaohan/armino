/*
 * File      : rt_ota_flash_dev.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#include <rt_ota.h>
#include <rt_ota_flash_dev.h>
#include <string.h>

static const struct rt_ota_flash_dev * const device_table[] = RT_OTA_FLASH_DEV_TABLE;
static const size_t device_table_len = sizeof(device_table) / sizeof(device_table[0]);
static uint8_t init_ok = 0;

int rt_ota_flash_device_init(void)
{
    size_t i;

    if (init_ok)
    {
        return 0;
    }

    for (i = 0; i < device_table_len; i++)
    {
        assert(device_table[i]->ops.erase);
        assert(device_table[i]->ops.write);
        assert(device_table[i]->ops.read);
    }

    init_ok = 1;

    return 0;
}

const struct rt_ota_flash_dev *rt_ota_flash_device_find(const char *name)
{
    assert(init_ok);
    assert(name);

    size_t i;

    for (i = 0; i < device_table_len; i++)
    {
        if (!strcmp(name, device_table[i]->name)) {
            return device_table[i];
        }
    }

    return NULL;
}
