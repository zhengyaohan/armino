set(incs include include/bk_private)

if(CONFIG_BK_NETIF)
	set(srcs bk_netif.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common)
