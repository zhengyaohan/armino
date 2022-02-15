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

#include <stdarg.h>
#include "bk_api_uart.h"
#include "bk_uart.h"
#include "sys_config.h"
#include "bk_api_mem.h"
#include "printf_impl.h"
#if CONFIG_SHELL_ASYNCLOG
#include "bk_api_cli.h"
#endif

#if (CONFIG_SHELL_ASYNCLOG && CONFIG_SLAVE_CORE)
#define CPU1_TAG "cpu1"
#endif

#ifndef CONFIG_PRINTF_BUF_SIZE
#define CONFIG_PRINTF_BUF_SIZE    (128)
#endif

#if (!CONFIG_SHELL_ASYNCLOG)
static char s_exception_mode_printf_buf[CONFIG_PRINTF_BUF_SIZE] = {0};
static char s_task_mode_printf_buf[CONFIG_PRINTF_BUF_SIZE] = {0};
#endif

#if (!CONFIG_SHELL_ASYNCLOG)
static uint8_t s_task_printf_enable = 1;
#endif

static beken_mutex_t s_printf_lock = NULL;

#if (!CONFIG_SHELL_ASYNCLOG)
void printf_lock(void)
{
        rtos_lock_mutex(&s_printf_lock);
}

void printf_unlock(void)
{
        rtos_unlock_mutex(&s_printf_lock);
}
#endif

int printf_lock_init(void)
{
        int ret = rtos_init_mutex(&s_printf_lock);
        if (kNoErr != ret) {
                return BK_ERR_NO_MEM;
        }

	return BK_OK;
}

int printf_lock_deinit(void)
{
	if (s_printf_lock)
		rtos_deinit_mutex(&s_printf_lock);

	s_printf_lock = NULL;
	return BK_OK;
}

#if (!CONFIG_SHELL_ASYNCLOG)
static void exception_mode_printf(const char *fmt, va_list ap)
{
	vsnprintf(s_exception_mode_printf_buf, sizeof(s_exception_mode_printf_buf) - 1, fmt, ap);
	s_exception_mode_printf_buf[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
	uart_write_string(CONFIG_UART_PRINT_PORT, s_exception_mode_printf_buf);
}

#if (!CONFIG_ARCH_RISCV)
static void irq_printf(const char *fmt, va_list ap)
{
	char string[CONFIG_PRINTF_BUF_SIZE];

	vsnprintf(string, sizeof(string) - 1, fmt, ap);
	string[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
	uart_write_string(CONFIG_UART_PRINT_PORT, string);
}
#endif

static void task_printf(const char *fmt, va_list ap)
{
	if(!s_task_printf_enable)
	{
	    return;
	}

	printf_lock();

	int len = vsnprintf(s_task_mode_printf_buf, sizeof(s_task_mode_printf_buf) - 1, fmt, ap);

	if (len <= sizeof(s_task_mode_printf_buf)) {
		s_task_mode_printf_buf[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
		uart_write_string(CONFIG_UART_PRINT_PORT, s_task_mode_printf_buf);
		printf_unlock();
		return;
	} else {
		printf_unlock(); //unlock first in case os_malloc() calls bk_printf()

		char *very_long_string = os_malloc(len + 1);

		if (!very_long_string)
			return;

		vsnprintf(very_long_string, len, fmt, ap);
		very_long_string[len] = 0;

		printf_lock();
		uart_write_string(CONFIG_UART_PRINT_PORT, very_long_string);
		printf_unlock();

		os_free(very_long_string);
	}
}
#endif //#if (!CONFIG_SHELL_ASYNCLOG)

void bk_printf(const char *fmt, ...)
{
#if CONFIG_SHELL_ASYNCLOG
	va_list args;

	if (!printf_is_init())
		return;

	va_start(args, fmt);
	
#if (CONFIG_SLAVE_CORE)
	shell_log_out_port(7, CPU1_TAG, fmt, args);
#else
	shell_log_out_port(7, NULL, fmt, args);
#endif

	va_end(args);

	return;
#elif  (CONFIG_ARCH_RISCV)
	va_list args;

	if (!printf_is_init())
		return;

	va_start(args, fmt);
  	if (rtos_is_in_interrupt_context() || (!rtos_is_scheduler_started()))
		exception_mode_printf(fmt, args);
	else
		task_printf(fmt, args);

	va_end(args);

	return;
#else // #if CONFIG_SHELL_ASYNCLOG
	uint32_t cpsr_val = rtos_get_cpsr();
	uint32_t arm_mode = cpsr_val & /*ARM968_MODE_MASK*/0x1f;
	va_list args;

	if (!printf_is_init())
		return;

	va_start(args, fmt);
	if ((/*ARM_MODE_FIQ*/17 == arm_mode)
		|| (/*ARM_MODE_ABT*/23 == arm_mode)
		|| (/*ARM_MODE_UND*/27 == arm_mode)
		|| (!rtos_is_scheduler_started()))
		exception_mode_printf(fmt, args);
	else if (/*ARM_MODE_IRQ*/18 == arm_mode)
		irq_printf(fmt, args);
	else
		task_printf(fmt, args);

	va_end(args);

	return;
#endif // #if CONFIG_SHELL_ASYNCLOG
}

void bk_set_printf_enable(uint8_t enable)
{
#if CONFIG_SHELL_ASYNCLOG
	if(0 == enable) {
		shell_echo_set(0);
		shell_set_log_level(9);
	} else {
		shell_echo_set(1);
		shell_set_log_level(0);
	}
#else
	s_task_printf_enable = enable;
#endif
}
