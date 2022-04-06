/*
 * File      : rt_ota_cfg.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 * 
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-23     armink       the first version
 * 2018-02-03     Myrphy       Adapted beken7231 board feature
 */
/* 
 * Note: 
 * Beken7231 program code is stored in ROM, the address range is 0x0-0x200000(total size 2Mbytes).
 * Code storage features on flash: Add 2 bytes of checksum every 32 bytes using crc16.
 * Address translation: flash storage addr is physical address; CPU execution address is logical address.
 *                      [physical_addr = logical_addr/32*34]
 * 
 * Flash Table: <Logical address>   <Physical address>
 * +------------+  0x0000000          0x00000000
 * |vectors(32b)|
 * |            |
 * |------------+  boot: 64k          68K
 * |Bootloader  |
 * |            |
 * |------------+  
 * | Boot header|                     0xFFA0
 * |   (96B)    | 
 * +------------+  0x0010000          0x11000   #Note: [0x11000] - 4K aligned; 32 aligned; 34 aligned
 * |vectors(32b)|
 * |            |
 * |------------+
 * |text        |
 * |data        |  app: 928k          986k
 * |            |
 * |------------+  
 * | app header |  0xF7FA0            0x10779A  #Note: [0x10779A] - 34 aligned
 * |  (96+6)B   | 
 * +------------+  0xF8000            0x107800  #Note: [0x107800] - 32 aligned; 34 aligned
 * +------------+  0x108000           0x108000  #Note: [0x108000] - 4K aligned
 * | DL  header |
 * |   (96B)    |
 * +------------+  download: 980k
 * |     OTA    |
 * |   PACKAGE  |  
 * |            |
 * +------------+  0x1FD000           0x1FD000
 * +------------+  0X1FD000           0x1FD000  #Note: [0x1FD000] - 4K aligned
 * |User        |
 * |Param       |  UserParam: 4k
 * |            |
 * +------------+  0x1FE000           0x1FE000
 * +------------+  0X1FE000           0x1FE000  #Note: [0x1FE000] - 4K aligned
 * |User        |
 * |Param       |  UserParamBak: 4k
 * |Bak         |
 * +------------+  0x1FF000           0x1FF000
 * +------------+  0x1FF000           0x1FF000
 * |  Sys Param |
 * |   (WiFi)   |  SysParam: 4k
 * |            |
 * +------------+  0x200000           0x200000
*/

#ifndef _RT_OTA_CFG_H_
#define _RT_OTA_CFG_H_

#include <board.h>

#define RT_OTA_MALLOC                  user_malloc
#define RT_OTA_REALLOC                 user_realloc
#define RT_OTA_FREE                    user_free

#define RT_OTA_FLASH_PORT_DRIVER_BEKEN
#define RT_OTA_FLASH_PORT_DRIVER_BEKEN_CRC

/* ========================== Debug Configuration =========================== */
#define RT_OTA_DEBUG                   1

/* ===================== Flash device Configuration ========================= */
extern const struct rt_ota_flash_dev beken_onchip_flash;
extern const struct rt_ota_flash_dev beken_onchip_flash_crc;

/* flash device table */
#define RT_OTA_FLASH_DEV_TABLE                                       \
{                                                                    \
    &beken_onchip_flash,                                             \
    &beken_onchip_flash_crc,                                         \
}

/* ====================== Partition Configuration ========================== */
/* has partition table configuration will defined 1 when running on bootloader */
#define RT_OTA_PART_HAS_TABLE_CFG      1

/* flash start addr */
#define RT_OTA_FLASH_START_ADDR        0x00000000
#define RT_OTA_FLASH_SIZE              (2 * 1024 * 1024)               // unit: bytes, total 2Mbytes

/* the flash device name which saved bootloader */
#define RT_OTA_BL_FLASH_DEV_NAME       "beken_onchip"
#define RT_OTA_BL_FLASH_DEV_CRC_NAME   "beken_onchip_crc"

/* bootloader partition name */
#define RT_OTA_BL_PART_NAME            "bootloader"

/* bootloader partition offset address */
#define RT_OTA_BL_PART_OFFSET          0
/* bootloader partition length */
#define RT_OTA_BL_PART_LEN             (64 * 1024)

#define RT_OTA_APP_NAME                "app"
#define RT_OTA_APP_PART_ADDR           (RT_OTA_BL_PART_OFFSET + RT_OTA_BL_PART_LEN) // 0x10000
#define RT_OTA_APP_PART_LEN            (928 * 1024)                                 // 928kbytes
#define RT_OTA_APP_PART_END_ADDR       (RT_OTA_APP_PART_ADDR + RT_OTA_APP_PART_LEN) // 0xF8000

#define RT_OTA_DL_PART_ADDR            (0x108000)
#define RT_OTA_DL_PART_LEN             (980 * 1024)                                 // 980kbytes
#define RT_OTA_DL_PART_END_ADDR        (RT_OTA_DL_PART_ADDR + RT_OTA_DL_PART_LEN)   // 0x1FD000

#if RT_OTA_PART_HAS_TABLE_CFG
/* partition table of rt_ota_partition struct, app table need crc_write mode */
#define RT_OTA_PART_TABLE                                            \
{                                                                    \
    {RT_OTA_PART_MAGIC_WROD, RT_OTA_BL_PART_NAME, RT_OTA_BL_FLASH_DEV_NAME, RT_OTA_BL_PART_OFFSET, RT_OTA_BL_PART_LEN, 0}, \
    {RT_OTA_PART_MAGIC_WROD, RT_OTA_APP_NAME, RT_OTA_BL_FLASH_DEV_CRC_NAME, RT_OTA_APP_PART_ADDR, RT_OTA_APP_PART_LEN, 0},  \
    {RT_OTA_PART_MAGIC_WROD, RT_OTA_DL_PART_NAME, RT_OTA_BL_FLASH_DEV_NAME, RT_OTA_DL_PART_ADDR, RT_OTA_DL_PART_LEN, 0}, \
}
#endif /* RT_OTA_PART_HAS_TABLE_CFG */


#endif /* _RT_OTA_CFG_H_ */
