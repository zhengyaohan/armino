/*
 * File      : drv_uart.h
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     Murphy      the first version
 */

#ifndef _DRV_UART_H_
#define _DRV_UART_H_

int uart_init(void);
int uart_putc(char c);
int uart_getc(void);

void uart_console_output(const char *str, int length);

#endif /* _DRV_UART_H_ */
