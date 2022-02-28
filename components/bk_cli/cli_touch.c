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


#include "rtos_pub.h"
#include "cli.h"
#include "bk_api_touch.h"
#include "bk_api_touch_type.h"
#include "sys_driver.h"
#include "touch_driver.h"
#include "aon_pmu_driver.h"


extern void delay(int num);

static void cli_touch_help(void)
{
	CLI_LOGI("touch_single_channel_calib_mode_test {0|1|2|...|15}\r\n");
	CLI_LOGI("touch_single_channel_manul_mode_test {calibration_value}\r\n");
	CLI_LOGI("touch_multi_channel_scan_mode_test {multi_channel_value} {start|stop}\r\n");
}

static void cli_touch_isr(void *param)
{
	uint32_t int_status = 0;
	int_status = bk_touch_get_int_status();
	CLI_LOGI("interrupt status = %x\r\n", int_status);
}

static void cli_touch_single_channel_calib_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t touch_id = 0;
	touch_config_t touch_config;

	if (argc != 2) {
		cli_touch_help();
		return;
	}

	touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		bk_touch_gpio_init(touch_id);
		bk_touch_enable(touch_id);
		bk_touch_register_touch_isr(touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
		touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
		bk_touch_config(&touch_config);

		bk_touch_scan_mode_enable(0);
		bk_touch_calib_enable(0);
		delay(100);
		bk_touch_calib_enable(1);
		delay(100);
		bk_touch_int_enable(touch_id, 1);
	} else {
		CLI_LOGI("unsupported touch channel selection command!\r\n");
	}

	if (os_strcmp(argv[1], "STOP") == 0) {
		CLI_LOGI("single channel calib test stop!\r\n");
		bk_touch_disable();
	}
}

static void cli_touch_single_channel_manul_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t calib_value = 0;
	uint32_t cap_out = 0;
	uint32_t touch_id = 0;
	touch_config_t touch_config;

	if (argc != 3) {
		cli_touch_help();
		return;
	} 

	touch_id = os_strtoul(argv[1], NULL, 16) & 0xFF;
	if(touch_id >= 0 && touch_id < 16) {
		bk_touch_gpio_init(touch_id);
		bk_touch_enable(touch_id);
		bk_touch_register_touch_isr(touch_id, cli_touch_isr, NULL);

		touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_5;
		touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
		bk_touch_config(&touch_config);
		bk_touch_scan_mode_enable(0);

		calib_value = os_strtoul(argv[2], NULL, 16) & 0xFFF;
		CLI_LOGI("calib_value = %x\r\n", calib_value);
		bk_touch_manul_mode_enable(calib_value);
		delay(100);
		bk_touch_int_enable(touch_id, 1);
		cap_out = bk_touch_get_calib_value();
		CLI_LOGI("cap_out = %x\r\n", cap_out);
		if(calib_value == cap_out) {
			CLI_LOGI("single channel manul mode test is successful!\r\n");
		} else {
			CLI_LOGI("single channel manul mode test is failed!\r\n");
		}
	} else {
		CLI_LOGI("unsupported touch channel selection command!\r\n");
	}
	

}

static void cli_touch_multi_channel_scan_mode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t multi_chann_value = 0;

	if (argc != 3) {
		cli_touch_help();
		return;
	} 

	multi_chann_value = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
	CLI_LOGI("multi_channel_value = %x\r\n", multi_chann_value);

	if (os_strcmp(argv[2], "START") == 0) {
		CLI_LOGI("multi_channel_scan_mode_test start!\r\n");
		bk_touch_scan_mode_mult_channl_set(multi_chann_value);
		bk_touch_scan_mode_enable(1);
	} else if (os_strcmp(argv[2], "STOP") == 0) {
		CLI_LOGI("multi_channel_scan_mode_test stop!\r\n");
		bk_touch_scan_mode_enable(0);
		bk_touch_disable();
	}
}


#define TOUCH_CMD_CNT	(sizeof(s_touch_commands) / sizeof(struct cli_command))

static const struct cli_command s_touch_commands[] = {
	{"touch_single_channel_calib_mode_test", "touch_single_channel_calib_mode_test {0|1|...|15}", cli_touch_single_channel_calib_mode_test_cmd},
	{"touch_single_channel_manul_mode_test", "touch_single_channel_manul_mode_test {calibration_value}", cli_touch_single_channel_manul_mode_test_cmd},
	{"touch_multi_channel_scan_mode_test", "touch_multi_channel_scan_mode_test {multi_channel_value} {start|stop}", cli_touch_multi_channel_scan_mode_test_cmd},
};

int cli_touch_init(void)
{
	return cli_register_commands(s_touch_commands, TOUCH_CMD_CNT);
}


