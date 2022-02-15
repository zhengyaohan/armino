set(incs .)

if (NOT CONFIG_SOC_BK7256_CP1)
	set(srcs saradc_intf.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES temp_detect
	PRIV_REQUIRES bk_common bk_init
	)
