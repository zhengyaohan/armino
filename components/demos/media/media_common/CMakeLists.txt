if (CONFIG_DUAL_CORE)
set(incs .)

set(srcs
	media_common.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES audio_transfer)