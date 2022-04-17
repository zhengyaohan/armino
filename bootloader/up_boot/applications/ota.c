/*
 * File      : ota.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <fal.h>
#include <string.h>
#include "BK_System.h"
#include "driver_flash.h"
#include "interrupt.h"

#if CFG_BEKEN_OTA
#include "data_move.h"
#endif

#define log_ota(...)                     printf(__VA_ARGS__);printf("\r\n")

/* 1: open debug; 0: close debug*/
#define BOOT_OTA_DEBUG       (1) 

int ota_print = 0;

extern u32 ota_over ;
extern u32 app_bin_start_addr ;
extern u32 app_bin_len ;

extern void upgrade_boot_flash_earse(void);

/*
 * CRC16 checksum
 * MSB first; Polynomial: 8005; Initial value: 0xFFFF
 * */
unsigned short CRC16_CCITT_FALSE(unsigned char *puchMsg, unsigned int usDataLen)
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

int ota_erase_dl_rbl(void )
{
    const struct fal_partition *part_table = NULL;
    size_t i, part_table_size;

    //erase ota sector 2
    part_table = fal_get_partition_table(&part_table_size);
    /* verify all partition */
    for (i = 0; i < part_table_size; i++)
    {
        /* ignore bootloader partition and OTA download partition */
        if (!strncmp(part_table[i].name, RT_BK_DL_PART_NAME, FAL_DEV_NAME_MAX))
        {
            log_e("erase:%x %x\r\n",part_table[i].offset,part_table[i].len);
            // verify ota firmware
            flash_erase_sector(part_table[i].offset);
            upgrade_boot_flash_earse();
        }
    }

    ota_over = 2;
		
		return 0;
}

int ota_get_app_len(void )
{
    const struct fal_partition *part_table = NULL;
    size_t i, part_table_size;

    //erase ota sector 2
    part_table = fal_get_partition_table(&part_table_size);
    /* verify all partition */
    for (i = 0; i < part_table_size; i++)
    {
        /* ignore bootloader partition and OTA download partition */
        if (!strncmp(part_table[i].name, RT_BK_APP_NAME, FAL_DEV_NAME_MAX))
        {
            app_bin_start_addr = part_table[i].offset*34/32;
            app_bin_len = part_table[i].len*34/32;
        }
    }
		
		return 0;
}

#if CFG_RTT_OTA_ORIGINAL
int ota_main(void)
{
    int result = 0;
    const struct fal_partition *dl_part = NULL;
    const char *dest_part_name = NULL;

    ota_print = 1;
    result = rt_ota_init();
    ota_print = 0;
    
    if (result >= 0)
    {
        /* verify bootloader partition
         * 1. Check if the BL partition exists
         * 2. CRC BL FW HDR
         * 3. HASH BL FW
         * */
        if (rt_ota_part_fw_verify_header(fal_partition_find(RT_BK_BL_PART_NAME)) < 0)
        {
            //TODO upgrade bootloader to safe image
            // firmware HDR crc failed or hash failed. if boot verify failed, may not jump to app running
#if !BOOT_OTA_DEBUG // close debug
            return -1;
#endif
        }

        ota_get_app_len();
            
        // 4. Check if the download partition exists
        dl_part = fal_partition_find(RT_BK_DL_PART_NAME);
        if (!dl_part)
        {
            log_e("# par not exist");
            return -1;
        }
        /* 5. Check if the target partition name is bootloader, skip ota upgrade if yes */
        dest_part_name = rt_ota_get_fw_dest_part_name(dl_part);
        if (dest_part_name && !strncmp(dest_part_name, RT_BK_BL_PART_NAME, strlen(RT_BK_BL_PART_NAME)))
        {
            log_e("#");
            goto _app_check;
        }

        /* do upgrade when check upgrade OK 
         * 5. CRC DL FW HDR
         * 6. Check if the dest partition exists
         * 7. CRC APP FW HDR
         * 8. Compare DL and APP HDR, containning fw version
         */
        log_d("check...");
        if ((result = rt_ota_check_upgrade()) == 1) // need to upgrade
        {
            boot_ota_pre_process();

            /* verify OTA download partition 
            * 9. CRC DL FW HDR
            * 10. CRC DL FW
            */
            if (rt_ota_part_fw_verify(dl_part) == 0)
            {
                // 11. rt_ota_custom_verify
                // 12. upgrade
                ota_over = 3;
                flash_set_protect(NONE);
                if (rt_ota_upgrade() < 0)
                {
                    log_e("OTA upgraded");
                    /*
                     *  upgrade failed, goto app check. If success, jump to app to run, otherwise goto recovery factory firmware.
                     **/
                    if(1 == ota_over)
                        {
                        ota_erase_dl_rbl();
                    }
                    flash_set_protect(ALL);
                    goto _app_check;
                }
                ota_erase_dl_rbl();
                flash_set_protect(ALL);
            }
            else
            {
                flash_set_protect(NONE);
                ota_erase_dl_rbl();
                flash_set_protect(ALL);
                goto _app_check;
            }
        }
        else if (result == 0)
        {
            log_d("#");
        }
        else if (result == -1)
        {
            goto _app_check;
        }
        else
        {
            log_e("#");
            return -1;
        }

_app_check:
        ;
    }
    else
    {
        result = -1;
    }

    return result;
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
            log_e("#");
            return -1;
        }
        else
        {
            log_d("#");
        }

        dest_name = rt_ota_get_fw_dest_part_name(cur_part);
        if (strcmp(dest_name, "app"))
        {
            log_i("# %s", dest_name);
            return 0;
        }

        /* link script check */
        link_addr = buffer_out[32] | (buffer_out[33] << 8) | (buffer_out[34] << 16) | (buffer_out[35] << 24);
        log_d("#  %x, %x, %x", link_addr, cur_part->offset, cur_part->offset + cur_part->len);

        app_part = fal_partition_find(RT_BK_APP_NAME);
        if (!app_part)
        {
            log_e("#");
            return -1;
        }

        if( (link_addr < app_part->offset) || (link_addr > (app_part->offset + app_part->len)) )
        {
            log_e("#");
            return -1;
        }
        else
        {
            log_d("#");
        }
    }

    /* Add others custom validation rules. User TODO */

    return 0;
}
#endif

#if CFG_BEKEN_OTA
//int ota_main(void)
int __attribute__((section(".itcm_write_flash"))) ota_main(void)
{
	int ret;
	ret = fal_init();
	if (ret == -1)
    {
    	bk_printf("partition table not found!\r\n");
		return ret;
    }else if(ret == -2){
		bk_printf("download partition not found!\r\n");
		return ret;
	}

	bk_printf("ota_main start \r\n");
#if (CFG_SOC_NAME != SOC_BK7256)
	sys_forbidden_interrupts();
#endif

	set_flash_protect(NONE);
	ret = data_move_handler();
	bk_printf("\r\n");
	bk_printf("data_move_handler return -");
	bk_print_hex(-ret);
	bk_printf("\r\n");

	if (ret == RET_DM_FAILURE)
    {
    	bk_printf("RET_DM_FAILURE!\r\n");
		return ret;
    }else if(ret == RET_DM_NO_OTA_DATA){
		bk_printf("download partition NO_OTA_DATA!\r\n");
		return ret;
	}
	
	set_flash_protect(ALL);
	return 0;
}
#endif

// eof

