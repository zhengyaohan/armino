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

#include <os/os.h>
#include <components/log.h>

#include "media_core.h"
#include "trs_act.h"
#include "lcd_act.h"
#include "dvp_act.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include <soc/mapping.h>

#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/jpeg_dec.h>
#include <driver/dma2d.h>
//#include <lcd_dma2d_config.h>
#include <driver/jpeg_dec_types.h>
#include "modules/image_scale.h"
//#include "lcd_blend_config.h"


#define TAG "lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

lcd_info_t lcd_info = {0};

#define LCD_DMA_TRANSFER_SIZE (65280)

typedef struct
{
	uint32_t src_addr; /*<  src image addr*/
	uint32_t dst_addr; /*<  dst image addr*/
	uint32_t x;         /*< src image crop x point coo*/
	uint32_t y;         /*< src image crop y point coo*/
	uint32_t src_width; /*< src image width*/
	uint32_t src_height; /*< src image height*/
	uint32_t dst_width;  /*< dst image width*/
	uint32_t dst_height;  /*< dst image height*/
} dma2d_crop_params_t;

static void dma_rgb_finish_isr(dma_id_t id)
{
	//LOGD("%s\n", __func__);

	lcd_info.count ++;

	if (lcd_info.count == 4)
	{
		lcd_info.count = 0;
		bk_dma_set_src_start_addr(lcd_info.dma_channel, (uint32_t)lcd_info.display_address);
		media_msg_t msg;

		msg.event = EVENT_TRS_FRAME_FREE_IND;
		msg.param = (uint32_t)lcd_info.frame;
		media_send_msg(&msg);
		lcd_info.frame = NULL;
	}
	else
	{
		bk_dma_set_src_start_addr(lcd_info.dma_channel, ((uint32_t)lcd_info.display_address + (uint32_t)(LCD_DMA_TRANSFER_SIZE * lcd_info.count)));
		bk_dma_start(lcd_info.dma_channel);
	}
}

static void dma2d_crop_image(dma2d_crop_params_t *crop_params)
{
	uint32_t src_offset = crop_params->src_width - crop_params->dst_width;
	uint16_t *src_crop_start_addr = (uint16_t *)crop_params->src_addr + (crop_params->y * crop_params->src_width) + crop_params->x;

	dma2d_config_t dma2d_config = {0};
	dma2d_config.init.mode        = DMA2D_M2M;             /**< Mode Memory To Memory */
	dma2d_config.init.color_mode    = DMA2D_OUTPUT_RGB565; /**< Output color mode is ARGB4444 : 16 bpp */
	dma2d_config.init.output_offset = 0;                  /**< No offset on output */
	dma2d_config.init.red_blue_swap  = DMA2D_RB_REGULAR;     /**< No R&B swap for the output image */
	dma2d_config.init.alpha_inverted = DMA2D_REGULAR_ALPHA; /**< No alpha inversion for the output image */

	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_mode = DMA2D_NO_MODIF_ALPHA;     /**< Keep original Alpha from ARGB4444 input */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_alpha = 0xFF;                    /**< Fully opaque */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_color_mode = DMA2D_INPUT_RGB565;
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].input_offset = src_offset;                    /**< No offset in input */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].red_blue_swap   = DMA2D_RB_REGULAR;     /**< No R&B swap for the input image */
	dma2d_config.layer_cfg[DMA2D_FOREGROUND_LAYER].alpha_inverted = DMA2D_REGULAR_ALPHA;   /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&dma2d_config);
	bk_dma2d_layer_config(&dma2d_config, DMA2D_FOREGROUND_LAYER);

	bk_dma2d_start_transfer(&dma2d_config, (uint32_t)src_crop_start_addr, (uint32_t)crop_params->dst_addr, crop_params->dst_width, crop_params->dst_height);
	while (bk_dma2d_is_transfer_busy()) {}
}

static void jpeg_dec_eof_cb(void *param)
{
	//LOGD("%s\n", __func__);
#if 1
	media_msg_t msg;

	msg.event = EVENT_LCD_RESIZE_IND;
	media_send_msg(&msg);
#else
	bk_lcd_rgb_display_en(1);
	bk_dma_start(lcd_info.dma_channel);
#endif
}


static void dma_video_lcd_config(uint32_t dma_ch, uint32_t dma_src_mem_addr)
{
	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) dma_src_mem_addr;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	bk_dma_init(dma_ch, &dma_config);
	bk_dma_register_isr(dma_ch, NULL, dma_rgb_finish_isr);
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, LCD_DMA_TRANSFER_SIZE));
	bk_dma_enable_finish_interrupt(dma_ch);
}

int lcd_driver_init(void)
{
	int ret = kGeneralErr;
	dvp_camera_device_t *device = NULL;
	uint16_t dec_pixel_x = 0;

	LOGI("%s\n", __func__);

	if (lcd_info.state == LCD_STATE_ENABLED)
	{
		LOGE("alread enable\n", __func__);
		return ret;
	}

	device = bk_dvp_camera_get_device();

	if (device == NULL || device->id == ID_UNKNOW)
	{
		LOGE("camera was not init\n");
		return kGeneralErr;
	}

	dec_pixel_x = ppi_to_pixel_x(device->ppi) / 8;

	if (dec_pixel_x == 0x50)
	{
		lcd_info.dec_pixel_x = JPEGDEC_X_PIXEL_640;
	}
	else if (dec_pixel_x == 0x3c)
	{
		lcd_info.dec_pixel_x = JPEGDEC_X_PIXEL_480;
	}
	else if (dec_pixel_x == 0xA0)
	{
		lcd_info.dec_pixel_x = JPEGDEC_X_PIXEL_720;
	}

	lcd_info.decoder_address = psram_map->jpeg_dec;
	lcd_info.display_address = psram_map->display;
	lcd_info.count = 0;

	bk_lcd_driver_init(LCD_96M);
	bk_lcd_rgb_init(15, X_PIXEL_RGB, Y_PIXEL_RGB, VUYY_DATA);
	//bk_lcd_rgb_init(13, 640, 480, VUYY_DATA);
	bk_lcd_rgb_display_en(1);

	lcd_info.dma_channel = bk_dma_alloc(DMA_DEV_LCD_DATA);

	if ((lcd_info.dma_channel < DMA_ID_0) || (lcd_info.dma_channel >= DMA_ID_MAX))
	{
		LOGI("malloc lcd dma fail\n");
		return ret;
	}

	LOGI("malloc lcd display dma = ch%d\n", lcd_info.dma_channel);

	dma_video_lcd_config(lcd_info.dma_channel, (uint32_t)lcd_info.display_address);

	ret = bk_jpeg_dec_driver_init();

	if (ret != BK_OK)
	{
		return ret;
	}

	lcd_info.state = LCD_STATE_ENABLED;
	LOGI("%s successful\n", __func__);

	bk_jpeg_dec_complete_cb(jpeg_dec_eof_cb, lcd_info.dec_pixel_x);

	return ret;
}


void lcd_open_handle(void)
{
	LOGI("%s\n", __func__);

	dvp_camera_device_t *device = bk_dvp_camera_get_device();

	if (device == NULL || device->id == ID_UNKNOW)
	{
		media_msg_t msg;

		LOGI("%s register camera init\n", __func__);

		msg.event = EVENT_DVP_LCD_REG_CAM_INIT_REQ;

		media_send_msg(&msg);
	}
	else
	{
		lcd_driver_init();
	}
}

void lcd_dvp_reg_cam_init_res_handle(void)
{
	LOGI("%s\n", __func__);

	lcd_driver_init();
}

void lcd_frame_complete_handle(frame_buffer_t *buffer)
{
	LOGD("%s\n", __func__);

#if 1

	if (lcd_info.frame == NULL)
	{
		LOGD("%s decoder start\n", __func__);

		bk_jpeg_dec_init((uint32_t *)buffer->frame, (uint32_t *)lcd_info.decoder_address);

		lcd_info.frame = buffer;
	}
#else

	media_msg_t msg;

	msg.event = EVENT_TRS_FRAME_FREE_IND;
	msg.param = (uint32_t)buffer;
	media_send_msg(&msg);
#endif
}


void lcd_resize_handle(void)
{
	if (lcd_info.dec_pixel_x == JPEGDEC_X_PIXEL_640)
	{
		dma2d_crop_params_t  crop_params;
		crop_params.src_addr = (uint32_t)lcd_info.decoder_address;
		crop_params.dst_addr = (uint32_t)lcd_info.display_address;
		crop_params.x = (640 - 480) / 2;
		crop_params.y = (480 - 272) / 2;
		crop_params.src_width = 640;
		crop_params.src_height = 480;
		crop_params.dst_width = 480;
		crop_params.dst_height = 272;
		dma2d_crop_image(&crop_params);
	}
	else if (lcd_info.dec_pixel_x == JPEGDEC_X_PIXEL_720)
	{
		dma2d_crop_params_t  crop_params;
		crop_params.src_addr = (uint32_t)lcd_info.decoder_address;
		crop_params.dst_addr = (uint32_t)lcd_info.display_address;
		crop_params.x = (1280 - 480) / 2;
		crop_params.y = (720 - 272) / 2;
		crop_params.src_width = 1280;
		crop_params.src_height = 720;
		crop_params.dst_width = 480;
		crop_params.dst_height = 272;
		dma2d_crop_image(&crop_params);
	}

	bk_lcd_rgb_display_en(1);
	bk_dma_start(lcd_info.dma_channel);
}

void lcd_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_LCD_OPEN_IND:
			lcd_open_handle();
			break;

		case EVENT_LCD_DVP_REG_CAM_INIT_RES:
			lcd_dvp_reg_cam_init_res_handle();
			break;

		case EVENT_LCD_FRAME_COMPLETE_IND:
			lcd_frame_complete_handle((frame_buffer_t *)param);
			break;

		case EVENT_LCD_RESIZE_IND:
			lcd_resize_handle();
			break;
	}
}

void lcd_frame_complete_notify(frame_buffer_t *buffer)
{
	media_msg_t msg;

	msg.event = EVENT_LCD_FRAME_COMPLETE_IND;
	msg.param = (uint32_t)buffer;

	media_send_msg(&msg);
}

lcd_state_t get_lcd_state(void)
{
	return lcd_info.state;
}

void set_lcd_state(lcd_state_t state)
{
	lcd_info.state = state;
}

void lcd_init(void)
{
	os_memset(&lcd_info, 0, sizeof(lcd_info_t));

	lcd_info.state = LCD_STATE_DIEABLED;
}
