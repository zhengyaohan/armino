set(incs include include/bk_private include/bk_private/legacy)
set(srcs)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	# REQUIRES freertos_v9 app bk_event bk_rtos bk_config driver soc lwip_intf_v2_0 lwip_intf_v2_1 hal soc bk_log bk_ps bk_init
	REQUIRES release bk_startup os_source app bk_event bk_rtos bk_config driver lwip_intf_v2_0 lwip_intf_v2_1 bk_log bk_system bk_ps bk_init
	)
