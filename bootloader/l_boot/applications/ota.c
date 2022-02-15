/*
 * File      : ota.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <fal.h>
#include <rt_ota.h>
#include <string.h>

#include "driver_flash.h"

#define log_ota(...)                     printf("\033[36;22m");printf(__VA_ARGS__);printf("\033[0m\r\n")

#define BOOT_OTA_DEBUG (1) // 1: open debug; 0: close debug

int ota_main(void)
{
    int result = 0;
    size_t i, part_table_size;
    const struct fal_partition *dl_part = NULL;
    const struct fal_partition *part_table = NULL;
    const char *dest_part_name = NULL;

    if (rt_ota_init() >= 0)
    {
        /* verify bootloader partition
         * 1. Check if the BL partition exists
         * 2. CRC BL FW HDR
         * 3. HASH BL FW
         * */
        if (rt_ota_part_fw_verify(fal_partition_find(RT_BK_BL_PART_NAME)) < 0)
        {
            //TODO upgrade bootloader to safe image
            // firmware HDR crc failed or hash failed. if boot verify failed, may not jump to app running
#if !BOOT_OTA_DEBUG // close debug
            return -1;
#endif
        }

        // 4. Check if the download partition exists
        dl_part = fal_partition_find(RT_BK_DL_PART_NAME);
        if (!dl_part)
        {
            log_e("download partition is not exist, please check your configuration!");
            return -1;
        }

        /* 5. Check if the target partition name is bootloader, skip ota upgrade if yes */
        dest_part_name = rt_ota_get_fw_dest_part_name(dl_part);
        if (dest_part_name && !strncmp(dest_part_name, RT_BK_BL_PART_NAME, strlen(RT_BK_BL_PART_NAME)))
        {
            log_e("Can not upgrade bootloader partition!");
            goto _app_check;
        }

        /* do upgrade when check upgrade OK 
         * 5. CRC DL FW HDR
         * 6. Check if the dest partition exists
         * 7. CRC APP FW HDR
         * 8. Compare DL and APP HDR, containning fw version
         */
        log_d("check upgrade...");
        if ((result = rt_ota_check_upgrade()) == 1) // need to upgrade
        {
            if((rt_ota_get_fw_algo(dl_part) & RT_OTA_CRYPT_STAT_MASK) == RT_OTA_CRYPT_ALGO_NONE)
            {
                log_e("none encryption Not allow!");
                goto _app_check;
            }

            /* verify OTA download partition 
            * 9. CRC DL FW HDR
            * 10. CRC DL FW
            */
            if (rt_ota_part_fw_verify(dl_part) == 0)
            {
                // 11. rt_ota_custom_verify
                // 12. upgrade
                set_flash_protect(NONE);
                if (rt_ota_upgrade() < 0)
                {
                    log_e("OTA upgrade failed!");
                    /*
                     *  upgrade failed, goto app check. If success, jump to app to run, otherwise goto recovery factory firmware.
                     **/
                    goto _app_check;
                }
            }
            else
            {
                goto _app_check;
            }
        }
        else if (result == 0)
        {
            log_d("No firmware upgrade!");
        }
        else if (result == -1)
        {
            goto _app_check;
        }
        else
        {
            log_e("OTA upgrade failed! Need to recovery factory firmware.");
            return -1;
        }

_app_check:
        part_table = fal_get_partition_table(&part_table_size);
        /* verify all partition */
        for (i = 0; i < part_table_size; i++)
        {
            /* ignore bootloader partition and OTA download partition */
            if (!strncmp(part_table[i].name, RT_BK_APP_NAME, FAL_DEV_NAME_MAX))
            {
                // verify app firmware
                if (rt_ota_part_fw_verify(&part_table[i]) < 0)
                {
                    // TODO upgrade to safe image
                    log_e("App verify failed! Need to recovery factory firmware.");
                    return -1;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    else
    {
        result = -1;
    }

    return result;
}


/*
 * CRC16 checksum
 * MSB first; Polynomial: 8005; Initial value: 0xFFFF
 * */
static unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int usDataLen)
{
    unsigned short wCRCin = 0xFFFF;
    unsigned short wCPoly = 0x8005;
    unsigned char wChar = 0;

    while (usDataLen--)
    {
        wChar = *(puchMsg++);
        wCRCin ^= (wChar << 8);
        for(int i = 0; i < 8; i++)
        {
            if(wCRCin & 0x8000)
                wCRCin = (wCRCin << 1) ^ wCPoly;
            else
                wCRCin = wCRCin << 1;
        }
    }
    return (wCRCin);
}

int rt_ota_custom_verify(const struct fal_partition *cur_part, long offset, const uint8_t *buf, size_t len)
{
    uint16_t crc1, crc2;
    uint32_t link_addr;

    /* Do not modify buf content */
    uint8_t *buffer_out = (uint8_t *)buf;
    static uint8_t _custom_verify_flag = 0;
    const struct fal_partition *app_part = NULL;
    const char *dest_name = NULL;

    /* Beken7231 custom validation rules, verify first packet */
    if ((offset < 1024) && (_custom_verify_flag == 0) && !strcmp(cur_part->name, RT_BK_DL_PART_NAME))
    {
        _custom_verify_flag = 1;
        crc1 = CRC16_CCITT_FALSE((unsigned char *)buffer_out, 32);
        crc2 =  (buffer_out[32] << 8) | buffer_out[33];

        if(crc1 == crc2)
        {
            log_e("Please use none crc image to build rbl file!\r\n");
            return -1;
        }
        else
        {
            log_d("The raw firmware crc check success.");
        }

        dest_name = rt_ota_get_fw_dest_part_name(cur_part);
        if (strcmp(dest_name, "app"))
        {
            log_i("The current part name: %s, it does not need to custom verity.", dest_name);
            return 0;
        }

        /* link script check */
        link_addr = buffer_out[32] | (buffer_out[33] << 8) | (buffer_out[34] << 16) | (buffer_out[35] << 24);
        log_d("link_addr = %x, cur_part->offset:%x, endaddr:%x", link_addr, cur_part->offset, cur_part->offset + cur_part->len);

        app_part = fal_partition_find(RT_BK_APP_NAME);
        if (!app_part)
        {
            log_e("download partition is not exist, please check your configuration!");
            return -1;
        }

        if( (link_addr < app_part->offset) || (link_addr > (app_part->offset + app_part->len)) )
        {
            log_e("The application must link to the flash 64KB location!\r\n");
            return -1;
        }
        else
        {
            log_d("The raw firmware linkaddr passed the check.");
        }
    }

    /* Add others custom validation rules. User TODO */

    return 0;
}
