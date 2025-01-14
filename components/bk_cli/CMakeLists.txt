set(incs include include/bk_private)

list(APPEND incs
	../../properties/modules/aec/
	)

if (CONFIG_CLI)
set(srcs
	cli_adc.c
	cli_airkiss.c
	cli_ble.c
	cli_event.c
	cli_keyValue.c
	cli_gpio.c
	cli_iperf.c
	cli_main.c
	cli_misc.c
	cli_mico.c
	cli_mem.c
	cli_os.c
	cli_ota.c
	cli_rpc.c
	cli_phy.c
	cli_pwr.c
	cli_reg.c
	cli_netif.c
	cli_timer.c
	cli_temp_detect.c
	cli_touch.c
	cli_uart.c
	cli_wifi.c
	cli_wdt.c
	cli_security.c
	cli_spi.c
	cli_exception.c
	cli_icu.c
	)

if (CONFIG_FLASH)
	list(APPEND srcs
		cli_flash.c
	)
endif()

if (CONFIG_GENERAL_DMA)
	list(APPEND srcs
		cli_dma.c
	)
endif()

if (CONFIG_SDCARD_HOST)
	list(APPEND srcs
		cli_sd.c
	)
endif()

if (CONFIG_PWM)
	list(APPEND srcs
		cli_pwm.c
	)
endif()

if (CONFIG_I2C)
	list(APPEND srcs
		cli_i2c.c
	)
endif()

if (CONFIG_SUPPORT_MATTER)
	list(APPEND srcs
		cli_matter.c
	)
endif()

if (CONFIG_FLASH)
	list(APPEND srcs
		cli_flash.c
	)
endif()

if (CONFIG_FLASH_QUAD_ENABLE)
	list(APPEND srcs
		cli_flash_test.c
	)
endif()

if (CONFIG_TRNG_SUPPORT)
	list(APPEND srcs
		cli_trng.c
	)
endif()

if (CONFIG_I2S)
	list(APPEND srcs
		cli_i2s.c
		)
endif()

if (CONFIG_FATFS)
	list(APPEND srcs "cli_fatfs.c")
endif()

if (CONFIG_EFUSE)
	list(APPEND srcs
		cli_efuse.c
	)
endif()

if (CONFIG_LWIP)
	list(APPEND srcs
		cli_lwip.c
	)
endif()

if (CONFIG_SHELL_ASYNCLOG)
	list(APPEND srcs
			shell_task.c
			shell_uart.c
	)
	if (CONFIG_MASTER_CORE)
		list(APPEND srcs
			shell_mailbox_ipc.c
		)
	endif()
	if (CONFIG_SLAVE_CORE)
		list(APPEND srcs
			shell_mailbox_cp1.c
		)
	endif()
endif()

if (CONFIG_DUAL_CORE)
	list(APPEND srcs
		rpc_client_gpio.c
	)
endif()

if (CONFIG_SOC_BK7271)
	list(APPEND srcs "cli_dsp_bk7271.c")
endif()

if (CONFIG_SOC_BK7256XX)
	list(APPEND srcs "cli_usb.c")
endif()

if (CONFIG_SOC_BK7256XX OR CONFIG_USB_UVC)
if (CONFIG_DUAL_CORE)
	list(APPEND srcs "cli_uvc.c")
endif()
endif()

if (CONFIG_SECURITYIP)
	list(APPEND srcs "cli_securityip.c")
endif()

if (CONFIG_QSPI)
	list(APPEND srcs
		"cli_qspi.c"
	)
endif()

if (CONFIG_AON_RTC)
	list(APPEND srcs
		"cli_aon_rtc.c"
	)
endif()

if (CONFIG_JPEG_ENCODE)
	list(APPEND srcs
		"cli_jpegenc.c"
	)
endif()


if (CONFIG_AT_CMD)
	list(APPEND srcs "cli_at.c")
endif()

if (CONFIG_LCD_TEST)
	list(APPEND srcs
		"cli_lcd.c")
endif() #CONFIG_LCD_TEST
if (CONFIG_DMA2D_TEST)
	list(APPEND srcs
		"cli_dma2d.c"
	)
endif() #CONFIG_DMA2D_TEST


if (CONFIG_AUDIO_TEST AND CONFIG_AUDIO_RISCV_IP_V1_0)
	list(APPEND srcs "cli_aud.c")
endif() #if(CONFIG_AUDIO)

if (CONFIG_AUDIO_TEST)
if (CONFIG_AUDIO_RISCV_IP_V1_0 AND CONFIG_DUAL_CORE)
if (NOT CONFIG_SOC_BK7256_CP1)
	list(APPEND srcs cli_aud_cp0.c)
endif()
endif()
endif()

if (CONFIG_AUDIO_AEC_TEST)
	list(APPEND srcs "cli_aec.c")
endif()

if (CONFIG_AUDIO_G711_TEST)
	list(APPEND srcs "cli_g711.c")
endif()

if (CONFIG_FFT_TEST)
	list(APPEND srcs "cli_fft.c")
endif()

if (CONFIG_SBC_TEST)
	list(APPEND srcs "cli_sbc.c")
endif()

if (CONFIG_I2S_TEST)
	list(APPEND srcs "cli_i2s.c")
endif()

if (CONFIG_TOUCH_TEST)
	list(APPEND srcs "cli_touch.c")
endif()

if (CONFIG_CALENDAR)
	list(APPEND srcs "cli_calendar.c")
endif()

endif() #if(CONFIG_CLI)

if (CONFIG_JPEG_DECODE)
	list(APPEND srcs cli_jpegdec.c)
endif()

if (CONFIG_JPEG_DECODE)
	list(APPEND srcs cli_jpegdec.c)
endif()

if (CONFIG_CAMERA OR CONFIG_SPIDMA)
if (CONFIG_APP_DEMO_VIDEO_TRANSFER)
	list(APPEND srcs "cli_dvp.c")
endif()
endif()

if (CONFIG_DOORBELL AND CONFIG_DUAL_CORE)
	list(APPEND srcs cli_doorbell.c)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES utf8 temp_detect app driver user_driver bk_system iperf fatfs user_driver http
	#PRIV_REQUIRES airkiss bk_common bk_netif bk_ble bk_ps
	PRIV_REQUIRES bk_common bk_netif bk_ble bk_ps fatfs bk_init at bk_wifi compal
	)
