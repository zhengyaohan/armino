if (CONFIG_SIM_UART)
	set(srcs gpio_uart.c pwm_uart.c)
endif()
armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common bk_init)
