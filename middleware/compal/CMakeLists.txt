

set(incs homekit matter)
set(srcs 
	homekit/key_value.c
	homekit/func_convert.c
	)

if(CONFIG_SUPPORT_MATTER)
	list(APPEND srcs
                                matter/flash_namespace_value.c
	)
endif()

if(CONFIG_COMMON_IO)
	list(APPEND incs
				common_io/include
				common_io/test
	)
	list(APPEND srcs
				common_io/iot_uart.c
				common_io/iot_adc.c
				common_io/iot_spi.c
				common_io/iot_gpio.c
				common_io/iot_flash.c
	)

if(CONFIG_PWM)
	list(APPEND srcs	common_io/iot_pwm.c)
endif()

	list(APPEND srcs
				common_io/iot_test_common_io_internal.c
				common_io/test/test_iot_adc.c
				common_io/test/test_iot_gpio.c
				common_io/test/test_iot_i2c.c
				common_io/test/test_iot_pwm.c
				common_io/test/test_iot_uart.c
				common_io/test/test_iot_spi.c
				common_io/test/test_iot_flash.c
				common_io/test/all_tests.c
	)
	list(APPEND incs
				unity/extras/fixture/src
				unity/src
	)
	list(APPEND srcs
				unity/extras/fixture/src/unity_fixture.c
				unity/src/unity.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
