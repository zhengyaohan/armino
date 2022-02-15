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

#include "gpio_types.h"
#include "gpio_cap.h"
#include "soc.h"

#define BIT_64(i)	((1L) << (i))
#define GPIO_DEV_MAP  \
{\
	{GPIO_0, {GPIO_DEV_UART1_TXD, GPIO_DEV_I2C1_SCL, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_1, {GPIO_DEV_UART1_RXD, GPIO_DEV_I2C1_SDA, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_2, {GPIO_DEV_SPI0_CSN, GPIO_DEV_IRDA, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_3, {GPIO_DEV_SPI0_SCK, GPIO_DEV_PWM0, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_4, {GPIO_DEV_SPI0_MOSI, GPIO_DEV_PWM1, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_5, {GPIO_DEV_SPI0_MISO, GPIO_DEV_PWM2, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_6, {GPIO_DEV_UART2_TXD, GPIO_DEV_PWM3, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_7, {GPIO_DEV_UART2_RXD, GPIO_DEV_PWM4, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_8, {GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_PWM5, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_9, {GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_TXEN, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_10, {GPIO_DEV_SDIO_HOST_DATA0, GPIO_DEV_RXEN, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_11, {GPIO_DEV_SPDIF1, GPIO_DEV_JTAG_TCK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_12, {GPIO_DEV_SPDIF2, GPIO_DEV_JTAG_TMS, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_13, {GPIO_DEV_HDMI_CEC, GPIO_DEV_JTAG_TDI, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_14, {GPIO_DEV_SPDIF3, GPIO_DEV_JTAG_TDO, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_15, {GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_16, {GPIO_DEV_I2C2_SCL, GPIO_DEV_UART2_TXD, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_17, {GPIO_DEV_I2C2_SDA, GPIO_DEV_UART2_RXD, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_18, {GPIO_DEV_I2S1_CLK, GPIO_DEV_PWM6, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_19, {GPIO_DEV_I2S1_SYNC, GPIO_DEV_PWM7, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_20, {GPIO_DEV_I2S1_DIN, GPIO_DEV_PWM8, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_21, {GPIO_DEV_I2S1_DOUT, GPIO_DEV_PWM9, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_22, {GPIO_DEV_SPI1_CSN, GPIO_DEV_I2S1_DOUT2, GPIO_DEV_QSPI_FLASH_CLK, GPIO_DEV_INVALID, GPIO_DEV_JPEG_MCLK}},\
	{GPIO_23, {GPIO_DEV_SPI1_SCK, GPIO_DEV_I2S1_DOUT3, GPIO_DEV_QSPI_FLASH_CSN, GPIO_DEV_INVALID, GPIO_DEV_JPEG_PCLK}},\
	{GPIO_24, {GPIO_DEV_SPI1_MOSI, GPIO_DEV_I2S1_MCLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_JPEG_HSYNC}},\
	{GPIO_25, {GPIO_DEV_SPI1_MISO, GPIO_DEV_PWM10, GPIO_DEV_INVALID, GPIO_DEV_INVALID, GPIO_DEV_JPEG_VSYNC}},\
	{GPIO_26, {GPIO_DEV_I2S2_CLK, GPIO_DEV_PWM6, GPIO_DEV_QSPI_RAM_CLK, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_27, {GPIO_DEV_I2S2_SYNC, GPIO_DEV_PWM7, GPIO_DEV_QSPI_RAM_CSN, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_28, {GPIO_DEV_I2S2_DIN, GPIO_DEV_PWM8, GPIO_DEV_QSPI_IO0, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_29, {GPIO_DEV_I2S2_DOUT, GPIO_DEV_PWM9, GPIO_DEV_QSPI_IO1, GPIO_DEV_INVALID, GPIO_DEV_INVALID}},\
	{GPIO_30, {GPIO_DEV_I2S3_CLK, GPIO_DEV_PWM6, GPIO_DEV_QSPI_IO2, GPIO_DEV_SPI2_SCK, GPIO_DEV_INVALID}},\
	{GPIO_31, {GPIO_DEV_I2S3_SYNC, GPIO_DEV_PWM7, GPIO_DEV_QSPI_IO3, GPIO_DEV_SPI2_CSN, GPIO_DEV_INVALID}},\
	{GPIO_32, {GPIO_DEV_I2S3_DIN, GPIO_DEV_PWM8, GPIO_DEV_QSPI_FLASH_CLK, GPIO_DEV_SPI2_MISO, GPIO_DEV_JPEG_PXDATA0}},\
	{GPIO_33, {GPIO_DEV_I2S3_DOUT, GPIO_DEV_PWM9, GPIO_DEV_QSPI_FLASH_CSN, GPIO_DEV_SPI2_MOSI, GPIO_DEV_JPEG_PXDATA1}},\
	{GPIO_34, {GPIO_DEV_UART3_TXD, GPIO_DEV_INVALID, GPIO_DEV_DMIC2_CLK, GPIO_DEV_SDIO_HOST_DATA2, GPIO_DEV_JPEG_PXDATA2}},\
	{GPIO_35, {GPIO_DEV_UART3_RXD, GPIO_DEV_PWM11, GPIO_DEV_DMIC2_DAT, GPIO_DEV_SDIO_HOST_DATA1, GPIO_DEV_JPEG_PXDATA3}},\
	{GPIO_36, {GPIO_DEV_SPI2_CSN, GPIO_DEV_DIG_CLKOUT1, GPIO_DEV_INVALID, GPIO_DEV_SDIO_HOST_DATA3, GPIO_DEV_JPEG_PXDATA4}},\
	{GPIO_37, {GPIO_DEV_SPI2_SCK, GPIO_DEV_DIG_CLKOUT2, GPIO_DEV_INVALID, GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_JPEG_PXDATA5}},\
	{GPIO_38, {GPIO_DEV_SPI2_MOSI, GPIO_DEV_INVALID, GPIO_DEV_DMIC1_CLK, GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_JPEG_PXDATA6}},\
	{GPIO_39, {GPIO_DEV_SPI2_MISO, GPIO_DEV_INVALID, GPIO_DEV_DMIC1_DAT, GPIO_DEV_SDIO_HOST_DATA0, GPIO_DEV_JPEG_PXDATA7}},\
}

#define GPIO_MAP_TABLE(DEV_NUM, MODE_NUM, table) \
					 struct {\
						 uint64_t gpio_bits;\
						 uint8_t devs[(DEV_NUM)];\
					 } table[(MODE_NUM)]

#define GPIO_SDIO_MAP_TABLE \
				 {\
					 {BIT(8)|BIT(9)|BIT(10)|BIT64(34)|BIT64(35)|BIT64(36), {GPIO_DEV_SDIO_HOST_CLK, GPIO_DEV_SDIO_HOST_CMD, GPIO_DEV_SDIO_HOST_DATA0,GPIO_DEV_SDIO_HOST_DATA2,GPIO_DEV_SDIO_HOST_DATA1,GPIO_DEV_SDIO_HOST_DATA3}},\
					 {BIT64(34)|BIT64(35)|BIT64(36)|BIT64(37)|BIT64(38)|BIT64(39), {GPIO_DEV_SDIO_HOST_DATA2, GPIO_DEV_SDIO_HOST_DATA1, GPIO_DEV_SDIO_HOST_DATA3,GPIO_DEV_SDIO_HOST_CLK,GPIO_DEV_SDIO_HOST_CMD,GPIO_DEV_SDIO_HOST_DATA0}},\
				}

#define GPIO_SDIO_USED_GPIO_NUM 6

#define GPIO_SPI_MAP_TABLE \
				 {\
					{BIT(30)|BIT(31)|BIT64(32)|BIT64(33), {GPIO_DEV_SPI0_SCK, GPIO_DEV_SPI0_CSN, GPIO_DEV_SPI0_MISO, GPIO_DEV_SPI0_MOSI}},\
					{BIT64(36)|BIT64(37)|BIT64(38)|BIT64(39), {GPIO_DEV_SPI0_CSN, GPIO_DEV_SPI0_SCK, GPIO_DEV_SPI0_MOSI, GPIO_DEV_SPI0_MISO}},\
				}

#define GPIO_SPI_USED_GPIO_NUM 4

#define GPIO_UART2_MAP_TABLE \
				 {\
					{BIT(6)|BIT(7), {GPIO_DEV_UART2_TXD, GPIO_DEV_UART2_RXD}},\
					{BIT(16)|BIT(17), {GPIO_DEV_UART2_TXD, GPIO_DEV_UART2_RXD}},\
				}

#define GPIO_UART2_USED_GPIO_NUM 2

#define GPIO_PWM6_MAP_TABLE \
				{\
					{BIT(18), {GPIO_DEV_PWM6}},\
					{BIT(26), {GPIO_DEV_PWM6}},\
					{BIT(30), {GPIO_DEV_PWM6}},\
				}

#define GPIO_PWM6_USED_GPIO_NUM 1
#define GPIO_PWMS_USED_GPIO_NUM 1

#define GPIO_PWM7_MAP_TABLE \
				{\
					{BIT(19), {GPIO_DEV_PWM7}},\
					{BIT(27), {GPIO_DEV_PWM7}},\
					{BIT(31), {GPIO_DEV_PWM7}},\
				}

#define GPIO_PWM7_USED_GPIO_NUM 1


#define GPIO_PWM8_MAP_TABLE \
				{\
					{BIT(20), {GPIO_DEV_PWM8}},\
					{BIT(28), {GPIO_DEV_PWM8}},\
					{BIT64(32), {GPIO_DEV_PWM8}},\
				}

#define GPIO_PWM8_USED_GPIO_NUM 1


#define GPIO_PWM9_MAP_TABLE \
				{\
					{BIT(21), {GPIO_DEV_PWM9}},\
					{BIT(29), {GPIO_DEV_PWM9}},\
					{BIT64(33), {GPIO_DEV_PWM9}},\
				}

#define GPIO_PWM9_USED_GPIO_NUM 1

#define GPIO_SPDIF_MAP_TABLE \
					{\
						{BIT(11), {GPIO_DEV_SPDIF1}},\
						{BIT(12), {{GPIO_DEV_SPDIF1}}},\
						{BIT(13), {{GPIO_DEV_SPDIF1}}},\
					}

#define GPIO_SPDIF_USED_GPIO_NUM 1

#ifdef __cplusplus
}
#endif

