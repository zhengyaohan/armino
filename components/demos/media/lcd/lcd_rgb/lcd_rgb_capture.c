#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdlib.h>
#include <components/system.h>
#include "driver/lcd_disp_types.h"
#include <driver/jpeg_enc.h>
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/psram.h>
#include <components/video_transfer.h>
#include <components/dvp_camera.h>
#include <driver/i2c.h>
#include <driver/jpeg_dec.h>
#include "bk_cli.h"
#include "stdio.h"
#include <driver/dma2d.h>
#include <lcd_rgb_demo.h>
#include <lcd_dma2d_config.h>
#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"
#include "test_fatfs.h"
#endif
#include "modules/image_scale.h"
#include "lcd_capture_types.h"
#include "BK7256_RegList.h"
#include "bk_general_dma.h"

#if (!CONFIG_BK7256XX_MP)


extern void delay(int num);

#define TU_QITEM_COUNT      (60)
static beken_thread_t  lcd_capture_thread_hdl = NULL;
static beken_queue_t  lcd_capture_int_msg_que = NULL;
lcd_capture_setup_t lcd_capture_setup_bak = {0};

static dma_transfer_t s_dma_transfer_param = {0};
static uint32_t*  jpeg_dec_buff =  (uint32_t *)0x60000000;
static uint32_t*  jpeg_buff     = (uint32_t *)0x30000000;
static dma_id_t   lcd_dma_id    = DMA_ID_MAX;
static dma_id_t   jpeg_dma_id   = DMA_ID_MAX;
static volatile uint8_t lcd_capture_pre = 0; 
uint32_t frame_len = 0;

#if (CONFIG_SDCARD_HOST)
char *g_filename = NULL;
FIL file;
#endif

static bk_err_t lcd_send_msg(lcd_capture_msg_t msg)
{
	bk_err_t ret;
	if (lcd_capture_int_msg_que) {
		ret = rtos_push_to_queue(&lcd_capture_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("lcd_capture_send_msg failed\r\n");
			return kOverrunErr;
		}
		return ret;
	}
	return kNoResourcesErr;
}

static void jpeg_enc_end_of_frame_cb(jpeg_unit_t id, void *param)
{
	bk_dma_stop(jpeg_dma_id);
	if(lcd_capture_pre) 
	{
		lcd_capture_msg_t msg;
		frame_len = bk_jpeg_enc_get_frame_size();
		msg.op = LCD_CAPTURE;
		lcd_send_msg(msg);

		bk_dma_start(jpeg_dma_id);
		lcd_capture_pre = 0;
	} else {
		bk_jpeg_dec_init(jpeg_buff, jpeg_dec_buff);
	}
}

static void jpeg_dec_end_of_frame_cb()
{
	bk_dma_start(lcd_dma_id);
	bk_dma_start(jpeg_dma_id);
}

static void dma_finish_isr(dma_id_t id)
{
	s_dma_transfer_param.dma_int_cnt ++;
	if (s_dma_transfer_param.dma_int_cnt == s_dma_transfer_param.dma_transfer_cnt)
	{
		s_dma_transfer_param.dma_int_cnt = 0;
		bk_dma_set_src_start_addr(lcd_dma_id, (uint32_t)jpeg_dec_buff);
	}
	else {
		bk_dma_set_src_start_addr(lcd_dma_id, ((uint32_t)jpeg_dec_buff + (uint32_t)(s_dma_transfer_param.dma_transfer_len * s_dma_transfer_param.dma_int_cnt)));
		bk_dma_start(lcd_dma_id);
	}
}

static void dma_lcd_config(uint32_t dma_ch, uint32_t dma_src_mem_addr)
{
	uint32_t rgb_fifo = bk_lcd_get_rgb_data_fifo_addr();

	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) dma_src_mem_addr;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = rgb_fifo;

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, s_dma_transfer_param.dma_transfer_len));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
}

static void dma_jpeg_config(uint32_t dma_ch)
{
	uint32_t jpeg_fifo_addr;

	bk_jpeg_enc_get_fifo_addr(&jpeg_fifo_addr);

	dma_config_t dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_JPEG;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = jpeg_fifo_addr;
	dma_config.dst.start_addr = (uint32_t)jpeg_buff;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, 65536));
	BK_LOG_ON_ERR(bk_dma_start(dma_ch));
}

static void lcd_rgb_display_config(uint32_t x_pixel, uint32_t y_pixel)
{
	s_dma_transfer_param.dma_transfer_len = 65280;
	s_dma_transfer_param.dma_transfer_cnt = 4;
	uint32_t rgb_clk_div = 15;
	rgb_input_data_format_t yuv_mode  = VUYY_DATA;

	lcd_dma_id = bk_dma_alloc(DMA_DEV_LCD_DATA);
	if ((lcd_dma_id < DMA_ID_0) || (lcd_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}
	os_printf("malloc lcd dma ch is DMA_ch%x \r\n", lcd_dma_id);

	os_printf("lcd driver init. \r\n");
	bk_lcd_driver_init(LCD_96M);

	os_printf("lcd rgb reg init.\r\n");
	bk_lcd_rgb_init(rgb_clk_div,  x_pixel, y_pixel, yuv_mode);

//	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_rgb_isr);
	dma_lcd_config(lcd_dma_id, (uint32_t)jpeg_dec_buff);
	BK_LOG_ON_ERR(bk_dma_register_isr(lcd_dma_id, NULL, dma_finish_isr));
	bk_lcd_rgb_display_en(1);
}

static bk_err_t lcd_rgb_jpeg_config(void)
{
	int err = kNoErr;
 	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t dev = 3;// gc0328c
	uint32_t camera_cfg = 0;
	fps = 20;

	jpeg_dma_id = bk_dma_alloc(DMA_DEV_JPEG);
	if ((jpeg_dma_id < DMA_ID_0) || (jpeg_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc jpeg dma fail \r\n");
		return BK_FAIL;
	}
	os_printf("malloc jpeg dma ch is DMA_ch%x \r\n", jpeg_dma_id);

	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());
	bk_jpeg_enc_register_isr(END_OF_FRAME, jpeg_enc_end_of_frame_cb, NULL);

	os_printf("jpeg enc init.\r\n");
	jpeg_config.yuv_mode = 0;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
	jpeg_config.sys_clk_div = 4;
	jpeg_config.mclk_div = 0;
	camera_cfg = (272 << 16) | fps;
	err = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (err != BK_OK) {
		os_printf("jpeg init error\r\n");
		return BK_FAIL;
	}

	os_printf("jpeg dma config\r\n");
	dma_jpeg_config(jpeg_dma_id);

	i2c_config.baud_rate = 100000; 
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != BK_OK) {
		os_printf("i2c init error\r\n");
		return BK_FAIL;
	}

	err = bk_camera_set_param(dev, camera_cfg);
	if (err != BK_OK) {
		os_printf("set camera ppi and fps error\n");
		return BK_FAIL;
	}
	bk_camera_sensor_config();
	return BK_OK;
}

static bk_err_t lcd_rgb_jpegdec_config(void)
{
	int ret = kNoErr;

	ret = bk_jpeg_dec_driver_init();
	if (ret != BK_OK)
		return BK_FAIL;
	os_printf("jpegdec driver init successful.\r\n");

	bk_jpeg_dec_complete_cb(jpeg_dec_end_of_frame_cb, JPEGDEC_X_PIXEL_480);
	return kNoErr;
}

bk_err_t bk_lcd_rgb_capture_process(void)
{

#if (CONFIG_SDCARD_HOST)
	FRESULT fr;
	unsigned int uiTemp = 0;

	char *ucRdTemp = (char *)jpeg_buff;

//	char *ucRdTemp = (char *)lcd_cap_addr;
//	dma_memcpy((char*)lcd_cap_addr, (char *)jpeg_buff, frame_len);

	//	save data to sdcard
	fr = f_write(&file, (char *)ucRdTemp, frame_len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", g_filename);
		return BK_FAIL;
	}
	os_printf("\n");
	fr = f_close(&file);
	if (fr != FR_OK) {
		os_printf("close %s fail!\r\n", g_filename);
		return BK_FAIL;
	}
	os_printf("write file complete\r\n");
	
#else
		os_printf("Not support\r\n");
#endif

	return kNoErr;
}

static void lcd_rgb_display_capture_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	lcd_capture_setup_t *lcd_capture_setup = NULL;
	os_memset(&s_dma_transfer_param, 0, sizeof(s_dma_transfer_param));
	lcd_capture_setup = (lcd_capture_setup_t *)(int)param_data;

	/*  -------------------------step: init PSRAM---------------------------- */
	bk_psram_init();

	/*  -------------------------step: init LCD------------------------ */
	lcd_rgb_display_config(lcd_capture_setup->x_pixel, lcd_capture_setup->y_pixel);

	/*  -------------------------step: init JPEG DEC----------------- */
	lcd_rgb_jpegdec_config();

	/*  -------------------------step: init JPEG--------------------- */
	ret = lcd_rgb_jpeg_config();
	if (ret != BK_OK) {
		os_printf("jpeg init fail \r\n");
		return;
	}

	while(1) {
		lcd_capture_msg_t msg;
		ret = rtos_pop_from_queue(&lcd_capture_int_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case LCD_IDLE:
					break;

				case LCD_CAPTURE:
					ret = bk_lcd_rgb_capture_process();
					if (ret != BK_OK) {
						os_printf(" lcd capture process excute fail \r\n");
					}
					break;

				case LCD_EXIT:
					os_printf("goto lcd capture exit \r\n");
					goto lcd_capture_exit;
					break;

				default:
					break;
			}
		}
	}

lcd_capture_exit:
	/* stop dma */
	bk_dma_stop(jpeg_dma_id);
	bk_dma_deinit(jpeg_dma_id);
	bk_dma_free(DMA_DEV_JPEG, jpeg_dma_id);

	bk_dma_stop(lcd_dma_id);
	bk_dma_deinit(lcd_dma_id);
	bk_dma_free(DMA_DEV_LCD_DATA, lcd_dma_id);

	if( bk_i2c_deinit(CONFIG_CAMERA_I2C_ID) !=	BK_OK) {
		os_printf("i2c deinit error\n");
	}

	bk_jpeg_dec_driver_deinit();
	bk_jpeg_enc_dvp_deinit();
	bk_jpeg_enc_driver_deinit();
	bk_lcd_rgb_deinit();

	/* delate msg queue */
	ret = rtos_deinit_queue(&lcd_capture_int_msg_que);
	if (ret != kNoErr) {
		os_printf("delate message queue fail \r\n");
		return;
	}
	lcd_capture_int_msg_que = NULL;
	os_printf("delate message queue complete \r\n");

	/* delate task */
	lcd_capture_thread_hdl = NULL;
	rtos_delete_thread(NULL);
	os_printf("delate task \r\n");
}

bk_err_t bk_lcd_rgb_display(lcd_capture_setup_t *setup_cfg)
{
	bk_err_t ret = BK_OK;
	if ((! lcd_capture_thread_hdl) && (! lcd_capture_int_msg_que)) {
		os_printf("start lcd capture test \r\n");
		os_memcpy(& lcd_capture_setup_bak, setup_cfg, sizeof(lcd_capture_setup_t));

		ret = rtos_init_queue(&lcd_capture_int_msg_que,
							  "lcd_capture_internal_queue",
							  sizeof(lcd_capture_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr) {
			os_printf("ceate lcd_capture internal message queue failed \r\n");
			return BK_FAIL;
		}
		os_printf("ceate lcd_capture internal message queuecomplete \r\n");

		//creat lcd task
		ret = rtos_create_thread(&lcd_capture_thread_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "lcd_intf",
							 (beken_thread_function_t)lcd_rgb_display_capture_main,
							 4096,
							 (beken_thread_arg_t)&lcd_capture_setup_bak);
		if (ret != kNoErr) {
			os_printf("create lcd capture task fail \r\n");
			rtos_deinit_queue(&lcd_capture_int_msg_que);
			lcd_capture_int_msg_que = NULL;
			lcd_capture_thread_hdl = NULL;
		}
		os_printf("create lcd capture task complete \r\n");
		return kNoErr;
	} else
		return kInProgressErr;
}


void cli_cp0_lcd_rgb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	lcd_capture_msg_t msg;

	if (os_strcmp(argv[1], "display") == 0) {
		os_printf("cp0: start lcd display test \r\n");

		lcd_capture_setup_t config;
		config.x_pixel = X_PIXEL_RGB;
		config.y_pixel = Y_PIXEL_RGB;
		ret = bk_lcd_rgb_display(&config);
		if (ret != BK_OK) {
			os_printf("cp0: init lcd capture task fail \r\n");
			return;
		}
		os_printf("cp0: start  lcd capture test successful\n");
	} else if (os_strcmp(argv[1], "capture") == 0) {
		
#if (CONFIG_SDCARD_HOST)
		char cFileName[FF_MAX_LFN];
		g_filename = argv[2];
		FRESULT fr;
	
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, g_filename);
		fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", g_filename);
			return;
		}

		lcd_capture_pre = 1;
#endif
	}  else if (os_strcmp(argv[1], "close") == 0) {
		msg.op = LCD_EXIT;
		ret = lcd_send_msg(msg);
		if (ret != kNoErr) {
			os_printf("send msg: %d fails \r\n", msg.op);
		}
	} else {
		os_printf(" lcd capture cmd fail\r\n");
		return;
	}

}
#endif
