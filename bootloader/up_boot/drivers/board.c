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
#include "BK_System.h"

void assert_failed(uint8_t *file, u32 line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: bk_printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* Infinite loop */
	bk_printf("assert failed at %s:%ld \n", file, line);
	while (1) {
	}
}

/**
 * This function will initial beken7231 board.
 */
void rt_hw_board_init(void)
{
	uart_init();

	flash_set_line_mode(2);
	flash_set_clk(5);  // 78M
	get_flash_ID();
	flash_get_current_flash_config();
}
// eof
 
