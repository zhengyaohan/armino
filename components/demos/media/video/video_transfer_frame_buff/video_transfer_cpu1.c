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
#include <driver/i2c.h>
#include <driver/psram.h>
#include <driver/dma.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>
#include <components/dvp_camera.h>
#include <driver/timer.h>

#if CONFIG_GENERAL_DMA
#include "bk_general_dma.h"
#endif

#include "video_transfer_cpu1.h"
#include "video_transfer_camera_process.h"
#include "video_frame_buff.h"

//#include "BK7256_RegList.h"

#define EJPEG_CHECK_HTIMER_CHANNEL     TIMER_ID5
#define EJPEG_CHECK_HTIMER_VAL         1000// 1 second
#define EJPEG_I2C_DEFAULT_BAUD_RATE    I2C_BAUD_RATE_100KHZ
#define EJPEG_DMA_TRANSFER_LEN         0x2800
#define TU_QITEM_COUNT      (60)

static uint8_t  dvp_dma_channel = 0;
static uint8_t  frame_buffer_id = 0;
static uint8_t  eof_time_second = 0;
static uint32_t frame_id = 0;

static beken_thread_t  video_thread_cpu1_hdl = NULL;
static beken_queue_t vid_cpu1_msg_que = NULL;
video_transfer_setup_t video_transfer_setup_bak = {0};
frame_information_t info_cpu1 = {0};

static bk_err_t video_transfer_cpu1_send_msg(uint8_t msg_type, uint32_t data)
{
	bk_err_t ret;
	video_msg_t msg;

	if (vid_cpu1_msg_que) {
		msg.type = msg_type;
		msg.data = data;

		ret = rtos_push_to_queue(&vid_cpu1_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("video_transfer_cpu1_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void video_transfer_jpeg_eof_callback(jpeg_unit_t id, void *param)
{
	//addAON_GPIO_Reg0x8 = 2;
	frame_information_t info = {0};
	eof_time_second++;
	uint32_t left_len = bk_dma_get_remain_len(dvp_dma_channel);
	uint32_t rec_len = EJPEG_DMA_TRANSFER_LEN - left_len;
	uint32_t frame_len = bk_jpeg_enc_get_frame_size();

	// set current frame buff ready(a complete frame data copy finish)
	video_transfer_set_buff_alread_copy_len(frame_buffer_id, rec_len);

	// set current frame buff state
	video_transfer_set_buff_state(frame_buffer_id, BUFF_READY);

	// set current frame buff frame_len and frame_id
	video_transfer_set_buff_frame_len(frame_buffer_id, frame_len, frame_id++);

	bk_dma_stop(dvp_dma_channel);

	// send msg to inform cpu0 have a ready frame to transfer
	video_transfer_cpu1_send_msg(VIDEO_CPU1_EOF, (uint32_t)frame_buffer_id);

	if (frame_buffer_id == 2)
		frame_buffer_id = 0;
	else
		frame_buffer_id++;
	// get idle frame buff addr and buf_len
	bk_err_t ret = video_transfer_get_idle_buff(frame_buffer_id, &info);
	if (ret != BK_OK) {
		//os_printf("frame_buff not in idle\r\n");
		ret = video_transfer_get_ready_buff(&info);
		if (ret != BK_OK) {
			os_printf("frame_buff not in ready\r\n");
			bk_jpeg_enc_set_enable(0);
			video_transfer_cpu1_send_msg(VIDEO_CPU1_EXIT, 0);
			return;
		}
	}

	video_transfer_set_buff_state(info.buffer_id, BUFF_COPY);
	frame_buffer_id = info.buffer_id;
	BK_LOG_ON_ERR(bk_dma_set_dest_addr(dvp_dma_channel, (uint32_t)info.buffer_addr, (uint32_t)(info.buffer_addr + info.buffer_len)));
	bk_dma_start(dvp_dma_channel);
	//addAON_GPIO_Reg0x8 = 0;
}

static void video_transfer_dma_finish_callback(dma_id_t id)
{
//	addAON_GPIO_Reg0x9 = 2;
	video_transfer_set_buff_alread_copy_len(frame_buffer_id, EJPEG_DMA_TRANSFER_LEN);
//	addAON_GPIO_Reg0x9 = 0;
}

static bk_err_t video_transfer_dma_config(void)
{
	bk_err_t ret = BK_OK;
	uint32_t jpeg_fifo_addr;
	dma_config_t dma_config = {0};
	frame_information_t info = {0};
	dvp_dma_channel = bk_dma_alloc(DMA_DEV_JPEG);
	if ((dvp_dma_channel < DMA_ID_0) || (dvp_dma_channel >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		ret = BK_FAIL;
		return ret;
	}

	// get idle frame buff addr and buf_len
	ret = video_transfer_get_idle_buff(frame_buffer_id, &info);
	if (ret != BK_OK) {
		os_printf("frame_buff not in idle\r\n");
		return ret;
	}

	video_transfer_set_buff_state(info.buffer_id, BUFF_COPY);

	frame_buffer_id = info.buffer_id;

	bk_jpeg_enc_get_fifo_addr(&jpeg_fifo_addr);

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_JPEG;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = jpeg_fifo_addr;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = info.buffer_addr;
	dma_config.dst.end_addr = info.buffer_addr + info.buffer_len;

	//os_printf("dst_start_addr:%08x, dst_end_addr:%08x\r\n", (uint32_t)info.buffer_addr, dma_config.dst.end_addr);

	BK_LOG_ON_ERR(bk_dma_init(dvp_dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_dma_channel, EJPEG_DMA_TRANSFER_LEN));
	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_dma_channel, NULL, video_transfer_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_dma_channel));
	BK_LOG_ON_ERR(bk_dma_start(dvp_dma_channel));

	return ret;
}

static void jpeg_eof_checkout_callback(timer_id_t timer_id)
{
	video_transfer_cpu1_send_msg(VIDEO_CPU1_EOF_CHECK, 0);
}

static bk_err_t video_transfer_camera_cpu1_init(void)
{
	bk_err_t ret = BK_OK;
	i2c_config_t i2c_config = {0};
	jpeg_config_t jpeg_config = {0};
	camera_config_t camera_cfg = {0};

	// step 1: init frame_buff
	ret = video_transfer_frame_buff_init();
	if (ret != BK_OK) {
		os_printf("frame buff init failed\r\n");
		return ret;
	}

	// step 2: jpeg enc driver init
	ret = bk_jpeg_enc_driver_init();
	if (ret != BK_OK) {
		os_printf("jpeg encode driver init failed\r\n");
		return ret;
	}

	// step 3: dma_init
	ret = video_transfer_dma_config();
	if (ret != BK_OK) {
		os_printf("dma init failed\r\n");
		return ret;
	}

	// step 4: jpeg enc init
	video_transfer_set_camera_cfg(&video_transfer_setup_bak);

	ret = video_transfer_get_camera_cfg(&camera_cfg);
	if (BK_OK != ret) {
		os_printf("Get camera config failed!\r\n");
		return ret;
	}

	jpeg_config.x_pixel = camera_cfg.x_pixel;
	jpeg_config.y_pixel = camera_cfg.y_pixel;

	if (jpeg_config.y_pixel == Y_PIXEL_720) {
		jpeg_config.sys_clk_div = 3;
		jpeg_config.mclk_div = 2;
	}else {
		jpeg_config.sys_clk_div = 4;
		jpeg_config.mclk_div = 0;
	}

	ret = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (ret != BK_OK) {
		os_printf("jpeg init error\n");
		return ret;
	}

	bk_jpeg_enc_register_isr(END_OF_FRAME, video_transfer_jpeg_eof_callback, NULL);

	// step 5: init i2c
	i2c_config.baud_rate = EJPEG_I2C_DEFAULT_BAUD_RATE;
	i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
	ret = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (ret != kNoErr) {
		os_printf("i2c init error\n");
		return ret;
	}

	bk_camera_sensor_config();

	// step 5: init a timer for 1 second to checkout jpeg eof
	bk_timer_start(EJPEG_CHECK_HTIMER_CHANNEL, EJPEG_CHECK_HTIMER_VAL, jpeg_eof_checkout_callback);

	return BK_OK;
}

static bk_err_t video_transfer_camera_cpu1_deinit(void)
{
	bk_err_t ret = BK_OK;
	bk_timer_stop(EJPEG_CHECK_HTIMER_CHANNEL);

	// setp 1: deinit jpeg
	ret = bk_jpeg_enc_dvp_deinit();
	if (ret != BK_OK) {
		os_printf("Deinit jpeg encode failed!\r\n");
	}

	// step 2: deinit dma for fifo to frame_buff
	ret = bk_dma_stop(dvp_dma_channel);
	if (ret != BK_OK) {
		os_printf("Stop dma %d failed!\r\n", dvp_dma_channel);
	}

	ret = bk_dma_deinit(dvp_dma_channel);
	if (ret != BK_OK) {
		os_printf("Deinit dma %d failed!\r\n", dvp_dma_channel);
	}

	ret = bk_dma_free(DMA_DEV_JPEG, dvp_dma_channel);
	if (ret != BK_OK) {
		os_printf("Free dma %d failed!\r\n", dvp_dma_channel);
	}

	// step 3: deinit I2C
	ret = bk_i2c_deinit(CONFIG_CAMERA_I2C_ID);
	if (ret != BK_OK) {
		os_printf("Deinit I2C %d failed!\r\n", CONFIG_CAMERA_I2C_ID);
	}

	// step 4: deinit jpeg driver, power off
	ret = bk_jpeg_enc_driver_deinit();
	if (ret != BK_OK) {
		os_printf("Deinit jpeg encode driver failed!\r\n");
	}

	// step 5: deinit frame buff
	video_transfer_buff_deinit();

	eof_time_second = 0;
	frame_id = 0;
	frame_buffer_id = 0;
	os_memset(&info_cpu1, 0, sizeof(frame_information_t));

	return ret;
}

static bk_err_t video_transfer_inform_ready()
{
//	addAON_GPIO_Reg0x8 = 2;

	video_mailbox_msg_t mb_msg;
	mb_msg.mb_cmd = VID_MB_FRAME_BUFF_READY_CMD;
	mb_msg.param1 = 0;
	mb_msg.param2 = 0;
	mb_msg.param3 = 0;
//	addAON_GPIO_Reg0x8 = 0;
	return video_mailbox_send_msg(&mb_msg);
}

static bk_err_t video_transfer_send_buff_frame(void)
{
//	addAON_GPIO_Reg0x8 = 2;
	bk_err_t ret = BK_OK;
	video_mailbox_msg_t mb_msg;

	ret = video_transfer_get_ready_buff(&info_cpu1);
	if (ret != BK_OK) {
		os_printf("not ready buff\r\n");
		return ret;
	}

	video_transfer_set_buff_state(info_cpu1.buffer_id, BUFF_BUSY);

	mb_msg.mb_cmd = VID_MB_FRAME_BUFF_RESPONSE_CMD;
	mb_msg.param1 = info_cpu1.buffer_id;
	mb_msg.param2 = info_cpu1.frame_id;
	mb_msg.param3 = info_cpu1.frame_len;
//	addAON_GPIO_Reg0x8 = 0;
	return video_mailbox_send_msg(&mb_msg);
}

static void video_transfer_jpeg_eof_check()
{
	if (eof_time_second < 10) {
		//addAON_GPIO_Reg0x9 = 2;
		// jpeg eof error, reboot jpeg

		// step 1: deinit
		video_transfer_camera_cpu1_deinit();

		// step 2: init
		video_transfer_camera_cpu1_init();
		//addAON_GPIO_Reg0x9 = 0;
	} else {
		eof_time_second = 0;
	}
}

static void video_cp1_mailbox_rx_isr(video_mb_t *vid_mb, mb_chnl_cmd_t *cmd_buf)
{
	/* check mailbox msg and send msg to task */
	switch (cmd_buf->hdr.cmd) {
	case VID_MB_FRAME_BUFF_SET_STATE_CMD:
	{
		info_cpu1.buffer_id = cmd_buf->param1;
		info_cpu1.buffer_state = cmd_buf->param2;

		video_transfer_cpu1_send_msg(VIDEO_CPU1_BUFF_STATE, 0);
		break;
	}

	case VID_MB_FRAME_BUFF_REQUEST_CMD:
	{
		video_transfer_cpu1_send_msg(VIDEO_CPU1_SEND, 0);
		break;
	}

	case VID_MB_CPU1_EXIT_CMD:
	{
		if (video_thread_cpu1_hdl)
			video_transfer_cpu1_send_msg(VIDEO_CPU1_EXIT, 0);
		break;
	}

	default:
		break;
	}
}

static void video_cp1_mailbox_tx_isr(video_mb_t *vid_mb)
{
	//os_printf("enter cp1_mailbox_tx_isr \r\n");
}

static void video_cp1_mailbox_tx_cmpl_isr(video_mb_t *vid_mb, mb_chnl_ack_t *cmd_buf)
{
	//os_printf("enter cp1_mailbox_tx_cmpl \r\n");
}

static void video_transfer_cpu1_main(beken_thread_arg_t data)
{
	bk_err_t ret = BK_OK;
	video_mailbox_msg_t mb_msg;

	// step 1: init psram
	ret = bk_psram_init();
	if (ret != BK_OK) {
		os_printf("psram init error \r\n");
		return;
	}

	// setp 2: init mailbox callback
	video_mailbox_init(video_cp1_mailbox_rx_isr, video_cp1_mailbox_tx_isr, video_cp1_mailbox_tx_cmpl_isr);

	// step 2: dvp camera init
	ret = video_transfer_camera_cpu1_init();
	if (ret != BK_OK) {
		os_printf("camera init failed\r\n");
		return;
	}

	while (1) {
		video_msg_t msg;
		ret = rtos_pop_from_queue(&vid_cpu1_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.type) {
			case VIDEO_CPU1_EOF:
				video_transfer_inform_ready();
				break;

			case VIDEO_CPU1_EOF_CHECK:
				video_transfer_jpeg_eof_check();
				break;

			case VIDEO_CPU1_SEND:
				video_transfer_send_buff_frame();
				break;

			case VIDEO_CPU1_BUFF_STATE:
				video_transfer_set_buff_state(info_cpu1.buffer_id, info_cpu1.buffer_state);
				break;

			case VIDEO_CPU1_EXIT:
				goto exit;
				break;

			default:
				break;
			}
		}
	}

exit:
	os_printf("video_transfer cpu1 exit\r\n");
	mb_msg.mb_cmd = VID_MB_CPU0_EXIT_CMD;
	mb_msg.param1 = 0;
	mb_msg.param2 = 0;
	mb_msg.param3 = 0;
	video_mailbox_send_msg(&mb_msg);

	video_transfer_camera_cpu1_deinit();

	video_mailbox_deinit();

	rtos_deinit_queue(&vid_cpu1_msg_que);
	vid_cpu1_msg_que = NULL;

	video_thread_cpu1_hdl = NULL;
	rtos_delete_thread(NULL);
}

bk_err_t bk_video_transfer_cpu1_init(video_transfer_setup_t *setup_cfg)
{
	bk_err_t ret = BK_OK;
	if ((!video_thread_cpu1_hdl) && (!vid_cpu1_msg_que)) {
		os_printf("cp1: start audio_transfer test \r\n");
		os_memcpy(&video_transfer_setup_bak, setup_cfg, sizeof(video_transfer_setup_t));

		ret = rtos_init_queue(&vid_cpu1_msg_que,
							  "video_transfer_queue_cpu1",
							  sizeof(video_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr) {
			os_printf("cp1: ceate video internal message queue in cpu1 failed \r\n");
			return BK_FAIL;
		}

		//creat video transfer task
		ret = rtos_create_thread(&video_thread_cpu1_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "video_transfer_cpu1",
							 (beken_thread_function_t)video_transfer_cpu1_main,
							 4 * 1024,
							 (beken_thread_arg_t)NULL);
		if (ret != kNoErr) {
			os_printf("cp1: create video transfer task fail \r\n");
			rtos_deinit_queue(&vid_cpu1_msg_que);
			vid_cpu1_msg_que = NULL;
			video_thread_cpu1_hdl = NULL;
		}

		return kNoErr;
	} else
		return kInProgressErr;
}


