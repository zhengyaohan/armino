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

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <driver/hal/gpio_types.h>
#include "gpio_cap.h"
#include <arch/soc.h>

#define BIT_64(i)	((1L) << (i))
#define GPIO_DEV_MAP  \
{\
	{GPIO_0, {GPIO_DEV_UART2_TXD, GPIO_DEV_I2C1_SCL, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_1, {GPIO_DEV_UART2_RXD, GPIO_DEV_I2C1_SDA, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_2, {GPIO_DEV_SPI1_SCK, GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG0}},\
	{GPIO_3, {GPIO_DEV_SPI1_CSN, GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG1}},\
	{GPIO_4, {GPIO_DEV_SPI1_MOSI, GPIO_DEV_SDIO_HOST_DATA0, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG2}},\
	{GPIO_5, {GPIO_DEV_SPI1_MISO, GPIO_DEV_SDIO_HOST_DATA1, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG3}},\
	{GPIO_6, {GPIO_DEV_CLK13M, GPIO_DEV_PWM0, GPIO_DEV_I2S1_CLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG4}},\
	{GPIO_7, {GPIO_DEV_WIFI_ACTIVE, GPIO_DEV_PWM1, GPIO_DEV_I2S1_SYNC, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG5}},\
	{GPIO_8, {GPIO_DEV_BT_ACTIVE, GPIO_DEV_PWM2, GPIO_DEV_I2S1_DIN, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG6}},\
	{GPIO_9, {GPIO_DEV_BT_PRIORITY, GPIO_DEV_PWM3, GPIO_DEV_I2S1_DOUT, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_DEBUG7}},\
	{GPIO_10,{GPIO_DEV_UART1_RXD, GPIO_DEV_SDIO_HOST_DATA2, GPIO_DEV_CLK_AUXS, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_11, {GPIO_DEV_UART1_TXD, GPIO_DEV_SDIO_HOST_DATA3, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_12, {GPIO_DEV_UART1_CTS, GPIO_DEV_USB0_DP, GPIO_DEV_INVALID, GPIO_DEV_TOUCH0, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_13, {GPIO_DEV_UART1_RTS, GPIO_DEV_USB0_DN, GPIO_DEV_INVALID, GPIO_DEV_TOUCH1, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_14, {GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_SPI0_SCK, GPIO_DEV_BT_ANT0, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB20, GPIO_DEV_DEBUG8}},\
	{GPIO_15, {GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_SPI0_CSN, GPIO_DEV_BT_ANT1, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB19, GPIO_DEV_DEBUG9}},\
	{GPIO_16, {GPIO_DEV_SDIO_HOST_DATA0, GPIO_DEV_SPI0_MOSI, GPIO_DEV_BT_ANT2, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB18, GPIO_DEV_DEBUG10}},\
	{GPIO_17, {GPIO_DEV_SDIO_HOST_DATA1, GPIO_DEV_SPI0_MISO, GPIO_DEV_BT_ANT3, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB17, GPIO_DEV_DEBUG11}},\
	{GPIO_18, {GPIO_DEV_SDIO_HOST_DATA2, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB16, GPIO_DEV_DEBUG12}},\
	{GPIO_19, {GPIO_DEV_SDIO_HOST_DATA3, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB15, GPIO_DEV_DEBUG13}},\
	{GPIO_20, {GPIO_DEV_I2C0_SCL, GPIO_DEV_JTAG_TCK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB14, GPIO_DEV_INVALID}},\
	{GPIO_21, {GPIO_DEV_I2C0_SDA, GPIO_DEV_JTAG_TMS, GPIO_DEV_ADC6,  GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB13, GPIO_DEV_INVALID}},\
	{GPIO_22, {GPIO_DEV_CLK26M, GPIO_DEV_JTAG_TDI, GPIO_DEV_ADC5, GPIO_DEV_QSPI_CLK, GPIO_DEV_LCD_RGB12, GPIO_DEV_INVALID}},\
	{GPIO_23, {GPIO_DEV_INVALID, GPIO_DEV_JTAG_TDO, GPIO_DEV_ADC3, GPIO_DEV_QSPI_CSN, GPIO_DEV_LCD_RGB11, GPIO_DEV_INVALID}},\
	{GPIO_24, {GPIO_DEV_LPO_CLK, GPIO_DEV_PWM4, GPIO_DEV_ADC2,  GPIO_DEV_QSPI_IO0, GPIO_DEV_LCD_RGB10, GPIO_DEV_DEBUG14}},\
	{GPIO_25, {GPIO_DEV_IRDA, GPIO_DEV_PWM5, GPIO_DEV_ADC1, GPIO_DEV_QSPI_IO1, GPIO_DEV_LCD_RGB9, GPIO_DEV_DEBUG15}},\
	{GPIO_26, {GPIO_DEV_TXEN, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_QSPI_IO2, GPIO_DEV_LCD_RGB8, GPIO_DEV_DEBUG16}},\
	{GPIO_27, {GPIO_DEV_JPEG_MCLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_QSPI_IO3, GPIO_DEV_INVALID, GPIO_DEV_DEBUG17}},\
	{GPIO_28, {GPIO_DEV_RXEN, GPIO_DEV_I2S1_MCLK,  GPIO_DEV_ADC4, GPIO_DEV_TOUCH2, GPIO_DEV_INVALID, GPIO_DEV_DEBUG18}},\
	{GPIO_29, {GPIO_DEV_JPEG_PCLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_TOUCH3, GPIO_DEV_INVALID, GPIO_DEV_DEBUG19}},\
	{GPIO_30, {GPIO_DEV_JPEG_HSYNC, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_TOUCH4, GPIO_DEV_INVALID, GPIO_DEV_DEBUG20}},\
	{GPIO_31, {GPIO_DEV_JPEG_VSYNC, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_TOUCH5, GPIO_DEV_INVALID, GPIO_DEV_DEBUG21}},\
	{GPIO_32, {GPIO_DEV_JPEG_PXDATA0, GPIO_DEV_PWM6, GPIO_DEV_INVALID, GPIO_DEV_TOUCH6, GPIO_DEV_INVALID, GPIO_DEV_DEBUG22}},\
	{GPIO_33, {GPIO_DEV_JPEG_PXDATA1, GPIO_DEV_PWM7, GPIO_DEV_INVALID, GPIO_DEV_TOUCH7, GPIO_DEV_INVALID, GPIO_DEV_DEBUG23}},\
	{GPIO_34, {GPIO_DEV_JPEG_PXDATA2, GPIO_DEV_PWM8, GPIO_DEV_INVALID, GPIO_DEV_TOUCH8, GPIO_DEV_INVALID, GPIO_DEV_DEBUG24}},\
	{GPIO_35, {GPIO_DEV_JPEG_PXDATA3, GPIO_DEV_PWM9, GPIO_DEV_INVALID, GPIO_DEV_TOUCH9, GPIO_DEV_INVALID, GPIO_DEV_DEBUG25}},\
	{GPIO_36, {GPIO_DEV_JPEG_PXDATA4, GPIO_DEV_PWM10, GPIO_DEV_INVALID, GPIO_DEV_TOUCH10, GPIO_DEV_INVALID, GPIO_DEV_DEBUG26}},\
	{GPIO_37, {GPIO_DEV_JPEG_PXDATA5, GPIO_DEV_PWM11, GPIO_DEV_INVALID, GPIO_DEV_TOUCH11, GPIO_DEV_INVALID, GPIO_DEV_DEBUG27}},\
	{GPIO_38, {GPIO_DEV_JPEG_PXDATA6, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_TOUCH12, GPIO_DEV_INVALID, GPIO_DEV_DEBUG28}},\
	{GPIO_39, {GPIO_DEV_JPEG_PXDATA7, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_TOUCH13, GPIO_DEV_INVALID, GPIO_DEV_DEBUG29}},\
	{GPIO_40, {GPIO_DEV_UART3_RXD, GPIO_DEV_I2S1_CLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB7, GPIO_DEV_INVALID}},\
	{GPIO_41, {GPIO_DEV_UART3_TXD, GPIO_DEV_I2S1_SYNC, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB6, GPIO_DEV_INVALID}},\
	{GPIO_42, {GPIO_DEV_I2C1_SCL, GPIO_DEV_I2S1_DIN, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB5, GPIO_DEV_DEBUG30}},\
	{GPIO_43, {GPIO_DEV_I2C1_SDA, GPIO_DEV_I2S1_DOUT, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB4, GPIO_DEV_DEBUG31}},\
	{GPIO_44, {GPIO_DEV_CAN_TX, GPIO_DEV_SPI0_SCK, GPIO_DEV_ADC0, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB3, GPIO_DEV_INVALID}},\
	{GPIO_45, {GPIO_DEV_CAN_RX, GPIO_DEV_SPI0_CSN, GPIO_DEV_ADC7, GPIO_DEV_INVALID, GPIO_DEV_LCD_RGB2, GPIO_DEV_INVALID}},\
	{GPIO_46, {GPIO_DEV_CAN_STANDBY, GPIO_DEV_SPI0_MOSI, GPIO_DEV_INVALID, GPIO_DEV_TOUCH14, GPIO_DEV_LCD_RGB1, GPIO_DEV_INVALID}},\
	{GPIO_47, {GPIO_DEV_INVALID, GPIO_DEV_SPI0_MISO, GPIO_DEV_INVALID, GPIO_DEV_TOUCH15, GPIO_DEV_LCD_RGB0, GPIO_DEV_INVALID}},\
}

#define GPIO_MAP_TABLE(DEV_NUM, MODE_NUM, table) \
	struct {\
			uint64_t gpio_bits;\
			gpio_dev_t devs[DEV_NUM];\
		} table[MODE_NUM]

#define GPIO_SDIO_MAP_TABLE \
		{\
			 {BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(10)|BIT(11), {GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_SDIO_HOST_DATA0,GPIO_DEV_SDIO_HOST_DATA1,GPIO_DEV_SDIO_HOST_DATA2,GPIO_DEV_SDIO_HOST_DATA3}},\
			 {BIT(14)|BIT(15)|BIT(16)|BIT(17)|BIT(18)|BIT(19), {GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_SDIO_HOST_DATA0,GPIO_DEV_SDIO_HOST_DATA1,GPIO_DEV_SDIO_HOST_DATA2,GPIO_DEV_SDIO_HOST_DATA3}},\
		}

#define GPIO_SDIO_USED_GPIO_NUM 6

#define GPIO_SPI0_MAP_TABLE \
		{\
			{BIT(14)|BIT(15)|BIT64(16)|BIT64(17), {GPIO_DEV_SPI0_SCK, GPIO_DEV_SPI0_CSN, GPIO_DEV_SPI0_MOSI, GPIO_DEV_SPI0_MISO}},\
			{BIT64(44)|BIT64(45)|BIT64(46)|BIT64(47), {GPIO_DEV_SPI0_CSN, GPIO_DEV_SPI0_SCK, GPIO_DEV_SPI0_MOSI, GPIO_DEV_SPI0_MISO}},\
		}
#define GPIO_SPI0_USED_GPIO_NUM 4

#define GPIO_I2C1_MAP_TABLE \
		{\
			{BIT(0)|BIT(1), {GPIO_DEV_I2C1_SCL, GPIO_DEV_I2C1_SDA}},\
			{BIT64(42)|BIT64(43), {GPIO_DEV_I2C1_SCL, GPIO_DEV_I2C1_SDA}},\
		}
#define GPIO_I2C1_USED_GPIO_NUM 2

#define GPIO_I2S_MAP_TABLE \
		{\
			{BIT(6)|BIT(7)|BIT(8)|BIT(9), {GPIO_DEV_I2S1_CLK, GPIO_DEV_I2S1_SYNC, GPIO_DEV_I2S1_DIN, GPIO_DEV_I2S1_DOUT}},\
			{BIT64(40)|BIT64(41)|BIT64(42)|BIT64(43), {GPIO_DEV_I2S1_CLK, GPIO_DEV_I2S1_SYNC, GPIO_DEV_I2S1_DIN, GPIO_DEV_I2S1_DOUT}},\
		}
#define GPIO_I2S_USED_GPIO_NUM 4




#define GPIO_LCD_8080_USED_GPIO_NUM 13
#define GPIO_LCD_8080_GPIO_MAP \
{\
	{GPIO_47, GPIO_DEV_LCD_RGB0},\
	{GPIO_46, GPIO_DEV_LCD_RGB1},\
	{GPIO_45, GPIO_DEV_LCD_RGB2},\
	{GPIO_44, GPIO_DEV_LCD_RGB3},\
	{GPIO_43, GPIO_DEV_LCD_RGB4},\
	{GPIO_42, GPIO_DEV_LCD_RGB5},\
	{GPIO_41, GPIO_DEV_LCD_RGB6},\
	{GPIO_40, GPIO_DEV_LCD_RGB7},\
	{GPIO_19, GPIO_DEV_LCD_RGB15},\
	{GPIO_20, GPIO_DEV_LCD_RGB14},\
	{GPIO_21, GPIO_DEV_LCD_RGB13},\
	{GPIO_22, GPIO_DEV_LCD_RGB12},\
	{GPIO_23, GPIO_DEV_LCD_RGB11},\
}


#define GPIO_LCD_RGB_GPIO_NUM 21
#define GPIO_LCD_RGB_GPIO_MAP \
	{\
	{GPIO_47, GPIO_DEV_LCD_RGB0},\
	{GPIO_46, GPIO_DEV_LCD_RGB1},\
	{GPIO_45, GPIO_DEV_LCD_RGB2},\
	{GPIO_44, GPIO_DEV_LCD_RGB3},\
	{GPIO_43, GPIO_DEV_LCD_RGB4},\
	{GPIO_42, GPIO_DEV_LCD_RGB5},\
	{GPIO_41, GPIO_DEV_LCD_RGB6},\
	{GPIO_40, GPIO_DEV_LCD_RGB7},\
	{GPIO_26, GPIO_DEV_LCD_RGB8},\
	{GPIO_25, GPIO_DEV_LCD_RGB9},\
	{GPIO_24, GPIO_DEV_LCD_RGB10},\
	{GPIO_23, GPIO_DEV_LCD_RGB11},\
	{GPIO_22, GPIO_DEV_LCD_RGB12},\
	{GPIO_21, GPIO_DEV_LCD_RGB13},\
	{GPIO_20, GPIO_DEV_LCD_RGB14},\
	{GPIO_19, GPIO_DEV_LCD_RGB15},\
	{GPIO_18, GPIO_DEV_LCD_RGB16},\
	{GPIO_17, GPIO_DEV_LCD_RGB17},\
	{GPIO_16, GPIO_DEV_LCD_RGB18},\
	{GPIO_15, GPIO_DEV_LCD_RGB19},\
	{GPIO_14, GPIO_DEV_LCD_RGB20},\
}


#ifdef __cplusplus
}
#endif
