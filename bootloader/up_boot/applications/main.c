/*
 * File      : main.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <board.h>

#include "BK_System.h"
#include "rt_heap.h"

int boot_main(void)
{
    __disable_irq();
    __disable_fiq();
	
    intc_forbidden_all();
    rt_hw_board_init();	
    heap_init();
	
    system_timeout_startup();

    return 0;
}
// eof

