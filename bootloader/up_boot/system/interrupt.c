/*************************************************************
 * @file        interrupt.c
 * @brief       code of interrupt driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */
#include <stdio.h>
#include "interrupt.h"
#include "BK_System.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"

void int_spurious(void)
{
}

void int_enable(int index)
{
	ICU_INT_ENABLE_SET(0x01UL << index);
}

void int_disable(int index)
{
	ICU_INT_ENABLE_CLEAR(0x01UL << index);
}

void int_init(void)
{
}

void intc_start(void)
{
	u32 reg;
	u32 param;

	param = ICU_INT_GLOBAL_ENABLE_IRQ_SET;

	reg = REG_READ(REG_ICU_INT_GLOBAL_ENABLE_ADDR);
	reg |= param;
	REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, reg);

	return;
}

void intc_disable_top(void)
{
	u32 reg;
	u32 param;

	param = 0xffffffff;
	reg = REG_READ(REG_ICU_INT_GLOBAL_ENABLE_ADDR);
	reg &= ~(param);
	REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, reg);
}

void intc_forbidden_all(void)
{
	REG_WRITE(REG_ICU_INT_ENABLE_ADDR, 0);

#if (CFG_SOC_NAME == SOC_BK7271)
	REG_WRITE(ICU_FIQ_ENABLE, 0);
#endif

	REG_WRITE(REG_ICU_INT_GLOBAL_ENABLE_ADDR, 0);
	REG_WRITE(REG_ICU_INT_RAW_STATUS_ADDR, REG_READ(REG_ICU_INT_RAW_STATUS_ADDR));
#if (CFG_SOC_NAME == SOC_BK7271)
	REG_WRITE(ICU_FIQ_RAW_STATUS, REG_READ(ICU_FIQ_RAW_STATUS));
#endif

	REG_WRITE(REG_ICU_INT_STATUS_ADDR, REG_READ(REG_ICU_INT_STATUS_ADDR));
#if (CFG_SOC_NAME == SOC_BK7271)
	REG_WRITE(ICU_FIQ_STATUS, REG_READ(ICU_FIQ_STATUS));
#endif
}

void sys_forbidden_interrupts(void)
{
	__disable_irq();
	__disable_fiq();

	intc_disable_top();
	intc_forbidden_all();
}

void intc_irq(void)
{
	UINT32 irq_status;


	irq_status = REG_ICU_INT_STATUS & ICU_INT_ENABLE_IRQ_MASK;

	if (irq_status & ICU_INT_ENABLE_IRQ_UART0_MASK)
		UART0_InterruptHandler();

	if (irq_status & ICU_INT_ENABLE_IRQ_UART1_MASK)
		UART1_InterruptHandler();

	if (irq_status & ICU_INT_ENABLE_IRQ_PWM_MASK)
		pwm_isr();

	if (irq_status & ICU_INT_ENABLE_IRQ_GPIO_MASK)
		GPIO_InterruptHandler();
}

void intc_fiq(void)
{
	UINT32 fiq_status;

	fiq_status = REG_ICU_INT_STATUS & ICU_INT_ENABLE_FIQ_MASK;

	if (fiq_status & ICU_INT_ENABLE_FIQ_SDIO_DMA_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_GENERAL_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_PROT_TRIG_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_TX_TRIG_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_RX_TRIG_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_MISC_MASK)
		int_spurious();

	if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_TIMER_MASK) {
	}

	if (fiq_status & ICU_INT_ENABLE_FIQ_MODEM_MASK) {
	}
}

void do_irq(void)
{
	intc_irq();
}

void jump_pc_address(u32 ptr)
{
	typedef void (*jump_point)(void);
	jump_point jump = (jump_point)ptr;

	jump();
}

void bad_mode(void)
{
	jump_pc_address(0);
}

void do_undefined_instruction(void)
{
	bad_mode();
}

void do_software_interrupt(void)
{
}

void do_prefetch_abort(void)
{
	bad_mode();
}

void do_data_abort(void)
{
	bad_mode();
}

void do_not_used(void)
{
	bad_mode();
}

void do_fiq(void)
{
	intc_fiq();
}
// eof

