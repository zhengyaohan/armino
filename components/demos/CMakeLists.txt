#TODO optimize it
if (CONFIG_DEMOS)
set(srcs
	net/iperf/iperf.c
	)
endif()

armino_component_register(SRCS "${srcs}" PRIV_REQUIRES bk_common)
