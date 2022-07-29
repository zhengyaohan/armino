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
#include "media_evt.h"
#include "lcd_act.h"
#include "frame_buffer.h"

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
#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>

#include <driver/pwm.h>

#define TAG "lcd"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uint32_t media_decoder_isr_count = 0;
uint32_t media_rgb_isr_count = 0;


lcd_info_t lcd_info = {0};
static uint8_t lcd_frame_id = 0;
#define LCD_DMA_TRANSFER_SIZE (65280)
#define PARTICAL_XS   101
#define PARTICAL_XE   580
#define PARTICAL_YS   101
#define PARTICAL_YE   372

void lcd_frame_pingpong_insert(frame_buffer_t *buffer)
{
	LOGD("YUV frame: %u\n", buffer->sequence);

	if (lcd_info.ping_frame == NULL
	    && lcd_info.pong_frame == NULL)
	{
		lcd_info.ping_frame = buffer;
		lcd_info.display_frame = buffer;
		bk_lcd_set_display_base_addr((uint32_t)lcd_info.ping_frame->frame);
		bk_lcd_rgb_display_en(1);
	}
	else
	{
		if (lcd_info.ping_frame != lcd_info.display_frame)
		{
			if (lcd_info.ping_frame != NULL)
			{
				frame_buffer_free_request(lcd_info.ping_frame, MODULE_DISPLAY);
			}
			lcd_info.ping_frame = buffer;
		}

		if (lcd_info.pong_frame != lcd_info.display_frame)
		{
			if (lcd_info.pong_frame != NULL)
			{
				frame_buffer_free_request(lcd_info.pong_frame, MODULE_DISPLAY);
			}
			lcd_info.pong_frame = buffer;
		}

	}
}


static void lcd_rgb_isr(void)
{
	media_rgb_isr_count++;

	if (lcd_info.ping_frame == lcd_info.display_frame
	    && lcd_info.pong_frame != NULL)
	{
		frame_buffer_free_request(lcd_info.display_frame, MODULE_DISPLAY);
		lcd_info.ping_frame = NULL;
		lcd_info.display_frame = lcd_info.pong_frame;
		bk_lcd_set_display_base_addr((uint32_t)lcd_info.pong_frame->frame);
	}

	if (lcd_info.pong_frame == lcd_info.display_frame
	    && lcd_info.ping_frame != NULL)
	{
		frame_buffer_free_request(lcd_info.display_frame, MODULE_DISPLAY);
		lcd_info.display_frame = lcd_info.ping_frame;
		lcd_info.pong_frame = NULL;
		bk_lcd_set_display_base_addr((uint32_t)lcd_info.ping_frame->frame);
	}

	bk_lcd_rgb_display_en(1);
}

static void jpeg_dec_eof_cb(void)
{
	//  bk_gpio_set_output_high(GPIO_3)
	media_decoder_isr_count++;

	lcd_frame_pingpong_insert(lcd_info.decoder_frame);

	if (lcd_info.jpeg_frame && true == frame_buffer_get_state())
	{
		media_msg_t msg;

		LOGD("free decoder frame: %u\n", lcd_info.jpeg_frame->sequence);
		msg.event = EVENT_COM_FRAME_DECODER_FREE_IND;
		msg.param = (uint32_t)lcd_info.jpeg_frame;
		media_send_msg(&msg);
		lcd_info.jpeg_frame = NULL;
	}

	//  bk_gpio_set_output_low(GPIO_3);
}



int lcd_set_partical(uint16_t src_x, uint16_t src_y, uint16_t dst_x, uint16_t dst_y)
{
	uint16_t start_x, end_x, start_y, end_y;

	start_x = (src_x - dst_x) / 2 + 1;
	end_x = start_x + dst_x - 1;

	start_y = (src_y - dst_y) / 2 + 1;
	end_y = start_y + dst_y - 1;

	LOGI("%s, offset %d, %d, %d, %d\n", __func__, start_x, end_x, start_y, end_y);

	bk_lcd_set_partical_display(start_x, end_x, start_y, end_y);
	return 0;
}

int lcd_driver_init(uint32_t lcd_ppi)
{
	int ret = kGeneralErr;
	dvp_camera_device_t *dvp_device = NULL;
	bool yuv_mode = false;

	LOGI("%s, ppi: %dX%d\n", __func__, lcd_ppi >> 16, lcd_ppi & 0xFFFF);

	if (lcd_info.state == LCD_STATE_ENABLED)
	{
		LOGE("alread enable\n", __func__);
		return ret;
	}

	dvp_device = bk_dvp_camera_get_device();

	if (dvp_device == NULL || dvp_device->id == ID_UNKNOW)
	{
		LOGE("dvp camera was not init\n");
#if (CONFIG_USB_UVC)
		uvc_camera_device_t *uvc_device = NULL;

		uvc_device = bk_uvc_camera_get_device();

		if (uvc_device == NULL)
		{
			LOGE("uvc camera was not init\n");
			return kGeneralErr;
		}

		lcd_info.src_pixel_x = uvc_device->width;
		lcd_info.src_pixel_y = uvc_device->height;
#else
		return kGeneralErr;
#endif
	}
	else
	{
		lcd_info.src_pixel_x = ppi_to_pixel_x(dvp_device->ppi);
		lcd_info.src_pixel_y = ppi_to_pixel_y(dvp_device->ppi);

		if (dvp_device->mode == DVP_MODE_YUV)
		{
			yuv_mode = true;
		}
	}

	lcd_info.dec_pixel_x = lcd_info.src_pixel_x;

	lcd_info.count = 0;

	ret = bk_jpeg_dec_driver_init();

	media_decoder_isr_count = 0;
	media_rgb_isr_count = 0;

	if (ret != BK_OK)
	{
		return ret;
	}

	lcd_info.lcd_pixel_x = ppi_to_pixel_x(lcd_ppi);
	lcd_info.lcd_pixel_y = ppi_to_pixel_y(lcd_ppi);

	//  bk_gpio_enable_output(GPIO_2);
	//  bk_gpio_enable_output(GPIO_3);
	//  bk_gpio_enable_output(GPIO_6);

#if CONFIG_PWM

	pwm_init_config_t config = {0};
	BK_LOG_ON_ERR(bk_pwm_driver_init());

	config.period_cycle = 10;
	config.duty_cycle = 9;
	BK_LOG_ON_ERR(bk_pwm_init(PWM_ID_1, &config));
	BK_LOG_ON_ERR(bk_pwm_start(PWM_ID_1));
#endif

	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());
	bk_lcd_driver_init(LCD_8M);

	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_rgb_isr);

	lcd_info.ping_frame = NULL;
	lcd_info.pong_frame = NULL;
	lcd_info.jpeg_frame = NULL;
	lcd_info.decoder_frame = NULL;

	if (yuv_mode)
	{
		LOGI("set lcd for yuv display\n");
		bk_lcd_rgb_init(LCD_TYPE_480_272, lcd_info.src_pixel_x, lcd_info.src_pixel_y, ORGINAL_YUYV_DATA);
	}
	else
	{
		LOGI("set lcd for jpg decode display\n");
		bk_lcd_rgb_init(POSEDGE_OUTPUT, lcd_info.src_pixel_x, lcd_info.src_pixel_y, VUYY_DATA);
	}

	lcd_set_partical(lcd_info.src_pixel_x, lcd_info.src_pixel_y,
	                 lcd_info.lcd_pixel_x, lcd_info.lcd_pixel_y);

	bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_dec_eof_cb);
	
	lcd_info.state = LCD_STATE_ENABLED;
	LOGI("%s successful\n", __func__);
	return ret;
}

uint8_t lcd_frame_complete_callback(frame_buffer_t *buffer)
{
	bool lock = false;

	if (buffer->type == FRAME_JPEG)
	{
		media_msg_t msg;

		msg.event = EVENT_LCD_FRAME_COMPLETE_IND;
		msg.param = (uint32_t)buffer;

		media_send_msg(&msg);
	}
	else if (buffer->type == FRAME_DISPLAY)
	{
		lcd_frame_pingpong_insert(buffer);
	}

	return lock;
}

void lcd_open_handle(param_pak_t *param)
{
	int ret = BK_OK;
	//media_msg_t msg;

	LOGI("%s\n", __func__);

	if (LCD_STATE_ENABLED == get_lcd_state())
	{
		LOGW("%s already open\n", __func__);
		goto out;
	}


	ret = lcd_driver_init(param->param);

	frame_buffer_frame_register(MODULE_DECODER, lcd_frame_complete_callback);
	frame_buffer_frame_register(MODULE_DISPLAY, lcd_frame_complete_callback);

	set_lcd_state(LCD_STATE_ENABLED);

out:

	MEDIA_EVT_RETURN(param, ret);
}

void lcd_close_handle(param_pak_t *param)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

	if (LCD_STATE_DISABLED == get_lcd_state())
	{
		LOGW("%s already open\n", __func__);
		goto out;
	}

	bk_dma_deinit(lcd_info.dma_channel);

	if (bk_dma_free(DMA_DEV_LCD_DATA, lcd_info.dma_channel) != BK_OK)
	{
		LOGE("free lcd dma: %d error\r\n", lcd_info.dma_channel);
	}

	bk_jpeg_dec_driver_deinit();
	bk_lcd_rgb_deinit();
	bk_dma2d_driver_deinit();

	if (lcd_info.frame)
	{
		media_msg_t msg;

		msg.event = EVENT_COM_FRAME_DECODER_FREE_IND;
		msg.param = (uint32_t)lcd_info.frame;
		media_send_msg(&msg);
		lcd_info.frame = NULL;
	}

	frame_buffer_frame_deregister(MODULE_DECODER);

	set_lcd_state(LCD_STATE_DISABLED);

out:

	MEDIA_EVT_RETURN(param, ret);
}

bk_err_t lcd_frame_decoder(frame_buffer_t *buffer)
{
	int ret = BK_FAIL;
	frame_buffer_t *frame = NULL;


	if (buffer->frame[0] != 0xFF || buffer->frame[1] != 0xD8)
	{
		ret = BK_FAIL;
		LOGE("%s frame header error\n");
		goto error;
	}

	if (lcd_info.jpeg_frame == NULL)
	{
		lcd_info.jpeg_frame = buffer;
		frame = frame_buffer_alloc(FRAME_DISPLAY);
		frame->lock++;

		if (frame == NULL)
		{
			LOGE("%s malloc decoder frame NULL\n");
			//TODO;
			goto error;
		}

		LOGD("decoder frame: %u\n", lcd_info.jpeg_frame->sequence);
		bk_jpeg_dec_hw_init(lcd_info.src_pixel_x, lcd_info.jpeg_frame->frame, frame->frame);
		ret = bk_jpeg_dec_hw_start();
	}
	else
	{
		LOGD("decoder frame not NULL\n");
	}

	if (ret != BK_OK)
	{
		LOGE("%s frame decoder error\n", __func__);
		goto error;
	}

	lcd_info.decoder_frame = frame;

	return ret;

error:

	lcd_info.display_address = 0;
	lcd_frame_id = 0;
	lcd_info.jpeg_frame = NULL;

	if (frame != NULL)
	{
		frame_buffer_free_request(frame, MODULE_DISPLAY);
	}
	return ret;
}



void lcd_frame_complete_handle(frame_buffer_t *buffer)
{
	int ret = BK_FAIL;

	if (lcd_info.debug == true)
	{
		bk_gpio_set_output_high(GPIO_2);
	}

	if (buffer->type == FRAME_JPEG)
	{
		ret = lcd_frame_decoder(buffer);
	}
	else if (buffer->type == FRAME_DISPLAY)
	{
		//TODO
		LOGE("%s frame error\n");
	}


	if (ret != BK_OK)
	{
		media_msg_t msg;

		msg.event = EVENT_COM_FRAME_DECODER_FREE_IND;
		msg.param = (uint32_t)buffer;
		media_send_msg(&msg);
	}


	if (lcd_info.debug == true)
	{
		bk_gpio_set_output_low(GPIO_2);
	}
}


void lcd_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_LCD_OPEN_IND:
			lcd_open_handle((param_pak_t *)param);
			break;

		case EVENT_LCD_FRAME_COMPLETE_IND:
			lcd_frame_complete_handle((frame_buffer_t *)param);
			break;

		case EVENT_LCD_CLOSE_IND:
			lcd_close_handle((param_pak_t *)param);
			break;
	}
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

	lcd_info.state = LCD_STATE_DISABLED;
	lcd_info.debug = false;
}


