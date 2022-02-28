set(incs include include/bk_private)

if (CONFIG_CLI)
set(srcs
	cli_adc.c
	cli_airkiss.c
	cli_ble.c
	cli_event.c
	cli_flash.c
	cli_gpio.c
	cli_iperf.c
	cli_main.c
	cli_misc.c
	cli_mico.c
	cli_mem.c
	cli_os.c
	cli_ota.c
	cli_phy.c
	cli_pwr.c
	cli_pwm.c
	cli_reg.c
	cli_sd.c
	cli_security.c
	cli_netif.c
	cli_timer.c
	cli_touch.c
	cli_temp_detect.c
	cli_uart.c
	cli_wifi.c
	cli_dma.c
	cli_wdt.c

	cli_spi.c
	cli_exception.c
	cli_icu.c
	cli_i2c.c
	)

if (CONFIG_TRNG_SUPPORT)
	list(APPEND srcs
		cli_trng.c
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

if (CONFIG_SOC_BK7271)
	list(APPEND srcs "cli_dsp_bk7271.c")
endif()

if (CONFIG_SOC_BK7256XX)
	list(APPEND srcs "cli_vault.c")
	list(APPEND srcs "cli_usb.c")
	if (CONFIG_LCD)
	list(APPEND incs "include/bk_private/lcd"
		)
	list(APPEND srcs
		"cli_lcd.c"
	)
endif() #CONFIG_LCD
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

if (CONFIG_JPEG)
	list(APPEND srcs
		"cli_jpeg.c"
	)
endif()

if (CONFIG_AT_CMD)
	list(APPEND srcs "cli_at.c")
endif()

if (CONFIG_AUDIO_TEST)
	list(APPEND srcs "cli_aud.c")
endif() #if(CONFIG_AUDIO)

if (CONFIG_FFT_TEST)
	list(APPEND srcs "cli_fft.c")
endif()

if (CONFIG_I2S_TEST)
	list(APPEND srcs "cli_i2s.c")
endif()

if (CONFIG_TOUCH_TEST)
	list(APPEND srcs "cli_touch.c")
endif()

endif() #if(CONFIG_CLI)

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES utf8 temp_detect app driver user_driver bk_system security demos fatfs http user_driver
	#PRIV_REQUIRES airkiss bk_common bk_netif bk_ble bk_ps
	PRIV_REQUIRES bk_common bk_netif bk_ble bk_ps fatfs bk_init at bk_wifi
	)
