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
#include "media_evt.h"
#include "storage_act.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include "camera.h"
#include "frame_buffer.h"


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>


#define TAG "dvp_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


dvp_info_t dvp_info;



void dvp_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (DVP_STATE_DISABLED != get_dvp_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	frame_buffer_enable(true);

	ret = bk_dvp_camera_open();


	if (ret != kNoErr)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_dvp_camera_state(DVP_STATE_ENABLED);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void dvp_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (DVP_STATE_DISABLED == get_dvp_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_dvp_camera_close();

	set_dvp_camera_state(DVP_STATE_DISABLED);

	frame_buffer_enable(false);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void dvp_camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_DVP_OPEN_IND:
			dvp_open_handle((param_pak_t*)param);
			break;
		case EVENT_DVP_CLOSE_IND:
			dvp_close_handle((param_pak_t*)param);
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
