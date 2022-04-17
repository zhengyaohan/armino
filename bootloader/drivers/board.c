/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2018 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     armink      first implementation
 * 2018-02-06     Murphy      add flash control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "drv_uart.h"
#include "driver_icu.h"
#include "driver_flash.h"
#include "driver_sys_ctrl.h"
#include "BK_System.h"
#include "drv_timer.h"

#define GPIO_RX_CFG    (*((volatile unsigned long *)    (0x44000400+10*4)))
#define GPIO_TX_CFG    (*((volatile unsigned long *)    (0x44000400+11*4)))
static void GPIO_Configuration(void)
{
    // TODO
}

#ifdef RELEASE
static void IWDG_Configuration(void)
{
    // TODO
}

void IWDG_Feed(void)
{
    // TODO
    // IWDG_ReloadCounter();
}
#endif /* RELEASE */

/**
 * This function will initial beken7231 board.
 */
 //void rt_hw_board_init(void)
 void __attribute__((section(".itcm_write_flash"))) rt_hw_board_init(void)
{
#if (SOC_BK7271 == CFG_SOC_NAME)
    /* told by huaming to avoid reset */
    addGPIO_Reg0xf = 0x38;
#endif

#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
	icu_init();
#else
    ICU_init();
#endif
#if ((SOC_BK7271 != CFG_SOC_NAME) && (SOC_BK7256 != CFG_SOC_NAME))
    sys_ctrl_save_deep_gpio_wake_status();
#endif
    GPIO_Configuration();
    uart_init(); 
    flash_init(); //flash need init when upgrade OTA 
    flash_set_line_mode(2);
    flash_set_clk(5);  // 78M
    get_flash_ID();
    flash_get_current_flash_config();

#ifdef RELEASE
    IWDG_Configuration();
#endif /* RELEASE */

}

