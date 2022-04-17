#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "drv_timer.h"
#include "drv_pwm.h"
#include "driver_flash.h"
#include "crc.h"
#include "interrupt.h"

#define SYSTEM_TIMEOUT_STARTUP   (400)

#if (CFG_SUPPORT_OS == OS_ALIOS)
typedef struct img_head
{
#if OTA_TWO_IMAGE_SWITCH_RUN
	u32 image_adr;
#else
    u32 dst_adr;
    u32 src_adr;
    u32 siz;
    u16 crc;
#endif
} __attribute__((packed)) IMG_HEAD, *IMG_HEAD_P;
#elif (CFG_SUPPORT_OS == OS_FREERTOS)

typedef struct img_head
{
    u32 bkup_addr;
    u32 bkup_len;
    u32 ex_addr;
    u32 os_addr;
    u32 hd_addr;
    u32 crc;
    u32 status;
    u32 bk;

} IMG_HEAD, *IMG_HEAD_P;

typedef struct boot_param_head
{
    IMG_HEAD i_hd;
    u32 jump_addr;
} BOOT_PARAM_HD, *BOOT_PARAM_HD_P;
#endif

u32 boot_downloading = TRUE;
u32 startup = FALSE;

extern u32 uart_download_status;

#if (CFG_SUPPORT_OS == OS_ALIOS)

/*return value :
		0: no valid image;

*/
#if (! OTA_TWO_IMAGE_SWITCH_RUN)
s8 os_do_crc_check(IMG_HEAD *phd)
{
	u32 i = 0, len = 0;
	u16 ota_crc = 0;
	u8 *f_data;
	CRC16_Context contex;
	s8 ret = -1;

	f_data = (u8 *)malloc (0x1000);
	if(0 == f_data)
	{
		return -1;
	}

	CRC16_Init( &contex );
	while(i < phd->siz)
	{
		if((0x1000 + i) <= phd->siz)
		{
			len = 0x1000;
		}
		else
		{
			len = phd->siz - i;
		}
		flash_read_data(f_data, (u32)(phd->src_adr + i), len);
		CRC16_Update( &contex, f_data, len);
		i += 0x1000;
	}
	CRC16_Final( &contex, &ota_crc );

	printf("ota_crc: %X.\r\n", ota_crc);
	if(ota_crc == phd->crc)
	{
		ret = 0;
	}

	if(f_data)
	{
		free(f_data);
	}
	return ret;
}

u32 os_do_backup(void)
{
	u32 i = 0;
	u8 *f_data;
	u32 ret = 1;
	IMG_HEAD hd;
	
	f_data = (u8 *)malloc (0x1000);
	if(0 == f_data)
	{
		goto rt_back;
	}
	
	flash_read_data((void *)&hd, (u32)OS1_HD_ADDR, sizeof(hd));
	printf ("hd:%X %X %X %X \r\n", hd.dst_adr, hd.src_adr, hd.siz, hd.crc);
	if(hd.crc != 0xFFFF)
	{
		if(hd.src_adr >= 0x200000 || hd.src_adr < hd.dst_adr + hd.siz)
		{
			printf("add error.\r\n");
			goto rt_back;
		}
		if(os_do_crc_check(&hd))
		{
			printf("crc error.\r\n");
			goto rt_back;
		}
		
		printf("do image updating....\r\n");
		flash_set_protect(NONE);
		while(i < hd.siz)
		{
			flash_read_data(f_data, (u32)(hd.src_adr + i), 0x1000);
			flash_write_data_with_erase(f_data, (u32)(hd.dst_adr + i), 0x1000);
			i += 0x1000;
		}

		flash_erase_sector(OS1_HD_ADDR);
		
		flash_set_protect(ALL);
		
		printf("do image update successfully\r\n");
	}
	else
	{
		if(hd.crc != 0)
		{
			flash_read_data(f_data, (u32)OS_CRC_BIN_START_ADDR, 32);
			for(i = 0; i < 32; i++)
			{
				if(f_data[i] != 0xff)
				{
					printf("f_data[%d]=0x%x\r\n", i, f_data[i]);
					break;
				}
			}
			
			if(i == 32)
				ret = 0;
		}
	}
	
rt_back:
	if(f_data)
	{
		free(f_data);
	}
	
	return ret;
}
#else
u32 os_do_switch(void)
{
	u32 addr;
	IMG_HEAD hd;
	
	flash_read_data((u8 *)&hd, (u32)OS1_HD_ADDR, sizeof(hd));
	
	if(hd.image_adr == 0xFFFFFFFF)
	{
		addr = OS_EX_ADDR;
	}
	else
	{
		addr = hd.image_adr / 34 * 32;
	}

	return addr;
}
#endif

void system_timeout_startup(void)
{
    if((1 == uart_download_status))
    {
        return;
    }

    if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
    {
        boot_downloading = FALSE;
        reset_register_dump();
#if OTA_TWO_IMAGE_SWITCH_RUN
		reset = os_do_switch();
#else
		if(os_do_backup() == 0)
		{
			return;
		}

		reset = OS_EX_ADDR;
#endif
        printf("\r\n\r\nsystem startup(0x%x)..........\r\n", reset);
#if (CFG_SOC_NAME != SOC_BK7256)
        sys_forbidden_interrupts();
#endif
        jump_pc_address(reset);
    }
}

#elif (CFG_SUPPORT_OS == OS_FREERTOS)

/*return value :
		0: no valid image;

*/
#if (! OTA_TWO_IMAGE_SWITCH_RUN)


u32 os_do_backup(void)
{
    u32 i = 0;
    u32 ret = 0;
    IMG_HEAD hd;
    BOOT_PARAM_HD b_param;

    u8 *f_data;
    f_data = (u8 *)malloc (0x1000);
    if(0 == f_data)
    {
        goto rt_back;
    }

    flash_read_data((void *)&hd, (u32)OS1_HD_ADDR, sizeof(hd));
    printf ("hd:%X %X %X %X %X %X %X \r\n", hd.bkup_addr,
            hd.bkup_len, hd.crc,
            hd.ex_addr, hd.os_addr,
            hd.hd_addr, hd.status);
    if(hd.status == 1 && hd.bkup_len && hd.bkup_addr)
    {
        if(hd.bkup_addr >= 0x200000 || hd.bkup_addr < hd.os_addr + hd.bkup_len )
        {
            ret = 0;
            goto rt_back;
        }

        flash_set_protect(NONE);
        while(i < hd.bkup_len)
        {
            flash_read_data(f_data, (u32)(hd.bkup_addr + i), 0x1000);
            flash_write_data_with_erase(f_data, (u32)(hd.os_addr + i), 0x1000);
            i += 0x1000;
        }

        hd.status = 0;
        memcpy(&b_param.i_hd, &hd, sizeof(hd));
        b_param.jump_addr = hd.ex_addr;
#if 1
        flash_read_data(f_data, (u32)OS1_HD_ADDR, 0x1000);
        memcpy(f_data, (void *)&b_param, sizeof(b_param));
        flash_write_data_with_erase(f_data, (u32)OS1_HD_ADDR, 0x1000);
#else
        flash_write_data_with_erase((void *)&b_param, (void *)OS1_HD_ADDR, sizeof(b_param));
#endif
        flash_set_protect(ALL);
        ret = hd.ex_addr;

    }
    else
    {
        ret = OS_EX_ADDR;
        if(hd.status != 0)
        {
            flash_read_data(f_data, (u32)OS_CRC_BIN_START_ADDR, 32);
            for(i = 0; i < 32; i++)
            {
                if(f_data[i] != 0xff)
                {
                    break;
                }
            }

            if(i == 32)
                ret = 0;
        }
    }

rt_back:

    if(f_data)
    {
        free(f_data);
    }

    return ret;
}

#endif

void system_timeout_startup(void)
{
#if OTA_TWO_IMAGE_SWITCH_RUN
    IMG_HEAD hd;
#endif

    if( (1 == uart_download_status) )
    {
        return;
    }


    if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
    {
        boot_downloading = FALSE;
        reset_register_dump();
#if OTA_TWO_IMAGE_SWITCH_RUN
        flash_read_data((void *)&hd, (void *)OS1_HD_ADDR, sizeof(hd));
        if((hd.status == 0x5a5aa5a5) && hd.bkup_len && hd.bkup_addr)
        {
            if(hd.bkup_addr >= 0x200000 || hd.bkup_addr < 0x22000 )
            {
                return;
            }
            else
            {
                reset = hd.ex_addr;
            }
        }
        else
            reset = OS_EX_ADDR;
#else
        if(os_do_backup() == 0)
        {
            return;
        }

        reset = OS_EX_ADDR;
#endif
#if (CFG_SOC_NAME != SOC_BK7256)
        sys_forbidden_interrupts();
#endif
        jump_pc_address(reset);
    }
}

#elif (CFG_SUPPORT_OS == OS_RTTOS)
extern int ota_main(void);
extern int jump_to_app(void);
//void system_startup(void)
 void __attribute__((section(".itcm_write_flash"))) system_startup(void)
{
	ota_main();
    
	bk_printf("system_startup \r\n");
    //printf("J 0x%x\r\n", OS_EX_ADDR);
#if (CFG_SOC_NAME != SOC_BK7256)
    sys_forbidden_interrupts();
#endif
    
    jump_to_app();
    while(1);
}

//void system_timeout_startup(void)
 void __attribute__((section(".itcm_write_flash"))) system_timeout_startup(void)
{
    if((1 == uart_download_status) )
    {
        return;
    }
	bk_printf("system_timeout_startup begin  \r\n");
    //if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
    {
    	bk_printf("system_timeout_startup middle  \r\n");
        boot_downloading = FALSE;
        system_startup();
    }
}
#endif



