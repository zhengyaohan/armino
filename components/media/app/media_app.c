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
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_include.h>
#include <driver/int.h>
#include <driver/dvp_camera_types.h>

#include <components/video_transfer.h>


#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "transfer_act.h"
#include "camera_act.h"
#include "lcd_act.h"

#define TAG "media_app"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static beken_thread_t media_app_th_hd = NULL;
static beken_queue_t media_app_msg_queue = NULL;

bk_err_t media_send_msg_sync(uint32_t event, uint32_t param)
{
	int ret = kGeneralErr;
	param_pak_t *param_pak = (param_pak_t *) os_malloc(sizeof(param_pak_t));
	media_msg_t msg;

	if (param_pak == NULL)
	{
		LOGE("%s malloc param_pak_t failed\n", __func__);
		goto out;
	}

	ret = rtos_init_semaphore_ex(&param_pak->sem, 1, 0);

	if (ret != kNoErr)
	{
		LOGE("%s init semaphore failed\n", __func__);
		goto out;
	}

	param_pak->param = param;

	msg.event = event;
	msg.param = (uint32_t)param_pak;

	media_send_msg(&msg);

	ret = rtos_get_semaphore(&param_pak->sem, BEKEN_WAIT_FOREVER);

	if (ret != kNoErr)
	{
		LOGE("%s wait semaphore failed\n", __func__);
		goto out;
	}

	ret = param_pak->ret;

	rtos_deinit_semaphore(&param_pak->sem);

out:

	if (param_pak)
	{
		os_free(param_pak);
	}

	return ret;
}


bk_err_t media_app_camera_open(app_camera_type_t type)
{
	int ret = kGeneralErr;

	LOGI("%s\n", __func__);

	if (type == APP_CAMERA_DVP)
	{
		if (DVP_STATE_DISABLED != get_dvp_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_DVP_OPEN_IND, DVP_MODE_JPG);
	}
	else if (type == APP_CAMERA_YUV)
	{
		if (DVP_STATE_DISABLED != get_dvp_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_DVP_OPEN_IND, DVP_MODE_YUV);
	}
	else if (type == APP_CAMERA_UVC)
	{

#if (CONFIG_USB_UVC)
		if (UVC_STATE_DISABLED != get_uvc_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_UVC_OPEN_IND, 0);
#endif
	}

	return ret;
}

bk_err_t media_app_camera_close(app_camera_type_t type)
{
	int ret = kGeneralErr;

	LOGI("%s\n", __func__);

	if (type == APP_CAMERA_DVP)
	{
		if (DVP_STATE_ENABLED != get_dvp_camera_state())
		{
			LOGI("%s already closed\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_DVP_CLOSE_IND, DVP_MODE_JPG);
	}
	else if (type == APP_CAMERA_YUV)
	{
		if (DVP_STATE_ENABLED != get_dvp_camera_state())
		{
			LOGI("%s already closed\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_DVP_CLOSE_IND, DVP_MODE_YUV);
	}
	else if (type == APP_CAMERA_UVC)
	{


#if (CONFIG_USB_UVC)

		if (UVC_STATE_DISABLED == get_uvc_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_UVC_CLOSE_IND, 0);
#endif
	}

	return ret;
}

bk_err_t media_app_transfer_video_open(void *setup_cfg)
{
	int ret = kNoErr;
	video_setup_t *ptr = NULL;

	LOGI("%s, %p\n", __func__, ((video_setup_t *)setup_cfg)->send_func);

	if (TRS_STATE_DISABLED != get_trs_video_transfer_state())
	{
		LOGI("%s already opened\n", __func__);
		return ret;
	}

	ptr = (video_setup_t *)os_malloc(sizeof(video_setup_t));
	os_memcpy(ptr, (video_setup_t *)setup_cfg, sizeof(video_setup_t));

	ret = media_send_msg_sync(EVENT_TRS_VIDEO_TRANSFER_OPEN_IND, (uint32_t)ptr);

	os_free(ptr);

	return ret;
}


bk_err_t media_app_transfer_video_close(void)
{
	if (TRS_STATE_ENABLED != get_trs_video_transfer_state())
	{
		LOGI("%s already closed\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_TRS_VIDEO_TRANSFER_CLOSE_IND, 0);
}


bk_err_t media_app_lcd_open(uint32_t lcd_ppi)
{
	if (LCD_STATE_DISABLED != get_lcd_state())
	{
		LOGI("%s already opened\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_LCD_OPEN_IND, lcd_ppi);
}

bk_err_t media_app_lcd_close(void)
{
	if (LCD_STATE_ENABLED != get_lcd_state())
	{
		LOGI("%s already closed\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_LCD_CLOSE_IND, 0);
}

bk_err_t media_app_uvc_start(void)
{
	if (UVC_STATE_STOP != get_uvc_camera_state())
	{
		LOGI("%s not opened\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_UVC_START_IND, 0);
}

bk_err_t media_app_uvc_stop(void)
{
	if (UVC_STATE_ENABLED != get_uvc_camera_state())
	{
		LOGI("%s not start\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_UVC_STOP_IND, 0);
}

bk_err_t media_app_uvc_param_set(uvc_camera_device_t *param)
{
	if (UVC_STATE_STOP != get_uvc_camera_state())
	{
		LOGI("%s not start\n", __func__);
		return kNoErr;
	}

	return media_send_msg_sync(EVENT_UVC_PARAM_IND, (uint32_t)param);
}


bk_err_t media_app_send_msg(media_msg_t *msg)
{
	bk_err_t ret;

	if (media_app_msg_queue)
	{
		ret = rtos_push_to_queue(&media_app_msg_queue, msg, BEKEN_NO_WAIT);

		if (kNoErr != ret)
		{
			LOGE("%s failed\n", __func__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

bk_err_t media_app_capture(char *name)
{
	int ret;
	char *capture_name = NULL;

	LOGI("%s, %s\n", __func__, name);

	if (name != NULL)
	{
		uint32_t len = os_strlen(name);

		if (len > 31)
		{
			len = 31;
		}

		capture_name = (char *)os_malloc(len);
		os_memset(capture_name, 0, len);
		os_memcpy(capture_name, name, len);
	}

	ret = media_send_msg_sync(EVENT_STORAGE_CAPTURE_IND, (uint32_t)capture_name);
	os_free(capture_name);

	return ret;
}


static void media_app_message_handle(void)
{
	bk_err_t ret = BK_OK;
	media_msg_t msg;

	while (1)
	{
		ret = rtos_pop_from_queue(&media_app_msg_queue, &msg, BEKEN_WAIT_FOREVER);

		if (kNoErr == ret)
		{
			switch (msg.event)
			{
				case EVENT_APP_DVP_OPEN:
					break;

				case EVENT_APP_EXIT:
					goto exit;
					break;

				default:
					break;
			}
		}
	}

exit:

	/* delate msg queue */
	ret = rtos_deinit_queue(&media_app_msg_queue);

	if (ret != kNoErr)
	{
		LOGE("delate message queue fail\n");
	}

	media_app_msg_queue = NULL;

	LOGE("delate message queue complete\n");

	/* delate task */
	media_app_th_hd = NULL;
	rtos_delete_thread(NULL);

	LOGE("delate task complete\n");
}


bk_err_t media_app_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_app_msg_queue != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_app_msg_queue allready init, exit!\n");
		goto error;
	}

	if (media_app_th_hd != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_app_th_hd allready init, exit!\n");
		goto error;
	}

	ret = rtos_init_queue(&media_app_msg_queue,
	                      "media_app_msg_queue",
	                      sizeof(media_msg_t),
	                      MEDIA_MINOR_MSG_QUEUE_SIZE);

	if (ret != kNoErr)
	{
		LOGE("%s, ceate media minor message queue failed\n");
		goto error;
	}

	ret = rtos_create_thread(&media_app_th_hd,
	                         BEKEN_DEFAULT_WORKER_PRIORITY,
	                         "media_app_thread",
	                         (beken_thread_function_t)media_app_message_handle,
	                         4096,
	                         NULL);

	if (ret != kNoErr)
	{
		LOGE("create media app thread fail\n");
		goto error;
	}


	LOGI("media minor thread startup complete\n");

	return kNoErr;
error:

	if (media_app_msg_queue)
	{
		rtos_deinit_queue(&media_app_msg_queue);
		media_app_msg_queue = NULL;
	}

	return ret;
}
