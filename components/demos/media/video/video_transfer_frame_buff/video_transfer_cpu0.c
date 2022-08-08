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
#include "stdio.h"
#include "mailbox_channel.h"
#include "sys_driver.h"
#include "video_mailbox.h"
#include "media_common.h"
#include <components/video_types.h>
#include "video_transfer_common.h"

#if CONFIG_GENERAL_DMA
#include "bk_general_dma.h"
#endif

#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"
#endif


#define TU_QITEM_COUNT      (60)

extern void delay(int num);

static beken_thread_t  video_thread_cpu0_hdl = NULL;
static beken_queue_t vid_cpu0_msg_que = NULL;
video_setup_t video_transfer_setup = {0};
static frame_information_t info_cpu0 = {0};
static uint32_t video_rx_node_len = 0;
static uint8_t image_save_cpu0_enable = 0;
video_transfer_setup_t video_cfg = {3, 20, (640 << 16) | 480};


bk_err_t video_transfer_cpu0_send_msg(uint8_t msg_type, uint32_t data)
{
	bk_err_t ret;
	video_msg_t msg;

	if (vid_cpu0_msg_que) {
		msg.type = msg_type;
		msg.data = data;

		ret = rtos_push_to_queue(&vid_cpu0_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("video_transfer_cpu1_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t video_transfer_set_frame_buff_state(void)
{
	video_mailbox_msg_t mb_msg;

	mb_msg.mb_cmd = VID_MB_FRAME_BUFF_SET_STATE_CMD;

	mb_msg.param1 = (uint32_t)info_cpu0.buffer_id;
	mb_msg.param2 = (uint32_t)info_cpu0.buffer_state;
	mb_msg.param3 = 0;
	
	return video_mailbox_send_msg(&mb_msg);
}

static void video_cp0_mailbox_rx_isr(video_mb_t *vid_mb, mb_chnl_cmd_t *cmd_buf)
{
	/* check mailbox msg and send msg to task */
	switch (cmd_buf->hdr.cmd) {
	case VID_MB_FRAME_BUFF_READY_CMD:
	{
		// if buff ready
		if (info_cpu0.buffer_state == BUFF_IDLE) {
			video_transfer_cpu0_send_msg(VIDEO_CPU0_REQUEST, cmd_buf->param1);
		}
		break;
	}

	case VID_MB_FRAME_BUFF_RESPONSE_CMD:
	{
//		addAON_GPIO_Reg0x3 = 2;
		info_cpu0.buffer_id = cmd_buf->param1;
		info_cpu0.frame_id = cmd_buf->param2;
		info_cpu0.frame_len = cmd_buf->param3;
		info_cpu0.already_len = 0;
		info_cpu0.buffer_state = BUFF_BUSY;
		info_cpu0.buffer_len = IMAGE_LEN;

		if (info_cpu0.buffer_id == 0) {
			info_cpu0.buffer_addr = IMAGE_1;
		} else if (info_cpu0.buffer_id == 1) {
			info_cpu0.buffer_addr = IMAGE_2;
		} else {
			info_cpu0.buffer_addr = IMAGE_3;
		}

		video_transfer_cpu0_send_msg(VIDEO_CPU0_SEND, info_cpu0.buffer_id);
//		addAON_GPIO_Reg0x3 = 0;

		break;
	}

	case VID_MB_CPU0_EXIT_CMD:
	{
		if (video_thread_cpu0_hdl) {
			video_transfer_cpu0_send_msg(VIDEO_CPU0_EXIT, 0);
		}
		break;
	}

	default:
		break;
	}
}

static void video_cp0_mailbox_tx_isr(video_mb_t *vid_mb)
{
	//os_printf("enter cp0_mailbox_tx_isr \r\n");
}

static void video_cp0_mailbox_tx_cmpl_isr(video_mb_t *vid_mb, mb_chnl_ack_t *cmd_buf)
{
	//os_printf("enter cp0_mailbox_tx_cmpl \r\n");
}

static void video_pool_init(void *data)
{
}

static void video_config_desc(void)
{
	uint32_t node_len = TVIDEO_RXNODE_SIZE_TCP;

	if (video_transfer_setup.send_type == TVIDEO_SND_UDP) {
#if(TVIDEO_USE_HDR)
		if (video_transfer_setup.open_type != TVIDEO_OPEN_SPIDMA)
			node_len = TVIDEO_RXNODE_SIZE_UDP - video_transfer_setup.pkt_header_size;
		else
#endif
			node_len = TVIDEO_RXNODE_SIZE_UDP;

	} else if (video_transfer_setup.send_type == TVIDEO_SND_TCP)
		node_len = TVIDEO_RXNODE_SIZE_TCP;
	else if (video_transfer_setup.send_type == TVIDEO_SND_INTF) {
#if(TVIDEO_USE_HDR)
		node_len = TVIDEO_RXNODE_SIZE_UDP - video_transfer_setup.pkt_header_size;
#else
		node_len = TVIDEO_RXNODE_SIZE_UDP;
#endif
	} else if (video_transfer_setup.send_type == TVIDEO_SND_BUFFER)
		node_len = TVIDEO_RXNODE_SIZE_TCP;
	else
		os_printf("Err snd tpye in spidma\r\n");

	video_rx_node_len = node_len;

	/*video_cfg.dev_id = 3;
	video_cfg.frame_rate = 20;
	video_cfg.resolution = (640 << 16) | 480;*/
}

static void video_sendto_udp(void)
{
	uint32_t pkt_cnt = 0;
	uint32_t buf_len = 0;
	video_packet_t param;
	uint8_t node_buff[TVIDEO_RXNODE_SIZE] = {0};

	while (info_cpu0.already_len < info_cpu0.frame_len) {
//		addAON_GPIO_Reg0x4 = 2;
		param.ptk_ptr = &node_buff[0];
		param.frame_id = info_cpu0.frame_id;
		if ((info_cpu0.frame_len - info_cpu0.already_len) <= video_rx_node_len) {
			param.is_eof = 1;
			pkt_cnt = info_cpu0.frame_len / video_rx_node_len;
			if (info_cpu0.frame_len % video_rx_node_len)
				pkt_cnt += 1;
			param.frame_len = pkt_cnt;
			param.ptklen = video_rx_node_len;
			buf_len = (info_cpu0.frame_len % video_rx_node_len) + 4;
		} else {
			param.is_eof = 0;
			param.frame_len = 0;
			param.ptklen = video_rx_node_len;
			buf_len = TVIDEO_RXNODE_SIZE;
		}

		if (video_transfer_setup.add_pkt_header)
			video_transfer_setup.add_pkt_header(&param);

		dma_memcpy(param.ptk_ptr + video_transfer_setup.pkt_header_size, (uint8_t *)(info_cpu0.buffer_addr + info_cpu0.already_len), buf_len - 4);

		if (video_transfer_setup.send_func) {
			video_transfer_setup.send_func(&node_buff[0], buf_len);
		}

		info_cpu0.already_len += video_rx_node_len;
//		addAON_GPIO_Reg0x4 = 0;
	}

	info_cpu0.buffer_state = BUFF_IDLE;
}

static void video_transfer_save_image(frame_information_t * info)
{
#if (CONFIG_SDCARD_HOST)
	char *file_path = ".jpg";
	char cFileName[50];
	unsigned int uiTemp = 0;
	FIL fp1;

	sprintf(cFileName, "%d:%d%s", DISK_NUMBER_SDIO_SD, info->frame_id, file_path);
	FRESULT fr = f_open(&fp1, cFileName, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		os_printf("open %s fail.\r\n", cFileName);
		return;
	}

	fr = f_write(&fp1, (void *)info->buffer_addr, info->frame_len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("can not write file!\n");
		f_close(&fp1);
		return;
	}

	f_close(&fp1);
#endif
}

static bk_err_t video_transfer_request_frame(void)
{
//	addAON_GPIO_Reg0x2 = 2;
	video_mailbox_msg_t mb_msg;

	mb_msg.mb_cmd = VID_MB_FRAME_BUFF_REQUEST_CMD;

	mb_msg.param1 = 0;
	mb_msg.param2 = 0;
	mb_msg.param3 = 0;
//	addAON_GPIO_Reg0x2 = 0;
	return video_mailbox_send_msg(&mb_msg);
}

static void video_transfer_data_process(void)
{
	if (image_save_cpu0_enable) {
		video_transfer_save_image(&info_cpu0);
	}

	video_sendto_udp();

	video_transfer_set_frame_buff_state();
}

static bk_err_t video_transfer_inform_exit(void)
{
	video_mailbox_msg_t mb_msg;

	mb_msg.mb_cmd = VID_MB_CPU1_EXIT_CMD;

	mb_msg.param1 = 0;
	mb_msg.param2 = 0;
	mb_msg.param3 = 0;
	return video_mailbox_send_msg(&mb_msg);
}

static bk_err_t media_send_common_mb_msg(common_mailbox_msg_t *msg)
{
	mb_chnl_cmd_t mb_cmd;

	//send mailbox msg to cpu0
	mb_cmd.hdr.cmd = msg->mb_cmd;
	mb_cmd.param1 = msg->param1;
	mb_cmd.param2 = msg->param2;
	mb_cmd.param3 = msg->param3;
	return mb_chnl_write(MB_CHNL_COM, &mb_cmd);
}

static void media_mb_rx_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	//
}

static void media_mb_tx_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	//
}

static void media_mb_tx_cmpl_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	//
}

static void media_com_mailbox_init(void)
{
	//init maibox
	//mb_chnl_init();
	mb_chnl_open(MB_CHNL_COM, NULL);
	if (media_mb_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_RX_ISR, media_mb_rx_isr);
	if (media_mb_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_TX_ISR, media_mb_tx_isr);
	if (media_mb_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_TX_CMPL_ISR, media_mb_tx_cmpl_isr);
}

static void video_transfer_cpu0_main(beken_thread_arg_t data)
{
	bk_err_t ret = BK_OK;
	common_mailbox_msg_t com_mb_msg;

	/* init maibox */
	video_mailbox_init(video_cp0_mailbox_rx_isr, video_cp0_mailbox_tx_isr, video_cp0_mailbox_tx_cmpl_isr);
	os_printf("cp0: config mailbox complete \r\n");

	video_pool_init(data);
	video_config_desc();

	/* send "COM_MB_START_AUDIO_CMD" mailbox msg to media_common thread in cpu1 */
	media_com_mailbox_init();
	com_mb_msg.mb_cmd = COM_MB_START_VIDEO_CMD;
	com_mb_msg.param1 = video_cfg.dev_id;
	com_mb_msg.param2 = video_cfg.frame_rate;
	com_mb_msg.param3 = video_cfg.resolution;
	ret = media_send_common_mb_msg(&com_mb_msg);
	if (ret != BK_OK) {
		os_printf("cp0: init video transfer fail \r\n");
		goto exit;
	}

	delay(10000);

	while(1) {
		video_msg_t msg;
		ret = rtos_pop_from_queue(&vid_cpu0_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.type) {
			case VIDEO_CPU0_REQUEST:
				video_transfer_request_frame();
				break;
			case VIDEO_CPU0_SEND:
				video_transfer_data_process();
				break;
			case VIDEO_CPU1_EXIT:
				video_transfer_inform_exit();
				break;

			case VIDEO_CPU0_EXIT:
				goto exit;
				default:
					break;
			}
		}
	}


exit:
	// step 2: disable mailbox 
	video_mailbox_deinit();

	// step 3: reset info_cpu0
	os_memset(&info_cpu0, 0, sizeof(frame_information_t));

	// step 4: delate msg queue
	rtos_deinit_queue(&vid_cpu0_msg_que);
	vid_cpu0_msg_que = NULL;

	// step 5: delate task
	video_thread_cpu0_hdl = NULL;
	rtos_delete_thread(NULL);
}


bk_err_t bk_video_transfer_cpu0_init(video_setup_t *setup_cfg)
{
	int ret;

	os_printf("video_transfer_cpu0_init %d,%d\r\n", setup_cfg->send_type, setup_cfg->open_type);

	if ((!video_thread_cpu0_hdl) && (!vid_cpu0_msg_que)) {
		// bakup setup_cfg, because of that 'setup_cfg' may not static value.
		os_memcpy(&video_transfer_setup, setup_cfg, sizeof(video_setup_t));

		ret = rtos_init_queue(&vid_cpu0_msg_que,
							  "video_transfer_queue_cpu0",
							  sizeof(video_msg_t),
							  TU_QITEM_COUNT);
		if (kNoErr != ret) {
			os_printf("cp0: ceate video internal message queue in cpu0 failed \r\n");
			return kGeneralErr;
		}

		ret = rtos_create_thread(&video_thread_cpu0_hdl,
								 4,
								 "video_transfer_cpu0",
								 (beken_thread_function_t)video_transfer_cpu0_main,
								 4 * 1024,
								 (beken_thread_arg_t)&video_transfer_setup);
		if (ret != kNoErr) {
			rtos_deinit_queue(&vid_cpu0_msg_que);
			vid_cpu0_msg_que = NULL;
			video_thread_cpu0_hdl = NULL;
			os_printf("Error: Failed to create video transfer cpu0: %d\r\n", ret);
			return kGeneralErr;
		}

		return kNoErr;
	} else
		return kInProgressErr;
}

bk_err_t bk_video_transfer_cpu0_deinit(void)
{
	return video_transfer_cpu0_send_msg(VIDEO_CPU1_EXIT, 0);
}


void video_transfer_set_camera_config(uint32_t resolution, uint32_t frame_rate, uint32_t dev_id)
{
	video_cfg.dev_id = dev_id;
	video_cfg.frame_rate = frame_rate;
	video_cfg.resolution = resolution;
}

void video_transfer_cpu0_set_save_image_enable(uint8_t enable)
{
	image_save_cpu0_enable = enable;
}

