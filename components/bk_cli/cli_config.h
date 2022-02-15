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

#pragma once

#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_WIFI_ENABLE)
#define CLI_CFG_WIFI        1
#else
#define CLI_CFG_WIFI        0
#endif //#if (CONFIG_WIFI_ENABLE)
#define CLI_CFG_BLE         1

#if (CONFIG_LWIP)
#define CLI_CFG_NETIF       1
#else
#define CLI_CFG_NETIF       0
#endif //#if (CONFIG_LWIP)

#define CLI_CFG_MISC        1
#define CLI_CFG_MEM         1

#if (CONFIG_WIFI_ENABLE)
#define CLI_CFG_PHY         1
#else
#define CLI_CFG_PHY         0
#endif //#if (CONFIG_WIFI_ENABLE)

#define CLI_CFG_PWR         1
#define CLI_CFG_TIMER       1
#define CLI_CFG_WDT         1
#if CONFIG_TRNG_SUPPORT
#define CLI_CFG_TRNG        1
#else
#define CLI_CFG_TRNG        0
#endif
#define CLI_CFG_GPIO        1
#define CLI_CFG_OS          1
#define CLI_CFG_OTA         1
#define CLI_CFG_FLASH       1
#define CLI_CFG_UART        1
#define CLI_CFG_ADC         1
#define CLI_CFG_SPI         1
#define CLI_CFG_MICO        1
#define CLI_CFG_REG         1
#define CLI_CFG_DMA         1
#define CLI_CFG_EXCEPTION   1

#if(CONFIG_ICU)
#define CLI_CFG_ICU         1
#else
#define CLI_CFG_ICU         0
#endif

#define CLI_CFG_I2C         1

#if CONFIG_QSPI
#define CLI_CFG_QSPI        1
#else
#define CLI_CFG_QSPI        0
#endif

#if CONFIG_AON_RTC
#define CLI_CFG_AON_RTC     1
#else
#define CLI_CFG_AON_RTC     0
#endif

#if CONFIG_JPEG
#define CLI_CFG_JPEG        1
#else
#define CLI_CFG_JPEG        0
#endif

//TODO default to 0
#define CLI_CFG_EVENT       1

#if CONFIG_PERI_TEST
#define CLI_CFG_PERI        1
#define CLI_CFG_PWM         1
#else
#define CLI_CFG_PERI        0
#define CLI_CFG_PWM         1
#endif

#if (CONFIG_SOC_BK7251)
#define CLI_CFG_SECURITY    1
#else
#define CLI_CFG_SECURITY    1
#endif

#if CONFIG_TEMP_DETECT
#define CLI_CFG_TEMP_DETECT 1
#else
#define CLI_CFG_TEMP_DETECT 0
#endif

#if CONFIG_SDCARD_HOST
#define CLI_CFG_SD          1
#else
#define CLI_CFG_SD          0
#endif

#if CONFIG_FATFS
#define CLI_FATFS          1
#else
#define CLI_FATFS          0
#endif

#if CONFIG_AIRKISS_TEST
#define CLI_CFG_AIRKISS     1
#else
#define CLI_CFG_AIRKISS     0
#endif

#if CONFIG_IPERF_TEST
#define CLI_CFG_IPERF       1
#else
#define CLI_CFG_IPERF       0
#endif

#if (CONFIG_SOC_BK7256 || CONFIG_SOC_BK7256_CP1)
#if (CONFIG_AUDIO)
#define CLI_CFG_AUD         1

#endif
#define CLI_CFG_FFT         0
#endif

#if (CONFIG_SOC_BK7256)
#define CLI_CFG_VAULT		1
#define CLI_CFG_LCD		1
#else
#define CLI_CFG_vault		0
#define CLI_CFG_LCD		0

#endif
#ifdef __cplusplus
}
#endif
