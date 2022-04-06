/*************************************************************
 * @file        driver_gpio.c
 * @brief       code of GPIO driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#include <stdio.h>

#include "BK_System.h"
#include "driver_icu.h"
#include "driver_gpio.h"


#ifdef GPIO_3254_STYLE
static void (*p_GPIO_Int_Handler[GPIO_CHANNEL_NUMBER_ALL])(unsigned char ucGPIO_channel);


#if (SOC_BK7256 != CFG_SOC_NAME)
/*************************************************************
 * GPIO_Int_Enable
 * Description: set GPIO int enable
 * Parameters:  ucChannel:  GPIO channel
 *              ucMode:     GPIO mode, 0: GPIO low voltage interrupt
 *                                     1: GPIO high voltage interrupt
 *                                     2: GPIO positive edge interrupt
 *                                     3: GPIO negative edge interrupt
 *              p_Int_Handler: int handler
 * return:      none
 * error:       none
 */
void GPIO_Int_Enable(unsigned char ucChannel, unsigned char ucMode,
                     void (*p_Int_Handler)(unsigned char))
{
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }

    GPIO_Set_Mode(ucChannel, 0, TRUE, FALSE);

    REG_GPIO_INT_STATUS = GPIO_X_INT_STATUS_MASK(ucChannel);

    if (ucChannel < 16)
    {
        REG_GPIO_INT_TYPE_1 = (REG_GPIO_INT_TYPE_1
                               & (~GPIO_X_INT_TYPE_1_MASK(ucChannel)))
                              | ((ucMode & 0x03) << GPIO_X_INT_TYPE_1_POSI(ucChannel));
    }
    else
    {
        REG_GPIO_INT_TYPE_2 = (REG_GPIO_INT_TYPE_2
                               & (~GPIO_X_INT_TYPE_2_MASK(ucChannel)))
                              | ((ucMode & 0x03) << GPIO_X_INT_TYPE_2_POSI(ucChannel));
    }

    p_GPIO_Int_Handler[ucChannel] = p_Int_Handler;

    REG_GPIO_INT_ENABLE |= GPIO_X_INT_ENABLE_MASK(ucChannel);

    ICU_INT_ENABLE_SET(ICU_INT_ENABLE_IRQ_GPIO_MASK);
}

/*************************************************************
 * GPIO_Int_Disable
 * Description: set GPIO int disable
 * Parameters:  ucChannel:  GPIO channel
 * return:      none
 * error:       none
 */
void GPIO_Int_Disable(unsigned char ucChannel)
{
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }

    REG_GPIO_INT_STATUS = GPIO_X_INT_STATUS_MASK(ucChannel);
    REG_GPIO_INT_ENABLE &= (~GPIO_X_INT_ENABLE_MASK(ucChannel));
}

#endif

/*************************************************************
 * GPIO_int_handler_clear
 * Description: clear GPIO int handler
 * Parameters:  ucChannel:  GPIO channel
 * return:      none
 * error:       none
 */
void GPIO_int_handler_clear(unsigned char ucChannel)
{
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }
    p_GPIO_Int_Handler[ucChannel] = NULL;
}

/*************************************************************
 * GPIO_InterruptHandler
 * Description: GPIO interrupt handler
 * Parameters:  none
 * return:      none
 * error:       none
 */

#if (SOC_BK7256 != CFG_SOC_NAME)
void GPIO_InterruptHandler(void)
{
    int i;
    unsigned long ulIntStatus;

    //    printf("in GPIO_InterruptHandler\r\n");

    ulIntStatus = REG_GPIO_INT_STATUS;
    for (i = 0; i < GPIO_CHANNEL_NUMBER_ALL; i++)
    {
        if (ulIntStatus & (0x01UL << i))
        {
            if (p_GPIO_Int_Handler[i] != NULL)
            {
                (void)p_GPIO_Int_Handler[i]((unsigned char)i);
            }
        }
    }

    //    do
    //    {
    REG_GPIO_INT_STATUS = ulIntStatus;
    //    } while (ulIntStatus & REG_GPIO_INT_STATUS & REG_GPIO_INT_STATUS_MASK); // delay
}
#endif /* 7256 adapt */
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
// There is no interrupt mode for this style.
/*************************************************************
 * GPIO_InterruptHandler
 * Description: GPIO interrupt handler
 * Parameters:  none
 * return:      none
 * error:       none
 */


void GPIO_InterruptHandler(void)
{
    //    int i;
    unsigned long ulIntStatus;

    //    printf("in GPIO_InterruptHandler\r\n");

    ulIntStatus = REG_GPIO_ABCD_WU_STATUS;
    /*    for (i=0; i<GPIO_CHANNEL_NUMBER_ALL; i++)
        {
            if (ulIntStatus & (0x01UL << i))
            {
                if (p_GPIO_Int_Handler[i] != NULL)
                {
                    (void)p_GPIO_Int_Handler[i]((unsigned char)i);
                }
            }
        }*/

    do
    {
        REG_GPIO_ABCD_WU_STATUS = ulIntStatus;
    }
    while (ulIntStatus & REG_GPIO_ABCD_WU_STATUS & REG_GPIO_ABCD_WU_STATUS_MASK);   // delay
}

#endif      /* #ifdef GPIO_3254_STYLE */


/*************************************************************
 * GPIO_Set_Mode
 * Description: set GPIO mode
 * Parameters:  ucChannel:   GPIO channel
 *              ucDirection: GPIO direction, bit[0:1]: 0: input, 1: output
 *                                                     2/3: high-impedance state
 *                                           bit[4]: only available in output mode
 *                                                   0: output low, 1: output high
 *              bPullEnable: GPIO pull enable, FALSE: disable, TRUE: enable
 *              bPullmode:   GPIO pull up/down, FALSE: pull down, TRUE: pull up
 * return:      none
 * error:       none
 */
void GPIO_Set_Mode(unsigned char ucChannel, unsigned char ucDirection,
                   bool bPullEnable, bool bPullmode)
{
#ifdef GPIO_3254_STYLE
    unsigned long ulConfig = 0;

    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }

    if (ucDirection & 0x02)         // high-impedance state
    {
        ulConfig = (ulConfig & (~GPIO_CFG_INPUT_ENABLE_MASK)) | GPIO_CFG_OUTPUT_ENABLE_MASK;
    }
    else                            // input or output enable
    {
        if (ucDirection & 0x01)     // output enable
        {
            ulConfig &= ~(GPIO_CFG_INPUT_ENABLE_MASK | GPIO_CFG_OUTPUT_ENABLE_MASK);
            ulConfig |= ((ucDirection & 0x10) >> (4 - GPIO_CFG_OUTPUT_DATA_POSI));
        }
        else                        // input enable
        {
            ulConfig |= (GPIO_CFG_INPUT_ENABLE_MASK | GPIO_CFG_OUTPUT_ENABLE_MASK);
        }
    }

    if (bPullEnable == TRUE)        // pull enable
    {
        ulConfig |= GPIO_CFG_PULL_ENABLE_MASK;
    }
    else                            // pull disable
    {
        ulConfig &= (~GPIO_CFG_PULL_ENABLE_MASK);
    }

    if (bPullmode == TRUE)          // pull up
    {
        ulConfig |= GPIO_CFG_PULL_MODE_PULL_UP;
    }
    else                            // pull down
    {
        ulConfig |= GPIO_CFG_PULL_MODE_PULL_DOWN;
    }

    REG_GPIO_X_CONFIG(ucChannel) = ulConfig;
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
    unsigned long ul_config = 0;
    unsigned long ul_data = 0;
    volatile unsigned long *pul_gpio_config = NULL;
    volatile unsigned long *pul_gpio_data = NULL;

    if (ucChannel < 8)          // GPIO A
    {
        pul_gpio_config = (volatile unsigned long *)REG_GPIO_A_CONFIG_ADDR;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_A_DATA_ADDR;
    }
    else if (ucChannel < 16)    // GPIO B
    {
        ucChannel -= 8;
        pul_gpio_config = (volatile unsigned long *)REG_GPIO_B_CONFIG_ADDR;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_B_DATA_ADDR;
    }
    else if (ucChannel < 24)    // GPIO C
    {
        ucChannel -= 16;
        pul_gpio_config = (volatile unsigned long *)REG_GPIO_C_CONFIG_ADDR;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_C_DATA_ADDR;
    }
    else if (ucChannel < 32)    // GPIO D
    {
        ucChannel -= 24;
        pul_gpio_config = (volatile unsigned long *)REG_GPIO_D_CONFIG_ADDR;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_D_DATA_ADDR;
    }
    else if (ucChannel < 40)    // GPIO E
    {
        ucChannel -= 32;
        pul_gpio_config = (volatile unsigned long *)REG_GPIO_E_CONFIG_ADDR;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_E_DATA_ADDR;
    }
    else
    {
        return;
    }

    ul_config = *pul_gpio_config;
    ul_data   = *pul_gpio_data;

    ul_config |= GPIO_CONFIG_X_SECOND_FUNCTION_MASK(ucChannel);
    if (ucDirection & 0x02)         // high-impedance state
    {
        ul_config |=   GPIO_CONFIG_X_OUTPUT_ENABLE_MASK(ucChannel);
        ul_data   &= (~GPIO_DATA_X_INPUT_ENABLE_MASK(ucChannel));
    }
    else                            // input or output enable
    {
        if (ucDirection & 0x01)     // output enable
        {
            ul_config &= (~GPIO_CONFIG_X_OUTPUT_ENABLE_MASK(ucChannel));
            ul_data   &= (~GPIO_DATA_X_INPUT_ENABLE_MASK(ucChannel));
        }
        else                        // input enable
        {
            ul_config |= GPIO_CONFIG_X_OUTPUT_ENABLE_MASK(ucChannel);
            ul_data   |= GPIO_DATA_X_INPUT_ENABLE_MASK(ucChannel);
        }
    }

    if (bPullEnable == TRUE)        // pull enable
    {
        if (bPullmode == TRUE)          // pull up
        {
            ul_config = (ul_config | GPIO_CONFIG_X_PULL_UP_MASK(ucChannel))
                        & (~GPIO_CONFIG_X_PULL_DOWN_MASK(ucChannel));
        }
        else                            // pull down
        {
            ul_config = (ul_config | GPIO_CONFIG_X_PULL_DOWN_MASK(ucChannel))
                        & (~GPIO_CONFIG_X_PULL_UP_MASK(ucChannel));
        }
    }
    else                            // pull disable
    {
        ul_config &= (~(GPIO_CONFIG_X_PULL_UP_MASK(ucChannel) | GPIO_CONFIG_X_PULL_DOWN_MASK(ucChannel)));
    }

    *pul_gpio_config = ul_config;
    *pul_gpio_data   = ul_data;
#endif      /* #ifdef GPIO_3254_STYLE */
}

void GPIO_Output(unsigned char ucChannel, bool bOutputData)
{
#ifdef GPIO_3254_STYLE
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }

    if (bOutputData == TRUE)
    {
        GPIO_X_CFG_OUTPUT_DATA_SET(ucChannel);
    }
    else
    {
        GPIO_X_CFG_OUTPUT_DATA_CLEAR(ucChannel);
    }
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
    unsigned long ul_data = 0;
    volatile unsigned long *pul_gpio_data = NULL;

    if (ucChannel < 8)          // GPIO A
    {
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_A_DATA_ADDR;
    }
    else if (ucChannel < 16)    // GPIO B
    {
        ucChannel -= 8;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_B_DATA_ADDR;
    }
    else if (ucChannel < 24)    // GPIO C
    {
        ucChannel -= 16;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_C_DATA_ADDR;
    }
    else if (ucChannel < 32)    // GPIO D
    {
        ucChannel -= 24;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_D_DATA_ADDR;
    }
    else if (ucChannel < 40)    // GPIO E
    {
        ucChannel -= 32;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_E_DATA_ADDR;
    }
    else
    {
        return;
    }

    ul_data = *pul_gpio_data;

    if (bOutputData == TRUE)
    {
        ul_data |= GPIO_DATA_X_OUTPUT_VALUE_MASK(ucChannel);
    }
    else
    {
        ul_data &= (~GPIO_DATA_X_OUTPUT_VALUE_MASK(ucChannel));
    }

    *pul_gpio_data = ul_data;
#endif      /* #ifdef GPIO_3254_STYLE */
}

void GPIO_Output_Reverse(unsigned char ucChannel)
{
#ifdef GPIO_3254_STYLE
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return;
    }

    if (REG_GPIO_X_CONFIG(ucChannel) & (GPIO_CFG_OUTPUT_DATA_MASK))
    {
        GPIO_X_CFG_OUTPUT_DATA_CLEAR(ucChannel);
    }
    else
    {
        GPIO_X_CFG_OUTPUT_DATA_SET(ucChannel);
    }
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
    unsigned long ul_data = 0;
    volatile unsigned long *pul_gpio_data = NULL;

    if (ucChannel < 8)          // GPIO A
    {
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_A_DATA_ADDR;
    }
    else if (ucChannel < 16)    // GPIO B
    {
        ucChannel -= 8;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_B_DATA_ADDR;
    }
    else if (ucChannel < 24)    // GPIO C
    {
        ucChannel -= 16;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_C_DATA_ADDR;
    }
    else if (ucChannel < 32)    // GPIO D
    {
        ucChannel -= 24;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_D_DATA_ADDR;
    }
    else if (ucChannel < 40)    // GPIO E
    {
        ucChannel -= 32;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_E_DATA_ADDR;
    }
    else
    {
        return;
    }

    ul_data = *pul_gpio_data;

    if (ul_data & GPIO_DATA_X_OUTPUT_VALUE_MASK(ucChannel))
    {
        ul_data &= (~GPIO_DATA_X_OUTPUT_VALUE_MASK(ucChannel));
    }
    else
    {
        ul_data |= GPIO_DATA_X_OUTPUT_VALUE_MASK(ucChannel);
    }

    *pul_gpio_data = ul_data;
#endif      /* #ifdef GPIO_3254_STYLE */
}

unsigned char GPIO_Input(unsigned char ucChannel)
{
#ifdef GPIO_3254_STYLE
    if (ucChannel > GPIO_CHANNEL_NUMBER_MAX)
    {
        return FALSE;
    }
    return GPIO_X_CFG_INPUT_DATA_GET(ucChannel);
#elif defined GPIO_3231_STYLE       /* #ifdef GPIO_3254_STYLE */
    unsigned long ul_data = 0;
    volatile unsigned long *pul_gpio_data = NULL;

    if (ucChannel < 8)          // GPIO A
    {
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_A_DATA_ADDR;
    }
    else if (ucChannel < 16)    // GPIO B
    {
        ucChannel -= 8;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_B_DATA_ADDR;
    }
    else if (ucChannel < 24)    // GPIO C
    {
        ucChannel -= 16;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_C_DATA_ADDR;
    }
    else if (ucChannel < 32)    // GPIO D
    {
        ucChannel -= 24;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_D_DATA_ADDR;
    }
    else if (ucChannel < 40)    // GPIO E
    {
        ucChannel -= 32;
        pul_gpio_data   = (volatile unsigned long *)REG_GPIO_E_DATA_ADDR;
    }
    else
    {
        return FALSE;
    }

    ul_data = *pul_gpio_data;
    ul_data = (ul_data >> GPIO_DATA_X_INPUT_VALUE_POSI(ucChannel)) & 0x01UL;
    return ((unsigned char)ul_data);
#endif      /* #ifdef GPIO_3254_STYLE */
}


void GPIO_UART_function_enable(unsigned char ucChannel)
{
    if (ucChannel == 0)
    {
        GPIO_UART0_RX_CONFIG = GPIO_CFG_INPUT_ENABLE_MASK
                               | GPIO_CFG_FUNCTION_ENABLE_SET
                               | GPIO_CFG_PULL_MODE_PULL_UP
                               | GPIO_CFG_PULL_ENABLE_SET
                               | GPIO_CFG_OUTPUT_ENABLE_MASK;
        GPIO_UART0_TX_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
								|GPIO_CFG_PULL_MODE_PULL_UP
								|GPIO_CFG_PULL_ENABLE_SET
                               | GPIO_CFG_FUNCTION_ENABLE_SET;
#if (CFG_SOC_NAME == SOC_BK7271)
		REG_GPIO_FUNTION_MODE &= ~(GPIO_PCFG_MASK(GPIO_UART0_RX_PIN)
									| GPIO_PCFG_MASK(GPIO_UART0_TX_PIN));
		REG_GPIO_FUNTION_MODE |= (GPIO_PCFG_1_FUNC(GPIO_UART0_RX_PIN)
									| GPIO_PCFG_1_FUNC(GPIO_UART0_TX_PIN));
#elif (CFG_SOC_NAME == SOC_BK7256)
		REG_GPIO_FUNTION_MODE_2 &= ~(GPIO_PCFG_MASK(GPIO_UART0_RX_PIN)
									| GPIO_PCFG_MASK(GPIO_UART0_TX_PIN));
		REG_GPIO_FUNTION_MODE_2 |= (GPIO_PCFG2_1_FUNC(GPIO_UART0_RX_PIN)
									| GPIO_PCFG2_1_FUNC(GPIO_UART0_TX_PIN));
#else
        REG_GPIO_FUNTION_MODE &= ~(GPIO_X_FUNTION_MODE_MASK(GPIO_UART0_RX_PIN)
                                   | GPIO_X_FUNTION_MODE_MASK(GPIO_UART0_TX_PIN));
#endif
    }
    else if (ucChannel == 1)
    {
        GPIO_UART1_TX_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                               | GPIO_CFG_FUNCTION_ENABLE_SET;

        GPIO_UART1_RX_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                               | GPIO_CFG_FUNCTION_ENABLE_SET
                               | GPIO_CFG_PULL_MODE_PULL_UP
                               | GPIO_CFG_PULL_ENABLE_SET;

#if (CFG_SOC_NAME == SOC_BK7271)
		REG_GPIO_FUNTION_MODE_2 &= ~(GPIO_PCFG2_MASK(GPIO_UART1_TX_PIN)
									| GPIO_PCFG2_MASK(GPIO_UART1_RX_PIN));
		REG_GPIO_FUNTION_MODE_2 |= (GPIO_PCFG2_2_FUNC(GPIO_UART1_TX_PIN)
									| GPIO_PCFG2_2_FUNC(GPIO_UART1_RX_PIN));

		// for bk7271 uart2 rx must be enable those code
		// but tuya bk7271 only need uart1 download, so disable uart2 & uart3
		//REG_GPIO_MODULE_SEL &= ~(ICU_GPIO_MULTICOMPLEXING_UART2_MASK);
		//REG_GPIO_MODULE_SEL |= ICU_GPIO_MULTICOMPLEXING_UART2_GPIO16_GPIO17;
#elif (CFG_SOC_NAME == SOC_BK7256)
		REG_GPIO_FUNTION_MODE &= ~(GPIO_PCFG_MASK(GPIO_UART1_RX_PIN)
									| GPIO_PCFG_MASK(GPIO_UART1_TX_PIN));
		REG_GPIO_FUNTION_MODE |= (GPIO_PCFG_1_FUNC(GPIO_UART1_RX_PIN)
									| GPIO_PCFG_1_FUNC(GPIO_UART1_TX_PIN));
#else
        REG_GPIO_FUNTION_MODE &= ~(GPIO_X_FUNTION_MODE_MASK(GPIO_UART1_RX_PIN)
                                   | GPIO_X_FUNTION_MODE_MASK(GPIO_UART1_TX_PIN));
#endif
    }
    else if (ucChannel == 2) {
#if (CFG_SOC_NAME == SOC_BK7271)
		GPIO_UART2_TX_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
								| GPIO_CFG_FUNCTION_ENABLE_SET;

		GPIO_UART2_RX_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
								| GPIO_CFG_FUNCTION_ENABLE_SET
								| GPIO_CFG_PULL_MODE_PULL_UP
								| GPIO_CFG_PULL_ENABLE_SET;

		REG_GPIO_FUNTION_MODE_3 &= ~(GPIO_PCFG3_MASK(GPIO_UART2_TX_PIN)
									| GPIO_PCFG3_MASK(GPIO_UART2_RX_PIN));
		REG_GPIO_FUNTION_MODE_3 |= (GPIO_PCFG3_1_FUNC(GPIO_UART2_TX_PIN)
									| GPIO_PCFG3_1_FUNC(GPIO_UART2_RX_PIN));
#elif (CFG_SOC_NAME == SOC_BK7256)
		REG_GPIO_FUNTION_MODE_6 &= ~(GPIO_PCFG_MASK(GPIO_UART2_RX_PIN)
									| GPIO_PCFG_MASK(GPIO_UART2_TX_PIN));
		REG_GPIO_FUNTION_MODE_6 |= (GPIO_PCFG6_1_FUNC(GPIO_UART2_RX_PIN)
									| GPIO_PCFG6_1_FUNC(GPIO_UART2_TX_PIN));


#endif
    }
}

#if (CFG_SOC_NAME != SOC_BK7256)
void GPIO_I2C_FM_function_enable(void)
{
    GPIO_I2C_FM_SCL_CONFIG = (/*GPIO_CFG_PULL_MODE_PULL_UP
                          | GPIO_CFG_PULL_ENABLE_SET
                          |*/ GPIO_CFG_OUTPUT_ENABLE_MASK
                                 | GPIO_CFG_FUNCTION_ENABLE_SET);
    GPIO_I2C_FM_SDA_CONFIG = (/*GPIO_CFG_PULL_MODE_PULL_UP
                          | GPIO_CFG_PULL_ENABLE_SET
                          |*/ GPIO_CFG_OUTPUT_ENABLE_MASK
                                 | GPIO_CFG_FUNCTION_ENABLE_SET);
    REG_GPIO_FUNTION_MODE &= (~(GPIO_X_FUNTION_MODE_MASK(GPIO_I2C_FM_SCL_PIN)
                                | GPIO_X_FUNTION_MODE_MASK(GPIO_I2C_FM_SDA_PIN)));
}

void GPIO_I2C0_function_enable(void)
{
    GPIO_2F_I2C0_SCL_CONFIG = (GPIO_CFG_PULL_MODE_PULL_UP
                               | GPIO_CFG_PULL_ENABLE_SET
                               |/**/ GPIO_CFG_OUTPUT_ENABLE_MASK
                               | GPIO_CFG_FUNCTION_ENABLE_SET);
    GPIO_2F_I2C0_SDA_CONFIG = (GPIO_CFG_PULL_MODE_PULL_UP
                               | GPIO_CFG_PULL_ENABLE_SET
                               |/**/ GPIO_CFG_OUTPUT_ENABLE_MASK
                               | GPIO_CFG_FUNCTION_ENABLE_SET);
    REG_GPIO_FUNTION_MODE |= (GPIO_X_FUNTION_MODE_MASK(GPIO_2F_I2C0_SCL_PIN)
                              | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_I2C0_SDA_PIN));
}


void GPIO_SPI_DMA_function_enable(unsigned char ucChannel)
{
    if (ucChannel == 0)
    {
        GPIO_2F_SPI_SCK_CONFIG  = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_CSN_CONFIG  = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_MOSI_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_MISO_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= (GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_SCK_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_CSN_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_MOSI_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_MISO_PIN));
        REG_GPIO_SPI_SDIO_SEL = 0;
    }
}

void GPIO_SPI_function_enable(unsigned char ucChannel)
{
    if (ucChannel == 0)
    {
        GPIO_2F_SPI_SCK_CONFIG  = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_CSN_CONFIG  = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_MOSI_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        GPIO_2F_SPI_MISO_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                                  | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= (GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_SCK_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_CSN_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_MOSI_PIN)
                                  | GPIO_X_FUNTION_MODE_MASK(GPIO_2F_SPI_MISO_PIN));
        REG_GPIO_SPI_SDIO_SEL = 1;
    }
}

void GPIO_I2S_function_enable(unsigned char ucChannel)
{
    GPIO_I2S_CLK_CONFIG  = (GPIO_CFG_OUTPUT_ENABLE_MASK
                            | GPIO_CFG_FUNCTION_ENABLE_SET);
    GPIO_I2S_SYNC_CONFIG = (GPIO_CFG_OUTPUT_ENABLE_MASK
                            | GPIO_CFG_FUNCTION_ENABLE_SET);
    GPIO_I2S_DIN_CONFIG  = (GPIO_CFG_OUTPUT_ENABLE_MASK
                            | GPIO_CFG_FUNCTION_ENABLE_SET);
    GPIO_I2S_DOUT_CONFIG = (GPIO_CFG_OUTPUT_ENABLE_MASK
                            | GPIO_CFG_FUNCTION_ENABLE_SET);
    REG_GPIO_FUNTION_MODE &= ~(GPIO_X_FUNTION_MODE_MASK(GPIO_I2S_CLK_PIN)
                               | GPIO_X_FUNTION_MODE_MASK(GPIO_I2S_SYNC_PIN)
                               | GPIO_X_FUNTION_MODE_MASK(GPIO_I2S_DIN_PIN)
                               | GPIO_X_FUNTION_MODE_MASK(GPIO_I2S_DOUT_PIN));
}
#endif


#if (CFG_SOC_NAME != SOC_BK7256)
void GPIO_PWM_function_enable(unsigned char ucChannel)
{
    switch (ucChannel)
    {
    case 0:
    {
        GPIO_2F_PWM0_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM0_PIN);
        break;
    }
    case 1:
    {
        GPIO_2F_PWM1_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM1_PIN);
        break;
    }
    case 2:
    {
        GPIO_2F_PWM2_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM2_PIN);
        break;
    }
    case 3:
    {
        GPIO_2F_PWM3_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM3_PIN);
        break;
    }
    case 4:
    {
        GPIO_2F_PWM4_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM4_PIN);
        break;
    }
    case 5:
    {
        GPIO_2F_PWM5_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_PWM5_PIN);
        break;
    }
    default:
        break;
    }
}
#else
void GPIO_PWM_function_enable(unsigned char ucChannel)
{
    switch (ucChannel)
    {
    case 0:
    {
        GPIO_2F_PWM0_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM0_PIN));
	REG_GPIO_FUNTION_MODE |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM0_PIN);
        break;
    }
    case 1:
    {
        GPIO_2F_PWM1_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM1_PIN));
	REG_GPIO_FUNTION_MODE |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM1_PIN);
        break;
    }
    case 2:
    {
        GPIO_2F_PWM2_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE_2 &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM2_PIN));
	REG_GPIO_FUNTION_MODE_2 |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM2_PIN);

        break;
    }
    case 3:
    {
        GPIO_2F_PWM3_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE_2 &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM3_PIN));
	REG_GPIO_FUNTION_MODE_2 |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM3_PIN);
        break;
    }
    case 4:
    {
        GPIO_2F_PWM4_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE_4 &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM4_PIN));
	REG_GPIO_FUNTION_MODE_4 |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM4_PIN);
        break;
    }
    case 5:
    {
        GPIO_2F_PWM5_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
	REG_GPIO_FUNTION_MODE_5 &= ~(GPIO_PCFG_MASK(GPIO_2F_PWM5_PIN));
	REG_GPIO_FUNTION_MODE_5 |= GPIO_PCFG_2_FUNC(GPIO_2F_PWM5_PIN);
        break;
    }
    default:
        break;
    }
}

#endif


#if (CFG_SOC_NAME != SOC_BK7256)
void GPIO_ADC_function_enable(unsigned char ucChannel)
{
    switch (ucChannel)
    {
    case 0:
    {
        //             GPIO_2F_ADC0_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
        //                                 | GPIO_CFG_FUNCTION_ENABLE_SET;
        //             REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_ADC0_PIN);
        break;
    }
    case 1:
    {
        GPIO_2F_ADC1_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_ADC1_PIN);
        break;
    }
    case 2:
    {
        GPIO_2F_ADC2_CONFIG = GPIO_CFG_OUTPUT_ENABLE_MASK
                              | GPIO_CFG_FUNCTION_ENABLE_SET;
        REG_GPIO_FUNTION_MODE |= GPIO_X_FUNTION_MODE_MASK(GPIO_2F_ADC2_PIN);
        break;
    }
    default:
        break;
    }
}

#endif
