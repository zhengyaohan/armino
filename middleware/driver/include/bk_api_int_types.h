// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once
#include "bk_include.h"
#include "bk_err.h"
#include "icu_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ICU defines
 * @defgroup bk_api_icu_defs macos
 * @ingroup bk_api_icu
 * @{
 */
#define BK_ERR_INT_DEVICE_NONE                  (BK_ERR_INT_BASE - 1) /**< icu device number is invalid */
#define BK_ERR_INT_NOT_EXIST                    (BK_ERR_INT_BASE - 2) /**< icu device number is invalid */

/**
 * @}
 */

/**
 * @brief ICU int devicde enum defines
 * @defgroup bk_api_icu_enum ICU  enums
 * @ingroup bk_api_icu
 * @{
 */

#if CONFIG_SOC_BK7256 || CONFIG_SOC_BK7256_CP1
typedef enum {
	INT_SRC_UART1 = 0x00,
	INT_SRC_UART2,
	INT_SRC_I2C0,
	INT_SRC_IRDA,
	INT_SRC_I2S,
	INT_SRC_I2C1,
	INT_SRC_SPI,
	INT_SRC_GPIO,
	INT_SRC_TIMER,
	INT_SRC_PWM,
	INT_SRC_AUDIO,
	INT_SRC_SARADC,
	INT_SRC_SDIO,
	INT_SRC_USB,
	INT_SRC_FFT,
	INT_SRC_GDMA,
	INT_SRC_MODEM,
	INT_SRC_MAC_TXRX_TIMER,
	INT_SRC_MAC_TXRX_MISC,
	INT_SRC_MAC_RX_TRIGGER,
	INT_SRC_MAC_TX_TRIGGER,
	INT_SRC_MAC_PROT_TRIGGER,
	INT_SRC_MAC_GENERAL,
	INT_SRC_SDIO_DMA,
	INT_SRC_USB_PLUG_INOUT,
	INT_SRC_SECURITY,
	INT_SRC_MAC_WAKEUP,
	INT_SRC_HSSPI_SLAVE,
	INT_SRC_PLL_UNLOCK,
	INT_SRC_JPEG_DEC,
	INT_SRC_BLE,
	INT_SRC_PSRAM,
	INT_SRC_LA,
	INT_SRC_BTDM,
	INT_SRC_BT,
	INT_SRC_UART3,
	INT_SRC_I2C2,
	INT_SRC_SPI2,
	INT_SRC_SPI3,
	INT_SRC_PWM2,
	INT_SRC_USB2,
	INT_SRC_MAILBOX0,
	INT_SRC_MAILBOX1,
	INT_SRC_BT_WDT,
	INT_SRC_DSP_WDT,
	INT_SRC_RTC,
	INT_SRC_TOUCH,
	INT_SRC_CEC,
	INT_SRC_MODEM_RC,
	INT_SRC_MAC_HSU,
	INT_SRC_TIMER1,
	INT_SRC_MAC_INTN_PHY,
	INT_SRC_MAC_INT_GEN,
	INT_SRC_MAC_INT_RESERVED0,	//WIFI IP reserved it, not used in really product
	INT_SRC_JPEG,
	INT_SRC_JPEG_ENC = INT_SRC_JPEG,	//add it for distinguish with INT_SRC_JPEG_ENC
	INT_SRC_EIP130_SEC,
	INT_SRC_EIP130,
	INT_SRC_LCD,
	INT_SRC_QSPI,
	INT_SRC_CAN,
	INT_SRC_SBC,
	INT_SRC_BMC32,	//Andes RISC-V
	INT_SRC_BMC64,	//Andes RISC-V
	INT_SRC_NONE,
} icu_int_src_t;
#else
typedef enum {
	INT_SRC_UART1 = 0x00,
	INT_SRC_UART2,
	INT_SRC_I2C0,
	INT_SRC_IRDA,
	INT_SRC_I2S,
	INT_SRC_I2C1,
	INT_SRC_SPI,
	INT_SRC_GPIO,
	INT_SRC_TIMER,
	INT_SRC_PWM,
	INT_SRC_AUDIO,
	INT_SRC_SARADC,
	INT_SRC_SDIO,
	INT_SRC_USB,
	INT_SRC_FFT,
	INT_SRC_GDMA,
	INT_SRC_MODEM,
	INT_SRC_MAC_TXRX_TIMER,
	INT_SRC_MAC_TXRX_MISC,
	INT_SRC_MAC_RX_TRIGGER,
	INT_SRC_MAC_TX_TRIGGER,
	INT_SRC_MAC_PROT_TRIGGER,
	INT_SRC_MAC_GENERAL,
	INT_SRC_SDIO_DMA,
	INT_SRC_USB_PLUG_INOUT,
	INT_SRC_SECURITY,
	INT_SRC_MAC_WAKEUP,
	INT_SRC_HSSPI_SLAVE,
	INT_SRC_PLL_UNLOCK,
	INT_SRC_JPEG,
	INT_SRC_BLE,
	INT_SRC_PSRAM,
	INT_SRC_LA,
	INT_SRC_BTDM,
	INT_SRC_BT,
	INT_SRC_UART3,
	INT_SRC_I2C2,
	INT_SRC_SPI2,
	INT_SRC_SPI3,
	INT_SRC_PWM2,
	INT_SRC_USB2,
	INT_SRC_MAILBOX0,
	INT_SRC_MAILBOX1,
	INT_SRC_BT_WDT,
	INT_SRC_DSP_WDT,
	INT_SRC_RTC,
	INT_SRC_TOUCH,
	INT_SRC_CEC,
	INT_SRC_MODEM_RC,
	INT_SRC_MAC_HSU,
	INT_SRC_NONE,
} icu_int_src_t;
#endif

/**
 * @}
 */



/**
 * @brief ICU struct defines
 * @defgroup bk_api_icu_structs structs in ICU
 * @ingroup bk_api_icu
 * @{
 */

typedef struct {
	uint32_t irq_int_statis[IRQ_STATIS_COUNT];
	uint32_t fiq_int_statis[FIQ_STATIS_COUNT];
} int_statis_t;


/**
 * @}
 */

#ifdef __cplusplus
}
#endif
