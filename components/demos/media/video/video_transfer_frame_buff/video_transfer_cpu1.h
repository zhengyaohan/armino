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

#include <common/bk_include.h>
#include "video_mailbox.h"
#include "video_transfer_common.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_video_transfer_cpu1_init(video_transfer_setup_t *setup_cfg);

bk_err_t video_transfer_frame_buff_init(void);

void video_transfer_buff_deinit(void);

void video_transfer_set_buff_frame_len(uint8_t id, uint32_t frame_len, uint32_t frame_id);

void video_transfer_set_buff_alread_copy_len(uint8_t id, uint32_t copy_len);

void video_transfer_set_buff_state(uint8_t id, frame_buff_state_t state);

bk_err_t video_transfer_get_idle_buff(uint8_t id, frame_information_t *info);

bk_err_t video_transfer_get_ready_buff(frame_information_t *info);

void video_transfer_get_buff(uint8_t id, frame_information_t *info);

#ifdef __cplusplus
}
#endif
