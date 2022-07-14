#include <common/bk_include.h>

#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>

#ifdef CONFIG_RTT
#include <sys/socket.h>
#endif
#include "lwip/sockets.h"

#include "doorbell.h"
#include "doorbell_config.h"
#include "bk_uart.h"
#include <os/mem.h>
#include <components/video_transfer.h>
#include <driver/dma.h>
#include <modules/audio_ring_buff.h>
#include "audio_transfer_cp0.h"
#include "video_transfer_cpu0.h"
#include "video_transfer_save.h"

#include "trs_api.h"

#define DBD_TAG       "doorbell"


#define DOORBELL_DEBUG_LOG              1
#if DOORBELL_DEBUG_LOG
#define DBD os_printf
#define DBW os_printf
#define DBE os_printf
#else
#define DBD(...)
#define DBW(...)
#define DBE(...)
#endif

#define AUDIO_TRANSFER_ENABLE           1

#if CONFIG_AUDIO_TRANSFER_CPU0
#define AUDIO_TRANSFER_CPU0_MODE             1       //0: default cpu1 mode, 1: cpu0 mode
#endif

#define APP_DEMO_UDP_RCV_BUF_LEN        1472
#define APP_DEMO_UDP_SOCKET_TIMEOUT     100  // ms

#if AUDIO_TRANSFER_ENABLE
/* audio used */
#define READ_SIZE    160
#define WRITE_SIZE    320
#endif

int demo_doorbell_udp_img_fd = -1;
volatile int demo_doorbell_udp_romote_connected = 0;
volatile int demo_doorbell_udp_run = 0;
beken_thread_t demo_doorbell_udp_hdl = NULL;
struct sockaddr_in *demo_doorbell_remote = NULL;
int demo_doorbell_udp_voice_romote_connected = 0;

struct sockaddr_in *demo_doorbell_voice_udp_remote = NULL;

int demo_doorbell_udp_voice_fd = -1;

/* audio used */
#if AUDIO_TRANSFER_ENABLE
static bool audio_transfer_work_status = false;
static bool audio_transfer_start_status = false;
static bool audio_apk_status = false;    //false: apk open audio  true: apk close audio
#if AUDIO_TRANSFER_CPU0_MODE
static uint32_t write_aud_data_err_count = 0;

#else
static uint8_t *temp_read_buffer = NULL;
static uint8_t *temp_write_buffer = NULL;
static uint8_t *audio_rx_ring_buffer = NULL;    //save data received from apk
static RingBufferContext audio_rx_rb;
bool temp_write_buffer_status = false;
static uint16_t audio_rx_seq = 0;    //save current audio data sequence number received from apk
static bool audio_apk_rx_status = false;    //save audio rx status
static uint32_t audio_rx_lost_count = 0;    //save lost audio data count recevied from apk
static uint32_t aud_rx_mode = 0;    //1: audio rx data not include sequence and time stamp  0: aduio rx data
#endif  //AUDIO_TRANSFER_CPU0_MODE

#endif  //AUDIO_TRANSFER_ENABLE

typedef enum
{
	CPU0 = 0,
	CPU1,
	CPU1_PLUS,
	DOORBELL_PLUS,
} doorbell_mode_t;


static doorbell_mode_t video_transfer_mode = CPU0;

extern void delay(int num);

int demo_doorbell_udp_send_packet(uint8_t *data, uint32_t len)
{
	int send_byte = 0;

	if (!demo_doorbell_udp_romote_connected)
		return 0;

	send_byte = sendto(demo_doorbell_udp_img_fd, data, len, MSG_DONTWAIT | MSG_MORE,
					   (struct sockaddr *)demo_doorbell_remote, sizeof(struct sockaddr_in));

	if (send_byte < 0) {
		/* err */
		//DBD("send return fd:%d\r\n", send_byte);
		send_byte = 0;
	}

	return send_byte;
}

#if DEMO_DOORBELL_EN_VOICE_TRANSFER
int demo_doorbell_udp_voice_send_packet(UINT8 *data, UINT32 len)
{
	int send_byte = 0;

	if (!demo_doorbell_udp_voice_romote_connected)
		return 0;

	send_byte = sendto(demo_doorbell_udp_voice_fd, data, len, MSG_DONTWAIT | MSG_MORE,
					   (struct sockaddr *)demo_doorbell_voice_udp_remote, sizeof(struct sockaddr_in));

	if (send_byte < 0) {
		/* err */
		//LWIP_UDP_PRT("send return fd:%d\r\n", send_byte);
		send_byte = 0;
	}

	return send_byte;
}
#endif //DEMO_DOORBELL_EN_VOICE_TRANSFER


#if AUDIO_TRANSFER_ENABLE

#if AUDIO_TRANSFER_CPU0_MODE
void cp0_audio_transfer_init_callback(void)
{
	GLOBAL_INT_DECLARATION();

	/* set status */
	GLOBAL_INT_DISABLE();
	audio_transfer_work_status = true;
	GLOBAL_INT_RESTORE();
	DBD("init and start audio transfer complete \r\n");
}

static void cp0_audio_transfer_init(void)
{
	bk_err_t ret = BK_OK;

	/* register callbacks */
	bk_audio_register_rw_cb(demo_doorbell_udp_voice_send_packet, cp0_audio_transfer_init_callback);
	DBD("register callbacks complete \r\n");

	/* init audio cpu0 transfer task */
	audio_setup_t config;
	config.samp_rate = AUDIO_SAMP_RATE_8K;
	/* init audio transfer task */
	ret = bk_audio_transfer_init(&config);
	if (ret != BK_OK) {
		os_printf("init audio transfer task fail \r\n");
	}

	DBD("init audio transfer driver complete \r\n");
}

static void cp0_audio_transfer_deinit(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_audio_transfer_deinit();
	if (ret != BK_OK) {
		DBD("cp0: audio transfer deinit fail \r\n");
	}

	/* clear audio transfer status */
	audio_transfer_work_status = false;
	audio_transfer_start_status = false;

	DBD("cp0: stop audio_transfer test successful \r\n");
}
#else
static void audio_cp0_read_done_callback(uint8_t *buffer, uint32_t length)
{
	uint32 size = 0;
	//DBD("read done \r\n");

	/* send audio data to apk */
//	if (apk_work_status) {
		size = demo_doorbell_udp_voice_send_packet(temp_read_buffer, READ_SIZE);
		if (size != READ_SIZE) {
			DBD("send audio data to apk fail, size=%d \r\n", size);
			return;
		}
//	}
//	bk_gpio_set_output_low(GPIO_3);
}

/* get encoder used size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
static void audio_cp0_write_done_callback(uint8_t *buffer, uint32_t length)
{
	//DBD("write data done \r\n");

	/* reset temp_write_buffer_status */
	temp_write_buffer_status = false;
}

/* get encoder used size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
static void audio_cp0_get_encoder_used_size_callback(uint32_t size)
{
	bk_err_t ret = BK_OK;

	if (size >= READ_SIZE) {
		ret = bk_audio_read_req(temp_read_buffer, READ_SIZE);
		if (ret != BK_OK)
			DBD("send read request fail \r\n");
	}
}

/* get decoder remain size,
 * if the size is equal or greater than READ_SIZE, send read request to read data 
 */
static void audio_cp0_get_decoder_remain_size_callback(uint32_t size)
{
	bk_err_t ret = BK_OK;
	DBD("audio_cp0_get_decoder_remain_size_callback \r\n");

	/* check receive data buffer and write data to audio dac */
	if ((size >= WRITE_SIZE) && (temp_write_buffer_status)) {
		ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
		if (ret != BK_OK)
			DBD("send write request fail \r\n");
	}
}

/* receive CPU1 read request, and send read reqest to CPU1 */
static void audio_cp0_encoder_read_req_handler(void)
{
	bk_err_t ret = BK_OK;
	//DBD("read request \r\n");

//	if (apk_work_status) {
		ret = bk_audio_read_req(temp_read_buffer, READ_SIZE);
		if (ret != BK_OK)
			DBD("send write request fail \r\n");
//	}
}

/* receive CPU1 write request, and send write decoder data reqest to CPU1 */
static void audio_cp0_decoder_write_req_handler(void)
{
	bk_err_t ret = BK_OK;
	//DBD("write request \r\n");

	/* receive write request, and write data in temp_write_buffer to audio dac */
	ret = bk_audio_write_req(temp_write_buffer, WRITE_SIZE);
	if (ret != BK_OK)
		DBD("send write request fail \r\n");
//	else
//		temp_write_buffer_status = true;
}

/*
static void audio_cp0_write_data_req(void)
{
	bk_audio_get_decoder_remain_size();
	//DBD("get decoder remain size \r\n");
}
*/

static void audio_cp0_transfer_ready_callback(void)
{
	audio_transfer_work_status = true;
	DBD("audio_cp0_transfer_ready_callback \r\n");
}

static void cp0_audio_transfer_init(void)
{
	bk_err_t ret = BK_OK;

	temp_read_buffer = (uint8_t *)os_malloc(READ_SIZE);
	if (temp_read_buffer == NULL) {
		DBD("malloc temp_read_buffer fail \r\n");
		return;
	}
	DBD("malloc temp_read_buffer:%p complete \r\n", temp_read_buffer);
	temp_write_buffer = (uint8_t *)os_malloc(WRITE_SIZE);
	if (temp_write_buffer == NULL) {
		DBD("malloc temp_write_buffer fail \r\n");
		return;
	}
	DBD("malloc temp_write_buffer:%p complete \r\n", temp_write_buffer);

	audio_rx_ring_buffer = (uint8_t *)os_malloc(WRITE_SIZE*2);
	if (audio_rx_ring_buffer == NULL) {
		DBD("malloc audio_rx_ring_buffer fail \r\n");
		return;
	}
	DBD("malloc audio_rx_ring_buffer:%p complete \r\n", audio_rx_ring_buffer);

	ring_buffer_init(&audio_rx_rb, audio_rx_ring_buffer, WRITE_SIZE*2, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	DBD("init audio rx ring buffer complete \r\n");

	/* register callbacks */
	bk_audio_register_rw_cb(audio_cp0_read_done_callback,
							audio_cp0_write_done_callback,
							audio_cp0_get_encoder_used_size_callback,
							audio_cp0_get_decoder_remain_size_callback,
							audio_cp0_encoder_read_req_handler,
							audio_cp0_decoder_write_req_handler,
							audio_cp0_transfer_ready_callback);
	DBD("register callbacks complete \r\n");

	/* init audio cpu0 transfer task */
	ret = bk_audio_cp0_transfer_init(NULL);
	if (ret != BK_OK) {
		return;
	}
	DBD("init audio transfer driver complete \r\n");
}

static void cp0_audio_transfer_deinit(void)
{
	bk_err_t ret = BK_OK;

	ret = bk_audio_cp0_transfer_deinit();
	if (ret != BK_OK) {
		DBD("cp0: audio transfer deinit fail \r\n");
	}

	/* free memory */
	os_free(temp_read_buffer);
	os_free(temp_write_buffer);
	ring_buffer_clear(&audio_rx_rb);
	os_free(audio_rx_ring_buffer);

	/* clear audio transfer status */
	audio_transfer_work_status = false;
	audio_transfer_start_status = false;

	DBD("cp0: stop audio_transfer test successful \r\n");
}
#endif

#endif

void demo_doorbell_add_pkt_header(video_packet_t *param)
{
	media_hdr_t *elem_tvhdr = (media_hdr_t*)param->ptk_ptr;

	elem_tvhdr->id = (UINT8)param->frame_id;
	elem_tvhdr->is_eof = param->is_eof;
	elem_tvhdr->pkt_cnt = param->frame_len;
	elem_tvhdr->size = 0;

#if SUPPORT_TIANZHIHENG_DRONE
	elem_tvhdr->unused = 0;
#endif
}

#if AUDIO_TRANSFER_ENABLE
static void audio_transfer_cmd_handle(UINT32 cmd, UINT32 param)
{
	switch(cmd) {
		case AUDIO_TRANSFER_CLOSE:
			audio_apk_status = false;
#if AUDIO_TRANSFER_CPU0_MODE

#else
			if (param == 1)
				aud_rx_mode = 1;
#endif
			cp0_audio_transfer_deinit();
			break;

		case AUDIO_TRANSFER_OPEN:
			audio_apk_status = true;
#if AUDIO_TRANSFER_CPU0_MODE

#else
			if (param == 1)
				aud_rx_mode = 1;
			DBD("audio transfer init in apk mode \r\n");
			cp0_audio_transfer_init();
#endif

		default:
			break;
	}
}
#endif

static void demo_doorbell_udp_handle_cmd_data(UINT8 *data, UINT16 len)
{
	UINT32 param = 0;
	UINT32 cmd = (UINT32)data[0] << 24 | (UINT32)data[1] << 16 | (UINT32)data[2] << 8 | data[3];
	if (len >= 8) {
		param = (UINT32)data[4] << 24 | (UINT32)data[5] << 16 | (UINT32)data[6] << 8 | data[7];
	}

	DBD("doorbell cmd: %08X, param: %d, len: %d\n", cmd, param, len);
	switch(cmd) {
	case LCD_DISPLAY_BLEND_OPEN:
		bk_lcd_video_blending(1);
		break;

	case LCD_DISPLAY_BLEND_CLOSE:
		bk_lcd_video_blending(0);;
		break;

	default:
		break;
	}

#if AUDIO_TRANSFER_ENABLE
	audio_transfer_cmd_handle(cmd, param);
#endif

}

static void demo_doorbell_udp_app_connected(void)
{
	//app_demo_softap_send_msg(DAP_APP_CONECTED, 0);
}

static void demo_doorbell_udp_app_disconnected(void)
{
	//app_demo_softap_send_msg(DAP_APP_DISCONECTED, 0);
}

#if CFG_SUPPORT_HTTP_OTA
TV_OTA_ST ota_param = {
	NULL,
	0,
	0
};
static void demo_doorbell_udp_http_ota_handle(char *rev_data)
{
	//if(app_demo_softap_is_ota_doing() == 0)
	//{
	// to do
	//
	//app_demo_softap_send_msg(DAP_START_OTA, (u32)&ota_param);

	//os_memset(&ota_param, 0, sizeof(TV_OTA_ST));
	//}
}
#endif

static void demo_doorbell_udp_receiver(UINT8 *data, UINT32 len, struct sockaddr_in *demo_doorbell_remote)
{
	DBD("demo_doorbell_udp_receiver\r\n");

	GLOBAL_INT_DECLARATION();

	if (len < 2)
		return;

	if (data[0] == DOORBELL_CMD_IMG_HEADER) {
		if (data[1] == DOORBELL_CMD_START_IMG) {

			UINT8 *src_ipaddr = (UINT8 *)&demo_doorbell_remote->sin_addr.s_addr;
			DBD("src_ipaddr: %d.%d.%d.%d\r\n", src_ipaddr[0], src_ipaddr[1],
							 src_ipaddr[2], src_ipaddr[3]);
			DBD("udp connect to new port:%d\r\n", demo_doorbell_remote->sin_port);

			GLOBAL_INT_DISABLE();
			demo_doorbell_udp_romote_connected = 1;
			GLOBAL_INT_RESTORE();

#if (CONFIG_SPIDMA || CONFIG_CAMERA)
			video_setup_t setup;

			setup.open_type = TVIDEO_OPEN_SCCB;
			setup.send_type = TVIDEO_SND_UDP;
			setup.send_func = demo_doorbell_udp_send_packet;
			setup.start_cb = demo_doorbell_udp_app_connected;
			setup.end_cb = demo_doorbell_udp_app_disconnected;

			setup.pkt_header_size = sizeof(media_hdr_t);
			setup.add_pkt_header = demo_doorbell_add_pkt_header;

			if (video_transfer_mode == CPU0)
			{
				bk_video_transfer_init(&setup);
			}
			else if (video_transfer_mode == CPU1)
			{
#if CONFIG_DUAL_CORE
				bk_video_transfer_cpu0_init(&setup);
#endif
			}
			else if (video_transfer_mode == DOORBELL_PLUS)
			{
				video_transfer_open(&setup);
			}
			else if (video_transfer_mode == CPU1_PLUS)
			{

			}
#endif
		} else if (data[1] == DOORBELL_CMD_STOP_IMG) {
			DBD("udp close\r\n");

#if (CONFIG_SPIDMA || CONFIG_CAMERA)
			if (video_transfer_mode == CPU0)
			{
				bk_video_transfer_deinit();
			}
			else if (video_transfer_mode == CPU1)
			{
#if CONFIG_DUAL_CORE
				bk_video_transfer_cpu0_deinit();
#endif
			}
			else if (video_transfer_mode == DOORBELL_PLUS)
			{
				video_transfer_close();
			}
			else if (video_transfer_mode == CPU1_PLUS)
			{

			}
#endif

			GLOBAL_INT_DISABLE();
			demo_doorbell_udp_romote_connected = 0;
			GLOBAL_INT_RESTORE();
		}
#if CFG_SUPPORT_HTTP_OTA
		else if (data[1] == CMD_START_OTA)
			demo_doorbell_udp_http_ota_handle(&data[2]);
#endif
	}

}

#if DEMO_DOORBELL_EN_VOICE_TRANSFER
static void demo_doorbell_udp_voice_receiver(UINT8 *data, UINT32 len, struct sockaddr_in *udp_remote)
{
	//DBD("demo_doorbell_udp_voice_receiver, data len=%d \r\n", len);
#if AUDIO_TRANSFER_ENABLE

#if AUDIO_TRANSFER_CPU0_MODE
	bk_err_t ret = BK_OK;

	//receive data from apk, send mailbox msg to start audio transfer
	if (!audio_transfer_work_status && audio_apk_status) {
		if (!audio_transfer_start_status) {
			audio_transfer_start_status = true;
			/* init audio transfer */
			DBD("audio transfer init in cpu0 mode \r\n");
			cp0_audio_transfer_init();
			DBD("init and start audio transfer \r\n");
		}
		return;
	}

	if (len > 0)
		demo_doorbell_udp_voice_romote_connected = 1;

	//DBD("len: %d \r\n", len);

	ret = bk_audio_write_data(data, len);
	if (ret != BK_OK) {
		write_aud_data_err_count++;
		if (write_aud_data_err_count == 50) {
			DBD("write audio data fial 50 times \r\n", len);
			write_aud_data_err_count = 0;
		}
	}
#else
	uint32_t size = 0;
	uint16_t temp_aud_rx_seq = 0;
	long long temp_aud_time_stamp = 0;

	//receive data from apk, send mailbox msg to start audio transfer
	if (!audio_transfer_work_status && audio_apk_status) {
		if (!audio_transfer_start_status) {
			audio_transfer_start_status = true;
			/* wait some time to ensure that audio transfer init complete in cpu1 */
			delay(50000);
			/* send mailbox msg to start audio transfer */
			bk_audio_start_transfer();
			DBD("send mailbox msg to start audio transfer \r\n", len);
		}
		return;
	}

	if (aud_rx_mode) {
		temp_aud_rx_seq = (UINT16)data[0] << 8 | (UINT16)data[1];
		temp_aud_time_stamp = (long long)data[2] << 56
								| (long long)data[3] << 48
								| (long long)data[4] << 40
								| (long long)data[5] << 32
								| (long long)data[6] << 24
								| (long long)data[7] << 16
								| (long long)data[8] << 8
								| data[9];
		//DBD("receive data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
		//DBD("temp_aud_rx_seq:%d, audio_rx_seq:%d \r\n", temp_aud_rx_seq, audio_rx_seq);
		if (!audio_apk_rx_status) {
			DBD("audio_rx_seq: %d \r\n", audio_rx_seq);
			audio_apk_rx_status = true;
		} else {
			if (temp_aud_rx_seq != (audio_rx_seq + 1)) {
				audio_rx_lost_count++;
				DBD("lost seq:%d temp_aud_time_stamp:0x%x%x \r\n", temp_aud_rx_seq, (UINT32)(temp_aud_time_stamp>>32), (UINT32)(temp_aud_time_stamp & 0xFFFFFFFF));
				if (audio_rx_lost_count == 20) {
					DBD("lost apk audio data: 20 frames \r\n");
					audio_rx_lost_count = 0;
				}
			}
		}
		audio_rx_seq = temp_aud_rx_seq;
	}

	if (len > 0)
		demo_doorbell_udp_voice_romote_connected = 1;

	//DBD("len: %d \r\n", len);

	//GLOBAL_INT_DECLARATION();
	size = ring_buffer_get_free_size(&audio_rx_rb);
	if (size < WRITE_SIZE) {
		DBD("free size of audio_rx_rb is enough, size:%d \r\n", size);
		return;
	}

	if (aud_rx_mode)
		size = ring_buffer_write(&audio_rx_rb, &(data[10]), WRITE_SIZE);
	else
		size = ring_buffer_write(&audio_rx_rb, data, WRITE_SIZE);
	if (size != WRITE_SIZE) {
		DBD("write ring buffer data fail, size=%d \r\n", size);
		return;
	}

	size = ring_buffer_get_fill_size(&audio_rx_rb);
	if ((size >= WRITE_SIZE) && (!temp_write_buffer_status)) {
		size = ring_buffer_read(&audio_rx_rb, temp_write_buffer, WRITE_SIZE);
		temp_write_buffer_status = true;
		if (size != WRITE_SIZE) {
			DBD("read ring buffer data to temp_write_buffer fail, size=%d \r\n", size);
			return;
		}
	}
#endif //AUDIO_TRANSFER_CPU0_MODE

#endif  //AUDIO_TRANSFER_ENABLE
}
#endif // DEMO_DOORBELL_EN_VOICE_TRANSFER

static void demo_doorbell_udp_main(beken_thread_arg_t data)
{
	GLOBAL_INT_DECLARATION();
	int maxfd, udp_cmd_fd = -1, ret = 0;
	int rcv_len = 0;
	struct sockaddr_in cmd_remote;
	socklen_t srvaddr_len = 0;
	fd_set watchfd;
	struct timeval timeout;
	u8 *rcv_buf = NULL;

	DBD("demo_doorbell_udp_main entry\r\n");
	(void)(data);

	rcv_buf = (u8 *) os_malloc((APP_DEMO_UDP_RCV_BUF_LEN + 1) * sizeof(u8));
	if (!rcv_buf) {
		DBD("udp os_malloc failed\r\n");
		goto app_udp_exit;
	}

	demo_doorbell_remote = (struct sockaddr_in *)os_malloc(sizeof(struct sockaddr_in));
	if (!demo_doorbell_remote) {
		DBD("udp os_malloc failed\r\n");
		goto app_udp_exit;
	}

	// for data transfer
	demo_doorbell_udp_img_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (demo_doorbell_udp_img_fd == -1) {
		DBD("socket failed\r\n");
		goto app_udp_exit;
	}

	demo_doorbell_remote->sin_family = AF_INET;
	demo_doorbell_remote->sin_port = htons(DEMO_DOORBELL_UDP_IMG_PORT);
	demo_doorbell_remote->sin_addr.s_addr = htonl(INADDR_ANY);

	srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (bind(demo_doorbell_udp_img_fd, (struct sockaddr *)demo_doorbell_remote, srvaddr_len) == -1) {
		DBD("bind failed\r\n");
		goto app_udp_exit;
	}

	//  for recv uart cmd
	udp_cmd_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_cmd_fd == -1) {
		DBD("socket failed\r\n");
		goto app_udp_exit;
	}

	cmd_remote.sin_family = AF_INET;
	cmd_remote.sin_port = htons(DEMO_DOORBELL_UDP_CMD_PORT);
	cmd_remote.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(udp_cmd_fd, (struct sockaddr *)&cmd_remote, srvaddr_len) == -1) {
		DBD("bind failed\r\n");
		goto app_udp_exit;
	}

	maxfd = (udp_cmd_fd > demo_doorbell_udp_img_fd) ? udp_cmd_fd : demo_doorbell_udp_img_fd;

	// for voice transfer
#if DEMO_DOORBELL_EN_VOICE_TRANSFER
	demo_doorbell_voice_udp_remote = (struct sockaddr_in *)os_malloc(sizeof(struct sockaddr_in));
	if (!demo_doorbell_voice_udp_remote) {
		DBD("udp os_malloc failed\r\n");
		goto app_udp_exit;
	}

	demo_doorbell_udp_voice_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (demo_doorbell_udp_voice_fd == -1) {
		DBD("vo socket failed\r\n");
		goto app_udp_exit;
	}

	demo_doorbell_voice_udp_remote->sin_family = AF_INET;
	demo_doorbell_voice_udp_remote->sin_port = htons(DEMO_DOORBELL_UDP_VOICE_PORT);
	demo_doorbell_voice_udp_remote->sin_addr.s_addr = htonl(INADDR_ANY);

	srvaddr_len = (socklen_t)sizeof(struct sockaddr_in);
	if (bind(demo_doorbell_udp_voice_fd, (struct sockaddr *)demo_doorbell_voice_udp_remote, srvaddr_len) == -1) {
		DBD("bind failed\r\n");
		goto app_udp_exit;
	}
	maxfd = (maxfd > demo_doorbell_udp_voice_fd) ? maxfd : demo_doorbell_udp_voice_fd;
#endif // DEMO_DOORBELL_EN_VOICE_TRANSFER

	timeout.tv_sec = APP_DEMO_UDP_SOCKET_TIMEOUT / 1000;
	timeout.tv_usec = (APP_DEMO_UDP_SOCKET_TIMEOUT % 1000) * 1000;

	GLOBAL_INT_DISABLE();
	demo_doorbell_udp_romote_connected = 0;
	demo_doorbell_udp_run = 1;
#if DEMO_DOORBELL_EN_VOICE_TRANSFER
	demo_doorbell_udp_voice_romote_connected = 0;
#endif
	GLOBAL_INT_RESTORE();

	while (demo_doorbell_udp_run) {
		FD_ZERO(&watchfd);
		FD_SET(demo_doorbell_udp_img_fd, &watchfd);
		FD_SET(udp_cmd_fd, &watchfd);
#if DEMO_DOORBELL_EN_VOICE_TRANSFER
		FD_SET(demo_doorbell_udp_voice_fd, &watchfd);
#endif

		//DBD("wait for select\r\n");
		ret = select(maxfd + 1, &watchfd, NULL, NULL, &timeout);
		if (ret < 0) {
			DBD("select ret:%d\r\n", ret);
			break;
		} else {
			// is img fd, data transfer
			if (FD_ISSET(demo_doorbell_udp_img_fd, &watchfd)) {
				rcv_len = recvfrom(demo_doorbell_udp_img_fd, rcv_buf, APP_DEMO_UDP_RCV_BUF_LEN, 0,
								   (struct sockaddr *)demo_doorbell_remote, &srvaddr_len);

				if (rcv_len <= 0) {
					// close this socket
					DBD("recv close fd:%d\r\n", demo_doorbell_udp_img_fd);
					break;
				} else {
					rcv_len = (rcv_len > APP_DEMO_UDP_RCV_BUF_LEN) ? APP_DEMO_UDP_RCV_BUF_LEN : rcv_len;
					rcv_buf[rcv_len] = 0;

					DBD("demo_doorbell_udp_receiver");
					demo_doorbell_udp_receiver(rcv_buf, rcv_len, demo_doorbell_remote);
				}
			} else if (FD_ISSET(udp_cmd_fd, &watchfd)) {
				rcv_len = recvfrom(udp_cmd_fd, rcv_buf, APP_DEMO_UDP_RCV_BUF_LEN, 0,
								   (struct sockaddr *)&cmd_remote, &srvaddr_len);

				if (rcv_len <= 0) {
					// close this socket
					DBD("recv close fd:%d\r\n", udp_cmd_fd);
					break;
				} else {
					rcv_len = (rcv_len > APP_DEMO_UDP_RCV_BUF_LEN) ? APP_DEMO_UDP_RCV_BUF_LEN : rcv_len;
					rcv_buf[rcv_len] = 0;

					demo_doorbell_udp_handle_cmd_data(rcv_buf, rcv_len);
				}
			}
#if DEMO_DOORBELL_EN_VOICE_TRANSFER
			if (FD_ISSET(demo_doorbell_udp_voice_fd, &watchfd)) {
				rcv_len = recvfrom(demo_doorbell_udp_voice_fd, rcv_buf, APP_DEMO_UDP_RCV_BUF_LEN, 0,
								   (struct sockaddr *)demo_doorbell_voice_udp_remote, &srvaddr_len);

				if (rcv_len <= 0) {
					// close this socket
					DBD("recv close fd:%d\r\n", demo_doorbell_udp_voice_fd);
					break;
				} else {
					rcv_len = (rcv_len > APP_DEMO_UDP_RCV_BUF_LEN) ? APP_DEMO_UDP_RCV_BUF_LEN : rcv_len;
					rcv_buf[rcv_len] = 0;
					demo_doorbell_udp_voice_receiver(rcv_buf, rcv_len, demo_doorbell_voice_udp_remote);
				}
			}
#endif // DEMO_DOORBELL_EN_VOICE_TRANSFER
		}
	}

app_udp_exit:

	DBE("demo_doorbell_udp_main exit %d\r\n", demo_doorbell_udp_run);

#if (CONFIG_SPIDMA || CONFIG_CAMERA)
	bk_video_transfer_deinit();
#endif

#if AUDIO_TRANSFER_ENABLE
	/* deinit audio transfer driver */

	cp0_audio_transfer_deinit();
#endif

	if (rcv_buf) {
		os_free(rcv_buf);
		rcv_buf = NULL;
	}

#if DEMO_DOORBELL_EN_VOICE_TRANSFER
	if (demo_doorbell_udp_voice_fd != -1) {
		close(demo_doorbell_udp_voice_fd);
		demo_doorbell_udp_voice_fd = -1;
	}

	if (demo_doorbell_voice_udp_remote) {
		os_free(demo_doorbell_voice_udp_remote);
		demo_doorbell_voice_udp_remote = NULL;
	}
#endif // DEMO_DOORBELL_EN_VOICE_TRANSFER
	if (demo_doorbell_remote) {
		os_free(demo_doorbell_remote);
		demo_doorbell_remote = NULL;
	}

	if (demo_doorbell_udp_img_fd != -1) {
		close(demo_doorbell_udp_img_fd);
		demo_doorbell_udp_img_fd = -1;
	}

	if (udp_cmd_fd != -1) {
		close(udp_cmd_fd);
		udp_cmd_fd = -1;
	}

	GLOBAL_INT_DISABLE();
	demo_doorbell_udp_romote_connected = 0;
	demo_doorbell_udp_run = 0;
#if DEMO_DOORBELL_EN_VOICE_TRANSFER
	demo_doorbell_udp_voice_romote_connected = 0;
#endif
	GLOBAL_INT_RESTORE();

	demo_doorbell_udp_hdl = NULL;
	rtos_delete_thread(NULL);
}

UINT32 demo_doorbell_udp_init(void)
{
	int ret;

	DBD("app_demo_udp_init\r\n");
	if (!demo_doorbell_udp_hdl) {
		ret = rtos_create_thread(&demo_doorbell_udp_hdl,
								 4,
								 "app_udp",
								 (beken_thread_function_t)demo_doorbell_udp_main,
								 1024,
								 (beken_thread_arg_t)NULL);
		if (ret != kNoErr) {
			DBD("Error: Failed to create spidma_intfer: %d\r\n", ret);
			return kGeneralErr;
		}
	}

	return kNoErr;
}

void demo_doorbell_udp_deinit(void)
{
	GLOBAL_INT_DECLARATION();

	DBD("app_demo_udp_deinit\r\n");
	if (demo_doorbell_udp_run == 0)
		return;

	GLOBAL_INT_DISABLE();
	demo_doorbell_udp_run = 0;
	GLOBAL_INT_RESTORE();

	while (demo_doorbell_udp_hdl)
		rtos_delay_milliseconds(10);
}

void cli_doorbell_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc > 0)
	{
		uint8_t mode = os_strtoul(argv[1], NULL, 10);

		if (mode == CPU0)
		{
			video_transfer_mode = CPU0;
		}
		else if (mode == CPU1)
		{
			video_transfer_mode = CPU1;
		}
		else if (mode == CPU1_PLUS)
		{
			video_transfer_mode = CPU1_PLUS;
		}
		else if (mode == DOORBELL_PLUS)
		{
			video_transfer_mode = DOORBELL_PLUS;
		}
		else
		{
			video_transfer_mode = CPU0;
		}
	}

	os_printf("cli_doorbell_test_cmd, mode: %d\n", video_transfer_mode);


	demo_doorbell_udp_init();
}

