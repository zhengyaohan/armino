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

#ifdef __cplusplus
extern "C" {
#endif

void camera_flip(uint8_t n);
void camera_intfer_init(void *data);
void camera_intfer_deinit(void);
void camera_intf_config_senser(void);
uint32_t camera_intfer_set_video_param(uint32_t ppi_type, uint32_t pfs_type);
int camera_set_ppi_fps(uint32_t ppi_type, uint32_t fps_type);
int camera_intfer_set_yuv_mode(uint8_t mode);
#ifdef __cplusplus
}
#endif

