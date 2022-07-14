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

#include "media_core.h"
#include "dvp_act.h"
#include "dvp_api.h"

#define TAG "dvp_api"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)



void dvp_open(const dvp_camera_config_t *config)
{
	media_msg_t msg;

	LOGI("%s\n", __func__);

	if (DVP_STATE_DISABLED != get_dvp_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		return;
	}

	msg.event = EVENT_DVP_OPEN_IND;
	msg.param = (uint32_t)config;

	media_send_msg(&msg);
}

void dvp_close(void)
{
	media_msg_t msg;

	if (DVP_STATE_DISABLED == get_dvp_camera_state())
	{
		LOGI("%s already close\n", __func__);
		return;
	}

	msg.event = EVENT_DVP_CLOSE_IND;
	msg.param = 0;

	media_send_msg(&msg);
}

