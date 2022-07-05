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


#define MJ_TAG "media0"

#define LOGI(...) BK_LOGI(MJ_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(MJ_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(MJ_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(MJ_TAG, ##__VA_ARGS__)

static beken_thread_t media_major_th_hd = NULL;
static beken_queue_t media_major_msg_queue = NULL;



__attribute__((unused)) static bk_err_t media_major_send_msg(media_msg_t msg)
{
	bk_err_t ret;

	if (media_major_msg_queue)
	{
		ret = rtos_push_to_queue(&media_major_msg_queue, &msg, BEKEN_NO_WAIT);

		if (kNoErr != ret)
		{
			LOGE("%s failed\n", __func__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}



static void media_major_message_handle(void)
{
	bk_err_t ret = BK_OK;
	media_msg_t msg;

	while (1)
	{

		ret = rtos_pop_from_queue(&media_major_msg_queue, &msg, BEKEN_WAIT_FOREVER);

		if (kNoErr == ret)
		{
			switch (msg.event >> MEDIA_EVT_BIT)
			{
				case DVP_EVENT:
					dvp_camera_event_handle(msg.event, msg.param);
					break;

				default:
					break;
			}
		}
	}

#if 0
com_thread_exit:

	/* delate msg queue */
	ret = rtos_deinit_queue(&media_major_msg_queue);

	if (ret != kNoErr)
	{
		LOGE("delate message queue fail\n");
	}

	media_major_msg_queue = NULL;

	LOGE("delate message queue complete\n");

	/* delate task */
	rtos_delete_thread(NULL);

	media_major_th_hd = NULL;

	LOGE("delate task complete\n");
#endif
}


bk_err_t media_major_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_major_msg_queue != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_major_msg_queue allready init, exit!\n");
		goto error;
	}

	if (media_major_th_hd != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_major_th_hd allready init, exit!\n");
		goto error;
	}

	ret = rtos_init_queue(&media_major_msg_queue,
	                      "media_major_queue",
	                      sizeof(media_msg_t),
	                      MEDIA_MAJOR_MSG_QUEUE_SIZE);

	if (ret != kNoErr)
	{
		LOGE("%s, ceate media major message queue failed\n");
		goto error;
	}

	ret = rtos_create_thread(&media_major_th_hd,
	                         BEKEN_DEFAULT_WORKER_PRIORITY,
	                         "media_major_thread",
	                         (beken_thread_function_t)media_major_message_handle,
	                         4096,
	                         NULL);

	if (ret != kNoErr)
	{
		LOGE("create media major thread fail\n");
		goto error;
	}

	LOGI("media major thread startup complete\n");

	return kNoErr;
error:

	if (media_major_msg_queue)
	{
		rtos_deinit_queue(&media_major_msg_queue);
		media_major_msg_queue = NULL;
	}

	return ret;
}
