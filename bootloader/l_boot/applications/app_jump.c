/*
 * File      : app_jump.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <board.h>
#include <stdio.h>
#include <stdint.h>
#include "BK_System.h"
#include "board.h"
#include "drv_uart.h"
#include "platform.h"

#define APP_ENTRY_ADDR      0x1F00
#define BOOT_END_ADDR       BK_BOOT_END

void on_jump(uint32_t app_stack_addr, void (*app_entry)(void))
{
    /* jump to bl_up */
    app_entry();
}


int jump_to_app(void)
{
	unsigned char rev_data;
    void (*jump2app)(void);

    jump2app = (void ( *)(void))APP_ENTRY_ADDR;
    //if (APP_ENTRY_ADDR >=  BOOT_END_ADDR && APP_ENTRY_ADDR < USER_APP_END)
	if (1)
	{
        bk_printf("jump_to_app to ");
		bk_print_hex(APP_ENTRY_ADDR);

        /* do something then jump to application */
		uart1_wait_tx_finish();
	
		/* clear mcause */
		clear_csr(NDS_MSTATUS, MSTATUS_MIE);
		clear_csr(NDS_MCAUSE, read_csr(NDS_MCAUSE));
        on_jump(0, jump2app);
        while(1)
		{
			while(!((*((volatile unsigned long *) REG_UART0_FIFO_STATUS_ADDR))&(0x1<<21)));
			rev_data = (unsigned char) ((*((volatile unsigned long *) (0x44820000UL+0x3*4))&0xff00)>>8);
			bk_print_hex(rev_data);
			
		}
        return 0;
    }
    else
    {
        printf("Not find user application. Will stop at bootloader.\r\n");
        return -1;
    }
}


