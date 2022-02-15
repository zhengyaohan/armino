set(incs .)
set(srcs camera_intf.c)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common video_transfer)
