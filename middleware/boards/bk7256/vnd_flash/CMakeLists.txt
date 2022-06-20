if (CONFIG_VND_CAL)

set(incs .
	)
set(srcs
	vnd_flash.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_libs)
