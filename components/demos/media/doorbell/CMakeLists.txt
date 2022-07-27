
if (CONFIG_DOORBELL)
set(incs include)
set(srcs
	"src/doorbell.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES video media audio_transfer)
