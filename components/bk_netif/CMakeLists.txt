set(incs include include/bk_private)

if(NOT CONFIG_SOC_BK7256_CP1)
	set(srcs bk_netif.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common)
