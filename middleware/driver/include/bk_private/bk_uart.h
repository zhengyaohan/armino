// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "include.h"
#include "uart_types.h"
#include "bk_api_printf.h"

#ifndef CONFIG_UART_PRINT_PORT
#define CONFIG_UART_PRINT_PORT         UART_ID_2
#endif

#define warning_prf                    bk_printf
#define fatal_prf                      bk_printf
#define null_prf                       bk_null_printf

#define UART_SUCCESS                 (0)
#define UART_FAILURE                 ((UINT32)-1)

#if (CONFIG_SOC_BK7271)
#define UART3_DEV_NAME               ("uart3")   /*debug purpose*/
#endif
#define UART2_DEV_NAME               ("uart2")   /*debug purpose*/
#define UART1_DEV_NAME               ("uart1")   /*comm purpose*/

#define UART_CMD_MAGIC               (0xC124000)

#define UART1_PORT				0
#define UART2_PORT				1
#if (CONFIG_SOC_BK7271)
#define UART3_PORT				2
#endif

enum
{
	CMD_SEND_BACKGROUND            = UART_CMD_MAGIC + 0,
	CMD_UART_RESET                 = UART_CMD_MAGIC + 1,
	CMD_RX_COUNT,
	CMD_RX_PEEK,
	CMD_UART_INIT,
	CMD_UART_SET_RX_CALLBACK,
	CMD_UART_SET_TX_CALLBACK,
	CMD_SET_STOP_END,
	CMD_UART_SET_TX_FIFO_NEEDWR_CALLBACK,
	CMD_SET_TX_FIFO_NEEDWR_INT,
	CMD_SET_BAUT,
};

/* CMD_RX_PEEK*/
#define URX_PEEK_SIG              (0x0ee)

typedef struct _peek_rx_
{
	UINT32 sig;

	UINT32 len;
	void *ptr;
} UART_PEEK_RX_T, *UART_PEEK_RX_PTR;

typedef void (*uart_callback)(int uport, void *param);

typedef struct uart_callback_des
{
	uart_callback callback;
	void *param;
}UART_CALLBACK_RX_T, *UART_CALLBACK_RX_PTR;

/*******************************************************************************
* Function Declarations
*******************************************************************************/
void uart1_init(void);
void uart1_exit(void);
void uart1_isr(void);

void uart2_init(void);
void uart2_exit(void);
void uart2_isr(void);

#if (CONFIG_SOC_BK7271)
void uart3_init(void);
void uart3_exit(void);
void uart3_isr(void);
#endif

void print_hex_dump(const char *prefix, void *b, int len);

uint32_t uart_get_length_in_buffer(uart_id_t id);
bk_err_t uart_write_ready(uart_id_t id);
bk_err_t uart_write_byte(uart_id_t id, uint8_t data);
bk_err_t uart_write_string(uart_id_t id, const char *string);
uint32_t uart_wait_tx_over();
bk_err_t uart_read_ready(uart_id_t id);
int uart_read_byte(uart_id_t id);
int uart_read_byte_ex(uart_id_t id, uint8_t *ch);

#ifdef __cplusplus
}
#endif
