set(incs
	.
	)

if (NOT CONFIG_SOC_BK7256_CP1)
set(srcs
	utils_httpc.c
	utils_net.c
	utils_timer.c
	lite-log.c
	)
endif() #if (NOT CONFIG_SOC_BK7256_CP1)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
