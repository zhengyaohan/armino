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
#include <os/mem.h>
#include <os/str.h>
#include "sys_driver.h"
#include "audio_transfer_cp0.h"
#include "ff.h"
#include "diskio.h"

#define  AUDIO_AEC_DUMP_DEBUG  0
#define  AUDIO_TRANSFER_AEC_SOFTWARE_MODE  0


#define READ_SIZE    160
#define WRITE_SIZE    320

#if AUDIO_AEC_DUMP_DEBUG
#define PSRAM_AUDIO_AEC_SIN_DUMP_DEBUG_BASE      PSRAM_AUD_DECD_RING_BUFF_BASE + 1500
#define PSRAM_AUDIO_AEC_REF_DUMP_DEBUG_BASE      PSRAM_AUDIO_AEC_SIN_DUMP_DEBUG_BASE + 600
#endif

FIL encoder_file;
FIL decoder_file;


char encoder_file_name[] = "1:/g711_encoder_data.pcm";    //save mic data received
char decoder_file_name[] = "1:/g711_decoder_data.pcm";    //save speaker data sended
#if AUDIO_AEC_DUMP_DEBUG
static char sin_file_name[] = "1:/sin.pcm";
static char ref_file_name[] = "1:/ref.pcm";
static char unused_file_name[] = "1:/unused.pcm";
FIL sin_file;
FIL ref_file;
FIL unused_file;
#endif

#if AUDIO_TRANSFER_AEC_SOFTWARE_MODE
static bool dac_ready_flag = false;
#endif

static uint8_t *temp_read_buffer = NULL;
static uint8_t *temp_write_buffer = NULL;


extern void delay(int num);

static void cli_aud_cp0_help(void)
{
	os_printf("aud_cp0_audio_transfer_test {start|stop} \r\n");
}

void audio_cp0_read_done_callback(uint8_t *buffer, uint32_t length)
{
	FRESULT fr;
//	uint32_t i = 0;
	uint32 uiTemp = 0;

	//os_printf("audio_cp0_read_done_callback: buffer=0x%p, length=%d \r\n", buffer, length);

	/* write data to file */
	fr = f_write(&encoder_file, (void *)buffer, length, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", encoder_file_name);
		return;
	}
	//os_printf("read done \r\n");

#if AUDIO_AEC_DUMP_DEBUG
	/* write sin data to file */
	fr = f_write(&sin_file, (void *)PSRAM_AUDIO_AEC_SIN_DUMP_DEBUG_BASE, 320, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", sin_file_name);
		return;
	}

	/* write ref data to file */
	fr = f_write(&ref_file, (void *)PSRAM_AUDIO_AEC_REF_DUMP_DEBUG_BASE, 320, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", ref_file_name);
		return;
	}

	/* write unused data to file */
	fr = f_write(&unused_file, (void *)PSRAM_AUDIO_AEC_REF_DUMP_DEBUG_BASE, 320, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", unused_file_name);
		return;
	}

	//os_printf("write done \r\n");
#endif
}

/* get encoder used size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
void audio_cp0_write_done_callback(uint8_t *buffer, uint32_t length)
{
	FRESULT fr;
	uint32 uiTemp = 0;

	/* read encoder data from file */
	fr = f_read(&decoder_file, temp_write_buffer, WRITE_SIZE, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read %s fail.\r\n", decoder_file_name);
		return;
	}

#if AUDIO_TRANSFER_AEC_SOFTWARE_MODE
	bk_err_t ret = BK_OK;
	if (dac_ready_flag) {
		ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
		if (ret != BK_OK)
			os_printf("send write request fail \r\n");
		dac_ready_flag = true;
	}
#endif
	//os_printf("write done \r\n");
}

void audio_cp0_get_write_data(void)
{
	FRESULT fr;
	//FIL file;
	uint32 uiTemp = 0;

	/* read encoder data from file */
	fr = f_read(&decoder_file, temp_write_buffer, WRITE_SIZE, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read %s fail.\r\n", decoder_file_name);
		return;
	}
	//os_printf("read done \r\n");
}

#if AUDIO_TRANSFER_AEC_SOFTWARE_MODE
void audio_cp0_write_ready_data(void)
{
	FRESULT fr;
	uint32 uiTemp = 0;

	/* read encoder data from file */
	fr = f_read(&decoder_file, temp_write_buffer, WRITE_SIZE, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read %s fail.\r\n", decoder_file_name);
		return;
	}
	//os_printf("read done \r\n");
/*
	bk_err_t ret = BK_OK;
	ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
	if (ret != BK_OK)
		os_printf("[audio_cp0_write_ready_data] send write request fail \r\n");
*/
}
#endif

/* get encoder used size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
void audio_cp0_get_encoder_used_size_callback(uint32_t size)
{
	bk_err_t ret = BK_OK;

	if (size >= READ_SIZE) {
		ret = bk_audio_read_req(temp_read_buffer, READ_SIZE);
		if (ret != BK_OK)
			os_printf("send read request fail \r\n");
	}
}

/* get decoder remain size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
void audio_cp0_get_decoder_remain_size_callback(uint32_t size)
{
	bk_err_t ret = BK_OK;
	os_printf("audio_cp0_get_decoder_remain_size_callback, size:%d \r\n", size);

	if (size >= WRITE_SIZE) {
		ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
		if (ret != BK_OK)
			os_printf("send write request fail \r\n");
	}
}

/* receive CPU1 read request, and send get decoder remain size reqest to CPU1 */
void audio_cp0_encoder_read_req_handler(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_audio_read_req(temp_read_buffer, READ_SIZE);
	if (ret != BK_OK)
		os_printf("send write request fail \r\n");
}

/* receive CPU1 write request, and send write decoder data reqest to CPU1 */
void audio_cp0_decoder_write_req_handler(void)
{
	bk_err_t ret = BK_OK;
	os_printf("audio_cp0_decoder_write_req_handler \r\n");

	ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
	if (ret != BK_OK)
		os_printf("send write request fail \r\n");
}

void audio_cp0_write_data(void)
{
	bk_audio_get_decoder_remain_size();
	//os_printf("get decoder remain size \r\n");
}

void cli_cp0_audio_transfer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;

	if (argc != 2) {
		cli_aud_cp0_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("cp0: start audio_transfer test \r\n");
		temp_read_buffer = (uint8_t *)os_malloc(READ_SIZE);
		if (temp_read_buffer == NULL) {
			os_printf("malloc temp_read_buffer fail \r\n");
			return;
		}
		os_printf("malloc temp_read_buffer:%p complete \r\n", temp_read_buffer);
		temp_write_buffer = (uint8_t *)os_malloc(WRITE_SIZE);
		if (temp_write_buffer == NULL) {
			os_printf("malloc temp_write_buffer fail \r\n");
			return;
		}
		os_printf("malloc temp_write_buffer:%p complete \r\n", temp_write_buffer);

		/*open file to save mic data has been encoding by g711a */
		fr = f_open(&encoder_file, encoder_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", encoder_file_name);
			return;
		}

		/*open file to save mic data has been encoding by g711a */
		fr = f_open(&decoder_file, decoder_file_name, FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", decoder_file_name);
			return;
		}

#if AUDIO_AEC_DUMP_DEBUG
		/*open file to save sin data of AEC */
		fr = f_open(&sin_file, sin_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", sin_file_name);
			return;
		}

		/*open file to save ref data of AEC */
		fr = f_open(&ref_file, ref_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", ref_file_name);
			return;
		}

		/*open file to save unused data of AEC */
		fr = f_open(&unused_file, unused_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", unused_file_name);
			return;
		}
#endif

		/* register callbacks */
		bk_audio_register_rw_cb(audio_cp0_read_done_callback,
								audio_cp0_write_done_callback,
								audio_cp0_get_encoder_used_size_callback,
								audio_cp0_get_decoder_remain_size_callback,
								audio_cp0_encoder_read_req_handler,
								audio_cp0_decoder_write_req_handler);
		os_printf("register callbacks complete \r\n");

		/* get one frame data */
		audio_cp0_get_write_data();

		/* init audio cpu0 transfer task */
		ret = bk_audio_cp0_transfer_init(NULL);
		if (ret != BK_OK) {
			os_printf("cp0: init audio transfer task fail \r\n");
			return;
		}

		/* write two frame data */
#if AUDIO_TRANSFER_AEC_SOFTWARE_MODE
		os_printf("cp0: write dac ready data \r\n");
		//delay(1000000);
		audio_cp0_write_ready_data();
		//audio_cp0_write_data();
		os_printf("cp0: write dac ready data complete \r\n");
#else
		//audio_cp0_get_write_data();
#endif

		os_printf("cp0: start audio_transfer test successful\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_audio_cp0_transfer_deinit();
		if (ret != BK_OK) {
			os_printf("cp0: audio transfer deinit fail \r\n");
		}

		/* close encoder file */
		fr = f_close(&encoder_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", encoder_file_name);
			return;
		}

		/* close decoder file */
		fr = f_close(&decoder_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", decoder_file_name);
			return;
		}
		os_printf("cp0: close file complete \r\n");

#if AUDIO_AEC_DUMP_DEBUG
		/* close sin file */
		fr = f_close(&sin_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", sin_file_name);
			return;
		}
		os_printf("cp0: close sin file complete \r\n");

		/* close ref file */
		fr = f_close(&ref_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", ref_file_name);
			return;
		}
		os_printf("cp0: close ref file complete \r\n");

		/* close unused file */
		fr = f_close(&unused_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", unused_file_name);
			return;
		}
#endif

		/* free memory */
		os_free(temp_read_buffer);
		os_free(temp_write_buffer);

		os_printf("cp0: stop audio_transfer test successful \r\n");
	} else {
		cli_aud_cp0_help();
		return;
	}

}
