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
#include <components/video_transfer.h>

#include "media_core.h"
#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "lcd_act.h"
#include "trs_act.h"
#include "trs_task.h"
#include "dvp_api.h"

#define TAG "transfer"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

trs_info_t trs_info;

frame_buffer_t *lock_frame = NULL;

uint8_t lcd_frame_req = false;

uint8_t lock_frame_count = 0;


void video_transfer_open_handle(video_setup_t *setup_cfg)
{
	LOGI("%s\n", __func__);

	trs_task_start(setup_cfg);

	set_trs_video_transfer_state(TRS_STATE_ENABLED);
}

void video_transfer_close_handle(void)
{
	LOGI("%s\n", __func__);

	trs_task_stop();

	set_trs_video_transfer_state(TRS_STATE_DISABLED);
}


void video_transfer_frame_complete_handle(frame_buffer_t *buffer)
{
	lock_frame_count++;

	lock_frame = buffer;

	trs_task_send_msg(TRS_TRANSFER_DATA, (uint32_t)buffer);

#ifdef CONFIG_LCD

	if (true == lcd_frame_req)
	{
		lock_frame_count++;
		lcd_frame_complete_notify(buffer);
	}

#endif
}

void video_transfer_frame_free_handle(frame_buffer_t *buffer)
{
	if (buffer != lock_frame)
	{
		LOGE("%s invalid frame\n", __func__);
	}

	lock_frame_count--;

	if (lock_frame_count < 0)
	{
		LOGE("%s invalid frame free\n", __func__);
		lock_frame_count = 0;
	}


	if (lock_frame_count == 0)
	{
		LOGD("%s free: %p\n", __func__, lock_frame);
		lock_frame->state = STATE_FRAMED;
		lock_frame = NULL;
	}

}

void video_transfer_frame_register_handle(transfer_module_t module)
{
	if (module == MODULE_LCD)
	{
		lcd_frame_req = true;
	}
}

void video_transfer_frame_register(transfer_module_t module)
{
	media_msg_t msg;

	msg.event = EVENT_TRS_FRAME_REGISTER_IND;
	msg.param = module;

	media_send_msg(&msg);
}

void wifi_transfer_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_TRS_VIDEO_TRANSFER_OPEN_IND:
			video_transfer_open_handle((video_setup_t *)param);
			os_free((video_setup_t *)param);
			break;
		case EVENT_TRS_VIDEO_TRANSFER_CLOSE_IND:
			video_transfer_close_handle();
			break;
		case EVENT_TRS_FRAME_COMPLETE_IND:
			video_transfer_frame_complete_handle((frame_buffer_t *)param);
			break;
		case EVENT_TRS_FRAME_FREE_IND:
			video_transfer_frame_free_handle((frame_buffer_t *)param);
			break;
		case EVENT_TRS_FRAME_REGISTER_IND:
			video_transfer_frame_register_handle((transfer_module_t)param);
			break;
	}
}


frame_buffer_t *get_trs_transfer_lock_frame(void)
{
	return lock_frame;
}

void set_trs_transfer_lock_frame(frame_buffer_t *frame)
{
	lock_frame = frame;
}

trs_state_t get_trs_video_transfer_state(void)
{
	return trs_info.state;
}

void set_trs_video_transfer_state(trs_state_t state)
{
	trs_info.state = state;
}

void trs_video_transfer_init(void)
{
	trs_info.state = TRS_STATE_DISABLED;
}
