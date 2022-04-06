#include <stdio.h>
#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include "bk_uart.h"
#include "bk_drv_model.h"
#include "bk_sys_ctrl.h"
#include "sys_driver.h"
#include <os/mem.h>
#include "bk_icu.h"
#include "bk_gpio.h"
#include "gpio_uart.h"
#include <os/mem.h>
#include <driver/int.h>
#include "bk_icu.h"
#include "bk_gpio.h"

void gu_delay(uint32_t usec)
{
	volatile uint32_t loops;

	while (usec--) {
		loops = 40;
		while (loops--);
	}
}

#ifdef CONFIG_GPIO_SIMU_UART_TX
void gpio_uart_send_init(void)
{
	UINT32 param;

	param = GPIO_CFG_PARAM(SIMU_UART_GPIONUM, GMODE_OUTPUT);
	sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_CFG, &param);
}

void gpio_uart_send_byte(unsigned char *buff, unsigned int len)
{
	volatile unsigned char c, n, loops;
	UINT32 param;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	param = GPIO_CFG_PARAM(SIMU_UART_GPIONUM, GMODE_OUTPUT);
	sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_OUTPUT, &param);
	gu_delay(60);

	while (len--) {
		param = GPIO_CFG_PARAM(SIMU_UART_GPIONUM, GMODE_INPUT_PULLDOWN);
		sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_OUTPUT, &param);
		gu_delay(33);
		loops = 10;
		while (loops--);
		c = *buff++;
		n = 8;
		while (n--) {
			param = GPIO_CFG_PARAM(SIMU_UART_GPIONUM, (c & 0x01));
			sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_OUTPUT, &param);
			gu_delay(33);
			loops = 6;
			while (loops--);
			c >>= 1;
		}
		param = GPIO_CFG_PARAM(SIMU_UART_GPIONUM, GMODE_OUTPUT);
		sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_OUTPUT, &param);
		gu_delay(33);
		loops = 6;
		while (loops--);
	}
	GLOBAL_INT_RESTORE();
}

int guart_fputc(int ch, FILE *f)
{
	gpio_uart_send_byte((unsigned char *)&ch, 1);

	return ch;
}

#endif

#ifdef CONFIG_GPIO_SIMU_UART_TX
void GPIO_Simu_Isr(unsigned char ucChannel)
{
	UINT8 c = 0, n, loops;

	if (ucChannel == SIMU_UART_GPIO_RX) {
		gu_delay(21);
		loops = 5;
		while (loops--);
		for (n = 0; n < 8; n++) {
			c >>= 1;
			if (bk_gpio_input(SIMU_UART_GPIO_RX))
				c |= 0x80;

			gu_delay(33);
			loops = 20;
			while (loops--);
			loops++;
		}
	}
}

void gpio_uart_recv_init(void)
{
	UINT32 param;
	GPIO_INT_ST int_param;

	param = GPIO_CFG_PARAM(SIMU_UART_GPIO_RX, GMODE_INPUT_PULLUP);
	sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_CFG, &param);

	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);

	param = IRQ_GPIO_BIT;
	// sddev_control(DD_DEV_TYPE_ICU, CMD_ICU_INT_ENABLE, &param);
	(void)sys_drv_int_enable(param);

	int_param.id = SIMU_UART_GPIO_RX;
	int_param.mode = GMODE_INPUT_PULLDOWN;
	int_param.phandler = GPIO_Simu_Isr;
	sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_INT_ENABLE, &int_param);
}
#endif
// eof

