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
#include "cli.h"
#include "media_cli.h"
#include "aud_api.h"

#include "media_cli_comm.h"
#include "media_app.h"

#define TAG "mcli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)



void media_cli_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	LOGI("%\n", __func__);

	if (argc > 0)
	{
		if (os_strcmp(argv[1], "dvp") == 0)
		{
#if (defined(CONFIG_CAMERA) && !defined(CONFIG_SLAVE_CORE))
			if (os_strcmp(argv[2], "open") == 0)
			{
				media_app_camera_open(APP_CAMERA_DVP);
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				media_app_camera_close(APP_CAMERA_DVP);
			}
#endif
		}

#ifdef CONFIG_AUDIO
		if (os_strcmp(argv[1], "adc") == 0)
		{
			adc_open();
		}

		if (os_strcmp(argv[1], "dac") == 0)
		{
			dac_open();
		}
#endif

		if (os_strcmp(argv[1], "capture") == 0)
		{
			LOGI("capture\n");
#if defined(CONFIG_CAMERA) && !defined(CONFIG_SLAVE_CORE)

			if (argc >= 3)
			{
				media_app_capture(argv[2]);
			}
			else
			{
				media_app_capture("unknow.jpg");
			}
#endif
		}

		if (os_strcmp(argv[1], "lcd") == 0)
		{
#if defined(CONFIG_LCD) && !defined(CONFIG_SLAVE_CORE)
			if (os_strcmp(argv[2], "open") == 0)
			{
				media_app_lcd_open();
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				media_app_lcd_close();
			}
#endif
		}
	}





}



#define MEDIA_CMD_CNT   (sizeof(s_media_commands) / sizeof(struct cli_command))

static const struct cli_command s_media_commands[] =
{
	{"media", "meida...", media_cli_test_cmd},
};

int media_cli_init(void)
{
	return cli_register_commands(s_media_commands, MEDIA_CMD_CNT);
}
