armino_build_get_property(target BDK_SOC)

# TODO
# 0. Too big components and makefile, need optimize it
# 2. optimize it when do driver optimization

set(incs
	.
	include
	include/bk_private
	include/bk_private/legacy
	common
	pwm
	rc_beken
	flash
	rw_pub
	uart
	sys_ctrl
	gpio
	general_dma
	spidma
	icu
	i2c
	sdcard
	)

set(srcs
	driver.c
	sys_driver.c
	common/dd.c
	common/drv_model.c
	general_dma/general_dma.c
	#i2s/i2s.c
	icu/icu.c
	pwm_driver.c
	icu_driver.c
	interrupt_base.c
	timer_driver.c
	dma_driver.c
	wdt_driver.c
	spi_driver.c
	spi_statis.c
	pwm/mcu_ps_timer.c
	spidma/spidma.c
	sys_ctrl/sys_ctrl.c
	aon_pmu_driver.c
	)


	list(APPEND srcs
		flash/flash.c
		gpio/gpio.c
		irda/irda.c
		macphy_bypass/mac_phy_bypass.c
		gpio_driver_base.c
		uart_driver.c
		uart_statis.c
		efuse_driver.c
		rw_pub/rw_platf_pub.c
		saradc/saradc.c
		uart/Retarget.c
		uart/uart.c
		uart/printf.c
		security/security.c
		security/hal_aes.c
		security/hal_sha.c
		adc_driver.c
		adc_statis.c
		i2c_driver.c
		i2c_statis.c
	)

if ((NOT CONFIG_SOC_BK7256) AND (NOT CONFIG_SOC_BK7256_CP1))
	list(APPEND srcs
		i2s/i2s.c
		)
endif()

if ((NOT CONFIG_SOC_BK7256) AND (NOT CONFIG_SOC_BK7256_CP1))
	list(APPEND srcs
		fft/fft.c
		)
endif()

if (CONFIG_SOC_BK7256)
	list(APPEND srcs
		fft_driver.c
		i2s_driver.c
		)
	if (CONFIG_LCD)
	list(APPEND srcs
		"lcd_disp_driver.c"
		)
endif() #if (CONFIG_LCD)
endif()

if (CONFIG_TRNG_SUPPORT)
	list(APPEND srcs
		trng_driver.c
		)
endif() #if (CONFIG_TRNG_SUPPORT)


if (CONFIG_QSPI)
	list(APPEND srcs
		"qspi_driver.c"
		"qspi_statis.c"
		"qspi_psram.c"
		)
endif() #if (CONFIG_QSPI)

if (CONFIG_JPEG)
	list(APPEND srcs
		"jpeg_driver.c"
		"jpeg_statis.c"
		)
endif() #if (CONFIG_JPEG)

if (CONFIG_PSRAM)
	list(APPEND srcs
		"psram_driver.c"
		)
endif() #if (CONFIG_PSRAM)

if (CONFIG_AON_RTC)
	list(APPEND srcs
		"aon_rtc_driver.c"
		)
endif() #if (CONFIG_AON_RTC)

if (CONFIG_SDCARD_HOST)
	list(APPEND srcs
		"sdcard/sdio_driver.c"
		"sdcard/sdcard.c"
		"sdcard/sdcard_test.c"
		)
endif() #if (CONFIG_SDCARD_HOST)

if (CONFIG_JPEG)
	list(APPEND srcs
		"jpeg_driver.c"
		"jpeg_statis.c"
		)
endif() #if (CONFIG_JPEG)

if (CONFIG_SDIO)
	list(APPEND srcs
		"sdio.c"
		"sdma.c"
		"sutil.c"
		)
endif() #if (CONFIG_SDIO)

if (CONFIG_AUDIO)
if (CONFIG_SOC_BK7251 OR CONFIG_SOC_BK7271)
	list(APPEND incs
		"audio"
		)

	list(APPEND srcs
		"audio/audio.c"
		"audio/ring_buffer.c"
		)
endif()

if (CONFIG_SOC_BK7256 OR CONFIG_SOC_BK7256_CP1)
	list(APPEND srcs
		"aud_driver.c"
		)
endif()

if (CONFIG_SOC_BK7251)
	list(APPEND srcs
		"audio/audio_adc.c"
		"audio/audio_dac.c"
		"audio/ring_buffer_dma_read.c"
		"audio/ring_buffer_dma_write.c"
		)
endif()
endif() #if (CONFIG_AUDIO)

if (CONFIG_TOUCH)
if (CONFIG_SOC_BK7256 OR CONFIG_SOC_BK7256_CP1)
	list(APPEND srcs
		"touch_driver.c"
		)
endif()
endif()

if (NOT CONFIG_FULLY_HOSTED)

	list(APPEND incs
		"spi"
		"jpeg"
		"sdcard"
		)

	list(APPEND srcs
		"dsp/dsp.c"
		"calendar/calendar.c"
		"i2c/i2c1.c"
		"i2c/i2c2.c"
		)

if (CONFIG_SOC_BK7231U)
	list(APPEND srcs
		"spi/spi.c"
		"spi/spi_slave.c"
		"spi/spi_master.c"
		)
endif()

if (CONFIG_SOC_BK7231N)
	list(APPEND srcs
		"spi/spi_bk7231n.c"
		"spi/spi_slave_bk7231n.c"
		"spi/spi_master_bk7231n.c"
		)
endif()

if (CONFIG_SOC_BK7256 OR CONFIG_SOC_BK7256_CP1)
	list(APPEND srcs
		"spi/spi_bk7231n.c"
		"spi/spi_slave_bk7231n.c"
		"spi/spi_master_bk7231n.c"
		"mailbox_driver_base.c"
		"bk7256/mailbox_driver.c"
		)
	if (CONFIG_DUAL_CORE)
		list(APPEND srcs
			mailbox_channel.c
		)
	endif()

endif()

if (CONFIG_SOC_BK7251)
	list(APPEND incs
		"spi"
		)

	list(APPEND srcs
		"spi/spi.c"
		"spi/spi_slave.c"
		"spi/spi_master.c"
		"qspi/qspi.c"
		)
endif()

if (CONFIG_SOC_BK7271)
	list(APPEND incs
		)

	list(APPEND srcs
		"gpio/gpio_bk7271.c"
		"spi/spi_bk7271.c"
		"spi/spi_slave_bk7271.c"
		"spi/spi_master_bk7271.c"
		"i2c/i2c1_bk7271.c"
		"mailbox_driver_base.c"
		"bk7271/mailbox_driver.c"
		"pwm/timer_bk7271.c"
		"qspi/qspi_bk7271.c"
		)
endif()

if (CONFIG_SOC_BK7236)
	list(APPEND srcs
		"spi/spi.c"
		"spi/spi_slave.c"
		"spi/spi_master.c"
		)
endif()
endif() #if (NOT CONFIG_FULLY_HOSTED)




if ("${target}" STREQUAL "bk7256" OR "${target}" STREQUAL "bk7256_cp1")
	list(APPEND incs
		"eip130/Kit/Log/incl"
		"eip130/Kit/Log/src/bk_printf"
		"eip130/Kit/Packages/DriverFramework_v4/DMAResource_API/incl"
		"eip130/Kit/Packages/DriverFramework_v4/Basic_Defs_API/incl"
		"eip130/Kit/Packages/DriverFramework_v4/CLib_Abstraction_API/incl"
		"eip130/Kit/Packages/DriverFramework_v4/Device_API/incl"
		"eip130/Kit/Packages/EIP130/TokenHelper/incl"
		"eip130/Kit/Packages/EIP130/TokenHelper/src"
		"eip130/Kit/Packages/EIP130/MailboxControl/incl"
		"eip130/Kit/Packages/EIP130/MailboxControl/src"
		"eip130/Integration/Adapter_GlobalControl/incl"
		"eip130/Integration/Adapter_GlobalControl/src"
		"eip130/Integration/DriverFramework/src"
		"eip130/Integration/DriverFramework/src/BK_Arm"
		"eip130/Integration/Adapter_Generic/src"
		"eip130/Integration/Adapter_Generic/src/BK_Arm"
		"eip130/Integration/Adapter_DriverInit/incl"
		"eip130/Integration/Adapter_DriverInit/src/BK72XX"
		"eip130/Integration/Adapter_DriverInit/src"
		"eip130/Integration/Adapter_VAL/incl"
		"eip130/Integration/Adapter_VAL/src/BK_Arm"
		"eip130/Integration/Adapter_VAL/src"
		"eip130/Integration/Adapter_VEX/src"
		"eip130/Examples/Driver_BEKEN/build"
		"eip130/Examples/Driver_BEKEN/build/BK72XX"
		"eip130/header"
		)

	list(APPEND srcs
		"vault_driver.c"
		"eip130/Kit/Log/src/log.c"
		"eip130/Kit/Packages/EIP130/TokenHelper/src/eip130_token_common.c"
		"eip130/Kit/Packages/EIP130/MailboxControl/src/eip130.c"
		"eip130/Integration/Adapter_GlobalControl/src/adapter_global_eip130.c"
		"eip130/Integration/DriverFramework/src/dmares_gen.c"
		"eip130/Integration/DriverFramework/src/BK_Arm/hwpal_dmares_bk.c"
		"eip130/Integration/DriverFramework/src/BK_Arm/hwpal_device_bk.c"
		"eip130/Integration/DriverFramework/src/device_generic.c"
		"eip130/Integration/Adapter_Generic/src/adapter_lock_internal.c"
		"eip130/Integration/Adapter_Generic/src/adapter_bufmanager.c"
		"eip130/Integration/Adapter_Generic/src/BK_Arm/adapter_firmware.c"
		"eip130/Integration/Adapter_Generic/src/BK_Arm/adapter_getpid.c"
		"eip130/Integration/Adapter_Generic/src/BK_Arm/adapter_lock.c"
		"eip130/Integration/Adapter_Generic/src/BK_Arm/adapter_sleep.c"
		"eip130/Integration/Adapter_DriverInit/src/BK72XX/adapter_global_control_init.c"
		#"eip130/Integration/Adapter_DriverInit/src/BK72XX/adapter_init_val.c"
		"eip130/Integration/Adapter_DriverInit/src/BK72XX/adapter_init.c"
		#"eip130/Integration/Adapter_DriverInit/src/adapter_driver_val_init.c"
		#"eip130/Integration/Adapter_DriverInit/src/adapter_global_init.c"
		"eip130/Integration/Adapter_DriverInit/src/adapter_driver_init.c"
		#"eip130/Integration/Adapter_DriverInit/src/adapter_global_driver_init.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_random.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_rsa.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_exchangetoken.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_securetimer.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_eddsa.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sf_milenage.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_ecc_elgamal.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_ecdh.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_encryptvector.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asset.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sym_keywrap.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_hardware.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sym_hash.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_dh.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_system.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_ecdsa.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_dsa.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_otp.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sym_common.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_nop.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_claim.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_curve25519.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_publicdata.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_emmc.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sym_cipher.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_dhdsa.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_service.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_aunlock.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_ecc.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_monotonic.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_pka.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_sym_mac.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_asym_common.c"
		"eip130/Integration/Adapter_VAL/src/adapter_val_internal.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_device.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_trng.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_emmc.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_keywrap.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_init.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_encryptedvector.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_logicaltoken.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_hardware.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sf.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_lock.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_encrypt.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_nop.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_genkey.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sym_mac.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sym_keywrap.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_service.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_system.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_extservice.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_genshared.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_identity.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sym_cipher.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sym_hash.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_physicaltoken.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_bufmanager.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_claim.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_pka.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_signverify.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asym_asset_keycheck.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_mailbox.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_sf_milenage.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_asset.c"
		"eip130/Integration/Adapter_VEX/src/adapter_vex_aunlock.c"
		)
	if (CONFIG_VAULT_SELFTEST)
	list(APPEND incs
		"eip130/TestTools/TestTool_VAL/ValTests"
		"eip130/TestTools/TestTool_VAL/TestVectors/incl"
		"eip130/TestTools/TestTool_VAL/TestVectors/src"
		"eip130/TestTools/TestTool_VAL/incl"
		"eip130/TestTools/TestTool_VAL/build"
		"eip130/TestTools/TestTool_VAL/src"
		)

	list(APPEND srcs
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_ecc_common.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_eddsa.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asset.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_ecc_elgamal.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_sym_crypto.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_parser.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_ecdh.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_sym_auth_crypto.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_securetimer.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_internal.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_specialfunctions.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_dh.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_random.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_hash.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_curve25519.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_cipher_mac.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_system.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_nop.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_pk.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_sym_keywrap.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_fips.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_service.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_ecdsa.c"
		"eip130/TestTools/TestTool_VAL/ValTests/valtest_asym_rsa.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_wrap.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aria_basic.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_rsa.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_cmac.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_dhdsa_domain.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_dsa.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_chacha20_poly1305_aead.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_chacha20.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aria_gcm.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_sm4_basic.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_des.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_basic.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_ccm.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_f8.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_gcm.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aria_cbcmac.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aria_ccm.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_ecc_curves.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_hash.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_cmac_s2v.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_siv.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_milenage.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_xts_aes.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_eddsa.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aria_cmac.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_ecc_elgamal.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_ecdsa.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_aes_cbcmac.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_poly1305.c"
		"eip130/TestTools/TestTool_VAL/TestVectors/src/testvectors_hmac.c"
		"eip130/TestTools/TestTool_VAL/src/sfzutf.c"
		"eip130/TestTools/TestTool_VAL/src/test_val.c"
		)
endif()#if (CONFIG_VAULT_SELFTEST)
endif()



if( "${target}" STREQUAL "bk7231u")
	set(target "bk7251")
endif()

if( "${target}" STREQUAL "bk7236")
	set(target "bk7231n")
endif()

if( "${target}" STREQUAL "bk7231n" OR  "${target}" STREQUAL "bk7251" OR "${target}" STREQUAL "bk7271" OR "${target}" STREQUAL "bk7256" OR "${target}" STREQUAL "bk7256_cp1")
	list(APPEND incs
		"${target}"
	)

	list(APPEND srcs
		"${target}/gpio_driver.c"
		"${target}/interrupt.c"
		"${target}/interrupt_statis.c"
		)
endif()


armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES app arm9 bk_wifi bk_usb riscv
	PRIV_REQUIRES music_player bk_common bk_log bk_ble bk_ps bk_ate bk_init video_transfer
)
