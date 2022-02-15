#include "include.h"
#include "bk_arm_arch.h"
#include "bk_timer.h"
#include "bk_drv_model.h"
#include "bk_api_int.h"
#include "bk_icu.h"
#include "bk_uart.h"
#include "icu_driver.h"

#if (CONFIG_SOC_BK7271)
#include "timer_bk7271.h"

void bk_timer_isr(void) __SECTION(".itcm");


static const DD_OPERATIONS bk_timer_op = {
	NULL,
	NULL,
	NULL,
	NULL,
	bk_timer_ctrl
};

void (*p_TIMER_Int_Handler[TIMER_CHANNEL_NO])(UINT8) = {NULL,};
volatile int current_timer_group = 0;

static UINT32 bk_timer_cal_endvalue(UINT32 ucChannel, UINT32 time_ms, UINT32 div)
{
	UINT64 value;

	if (div == 0)
		div = 1;

	if (ucChannel < 3) {
		/*26m clock*/
		value = time_ms * 26000 / div;
	} else {
		/*32k clock*/
		value = time_ms * 32 / div;
	}

	if (value > 0xffffffff)
		value = 0xffffffff;

	return (UINT32)value;
}

static UINT32 bk_timer_cal_endvalue_us(UINT32 ucChannel, UINT32 time_us, UINT32 div)
{
	UINT64 value;

	if (div == 0)
		div = 1;

	/*26m clock*/
	value = time_us * 26 / div;

	if (value > 0xffffffff)
		value = 0xffffffff;

	return (UINT32)value;
}

static UINT32 init_timer_param(timer_param_t *timer_param)
{
	UINT32 value;
	GLOBAL_INT_DECLARATION();
	UINT32 ucChannel = timer_param->channel;

	if (timer_param == NULL)
		return BK_TIMER_FAILURE;

	if (timer_param->channel > 5)
		return BK_TIMER_FAILURE;

	if (timer_param->channel > 5)
		return BK_TIMER_FAILURE;

	p_TIMER_Int_Handler[ucChannel] = timer_param->t_Int_Handler;

	GLOBAL_INT_DISABLE();
	if (ucChannel < 3) {
		current_timer_group = 0;

		value = (PWD_TIMER_26M_CLK_BIT);
		sddev_control(DD_DEV_TYPE_ICU, CMD_CLK_PWR_UP, (void *)&value);

		value = bk_timer_cal_endvalue(ucChannel, timer_param->period, timer_param->div);
		REG_WRITE(REG_TIMERCTLA_PERIOD_ADDR(ucChannel), value);

		value = REG_READ(TIMER0_2_CTL(current_timer_group));
		value &= ~(TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
		value |= ((timer_param->div - 1) << TIMERCTLA_CLKDIV_POSI);
		REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
	} else {
		current_timer_group = 1;

		value = (PWD_TIMER_32K_CLK_BIT);
		sddev_control(DD_DEV_TYPE_ICU, CMD_CLK_PWR_UP, (void *)&value);

		value = bk_timer_cal_endvalue(ucChannel, timer_param->period, timer_param->div);
		REG_WRITE(REG_TIMERCTLB_PERIOD_ADDR(ucChannel), value);

		value = REG_READ(TIMER0_2_CTL(current_timer_group));
		value &= ~(TIMERCTLB_CLKDIV_MASK << TIMERCTLB_CLKDIV_POSI);
		value |= ((timer_param->div - 1) << TIMERCTLB_CLKDIV_POSI);
		REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
	}
	GLOBAL_INT_RESTORE();

	os_printf("group:%d ,channel:%x,%x \r\n", current_timer_group, ucChannel,
			  REG_READ(TIMER0_2_CTL(current_timer_group)));

	icu_enable_timer_interrupt();

	return BK_TIMER_SUCCESS;
}

static UINT32 init_timer_param_us(timer_param_t *timer_param)
{
	UINT32 value;
	UINT32 ucChannel = timer_param->channel;
	GLOBAL_INT_DECLARATION();

	if (timer_param == NULL)
		return BK_TIMER_FAILURE;

	if (timer_param->channel > 2)
		return BK_TIMER_FAILURE;

	current_timer_group = 0;
	p_TIMER_Int_Handler[ucChannel] = timer_param->t_Int_Handler;
	GLOBAL_INT_DISABLE();

	value = (PWD_TIMER_26M_CLK_BIT);
	sddev_control(DD_DEV_TYPE_ICU, CMD_CLK_PWR_UP, (void *)&value);

	value = bk_timer_cal_endvalue_us(ucChannel, timer_param->period, timer_param->div);
	REG_WRITE(REG_TIMERCTLA_PERIOD_ADDR(ucChannel), value);

	value = REG_READ(TIMER0_2_CTL(current_timer_group));
	value &= ~(TIMERCTLA_CLKDIV_MASK << TIMERCTLA_CLKDIV_POSI);
	value |= ((timer_param->div - 1) << TIMERCTLA_CLKDIV_POSI);
	value |= (1 << ucChannel);
	REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
	GLOBAL_INT_RESTORE();

	icu_enable_timer_interrupt();

	return BK_TIMER_SUCCESS;
}

UINT32 bk_timer_ctrl(UINT32 cmd, void *param)
{
	int i_time_out;
	UINT32 ret = BK_TIMER_SUCCESS;
	UINT32 ucChannel;
	UINT32 value;
	timer_param_t *p_param;
	GLOBAL_INT_DECLARATION();

	switch (cmd) {
	case CMD_TIMER_UNIT_ENABLE:
		ucChannel = (*(UINT32 *)param);
		if (ucChannel > 5) {
			ret = BK_TIMER_FAILURE;
			break;
		}

		GLOBAL_INT_DISABLE();
		if (ucChannel < 3) {
			current_timer_group = 0;
			value = REG_READ(TIMER0_2_CTL(current_timer_group));
			value |= (1 << ucChannel);
			REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
		} else {
			current_timer_group = 1;
			value = REG_READ(TIMER0_2_CTL(current_timer_group));
			value |= (1 << (ucChannel - 3));
			REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
		}
		os_printf("group:%d ,channel:%x,%x \r\n", current_timer_group, ucChannel,
				  REG_READ(TIMER0_2_CTL(current_timer_group)));

		GLOBAL_INT_RESTORE();
		break;

	case CMD_TIMER_UNIT_DISABLE:
		ucChannel = (*(UINT32 *)param);
		if (ucChannel > 5) {
			ret = BK_TIMER_FAILURE;
			break;
		}

		GLOBAL_INT_DISABLE();
		if (ucChannel < 3) {
			current_timer_group = 0;
			value = REG_READ(TIMER0_2_CTL(current_timer_group));
			value &= ~(0x7 << TIMERCTLB_INT_POSI);
			value &= ~(1 << ucChannel);
			value |= (0x1 << (TIMERCTLB_INT_POSI + ucChannel));
			REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
		} else {
			current_timer_group = 1;
			value = REG_READ(TIMER0_2_CTL(current_timer_group));
			value &= ~(0x7 << TIMERCTLB_INT_POSI);
			value &= ~(1 << (ucChannel - 3));
			value |= (0x1 << (TIMERCTLB_INT_POSI + (ucChannel - 3)));
			REG_WRITE(TIMER0_2_CTL(current_timer_group), value);
		}
		GLOBAL_INT_RESTORE();
		break;

	case CMD_TIMER_INIT_PARAM:
		p_param = (timer_param_t *)param;
		ret = init_timer_param(p_param);
		break;

	case CMD_TIMER_INIT_PARAM_US:
		p_param = (timer_param_t *)param;
		ret = init_timer_param_us(p_param);
		break;

	case CMD_TIMER_READ_CNT:

		p_param = (timer_param_t *)param;
		i_time_out = 0;
		if (p_param->channel < 3) {
			current_timer_group = 0;
			REG_WRITE(TIMER0_2_READ_CTRL(current_timer_group), ((p_param->channel) << 2) | TIMER_READ_CNT_ENABLE);
			while (REG_READ(TIMER0_2_READ_CTRL(current_timer_group)) & 1) {
				i_time_out ++;
				if (i_time_out > (120 * 1000)) {
					ret = BK_TIMER_FAILURE;
					break;
				}
			}
			if (i_time_out <= (120 * 1000))
				p_param->period = REG_READ(TIMER0_2_READ_VALUE(current_timer_group));
		} else if (p_param->channel < 6) {
			current_timer_group = 1;
			value = REG_READ(TIMER0_2_READ_CTRL(current_timer_group));
			value &= ~(TIMER0_2_READ_CNT_MASK);
			value |= TIMER_READ_CNT_CHANNEL(p_param->channel - 3);
			value |= TIMER_READ_CNT_ENABLE;
			REG_WRITE(TIMER0_2_READ_CTRL(current_timer_group), value);

			while (REG_READ(TIMER0_2_READ_CTRL(current_timer_group)) & 1) {
				i_time_out ++;
				if (i_time_out > (120 * 1000)) {
					ret = BK_TIMER_FAILURE;
					break;
				}
			}
			if (i_time_out <= (120 * 1000)) {
				p_param->period = REG_READ(TIMER0_2_READ_VALUE(current_timer_group));
				//os_printf("read cnt :channel:%x cnt=%x\r\n",p_param->channel,p_param->period );
			}
		}
		break;
	default:
		ret = BK_TIMER_FAILURE;
		break;
	}

	return ret;
}

void bk_timer_init(void)
{
	UINT32 value;

	value = REG_READ(TIMER0_2_CTL(0));
	value &= ~(0x7);
	value &= ~(0x7 << 7);
	REG_WRITE(TIMER0_2_CTL(0), value);

	value = REG_READ(TIMER0_2_CTL(1));
	value &= ~(0x7);
	value &= ~(0x7 << 7);
	REG_WRITE(TIMER0_2_CTL(1), value);

	bk_int_isr_register(INT_SRC_TIMER, bk_timer_isr, NULL);
	sddev_register_dev(DD_DEV_TYPE_TIMER, (DD_OPERATIONS *)&bk_timer_op);
}

void bk_timer_exit(void)
{
	sddev_unregister_dev(DD_DEV_TYPE_TIMER);
}

void bk_timer_isr(void)
{
	int i;
	UINT32 status0, status1;
	status0 = REG_READ(TIMER0_2_CTL(0)) & TIMER0_3_INT_MASK;

	bk_printf("group:%d ,status:%x\r\n",current_timer_group, status0);

	for (i = 0; i < 3; i++) {
		if (status0 & (1 << (i + TIMERCTLA_INT_POSI))) {
			if (p_TIMER_Int_Handler[i])
				p_TIMER_Int_Handler[i]((UINT8)i);
		}
	}

	do {
		REG_WRITE(TIMER0_2_CTL(0), (REG_READ(TIMER0_2_CTL(0)) & (~(0x7 << TIMERCTLA_INT_POSI))) | status0);
	} while (REG_READ(TIMER0_2_CTL(0)) & status0 & (0x7 << TIMERCTLA_INT_POSI));

	status1 = REG_READ(TIMER0_2_CTL(1)) & TIMER0_3_INT_MASK;

	//os_printf("status1:%x\r\n",status1);

	for (i = 0; i < 3; i++) {
		if (status1 & (1 << (i + TIMERCTLB_INT_POSI))) {
			if (p_TIMER_Int_Handler[i + 3])
				p_TIMER_Int_Handler[i + 3]((UINT8)(i + 3));
		}
	}

	do {
		REG_WRITE(TIMER0_2_CTL(1), (REG_READ(TIMER0_2_CTL(1)) & (~(0x7 << TIMERCTLA_INT_POSI))) | status1);
	} while (REG_READ(TIMER0_2_CTL(1)) & status1 & (0x7 << TIMERCTLA_INT_POSI));
}

#endif

