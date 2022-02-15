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

#include "sys_rtos.h"
#include "rtdef.h"
#include "rthw.h"
#include "bk_api_uart.h"
#include "bk_uart.h"

static void printf_impl(const char *fmt, va_list ap)
{
	rt_size_t length;
	static char rt_log_buf[RT_CONSOLEBUF_SIZE];

	/* the return value of vsnprintf is the number of bytes that would be
	 * written to buffer had if the size of the buffer been sufficiently
	 * large excluding the terminating null byte. If the output string
	 * would be larger than the rt_log_buf, we have to adjust the output
	 * length. */
	length = rt_vsnprintf(rt_log_buf, sizeof(rt_log_buf) - 1, fmt, ap);
	if (length > RT_CONSOLEBUF_SIZE - 1)
		length = RT_CONSOLEBUF_SIZE - 1;
	rt_kprintf("%s", rt_log_buf);
}

void bk_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	printf_impl(fmt, args);
	va_end(args);
}

