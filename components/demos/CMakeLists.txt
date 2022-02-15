#TODO optimize it
if (NOT CONFIG_SOC_BK7256_CP1)
set(srcs
	net/iperf/iperf.c
	)
endif()

armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common)
