#ifndef _GPIO_BK7271_H_
#define _GPIO_BK7271_H_

#include "sys_config.h"
#if (CONFIG_SOC_BK7271)
#include "icu.h"

#if CONFIG_JTAG
#define JTAG_GPIO_FILTER
#endif

#ifndef GPIO_PRT
#define GPIO_PRT                        os_printf
#endif

#ifndef WARN_PRT
#define WARN_PRT                        os_printf
#endif

#ifndef WARN_PRT
#define FATAL_PRT                       os_printf
#endif

#define GPIO_INIT_FLAG                   ((UINT32)1)
#define GPIO_UNINIT_FLAG                 ((UINT32)-1)

#define GPIO_BASE_ADDR                       (0x0800300)

#define REG_GPIO_0_CONFIG                    (GPIO_BASE_ADDR + 0*4)
#define REG_GPIO_1_CONFIG                    (GPIO_BASE_ADDR + 1*4)
#define REG_GPIO_2_CONFIG                    (GPIO_BASE_ADDR + 2*4)
#define REG_GPIO_3_CONFIG                    (GPIO_BASE_ADDR + 3*4)
#define REG_GPIO_4_CONFIG                    (GPIO_BASE_ADDR + 4*4)
#define REG_GPIO_5_CONFIG                    (GPIO_BASE_ADDR + 5*4)
#define REG_GPIO_6_CONFIG                    (GPIO_BASE_ADDR + 6*4)
#define REG_GPIO_7_CONFIG                    (GPIO_BASE_ADDR + 7*4)
#define REG_GPIO_8_CONFIG                    (GPIO_BASE_ADDR + 8*4)
#define REG_GPIO_9_CONFIG                    (GPIO_BASE_ADDR + 9*4)
#define REG_GPIO_10_CONFIG                   (GPIO_BASE_ADDR + 10*4)
#define REG_GPIO_11_CONFIG                   (GPIO_BASE_ADDR + 11*4)
#define REG_GPIO_12_CONFIG                   (GPIO_BASE_ADDR + 12*4)
#define REG_GPIO_13_CONFIG                   (GPIO_BASE_ADDR + 13*4)
#define REG_GPIO_14_CONFIG                   (GPIO_BASE_ADDR + 14*4)
#define REG_GPIO_15_CONFIG                   (GPIO_BASE_ADDR + 15*4)
#define REG_GPIO_16_CONFIG                   (GPIO_BASE_ADDR + 16*4)
#define REG_GPIO_17_CONFIG                   (GPIO_BASE_ADDR + 17*4)
#define REG_GPIO_18_CONFIG                   (GPIO_BASE_ADDR + 18*4)
#define REG_GPIO_19_CONFIG                   (GPIO_BASE_ADDR + 19*4)
#define REG_GPIO_20_CONFIG                   (GPIO_BASE_ADDR + 20*4)
#define REG_GPIO_21_CONFIG                   (GPIO_BASE_ADDR + 21*4)
#define REG_GPIO_22_CONFIG                   (GPIO_BASE_ADDR + 22*4)
#define REG_GPIO_23_CONFIG                   (GPIO_BASE_ADDR + 23*4)
#define REG_GPIO_24_CONFIG                   (GPIO_BASE_ADDR + 24*4)
#define REG_GPIO_25_CONFIG                   (GPIO_BASE_ADDR + 25*4)
#define REG_GPIO_26_CONFIG                   (GPIO_BASE_ADDR + 26*4)
#define REG_GPIO_27_CONFIG                   (GPIO_BASE_ADDR + 27*4)
#define REG_GPIO_28_CONFIG                   (GPIO_BASE_ADDR + 28*4)
#define REG_GPIO_29_CONFIG                   (GPIO_BASE_ADDR + 29*4)
#define REG_GPIO_30_CONFIG                   (GPIO_BASE_ADDR + 30*4)
#define REG_GPIO_31_CONFIG                   (GPIO_BASE_ADDR + 31*4)
#define REG_GPIO_32_CONFIG                   (GPIO_BASE_ADDR + 32*4)
#define REG_GPIO_33_CONFIG                   (GPIO_BASE_ADDR + 33*4)
#define REG_GPIO_34_CONFIG                   (GPIO_BASE_ADDR + 34*4)
#define REG_GPIO_35_CONFIG                   (GPIO_BASE_ADDR + 35*4)
#define REG_GPIO_36_CONFIG                   (GPIO_BASE_ADDR + 36*4)
#define REG_GPIO_37_CONFIG                   (GPIO_BASE_ADDR + 37*4)
#define REG_GPIO_38_CONFIG                   (GPIO_BASE_ADDR + 38*4)
#define REG_GPIO_39_CONFIG                   (GPIO_BASE_ADDR + 39*4)

#define REG_GPIO_CFG_BASE_ADDR               (REG_GPIO_0_CONFIG)

#define GCFG_INPUT_POS                       0
#define GCFG_OUTPUT_POS                      1
#define GCFG_INPUT_ENABLE_POS                2
#define GCFG_OUTPUT_ENABLE_POS               3
#define GCFG_PULL_MODE_POS                   4
#define GCFG_PULL_ENABLE_POS                 5
#define GCFG_FUNCTION_ENABLE_POS             6
#define GCFG_INPUT_MONITOR_POS               7


#define GCFG_INPUT_BIT                       (1 << 0)
#define GCFG_OUTPUT_BIT                      (1 << 1)
#define GCFG_INPUT_ENABLE_BIT                (1 << 2)
#define GCFG_OUTPUT_ENABLE_BIT               (1 << 3)
#define GCFG_PULL_MODE_BIT                   (1 << 4)
#define GCFG_PULL_ENABLE_BIT                 (1 << 5)
#define GCFG_FUNCTION_ENABLE_BIT             (1 << 6)
#define GCFG_INPUT_MONITOR_BIT               (1 << 7)

#define REG_GPIO_INTLV0                      (GPIO_BASE_ADDR + 0x30*4)
#define REG_GPIO_INTLV1                      (GPIO_BASE_ADDR + 0x31*4)
#define REG_GPIO_INTLV2                      (GPIO_BASE_ADDR + 0x32*4)
#define REG_GPIO_INTEN                       (GPIO_BASE_ADDR + 0x33*4)
#define REG_GPIO_INTEN1                      (GPIO_BASE_ADDR + 0x34*4)
#define REG_GPIO_INTSTA                      (GPIO_BASE_ADDR + 0x35*4)
#define REG_GPIO_INTSTA1                     (GPIO_BASE_ADDR + 0x36*4)


// for GPIO 0-15
#define REG_GPIO_FUNC_CFG                    (ICU_BASE + 0x20* 4)
#define PERIAL_MODE_1                         (0)
#define PERIAL_MODE_2                         (1)
#define PERIAL_MODE_3                         (2)
#define PERIAL_MODE_4                         (3)

// for GPIO 16-31
#define REG_GPIO_FUNC_CFG_2                  (ICU_BASE + 0x21 * 4)
#define GPIO_PCFG2_POSI(x)                   (((x)-16)*2)
#define GPIO_PCFG2_MASK(x)                   (0x03UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_1_FUNC(x)                 (0x00UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_2_FUNC(x)                 (0x01UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_3_FUNC(x)                 (0x02UL << GPIO_PCFG2_POSI(x))
#define GPIO_PCFG2_4_FUNC(x)                 (0x03UL << GPIO_PCFG2_POSI(x))


// for GPIO 32-39
#define REG_GPIO_FUNC_CFG_3                  (ICU_BASE + 0x22 * 4)
#define GPIO_PCFG3_POSI(x)                   (((x)-32)*2)
#define GPIO_PCFG3_MASK(x)                   (0x03UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_1_FUNC(x)                 (0x00UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_2_FUNC(x)                 (0x01UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_3_FUNC(x)                 (0x02UL << GPIO_PCFG3_POSI(x))
#define GPIO_PCFG3_4_FUNC(x)                 (0x03UL << GPIO_PCFG3_POSI(x))


#define REG_GPIO_MODULE_SELECT               (ICU_BASE + 0x23 * 4)
#define GPIO_MODUL_NONE                      0xff

#define GPIO_UART2_GPIO6_GPIO7_MODULE           (0 << 0)
#define GPIO_UART2_GPIO16_GPIO17_MODULE         (1 << 0)
#define GPIO_UART2_MODULE_MASK                  (1 << 0)

#define GPIO_SPI3_GPIO30_33_MODULE				(0 << 1)
#define GPIO_SPI3_GPIO34_39_MODULE				(1 << 1)
#define GPIO_SPI3_MODULE_MASK					(1 << 1)

#define GPIO_SDIO_GPIO8_10_MODULE				(0 << 2)
#define GPIO_SDIO_GPIO34_39_MODULE				(1 << 2)
#define GPIO_SDIO_MODULE_MASK					(1 << 2)

#define GPIO_SD_DMA_MODULE						(0 << 3)
#define GPIO_SD_HOST_MODULE						(1 << 3)
#define GPIO_SD_MODULE_MASK						(1 << 3)

#define GPIO_PWM6_USE_GPIO18					(0 << 4)
#define GPIO_PWM6_USE_GPIO26					(1 << 4)
#define GPIO_PWM6_USE_GPIO30					(2 << 4)
#define GPIO_PWM6_USE_MASK						(3 << 4)

#define GPIO_PWM7_USE_GPIO19					(0 << 6)
#define GPIO_PWM7_USE_GPIO27					(1 << 6)
#define GPIO_PWM7_USE_GPIO31					(2 << 6)
#define GPIO_PWM7_USE_MASK						(3 << 6)

#define GPIO_PWM8_USE_GPIO20					(0 << 8)
#define GPIO_PWM8_USE_GPIO28					(1 << 8)
#define GPIO_PWM8_USE_GPIO32					(2 << 8)
#define GPIO_PWM8_USE_MASK						(3 << 8)

#define GPIO_PWM9_USE_GPIO21					(0 << 10)
#define GPIO_PWM9_USE_GPIO29					(1 << 10)
#define GPIO_PWM9_USE_GPIO33					(2 << 10)
#define GPIO_PWM9_USE_MASK						(3 << 10)

#define GPIO_SPDIF_USE_GPIO11					(0 << 12)
#define GPIO_SPDIF_USE_GPIO12					(1 << 12)
#define GPIO_SPDIF_USE_GPIO13					(2 << 12)
#define GPIO_SPDIF_USE_MASK						(3 << 12)

#define REG_GPIO_DBG_MUX                     (GPIO_BASE_ADDR + 0x24*4)
#define REG_GPIO_DBG_CFG                     (GPIO_BASE_ADDR + 0x25*4)
#define REG_GPIO_DBG_REPORT                  (GPIO_BASE_ADDR + 0x27*4)

void gpio_adc_function_set(uint8_t channel);

#endif // _GPIO_BK7271_H_
#endif

// EOF



