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
#include "dvp_act.h"
#include "lcd_act.h"

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


#define TAG "dvp_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

bool lcd_register = false;

dvp_info_t dvp_info;


extern void camera_frame_buffer_dump(void);



void dvp_open_handle(const dvp_camera_config_t *config)
{
	LOGI("%s\n", __func__);

	if (DVP_STATE_DISABLED != get_dvp_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		return;
	}

	bk_dvp_camera_driver_init(config);

	set_dvp_camera_state(DVP_STATE_ENABLED);
}

void dvp_close_handle(void)
{
	LOGI("%s\n", __func__);

	if (DVP_STATE_DISABLED == get_dvp_camera_state())
	{
		LOGI("%s already close\n", __func__);
		return;
	}

	bk_dvp_camera_driver_deinit();

	set_dvp_camera_state(DVP_STATE_DISABLED);
}


void dvp_lcd_reg_cam_init_req_handle(void)
{
	dvp_camera_device_t *device = bk_dvp_camera_get_device();

	lcd_register = true;

	if (device != NULL && device->id != ID_UNKNOW)
	{
		media_msg_t msg;

		LOGI("%s camera already init\n", __func__);

		msg.event = EVENT_LCD_DVP_REG_CAM_INIT_RES;

		media_send_msg(&msg);
	}
}

void dvp_camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_DVP_OPEN_IND:
			dvp_open_handle((const dvp_camera_config_t *)param);
			break;
		case EVENT_DVP_LCD_REG_CAM_INIT_REQ:
			dvp_lcd_reg_cam_init_req_handle();
			break;
		case EVENT_DVP_CLOSE_IND:
			dvp_close_handle();
			break;
	}
}

dvp_state_t get_dvp_camera_state(void)
{
	return dvp_info.state;
}

void set_dvp_camera_state(dvp_state_t state)
{
	dvp_info.state = state;
}

void dvp_camera_init(void)
{
	dvp_info.state = DVP_STATE_DISABLED;
}
