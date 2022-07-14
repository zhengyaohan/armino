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

#include <components/video_transfer.h>

#include "media_core.h"
#include "aud_act.h"
#include "adc_ccb.h"
#include "dac_ccb.h"
#include "trs_api.h"
#include "trs_act.h"


#define TAG "trs_api"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


void video_transfer_open(video_setup_t *setup_cfg)
{
	media_msg_t msg;
	video_setup_t *ptr = NULL;

	LOGI("%s\n", __func__);

	if (TRS_STATE_DISABLED != get_trs_video_transfer_state())
	{
		LOGI("%s already opened\n", __func__);
		return;
	}

	ptr = (video_setup_t *)os_malloc(sizeof(video_setup_t));
	os_memcpy(ptr, setup_cfg, sizeof(video_setup_t));

	msg.event = EVENT_TRS_VIDEO_TRANSFER_OPEN_IND;
	msg.param = (uint32_t)ptr;

	media_send_msg(&msg);
}

void video_transfer_close(void)
{
	media_msg_t msg;
	if (TRS_STATE_DISABLED == get_trs_video_transfer_state())
	{
		LOGI("%s already close\n", __func__);
		return;
	}

	msg.event = EVENT_TRS_VIDEO_TRANSFER_CLOSE_IND;
	msg.param = 0;

	media_send_msg(&msg);
}

