if (CONFIG_MEDIA)
set(incs include)

if (NOT CONFIG_SLAVE_CORE)
	set(srcs
		core/media_major.c
		dvp/dvp_act.c
	)
else()
	set(srcs
		core/media_minor.c
		dvp/dvp_act.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")