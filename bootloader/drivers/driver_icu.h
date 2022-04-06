/*************************************************************
 * @file        driver_icu.h
 * @brief       Header file of driver_icu.c
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#ifndef __DRIVER_ICU_H__

#define __DRIVER_ICU_H__


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#include "BK_config.h"

extern u32 fclk_get_tick(void);
extern void GPIO_PWM_function_enable(unsigned char ucChannel);

#if (CFG_SOC_NAME != SOC_BK7256)

#define REG_ICU_BASE_ADDR                   (0x00802000UL)


#define REG_ICU_PERI_CLK_MUX_ADDR           (REG_ICU_BASE_ADDR + 0x0 * 4)
#define REG_ICU_PERI_CLK_MUX_MASK           0x00000001UL
#define REG_ICU_PERI_CLK_MUX                (*((volatile unsigned long *) REG_ICU_PERI_CLK_MUX_ADDR))

#if (CFG_SOC_NAME != SOC_BK7231)
#define ICU_PERI_CLK_UART1_POSI                      0
#define ICU_PERI_CLK_UART2_POSI                      1
#if (SOC_BK7271 == CFG_SOC_NAME)
#define ICU_PERI_CLK_UART3_POSI                      2
#define ICU_PERI_CLK_IRDA_POSI                       3
#define ICU_PERI_CLK_FM_I2C_POSI                     4
#define ICU_PERI_CLK_I2C1_POSI                       5
#define ICU_PERI_CLK_I2C2_POSI                       6
#define ICU_PERI_CLK_SPI1_POSI                       7
#define ICU_PERI_CLK_SPI2_POSI                       8
#define ICU_PERI_CLK_SPI3_POSI                       9
#define ICU_PERI_CLK_PWMS_POSI                       10
#define ICU_PERI_CLK_SDIO_POSI                       11
#define ICU_PERI_CLK_EFUSE_POSI                      12
#define ICU_PERI_CLK_CEC_POSI                        13
#define ICU_PERI_CLK_MUX_SEL_DCO_CLK        (0x00UL << ICU_PERI_CLK_UART1_POSI  | \
                                             0x00UL << ICU_PERI_CLK_UART2_POSI  | \
                                             0x00UL << ICU_PERI_CLK_UART3_POSI  | \
                                             0x00UL << ICU_PERI_CLK_PWMS_POSI)
#define ICU_PERI_CLK_MUX_SEL_XTAL_26M       (0x01UL << ICU_PERI_CLK_UART1_POSI  | \
                                             0x01UL << ICU_PERI_CLK_UART2_POSI  | \
                                             0x01UL << ICU_PERI_CLK_UART3_POSI  | \
                                             0x01UL << ICU_PERI_CLK_PWMS_POSI)
#else
#define ICU_PERI_CLK_I2C1_POSI                       2
#define ICU_PERI_CLK_IRDA_POSI                       3
#define ICU_PERI_CLK_I2C2_POSI                       4
#define ICU_PERI_CLK_SPI_POSI                        5
#define ICU_PERI_CLK_SARADC_POSI                     6
#define ICU_PERI_CLK_PWMS_POSI                       7
#define ICU_PERI_CLK_SDIO_POSI                       8
#define ICU_PERI_CLK_SARADC_AUD_POSI                 9
#define ICU_PERI_CLK_MUX_SEL_DCO_CLK        (0x00UL << ICU_PERI_CLK_UART1_POSI  | \
                                             0x00UL << ICU_PERI_CLK_UART2_POSI  | \
                                             0x00UL << ICU_PERI_CLK_PWMS_POSI)
#define ICU_PERI_CLK_MUX_SEL_XTAL_26M       (0x01UL << ICU_PERI_CLK_UART1_POSI  | \
                                             0x01UL << ICU_PERI_CLK_UART2_POSI  | \
                                             0x01UL << ICU_PERI_CLK_PWMS_POSI)
#endif
#else
#define ICU_PERI_CLK_MUX_SEL_POSI           0
#define ICU_PERI_CLK_MUX_SEL_MASK           (0x01UL << ICU_PERI_CLK_MUX_SEL_POSI)
#define ICU_PERI_CLK_MUX_SEL_DCO_CLK        (0x00UL << ICU_PERI_CLK_MUX_SEL_POSI)
#define ICU_PERI_CLK_MUX_SEL_XTAL_26M       (0x01UL << ICU_PERI_CLK_MUX_SEL_POSI)
#endif


#define REG_ICU_PWM_CLK_MUX_ADDR            (REG_ICU_BASE_ADDR + 0x1 * 4)
#define REG_ICU_PWM_CLK_MUX_MASK            0x0000003FUL
#define REG_ICU_PWM_CLK_MUX                 (*((volatile unsigned long *) REG_ICU_PWM_CLK_MUX_ADDR))

#define ICU_PWM_X_CLK_MUX_SEL_POSI(x)       (x)
#define ICU_PWM_X_CLK_MUX_SEL_MASK(x)       (0x01UL << ICU_PWM_X_CLK_MUX_SEL_POSI(x))
#define ICU_PWM_X_CLK_MUX_SEL_LPO(x)        (0x00UL << ICU_PWM_X_CLK_MUX_SEL_POSI(x))
#define ICU_PWM_X_CLK_MUX_SEL_PCLK(x)       (0x01UL << ICU_PWM_X_CLK_MUX_SEL_POSI(x))


#define REG_ICU_PERI_CLK_PWD_ADDR           (REG_ICU_BASE_ADDR + 0x2 * 4)
#define REG_ICU_PERI_CLK_PWD_MASK           0x000BFFFFUL
#define REG_ICU_PERI_CLK_PWD                (*((volatile unsigned long *) REG_ICU_PERI_CLK_PWD_ADDR))

#if (SOC_BK7271 == CFG_SOC_NAME)
#define ICU_PERI_CLK_PWD_UART0_POSI         0
#define ICU_PERI_CLK_PWD_UART1_POSI         1
#define ICU_PERI_CLK_PWD_UART2_POSI         2
#define ICU_PERI_CLK_PWD_I2C_FM_POSI        3
#define ICU_PERI_CLK_PWD_I2C0_POSI          4
#define ICU_PERI_CLK_PWD_I2C1_POSI          5
#define ICU_PERI_CLK_PWD_SDIO_POSI          6
#define ICU_PERI_CLK_PWD_SPI1_POSI          8
#define ICU_PERI_CLK_PWD_SPI2_POSI          9
#define ICU_PERI_CLK_PWD_SPI3_POSI          10
#define ICU_PERI_CLK_PWD_IRDA_POSI          11

#define ICU_PERI_CLK_PWD_UART0_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART0_POSI)
#define ICU_PERI_CLK_PWD_UART1_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART1_POSI)
#define ICU_PERI_CLK_PWD_UART2_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART2_POSI)
#else
#define ICU_PERI_CLK_PWD_UART0_POSI         0
#define ICU_PERI_CLK_PWD_UART1_POSI         1
#define ICU_PERI_CLK_PWD_I2C_FM_POSI        2
#define ICU_PERI_CLK_PWD_IRDA_POSI          3
#define ICU_PERI_CLK_PWD_I2S_PCM_POSI       4
#define ICU_PERI_CLK_PWD_I2C0_POSI          5
#define ICU_PERI_CLK_PWD_SPI_POSI           6
#define ICU_PERI_CLK_PWD_ADC_POSI           7
#define ICU_PERI_CLK_PWD_ARM_WDT_POSI       8
#define ICU_PERI_CLK_PWD_PWM0_POSI          9
#define ICU_PERI_CLK_PWD_PWM1_POSI          10
#define ICU_PERI_CLK_PWD_PWM2_POSI          11
#define ICU_PERI_CLK_PWD_PWM3_POSI          12
#define ICU_PERI_CLK_PWD_PWM4_POSI          13
#define ICU_PERI_CLK_PWD_PWM5_POSI          14
#define ICU_PERI_CLK_PWD_PWM_X_POSI(x)      (ICU_PERI_CLK_PWD_PWM0_POSI + (x))
#define ICU_PERI_CLK_PWD_AUDIO_POSI         15
#define ICU_PERI_CLK_PWD_TL410_WDT_POSI     16
#define ICU_PERI_CLK_PWD_SDIO_POSI          17
#define ICU_PERI_CLK_PWD_USB_POSI           18
#define ICU_PERI_CLK_PWD_FFT_POSI           19

#define ICU_PERI_CLK_PWD_UART0_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART0_POSI)
#define ICU_PERI_CLK_PWD_UART1_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART1_POSI)
#define ICU_PERI_CLK_PWD_I2C_FM_MASK        (0x01UL << ICU_PERI_CLK_PWD_I2C_FM_POSI)
#define ICU_PERI_CLK_PWD_IRDA_MASK          (0x01UL << ICU_PERI_CLK_PWD_IRDA_POSI)
#define ICU_PERI_CLK_PWD_I2S_PCM_MASK       (0x01UL << ICU_PERI_CLK_PWD_I2S_PCM_POSI)
#define ICU_PERI_CLK_PWD_I2C0_MASK          (0x01UL << ICU_PERI_CLK_PWD_I2C0_POSI)
#define ICU_PERI_CLK_PWD_SPI_MASK           (0x01UL << ICU_PERI_CLK_PWD_SPI_POSI)
#define ICU_PERI_CLK_PWD_ADC_MASK           (0x01UL << ICU_PERI_CLK_PWD_ADC_POSI)
#define ICU_PERI_CLK_PWD_ARM_WDT_MASK       (0x01UL << ICU_PERI_CLK_PWD_ARM_WDT_POSI)
#define ICU_PERI_CLK_PWD_PWM0_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM0_POSI)
#define ICU_PERI_CLK_PWD_PWM1_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM1_POSI)
#define ICU_PERI_CLK_PWD_PWM2_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM2_POSI)
#define ICU_PERI_CLK_PWD_PWM3_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM3_POSI)
#define ICU_PERI_CLK_PWD_PWM4_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM4_POSI)
#define ICU_PERI_CLK_PWD_PWM5_MASK          (0x01UL << ICU_PERI_CLK_PWD_PWM5_POSI)
#define ICU_PERI_CLK_PWD_PWM_X_MASK(x)      (0x01UL << ICU_PERI_CLK_PWD_PWM_X_POSI(x))
#define ICU_PERI_CLK_PWD_AUDIO_MASK         (0x01UL << ICU_PERI_CLK_PWD_AUDIO_POSI)
#define ICU_PERI_CLK_PWD_TL410_WDT_MASK     (0x01UL << ICU_PERI_CLK_PWD_TL410_WDT_POSI)
#define ICU_PERI_CLK_PWD_SDIO_MASK          (0x01UL << ICU_PERI_CLK_PWD_SDIO_POSI)
#define ICU_PERI_CLK_PWD_USB_MASK           (0x01UL << ICU_PERI_CLK_PWD_USB_POSI)
#define ICU_PERI_CLK_PWD_FFT_MASK           (0x01UL << ICU_PERI_CLK_PWD_FFT_POSI)
#endif

#define ICU_PERI_CLK_PWD_SET(x)             do {REG_ICU_PERI_CLK_PWD |=  (x);} while(0)
#define ICU_PERI_CLK_PWD_CLEAR(x)           do {REG_ICU_PERI_CLK_PWD &= ~(x);} while(0)

#if (SOC_BK7271 == CFG_SOC_NAME)
#define ICU_FUNC_CLK_PWD                    (REG_ICU_BASE_ADDR + 0x3 * 4)

#define PWD_SDIO_DMA_CLK                             (1 << 6)
#define PWD_SECURITY_CLK                             (1 << 5)
#define PWD_QSPI_CLK                                 (1 << 4)
#define PWD_JEPG_CLK                                 (1 << 3)
#define PWD_USB2_CLK                                 (1 << 2)
#define PWD_USB1_CLK                                 (1 << 1)
#define PWD_ARM_WATCHDOG_CLK                         (1 << 0)
#else
#define REG_ICU_PERI_CLK_GATE_DIS_ADDR      (REG_ICU_BASE_ADDR + 0x3 * 4)
#define REG_ICU_PERI_CLK_GATE_DIS_MASK      0x0001FFFFUL
#define REG_ICU_PERI_CLK_GATE_DIS           (*((volatile unsigned long *) REG_ICU_PERI_CLK_GATE_DIS_ADDR))

#define ICU_PERI_CLK_GATE_DIS_ICU_POSI      0
#define ICU_PERI_CLK_GATE_DIS_UART0_POSI    1
#define ICU_PERI_CLK_GATE_DIS_UART1_POSI    2
#define ICU_PERI_CLK_GATE_DIS_I2C0_POSI     3
#define ICU_PERI_CLK_GATE_DIS_IRDA_POSI     4
#define ICU_PERI_CLK_GATE_DIS_I2S_PCM_POSI  5
#define ICU_PERI_CLK_GATE_DIS_I2C1_POSI     6
#define ICU_PERI_CLK_GATE_DIS_SPI_POSI      7
#define ICU_PERI_CLK_GATE_DIS_GPIO_POSI     8
#define ICU_PERI_CLK_GATE_DIS_WDT_POSI      9
#define ICU_PERI_CLK_GATE_DIS_PWM_POSI      10
#define ICU_PERI_CLK_GATE_DIS_AUDIO_POSI    11
#define ICU_PERI_CLK_GATE_DIS_ADC_POSI      12
#define ICU_PERI_CLK_GATE_DIS_SDIO_POSI     13
#define ICU_PERI_CLK_GATE_DIS_USB_POSI      14
#define ICU_PERI_CLK_GATE_DIS_FFT_POSI      15
#define ICU_PERI_CLK_GATE_DIS_MAC_POSI      16

#define ICU_PERI_CLK_GATE_DIS_ICU_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_ICU_POSI)
#define ICU_PERI_CLK_GATE_DIS_UART0_MASK    (0x01UL << ICU_PERI_CLK_GATE_DIS_UART0_POSI)
#define ICU_PERI_CLK_GATE_DIS_UART1_MASK    (0x01UL << ICU_PERI_CLK_GATE_DIS_UART1_POSI)
#define ICU_PERI_CLK_GATE_DIS_I2C0_MASK     (0x01UL << ICU_PERI_CLK_GATE_DIS_I2C0_POSI)
#define ICU_PERI_CLK_GATE_DIS_IRDA_MASK     (0x01UL << ICU_PERI_CLK_GATE_DIS_IRDA_POSI)
#define ICU_PERI_CLK_GATE_DIS_I2S_PCM_MASK  (0x01UL << ICU_PERI_CLK_GATE_DIS_I2S_PCM_POSI)
#define ICU_PERI_CLK_GATE_DIS_I2C1_MASK     (0x01UL << ICU_PERI_CLK_GATE_DIS_I2C1_POSI)
#define ICU_PERI_CLK_GATE_DIS_SPI_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_SPI_POSI)
#define ICU_PERI_CLK_GATE_DIS_GPIO_MASK     (0x01UL << ICU_PERI_CLK_GATE_DIS_GPIO_POSI)
#define ICU_PERI_CLK_GATE_DIS_WDT_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_WDT_POSI)
#define ICU_PERI_CLK_GATE_DIS_PWM_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_PWM_POSI)
#define ICU_PERI_CLK_GATE_DIS_AUDIO_MASK    (0x01UL << ICU_PERI_CLK_GATE_DIS_AUDIO_POSI)
#define ICU_PERI_CLK_GATE_DIS_ADC_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_ADC_POSI)
#define ICU_PERI_CLK_GATE_DIS_SDIO_MASK     (0x01UL << ICU_PERI_CLK_GATE_DIS_SDIO_POSI)
#define ICU_PERI_CLK_GATE_DIS_USB_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_USB_POSI)
#define ICU_PERI_CLK_GATE_DIS_FFT_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_FFT_POSI)
#define ICU_PERI_CLK_GATE_DIS_MAC_MASK      (0x01UL << ICU_PERI_CLK_GATE_DIS_MAC_POSI)

#define ICU_PERI_CLK_GATE_DIS_SET(x)        do {REG_ICU_PERI_CLK_GATE_DIS |=  (x);} while(0)
#define ICU_PERI_CLK_GATE_DIS_CLEAR(x)      do {REG_ICU_PERI_CLK_GATE_DIS &= ~(x);} while(0)
#endif

#define REG_ICU_TL410_CLK_PWD_ADDR          (REG_ICU_BASE_ADDR + 0x4 * 4)
#define REG_ICU_TL410_CLK_PWD_MASK          0x00000001UL
#define REG_ICU_TL410_CLK_PWD               (*((volatile unsigned long *) REG_ICU_TL410_CLK_PWD_ADDR))

#define ICU_TL410_CLK_PWD_POSI              0
#define ICU_TL410_CLK_PWD_MASK              (0x01UL << ICU_TL410_CLK_PWD_POSI)
#define ICU_TL410_CLK_PWD_CLEAR             (0x00UL << ICU_TL410_CLK_PWD_POSI)
#define ICU_TL410_CLK_PWD_SET               (0x01UL << ICU_TL410_CLK_PWD_POSI)


#define REG_ICU_CLK_26M_DIV_ADDR            (REG_ICU_BASE_ADDR + 0x5 * 4)
#define REG_ICU_CLK_26M_DIV_MASK            0x00000003UL
#define REG_ICU_CLK_26M_DIV                 (*((volatile unsigned long *) REG_ICU_CLK_26M_DIV_ADDR))

#define ICU_CLK_26M_DIV_SEL_POSI            0
#define ICU_CLK_26M_DIV_SEL_MASK            (0x03UL << ICU_CLK_26M_DIV_SEL_POSI)
#define ICU_CLK_26M_DIV_SEL_1               (0x00UL << ICU_CLK_26M_DIV_SEL_POSI)
#define ICU_CLK_26M_DIV_SEL_2               (0x01UL << ICU_CLK_26M_DIV_SEL_POSI)
#define ICU_CLK_26M_DIV_SEL_4               (0x02UL << ICU_CLK_26M_DIV_SEL_POSI)
#define ICU_CLK_26M_DIV_SEL_8               (0x03UL << ICU_CLK_26M_DIV_SEL_POSI)


#define REG_ICU_JTAG_SELECT_ADDR            (REG_ICU_BASE_ADDR + 0x6 * 4)
#define REG_ICU_JTAG_SELECT_MASK            0xFFFFFFFFUL
#define REG_ICU_JTAG_SELECT                 (*((volatile unsigned long *) REG_ICU_JTAG_SELECT_ADDR))

#define ICU_JTAG_SELECT_POSI                0
#define ICU_JTAG_SELECT_MASK                (0xFFFFFFFFUL << ICU_CLK_26M_DIV_SEL_POSI)
#define ICU_JTAG_SELECT_ARM
#define ICU_JTAG_SELECT_TL410


#define REG_ICU_INT_ENABLE_ADDR             (REG_ICU_BASE_ADDR + 0x10 * 4)
#define REG_ICU_INT_ENABLE_MASK             0x07FF7FFFUL
#define REG_ICU_INT_ENABLE                  (*((volatile unsigned long *) REG_ICU_INT_ENABLE_ADDR))

#define ICU_INT_ENABLE_IRQ_UART0_POSI       0
#define ICU_INT_ENABLE_IRQ_UART1_POSI       1
#if (CFG_SOC_NAME == SOC_BK7271)
#define ICU_INT_ENABLE_IRQ_UART2_POSI       2
#else
#define ICU_INT_ENABLE_IRQ_I2C_FM_POSI        2
#endif
#define ICU_INT_ENABLE_IRQ_IRDA_POSI        3
#define ICU_INT_ENABLE_IRQ_I2S_PCM_POSI     4
#define ICU_INT_ENABLE_IRQ_I2C0_POSI        5
#define ICU_INT_ENABLE_IRQ_SPI_POSI         6
#if (CFG_SOC_NAME == SOC_BK7271)
#define IRQ_USB1_BIT                        7 
#else
#define ICU_INT_ENABLE_IRQ_GPIO_POSI        7
#endif
#if (CFG_SOC_NAME == SOC_BK7231)
#define ICU_INT_ENABLE_IRQ_TL410_POSI       8
#else
#define ICU_INT_ENABLE_IRQ_TIMER_POSI       (8) 
#endif
#if (CFG_SOC_NAME == SOC_BK7271)
#define IRQ_SPI2_BIT 9
#else
#define ICU_INT_ENABLE_IRQ_PWM_POSI         9
#endif
#define ICU_INT_ENABLE_IRQ_AUDIO_POSI       10
#if (CFG_SOC_NAME == SOC_BK7271)
#define ICU_INT_ENABLE_IRQ_GPIO_POSI        11 
#else
#define ICU_INT_ENABLE_IRQ_ADC_POSI         11
#endif
#define ICU_INT_ENABLE_IRQ_SDIO_POSI        12
#define ICU_INT_ENABLE_IRQ_USB_POSI         13
#if (CFG_SOC_NAME == SOC_BK7271)
#define ICU_INT_ENABLE_IRQ_PWM_POSI         14
#else
#define ICU_INT_ENABLE_IRQ_FFT_POSI         14
#endif
#define ICU_INT_ENABLE_FIQ_MODEM_POSI       16
#define ICU_INT_ENABLE_FIQ_MAC_TIMER_POSI   17
#define ICU_INT_ENABLE_FIQ_MAC_MISC_POSI    18
#define ICU_INT_ENABLE_FIQ_MAC_RX_TRIG_POSI     19
#define ICU_INT_ENABLE_FIQ_MAC_TX_TRIG_POSI     20
#define ICU_INT_ENABLE_FIQ_MAC_PROT_TRIG_POSI   21
#define ICU_INT_ENABLE_FIQ_MAC_GENERAL_POSI     22
#define ICU_INT_ENABLE_FIQ_SDIO_DMA_POSI    23
#define ICU_INT_ENABLE_FIQ_MAILBOX1_POSI    24
#define ICU_INT_ENABLE_FIQ_MAILBOX2_POSI    25
#define ICU_INT_ENABLE_FIQ_MAILBOX_X_POSI(x)    (24 + (x))
#define ICU_INT_ENABLE_FIQ_MAC_WAKEUP_POSI  26
#define ICU_INT_ENABLE_FIQ_SPI_DMA_POSI     27

#define ICU_INT_ENABLE_IRQ_UART0_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART0_POSI)
#define ICU_INT_ENABLE_IRQ_UART1_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART1_POSI)
#if (CFG_SOC_NAME == SOC_BK7271)
#define ICU_INT_ENABLE_IRQ_UART2_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART2_POSI)
#endif
#define ICU_INT_ENABLE_IRQ_I2C_FM_MASK      (0x01UL << ICU_INT_ENABLE_IRQ_I2C_FM_POSI)
#define ICU_INT_ENABLE_IRQ_IRDA_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_IRDA_POSI)
#define ICU_INT_ENABLE_IRQ_I2S_PCM_MASK     (0x01UL << ICU_INT_ENABLE_IRQ_I2S_PCM_POSI)
#define ICU_INT_ENABLE_IRQ_I2C0_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_I2C0_POSI)
#define ICU_INT_ENABLE_IRQ_SPI_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_SPI_POSI)
#define ICU_INT_ENABLE_IRQ_GPIO_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_GPIO_POSI)
#define ICU_INT_ENABLE_IRQ_TL410_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_TL410_POSI)
#define ICU_INT_ENABLE_IRQ_PWM_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_PWM_POSI)
#define ICU_INT_ENABLE_IRQ_TIMER_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_TIMER_POSI)
#define ICU_INT_ENABLE_IRQ_AUDIO_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_AUDIO_POSI)
#define ICU_INT_ENABLE_IRQ_ADC_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_ADC_POSI)
#define ICU_INT_ENABLE_IRQ_SDIO_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_SDIO_POSI)
#define ICU_INT_ENABLE_IRQ_USB_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_USB_POSI)
#define ICU_INT_ENABLE_IRQ_FFT_MASK         (0x01UL << ICU_INT_ENABLE_IRQ_FFT_POSI)
#define ICU_INT_ENABLE_FIQ_MODEM_MASK       (0x01UL << ICU_INT_ENABLE_FIQ_MODEM_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_TIMER_MASK   (0x01UL << ICU_INT_ENABLE_FIQ_MAC_TIMER_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_MISC_MASK    (0x01UL << ICU_INT_ENABLE_FIQ_MAC_MISC_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_RX_TRIG_MASK     (0x01UL << ICU_INT_ENABLE_FIQ_MAC_RX_TRIG_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_TX_TRIG_MASK     (0x01UL << ICU_INT_ENABLE_FIQ_MAC_TX_TRIG_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_PROT_TRIG_MASK   (0x01UL << ICU_INT_ENABLE_FIQ_MAC_PROT_TRIG_POSI)
#define ICU_INT_ENABLE_FIQ_MAC_GENERAL_MASK     (0x01UL << ICU_INT_ENABLE_FIQ_MAC_GENERAL_POSI)
#define ICU_INT_ENABLE_FIQ_SDIO_DMA_MASK    (0x01UL << ICU_INT_ENABLE_FIQ_SDIO_DMA_POSI)
#define ICU_INT_ENABLE_FIQ_MAILBOX1_MASK    (0x01UL << ICU_INT_ENABLE_FIQ_MAILBOX1_POSI)
#define ICU_INT_ENABLE_FIQ_MAILBOX2_MASK    (0x01UL << ICU_INT_ENABLE_FIQ_MAILBOX2_POSI)
#define ICU_INT_ENABLE_FIQ_MAILBOX_X_MASK(x)    (0x01UL << ICU_INT_ENABLE_FIQ_MAILBOX_X_POSI(x))
#define ICU_INT_ENABLE_FIQ_MAC_WAKEUP_MASK  (0x01UL << ICU_INT_ENABLE_FIQ_MAC_WAKEUP_POSI)
#define ICU_INT_ENABLE_FIQ_SPI_DMA_MASK     (0x01UL << ICU_INT_ENABLE_FIQ_SPI_DMA_POSI)
#define ICU_INT_ENABLE_IRQ_MASK             (0x00007FFFUL)
#define ICU_INT_ENABLE_FIQ_MASK             (0x0FFF0000UL)

#if (CFG_SOC_NAME == SOC_BK7271)  
#define ICU_FIQ_ENABLE                               (REG_ICU_BASE_ADDR + 0x11 * 4)
#define FIQ_CEC_EN_BIT                               (1 << 6) 
#define FIQ_TOUCH_EN_BIT                             (1 << 5)
#define FIQ_RTC_EN_BIT                               (1 << 4) 
#define FIQ_DSP_WATCHDOG_EN_BIT                      (1 << 3) 
#define FIQ_BT_WATCHDOG_EN_BIT                       (1 << 2) 
#define FIQ_USB_PLUG_INOUT_EN_BIT                    (1 << 1)  
#define FIQ_DPLL_UNLOCK_EN_BIT                       (1 << 0) 
#endif

#if (CFG_SOC_NAME == SOC_BK7271)  
#define REG_ICU_INT_GLOBAL_ENABLE_ADDR      (REG_ICU_BASE_ADDR + 0x12 * 4)
#else
#define REG_ICU_INT_GLOBAL_ENABLE_ADDR      (REG_ICU_BASE_ADDR + 0x11 * 4)
#endif
#define REG_ICU_INT_GLOBAL_ENABLE_MASK      0x0003UL
#define REG_ICU_INT_GLOBAL_ENABLE           (*((volatile unsigned long *) REG_ICU_INT_GLOBAL_ENABLE_ADDR))

#define ICU_INT_GLOBAL_ENABLE_IRQ_POSI      0
#define ICU_INT_GLOBAL_ENABLE_IRQ_MASK      (0x01UL << ICU_INT_GLOBAL_ENABLE_IRQ_POSI)
#define ICU_INT_GLOBAL_ENABLE_IRQ_SET       (0x01UL << ICU_INT_GLOBAL_ENABLE_IRQ_POSI)

#define ICU_INT_GLOBAL_ENABLE_FIQ_POSI      1
#define ICU_INT_GLOBAL_ENABLE_FIQ_MASK      (0x01UL << ICU_INT_GLOBAL_ENABLE_FIQ_POSI)
#define ICU_INT_GLOBAL_ENABLE_FIQ_SET       (0x01UL << ICU_INT_GLOBAL_ENABLE_FIQ_POSI)

#define ICU_INT_ENABLE_SET(x)               do {                                    \
                REG_ICU_INT_ENABLE |=  (x);                                         \
                /*if (ICU_INT_ENABLE_FIQ_MODEM_MASK > (x))                            \
                {                                                                   \
                    REG_ICU_INT_GLOBAL_ENABLE |= ICU_INT_GLOBAL_ENABLE_IRQ_MASK;    \
                }                                                                   \
                else                                                                \
                {                                                                   \
                    REG_ICU_INT_GLOBAL_ENABLE |= ICU_INT_GLOBAL_ENABLE_FIQ_MASK;    \
                }*/                                                                \
            } while(0)
#define ICU_INT_ENABLE_CLEAR(x)             do {                                    \
                REG_ICU_INT_ENABLE &= ~(x);                                         \
            } while(0)

#if (CFG_SOC_NAME == SOC_BK7271)  
#define REG_ICU_INT_RAW_STATUS_ADDR      (REG_ICU_BASE_ADDR + 0x13 * 4)
#else
#define REG_ICU_INT_RAW_STATUS_ADDR         (REG_ICU_BASE_ADDR + 0x12 * 4)
#endif
#define REG_ICU_INT_RAW_STATUS_MASK         REG_ICU_INT_ENABLE_MASK
#define REG_ICU_INT_RAW_STATUS              (*((volatile unsigned long *) REG_ICU_INT_RAW_STATUS_ADDR))

#define ICU_INT_RAW_STATUS_BIT_GET(x)       (REG_ICU_INT_RAW_STATUS & (x))
#define ICU_INT_RAW_STATUS_BIT_CLEAR(x)     do {REG_ICU_INT_RAW_STATUS |= (x);} while(0)

#if (CFG_SOC_NAME == SOC_BK7271)  
#define ICU_FIQ_RAW_STATUS                           (REG_ICU_BASE_ADDR + 0x14 * 4)
#endif

#if (CFG_SOC_NAME == SOC_BK7271)  
#define REG_ICU_INT_STATUS_ADDR             (REG_ICU_BASE_ADDR + 0x15 * 4)
#else
#define REG_ICU_INT_STATUS_ADDR             (REG_ICU_BASE_ADDR + 0x13 * 4)
#endif
#define REG_ICU_INT_STATUS_MASK             REG_ICU_INT_ENABLE_MASK
#define REG_ICU_INT_STATUS                  (*((volatile unsigned long *) REG_ICU_INT_STATUS_ADDR))

#define ICU_INT_STATUS_BIT_GET(x)           (REG_ICU_INT_STATUS & (x))
#define ICU_INT_STATUS_BIT_CLEAR(x)         do {REG_ICU_INT_STATUS |= (x);} while(0)

#if (CFG_SOC_NAME == SOC_BK7271)  
#define ICU_FIQ_STATUS                           (REG_ICU_BASE_ADDR + 0x16 * 4)
#endif

#if (CFG_SOC_NAME == SOC_BK7271)  
#define REG_ICU_WAKEUP_ENABLE_ADDR             (REG_ICU_BASE_ADDR + 0x17 * 4)
#else
#define REG_ICU_WAKEUP_ENABLE_ADDR          (REG_ICU_BASE_ADDR + 0x14 * 4)
#endif
#define REG_ICU_WAKEUP_ENABLE_MASK          REG_ICU_INT_ENABLE_MASK
#define REG_ICU_WAKEUP_ENABLE               (*((volatile unsigned long *) REG_ICU_WAKEUP_ENABLE_ADDR))

#define ICU_INT_WAKEUP_ENABLE_SET(x)        do {REG_ICU_WAKEUP_ENABLE |=  (x);} while(0)
#define ICU_INT_WAKEUP_ENABLE_CLEAR(x)      do {REG_ICU_WAKEUP_ENABLE &= ~(x);} while(0)

#if (CFG_SOC_NAME == SOC_BK7271)  
#define ICU_ARM_WAKEUP2_EN                           (REG_ICU_BASE_ADDR + 0x18 * 4)
#endif


    typedef enum
    {
        WAKE_UP_PER_UART0        = 0x01,
        WAKE_UP_PER_UART1        = 0x02,
        WAKE_UP_PER_ADC          = 0x03,
        WAKE_UP_PER_I2C0         = 0x04,
        WAKE_UP_PER_I2C1         = 0x05,
        WAKE_UP_PER_I2S          = 0x06,
        WAKE_UP_PER_SPI          = 0x07,
        WAKE_UP_PER_DMA          = 0x08,
        WAKE_UP_PER_RTC          = 0x09,
        WAKE_UP_PER_WDT          = 0x0A,
        WAKE_UP_PER_FFT          = 0x0B,
        WAKE_UP_PER_PWM          = 0x0C,
        WAKE_UP_PER_PWM1         = 0x0D,
        WAKE_UP_PER_PWM2         = 0x0E,
        WAKE_UP_PER_PWM3         = 0x0F,
        WAKE_UP_PER_GPIO         = 0x10,
        WAKE_UP_PER_I2C_FM       = 0x11,
    }
    WakeUpPeripheral;

    extern void ICU_init(void);
	extern void icu_init(void);
    extern void ICU_Cali_32KHz(void);

    extern void sys_sleep_mode_normal_voltage(void);
    extern void sys_sleep_mode_low_voltage(void);
    extern void sys_sleep_mode_deep_sleep(void);
    extern void sys_wake_up1(void);

    extern void sys_idle_mode_normal_voltage(unsigned char ucWakeUpPeripheral, unsigned char ucParameter);
    extern void sys_idle_mode_low_voltage(unsigned char ucWakeUpPeripheral, unsigned char ucParameter);
    extern void sys_deep_sleep(unsigned char ucWakeUpPeripheral, unsigned char ucParameter);
    extern void sys_wake_up(unsigned char ucWakeUpPeripheral, unsigned char ucParameter);




#else

//bk7256 adapt
#define REG_ICU_BASE_ADDR                   (0x44010000UL)

#define ICU_PERI_CLK_PWD_UART0_POSI         2
#define ICU_PERI_CLK_PWD_UART1_POSI         10
#define ICU_PERI_CLK_PWD_UART2_POSI         11


#define ICU_PERI_CLK_PWD_UART0_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART0_POSI)
#define ICU_PERI_CLK_PWD_UART1_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART1_POSI)
#define ICU_PERI_CLK_PWD_UART2_MASK         (0x01UL << ICU_PERI_CLK_PWD_UART2_POSI)



#define REG_ICU_PERI_CLK_PWD_ADDR           (REG_ICU_BASE_ADDR + 0xc * 4)
#define REG_ICU_PERI_CLK_PWD                (*((volatile unsigned long *) REG_ICU_PERI_CLK_PWD_ADDR))

#define ICU_PERI_CLK_PWD_CLEAR(x)           do {REG_ICU_PERI_CLK_PWD |= (x);} while(0)

#define REG_ICU_PWM_CLK_MUX_ADDR            (REG_ICU_BASE_ADDR + 0x8 * 4)
#define REG_ICU_PWM_CLK_MUX                 (*((volatile unsigned long *) REG_ICU_PWM_CLK_MUX_ADDR))


#define ICU_INT_ENABLE_IRQ_GPIO_POSI        55
#define ICU_INT_ENABLE_IRQ_UART0_POSI       4
#define ICU_INT_ENABLE_IRQ_UART1_POSI       15
#define ICU_INT_ENABLE_IRQ_UART2_POSI       16

#define ICU_INT_ENABLE_IRQ_GPIO_MASK        (0x01UL << ICU_INT_ENABLE_IRQ_GPIO_POSI)
#define ICU_INT_ENABLE_IRQ_UART0_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART0_POSI)
#define ICU_INT_ENABLE_IRQ_UART1_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART1_POSI)
#define ICU_INT_ENABLE_IRQ_UART2_MASK       (0x01UL << ICU_INT_ENABLE_IRQ_UART2_POSI)


#define REG_ICU_INT_ENABLE_ADDR             (REG_ICU_BASE_ADDR + 0x20 * 4)
//#define REG_ICU_INT_ENABLE_MASK             0x07FF7FFFUL
#define REG_ICU_INT_ENABLE                  (*((volatile unsigned long *) REG_ICU_INT_ENABLE_ADDR))

#define ICU_INT_ENABLE_SET(x)               do {                                    \
                REG_ICU_INT_ENABLE |=  (x);                                         \
            } while(0)
#define ICU_INT_ENABLE_CLEAR(x)             do {                                    \
                REG_ICU_INT_ENABLE &= ~(x);                                         \
            } while(0)

    extern void ICU_init(void);

#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif      /* __DRIVER_ICU_H__ */
