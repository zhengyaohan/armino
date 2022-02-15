#include "include.h"
#include "bk_arm_arch.h"

#include "bk_icu.h"
#include "icu.h"

#include "bk_drv_model.h"
#include "bk_api_rtos.h"
#include "sys_driver.h"
#if !CONFIG_SOC_BK7256
static const DD_OPERATIONS icu_op = {
	NULL,
	NULL,
	NULL,
	NULL,
	icu_ctrl
};

/*******************************************************************/
void icu_init(void)
{
	UINT32 param;
	
	sddev_register_dev(DD_DEV_TYPE_ICU, (DD_OPERATIONS *)&icu_op);

	/*pclk select 26m */
#if (CONFIG_SOC_BK7231)
	param = PCLK_POSI;
#else
	param = PCLK_POSI_UART1 | PCLK_POSI_UART2
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
			| PCLK_POSI_SARADC
#endif
#if (CONFIG_SOC_BK7271)
			| PCLK_POSI_UART3
#endif
			| PCLK_POSI_PWMS | PCLK_POSI_SDIO
			| PCLK_POSI_I2C1 | PCLK_POSI_I2C2;
#endif // (CONFIG_SOC_BK7231)
	icu_ctrl(CMD_CONF_PCLK_26M, &param);

#if (CONFIG_SOC_BK7231)
	param = FIQ_MAILBOX0_BIT | FIQ_MAILBOX1_BIT;
	// icu_ctrl(CMD_ICU_INT_DISABLE, &param);
	(void)sys_drv_int_disable(param);
#endif // (CONFIG_SOC_BK7231)

	REG_WRITE(ICU_ARM_WAKEUP_EN, 0);
}

void icu_exit(void)
{
	sddev_unregister_dev(DD_DEV_TYPE_ICU);
}

UINT32 icu_ctrl(UINT32 cmd, void *param)
{
	UINT32 ret;
	UINT32 reg;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	ret = ICU_SUCCESS;

	switch (cmd) {
	case CMD_CONF_PCLK_26M:
		reg = REG_READ(ICU_PERI_CLK_MUX);
#if (CONFIG_SOC_BK7231)
		reg |= (1 << (*(UINT32 *)param));
#else
		reg |= (*(UINT32 *)param);
#endif // (CONFIG_SOC_BK7231)
		REG_WRITE(ICU_PERI_CLK_MUX, reg);
		break;


	case CMD_CONF_PWM_LPOCLK:
		reg = REG_READ(ICU_PWM_CLK_MUX);
		reg |= (1 << (*(UINT32 *)param));
		REG_WRITE(ICU_PWM_CLK_MUX, reg);
		break;

	case CMD_CONF_PCLK_DCO:
		reg = REG_READ(ICU_PERI_CLK_MUX);
#if (CONFIG_SOC_BK7231)
		reg &= ~(1 << (*(UINT32 *)param));
#else
		reg &= ~(*(UINT32 *)param);
#endif // (CONFIG_SOC_BK7231)
		REG_WRITE(ICU_PERI_CLK_MUX, reg);
		break;

	case CMD_CONF_PWM_PCLK:
		reg = REG_READ(ICU_PWM_CLK_MUX);
		reg &= ~(1 << (*(UINT32 *)param));
		REG_WRITE(ICU_PWM_CLK_MUX, reg);
		break;

	case CMD_CLK_PWR_UP:
		reg = REG_READ(ICU_PERI_CLK_PWD);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_PERI_CLK_PWD, reg);
		break;

	case CMD_CLK_PWR_DOWN:
		reg = REG_READ(ICU_PERI_CLK_PWD);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_PERI_CLK_PWD, reg);
		break;

	case CMD_TL410_CLK_PWR_UP:
		reg = REG_READ(ICU_TL410_CLK_PWD);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_TL410_CLK_PWD, reg);
		break;

	case CMD_TL410_CLK_PWR_DOWN:
		reg = REG_READ(ICU_TL410_CLK_PWD);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_TL410_CLK_PWD, reg);
		break;

	case CMD_ICU_CLKGATING_DISABLE:
		reg = REG_READ(ICU_PERI_CLK_GATING);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_PERI_CLK_GATING, reg);
		break;

	case CMD_ICU_CLKGATING_ENABLE:
		reg = REG_READ(ICU_PERI_CLK_GATING);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_PERI_CLK_GATING, reg);
		break;

	case CMD_ICU_INT_DISABLE:
		reg = REG_READ(ICU_INTERRUPT_ENABLE);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_INTERRUPT_ENABLE, reg);
		break;

	case CMD_ICU_INT_ENABLE:
		reg = REG_READ(ICU_INTERRUPT_ENABLE);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_INTERRUPT_ENABLE, reg);
		break;

#if (CONFIG_SOC_BK7271)
	case CMD_FUNC_CLK_PWR_UP:
		reg = REG_READ(ICU_FUNC_CLK_PWD);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_FUNC_CLK_PWD, reg);
		break;

	case CMD_FUNC_CLK_PWR_DOWN:
		reg = REG_READ(ICU_FUNC_CLK_PWD);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_FUNC_CLK_PWD, reg);
		break;

	case CMD_ICU_FIQ_DISABLE:
		reg = REG_READ(ICU_FIQ_ENABLE);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_FIQ_ENABLE, reg);
		break;

	case CMD_ICU_FIQ_ENABLE:
		reg = REG_READ(ICU_FIQ_ENABLE);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_FIQ_ENABLE, reg);
		break;

	case CMD_GET_FIQ_REG_STATUS:
		ret = REG_READ(ICU_FIQ_STATUS);
		break;

	case CMD_CLR_FIQ_REG_STATUS:
		BK_ASSERT(param);
		reg = REG_READ(ICU_FIQ_STATUS);
		REG_WRITE(ICU_FIQ_STATUS, (reg | *(UINT32 *)param));
		break;
#endif

	case CMD_ICU_GLOBAL_INT_DISABLE:
		reg = REG_READ(ICU_GLOBAL_INT_EN);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_GLOBAL_INT_EN, reg);
		break;

	case CMD_ICU_GLOBAL_INT_ENABLE:
		reg = REG_READ(ICU_GLOBAL_INT_EN);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_GLOBAL_INT_EN, reg);
		break;

	case CMD_GET_INTR_STATUS:
		ret = REG_READ(ICU_INT_STATUS);
		break;

	case CMD_CLR_INTR_STATUS:
		BK_ASSERT(param);
		reg = REG_READ(ICU_INT_STATUS);
		REG_WRITE(ICU_INT_STATUS, (reg | *(UINT32 *)param));
		break;

	case CMD_GET_INTR_RAW_STATUS:
		ret = REG_READ(ICU_INT_RAW_STATUS);
		break;

	case CMD_CLR_INTR_RAW_STATUS:
		BK_ASSERT(param);
		reg = REG_READ(ICU_INT_RAW_STATUS);
		REG_WRITE(ICU_INT_RAW_STATUS, (reg | *(UINT32 *)param));
		break;

	case CMD_SET_JTAG_MODE:
		BK_ASSERT(param);
		if (JTAG_ARM_MODE == (*(UINT32 *)param))
			REG_WRITE(ICU_JTAG_SELECT, JTAG_SEL_WR_ARM);
		else if (JTAG_TL410_MODE == (*(UINT32 *)param))
			REG_WRITE(ICU_JTAG_SELECT, JTAG_SEL_WR_TL4);

#if (CONFIG_SOC_BK7271)
		else if (JTAG_BT_MODE == (*(UINT32 *)param))
			REG_WRITE(ICU_JTAG_SELECT, JTAG_SEL_WR_BT);
#endif
		break;

	case CMD_GET_JTAG_MODE:
		ret = REG_READ(ICU_JTAG_SELECT);
		break;

	case CMD_ARM_WAKEUP_DISABLE:
		reg = REG_READ(ICU_ARM_WAKEUP_EN);
		reg &= ~(*(UINT32 *)param);
		REG_WRITE(ICU_ARM_WAKEUP_EN, reg);
		break;

	case CMD_ARM_WAKEUP_ENABLE:
		reg = REG_READ(ICU_ARM_WAKEUP_EN);
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_ARM_WAKEUP_EN, reg);
		break;

	case CMD_GET_ARM_WAKEUP:
		ret = REG_READ(ICU_ARM_WAKEUP_EN);
		break;

	case CMD_QSPI_CLK_SEL:
		reg = REG_READ(ICU_PERI_CLK_MUX);
		reg &= (~(3 << 16));
		reg |= (*(UINT32 *)param);
		REG_WRITE(ICU_PERI_CLK_MUX, reg);
		break;

	default:
		break;
	}

	GLOBAL_INT_RESTORE();

	return ret;
}
#endif
// EOF
