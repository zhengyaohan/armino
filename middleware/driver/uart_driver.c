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

#include "include.h"
#include "bk_api_mem.h"
#include "uart_driver.h"
#include "uart_hal.h"
#include "bk_api_uart.h"
#include "bk_uart.h"
#include "gpio_driver.h"
#include "bk_api_gpio.h"
#include "bk_fifo.h"
#include "bk_api_rtos.h"
#include "uart_statis.h"
#include "bk_api_int.h"
#include "icu_driver.h"
#include "power_driver.h"
#include "clock_driver.h"
#include "bk_api_rtos.h"
#include "bk_arch.h"
#include "bk_api_printf.h"
#if (CONFIG_SYSTEM_CTRL)
#include "sys_driver.h"
#endif


static void uart_isr(void) __SECTION(".itcm");
static uint32_t uart_id_read_fifo_frame(uart_id_t id, const kfifo_ptr_t rx_ptr) __SECTION(".itcm");

typedef struct {
	uart_hal_t hal;
	uint8_t id_init_bits;
	uint8_t id_sw_fifo_enable_bits;
} uart_driver_t;

typedef struct {
	bool rx_blocked;
	beken_semaphore_t rx_int_sema;
} uart_sema_t;

typedef struct
{
	uart_isr_t callback;
	void *param;
} uart_callback_t;

#define CONFIG_UART_MIN_BAUD_RATE (UART_CLOCK / (0x1fff + 1))
#define CONFIG_UART_MAX_BAUD_RATE (UART_CLOCK / (4 + 1))
#ifndef CONFIG_PRINTF_BUF_SIZE
#define CONFIG_PRINTF_BUF_SIZE    (128)
#endif

static uart_driver_t s_uart[SOC_UART_ID_NUM_PER_UNIT] = {0};
static bool s_uart_driver_is_init = false;
static uart_callback_t s_uart_rx_isr[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static uart_callback_t s_uart_tx_isr[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static kfifo_ptr_t s_uart_rx_kfifo[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static uart_sema_t s_uart_sema[SOC_UART_ID_NUM_PER_UNIT] = {0};

#define UART_RETURN_ON_NOT_INIT() do {\
		if (!s_uart_driver_is_init) {\
			UART_LOGE("UART driver not init\r\n");\
			return BK_ERR_UART_NOT_INIT;\
		}\
	} while(0)

#define UART_RETURN_ON_INVALID_ID(id) do {\
		if ((id) >= SOC_UART_ID_NUM_PER_UNIT) {\
			UART_LOGE("UART id number(%d) is invalid\r\n", (id));\
			return BK_ERR_UART_INVALID_ID;\
		}\
	} while(0)

#define UART_RETURN_ON_ID_NOT_INIT(id) do {\
		if (!(s_uart[id].id_init_bits & BIT((id)))) {\
			return BK_ERR_UART_ID_NOT_INIT;\
		}\
	} while(0)

#define UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(baud_rate) do {\
		if ((baud_rate) < CONFIG_UART_MIN_BAUD_RATE ||\
			(baud_rate) > CONFIG_UART_MAX_BAUD_RATE) {\
			UART_LOGE("UART baud rate(%d) not support\r\n", (baud_rate));\
			return BK_ERR_UART_BAUD_RATE_NOT_SUPPORT;\
		}\
	} while(0)

#if (CONFIG_DEBUG_FIRMWARE)
#define DEAD_WHILE() do{\
		while(1);\
	} while(0)
#else
#define DEAD_WHILE() do{\
		os_printf("dead\r\n");\
	} while(0)
#endif


#if (CONFIG_SYSTEM_CTRL)
static void uart_clock_enable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART1, CLK_PWR_CTRL_PWR_UP);
			break;
		case UART_ID_2:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART2, CLK_PWR_CTRL_PWR_UP);
			break;
		case UART_ID_3:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART3, CLK_PWR_CTRL_PWR_UP);
			break;
		default:
			break;
	}
}

static void uart_clock_disable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART1, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case UART_ID_2:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART2, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case UART_ID_3:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_UART3, CLK_PWR_CTRL_PWR_DOWN);
			break;
		default:
			break;
	}
}

static void uart_interrupt_enable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_1:
			sys_drv_int_enable(UART0_INTERRUPT_CTRL_BIT);
			break;
		case UART_ID_2:
			sys_drv_int_enable(UART1_INTERRUPT_CTRL_BIT);
			break;
		case UART_ID_3:
			sys_drv_int_enable(UART2_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
	}
}

static void uart_interrupt_disable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_1:
			sys_drv_int_disable(UART0_INTERRUPT_CTRL_BIT);
			break;
		case UART_ID_2:
			sys_drv_int_disable(UART1_INTERRUPT_CTRL_BIT);
			break;
		case UART_ID_3:
			sys_drv_int_disable(UART2_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
	}
}
#endif

static void uart_init_gpio(uart_id_t id)
{
	switch (id)
	{
		case UART_ID_1:
		{
			gpio_dev_map(uart_hal_get_tx_pin(id), GPIO_DEV_UART1_TXD);
			gpio_dev_map(uart_hal_get_rx_pin(id), GPIO_DEV_UART1_RXD);
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
#if CONFIG_UART1_FLOW_CTRL
			//NOTICE:BEKEN ASIC CTS PIN really function is RTS.
			gpio_dev_map(uart_hal_get_cts_pin(id), GPIO_DEV_UART1_CTS);
			bk_gpio_enable_output(uart_hal_get_cts_pin(id));
			bk_gpio_pull_down(uart_hal_get_cts_pin(id));

			gpio_dev_map(uart_hal_get_rts_pin(id), GPIO_DEV_UART1_RTS);
			bk_gpio_enable_input(uart_hal_get_rts_pin(id));
			bk_gpio_pull_down(uart_hal_get_rts_pin(id));
			bk_uart_set_hw_flow_ctrl(id, UART1_FLOW_CTRL_CNT);
#endif
			break;
		}
		case UART_ID_2:
		{
			gpio_dev_map(uart_hal_get_tx_pin(id), GPIO_DEV_UART2_TXD);
			gpio_dev_map(uart_hal_get_rx_pin(id), GPIO_DEV_UART2_RXD);
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
			break;
		}
		case UART_ID_3:
		{
			gpio_dev_map(uart_hal_get_tx_pin(id), GPIO_DEV_UART3_TXD);
			gpio_dev_map(uart_hal_get_rx_pin(id), GPIO_DEV_UART3_RXD);
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
			break;
		}
		default:
			break;
	}
}

static void uart_deinit_tx_gpio(uart_id_t id)
{
	switch (id)
	{
		case UART_ID_1:
		{
			gpio_dev_unmap(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			break;
		}
		case UART_ID_2:
		{
			gpio_dev_unmap(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			break;
		}
		case UART_ID_3:
		{
			gpio_dev_unmap(uart_hal_get_tx_pin(id));
			bk_gpio_pull_up(uart_hal_get_tx_pin(id));
			break;
		}
		default:
			break;
	}
}

static void uart_deinit_rx_gpio(uart_id_t id)
{
	switch (id)
	{
		case UART_ID_1:
		{
			gpio_dev_unmap(uart_hal_get_rx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
			break;
		}
		case UART_ID_2:
		{
			gpio_dev_unmap(uart_hal_get_rx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
			break;
		}
		case UART_ID_3:
		{
			gpio_dev_unmap(uart_hal_get_rx_pin(id));
			bk_gpio_pull_up(uart_hal_get_rx_pin(id));
			break;
		}
		default:
			break;
	}
}

static bk_err_t uart_id_init_kfifo(uart_id_t id)
{
	if (!s_uart_rx_kfifo[id]) {
		s_uart_rx_kfifo[id] = kfifo_alloc(CONFIG_KFIFO_SIZE);
		if (!s_uart_rx_kfifo[id]) {
			UART_LOGE("uart(%d) rx kfifo alloc failed\n", id);
			return BK_ERR_NULL_PARAM;
		}
	}
	return BK_OK;
}

static void uart_id_deinit_kfifo(uart_id_t id)
{
	if (s_uart_rx_kfifo[id]) {
		kfifo_free(s_uart_rx_kfifo[id]);
	}
	s_uart_rx_kfifo[id] = NULL;
}

/* 1. power up uart
 * 2. set clock
 * 3. set gpio as uart
 */
static bk_err_t uart_id_init_common(uart_id_t id)
{
	bk_err_t ret = 0;

#if (CONFIG_SYSTEM_CTRL)
	uart_clock_enable(id);
	sys_drv_uart_select_clock(id, UART_SCLK_XTAL_26M);
#else
	power_uart_pwr_up(id);
	clk_set_uart_clk_26m(id);
#endif

	uart_init_gpio(id);
	ret = uart_id_init_kfifo(id);
	uart_statis_id_init(id);

	s_uart_sema[id].rx_blocked = false;
	if (s_uart_sema[id].rx_int_sema == NULL) {
		ret = rtos_init_semaphore(&(s_uart_sema[id].rx_int_sema), 1);
		BK_ASSERT(kNoErr == ret);
	}
	s_uart[id].id_init_bits |= BIT(id);
	s_uart[id].id_sw_fifo_enable_bits |= BIT(id);

	return ret;
}

static void uart_id_deinit_common(uart_id_t id)
{
	s_uart[id].id_init_bits &= ~BIT(id);
	uart_hal_stop_common(&s_uart[id].hal, id);
	uart_hal_reset_config_to_default(&s_uart[id].hal, id);
#if (CONFIG_SYSTEM_CTRL)
	uart_interrupt_disable(id);
	uart_clock_disable(id);
#else
	icu_disable_uart_interrupt(id);
	power_uart_pwr_down(id);
#endif
	uart_id_deinit_kfifo(id);
	rtos_deinit_semaphore(&(s_uart_sema[id].rx_int_sema));
}

static inline bool uart_id_is_sw_fifo_enabled(uart_id_t id)
{
	return !!(s_uart[id].id_sw_fifo_enable_bits & BIT(id));
}

static uint32_t uart_id_read_fifo_frame(uart_id_t id, const kfifo_ptr_t rx_ptr)
{
	uint8_t read_val = 0;
	uint32_t rx_count = sizeof(read_val);
	uint32_t unused = kfifo_unused(rx_ptr);
	uint32_t kfifo_put_cnt = 0;
	__attribute__((__unused__)) uart_statis_t *uart_statis = uart_statis_get_statis(id);

	//TODO: optimize flow ctrl
	while (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		/* must read when fifo read ready, otherwise will loop forever */
		read_val = uart_hal_read_byte(&s_uart[id].hal, id);
		UART_LOGD("read val:0x%x, rx_count/unused: %d/%d\n", read_val, rx_count, unused);
		if (rx_count > unused) {
			if (!uart_hal_is_flow_control_enabled(&s_uart[id].hal, id)) {
				UART_LOGW("rx kfifo is full, out/in:%d/%d, unused:%d\n", rx_ptr->out, rx_ptr->in, unused);
				UART_STATIS_INC(uart_statis->kfifo_status.full_cnt);
#if CFG_CLI_DEBUG
				extern void cli_show_running_command(void);
				cli_show_running_command();
#endif
			}
		} else {
			kfifo_put_cnt += kfifo_put(rx_ptr, &read_val, sizeof(read_val));
			UART_STATIS_INC(uart_statis->kfifo_status.put_cnt);
			UART_STATIS_SET(uart_statis->kfifo_status.last_value, read_val);
		}
		unused = kfifo_unused(rx_ptr);
	}
	UART_STATIS_SET(uart_statis->kfifo_status.in, rx_ptr->in);
	UART_STATIS_SET(uart_statis->kfifo_status.out, rx_ptr->out);

	return kfifo_put_cnt;
}

void print_hex_dump(const char *prefix, void *buf, int len)
{
	int i;
	u8 *b = buf;

	if (prefix)
		UART_LOGI("%s", prefix);
	for (i = 0; i < len; i++)
		UART_LOGI("%02X ", b[i]);
	UART_LOGI("\n");
}

bk_err_t uart_write_byte(uart_id_t id, uint8_t data)
{
	/* wait for fifo write ready
	 * optimize it when write very fast
	 * wait for fifo write ready will waste CPU performance
	 */
	BK_WHILE (!uart_hal_is_fifo_write_ready(&s_uart[id].hal, id));
	uart_hal_write_byte(&s_uart[id].hal, id, data);
	return BK_OK;
}


bk_err_t uart_write_ready(uart_id_t id)
{
	/* wait for fifo write ready
	 * optimize it when write very fast
	 * wait for fifo write ready will waste CPU performance
	 */
	if (!uart_hal_is_fifo_write_ready(&s_uart[id].hal, id)) {
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t uart_write_string(uart_id_t id, const char *string)
{
	const char *p = string;

	while (*string) {
		if (*string == '\n') {
			if (p == string || *(string - 1) != '\r')
				uart_write_byte(id, '\r'); /* append '\r' */
		}
		uart_write_byte(id, *string++);
	}

	return BK_OK;
}

bk_err_t uart_read_ready(uart_id_t id)
{
	if (!uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		return BK_FAIL;
	}

	return BK_OK;
}

int uart_read_byte(uart_id_t id)
{
	int val = -1;
	if (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		val = uart_hal_read_byte(&s_uart[id].hal, id);
	}
	return val;
}

int uart_read_byte_ex(uart_id_t id, uint8_t *ch)
{
	int val = -1;
	if (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		*ch = uart_hal_read_byte(&s_uart[id].hal, id);
		val = 0;
	}
	return val;
}

uint32_t uart_get_length_in_buffer(uart_id_t id)
{
	return kfifo_data_size(s_uart_rx_kfifo[id]);
}

static void uart_isr_register_functions(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_1:
			bk_int_isr_register(INT_SRC_UART1, uart_isr, NULL);
			break;
		case UART_ID_2:
			bk_int_isr_register(INT_SRC_UART2, uart_isr, NULL);
			break;
		case UART_ID_3:
			bk_int_isr_register(INT_SRC_UART3, uart_isr, NULL);
			break;
		default:
			break;
	}
}

bk_err_t bk_uart_driver_init(void)
{
	if (s_uart_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_uart, 0, sizeof(s_uart));
	os_memset(&s_uart_rx_isr, 0, sizeof(s_uart_rx_isr));
	os_memset(&s_uart_tx_isr, 0, sizeof(s_uart_tx_isr));

	for(uart_id_t id = UART_ID_1; id < SOC_UART_ID_NUM_PER_UNIT; id++)
	{
		uart_isr_register_functions(id);
		s_uart[id].hal.id = id;
		uart_hal_init(&s_uart[id].hal);
	}

	uart_statis_init();
	s_uart_driver_is_init = true;

	//TODO place it to better place
	bk_printf_init();

	return BK_OK;
}

bk_err_t bk_uart_driver_deinit(void)
{
	if (!s_uart_driver_is_init)
		return BK_OK;

	for (uart_id_t id = UART_ID_1; id < SOC_UART_ID_NUM_PER_UNIT; id++) {
		uart_id_deinit_common(id);
	}
	s_uart_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_uart_init(uart_id_t id, const uart_config_t *config)
{
	UART_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(config);
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(config->baud_rate);

	uart_id_init_common(id);
#if (CONFIG_SYSTEM_CTRL)
	if (config->src_clk == UART_SCLK_APLL)
		sys_drv_uart_select_clock(id, UART_SCLK_APLL);
	else
		sys_drv_uart_select_clock(id, UART_SCLK_XTAL_26M);

#else
	if (config->src_clk == UART_SCLK_DCO) {
		clk_set_uart_clk_dco(id);
	} else {
		clk_set_uart_clk_26m(id);
	}
#endif

	uart_hal_init_uart(&s_uart[id].hal, id, config);
	uart_hal_start_common(&s_uart[id].hal, id);

	return BK_OK;
}

bk_err_t bk_uart_deinit(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_id_deinit_common(id);
	return BK_OK;
}

bk_err_t bk_uart_set_baud_rate(uart_id_t id, uint32_t baud_rate)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(baud_rate);
	uint32_t uart_clk = clk_get_uart_clk(id);
	uart_hal_set_baud_rate(&s_uart[id].hal, id, uart_clk, baud_rate);
	return BK_OK;
}

bk_err_t bk_uart_set_data_bits(uart_id_t id, uart_data_bits_t data_bits)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_data_bits(&s_uart[id].hal, id, data_bits);
	return BK_OK;
}

bk_err_t bk_uart_set_stop_bits(uart_id_t id, uart_stop_bits_t stop_bits)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_stop_bits(&s_uart[id].hal, id, stop_bits);
	return BK_OK;
}

bk_err_t bk_uart_set_parity(uart_id_t id, uart_parity_t partiy)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_parity(&s_uart[id].hal, id, partiy);
	return BK_OK;
}

bk_err_t bk_uart_enable_tx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
#if (CONFIG_SYSTEM_CTRL)
	uart_interrupt_enable(id);
#else
	icu_enable_uart_interrupt(id);
#endif
	uart_hal_enable_tx_interrupt(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_tx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_disable_tx_interrupt(&s_uart[id].hal, id);
	uart_hal_clear_id_tx_interrupt_status(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_enable_rx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
#if (CONFIG_SYSTEM_CTRL)
	uart_interrupt_enable(id);
#else
	icu_enable_uart_interrupt(id);
#endif
	uart_hal_enable_rx_interrupt(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_rx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_disable_rx_interrupt(&s_uart[id].hal, id);
	uart_hal_clear_id_rx_interrupt_status(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_register_rx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_uart_rx_isr[id].callback = isr;
	s_uart_rx_isr[id].param = param;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_uart_register_tx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_uart_tx_isr[id].callback = isr;
	s_uart_tx_isr[id].param = param;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

static uart_callback_t s_last_uart_rx_isr = {0};

void bk_uart_take_rx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	s_last_uart_rx_isr.callback = s_uart_rx_isr[id].callback;
	s_last_uart_rx_isr.param = s_uart_rx_isr[id].param;

	bk_uart_disable_sw_fifo(id);
	bk_uart_register_rx_isr(id, isr, param);
}

void bk_uart_recover_rx_isr(uart_id_t id)
{
	bk_uart_register_rx_isr(id, s_last_uart_rx_isr.callback, s_last_uart_rx_isr.param);

#if (!CONFIG_SHELL_ASYNCLOG)
	bk_uart_enable_sw_fifo(id);
#else
	if (id != CONFIG_UART_PRINT_PORT)
	{
		bk_uart_enable_sw_fifo(id);
	}
#endif

	s_last_uart_rx_isr.callback = NULL;
	s_last_uart_rx_isr.param = NULL;
}

bk_err_t bk_uart_write_bytes(uart_id_t id, const void *data, uint32_t size)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_ID_NOT_INIT(id);
	for (int i = 0; i < size; i++) {
		uart_write_byte(id, ((uint8 *)data)[i]);
	}
	return BK_OK;
}

bk_err_t bk_uart_read_bytes(uart_id_t id, void *data, uint32_t size, uint32_t timeout_ms)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_ID_NOT_INIT(id);

	__attribute__((__unused__)) uart_statis_t* uart_statis = uart_statis_get_statis(id);
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	uint32_t kfifo_data_len = kfifo_data_size(s_uart_rx_kfifo[id]);
	/* Only kfifo_data_len=0, wait for semaphore */
	if (kfifo_data_len == 0) {
		UART_LOGD("kfifo is empty, wait for recv data\r\n");
		/* when sema_cnt=0, rx_blocked=true, otherwise rx_blocked=false */
		s_uart_sema[id].rx_blocked = true;
		GLOBAL_INT_RESTORE();

		uint32_t ret = rtos_get_semaphore(&(s_uart_sema[id].rx_int_sema), timeout_ms);
		if (ret == kTimeoutErr) {
			GLOBAL_INT_DISABLE();
			if (!s_uart_sema[id].rx_blocked) {
				rtos_get_semaphore(&(s_uart_sema[id].rx_int_sema), 0);
			}
			s_uart_sema[id].rx_blocked = false;
			GLOBAL_INT_RESTORE();
			UART_LOGW("recv data timeout:%d\n", timeout_ms);
			UART_STATIS_INC(uart_statis->recv_timeout_cnt);
			return BK_ERR_UART_RX_TIMEOUT;
		}
	} else {
		GLOBAL_INT_RESTORE();
	}

	kfifo_data_len = kfifo_data_size(s_uart_rx_kfifo[id]); /* updata kfifo data size */
	if (size > kfifo_data_len) {
		if (kfifo_data_len) {
			kfifo_get(s_uart_rx_kfifo[id], (uint8_t *)data, kfifo_data_len);
		} else {
			UART_LOGW("kfifo data is empty\n");
			UART_STATIS_INC(uart_statis->kfifo_status.empty_cnt);
		}
		UART_STATIS_SET(uart_statis->kfifo_status.in, s_uart_rx_kfifo[id]->in);
		UART_STATIS_SET(uart_statis->kfifo_status.out, s_uart_rx_kfifo[id]->out);

		return kfifo_data_len;
	}
	kfifo_get(s_uart_rx_kfifo[id], (uint8_t *)data, size);
	UART_STATIS_SET(uart_statis->kfifo_status.in, s_uart_rx_kfifo[id]->in);
	UART_STATIS_SET(uart_statis->kfifo_status.out, s_uart_rx_kfifo[id]->out);

	return size;
}

bk_err_t bk_uart_set_rx_full_threshold(uart_id_t id, uint8_t threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_rx_fifo_threshold(&s_uart[id].hal, id, threshold);
	return BK_OK;
}

bk_err_t bk_uart_set_tx_empty_threshold(uart_id_t id, uint8_t threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_tx_fifo_threshold(&s_uart[id].hal, id, threshold);
	return BK_OK;
}

bk_err_t bk_uart_set_rx_timeout(uart_id_t id, uart_rx_stop_detect_time_t timeout_thresh)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_rx_stop_detect_time(&s_uart[id].hal, id, timeout_thresh);
	return BK_OK;
}

bk_err_t bk_uart_disable_rx(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_disable_rx(&s_uart[id].hal, id);
	uart_deinit_rx_gpio(id);

	return BK_OK;
}

bk_err_t bk_uart_disable_tx(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_disable_tx(&s_uart[id].hal, id);
	uart_deinit_tx_gpio(id);

	return BK_OK;
}

bk_err_t bk_uart_set_hw_flow_ctrl(uart_id_t id, uint8_t rx_threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_set_hw_flow_ctrl(&s_uart[id].hal, id, rx_threshold);
	uart_hal_enable_flow_control(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_hw_flow_ctrl(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	uart_hal_disable_hw_flow_ctrl(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_enable_sw_fifo(uart_id_t id)
{
	UART_RETURN_ON_INVALID_ID(id);
	s_uart[id].id_sw_fifo_enable_bits |= BIT(id);
	return BK_OK;
}

bk_err_t bk_uart_disable_sw_fifo(uart_id_t id)
{
	UART_RETURN_ON_INVALID_ID(id);
	s_uart[id].id_sw_fifo_enable_bits &= ~BIT(id);
	return BK_OK;
}

uint32_t bk_uart_get_ate_detect_gpio(void)
{
	return uart_hal_get_tx_pin(CONFIG_UART_ATE_PORT);
}

/* read int enable status
 * read int status
 * clear int status
 */
static void uart_isr(void)
{
	uint32_t int_status = 0;
	uint32_t int_enable_status = 0;
	uint32_t irq_uart_int_status = 0;
	uint32_t status = 0;
	UART_STATIS_DEC();

//Temp modify for BK7256 UART int status
//CPU0 uses UART1,UART2, CPU1 uses UART3.
#if ((defined CONFIG_SOC_BK7256XX) || (defined CONFIG_SOC_BK7256_CP1))
	irq_uart_int_status = sys_drv_get_int_source_status();
#else
	irq_uart_int_status = icu_get_uart_int_status();
#endif
	for (int id = UART_ID_1; id < UART_ID_MAX; id++) {
#if (defined CONFIG_SOC_BK7256XX)
		if ((id == UART_ID_1) && (!(irq_uart_int_status & UART0_INTERRUPT_CTRL_BIT)))
			continue;
		if ((id == UART_ID_2) && (!(irq_uart_int_status & UART1_INTERRUPT_CTRL_BIT)))
			continue;
		if(id == UART_ID_3)
			continue;
#elif (defined CONFIG_SOC_BK7256_CP1)
		if((id == UART_ID_1) || (id == UART_ID_2))
			continue;
		if ((id == UART_ID_3) && (!(irq_uart_int_status & UART2_INTERRUPT_CTRL_BIT)))
			continue;
#else
		if (!(irq_uart_int_status & BIT(id))) {
			continue;
		}
#endif
		int_status = uart_hal_get_interrupt_status(&s_uart[id].hal, id);
		int_enable_status = uart_hal_get_int_enable_status(&s_uart[id].hal, id);
		status = int_status & int_enable_status;
		uart_hal_clear_interrupt_status(&s_uart[id].hal, id, int_status);
		UART_STATIS_GET(uart_statis, id);
		UART_STATIS_INC(uart_statis->uart_isr_cnt);

		if (uart_hal_is_rx_interrupt_triggered(&s_uart[id].hal, id, status)) {
			UART_STATIS_INC(uart_statis->rx_isr_cnt);
			UART_STATIS_SET(uart_statis->rx_fifo_cnt, uart_hal_get_rx_fifo_cnt(&s_uart[id].hal, id));
			if (uart_id_is_sw_fifo_enabled(id)) {
				if (uart_id_read_fifo_frame(id, s_uart_rx_kfifo[id]) > 0) {
					if (s_uart_sema[id].rx_int_sema && s_uart_sema[id].rx_blocked) {
						rtos_set_semaphore(&(s_uart_sema[id].rx_int_sema));
						s_uart_sema[id].rx_blocked = false;
					}
				}
			}

			if (s_uart_rx_isr[id].callback) {
				s_uart_rx_isr[id].callback(id, s_uart_rx_isr[id].param);
			}
		}
		if (uart_hal_is_tx_interrupt_triggered(&s_uart[id].hal, id, status)) {
			if (s_uart_tx_isr[id].callback) {
				s_uart_tx_isr[id].callback(id, s_uart_tx_isr[id].param);
			}
		}
		if(uart_hal_is_rx_parity_err_int_triggered(&s_uart[id].hal, id, status)) {
			uart_hal_flush_fifo(&s_uart[id].hal, id);
		}
	}
}

