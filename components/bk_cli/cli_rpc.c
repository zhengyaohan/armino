/**
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 BEKEN Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of BEKEN Corporation.
 *
 */
#include <stdlib.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>

#include "bk_cli.h"
#include "stdarg.h"
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_phy.h"
#include "cli.h"
#include "cli_config.h"
#include <components/log.h>
#include <driver/uart.h>
#include "bk_rtos_debug.h"
#if CONFIG_SHELL_ASYNCLOG
#include "shell_task.h"
#endif
#include "bk_api_cli.h"

static void debug_help_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#if CONFIG_DUAL_CORE
#include "rpc_mb.h"
#include <driver/gpio.h>

#include "spinlock.h"

static void debug_rpc_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_rpc_gpio_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
static void debug_spinlock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

spinlock_t		gpio_spinlock;
spinlock_t  *	gpio_spinlock_ptr;

static u8     rpc_client = 0;
static u8     rpc_inited = 0;
#endif

#if CONFIG_ARCH_RISCV
static void debug_perfmon_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

const struct cli_command debug_cmds[] = {
	{"help", "list debug cmds", debug_help_command},
#if CONFIG_DUAL_CORE
	{"rpc", "rpc {server|client}", debug_rpc_command},
	{"gpio_out", "gpio_out gpio_id {0|1}", debug_rpc_gpio_command},
	{"spin_lock", "spin_lock [timeout 1~20]", debug_spinlock_command},
#endif

#if CONFIG_ARCH_RISCV
	{"perfmon", "perfmon(calc MIPS)", debug_perfmon_command},
#endif
};

const int cli_debug_table_size = ARRAY_SIZE(debug_cmds);

static void print_cmd_table(const struct cli_command *cmd_table, int table_items)
{
	int i;

	for (i = 0; i < table_items; i++)
	{
		if (cmd_table[i].name)
		{
			if (cmd_table[i].help)
				os_printf("%s: %s\r\n", cmd_table[i].name, cmd_table[i].help);
			else
				os_printf("%s\r\n", cmd_table[i].name);
		}
	}
}

static void debug_help_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("====Debug Commands====\r\n");

	print_cmd_table(debug_cmds, ARRAY_SIZE(debug_cmds));
}

#if CONFIG_DUAL_CORE
static void print_cmd_help(const struct cli_command *cmd_table, int table_items, void *func)
{
	int i;

	for (i = 0; i < table_items; i++)
	{
		if(cmd_table[i].function == func)
		{
			if (cmd_table[i].help)
				os_printf("%s\r\n", cmd_table[i].help);

			break;
		}
	}
}

static void print_debug_cmd_help(void *func)
{
	print_cmd_help(debug_cmds, ARRAY_SIZE(debug_cmds), func);
}

static void debug_rpc_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret_val;

	if (argc < 2)
	{
		print_debug_cmd_help(debug_rpc_command);
	//	os_printf("rpc {server|client}\r\n");
		return;
	}

	if(rpc_inited)
	{
		os_printf("rpc %s inited\r\n", rpc_client ? "client":"server");
		return;
	}

	if (os_strcmp(argv[1], "server") == 0)
	{
#if CONFIG_MASTER_CORE
		ret_val = rpc_server_init();
		os_printf("rpc svr%d\r\n", ret_val);
		rpc_client = 0;
		rpc_inited = 1;
#endif
	}
	else if(os_strcmp(argv[1], "client") == 0)
	{
#if CONFIG_SLAVE_CORE
		ret_val = rpc_client_init();
		os_printf("rpc client%d\r\n", ret_val);
		rpc_client = 1;
		rpc_inited = 1;

		client_send_simple_cmd(CPU1_POWER_UP, 0, 0);
		client_send_simple_cmd(SIMPLE_TEST_CMD, 12, 34);
		client_send_simple_cmd(GET_SPINLOCK_GPIO, 0, 0);
#endif
	}
	else
	{
	//	os_printf("rpc {server|client}\r\n");
		print_debug_cmd_help(debug_rpc_command);
		return;
	}
}

extern bk_err_t bk_gpio_enable_output_rpc(gpio_id_t gpio_id);
extern bk_err_t bk_gpio_set_output_high_rpc(gpio_id_t gpio_id);
extern bk_err_t bk_gpio_set_output_low_rpc(gpio_id_t gpio_id);

static void debug_rpc_gpio_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SLAVE_CORE
	u32 gpio_id = 0;
	u32 level = 0;

	if(rpc_client == 0)
	{
		os_printf("Failed: it is not a client.\r\n");
		return;
	}

	if (argc < 3)
	{
		print_debug_cmd_help(debug_rpc_gpio_command);
		return;
	}

	gpio_id = strtoul(argv[1], NULL, 0);
	level   = strtoul(argv[2], NULL, 0);

	bk_gpio_enable_output_rpc(gpio_id);

	if(level)
		bk_gpio_set_output_high_rpc(gpio_id);
	else
		bk_gpio_set_output_low_rpc(gpio_id);
#endif

}

static void debug_spinlock_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u32 timeout_second = 10;  // 10s

	if(rpc_inited == 0)
	{
		os_printf("Failed: no rpc client/server in CPU0/CPU1.\r\n");
		return;
	}

	if (argc > 1)
	{
		timeout_second = strtoul(argv[1], NULL, 0);

		if(timeout_second > 20)
			timeout_second = 20;
		if(timeout_second == 0)
			timeout_second = 10;
	}

	if(rpc_client == 0)
	{
		spinlock_acquire(&gpio_spinlock);
		os_printf("spinlock acquired %d\r\n", gpio_spinlock.locked);
		rtos_delay_milliseconds(timeout_second * 1000);
		spinlock_release(&gpio_spinlock);
		os_printf("spinlock released %d\r\n", gpio_spinlock.locked);
	}
	else
	{
		spinlock_acquire(gpio_spinlock_ptr);
		os_printf("spinlock acquired %d\r\n", gpio_spinlock_ptr->locked);
		rtos_delay_milliseconds(timeout_second * 1000);
		spinlock_release(gpio_spinlock_ptr);
		os_printf("spinlock released %d\r\n", gpio_spinlock_ptr->locked);
	}
}
#endif

const struct cli_command * cli_debug_cmd_table(int *num)
{
	*num = ARRAY_SIZE(debug_cmds);

	return &debug_cmds[0];
}

#if CONFIG_ARCH_RISCV

extern u64 riscv_get_instruct_cnt(void);
extern u64 riscv_get_mtimer(void);

static u64 		saved_time = 0;
static u64 		saved_inst_cnt = 0;

static void debug_perfmon_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	u64 cur_time = riscv_get_mtimer();
	u64 cur_inst_cnt = riscv_get_instruct_cnt();

	os_printf("cur time: %x:%08x\r\n", (u32)(cur_time >> 32), (u32)(cur_time & 0xFFFFFFFF));
	os_printf("cur inst_cnt: %x:%08x\r\n", (u32)(cur_inst_cnt >> 32), (u32)(cur_inst_cnt & 0xFFFFFFFF));

	saved_time = (cur_time - saved_time) / 26;
	saved_inst_cnt = cur_inst_cnt - saved_inst_cnt;

//	os_printf("elapse time(us): %x:%08x\r\n", (u32)(saved_time >> 32), (u32)(saved_time & 0xFFFFFFFF));
//	os_printf("diff inst_cnt: %x:%08x\r\n", (u32)(saved_inst_cnt >> 32), (u32)(saved_inst_cnt & 0xFFFFFFFF));

	os_printf("MIPS: %d KIPS\r\n", (u32)(saved_inst_cnt * 1000 / saved_time));

	saved_time = cur_time;
	saved_inst_cnt = cur_inst_cnt;
}

#endif

