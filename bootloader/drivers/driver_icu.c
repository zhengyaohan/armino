/*************************************************************
 * @file        driver_icu.c
 * @brief       code of ICU driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_icu.h"

#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_uart2.h"
#include "driver_sys_ctrl.h"


#if (CFG_SOC_NAME == SOC_BK7256)
void ICU_init(void)
{
//    REG_WIFI_PWD = 0x0;
//    REG_DSP_PWD  = 0x0;

//    REG_USB_PWD  = 0x0;

    DelayUS(100);
    //    set_LPO_clk_XTAL_32KHz();
#if 0
    #if (CFG_SOC_NAME != SOC_BK7271)
    set_LPO_clk_ROSC_32KHz();
    #endif

#if (MCU_CLK == MCU_CLK_26MHz)
    set_CPU_clk_XTAL_26MHz();
    REG_SYS_CTRL_ANALOG_REG4 = 0x59E04520;
#elif (MCU_CLK == MCU_CLK_120MHz)
    set_CPU_clk_DPLL();
#endif

#if (PER_CLK == PER_CLK_26MHz)
    set_PCLK_clk_XTAL_26MHz();
#elif (PER_CLK == PER_CLK_120MHz)
    //DcoCalib(DCO_CALIB_120M);
    sctrl_cali_dpll();
    set_PCLK_clk_DCO_CLK();
#endif
    DelayUS(100);
    //    REG_SYS_CTRL_CLK_SET |= SYS_CTRL_CLK_SET_AHB_CLK_DIV_MASK;

    REG_SYS_CTRL_BIAS_CALIB = (REG_SYS_CTRL_BIAS_CALIB
                               & (~(SYS_CTRL_BIAS_CALIB_MANUAL_MASK | SYS_CTRL_BIAS_CALIB_SETTING_MASK)))
                              | (0x01 << SYS_CTRL_BIAS_CALIB_MANUAL_POSI)
                              | (0x14 << SYS_CTRL_BIAS_CALIB_SETTING_POSI);

    REG_ICU_INT_ENABLE = 0;
    #if (CFG_SOC_NAME == SOC_BK7271)
    REG_WRITE(ICU_FIQ_ENABLE, 0);
    #endif

    REG_ICU_INT_GLOBAL_ENABLE = (ICU_INT_GLOBAL_ENABLE_IRQ_MASK | ICU_INT_GLOBAL_ENABLE_FIQ_MASK);
#endif
}
#else
// set CPU clk to 32KHz to save power
void set_CPU_clk_32KHz(void)
{
    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_32KHz;
    DelayUS(100);
    REG_SYS_CTRL_LPO_CLK = SYS_CTRL_LPO_CLK_MUX_XTAL_32KHz;
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                            & (~SYS_CTRL_CLK_SET_MCLK_MASK))
                           | SYS_CTRL_CLK_SET_MCLK_LPO;
    DelayUS(100);
}

// set CPU clk to XTAL 26MHz
void set_CPU_clk_XTAL_26MHz(void)
{
    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz;
    DelayUS(100);
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                            & (~SYS_CTRL_CLK_SET_MCLK_MASK))
                           | SYS_CTRL_CLK_SET_MCLK_XTAL_26M;
    DelayUS(100);
}

// set CPU clk to DCO 26MHz
void set_CPU_clk_DCO_26MHz(void)
{
    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_DCO_26MHz;
    DelayUS(100);
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                            & (~SYS_CTRL_CLK_SET_MCLK_MASK))
                           | SYS_CTRL_CLK_SET_MCLK_DCO;
    DelayUS(100);
}

#define DCO_CALIB_26M           0x0
#define DCO_CALIB_60M           0x1
#define DCO_CALIB_80M           0x2
#define DCO_CALIB_120M          0x3
#define DCO_CALIB_180M          0x4

#if 0
void DcoCalib(unsigned char speed)
{
    switch (speed)
    {
        case DCO_CALIB_180M:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0xDDUL << 16)
                                     | (0x1UL  << 31);
            break;
			
        case DCO_CALIB_120M:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0x127UL << 16);
            break;
			
        case DCO_CALIB_80M:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0x0C5UL << 16);
            break;
			
        case DCO_CALIB_60M:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0x127UL << 16) | (0x02UL << 12);
            break;
			
        case DCO_CALIB_26M:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0x0C0UL << 16) | (0x03UL << 12);
            break;
			
        default:
            REG_SYS_CTRL_ANALOG_REG1 = (REG_SYS_CTRL_ANALOG_REG1 & (~((0x1FFUL << 16) | (0x03UL << 12))))
                                     | (0x0C0UL << 16) | (0x03UL << 12);
            break;
    }

    REG_SYS_CTRL_ANALOG_REG1 &= (~0x02000000);
    DelayNops(100);
	
    REG_SYS_CTRL_ANALOG_REG1 |= ( 0x02000000);
    DelayNops(100);
	
    REG_SYS_CTRL_ANALOG_REG1 |= ( 0x8000);
    DelayNops(1000);
	
    REG_SYS_CTRL_ANALOG_REG1 &= (~0x8000);
    DelayNops(1000);
}

// set CPU clk to DCO
void set_CPU_clk_DCO(void)
{
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                         & (~SYS_CTRL_CLK_SET_MCLK_MASK))
                         | SYS_CTRL_CLK_SET_MCLK_DCO;
    DelayUS(100);
}
#endif

void sctrl_cali_dpll(void)
{
    UINT32 param;
   
    param = REG_READ(REG_SYS_CTRL_ANALOG_REG1_ADDR);
    param &= ~(SPI_TRIG_BIT);
    REG_WRITE(REG_SYS_CTRL_ANALOG_REG1_ADDR, param);
    
    DelayUS(10);

    param |= (SPI_TRIG_BIT);
    REG_WRITE(REG_SYS_CTRL_ANALOG_REG1_ADDR, param);   
    
    param = REG_READ(REG_SYS_CTRL_ANALOG_REG1_ADDR);
    param &= ~(SPI_DET_EN);
    REG_WRITE(REG_SYS_CTRL_ANALOG_REG1_ADDR, param);

    DelayUS(200);

    param = REG_READ(REG_SYS_CTRL_ANALOG_REG1_ADDR);
    param |= (SPI_DET_EN);
    REG_WRITE(REG_SYS_CTRL_ANALOG_REG1_ADDR, param);
}

// set CPU clk to DPLL
#if (CFG_SOC_NAME != SOC_BK7271)  
void set_CPU_clk_DPLL(void)
{
    UINT32 param;
    
    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz;

    #if (CFG_SOC_NAME == SOC_BK7231)  
    REG_SYS_CTRL_ANALOG_REG0 = 0x819A59B;
    #else
    REG_SYS_CTRL_ANALOG_REG0 = 0xF819A59B;//0x819A59B;
    #endif // (CFG_SOC_NAME == SOC_BK7231)
    
    REG_SYS_CTRL_ANALOG_REG1 = 0x6AC03102;
    
    #if (CFG_SOC_NAME == SOC_BK7231)
    param = 0x24006000;//0x27006000
    #else
    param = 0x24026080;   // xtalh_ctune   // 24006080
    param &= ~(XTALH_CTUNE_MASK<< XTALH_CTUNE_POSI);
    param |= ((0x10&XTALH_CTUNE_MASK) << XTALH_CTUNE_POSI);
    #endif // (CFG_SOC_NAME == SOC_BK7231)
    REG_SYS_CTRL_ANALOG_REG2 = param;
    
    REG_SYS_CTRL_ANALOG_REG3 = 0x4FE06C50;
    
    #if (CFG_SOC_NAME == SOC_BK7231)
    REG_SYS_CTRL_ANALOG_REG4 = 0x59E04520;
    #else	
    REG_SYS_CTRL_ANALOG_REG4 = 0x59C04520; 
    #endif // (CFG_SOC_NAME == SOC_BK7231)

    #if (CFG_SOC_NAME == SOC_BK7231)  
    DelayNops(100);
    REG_SYS_CTRL_ANALOG_REG0 = 0x0811A54B;
    DelayNops(100);
    REG_SYS_CTRL_ANALOG_REG0 = 0x0819A54B;
    DelayNops(1000);
    #else
    DelayNops(1300);
    #endif

    DelayUS(100);
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                            & (~SYS_CTRL_CLK_SET_MCLK_MASK)
                            & (~SYS_CTRL_CLK_SET_MCLK_DIV_MASK))
                            | (3 << SYS_CTRL_CLK_SET_MCLK_DIV_POSI)
                           | SYS_CTRL_CLK_SET_MCLK_DPLL;
    DelayUS(100);
}
#elif (CFG_SOC_NAME == SOC_BK7271)  
void set_CPU_clk_DPLL(void)
{
    UINT32 param;
    //set vdddig to 1.2v
    addPMU_Reg0x2 = 0x50;
    
    DelayUS(100);
    REG_SYS_CTRL_CLK_SET = (REG_SYS_CTRL_CLK_SET
                            & (~SYS_CTRL_CLK_SET_MCLK_MASK)
                            & (~SYS_CTRL_CLK_SET_MCLK_DIV_MASK))
                            & ~(HCLK_DIV2_MASK << HCLK_DIV2_POSI)
                            & ~(MTB_PRIVILEGE_MASK << MTB_PRIVILEGE_POSI)
                            | (HCLK_DIV2_EN_BIT)
                            | (MTB_PRIVILEGE_ACCESS_AHB << MTB_PRIVILEGE_POSI)
                            | (3 << SYS_CTRL_CLK_SET_MCLK_DIV_POSI)
                            | SYS_CTRL_CLK_SET_MCLK_DPLL;
    DelayUS(100);
    //rtc init
    addPMU_Reg0xd = 681800000;      //from 2000-01-01. date(2000-01-01)=730486
    addPMU_Reg0xf = 0x1;
}
#endif

// set PCLK to XTAL 26MHz
void set_PCLK_clk_XTAL_26MHz(void)
{
    REG_ICU_PERI_CLK_MUX = ICU_PERI_CLK_MUX_SEL_XTAL_26M;
}

// set PCLK to DCO clock
void set_PCLK_clk_DCO_CLK(void)
{
    REG_ICU_PERI_CLK_MUX = ICU_PERI_CLK_MUX_SEL_DCO_CLK;
}


void set_LPO_clk_ROSC_32KHz(void)
{
    REG_SYS_CTRL_LPO_CLK = SYS_CTRL_LPO_CLK_MUX_ROSC_32KHz;
    REG_SYS_CTRL_ROSC_CONFIG = 0x37;
}

void set_LPO_clk_XTAL_32KHz(void)
{
    REG_SYS_CTRL_LPO_CLK = SYS_CTRL_LPO_CLK_MUX_XTAL_32KHz;
}

void set_LPO_clk_32KHz_div_from_XTAL_26MHz(void)
{
    REG_SYS_CTRL_LPO_CLK = SYS_CTRL_LPO_CLK_MUX_DIV_32KHz;
}


ASK is_CPU_reset(void)
{
    return YES;
}


void ICU_init(void)
{
#if (CFG_SOC_NAME != SOC_BK7271)  
    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_FLASH
                            | SYS_CTRL_BLOCK_EN_SW_DCO_26MHz
                            | SYS_CTRL_BLOCK_EN_SW_DPLL
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_TO_RF
                            | SYS_CTRL_BLOCK_EN_SW_ROSC_32KHz
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_32KHz
                            ;

#else
	u32 reg;

    REG_SYS_CTRL_BLOCK_EN = (REG_SYS_CTRL_BLOCK_EN
                             & (~SYS_CTRL_BLOCK_EN_VALID_MASK))
                            | SYS_CTRL_BLOCK_EN_VALID_SET
                            | SYS_CTRL_BLOCK_EN_SW_DCO_26MHz
                            | SYS_CTRL_BLOCK_EN_SW_DPLL
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_TO_RF
                            | SYS_CTRL_BLOCK_EN_SW_XTAL_26MHz
                            ;
	reg = REG_READ(0x00802000+0x3*4);
	reg |= 0x1;
	REG_WRITE(0x00802000+0x3*4,reg);
#endif

    REG_WIFI_PWD = 0x0;
    REG_DSP_PWD  = 0x0;
    
    REG_USB_PWD  = 0x0;
    //(*((volatile unsigned long *) 0x01050000)) = 0x09;

    DelayUS(100);
    //    set_LPO_clk_XTAL_32KHz();
    #if (CFG_SOC_NAME != SOC_BK7271)  
    set_LPO_clk_ROSC_32KHz();
    #endif

#if (MCU_CLK == MCU_CLK_26MHz)
    set_CPU_clk_XTAL_26MHz();
    REG_SYS_CTRL_ANALOG_REG4 = 0x59E04520;
#elif (MCU_CLK == MCU_CLK_120MHz)
    set_CPU_clk_DPLL();
#endif

#if (PER_CLK == PER_CLK_26MHz)
    set_PCLK_clk_XTAL_26MHz();
#elif (PER_CLK == PER_CLK_120MHz)
    //DcoCalib(DCO_CALIB_120M);
    sctrl_cali_dpll();
    set_PCLK_clk_DCO_CLK();
#endif
    DelayUS(100);
    //    REG_SYS_CTRL_CLK_SET |= SYS_CTRL_CLK_SET_AHB_CLK_DIV_MASK;

    REG_SYS_CTRL_BIAS_CALIB = (REG_SYS_CTRL_BIAS_CALIB
                               & (~(SYS_CTRL_BIAS_CALIB_MANUAL_MASK | SYS_CTRL_BIAS_CALIB_SETTING_MASK)))
                              | (0x01 << SYS_CTRL_BIAS_CALIB_MANUAL_POSI)
                              | (0x14 << SYS_CTRL_BIAS_CALIB_SETTING_POSI);

    REG_ICU_INT_ENABLE = 0;
    #if (CFG_SOC_NAME == SOC_BK7271)
    REG_WRITE(ICU_FIQ_ENABLE, 0);
    #endif

    REG_ICU_INT_GLOBAL_ENABLE = (ICU_INT_GLOBAL_ENABLE_IRQ_MASK | ICU_INT_GLOBAL_ENABLE_FIQ_MASK);
    __enable_irq();
    __enable_fiq();
}

#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
void icu_init(void)
{
    UINT32 param;
	UINT32 reg;

    param = (1 << ICU_PERI_CLK_UART1_POSI)
			| (1 << ICU_PERI_CLK_UART2_POSI)
            | (1 << ICU_PERI_CLK_SARADC_POSI)
            | (1 << ICU_PERI_CLK_PWMS_POSI)
            | (1 << ICU_PERI_CLK_SDIO_POSI)
            | (1 << ICU_PERI_CLK_I2C1_POSI)
            | (1 << ICU_PERI_CLK_I2C2_POSI);

	reg = REG_READ(REG_ICU_PERI_CLK_MUX_ADDR);
	reg |= param;
	REG_WRITE(REG_ICU_PERI_CLK_MUX_ADDR, reg);
}
#endif

void set_GPIO_wake_up(unsigned char ucChannel)
{
    //bk_printf("set GPIO wake up\r\n");
    DelayMS(50);

#ifdef GPIO_3254_STYLE
    GPIO_Int_Enable(ucChannel, 2, NULL);
    ICU_INT_WAKEUP_ENABLE_SET(ICU_INT_ENABLE_IRQ_GPIO_MASK);
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
    if (ucChannel < 32UL)
    {
        GPIO_Set_Mode(ucChannel, 0, TRUE, FALSE);       // input pull_down
        REG_GPIO_ABCD_WU_TYPE   |= (0x01UL   << ucChannel);
        REG_GPIO_ABCD_WU_ENABLE |= (0x01UL   << ucChannel);
        REG_GPIO_ABCD_WU_STATUS |= (0x01UL   << ucChannel);
        //        REG_ICU_GPIO_DEEP_WAKE1  = (0x01UL   << ucChannel);
    }
#if (CHIP_ID == CHIP_3433)
    else if (ul_GPIO_wake_up_pin < 40UL)      // GPIOE all N/A
    {
        GPIO_Set_Mode(ucChannel, 0, TRUE, FALSE);
        ucChannel -= 32;
        REG_GPIO_E_WU_TYPE      |= (0x01UL   << ucChannel);
        REG_GPIO_E_WU_ENABLE    |= (0x01UL   << ucChannel);
        REG_GPIO_E_WU_STATUS    |= (0x01UL   << ucChannel);
        REG_ICU_GPIO_DEEP_WAKE2  = (REG_ICU_GPIO_DEEP_WAKE2 & (~REG_ICU_GPIO_DEEP_WAKE2_MASK))
                                   | (0x01UL   << ucChannel);
    }
#endif      /* #if (CHIP_ID == CHIP_3433) */
    else
    {
        //bk_printf("GPIO pin is out of range!\r\n");
    }
#endif      /* #ifdef GPIO_3254_STYLE */
}

void set_UART0_wake_up(void)
{
    //bk_printf("set UART0 wake up\r\n");
#if ((PRINT_PORT != DEBUG_PORT_UART0) )
    uart0_init(0);
#endif
    uart0_wait_tx_finish();
    //    ICU_INT_WAKEUP_ENABLE_SET(ICU_INT_ENABLE_IRQ_UART0_MASK);
}

void set_UART1_wake_up(void)
{
    //bk_printf("set UART1 wake up\r\n");
#if ((PRINT_PORT != DEBUG_PORT_UART1) )
    uart1_init(115200);
#endif
    uart1_wait_tx_finish();
}

void sys_idle_mode_normal_voltage(unsigned char ucWakeUpPeripheral, unsigned char ucParameter)
{
    DelayMS(50);

    switch (ucWakeUpPeripheral)
    {
    case WAKE_UP_PER_UART0:
        set_UART0_wake_up();
        break;

    case WAKE_UP_PER_UART1:
        set_UART1_wake_up();
        break;

    case WAKE_UP_PER_GPIO:
        set_GPIO_wake_up(ucParameter);
        ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_GPIO_MASK);
        break;

    default:
        break;
    }

    REG_ICU_INT_GLOBAL_ENABLE = 0;

    REG_SYS_CTRL_SLEEP = SYS_CTRL_SLEEP_SLEEP_MODE_NORMAL;
    DelayNops(500);
}

void sys_idle_mode_low_voltage(unsigned char ucWakeUpPeripheral, unsigned char ucParameter)
{
    uart0_wait_tx_finish();

    GPIO_Int_Enable(ucParameter, 2, NULL);
    ICU_INT_WAKEUP_ENABLE_SET(ICU_INT_ENABLE_IRQ_GPIO_MASK);

    REG_ICU_INT_GLOBAL_ENABLE = 0;

    REG_SYS_CTRL_SLEEP = SYS_CTRL_SLEEP_SLEEP_MODE_LOW;
    DelayNops(500);
}

void sys_deep_sleep(unsigned char ucWakeUpPeripheral, unsigned char ucParameter)
{
    REG_ICU_INT_GLOBAL_ENABLE = 0;

    uart0_wait_tx_finish();
    GPIO_Set_Mode(ucParameter, 0, TRUE, FALSE);

    REG_ICU_INT_GLOBAL_ENABLE = 0;
    REG_SYS_CTRL_SLEEP = SYS_CTRL_SLEEP_SLEEP_MODE_DEEPSLEEP;
    DelayNops(500);
}

void sys_wake_up(unsigned char ucWakeUpPeripheral, unsigned char ucParameter)
{
    switch (ucWakeUpPeripheral)
    {
    case WAKE_UP_PER_UART0:
        break;

    case WAKE_UP_PER_UART1:
        break;

    case WAKE_UP_PER_GPIO:
        break;

    default:
        break;
    }

    REG_ICU_INT_GLOBAL_ENABLE = (ICU_INT_GLOBAL_ENABLE_IRQ_MASK | ICU_INT_GLOBAL_ENABLE_FIQ_MASK);
}


#endif
// eof

