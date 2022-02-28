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

#if (CFG_OTA_TYPE == OTA_ALI)
#include "xz_hal.h"

#define SYSTEM_TIMEOUT_STARTUP   (400)
#elif (CFG_OTA_TYPE == OTA_BK)

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

u32 reset = 0;


#if (CFG_OTA_TYPE == OTA_ALI)

int ota_flash_erase(u32 addr)
{
	flash_erase_sector(addr);
	return 0;
}

int ota_flash_read(u32 addr, uint8_t *buf, u32 len)
{
	flash_read_data(buf, addr, len);
	return 0;
}

int ota_flash_write(u32 addr, uint8_t *buf, u32 len)
{
	flash_write_data(buf, addr, len);
	return 0;
}

void system_startup(void)
{
    ota_param_t hd;

    flash_read_data((uint8_t *)&hd, (u32)OTA_PART_PARAMETER_1, sizeof(hd));
    //reset_register_dump();

	if(hd.upg_flag == OTA_PINGPONG_FLAG)
	{
		reset = hd.dst_adr / 34 * 32;
	}
	else if(hd.upg_flag == OTA_UPG_FLAG)
	{
	    if(ota_update())
	    {
			bk_printf("\r\n\r\nota fail............\r\n");
	    }
		reset = OS_EX_ADDR;
	}
	else
	{
		reset = OS_EX_ADDR;
	}
	
	bk_printf("\r\n\r\nsystem startup(0x%x)..........\r\n", reset);

	sys_forbidden_interrupts();
	#if (PRINT_PORT == DEBUG_PORT_UART0)
	uart0_wait_tx_finish();
	#else
	uart1_wait_tx_finish();
	#endif
	jump_pc_address(reset);
	return;
}

void system_timeout_startup(void)
{
    system_startup();
}

#elif (CFG_OTA_TYPE == OTA_BK)

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

    uint8_t *f_data;
    f_data = (uint8_t *)malloc (0x1000);
    if(0 == f_data)
    {
        goto rt_back;
    }

    flash_read_data((void *)&hd, (u32)OS1_HD_ADDR, sizeof(hd));
    bk_printf ("hd:%X %X %X %X %X %X %X \r\n", hd.bkup_addr,
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

        set_flash_protect(NONE);
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
        set_flash_protect(ALL);
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

    {
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
        sys_forbidden_interrupts();
        jump_pc_address(reset);
    }
}

#elif (CFG_OTA_TYPE == OTA_RTT)

void system_startup(void)
{
    UINT32 ex_addr;
    
    #if ((CFG_SOC_NAME == SOC_BK7271) && (SB_FUNCTION_TEST_ENABLE == 1))
    extern void app_sb_function_test(void);
    app_sb_function_test();
    #endif
    
    if (ota_main(&ex_addr) == 0)
    {
    
    }
    else
    {
        ex_addr = OS_EX_ADDR;
    }

    bk_printf("\r\n\r\ngo os_addr(0x%x)..........\r\n", ex_addr);
	#if (DEBUG_PORT_UART0 == PRINT_PORT)
	uart0_wait_tx_finish();
	#elif (DEBUG_PORT_UART2 == PRINT_PORT) && (SOC_BK7271 == CFG_SOC_NAME)
	uart2_wait_tx_finish();
	#else
	uart1_wait_tx_finish();
    #endif
    sys_forbidden_interrupts();
    jump_pc_address(ex_addr);

    while(1);
}

void system_timeout_startup(void)
{
    system_startup();
}
#endif
//eof

