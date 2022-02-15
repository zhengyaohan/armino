#include "include.h"
#include "bk_arm_arch.h"
#include "pwm.h"
#include "bk_pwm.h"
#include "bk_timer.h"
#include "pwm_timer.h"
#include "bk_icu.h"
#include "bk_fake_clock.h"
#include "bk_mcu_ps.h"
#include "bk_misc.h"
#include "wlan_ui_pub.h"
#include "bk_intc.h"
#include "bk_api_tick.h"
#include "sys_driver.h"

#if (CONFIG_SOC_BK7271)
#include "../icu/icu_bk7271.h"
#else
#include "icu.h"
#endif

void ps_pwm_enable(void)
{
	UINT32 reg = 0;

	reg = REG_READ(PWM_CTL);
	reg &= (~(0xf << (PS_PWM_ID * 4)));
	reg |= (0x7 << (PS_PWM_ID * 4));
	REG_WRITE(PWM_CTL, reg);
}

void ps_pwm_disable(void)
{
	UINT32 reg;

	reg = REG_READ(PWM_CTL);
	REG_WRITE(PWM_CTL, reg & (~(0xf << (PS_PWM_ID * 4))));
}

void ps_pwm_set_period(UINT32 period, UINT8 clk_mux)
{
	uint32_t reg = 0, ch;

	ch = PS_PWM_ID;
	sys_drv_pwm_set_clock(clk_mux, ch);

#if (CONFIG_SOC_BK7231)
	reg = 0;
	reg |= period << PWM0_END_POSI | 0x0 << PWM0_DC_POSI;
	REG_WRITE(MCU_PS_PWM_COUNTER, reg);
#else
	reg = period;
	REG_WRITE(MCU_PS_PWM_COUNTER, reg);
	reg = 0;
	REG_WRITE(MCU_PS_PWM_DUTY_CYCLE, reg);
#endif
}


void ps_pwm_reconfig(UINT32 period, UINT8 clk_mux)
{
	//disable
#if (CONFIG_SOC_BK7231)
	ps_pwm_disable();
	delay(5);
	//new
	ps_pwm_set_period(period, clk_mux);
	delay(1);
	//reenable
	ps_pwm_enable();

	REG_WRITE(PWM_INTERRUPT_STATUS, 0x1);
#else
	ps_pwm_disable();
	//new
	ps_pwm_set_period(period, clk_mux);
	//reenable
	ps_pwm_enable();
	delay(5);

	REG_WRITE(PWM_INTERRUPT_STATUS, 0x3f);
#endif

}

void ps_pwm_suspend_tick(UINT32 period)
{
	ps_pwm_reconfig(period, PWM_MUX_LPO);
}

static uint32_t cal_endvalue(UINT32 mode)
{
        uint32_t value = 1;

        if (PWM_CLK_32K == mode) {
                /*32k clock*/
                value = bk_get_ms_per_tick() * 32;
        } else if (PWM_CLK_26M == mode) {
                /*26m clock*/
                value = bk_get_ms_per_tick() * 26000;
        }

        return value;
}

void ps_pwm_resume_tick(void)
{
	ps_pwm_reconfig(cal_endvalue(PWM_CLK_26M), PWM_MUX_PCLK);
}

UINT32 ps_pwm_int_status(void)
{
	return ((REG_READ(ICU_INT_STATUS) & (BIT(IRQ_PWM))) && (REG_READ(PWM_INTERRUPT_STATUS) & 0x1));
}

#if (!CONFIG_SOC_BK7231)
UINT32 timer_0_2_en_value;
void ps_timer02_disable(void)
{
	UINT32 reg;

	reg = REG_READ(TIMER0_2_CTL);
	timer_0_2_en_value = reg;
	reg &= (~TIMERCTLB_CLKDIV_MASK);
	REG_WRITE(TIMER0_2_CTL, reg);
}

void ps_timer02_restore(void)
{
	REG_WRITE(TIMER0_2_CTL, timer_0_2_en_value);
}

UINT32 ps_timer2_get(void)
{
	UINT32 reg = 0;

#if (CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
	reg = REG_READ(TIMER0_2_READ_CTL);
	reg &= ~(TIMER0_2_READ_INDEX_MASK << TIMER0_2_READ_INDEX_POSI);
	reg |= (TIMER0_2_READ_INDEX_2 << TIMER0_2_READ_INDEX_POSI);
	REG_WRITE(TIMER0_2_READ_CTL, reg);
	delay(2);

	reg = REG_READ(TIMER0_2_READ_CTL);
	reg |= TIMER0_2_READ_OP_BIT;
	REG_WRITE(TIMER0_2_READ_CTL, reg);
	delay(2);

	while (REG_READ(TIMER0_2_READ_CTL) & TIMER0_2_READ_OP_BIT);
	reg = REG_READ(TIMER0_2_READ_VALUE);
#endif

	return (reg / 26000);
}


void ps_timer3_enable(UINT32 period)
{
	UINT32 reg;

#if (CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
	reg = REG_READ(TIMER3_5_READ_CTL);
	reg &= ~(TIMER3_5_READ_INDEX_MASK << TIMER3_5_READ_INDEX_POSI);
	reg |= (TIMER3_5_READ_INDEX_3 << TIMER3_5_READ_INDEX_POSI);
	REG_WRITE(TIMER3_5_READ_CTL, reg);
#endif

	if (bk_get_tick_timer_id() == BK_TIMER_ID3) {
		reg = REG_READ(TIMER3_5_CTL);
		reg &= (~TIMERCTL3_EN_BIT);
		REG_WRITE(TIMER3_5_CTL, reg);
		delay(2);
	}

	REG_WRITE(TIMER3_CNT, period);
	delay(2);
	reg = REG_READ(TIMER3_5_CTL);
	reg |= (TIMERCTL3_EN_BIT);
	REG_WRITE(TIMER3_5_CTL, reg);
}

UINT32 ps_timer3_measure_prepare(void)
{
#if (CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
	UINT32 reg;
	if (!(REG_READ(TIMER3_5_CTL) & (TIMERCTL3_INT_BIT))) {
		reg = REG_READ(TIMER3_5_READ_CTL);
		reg |= TIMER3_5_READ_OP_BIT;
		REG_WRITE(TIMER3_5_READ_CTL, reg);
	}
#endif

	return 0;
}

UINT32 ps_timer3_disable(void)
{
	UINT32 reg, less;
	if (REG_READ(TIMER3_5_CTL) & (TIMERCTL3_INT_BIT))
		less = REG_READ(TIMER3_CNT);
	else {
#if (CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
		while (REG_READ(TIMER3_5_READ_CTL) & TIMER3_5_READ_OP_BIT);
		less = REG_READ(TIMER3_5_READ_VALUE);
#else
		less = REG_READ(TIMER3_CNT);
#endif
	}

	reg = REG_READ(TIMER3_5_CTL);
	reg &= (~TIMERCTL3_EN_BIT);
	reg &= ~(0x7 << TIMERCTLB_INT_POSI);
	REG_WRITE(TIMER3_5_CTL, reg);

	if (bk_get_tick_timer_id() == BK_TIMER_ID3) {
		delay(2);
		REG_WRITE(TIMER3_CNT, bk_get_ms_per_tick() * 32);
		delay(2);
		reg = REG_READ(TIMER3_5_CTL);
		reg |= (TIMERCTL3_EN_BIT);
		REG_WRITE(TIMER3_5_CTL, reg);
	}

	return (less / 32);
}
#endif

