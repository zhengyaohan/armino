if (CONFIG_MEDIA)
set(incs include)

if (NOT CONFIG_SLAVE_CORE)
	set(srcs
		core/media_major.c
		cli/media_cli.c
		cli/media_cli_dvp.c
		comm/comm_act.c
	)

if (CONFIG_CAMERA)
	list(APPEND srcs
		dvp/dvp_act.c
		dvp/dvp_api.c
	)
endif()

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		transfer/trs_act.c
		transfer/trs_api.c
		transfer/trs_task.c
	)
endif()

if (CONFIG_LCD)
	list(APPEND srcs
		lcd/lcd_act.c
		lcd/lcd_api.c
	)
endif()

if (CONFIG_AUDIO)
	list(APPEND srcs
		aud/aud_api.c
		aud/aud_act.c
		aud/adc_ccb.c
		aud/dac_ccb.c
	)
endif()

else()
	set(srcs
		core/media_minor.c
		cli/media_cli.c
		cli/media_cli_dvp.c
		comm/comm_act.c
	)


if (CONFIG_CAMERA)
	list(APPEND srcs
		dvp/dvp_act.c
		dvp/dvp_api.c
	)
endif()

if (CONFIG_WIFI_TRANSFER)
	list(APPEND srcs
		transfer/trs_act.c
		transfer/trs_api.c
		transfer/trs_task.c
	)
endif()

if (CONFIG_AUDIO)
	list(APPEND srcs
		aud/aud_api.c
		aud/aud_act.c
		aud/adc_ccb.c
		aud/dac_ccb.c
	)
endif()

if (CONFIG_LCD)
	list(APPEND srcs
		lcd/lcd_act.c
		lcd/lcd_api.c
	)
endif()

endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_cli)