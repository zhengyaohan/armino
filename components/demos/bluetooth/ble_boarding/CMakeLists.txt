if (CONFIG_AT_CMD)
set(srcs
	ble_boarding_demo.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES utf8 temp_detect app driver user_driver bk_system security iperf fatfs http user_driver
	# PRIV_REQUIRES airkiss bk_common bk_netif bk_ble bk_ps
	PRIV_REQUIRES bk_common bk_netif bk_ble bk_ps fatfs bk_init at bk_wifi compal homekit video
	)