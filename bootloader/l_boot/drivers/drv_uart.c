/*
 * File      : drv_uart.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     Murphy       the first version
 */

#include <board.h>
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_uart2.h"
#include "stdio.h"

int uart_init(void)
{
    uart0_init(115200);
    uart1_init(115200);
#if (SOC_BK7271 == CFG_SOC_NAME)
	uart2_init(115200);
#endif
    return 0;
}

int uart_putc(char c)
{
    char buf[1];
    buf[0] = c;

#if ((PRINT_PORT == DEBUG_PORT_UART0) )
    uart0_send((u8 *)&buf[0], 1);

#endif
#if ((PRINT_PORT == DEBUG_PORT_UART1) )
    uart1_send((u8 *)&buf[0], 1);
#endif
#if ((PRINT_PORT == DEBUG_PORT_UART2) && (SOC_BK7271 == CFG_SOC_NAME))
	uart2_send((u8 *)&buf[0], 1);
#endif
    return 1;
}

int uart_getc(void)
{
    int ch;
    ch = -1;

    return ch;
}

void uart_console_output(const char *str, int length)
{
    int index;

    for (index = 0; index < length; index ++)
    {
        uart_putc(*str);
        str++;
    }
}
