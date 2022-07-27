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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	APP_CAMERA_DVP,
	APP_CAMERA_UVC,
} app_camera_type_t;

typedef enum
{
	APP_LCD_RGB,
	APP_LCD_MCU,
} app_lcd_type_t;

bk_err_t media_app_camera_open(app_camera_type_t type);
bk_err_t media_app_camera_close(app_camera_type_t type);
bk_err_t media_app_transfer_video_open(void *setup_cfg);
bk_err_t media_app_transfer_video_close(void);
bk_err_t media_app_lcd_open(void);
bk_err_t media_app_lcd_close(void);
bk_err_t media_app_capture(char *name);


#ifdef __cplusplus
}
#endif
