if (CONFIG_AUDIO_TEST AND CONFIG_AUDIO_RISCV_IP_V1_0 AND CONFIG_AUDIO_TRANSFER_CPU0)

set(incs . include)

set(srcs
	./src/audio_transfer_cp0.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common media_common)