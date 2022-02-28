#include "include.h"
#include "bk_arm_arch.h"
#include "bk_gpio.h"
#include "gpio.h"
#include "bk_drv_model.h"
#include "bk_sys_ctrl.h"
#include "sys_driver.h"
#include "bk_uart.h"
#include "bk_api_int.h"
#include "bk_icu.h"

#if (!CONFIG_SOC_BK7271)

static void (*p_gpio_intr_handler[GPIONUM])(unsigned char);
static int gpio_ops_filter_flag = 0;
static const DD_OPERATIONS gpio_op = {
	NULL,
	NULL,
	NULL,
	NULL,
	gpio_ctrl
};

void gpio_isr(void)
{
	int i;
	unsigned long ulIntStatus;

#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	ulIntStatus = *(volatile UINT32 *)REG_GPIO_INTSTA;
	for (i = 0; i <= GPIO31; i++) {
		if (ulIntStatus & (0x01UL << i)) {
			if (p_gpio_intr_handler[i] != NULL)
				(void)p_gpio_intr_handler[i]((unsigned char)(i + GPIO0));
		}
	}

	*(volatile UINT32 *)REG_GPIO_INTSTA = ulIntStatus;

	ulIntStatus = *(volatile UINT32 *)REG_GPIO_INTSTA2;
	for (i = 0; i < (GPIONUM - GPIO32); i++) {
		if (ulIntStatus & (0x01UL << i)) {
			if (p_gpio_intr_handler[(i + GPIO32)] != NULL)
				(void)p_gpio_intr_handler[(i + GPIO32)]((unsigned char)(i + GPIO32));
		}
	}

	*(volatile UINT32 *)REG_GPIO_INTSTA2 = ulIntStatus;
#else
	ulIntStatus = *(volatile UINT32 *)REG_GPIO_INTSTA;
	for (i = 0; i < GPIONUM; i++) {
		if (ulIntStatus & (0x01UL << i)) {
			if (p_gpio_intr_handler[i] != NULL)
				(void)p_gpio_intr_handler[i]((unsigned char)i);
		}
	}

	*(volatile UINT32 *)REG_GPIO_INTSTA = ulIntStatus;
#endif
}

static UINT32 gpio_ops_filter(UINT32 index)
{
	UINT32 ret;

	ret = GPIO_FAILURE;

#ifdef JTAG_GPIO_FILTER
	if (gpio_ops_filter_flag)
		return ret;

	if ((GPIO20 == index)
		|| (GPIO21 == index)
		|| (GPIO22 == index)
		|| (GPIO23 == index)) {
		FATAL_PRT("[JTAG]gpio_filter_%d\r\n", index);
		ret = GPIO_SUCCESS;

		goto filter_exit;
	}
#endif

#ifdef JTAG_GPIO_FILTER
filter_exit:
#endif

	return ret;
}

void gpio_ops_disable_filter(void)
{
	gpio_ops_filter_flag = 1;
}

void gpio_config(UINT32 index, UINT32 mode)
{
	UINT32 val;
	UINT32 overstep = 0;
	volatile UINT32 *gpio_cfg_addr;

	if (GPIO_SUCCESS == gpio_ops_filter(index))
		goto cfg_exit;

	if (index >= GPIONUM) {
		WARN_PRT("gpio_id_cross_border\r\n");

		goto cfg_exit;
	}

#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U)|| (CONFIG_SOC_BK7236))
	if (index < GPIO32)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + index * 4);
	else if (index >= GPIO32)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_32_CONFIG + (index - 32) * 4);
#else
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + index * 4);
#endif

	switch (mode) {
	case GMODE_INPUT_PULLDOWN:
		val = 0x2C;
		break;

	case GMODE_OUTPUT:
		val = 0x00;
		break;

	case GMODE_SECOND_FUNC:
		val = 0x48;
		break;

	case GMODE_INPUT_PULLUP:
		val = 0x3C;
		break;

	case GMODE_INPUT:
		val = 0x0C;
		break;

	case GMODE_SECOND_FUNC_PULL_UP:
		val = 0x78;
		break;

	case GMODE_DEEP_PS:
	case GMODE_SET_HIGH_IMPENDANCE:
		val = 0x08;
		break;

	default:
		overstep = 1;
		WARN_PRT("gpio_mode_exception:%d\r\n", mode);
		break;
	}

	if (0 == overstep)
		REG_WRITE(gpio_cfg_addr, val);

cfg_exit:
	return;
}

#if ((CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7271))
void gpio_usb_second_function(void)
{
	gpio_config(GPIO_USB_DP_PIN, GMODE_SET_HIGH_IMPENDANCE);
	gpio_config(GPIO_USB_DN_PIN, GMODE_SET_HIGH_IMPENDANCE);
}
#endif

static void gpio_enable_second_function(UINT32 func_mode)
{
	UINT32 i, reg;
	UINT32 modul_select = GPIO_MODUL_NONE;
	UINT32 pmode = PERIAL_MODE_1;
	UINT32 pmask = 0;
	UINT32 end_index = 0;
	UINT32 start_index = 0;
	UINT32 config_mode = GMODE_SECOND_FUNC;

#if ((CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7256_CP1)  || \
     (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7271) || (CONFIG_SOC_BK7236)  || (CONFIG_SOC_BK7256XX) )
	UINT32 regist = 0, shift = 0;
#endif // (!CONFIG_SOC_BK7231)

	switch (func_mode) {
	case GFUNC_MODE_UART2:
#if (CONFIG_SOC_BK7271)
		start_index = 16;
		end_index = 17;
		modul_select = GPIO_UART2_GPIO16_GPIO17_MODULE;
		pmask = GPIO_UART2_MODULE_MASK;
		pmode = PERIAL_MODE_2;
#else
		start_index = 0;
		end_index = 1;
		pmode = PERIAL_MODE_1;
#endif
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		break;

	case GFUNC_MODE_I2C2:
#if (CONFIG_SOC_BK7271)
		start_index = 16;
		end_index = 17;
		pmode = PERIAL_MODE_1;
#else
		start_index = 0;
		end_index = 1;
		pmode = PERIAL_MODE_2;
#endif
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		break;

	case GFUNC_MODE_I2S:
#if (CONFIG_SOC_BK7271)
		start_index = 18;
		end_index = 21;
#else
		start_index = 2;
		end_index = 5;
#endif
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_ADC1:
#if (CONFIG_SOC_BK7231N)  || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		start_index = 26;
		end_index = 26;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SET_HIGH_IMPENDANCE;
#else
		start_index = 4;
		end_index = 4;
		pmode = PERIAL_MODE_2;
#endif
		break;

	case GFUNC_MODE_ADC2:
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		start_index = 24;
		end_index = 24;
		pmode = PERIAL_MODE_3;
		config_mode = GMODE_SET_HIGH_IMPENDANCE;
#else
		start_index = 5;
		end_index = 5;
		pmode = PERIAL_MODE_2;
#endif
		break;

	case GFUNC_MODE_CLK13M:
		start_index = 6;
		end_index = 6;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_PWM0:
#if (CONFIG_SOC_BK7271)
		start_index = 3;
		end_index = 3;
#else
		start_index = 6;
		end_index = 6;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_PWM1:
#if (CONFIG_SOC_BK7271)
		start_index = 4;
		end_index = 4;
#else
		start_index = 7;
		end_index = 7;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_PWM2:
#if (CONFIG_SOC_BK7271)
		start_index = 5;
		end_index = 5;
#else
		start_index = 8;
		end_index = 8;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_PWM3:
#if (CONFIG_SOC_BK7271)
		start_index = 6;
		end_index = 6;
#else
		start_index = 9;
		end_index = 9;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_BT_PRIORITY:
		start_index = 9;
		end_index = 9;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_WIFI_ACTIVE:
		start_index = 7;
		end_index = 7;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_BT_ACTIVE:
		start_index = 8;
		end_index = 8;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_UART1:
#if (CONFIG_SOC_BK7271)
		start_index = 0;
		end_index = 1;
#else
		start_index = 10;
		end_index = 13;
#endif
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		break;

	case GFUNC_MODE_SD_DMA:
#if (CONFIG_SOC_BK7271)
		start_index = 34;
		end_index = 39;
		pmode = PERIAL_MODE_4;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		modul_select = GPIO_SD_DMA_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#else
		start_index = 14;
		end_index = 19;
		pmode = PERIAL_MODE_1;
		modul_select = GPIO_SD_DMA_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#endif
		break;

	/* P34-39 SDIO */
	case GFUNC_MODE_SD1_DMA:
#if (!CONFIG_SOC_BK7271)
		start_index = 34;
		end_index = 39;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SD1_DMA_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#endif
		break;

	case GFUNC_MODE_SD_HOST:
#if (CONFIG_SOC_BK7271)
		start_index = 34;
		end_index = 39;
		pmode = PERIAL_MODE_4;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		modul_select = GPIO_SD_HOST_PERIAL4_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#else
		start_index = 14;
		end_index = 19;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		modul_select = GPIO_SD_HOST_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#endif
		break;

#if (!CONFIG_SOC_BK7271)
	case GFUNC_MODE_SPI_DMA:
		start_index = 14;
		end_index = 17;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SPI_DMA_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;
#endif

	case GFUNC_MODE_SPI:
#if (CONFIG_SOC_BK7271)
		start_index = 2;
		end_index = 5;
		pmode = PERIAL_MODE_1;
#else
		start_index = 14;
		end_index = 17;
		pmode = PERIAL_MODE_2;
#endif
		modul_select = GPIO_SPI_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;

	case GFUNC_MODE_SPI_GPIO_14:
		start_index = 14;
		end_index = 14;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SPI_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;

	case GFUNC_MODE_SPI_GPIO_16_17:
		start_index = 16;
		end_index = 17;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SPI_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;

	case GFUNC_MODE_PWM4:
#if (CONFIG_SOC_BK7231)
		start_index = 18;
		end_index = 18;
#elif (CONFIG_SOC_BK7271)
		start_index = 7;
		end_index = 7;
#else
		start_index = 24;
		end_index = 24;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_PWM5:
#if (CONFIG_SOC_BK7231)
		start_index = 19;
		end_index = 19;
#elif (CONFIG_SOC_BK7271)
		start_index = 8;
		end_index = 8;
#else
		start_index = 26;
		end_index = 26;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_I2C1:
#if (CONFIG_SOC_BK7271)
		start_index = 0;
		end_index = 1;
		pmode = PERIAL_MODE_2;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
#else
		start_index = 20;
		end_index = 21;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
#endif
		break;

	case GFUNC_MODE_JTAG:
#if (CONFIG_SOC_BK7271)
		start_index = 11;
		end_index = 14;
#else
		start_index = 20;
		end_index = 23;
#endif
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_CLK26M:
		start_index = 22;
		end_index = 22;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_ADC3:
		start_index = 23;
		end_index = 23;
		pmode = PERIAL_MODE_1;
		break;

#if ((CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7256_CP1) || \
	 (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7271)) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256XX)
	case GFUNC_MODE_DCMI:
		start_index = 27;
		end_index = 39;
		pmode = PERIAL_MODE_1;
		break;

	case GFUNC_MODE_ADC4:
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		start_index = 28;
		end_index = 28;
		pmode = PERIAL_MODE_2;
		config_mode = GMODE_SET_HIGH_IMPENDANCE;
#else
		start_index = 2;
		end_index = 2;
		pmode = PERIAL_MODE_2;
#endif
		break;

	case GFUNC_MODE_ADC5:
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		start_index = 22;
		end_index = 22;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SET_HIGH_IMPENDANCE;
#else
		start_index = 3;
		end_index = 3;
		pmode = PERIAL_MODE_2;
#endif
		break;

	case GFUNC_MODE_ADC6:
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		start_index = 21;
		end_index = 21;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SET_HIGH_IMPENDANCE;
#else
		start_index = 12;
		end_index = 12;
		pmode = PERIAL_MODE_2;
#endif
		break;

	case GFUNC_MODE_ADC7:
		start_index = 13;
		end_index = 13;
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_SD1_HOST:
#if (CONFIG_SOC_BK7271)
		start_index = 8;
		end_index = 10;
		pmode = PERIAL_MODE_1;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		modul_select = GPIO_SD_HOST_PERIAL1_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#else
		start_index = 34;
		end_index = 39;
		pmode = PERIAL_MODE_2;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		modul_select = GPIO_SD1_HOST_MODULE;
		pmask = GPIO_SD_MODULE_MASK;
#endif
		break;

#if (!CONFIG_SOC_BK7271)
	case GFUNC_MODE_SPI1:
		start_index = 30;
		end_index = 33;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SPI1_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;

	case GFUNC_MODE_SPI_DMA1:
		start_index = 30;
		end_index = 33;
		pmode = PERIAL_MODE_2;
		modul_select = GPIO_SPI1_DMA_MODULE;
		pmask = GPIO_SPI_MODULE_MASK;
		break;

	case GFUNC_MODE_QSPI_1LINE:
		start_index = 16;
		end_index = 17;
		pmode = PERIAL_MODE_3;
		config_mode = GMODE_SECOND_FUNC_PULL_UP;
		break;

	case GFUNC_MODE_QSPI_4LINE:
		start_index = 18;
		end_index = 19;
		pmode = PERIAL_MODE_2;
		break;

	case GFUNC_MODE_QSPI_CLK:
		start_index = 24;
		end_index = 24;
		pmode = PERIAL_MODE_3;
		break;

	case GFUNC_MODE_QSPI_CSN:
		start_index = 26;
		end_index = 26;
		pmode = PERIAL_MODE_3;
		break;
	case GFUNC_MODE_IRDA:
		start_index = 26;
		end_index = 26;
		pmode = PERIAL_MODE_1;
		break;

#endif
#endif // (!CONFIG_SOC_BK7231)

	default:
		break;
	}

	for (i = start_index; i <= end_index; i++) {
#if ((CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7256_CP1) || \
			 (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7271)) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256XX)
		if ((func_mode == GFUNC_MODE_DCMI) && (i == GPIO28))
			continue;
#endif // (!CONFIG_SOC_BK7231)

#if ((CONFIG_SOC_BK7231U) || (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7271) || (CONFIG_SOC_BK7236))
		if (i < GPIO16) {
			regist = REG_GPIO_FUNC_CFG;
			shift = i * 2;
		} else if (i < GPIO32) {
			regist = REG_GPIO_FUNC_CFG_2;
			shift = (i - 16) * 2;
		} else if (i < GPIONUM) {
			regist = REG_GPIO_FUNC_CFG_3;
			shift = (i - 32) * 2;
		}
		reg = REG_READ(regist);

		reg = (reg & ~(0x3u << shift)) | ((pmode & 0x3u) << shift);
		REG_WRITE(regist, reg);
#elif (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
		if (i < GPIO16) {
			regist = REG_GPIO_FUNC_CFG;
			shift = i * 2;
		} else if (i < GPIONUM) {
			regist = REG_GPIO_FUNC_CFG_2;
			shift = (i - 16) * 2;
		}
		reg = REG_READ(regist);

		reg = (reg & ~(0x3u << shift)) | ((pmode & 0x3u) << shift);
		REG_WRITE(regist, reg);
#else
		reg = REG_READ(REG_GPIO_FUNC_CFG);
		if (PERIAL_MODE_1 == pmode)
			reg &= ~(BIT(i));
		else /*(PERIAL_MODE_2 == pmode)*/
			reg |= BIT(i);
		REG_WRITE(REG_GPIO_FUNC_CFG, reg);
#endif // (!CONFIG_SOC_BK7231)

		gpio_config(i, config_mode);
	}


	if (modul_select != GPIO_MODUL_NONE) {
		reg = REG_READ(REG_GPIO_MODULE_SELECT);
		reg &= ~(pmask);
		reg |= modul_select;
		REG_WRITE(REG_GPIO_MODULE_SELECT, reg);
	}
	return;
}

#if (CONFIG_SOC_BK7251)
void gpio27_enable_second_function(void)
{
    UINT32 reg;
    UINT32 pmode = PERIAL_MODE_1;
    UINT32 GPIO_NUM = 0;
    UINT32 regist = 0, shift = 0;

    GPIO_NUM = 27;
    pmode = PERIAL_MODE_1;

    gpio_config(GPIO_NUM, GMODE_SECOND_FUNC);
    regist = REG_GPIO_FUNC_CFG_2;
    shift = (GPIO_NUM - 16) * 2;

    reg = REG_READ(regist);
    reg = (reg & ~(0x3u << shift)) | ((pmode & 0x3u) << shift);
    REG_WRITE(regist, reg);
}
#endif

UINT32 gpio_input(UINT32 id)
{
	UINT32 val = 0;
	volatile UINT32 *gpio_cfg_addr;

	if (GPIO_SUCCESS == gpio_ops_filter(id)) {
		WARN_PRT("gpio_input_fail\r\n");
		goto input_exit;
	}

#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	if (id <= GPIO31)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	else if (id >= GPIO32)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_32_CONFIG + (id - 32) * 4);
#else
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
#endif

	val = REG_READ(gpio_cfg_addr);

input_exit:
	return (val & GCFG_INPUT_BIT);
}

void gpio_output(UINT32 id, UINT32 val)
{
	UINT32 reg_val;
	volatile UINT32 *gpio_cfg_addr;

	if (GPIO_SUCCESS == gpio_ops_filter(id)) {
		WARN_PRT("gpio_output_fail\r\n");
		goto output_exit;
	}

#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	if (id <= GPIO31)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	else if (id >= GPIO32)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_32_CONFIG + (id - 32) * 4);
#else
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
#endif

	reg_val = REG_READ(gpio_cfg_addr);

	reg_val &= ~GCFG_OUTPUT_BIT;
	reg_val |= (val & 0x01) << GCFG_OUTPUT_POS;
	REG_WRITE(gpio_cfg_addr, reg_val);

output_exit:
	return;
}

static void gpio_output_reverse(UINT32 id)
{
	UINT32 reg_val;
	volatile UINT32 *gpio_cfg_addr;

	if (GPIO_SUCCESS == gpio_ops_filter(id)) {
		WARN_PRT("gpio_output_reverse_fail\r\n");
		goto reverse_exit;
	}

#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	if (id <= GPIO31)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
	else if (id >= GPIO32)
		gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_32_CONFIG + (id - 32) * 4);
#else
	gpio_cfg_addr = (volatile UINT32 *)(REG_GPIO_CFG_BASE_ADDR + id * 4);
#endif

	reg_val = REG_READ(gpio_cfg_addr);
	reg_val ^= GCFG_OUTPUT_BIT;
	REG_WRITE(gpio_cfg_addr, reg_val);

reverse_exit:
	return;
}


static void gpio_disable_jtag(void)
{
#ifndef JTAG_GPIO_FILTER
	int id;

	/*config GPIO_PCFG*/
	//#error "todo"

	for (id = GPIO20; id <= GPIO23; id ++) {
		//gpio_config(id, GMODE_OUTPUT);
	}
#endif
}

void gpio_test_isr(unsigned char ucChannel)
{
	gpio_output(4, 1);      // 161ms
}

void gpio_int_disable(UINT32 index)
{
#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	if (index <= GPIO31)
		*(volatile UINT32 *)REG_GPIO_INTEN &= ~(0x01U << index);
	else if (index >= GPIO32)
		*(volatile UINT32 *)REG_GPIO_INTEN2 &= ~(0x01U << (index - 32));
#else
	*(volatile UINT32 *)REG_GPIO_INTEN &= ~(0x01U << index);
#endif
}

void gpio_int_enable(UINT32 index, UINT32 mode, void (*p_Int_Handler)(unsigned char))
{
	UINT32 param;

	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);
	param = IRQ_GPIO_BIT;
	// sddev_control(DD_DEV_TYPE_ICU, CMD_ICU_INT_ENABLE, &param);
	(void)sys_drv_int_enable(param);

	if (index >= GPIONUM) {
		WARN_PRT("gpio_id_cross_border\r\n");

		return;
	}

	mode &= 0x03;
	if ((mode == 0) || (mode == 3))
		gpio_config(index, GMODE_INPUT_PULLUP);
	else
		gpio_config(index, GMODE_INPUT_PULLDOWN);

	if (index <= GPIO15)
		*(volatile UINT32 *)REG_GPIO_INTLV0 = (*(volatile UINT32 *)REG_GPIO_INTLV0 & (~(0x03 << (index << 1)))) | (mode << (index << 1));
#if ((!CONFIG_SOC_BK7231N)) && (!CONFIG_SOC_BK7236) && (!CONFIG_SOC_BK7256XX) && (!CONFIG_SOC_BK7256_CP1)
	else  if ((index >= GPIO16) && (index <= GPIO31))
#else
	else  if ((index >= GPIO16) && (index < GPIONUM))
#endif
	{
		*(volatile UINT32 *)REG_GPIO_INTLV1 = (*(volatile UINT32 *)REG_GPIO_INTLV1 & (~(0x03 << ((index - 16) << 1)))) | (mode << ((index - 16) << 1));
	}
#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	else  if ((index >= GPIO32) && (index <= GPIO39))
		*(volatile UINT32 *)REG_GPIO_INTLV3 = (*(volatile UINT32 *)REG_GPIO_INTLV3 & (~(0x03U << ((index - 32) << 1)))) | (mode << ((index - 32) << 1));
#endif

	p_gpio_intr_handler[index] = p_Int_Handler;
#if ((CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7231U))
	if (index <= GPIO31)
		*(volatile UINT32 *)REG_GPIO_INTEN |= (0x01U << index);
	else if (index >= GPIO32)
		*(volatile UINT32 *)REG_GPIO_INTEN2 |= (0x01U << (index - GPIO32));
#else
	*(volatile UINT32 *)REG_GPIO_INTEN |= (0x01 << index);
#endif
}

/*******************************************************************/
void gpio_init(void)
{
#if CFG_SYS_START_TIME
	UINT32 param;
#endif
	gpio_disable_jtag();

	sddev_register_dev(DD_DEV_TYPE_GPIO, (DD_OPERATIONS *)&gpio_op);
#if CFG_SYS_START_TIME
	param = 3 | (GMODE_OUTPUT << 8);
	gpio_ctrl(CMD_GPIO_CFG, &param);
	gpio_output(3, 0);					//141ms  delta time = 13ms
	param = 4 | (GMODE_OUTPUT << 8);
	gpio_ctrl(CMD_GPIO_CFG, &param);
	gpio_output(4, 0);
	param = 5 | (GMODE_OUTPUT << 8);
	gpio_ctrl(CMD_GPIO_CFG, &param);
	gpio_output(5, 0);

	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);
	param = IRQ_GPIO_BIT;
	// sddev_control(DD_DEV_TYPE_ICU, CMD_ICU_INT_ENABLE, &param);
	(void)sys_drv_int_enable(param);

	gpio_int_enable(2, 2, gpio_test_isr);
	gpio_output(3, 1);					// delta time = 3.1ms, delta time = 780us
#endif
}

void gpio_exit(void)
{
	sddev_unregister_dev(DD_DEV_TYPE_GPIO);
}

UINT32 gpio_ctrl(UINT32 cmd, void *param)
{
	UINT32 ret;
	ret = GPIO_SUCCESS;

	switch (cmd) {
	case CMD_GPIO_CFG: {
		UINT32 id;
		UINT32 mode;

		id = GPIO_CFG_PARAM_DEMUX_ID(*(UINT32 *)param);
		mode = GPIO_CFG_PARAM_DEMUX_MODE(*(UINT32 *)param);

		gpio_config(id, mode);

		break;
	}

	case CMD_GPIO_OUTPUT_REVERSE:
		BK_ASSERT(param);

		gpio_output_reverse(*(UINT32 *)param);
		break;

	case CMD_GPIO_OUTPUT: {
		UINT32 id;
		UINT32 val;

		id = GPIO_OUTPUT_DEMUX_ID(*(UINT32 *)param);
		val = GPIO_OUTPUT_DEMUX_VAL(*(UINT32 *)param);

		gpio_output(id, val);
		break;
	}

	case CMD_GPIO_INPUT: {
		UINT32 id;
		UINT32 val;

		BK_ASSERT(param);

		id = *(UINT32 *)param;
		val = gpio_input(id);

		ret = val;
		break;
	}

	case CMD_GPIO_ENABLE_SECOND: {
		UINT32 second_mode;

		BK_ASSERT(param);

		second_mode = *(UINT32 *)param;
		gpio_enable_second_function(second_mode);
		break;
	}

#if (CONFIG_SOC_BK7231)
	case CMD_GPIO_CLR_DPLL_UNLOOK_INT_BIT:
		REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, DPLL_UNLOCK_INT);
		break;
#endif

#if (CONFIG_SOC_BK7251)
	case CMD_GPIO_EN_USB_PLUG_IN_INT: {
		INT32 reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		BK_ASSERT(param);
		UINT32 enable = *(UINT32 *)param;
		if (enable) {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg |= USB_PLUG_IN_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		} else {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg &= ~USB_PLUG_IN_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		}
		break;
	}

	case CMD_GPIO_EN_USB_PLUG_OUT_INT: {
		INT32 reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		BK_ASSERT(param);
		UINT32 enable = *(UINT32 *)param;
		if (enable) {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg |= USB_PLUG_OUT_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		} else {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg &= ~USB_PLUG_OUT_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		}
		break;
	}
#endif // (CONFIG_SOC_BK7251)

#if ((!CONFIG_SOC_BK7231) && (!CONFIG_SOC_BK7271))
	case CMD_GPIO_CLR_DPLL_UNLOOK_INT_BIT: {
		UINT32 reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		reg &= ~GPIO_EXTRAL_INT_MASK;
		reg |= DPLL_UNLOCK_INT;
		REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		break;
	}

	case CMD_GPIO_EN_DPLL_UNLOOK_INT: {
		UINT32 reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		BK_ASSERT(param);
		UINT32 enable = *(UINT32 *)param;
		if (enable) {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg |= DPLL_UNLOCK_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		} else {
			reg &= ~GPIO_EXTRAL_INT_MASK;
			reg &= ~DPLL_UNLOCK_INT_EN;
			REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);
		}
		break;
	}
#endif // (CONFIG_SOC_BK7231)

	case CMD_GPIO_INT_ENABLE: {
		GPIO_INT_ST *ptr = param;
		gpio_int_enable(ptr->id, ptr->mode, ptr->phandler);
		break;
	}
	case CMD_GPIO_INT_DISABLE: {
		UINT32 id ;
		id = *(UINT32 *)param;
		gpio_int_disable(id);
		break;
	}
	default:
		break;
	}

	return ret;
}

#if(CONFIG_SOC_BK7251)
UINT32 usb_is_plug_in(void)
{
	UINT32 reg = REG_READ(REG_GPIO_DETECT);

	return (reg & IS_USB_PLUG_IN_BIT) ? 1 : 0;
}

void usb_plug_inout_isr(void)
{
	UINT32 reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);

	if (reg & USB_PLUG_IN_INT) {
		reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		reg &= ~GPIO_EXTRAL_INT_MASK;
		reg |= USB_PLUG_IN_INT;
		REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);

		if (usb_plug.handler)
			usb_plug.handler(usb_plug.usr_data, USB_PLUG_IN_EVENT);
	}

	reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
	if (reg & USB_PLUG_OUT_INT) {
		reg = REG_READ(REG_GPIO_EXTRAL_INT_CFG);
		reg &= ~GPIO_EXTRAL_INT_MASK;
		reg |= USB_PLUG_OUT_INT;
		REG_WRITE(REG_GPIO_EXTRAL_INT_CFG, reg);

		if (usb_plug.handler)
			usb_plug.handler(usb_plug.usr_data, USB_PLUG_OUT_EVENT);
	}
}
#endif
#endif
// EOF
