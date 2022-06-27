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

#pragma once

#include "video_co_list.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_video_transfer_cpu0_init(video_setup_t *setup_cfg);

bk_err_t bk_video_transfer_cpu0_deinit(void);

void video_transfer_set_camera_config(uint32_t resolution, uint32_t frame_rate, uint32_t dev_id);

void video_transfer_cpu0_set_save_image_enable(uint8_t enable);

#ifdef __cplusplus
}
#endif

