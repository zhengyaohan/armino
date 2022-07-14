
if (CONFIG_LCD_TEST)
set(incs . ../common/.)
set(srcs
	lcd_rgb_demo.c
	lcd_rgb_sdcard_test.c
	"../common/lcd_dma2d_config.c"
	"../common/lcd_sdcard_config.c"
	"../common/lcd_spi_config.c"
	"../lcd_dev_init/lcd_device_init.c"
	lcd_rgb_capture.c
	)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)