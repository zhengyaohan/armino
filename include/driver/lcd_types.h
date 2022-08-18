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

#include <driver/media_types.h>

#define  USE_LCD_REGISTER_CALLBACKS  1
typedef void (*lcd_isr_t)(void);

typedef enum {
	LCD_DEVICE_UNKNOW,
	LCD_DEVICE_ST7282, /* 480X270  RGB */
	LCD_DEVICE_HX8282, /* 1024X600 RGB  */
	LCD_DEVICE_GC9503V, /* 480X800 RGB  */
	LCD_DEVICE_ST7796S, /* 320X480 MCU  */
} lcd_device_id_t;

typedef enum {
	LCD_TYPE_RGB565,
	LCD_TYPE_MCU8080,
} lcd_type_t;

/**< rgb lcd clk select*/
typedef enum {
	RGB_OUTPUT_EOF =1 << 5 ,	/**< reg end of frame int,*/
	RGB_OUTPUT_SOF =1 << 4, 	 /**< reg display output start of frame  */
	I8080_OUTPUT_SOF =1 << 6,	/**< 8080 display output start of frame  */
	I8080_OUTPUT_EOF = 1 << 7,	 /**< 8080 display output end of frame	  */
}lcd_int_type_t;

typedef enum {
	LCD_320M = 0,
	LCD_160M,
	LCD_120M,
	LCD_80M,
	LCD_60M,
	LCD_54M,  //5
	LCD_40M,
	LCD_32M,
	LCD_26M,
	LCD_20M,
	LCD_12M,//10
	LCD_10M,
	LCD_8M   //12
}lcd_clk_t;

typedef enum {
	LCD_FMT_RGB565,
	LCD_FMT_ORGINAL_YUYV,
	LCD_FMT_UYVY,
	LCD_FMT_YYUV,
	LCD_FMT_UVYY,
	LCD_FMT_VUYY,
}lcd_format_t;

/** rgb data output in clk rising or falling */
typedef enum {
	POSEDGE_OUTPUT = 0,    /**< output in clk falling*/
	NEGEDGE_OUTPUT, 	   /**< output in clk rising*/
}rgb_out_clk_edge_t;

typedef struct
{
	lcd_clk_t clk;
	rgb_out_clk_edge_t data_out_clk_edge;

	uint16_t hsync_back_porch;
	uint16_t hsync_front_porch;
	uint16_t vsync_back_porch;
	uint16_t vsync_front_porch;
} lcd_rgb_t;

/** rgb lcd input data format */
typedef enum {
	RGB565_DATA = 0,        /**< input data format is rgb 565,support data width is 16bits, this is to say
		                       input data is low 16 bit valid*/
	ORGINAL_YUYV_DATA,    /**< input data is yuyv format, data width is 32bits*/
	UYVY_DATA,
	YYUV_DATA,            /**< input data is yyuv format, data width is 32bits*/
	UVYY_DATA,            /**< input data is uvyy format,data width is 32bits*/
	VUYY_DATA,            /**< input data is uvyy format,data width is 32bits*/
	YVYU_DATA,
	VYUY_DATA,
	YYVU_DATA
}rgb_input_data_format_t;

typedef struct
{
	lcd_clk_t clk;

	void (*set_display_area)(uint16 xs, uint16 xe, uint16 ys, uint16 ye);
} lcd_mcu_t;



typedef struct
{
	lcd_device_id_t id;
	char *name;
	lcd_type_t type;
	media_ppi_t ppi;
	union {
		const lcd_rgb_t *rgb;
		const lcd_mcu_t *mcu;
	};
	void (*init)(void);
} lcd_device_t;

typedef struct
{
	const lcd_device_t *device;
	lcd_format_t fmt;
	uint16_t pixel_x;
	uint16_t pixel_y;
#if	(USE_LCD_REGISTER_CALLBACKS == 1) 
	void (*complete_callback)(void);
#endif
} lcd_config_t;



/*
 * @}
 */

#ifdef __cplusplus
}
#endif


