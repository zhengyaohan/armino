
#TODO should move all public include dir to modules
set(incs .
	)
set(srcs
	video_transfer
	)

#TODO fix main
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES spidma_intf camera_intf
	PRIV_REQUIRES
	)
