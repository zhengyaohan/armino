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

#include <common/bk_err.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TRNG defines
 * @addtogroup bk_api_trng_defs TRNG API group
 * @{
 */
 typedef void (*int_isr_t)(void);

#define REG_DISP_DAT_FIFO                 (*((volatile unsigned long *)   (0x48060000 +6*4))) /**<define 8080 data fifo addr */
#define REG_DISP_RGB_FIFO                 (*((volatile unsigned long *)   (0x48060000 +2*4))) /**<define rgb fifo addr */


#define vsync_front_porch  8
#define vsync_back_porch   8
#define hsync_front_porch  5
#define hsync_back_porch   40


#if(0)
typedef enum {
	DAT_WR_THRD=0,  /*< max 8bit size */
	CMD_WR_THRD,	/*< max 8bit size  */
	DAT_RD_THRD,    /*< max 8bit size */
	CMD_RD_THRD     /*< max 8bit size */

}LCD_8080_THRD_TYPE;

typedef enum {
	WR_THRD=0, /*< max 10bit size */
	RD_THRD,   /*< max 10bit size */
}LCD_RGB_THRD_TYPE;

#endif

typedef enum {
	X_PIXEL_8080 = 320,
	Y_PIXEL_8080 = 480,
	X_PIXEL_RGB = 480,
	Y_PIXEL_RGB = 272,
}DISPLAY_PIXEL_FORMAT;

typedef enum {
	POSEDGE = 0,
	NEGEDGE
}LCD_RGB_OUTPUT_EDGE;


typedef enum {
	RGB565_DATA=0,     /**< bit width is 16bits*/
	ORGINAL_YUYV_DATA, /**< bit width is 32bits*/ /**<uyvy_data yyuv_data uvyy_data vuyy_data*/
	UYVY_DATA,
	YYUV_DATA,
	UVYY_DATA,
	VUYY_DATA,
}RGB_DATA_FORMAT;

/**< used as param API bk_lcd_int_config,
 *< also used get int status check as  API bk_lcd_int_status_get
 *< also used clear int status as  API bk_lcd_int_status_clear*/
typedef enum {
	RGB_OUTPUT_EOF=1 << 5 , 	/**< reg end of frame int,*/
	RGB_OUTPUT_SOF=1 << 4,      /**< reg display output start of frame  */
	I8080_OUTPUT_SOF =1 << 6,   /**< 8080 display output start of frame  */
	I8080_OUTPUT_EOF= 1 << 7,   /**< 8080 display output end of frame    */
}LCD_INT_TYPE;

typedef enum {
	 X_PIXEL = (0x7FF << 0),       /**< read lcd status, used bond with API bk_lcd_display_ll_status_get */
	 STRFIFO_BYTE_REV = (1 << 11),
	 Y_PIXEL = (0x7FF << 12),
	 STR_FIFO_CLR = (1 << 23),
	 RGB_DISP_ON = (1 << 24),
	 MEM_DAT_WIDTH = (1 << 25),
	 DISPLAY_ON  = (1 << 26),
	 RGB_CLK_DIV = (7 << 27),
	 RGB_ON = (1 << 30),
	 DISCONTI_MODE = (1<< 31)
}LCD_STATUS;


/*
 * @}
 */

#ifdef __cplusplus
}
#endif


