/*
 * Copyright (c) 2012-2021 Andes Technology Corporation
 * All rights reserved.
 *
 */

#include <stdint.h>
#include "boot.h"
#include "sdkconfig.h"
#include "bk_private/reset_reason.h"
#include <os/os.h>
#include "bk_arch.h"
#include "stack_base.h"
#include "bk_uart.h"
#include "bk_rtos_debug.h"
#include <os/os.h>
#include <stdio.h>
#include "platform.h"
#include "bk_api_printf.h"
#include <os/mem.h>

#if (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7235)
#define RAM_BASE_ADDR 0x30000000
#define RAM_DUMP_SIZE (352*1024)
#elif CONFIG_SOC_BK7256_CP1
#define RAM_BASE_ADDR 0x30060000
#define RAM_DUMP_SIZE (128*1024)
#endif

#define SYS_DELAY_TIME_5S	    (85000000UL)

typedef struct {
	union {
		struct {
			long x1;
			long x4;
			long x5;
			long x6;
			long x7;
			long x10;
			long x11;
			long x12;
			long x13;
			long x14;
			long x15;
			long x16;
			long x17;
			long x28;
			long x29;
			long x30;
			long x31;
		};
		long caller_regs[17];
	};
	long mepc;
	long mstatus;
	long mxstatus;
#ifdef __riscv_flen
#if __riscv_flen == 64
	long long fp_caller_regs[20];
#else
	long fp_caller_regs[20];
#endif
	int fcsr;
#endif
} SAVED_CONTEXT;
extern void mtime_handler(void);
extern void mswi_handler(void);
extern void mext_interrupt(void);
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
extern void user_except_handler (unsigned long mcause, SAVED_CONTEXT *context);

volatile unsigned int g_enter_exception = 0;

unsigned int arch_is_enter_exception(void) {
	return g_enter_exception;
}

__attribute__((weak)) void syscall_handler(long n, long a0, long a1, long a2, long a3)
{
	os_printf("syscall #%ld (a0:0x%lx,a1:0x%lx, a2:0x%lx, a3:0x%lx)\n", n, a0, a1, a2, a3);
}

__attribute__((weak)) long except_handler(long cause, long epc, long *reg)
{
	/* Unhandled Trap */
	os_printf("Unhandled Trap : mcause = 0x%x, mepc = 0x%x\n", (unsigned int)cause, (unsigned int)epc);
	while(1);
	return epc;
}

void trap_handler(unsigned long mcause, SAVED_CONTEXT *context)
{
	// Make sure the interrupt is disable
	rtos_stop_int();

	if (0 == g_enter_exception) {
		/* Handled Trap */
		g_enter_exception = 1;
		bk_set_printf_sync(true);
		
		user_except_handler(mcause, context);
		os_printf("Unhandled Trap : mcause = 0x%x, mepc = 0x%x\n", mcause, context->mepc);
		while(g_enter_exception);
	}

}


/**
 * this function will show registers of CPU
 *
 * @param mcause
 * @param context
 */
void arch_dump_cpu_registers (unsigned long mcause, SAVED_CONTEXT *context)
{

	os_printf("Current regs:\n");


	os_printf("1 ra x 0x%lx\n", context->x1);
	os_printf("2 sp x 0x%lx\n", context);
	os_printf("4 tp x 0x%lx\n", context->x4);
	os_printf("5 t0 x 0x%lx\n", context->x5);
	os_printf("6 t1 x 0x%lx\n", context->x6);
	os_printf("7 t2 x 0x%lx\n", context->x7);
	os_printf("10 a0 x 0x%lx\n", context->x10);
	os_printf("11 a1 x 0x%lx\n", context->x11);
	os_printf("12 a2 x 0x%lx\n", context->x12);
	os_printf("13 a3 x 0x%lx\n", context->x13);
	os_printf("14 a4 x 0x%lx\n", context->x14);
	os_printf("15 a5 x 0x%lx\n", context->x15);
	os_printf("16 a6 x 0x%lx\n", context->x16);
	os_printf("17 a7 x 0x%lx\n", context->x17);
	os_printf("28 t3 x 0x%lx\n", context->x28);
	os_printf("29 t4 x 0x%lx\n", context->x29);
	os_printf("30 t5 x 0x%lx\n", context->x30);
	os_printf("31 t6 x 0x%lx\n", context->x31);
	os_printf("32 pc x 0x%lx\n", context->mepc-4);

#ifdef __riscv_flen
	os_printf("68 fcsr x 0x%lx\n", context->fcsr);
#endif

	os_printf("833 mstatus x 0x%lx\n", context->mstatus);

	os_printf("898 mepc x 0x%lx\n", context->mepc);
	os_printf("899 mcause x 0x%lx\n", mcause);

	os_printf("2053 mxstatus x 0x%lx\n", context->mxstatus);

	os_printf("\r\n");
}

void sys_delay_sync(uint32_t time_count )
{
	volatile UINT32 i = 0;

	for (i = 0; i < time_count; i ++)
		;
}

void user_except_handler (unsigned long mcause, SAVED_CONTEXT *context) {
	os_printf("***********************************************************************************************\n");
	os_printf("***********************************user except handler begin***********************************\n");
	os_printf("***********************************************************************************************\n");

	arch_dump_cpu_registers(mcause, context);
	
	os_printf("System will dump memory in 5s, please ready to save whole log.........\n");

	sys_delay_sync(SYS_DELAY_TIME_5S);

#if CONFIG_MEMDUMP_ALL			
	stack_mem_dump(RAM_BASE_ADDR, RAM_BASE_ADDR + RAM_DUMP_SIZE);
#endif

#if CONFIG_FREERTOS
	rtos_dump_backtrace();
	rtos_dump_task_list();
	rtos_dump_task_runtime_stats();
#endif

#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
	os_dump_memory_stats(0, 0, NULL);
#endif

	arch_dump_cpu_registers(mcause, context);

	os_printf("***********************************************************************************************\n");
	os_printf("************************************user except handler end************************************\n");
	os_printf("***********************************************************************************************\n");
}

void set_reboot_tag(uint32_t tag) {
	uint32_t p_tag = REBOOT_TAG_ADDR;
	*((uint32_t *)p_tag) = tag;
}

uint32_t get_reboot_tag(void) {
	uint32_t p_tag = REBOOT_TAG_ADDR;
	return *((uint32_t *)p_tag);
}

extern void trap_entry(void);

void user_nmi_handler(unsigned long mcause) {
	uint32_t tag = 0;
	if(mcause == MCAUSE_CAUSE_WATCHDOG) {
		tag = get_reboot_tag();
		if( REBOOT_TAG_REQ == tag ) {
			os_printf("Wait reboot.\n");
			while(1);
		}
	}
	//TODO: close wdt
	trap_entry();
}
