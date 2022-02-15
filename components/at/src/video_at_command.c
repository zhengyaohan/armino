#include "at_common.h"

// this moudule only support camera in chip, with sccb interface.
#if (CONFIG_CAMERA && CONFIG_APP_DEMO_VIDEO_TRANSFER)// && (APP_DEMO_CFG_USE_VIDEO_BUFFER))
#include "video_transfer.h"
#if (/*CONFIG_SDCARD_HOST || */ CONFIG_USB_HOST)
#include "ff.h"
#endif

#if CONFIG_PSRAM
#include "bk_api_psram.h"
#endif

#if CONFIG_GENERAL_DMA
#include "general_dma_pub.h"
#include "bk_general_dma.h"
#endif

#include "at_video_common.h"

static video_buff_t *g_video_buff = NULL;
static UINT32 g_pkt_seq = 0;

int video_set_camera_enable_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int video_take_photo_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#if CONFIG_PSRAM
int video_read_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int video_disable_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

const at_command_t video_at_cmd_table[] = {
	{0, "ENABLE", 0, "open/close camera(1/0)", video_set_camera_enable_handler},
	{1, "TAKE_PHOTO", 1, "take a picture", video_take_photo_handler},
#if CONFIG_PSRAM
	{2, "READPSRAM", 0, "psram", video_read_psram_handler},
	{3, "DEINITPSRAM", 0, "deinit psram", video_disable_psram_handler},
#endif

};

int video_at_cmd_cnt(void)
{
	return sizeof(video_at_cmd_table) / sizeof(video_at_cmd_table[0]);
}

int video_buffer_recv_data(UINT8 * data, UINT32 len)
{
	if (g_video_buff->receive_state == VIDEO_BUFF_COPY) {
		os_printf("---123456---\r\n");
		vbuf_header_t *hdr = (vbuf_header_t*)data;
		UINT32 org_len = 0;
		GLOBAL_INT_DECLARATION();

		if (len < sizeof(vbuf_header_t)) {
			os_printf("unknow err!\r\n");
			return len;
		}

		org_len = len - sizeof(vbuf_header_t); // video data len not include header
		data = data + sizeof(vbuf_header_t); // video data pointer except header

		os_printf("%d-%d: %d-%d: %d-%d\r\n", hdr->id, g_video_buff->frame_id,
		hdr->pkt_seq, (g_video_buff->frame_pkt_cnt + 1), g_video_buff->receive_state, hdr->is_eof);

		if ((hdr->id != g_video_buff->frame_id) && (hdr->pkt_seq == 1)) {
			// start of frame;
			GLOBAL_INT_DISABLE();
			g_video_buff->frame_id = hdr->id;
			g_video_buff->frame_len = 0;
			g_video_buff->frame_pkt_cnt = 0; // enter this function one time add 1
			g_video_buff->buf_ptr = g_video_buff->buf_base;
			GLOBAL_INT_RESTORE();
			//os_printf("sof:%d\r\n", g_vbuf->frame_id);
		}

		os_printf("%d-%d: %d-%d: %d-%d\r\n", hdr->id, g_video_buff->frame_id,
		hdr->pkt_seq, (g_video_buff->frame_pkt_cnt + 1), g_video_buff->receive_state, hdr->is_eof);
		if ((hdr->id == g_video_buff->frame_id) // ensure same frame
			&& ((g_video_buff->frame_pkt_cnt + 1) == hdr->pkt_seq)) {
			// the same frame data

			// step1: save two byte of first packet video data
			/*if (g_video_buff->frame_pkt_cnt == 0) {
				#if CFG_GENERAL_DMA
				gdma_memcpy(g_video_buff->buf_ptr, data, 2); // data copy to read_buf by dma
				#else
				os_memcpy(g_video_buff->buf_ptr, data, 2);
				#endif
			}*/

			// step2: update recv video data information
			GLOBAL_INT_DISABLE();
			g_video_buff->frame_len += org_len; // frame totle legth
			g_video_buff->frame_pkt_cnt += 1; // frame packet count
			GLOBAL_INT_RESTORE();

			// step3: checkout send output func is NULL
			if (g_video_buff->file_ptr) {
				// send out
				#if 0
				UINT32 bw = 0;
				f_write((FIL *)g_video_buff->file_ptr, data, org_len, &bw);
				#else
				os_printf("\r\n");
				for (int s = 0; s < 20; s ++)
					os_printf("%d", data[s]);
				os_printf("\r\n");
				#endif
			}


			// step4: if end of the current frame
			if (hdr->is_eof == 1) {
				GLOBAL_INT_DISABLE();
				g_video_buff->receive_state = VIDEO_BUFF_IDLE;
				GLOBAL_INT_RESTORE();

				// all frame data have received, wakeup usr thread
				rtos_set_semaphore(&g_video_buff->aready_semaphore);
				/*
				UINT8 *sof_ptr, *eof_ptr, *crc_ptr;
				UINT32 p_len, right_image = 0;

				sof_ptr = g_video_buff->buf_base;
				eof_ptr = data -7;
				crc_ptr = eof_ptr + 3;

				if (((sof_ptr[0] == 0xff) && (sof_ptr[1] == 0xd8)) &&
						((eof_ptr[0] == 0xff) && (eof_ptr[1] == 0xd9))) {
					p_len = crc_ptr[0] + (crc_ptr[1] << 8)
						+ (crc_ptr[2] << 16) + (crc_ptr[3] << 24);

					os_printf("vb,len:%d - %d\r\n", p_len, (g_video_buff->frame_len - 5));
					if (p_len == (g_video_buff->frame_len - 5)) {
						right_image = 1;
					}
				}

				if (right_image) {
					GLOBAL_INT_DISABLE();
					g_video_buff->receive_state = VIDEO_BUFF_IDLE;
					GLOBAL_INT_RESTORE();

					// all frame data have received, wakeup usr thread
					rtos_set_semaphore(&g_video_buff->aready_semaphore);
				}*/
			}
		}
		return len;
	} else {
		return len;
	}
}

void video_buffer_add_pkt_header(TV_HDR_PARAM_PTR param)
{
	vbuf_header_t *elem_tvhdr = (vbuf_header_t *)param->ptk_ptr;

	g_pkt_seq++;
	elem_tvhdr->id = (UINT8)param->frame_id;
	elem_tvhdr->is_eof = param->is_eof;
	elem_tvhdr->pkt_cnt = param->frame_len;
	elem_tvhdr->pkt_seq = g_pkt_seq;

	//os_printf("i:%d,%d\r\n", param->frame_id, g_pkt_seq);

	if (param->is_eof) {
		//os_printf("eof\r\n");
		g_pkt_seq = 0;
	}
}

int video_buff_set_open(void)
{
	if (g_video_buff == NULL) {
		int ret;
		GLOBAL_INT_DECLARATION();
		TVIDEO_SETUP_DESC_ST setup;

		g_video_buff = (video_buff_t *)os_malloc(sizeof(video_buff_t));
		if (g_video_buff == NULL) {
			os_printf("vbuf init no mem\r\n");
			ret = kNoMemoryErr;
			return ret;
		}

		ret = rtos_init_semaphore(&g_video_buff->aready_semaphore, 1);
		if (ret != kNoErr) {
			os_printf("vbuf init semaph failed\r\n");
			os_free(g_video_buff);
			g_video_buff = NULL;
			return ret;
		}

		GLOBAL_INT_DISABLE();
		g_video_buff->buf_base = NULL;
		g_video_buff->buf_len = 0;

		g_video_buff->frame_len = 0;
		g_video_buff->buf_ptr = NULL;
		g_video_buff->receive_state = VIDEO_BUFF_IDLE;

		g_video_buff->frame_id = 0xffff;
		g_video_buff->frame_pkt_cnt = 0;
		g_video_buff->file_ptr = 0;
		GLOBAL_INT_RESTORE();

		setup.open_type = TVIDEO_OPEN_SCCB;
		setup.send_type = TVIDEO_SND_INTF;
		setup.send_func = video_buffer_recv_data;
		setup.start_cb = NULL;
		setup.end_cb = NULL;

		setup.pkt_header_size = sizeof(vbuf_header_t);
		setup.add_pkt_header = video_buffer_add_pkt_header;

		ret = video_transfer_init(&setup);
		if (ret != kNoErr) {
			os_printf("video_transfer_init failed\r\n");
			rtos_deinit_semaphore(&g_video_buff->aready_semaphore);
			os_free(g_video_buff);
			g_video_buff = NULL;
		}

		os_printf("vbuf opened\r\n");
		return ret;
	}

	return kGeneralErr;
}

int video_buff_set_close(void)
{
	if (g_video_buff) {
		int ret;
		ret = video_transfer_deinit();
		if (ret != kNoErr) {
			os_printf("video_buffer_close failed\r\n");
			return ret;
		}

		if (g_video_buff->buf_base) {
			// user all video_buffer_read_frame and blocked, so wakeup it
			rtos_set_semaphore(&g_video_buff->aready_semaphore);

			// wait until clear the buf flag
			while (g_video_buff->buf_base);
		}

		rtos_deinit_semaphore(&g_video_buff->aready_semaphore);

		GLOBAL_INT_DECLARATION();

		GLOBAL_INT_DISABLE();
		g_video_buff->aready_semaphore = NULL;
		GLOBAL_INT_RESTORE();

		os_free(g_video_buff);
		g_video_buff = NULL;
		os_printf("voide close\r\n");
		return kNoErr;
	}

	os_printf("video not open\r\n");
	return kGeneralErr;
}

#if (/*CONFIG_SDCARD_HOST || */ CONFIG_USB_HOST)
int video_buff_read_image(FIL *file_ptr)
#else
int video_buff_read_image(int *file_ptr)
#endif
{
	os_printf("[%s],[%d]\r\n", __func__, __LINE__);
	UINT32 frame_len = 0;
	GLOBAL_INT_DECLARATION();

	if (g_video_buff && (g_video_buff->buf_base == NULL))
	{
		int ret;
		UINT32 timeout;

		// try to get semaphore, clear send by the previous frame
		while (rtos_get_semaphore(&g_video_buff->aready_semaphore, 0) == kNoErr);

		os_printf("[%s],[%d]\r\n", __func__, __LINE__);
		GLOBAL_INT_DISABLE();
		g_video_buff->buf_base = (UINT8 *)os_malloc(2);
		g_video_buff->file_ptr = (UINT32)file_ptr;
		g_video_buff->receive_state = VIDEO_BUFF_COPY;
		GLOBAL_INT_RESTORE();

		timeout = BEKEN_WAIT_FOREVER;

		ret = rtos_get_semaphore(&g_video_buff->aready_semaphore, timeout);
		if (ret == kNoErr) {
			frame_len = g_video_buff->frame_len;
		} else {
			os_printf("read frame time out\r\n");
			frame_len = 0;
		}

		GLOBAL_INT_DISABLE();
		os_free(g_video_buff->buf_base);
		g_video_buff->buf_base = NULL;
		GLOBAL_INT_RESTORE();
	}

	return frame_len;
}

int video_set_camera_enable_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;

	if (argc != 1)
	{
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

	if(!os_strcmp(argv[0], "1")) {
		err = video_buff_set_open();
	} else if(!os_strcmp(argv[0], "0")) {
		err = video_buff_set_close();
	} else {
		os_printf("input first param error\n");
		err = kParamErr;
		goto error;
	}

	if (err == kNoErr) {
		msg = AT_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return err;
	}

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}

int video_take_photo_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("[%s],[%d]\r\n", __func__, __LINE__);

	char *msg = NULL;
	int err = kNoErr;

	if (argc != 0)
	{
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

#if (/*CONFIG_SDCARD_HOST || */CONFIG_USB_HOST)
	//onst char* file_path = SET_PICTURE_SAVE_PATH(1);
	const TCHAR *file_path = "./bk7271/bk7271_1.txt";
	FIL fp1;
	FRESULT fr = f_open(&fp1, file_path, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
	if (fr != FR_OK)
	{
		os_printf("can not open file:%s!\n", file_path);
		return kWriteErr;
	}
	err = video_buff_read_image(&fp1);
	if (err == 0) {
		os_printf("read image failed!!!\n");
		goto error;
	}

	fr = f_close(&fp1);
	if (fr != FR_OK) {
		os_printf("can not close file:%s!\n", file_path);
	}
#else
	err = video_buff_read_image(&err);
#endif
	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}

#if CONFIG_PSRAM

int video_read_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;
	uint8_t i = 0;

	uint32_t psram = 0x60000000;
	uint32_t mode = 0x00054043;
	if (argc != 0) {
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

	err = bk_psram_init(mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		err = kParamErr;
		goto error;
	}

	os_memset((uint8_t *)psram, 0, 30);

	for (i = 0; i < 30; i++) {
		*((uint8_t *)psram + i) = i;
	}

	os_printf("data:\n");
	for (i = 0; i <30; i++) {
		os_printf("%d ", *((uint8_t *)psram + i));
	}
	os_printf("\n");

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}

int video_disable_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;

	if (argc != 0) {
			os_printf("input param error\n");
			err = kParamErr;
			goto error;
	}

	err = bk_psram_deinit();
	if (err != kNoErr) {
		os_printf("psram deinit error\n");
		err = kParamErr;
		goto error;
	}

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}
#endif // CONFIG_PSRAM


#endif

