/*
 * File      : rt_ota_partition.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 */

#include <rt_ota.h>
#include <string.h>
#include <stdlib.h>

#if !defined(RT_OTA_BL_FLASH_DEV_NAME)
#error "You must defined RT_OTA_BL_FLASH_DEV_NAME on 'rt_ota_cfg.h'"
#endif

#if !defined(RT_OTA_BL_PART_OFFSET) || !defined(RT_OTA_BL_PART_LEN)
#error "You must defined RT_OTA_BL_PART_OFFSET and RT_OTA_BL_PART_LEN on 'rt_ota_cfg.h'"
#endif


/* partition table end offset address */
#define PART_TABLE_END_OFFSET          (RT_OTA_BL_PART_OFFSET + RT_OTA_BL_PART_LEN - sizeof(struct rt_ota_rbl_hdr))

#if RT_OTA_PART_HAS_TABLE_CFG
#ifdef __CC_ARM                         /* ARM Compiler */
    #define SECTION(x)                  __attribute__((section(x)))
#elif defined (__IAR_SYSTEMS_ICC__)     /* for IAR Compiler */
    #define SECTION(x)                  @ x
#elif defined (__GNUC__)                /* GNU GCC Compiler */
    #define SECTION(x)                  __attribute__((section(x)))
#else
    #error not supported tool chain
#endif
static const struct rt_ota_partition partition_table_def[] SECTION("RtOtaPart") = RT_OTA_PART_TABLE;
//static const struct rt_ota_partition partition_table_def[] = RT_OTA_PART_TABLE;
static const struct rt_ota_partition *partition_table = NULL;
#else
static struct rt_ota_partition *partition_table = NULL;
#endif /* RT_OTA_PART_HAS_TABLE_CFG */

static uint8_t init_ok = 0;
static size_t partition_table_len = 0;

/**
 * partition initialization
 *
 * @return partitions total number
 */
int rt_ota_partition_init(void)
{
    size_t i;

    if (init_ok)
    {
        return partition_table_len;
    }

#if RT_OTA_PART_HAS_TABLE_CFG
    partition_table = &partition_table_def[0];
    partition_table_len = sizeof(partition_table_def) / sizeof(partition_table_def[0]);
#else
    /* load partition table from the end address PART_TABLE_END_OFFSET, error return 0 */
    size_t table_num = 0, table_item_size = 0;
    rt_ota_partition_t new_part = NULL;
    const struct rt_ota_flash_dev *flash_dev = NULL;

    flash_dev = rt_ota_flash_device_find(RT_OTA_BL_FLASH_DEV_NAME);
    if (flash_dev == NULL)
    {
        log_e("Initialize failed! Don't found flash device(%s).", RT_OTA_BL_FLASH_DEV_NAME);
        goto _exit;
    }

    table_item_size = sizeof(struct rt_ota_partition);
    new_part = (rt_ota_partition_t)RT_OTA_MALLOC(table_item_size);
    if (new_part == NULL)
    {
        log_e("Initialize failed! No memory for table buffer.");
        goto _exit;
    }

    do
    {
        memset(new_part, 0x00, table_num);
        if (flash_dev->ops.read(PART_TABLE_END_OFFSET - table_item_size * (table_num + 1), (uint8_t *) new_part,
                table_item_size) < 0)
        {
            log_e("Initialize failed! Bootloader flash device read error!");
            table_num = 0;
            break;
        }

        if (new_part->magic_word != RT_OTA_PART_MAGIC_WROD)
        {
            break;
        }

        partition_table = (rt_ota_partition_t) RT_OTA_REALLOC(partition_table, table_item_size * (table_num + 1));
        if (partition_table == NULL)
        {
            log_e("Initialize failed! No memory for partition table");
            table_num = 0;
            break;
        }

        memcpy(partition_table + table_num * table_item_size, new_part, table_item_size);

        table_num++;
    } while (1);

    if (table_num == 0)
    {
        goto _exit;
    }
    else
    {
        partition_table_len = table_num;
    }
#endif /* RT_OTA_PART_HAS_TABLE_CFG */

    /* check the partition table device exists */
    const struct rt_ota_flash_dev *fl_dev = NULL;

    for (i = 0; i < partition_table_len; i++)
    {
        fl_dev = rt_ota_flash_device_find(partition_table[i].flash_name);
        if (fl_dev == NULL)
        {
            log_e("Initialize failed! Don't found the flash device(%s).", partition_table[i].flash_name);
            partition_table_len = 0;
            goto _exit;
        }

        if (partition_table[i].offset >= fl_dev->len)
        {
            log_e("Initialize failed! Partition(%s) offset address(%ld) out of flash bound(%ld).",
                    partition_table[i].name, partition_table[i].offset, fl_dev->len);
            partition_table_len = 0;
            goto _exit;
        }
    }

    init_ok = 1;

_exit:

#if !RT_OTA_PART_HAS_TABLE_CFG
    if (new_part)
    {
        RT_OTA_FREE(new_part);
    }
#endif /* !RT_OTA_PART_HAS_TABLE_CFG */

    return partition_table_len;
}

const struct rt_ota_partition *rt_ota_partition_find(const char *name)
{
    assert(init_ok);

    size_t i;

    for (i = 0; i < partition_table_len; i++)
    {
        if (!strcmp(name, partition_table[i].name))
        {
            return &partition_table[i];
        }
    }

    return NULL;
}

const struct rt_ota_partition *rt_ota_get_partition_table(size_t *len)
{
    assert(init_ok);
    assert(len);

    *len = partition_table_len;

    return partition_table;
}

/**
 * read data from partition
 *
 * @param part parttion
 * @param addr relative address for partition
 * @param buf read buffer
 * @param size read size
 *
 * @return -1: error
 */
int rt_ota_partition_read(const struct rt_ota_partition *part, uint32_t addr, uint8_t *buf, size_t size)
{
    int ret = 0;
    const struct rt_ota_flash_dev *flash_dev = NULL;

    assert(part);
    assert(buf);

    if (addr + size > part->len)
    {
        log_e("Partition read error! Partition address out of bound.");
        return -1;
    }

    flash_dev = rt_ota_flash_device_find(part->flash_name);
    if (flash_dev == NULL)
    {
        log_e("Partition read error! Don't found flash device(%s) of the partition(%s).", part->flash_name, part->name);
        return -1;
    }

    ret = flash_dev->ops.read(part->offset + addr, buf, size);
    if (ret < 0)
    {
        log_e("Partition read error! Flash device(%s) read error!", part->flash_name);
    }

    return ret;
}

/**
 * write data to partition
 *
 * @param part parttion
 * @param addr relative address for partition
 * @param buf write buffer
 * @param size write size
 *
 * @return -1: error
 */
int rt_ota_partition_write(const struct rt_ota_partition *part, uint32_t addr, const uint8_t *buf, size_t size)
{
    int ret = 0;
    const struct rt_ota_flash_dev *flash_dev = NULL;

    assert(part);
    assert(buf);

    if (addr + size > part->len)
    {
        log_e("Partition write error! Partition address out of bound.");
        return -1;
    }

    flash_dev = rt_ota_flash_device_find(part->flash_name);
    if (flash_dev == NULL)
    {
        log_e("Partition write error!  Don't found flash device(%s) of the partition(%s).", part->flash_name, part->name);
        return -1;
    }

    ret = flash_dev->ops.write(part->offset + addr, buf, size);
    if (ret < 0)
    {
        log_e("Partition write error! Flash device(%s) write error!", part->flash_name);
    }

    return ret;
}

/**
 * erase partition data
 *
 * @param part parttion
 * @param addr relative address for partition
 * @param size erase size
 *
 * @return -1: error
 */
int rt_ota_partition_erase(const struct rt_ota_partition *part, uint32_t addr, size_t size)
{
    int ret = 0;
    const struct rt_ota_flash_dev *flash_dev = NULL;

    assert(part);

    if (addr + size > part->len)
    {
        log_e("Partition erase error! Partition address out of bound.");
        return -1;
    }

    flash_dev = rt_ota_flash_device_find(part->flash_name);
    if (flash_dev == NULL)
    {
        log_e("Partition erase error! Don't found flash device(%s) of the partition(%s).", part->flash_name, part->name);
        return -1;
    }

    ret = flash_dev->ops.erase(part->offset + addr, size);
    if (ret < 0)
    {
        log_e("Partition erase error! Flash device(%s) erase error!", part->flash_name);
    }

    return ret;
}

/**
 * erase partition all data
 *
 * @param part parttion
 *
 * @return -1: error
 */
int rt_ota_partition_erase_all(const struct rt_ota_partition *part)
{
    return rt_ota_partition_erase(part, 0, part->len);
}
