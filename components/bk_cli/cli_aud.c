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

#include "cli.h"

#include <components/video_transfer.h>

static beken_thread_t  audio_thread_hdl = NULL;
static beken_thread_t  video_thread_hdl = NULL;

static bool audio_test_flag = false;
static bool video_test_flag = false;




static void audio_main(void)
{
	audio_test_flag = true;
	int length = 40 * 1024;
	uint8_t * test_buf = os_malloc(length);
	if (test_buf == NULL) {
		os_printf("malloc failed\r\n");
		goto audio_exit;
	}

	int frame_len = 0;
	int status_ret = 0;

	while (1) {
		frame_len  = bk_video_buffer_read_frame(test_buf, length, &status_ret, 4000);
		os_printf("frame_len:%d, status_ret:%d\r\n", frame_len, status_ret);
		if (!audio_test_flag)
			goto audio_exit;
	}

audio_exit:

	if (test_buf != NULL)
		os_free(test_buf);

	bk_video_buffer_close();

	/* delate task */
	audio_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t bk_audio_init()
{
	bk_err_t ret = BK_OK;
	if (!audio_thread_hdl) {

		//creat audio record task
		ret = rtos_create_thread(&audio_thread_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "audio_intf",
							 (beken_thread_function_t)audio_main,
							 4096,
							 NULL);
		if (ret != kNoErr) {
			os_printf("create audio task fail \r\n");
			audio_thread_hdl = NULL;
		}
		os_printf("create audio task complete \r\n");

		return kNoErr;
	} else
		return kInProgressErr;
}

void bk_audio_deinit(void)
{
	audio_test_flag = false;
}


static void video_main(void)
{
	video_test_flag = true;
	int ret = 0;
	ret = bk_video_buffer_open();
	os_printf("init: %d\r\n", ret);

	while(1) {
		rtos_delay_milliseconds(1000);
		if (!video_test_flag)
			goto video_exit;
	}

video_exit:
		/* delate task */
		video_thread_hdl = NULL;
		rtos_delete_thread(NULL);
}

bk_err_t bk_video_init()
{
	bk_err_t ret = BK_OK;
	if (!video_thread_hdl) {

		//creat video task
		ret = rtos_create_thread(&video_thread_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "video_intf",
							 (beken_thread_function_t)video_main,
							 4096,
							 NULL);
		if (ret != kNoErr) {
			os_printf("create video task fail \r\n");
			audio_thread_hdl = NULL;
		}
		os_printf("create video task complete \r\n");

		return kNoErr;
	} else
		return kInProgressErr;
}


void bk_video_deinit(void)
{
	video_test_flag = false;
}


static void cli_media_transfer_help(void)
{
	os_printf("media_transfer_test {start|stop} {video|audio}\r\n");
}

void cli_media_transfer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc > 3) {
		cli_media_transfer_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("cp0: start media transfer demo test \r\n");

		/*  -------------------------step1: init audio and config ADC -------------------------------- */
		if (os_strcmp(argv[2], "video") == 0) {
			/* init video task */
			ret = bk_video_init();
			if (ret != BK_OK) {
				os_printf("cp0: init video task fail \r\n");
				return;
			}
			os_printf("init video task complete \r\n");
		} else if (os_strcmp(argv[2], "audio") == 0) {
			/* init audio task */
			ret = bk_audio_init();
			if (ret != BK_OK) {
				os_printf("cp0: init audio task fail \r\n");
				return;
			}
		} else {
			return;
		}

		os_printf("cp0: start media transfer demo test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		bk_audio_deinit();

		bk_video_deinit();

		os_printf("stop media transfer demo test successful \r\n");
	} else {
		cli_media_transfer_help();
		return;
	}

}

extern void cli_aud_adc_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_dtmf_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_adc_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_adc_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_dtmf_loop_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_pcm_mcp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_pcm_dma_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_enable_adc_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_eq_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_aud_mic_to_pcm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#define AUD_CMD_CNT (sizeof(s_aud_commands) / sizeof(struct cli_command))
static const struct cli_command s_aud_commands[] = {
	{"aud_adc_mcp_test", "aud_adc_mcp_test {start|stop}", cli_aud_adc_mcp_test_cmd},
	{"aud_dtmf_mcp_test", "aud_dtmf_mcp_test {start|stop}", cli_aud_dtmf_mcp_test_cmd},
	{"aud_adc_dma_test", "aud_adc_dma_test {start|stop}", cli_aud_adc_dma_test_cmd},
	{"aud_adc_loop_test", "aud_adc_loop_test {start|stop}", cli_aud_adc_loop_test_cmd},
	{"aud_dtmf_loop_test", "aud_dtmf_loop_test {start|stop}", cli_aud_dtmf_loop_test_cmd},
	{"aud_pcm_mcp_test", "aud_pcm_mcp_test {8000|16000|44100|48000}", cli_aud_pcm_mcp_test_cmd},
	{"aud_pcm_dma_test", "aud_pcm_dma_test {8000|16000|44100|48000|stop}", cli_aud_pcm_dma_test_cmd},
//	{"aud_file_to_dac_test", "aud_file_to_dac_test {start|stop} {filename}", cli_aud_file_to_dac_test_cmd},
	{"aud_enable_adc_test", "aud_enable_adc_test {start|stop}", cli_aud_enable_adc_test_cmd},
	{"aud_eq_test", "aud_eq_test {start|stop}", cli_aud_eq_test_cmd},
	{"aud_mic_to_pcm_test", "aud_mic_to_pcm_test {start|stop}", cli_aud_mic_to_pcm_cmd},
	{"media_transfer_test", "media_transfer_test {start|stop}", cli_media_transfer_cmd},
};

int cli_aud_init(void)
{
	return cli_register_commands(s_aud_commands, AUD_CMD_CNT);
}

