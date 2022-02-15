set(srcs usb_plug.c)
armino_component_register(SRCS "${srcs}"
	PRIV_REQUIRES bk_common bk_ps)
