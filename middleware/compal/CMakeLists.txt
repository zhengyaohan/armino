set(incs homekit matter)
set(srcs 
	homekit/key_value.c
	homekit/func_convert.c
	matter/flash_namespace_value.c
	)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)
