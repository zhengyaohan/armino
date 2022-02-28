#ifndef _PWM_H_
#define _PWM_H_

#define PWM_FAILURE                (1)
#define PWM_SUCCESS                (0)

#define PWM_WARN                   bk_printf

#define PWD_FFT_CLK_BIT                      (1 << 19)
#define PWD_USB_CLK_BIT                      (1 << 18)
#define PWD_SDIO_CLK_BIT                     (1 << 17)
#define PWD_TL410_WATCHDOG_BIT               (1 << 16)
#define PWD_AUDIO_CLK_BIT                    (1 << 15)
#define PWD_PWM5_CLK_BIT                     (1 << 14)
#define PWD_PWM4_CLK_BIT                     (1 << 13)
#define PWD_PWM3_CLK_BIT                     (1 << 12)
#define PWD_PWM2_CLK_BIT                     (1 << 11)
#define PWD_PWM1_CLK_BIT                     (1 << 10)
#define PWD_PWM0_CLK_BIT                     (1 <<  9)
#define PWD_ARM_WATCHDOG_CLK_BIT             (1 <<  8)
#define PWD_SARADC_CLK_BIT                   (1 <<  7)
#define PWD_SPI_CLK_BIT                      (1 <<  6)
#define PWD_I2C2_CLK_BIT                     (1 <<  5)
#define PWD_I2S_PCM_CLK_BIT                  (1 <<  4)
#define PWD_IRDA_CLK_BIT                     (1 <<  3)
#define PWD_I2C1_CLK_BIT                     (1 <<  2)
#define PWD_UART2_CLK_BIT                    (1 <<  1)
#define PWD_UART1_CLK_BIT                    (1 <<  0)


#define FIQ_SPI_DMA_BIT                      (1 << 27)
#define FIQ_MAC_WAKEUP_BIT                   (1 << 26)
#define FIQ_MAILBOX1_BIT                     (1 << 25)
#define FIQ_MAILBOX0_BIT                     (1 << 24)
#define FIQ_SDIO_DMA_BIT                     (1 << 23)
#define FIQ_MAC_GENERAL_BIT                  (1 << 22)
#define FIQ_MAC_PROT_TRIGGER_BIT             (1 << 21)
#define FIQ_MAC_TX_TRIGGER_BIT               (1 << 20)
#define FIQ_MAC_RX_TRIGGER_BIT               (1 << 19)
#define FIQ_MAC_TX_RX_MISC_BIT               (1 << 18)
#define FIQ_MAC_TX_RX_TIMER_BIT              (1 << 17)
#define FIQ_MODEM_BIT                        (1 << 16)
#define IRQ_GDMA_BIT                         (1 << 15)
#define IRQ_FFT_BIT                          (1 << 14)
#define IRQ_USB_BIT                          (1 << 13)
#define IRQ_SDIO_BIT                         (1 << 12)
#define IRQ_SARADC_BIT                       (1 << 11)
#define IRQ_AUDIO_BIT                        (1 << 10)
#define IRQ_PWM_BIT                          (1 << 9)
#define IRQ_TL410_WATCHDOG_BIT               (1 << 8)
#define IRQ_GPIO_BIT                         (1 << 7)
#define IRQ_SPI_BIT                          (1 << 6)
#define IRQ_I2C2_BIT                         (1 << 5)
#define IRQ_I2S_PCM_BIT                      (1 << 4)
#define IRQ_IRDA_BIT                         (1 << 3)
#define IRQ_I2C1_BIT                         (1 << 2)
#define IRQ_UART2_BIT                        (1 << 1)
#define IRQ_UART1_BIT                        (1 << 0)

#define PWM_CMD_MAGIC              (0xe230000)
enum
{
    CMD_PWM_UNIT_ENABLE = PWM_CMD_MAGIC + 1,
    CMD_PWM_UINT_DISABLE,
    CMD_PWM_IR_ENABLE,
    CMD_PWM_IR_DISABLE,
    CMD_PWM_IR_CLEAR,
    CMD_PWM_INIT_PARAM,
    CMD_PWM_CAP_GET
};

enum
{
    PWM0     = 0,
    PWM1,
    PWM2,
    PWM3,
    PWM4,
    PWM5,
    PWM_COUNT
};

typedef void (*PFUNC)(uint8_t);


#define PWM_ENABLE           (0x01)
#define PWM_DISABLE          (0x00)

#define PWM_INT_EN               (0x01)
#define PWM_INT_DIS              (0x00)

#define PMODE_PWM                   (0x00)
#define PMODE_TIMER                 (0x01)
#define PMODE_CAP_POS               (0x02)
#define PMODE_CAP_NEG               (0x03)

#define PWM_CLK_32K                        (0x00)
#define PWM_CLK_26M                        (0x01)

typedef struct
{
    uint8_t channel;


    /* cfg--PWM config
     * bit[0]:   PWM enable
     *          0:  PWM disable
     *          1:  PWM enable
     * bit[1]:   PWM interrupt enable
     *          0:  PWM interrupt disable
     *          1:  PWM interrupt enable
     * bit[3:2]: PWM mode selection
     *          00: PWM mode
     *          01: TIMER
     *          10: Capture Posedge
     *          11: Capture Negedge
     * bit[5:4]: PWM clock select
     *          00: PWM clock 32KHz
     *          01: PWM clock 26MHz
     *          10/11: PWM clock DPLL
     * bit[7:6]: reserved
     */
    union
    {
        uint8_t val;
        struct
        {
            uint8_t en: 1;
            uint8_t int_en: 1;
            uint8_t mode: 2;
            uint8_t clk: 2;
            uint8_t rsv: 2;
        } bits;
    } cfg;

    u16 end_value;
    u16 duty_cycle;
    PFUNC p_Int_Handler;
} pwm_param_t;

typedef struct
{
    u32 ucChannel;
    u16 value;
} pwm_capture_t;

#if (CFG_SOC_NAME != SOC_BK7231)
#define PWM_NEW_BASE                                 (0x00802A00)

#define PWM0_CNT                                     (PWM_NEW_BASE + 0 * 4)

#define PWM1_CNT                                     (PWM_NEW_BASE + 1 * 4)

#define PWM2_CNT                                     (PWM_NEW_BASE + 2 * 4)

#define PWM0_2_CTL                                   (PWM_NEW_BASE + 3 * 4)
#define PWMCTL0_EN_BIT                               (0x01UL << 0)
#define PWMCTL1_EN_BIT                               (0x01UL << 1)
#define PWMCTL2_EN_BIT                               (0x01UL << 2)
#define PWMCTLA_CLKDIV_POSI                          (3)
#define PWMCTLA_CLKDIV_MASK                          (0x07)
#define PWMCTL0_INT_BIT                              (0x01UL << 7)
#define PWMCTL1_INT_BIT                              (0x01UL << 8)
#define PWMCTL2_INT_BIT                              (0x01UL << 9)

#define PWM3_CNT                                     (PWM_NEW_BASE + 0x10 * 4)

#define PWM4_CNT                                     (PWM_NEW_BASE + 0x11 * 4)

#define PWM5_CNT                                     (PWM_NEW_BASE + 0x12 * 4)

#define PWM4_5_CTL                                   (PWM_NEW_BASE + 0x13 * 4)
#define PWMCTL3_EN_BIT                               (0x01UL << 0)
#define PWMCTL4_EN_BIT                               (0x01UL << 1)
#define PWMCTL5_EN_BIT                               (0x01UL << 2)
#define PWMCTLB_CLKDIV_POSI                          (3)
#define PWMCTLB_CLKDIV_MASK                          (0x07)
#define PWMCTL3_INT_BIT                              (0x01UL << 7)
#define PWMCTL4_INT_BIT                              (0x01UL << 8)
#define PWMCTL5_INT_BIT                              (0x01UL << 9)

#define PWM_BASE                                     (PWM_NEW_BASE + 0x20 * 4 )
#else
#define PWM_BASE                                     (0x00802A00)
#endif

#define PWM_CTL                                      (PWM_BASE + 0 * 4)

#define PWM0_EN_BIT                                  (0x01UL << 0)
#define PWM0_INT_EN_BIT                              (0x01UL << 1)
#define PWM0_MODE_POSI                               (2)
#define PWM0_MODE_MASK                               (0x03)

#define PWM1_EN_BIT                                  (0x01UL << 4)
#define PWM1_INT_EN_BIT                              (0x01UL << 5)
#define PWM1_MODE_POSI                               (6)
#define PWM1_MODE_MASK                               (0x03)

#define PWM2_EN_BIT                                  (0x01UL << 8)
#define PWM2_INT_EN_BIT                              (0x01UL << 9)
#define PWM2_MODE_POSI                               (10)
#define PWM2_MODE_MASK                               (0x03)

#define PWM3_EN_BIT                                  (0x01UL << 12)
#define PWM3_INT_EN_BIT                              (0x01UL << 13)
#define PWM3_MODE_POSI                               (14)
#define PWM3_MODE_MASK                               (0x03)

#define PWM4_EN_BIT                                  (0x01UL << 16)
#define PWM4_INT_EN_BIT                              (0x01UL << 17)
#define PWM4_MODE_POSI                               (18)
#define PWM4_MODE_MASK                               (0x03)

#define PWM5_EN_BIT                                  (0x01UL << 20)
#define PWM5_INT_EN_BIT                              (0x01UL << 21)
#define PWM5_MODE_POSI                               (22)
#define PWM5_MODE_MASK                               (0x03)

#define PWM_INTERRUPT_STATUS                         (PWM_BASE + 1 * 4)

#define PWM0_INIT_BIT                                (0x01UL << 0)
#define PWM1_INIT_BIT                                (0x01UL << 1)
#define PWM2_INIT_BIT                                (0x01UL << 2)
#define PWM3_INIT_BIT                                (0x01UL << 3)
#define PWM4_INIT_BIT                                (0x01UL << 4)
#define PWM5_INIT_BIT                                (0x01UL << 5)

#define PWM0_COUNTER                                 (PWM_BASE + 2 * 4)

#define PWM0_END_POSI                                (0)
#define PWM0_END_MASK                                (0xFFFF)
#define PWM0_DC_POSI                                 (16)
#define PWM0_DC_MASK                                 (0xFFFF)

#define PWM0_CAPTURE                                 (PWM_BASE + 3 * 4)

#define PWM0_CAP_OUT_POSI                            (0)
#define PWM0_CAP_OUT_MASK                            (0xFFFF)

#define PWM1_COUNTER                                 (PWM_BASE + 4 * 4)

#define PWM1_END_POSI                                (0)
#define PWM1_END_MASK                                (0xFFFF)
#define PWM1_DC_POSI                                 (16)
#define PWM1_DC_MASK                                 (0xFFFF)

#define PWM1_CAPTURE                                 (PWM_BASE + 5 * 4)

#define PWM1_CAP_OUT_POSI                            (0)
#define PWM1_CAP_OUT_MASK                            (0xFFFF)

#define PWM2_COUNTER                                 (PWM_BASE + 6 * 4)

#define PWM2_END_POSI                                (0)
#define PWM2_END_MASK                                (0xFFFF)
#define PWM2_DC_POSI                                 (16)
#define PWM2_DC_MASK                                 (0xFFFF)

#define PWM2_CAPTURE                                 (PWM_BASE + 7 * 4)

#define PWM2_CAP_OUT_POSI                            (0)
#define PWM2_CAP_OUT_MASK                            (0xFFFF)

#define PWM3_COUNTER                                 (PWM_BASE + 8 * 4)

#define PWM3_END_POSI                                (0)
#define PWM3_END_MASK                                (0xFFFF)
#define PWM3_DC_POSI                                 (16)
#define PWM3_DC_MASK                                 (0xFFFF)

#define PWM3_CAPTURE                                 (PWM_BASE + 9 * 4)

#define PWM3_CAP_OUT_POSI                            (0)
#define PWM3_CAP_OUT_MASK                            (0xFFFF)

#define PWM4_COUNTER                                 (PWM_BASE + 10 * 4)

#define PWM4_END_POSI                                (0)
#define PWM4_END_MASK                                (0xFFFF)
#define PWM4_DC_POSI                                 (16)
#define PWM4_DC_MASK                                 (0xFFFF)

#define PWM4_CAPTURE                                 (PWM_BASE + 11 * 4)

#define PWM4_CAP_OUT_POSI                            (0)
#define PWM4_CAP_OUT_MASK                            (0xFFFF)

#define PWM5_COUNTER                                 (PWM_BASE + 12 * 4)

#define PWM5_END_POSI                                (0)
#define PWM5_END_MASK                                (0xFFFF)
#define PWM5_DC_POSI                                 (16)
#define PWM5_DC_MASK                                 (0xFFFF)

#define PWM5_CAPTURE                                 (PWM_BASE + 13 * 4)

#define PWM5_CAP_OUT_POSI                            (0)
#define PWM5_CAP_OUT_MASK                            (0xFFFF)

#define REG_APB_BK_PWMn_CNT_ADDR(n)                  (PWM_BASE + 0x08 + 2 * 0x04 * (n))
#define REG_APB_BK_PWMn_CAP_ADDR(n)                  (PWM_BASE + 0x0C + 2 * 0x04 * (n))

#define CHANNEL_NO                                    6

/*******************************************************************************
* Function Declarations
*******************************************************************************/
extern void pwm_init(void);
extern void pwm_exit(void);
extern void pwm_isr(void);
u32 pwm_ctrl(u32 cmd, void *param);
#endif // _PWM_H_
// eof

