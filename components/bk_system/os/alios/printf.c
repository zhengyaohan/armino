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
#include "printf_impl.h"

static void printf_impl(const char *fmt, va_list ap)
{
	//TODO handle long string case, need while
	char string[CONFIG_PRINTF_BUF_SIZE];

	vsnprintf(string, sizeof(string) - 1, fmt, ap);
	string[CONFIG_PRINTF_BUF_SIZE - 1] = 0;
	uart_write_string(CONFIG_UART_PRINT_PORT, string);
}

void bk_printf(const char *fmt, ...)
{
	va_list args;

	if (!printf_is_init())
		return;

	va_start(args, fmt);
	printf_impl(fmt, args);
	va_end(args);
}
