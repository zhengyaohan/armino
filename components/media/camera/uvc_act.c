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

#include <os/mem.h>

#include "camera.h"
#include "camera_act.h"
#include "media_evt.h"

#include "frame_buffer.h"


#define TAG "uvc_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uvc_info_t uvc_info;

void uvc_open_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (UVC_STATE_DISABLED != get_uvc_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	frame_buffer_enable(true);

	ret = bk_uvc_camera_open();

	if (ret != kNoErr)
	{
		LOGE("%s open failed\n", __func__);
		goto out;
	}

	set_uvc_camera_state(UVC_STATE_ENABLED);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void uvc_start_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (UVC_STATE_STOP != get_uvc_camera_state())
	{
		LOGI("%s not opened\n", __func__);
		ret = kNoErr;
		goto out;
	}

	ret = bk_uvc_camera_start();

	if (ret != kNoErr)
	{
		LOGE("%s start failed\n", __func__);
		goto out;
	}

	set_uvc_camera_state(UVC_STATE_ENABLED);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void uvc_stop_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (UVC_STATE_ENABLED != get_uvc_camera_state())
	{
		LOGI("%s not start\n", __func__);
		ret = kNoErr;
		goto out;
	}

	ret = bk_uvc_camera_stop();

	if (ret != kNoErr)
	{
		LOGE("%s stop failed\n", __func__);
		goto out;
	}

	set_uvc_camera_state(UVC_STATE_STOP);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void uvc_param_set_handle(param_pak_t *param)
{
	int ret = 0;
	LOGI("%s\n", __func__);

	if (UVC_STATE_STOP != get_uvc_camera_state())
	{
		LOGI("%s not stop\n", __func__);
		ret = kNoErr;
		goto out;
	}

	ret = bk_uvc_camera_param_set(param);

	if (ret != kNoErr)
	{
		LOGE("%s set param failed\n", __func__);
		goto out;
	}

	set_uvc_camera_state(UVC_STATE_STOP);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void uvc_close_handle(param_pak_t *param)
{
	int ret = 0;

	LOGI("%s\n", __func__);

	if (UVC_STATE_DISABLED == get_uvc_camera_state())
	{
		LOGI("%s already close\n", __func__);
		ret = kNoErr;
		goto out;
	}

	bk_uvc_camera_close();

	set_uvc_camera_state(DVP_STATE_DISABLED);

	frame_buffer_enable(false);

out:
	MEDIA_EVT_RETURN(param, ret);
}

void uvc_camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_UVC_OPEN_IND:
			uvc_open_handle((param_pak_t *)param);
			break;
		case EVENT_UVC_START_IND:
			uvc_start_handle((param_pak_t *)param);
			break;
		case EVENT_UVC_STOP_IND:
			uvc_stop_handle((param_pak_t *)param);
			break;
		case EVENT_UVC_PARAM_IND:
			uvc_param_set_handle((param_pak_t *)param);
			break;
		case EVENT_UVC_CLOSE_IND:
			uvc_close_handle((param_pak_t *)param);
			break;
	}
}

uvc_state_t get_uvc_camera_state(void)
{
	return uvc_info.state;
}

void set_uvc_camera_state(uvc_state_t state)
{
	uvc_info.state = state;
}

void uvc_camera_init(void)
{
	uvc_info.state = UVC_STATE_DISABLED;
}


