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


#define DVP_TAG "dvp"

#define LOGI(...) BK_LOGI(DVP_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(DVP_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(DVP_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(DVP_TAG, ##__VA_ARGS__)






void dvp_camera_event_handle(uint32_t event, uint32_t param)
{
	switch (event)
	{
		case EVENT_DVP_OPEN:


			break;
	}
}
