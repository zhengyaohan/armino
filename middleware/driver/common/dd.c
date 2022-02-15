#include "include.h"
#include "bk_arm_arch.h"

#include "dd_pub.h"

#include "bk_sdio.h"
#include "bk_uart.h"
#include "bk_gpio.h"
#include "bk_icu.h"
#include "bk_usb.h"
#include "bk_pwm.h"
#include "bk_flash.h"
#include "spi_pub.h"
#include "bk_fft.h"
#include "bk_i2s.h"
#include "bk_saradc.h"
#include "bk_irda.h"
#include "bk_mac_phy_bypass.h"
#include "bk_timer.h"
#if (CONFIG_SOC_BK7271)
#include "bk_dsp.h"
#include "bk_mailbox.h"
#include "bk_api_mailbox.h"
#include "spi_bk7271.h"
#if (CONFIG_BT)
#include "bk_bt.h"
#endif
#endif

#if CONFIG_CAMERA
#include "jpeg_encoder_pub.h"
#include "i2c_pub.h"
#endif

#if CONFIG_AUDIO
#if (CONFIG_SOC_BK7271) || (CONFIG_SOC_BK7251)
#include "audio_pub.h"
#endif
#endif

#if CONFIG_SDCARD_HOST
#include "sdcard_pub.h"
#endif

#if CONFIG_HSLAVE_SPI
#include "spidma_pub.h"
#endif

#if CONFIG_GENERAL_DMA
#include "general_dma_pub.h"
#endif

#if CONFIG_SDCARD_HOST
#include "sdcard_pub.h"
#endif

#include "bk_sys_ctrl.h"

#if ((!CONFIG_SOC_BK7231) && (CONFIG_BLE == 1))
#include "bk_ble.h"
#endif

#if CONFIG_QSPI
#include "bk_qspi.h"
#endif

#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236)|| (CONFIG_SOC_BK7256)
#include "bk_calendar.h"
#endif

typedef struct _dd_init_s_ {
	dd_device_type dev;

	void (*init)(void);
	void (*exit)(void);
} DD_INIT_S;

#if (!CONFIG_SOC_BK7256) && (!CONFIG_SOC_BK7256_CP1)
static const DD_INIT_S dd_init_tbl[] = {
	/* name*/              /* init function*/          /* exit function*/
#if  (!CONFIG_SOC_BK7256_CP1)
#if !CONFIG_SOC_BK7256
	{DD_DEV_TYPE_SCTRL,        sctrl_init,                 sctrl_exit},
	{DD_DEV_TYPE_ICU,          icu_init,                   icu_exit},
//	{DD_DEV_TYPE_WDT,          wdt_init,                   wdt_exit},
#endif

	{DD_DEV_TYPE_GPIO,         gpio_init,                  gpio_exit},
#endif

#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256)
	{DD_DEV_TYPE_CAL,          cal_init,                   cal_exit},
#endif

#if CONFIG_UART2
	{DD_DEV_TYPE_UART2,        uart2_init,                 uart2_exit},
#endif

#if (CONFIG_SOC_BK7271)
#if CONFIG_UART3
//	{DD_DEV_TYPE_UART3,        uart3_init,                 uart3_exit},
#endif
#endif

#if CONFIG_UART1
//	{DD_DEV_TYPE_UART1,        uart1_init,                 uart1_exit},
#endif

#if  (!CONFIG_SOC_BK7256_CP1)
	{DD_DEV_TYPE_FLASH,        flash_init,                 flash_exit},
#endif

#if CONFIG_GENERAL_DMA
	//{DD_DEV_TYPE_GDMA,         gdma_init,                  gdma_exit},
#endif

#if CONFIG_HSLAVE_SPI
	{DD_DEV_TYPE_SPIDMA,       spidma_init,                spidma_uninit},
#endif

#if CONFIG_QSPI
	{DD_DEV_TYPE_QSPI,         qspi_init,                  qspi_exit},
#endif

#if CONFIG_CAMERA
	{DD_DEV_TYPE_I2C1,         i2c1_init,                  i2c1_exit},
	{DD_DEV_TYPE_I2C2,         i2c2_init,                  i2c2_exit},
#endif

#if (CONFIG_SOC_BK7271)
	{DD_DEV_TYPE_DSP,          dsp_init,                   dsp_exit},
	{DD_DEV_TYPE_MAILBOX,      mailbox_driver_init,        mailbox_driver_deinit},
#if (CONFIG_BT)
	{DD_DEV_TYPE_BT,           bt_init,                    bt_exit},
#endif
#endif

#if CONFIG_AUDIO
#if (CONFIG_SOC_BK7271) || (CONFIG_SOC_BK7251)
	{DD_DEV_TYPE_AUD_DAC,      audio_init,                 audio_exit},
#endif
#endif

#if CONFIG_SDIO || CONFIG_SDIO_TRANS
	{DD_DEV_TYPE_SDIO,         sdio_init,                  sdio_exit},
#endif

#if CONFIG_USB
	{DD_DEV_TYPE_USB,          usb_init,                   usb_exit},
#endif

#if (CONFIG_SOC_BK7251)
#if CONFIG_USB
	{DD_DEV_TYPE_USB_PLUG,     usb_plug_inout_init,        usb_plug_inout_exit},
#endif
#endif

#if  (!CONFIG_SOC_BK7256_CP1)
#if CONFIG_NO_HOSTED
	{DD_DEV_TYPE_SPI,          spi_init,                   spi_exit},
#endif
#endif

#if (CONFIG_SOC_BK7271)
	{DD_DEV_TYPE_SPI2,         spi2_init,                  spi2_exit},
	{DD_DEV_TYPE_SPI3,         spi3_init,                  spi3_exit},
#endif

#if (!CONFIG_SOC_BK7271)
#if CONFIG_FFT
	{DD_DEV_TYPE_FFT,          fft_init,                   fft_exit},
#endif

#if CONFIG_I2S
	//{DD_DEV_TYPE_I2S,          i2s_init,                   i2s_exit},
#endif
#endif

	//{DD_DEV_TYPE_SARADC,       saradc_init,                saradc_exit},
#if (!CONFIG_SOC_BK7231) && (!CONFIG_SOC_BK7256_CP1)
	{DD_DEV_TYPE_IRDA,         irda_init,                  irda_exit},
//	{DD_DEV_TYPE_TRNG,         trng_init,                  trng_exit},
#endif

#if CONFIG_MAC_PHY_BYPASS
	{DD_DEV_TYPE_MPB,          mpb_init,                   mpb_exit},
#endif

#if CONFIG_SDCARD_HOST
	{DD_DEV_TYPE_SDCARD,       sdcard_init,                sdcard_exit},
#endif

#if ((!CONFIG_SOC_BK7231) && (CONFIG_BLE == 1))
	//{DD_DEV_TYPE_BLE,          ble_init,                   ble_exit}, //sean
#endif
	{DD_DEV_TYPE_NONE,         NULLPTR,                    NULLPTR}
};
#else
static const DD_INIT_S dd_init_tbl[] = {
    /* name*/              /* init function*/          /* exit function*/
#if  (!CONFIG_SOC_BK7256_CP1)
    {DD_DEV_TYPE_FLASH,        flash_init,                 flash_exit},
#endif
};

#endif

void g_dd_init(void)
{
	UINT32 i;
	UINT32 tbl_count;
	const DD_INIT_S *dd_element;

	tbl_count = sizeof(dd_init_tbl) / sizeof(DD_INIT_S);
	for (i = 0; i < tbl_count; i ++) {
		dd_element = &dd_init_tbl[i];
		if (dd_element->dev && dd_element->init)
			(dd_element->init)();
		else
			return;
	}
}

void g_dd_exit(void)
{
	UINT32 i;
	UINT32 tbl_count;
	const DD_INIT_S *dd_element;

	tbl_count = sizeof(dd_init_tbl) / sizeof(DD_INIT_S);
	for (i = 0; i < tbl_count; i ++) {
		dd_element = &dd_init_tbl[i];
		if (dd_element->dev && dd_element->exit)
			(dd_element->exit)();
		else
			return;
	}
}
// EOF

