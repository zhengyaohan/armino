set(incs)
set(srcs)

if (CONFIG_DOORBELL)
	list(APPEND incs 
		include
	)

	list(APPEND srcs
		"src/doorbell.c"
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES video media audio_transfer bk_common)
