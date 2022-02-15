/*************************************************************
 * @file        driver_sys_ctrl.c
 * @brief       code of sys control driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#include <stdio.h>

#include "BK_System.h"
#include "driver_sys_ctrl.h"


unsigned long get_chip_id(void)
{
    return REG_SYS_CTRL_CHIP_ID;
}

unsigned long get_device_id(void)
{
    return REG_SYS_CTRL_DEVICE_ID;
}
