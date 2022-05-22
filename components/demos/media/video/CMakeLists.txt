set(incs
	dvp
	spidma
	jpeg_dec
	uvc
	video_transfer
	image_save
	)
set(srcs)

if (CONFIG_CAMERA)
	list(APPEND srcs
		"dvp/dvp_camera_driver.c"
		)
endif()

if (CONFIG_SPIDMA)
	list(APPEND srcs
		"spidma/spidma_driver.c"
		)
endif()

if (CONFIG_JPEG_DECODE)
	list(APPEND srcs
		"jpeg_dec/tjpgd.c"
		"jpeg_dec/jdev.c"
		"uvc_lcd_display/uvc_lcd_display.c"
		)
endif()

if (CONFIG_USB_UVC)
	list(APPEND srcs
		"uvc/uvc_camera_driver.c"
		"image_save/uvc_image_save.c"
		)
endif()

if (CONFIG_CAMERA OR CONFIG_SPIDMA)
if (CONFIG_APP_DEMO_VIDEO_TRANSFER)
	list(APPEND srcs
		"video_transfer/video_transfer.c"
		)
if (CONFIG_SDCARD_HOST)
	list(APPEND srcs
		"image_save/dvp_image_save.c"
		)
endif()
endif()
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES bk_common bk_log)