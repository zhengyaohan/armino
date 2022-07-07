
if (CONFIG_DOORBELL AND CONFIG_DUAL_CORE)
set(incs include)
set(srcs
	"src/doorbell.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES video audio_transfer_apk audio_transfer_sd audio_transfer_cpu0_mode)
