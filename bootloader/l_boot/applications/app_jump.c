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

#define APP_ENTRY_ADDR      USER_APP_ENTRY
#define BOOT_END_ADDR       BK_BOOT_END

void on_jump(uint32_t app_stack_addr, void (*app_entry)(void))
{
    /* jump to application */
    app_entry();
}

int jump_to_app(void)
{
    void (*jump2app)(void);

    jump2app = (void ( *)(void))APP_ENTRY_ADDR;


    if (APP_ENTRY_ADDR >=  BOOT_END_ADDR && APP_ENTRY_ADDR < USER_APP_END)
    {
        printf("Find user application success.\r\n");

        printf("The Bootloader will go to %x.\r\n", jump2app);

        /* do something then jump to application */
        on_jump(0, jump2app);
        return 0;
    }
    else
    {
        printf("Not find user application. Will stop at bootloader.\r\n");
        return -1;
    }
}
