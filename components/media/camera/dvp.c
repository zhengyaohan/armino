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
#include "camera_act.h"
#include "lcd_act.h"
#include "storage_act.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "frame_buffer.h"

#define TAG "dvp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static const dvp_host_config_t host_config =
{
	.id = CONFIG_CAMERA_I2C_ID,
	.baud_rate = I2C_BAUD_RATE_100KHZ,
	.addr_mode = I2C_ADDR_MODE_7BIT,
	.mclk = GPIO_27,
	.pck = GPIO_29,
	.hsync = GPIO_30,
	.vsync = GPIO_31,
	.pxdata0 = GPIO_32,
	.pxdata1 = GPIO_33,
	.pxdata2 = GPIO_34,
	.pxdata3 = GPIO_35,
	.pxdata4 = GPIO_36,
	.pxdata5 = GPIO_37,
	.pxdata6 = GPIO_38,
	.pxdata7 = GPIO_39
};


void frame_buffer_jpg_complete(frame_buffer_t *buffer)
{
	frame_buffer_generate_complete(buffer, FRAME_JPEG);
}

frame_buffer_t *frame_buffer_jpg_alloc(void)
{
	return frame_buffer_alloc(FRAME_JPEG);
}

static const dvp_camera_config_t dvp_camera_jpg_config =
{
	.host = &host_config,
	.frame_complete = frame_buffer_jpg_complete,
	.frame_alloc = frame_buffer_jpg_alloc,
};

void frame_buffer_yuv_complete(frame_buffer_t *buffer)
{
	frame_buffer_generate_complete(buffer, FRAME_DISPLAY);
}

frame_buffer_t *frame_buffer_yuv_alloc(void)
{
	return frame_buffer_alloc(FRAME_DISPLAY);
}

static const dvp_camera_config_t dev_camera_yuv_config =
{
	.host = &host_config,
	.frame_complete = frame_buffer_yuv_complete,
	.frame_alloc = frame_buffer_yuv_alloc,
};


bk_err_t bk_dvp_camera_open(dvp_mode_t mode)
{
	int ret = BK_OK;

	if (DVP_MODE_JPG == mode)
	{
		ret = bk_dvp_camera_driver_init(&dvp_camera_jpg_config, mode);
	}
	else if (DVP_MODE_YUV == mode)
	{
		ret = bk_dvp_camera_driver_init(&dev_camera_yuv_config, mode);
	}

	return ret;
}

bk_err_t bk_dvp_camera_close(void)
{
	bk_dvp_camera_driver_deinit();
	return 0;
}
