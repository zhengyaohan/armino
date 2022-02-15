if (NOT CONFIG_SOC_BK7256_CP1)
	set(srcs gpio_uart.c pwm_uart.c)
endif()
armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common bk_init)
