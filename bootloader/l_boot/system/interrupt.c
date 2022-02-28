/*************************************************************
 * @file        interrupt.c
 * @brief       code of interrupt driver of BK7231
 */

#include <stdio.h>
#include "interrupt.h"
#include "BK_System.h"

#include "driver_icu.h"
#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"

extern void do_irq( void );
extern void do_fiq( void );
extern void do_swi( void );
extern void do_undefined_instruction( void );

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
    *((volatile u32 *)0x400000) = (u32)&do_irq;
    *((volatile u32 *)0x400004) = (u32)&do_fiq;
    *((volatile u32 *)0x400008) = (u32)&do_swi;
    *((volatile u32 *)0x40000c) = (u32)&do_undefined_instruction;
    *((volatile u32 *)0x400010) = (u32)&do_undefined_instruction;
    *((volatile u32 *)0x400014) = (u32)&do_undefined_instruction;
    *((volatile u32 *)0x400018) = (u32)&do_undefined_instruction;
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

#if (CFG_SOC_NAME == SOC_BK7271)
    irq_status = REG_ICU_INT_STATUS;
#else
    irq_status = REG_ICU_INT_STATUS & ICU_INT_ENABLE_IRQ_MASK;
#endif

    if (irq_status & ICU_INT_ENABLE_IRQ_UART0_MASK)
    {
        UART0_InterruptHandler();
    }

    if (irq_status & ICU_INT_ENABLE_IRQ_UART1_MASK)
    {
        UART1_InterruptHandler();
    }
#if (CFG_SOC_NAME == SOC_BK7271)
    if (irq_status & ICU_INT_ENABLE_IRQ_UART2_MASK)
    {
        //UART2_InterruptHandler();
    }
#endif
    if (irq_status & ICU_INT_ENABLE_IRQ_PWM_MASK)
    {
        pwm_isr();
    }

    #if ((1 == CFG_TICK_USE_TIMER) && CFG_SOC_NAME != SOC_BK7231)
    if (irq_status & ICU_INT_ENABLE_IRQ_TIMER_MASK)
    {
        bk_timer_isr();
    }
    #endif

    if (irq_status & ICU_INT_ENABLE_IRQ_GPIO_MASK)
    {
        GPIO_InterruptHandler();
    }
}

void intc_fiq(void)
{
    UINT32 fiq_status;
#if (CFG_SOC_NAME == SOC_BK7271)
    fiq_status = REG_READ(ICU_FIQ_STATUS) & 0x000000FF;
#else
    fiq_status = REG_ICU_INT_STATUS & ICU_INT_ENABLE_FIQ_MASK;
#endif

    if (fiq_status & ICU_INT_ENABLE_FIQ_SDIO_DMA_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_GENERAL_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_PROT_TRIG_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_TX_TRIG_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_RX_TRIG_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_MISC_MASK)
    {
        int_spurious();
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MAC_TIMER_MASK)
    {
    }

    if (fiq_status & ICU_INT_ENABLE_FIQ_MODEM_MASK)
    {
    }
}

void jump_pc_address(u32 ptr)
{
    typedef void (*jump_point)(void);
    jump_point jump = (jump_point)ptr;

    jump();
}

void bad_mode (void)
{
    wdt_reboot();
}

void do_anomaly_print (void)
{
    bk_printf("[ARM ANOMALY][");
    bk_printf("2]\r\n");
#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)(CRASH_IN_BOOT_ABNORMAL_VALUE & 0xffff);
#else
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_IN_BOOT_ABNORMAL_VALUE;
#endif
    bad_mode ();
}

void do_software_interrupt (void)
{
}
// eof

