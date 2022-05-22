#include <common/bk_include.h>
#include <stdio.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include "bk_cli.h"

#include "sys_driver.h"
#include <common/bk_err.h>
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/psram.h>
#include <components/jpeg_decode.h>
#include <components/uvc_camera.h>

#if (CONFIG_SDCARD_HOST || CONFIG_USB_HOST)
#include "ff.h"
#include "diskio.h"
#include "test_fatfs.h"
#endif


#define JPEGDEC_DATA_ADDR     (0x60010000)
#define PSRAM_BASEADDR        (0x60000000)
#define DMA_TRANSFER_LEN      0xF000//640*480/5;480*272/2;

extern void delay(INT32 num);

static uint8_t dma_int_cnt = 0;
static uint8_t dma_channel_id = 0;
static bool g_uvc_enable = false;

static void cli_jpegdec_help(void)
{
	os_printf("lcd_jpegdec {init|display_picture|stop_display|uvc_dispaly_init|start_uvc|close_uvc|deinit}\n");
}

static void lcd_display_frame_isr(void)
{
#if CONFIG_USB_UVC
	if (g_uvc_enable) {
		bk_lcd_rgb_display_en(0);// close rgb display
		bk_uvc_set_mem_status(UVC_MEM_IDLE);
	}
#endif
}

static void dma_jpeg_dec_to_lcdfifo_isr(dma_id_t id)
{
	uint32_t source_start_addr;
	dma_int_cnt++;
	if(dma_int_cnt == 10)
	{
		if (g_uvc_enable) {
			dma_int_cnt = 0;
			bk_dma_stop(dma_channel_id);
			source_start_addr = (uint32_t)JPEGDEC_DATA_ADDR;
			bk_dma_set_src_addr(dma_channel_id, source_start_addr, 0);
		} else {
			dma_int_cnt = 0;
			source_start_addr = (uint32_t)JPEGDEC_DATA_ADDR;
			bk_dma_set_src_addr(dma_channel_id, source_start_addr, 0);
			bk_dma_start(dma_channel_id);
		}
	}
	else {
		source_start_addr = (uint32_t)JPEGDEC_DATA_ADDR + (uint32_t)(DMA_TRANSFER_LEN * dma_int_cnt);
		bk_dma_set_src_addr(dma_channel_id, source_start_addr, 0);
		bk_dma_start(dma_channel_id);
	}
}

static void dma_jpeg_dec_to_lcdfifo(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) JPEGDEC_DATA_ADDR;
	dma_config.src.end_addr = (uint32) JPEGDEC_DATA_ADDR + DMA_TRANSFER_LEN;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr =  (uint32) REG_DISP_RGB_FIFO;

	dma_channel_id = bk_dma_alloc(DMA_DEV_DTCM);
	if ((dma_channel_id < DMA_ID_0) || (dma_channel_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}

	BK_LOG_ON_ERR(bk_dma_init(dma_channel_id, &dma_config));

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_channel_id, DMA_TRANSFER_LEN));
	BK_LOG_ON_ERR(bk_dma_register_isr(dma_channel_id, NULL, dma_jpeg_dec_to_lcdfifo_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_channel_id));
}

static void uvc_jpegdec_frame_end_callback(void)
{
	bk_lcd_rgb_display_en(1);
	bk_dma_start(dma_channel_id);
}

#if CONFIG_USB_UVC
static void uvc_jpeg_frame_end_callback(uint32_t pic_size)
{
	bk_jpeg_dec_sw_fun((uint8_t *)PSRAM_BASEADDR, (uint8_t *)JPEGDEC_DATA_ADDR, pic_size);
}
#endif // CONFIG_USB_UVC

void lcd_jpeg_dec_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = 0;
	uint32_t rgb_clk_div = 0;
	uint16_t width = 640, height = 480;
	uint8_t ratio = 0;

	if (argc < 2) {
		cli_jpegdec_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		uint32_t mode = 0x00054043;
		if (argc < 5) {
			os_printf("init: width height ratio\r\n");
			return;
		}

		width = os_strtoul(argv[2], NULL, 10);
		height = os_strtoul(argv[3], NULL, 10);
		ratio = os_strtoul(argv[4], NULL, 10);
		rgb_clk_div = os_strtoul(argv[5], NULL, 10);

		// step 1: init psram
		err = bk_psram_init(mode);
		if (err != kNoErr) {
			os_printf("init psram failed\r\n");
			return;
		}

		// step 2: video power_up
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_ON);

		// step 3: init lcd
		bk_lcd_driver_init(LCD_96M);
		if (rgb_clk_div == 0)
			rgb_clk_div = 25;
		bk_lcd_rgb_init(rgb_clk_div, width, height, YYUV_DATA);

		bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_display_frame_isr);

		// step 4: init lcd_dma, jpeg_dec to rgb_fifo
		dma_jpeg_dec_to_lcdfifo();

		// step 5: init jpeg_dec
		err = bk_jpeg_dec_sw_init(width, height, ratio);
		if (err != kNoErr) {
			os_printf("init jpeg_decoder failed\r\n");
			return;
		}

		//jpeg_sw_decode_register_finish_callback(uvc_jpegdec_frame_end_callback);

		// step 6: f_mount sdcard if need read sdcard
#if (CONFIG_SDCARD_HOST || CONFIG_USB_HOST)
		test_mount(DISK_NUMBER_SDIO_SD);
#endif
	} else if (os_strcmp(argv[1], "picture_jpegdec_to_sdcard") == 0) {
#if (CONFIG_SDCARD_HOST || CONFIG_USB_HOST)
		char *filename = NULL;
		char *fileout = NULL;
		char cFileName[FF_MAX_LFN];
		FIL file;
		FRESULT fr;
		FSIZE_t size_64bit = 0;
		char *ucRdTemp = (char *)PSRAM_BASEADDR;
		unsigned int uiTemp = 0;

		if (argc != 4) {
			os_printf("picture_jpegdec_to_sdcard: picture_name output_file_name\r\n");
			return;
		}

		filename = argv[2];
		fileout = argv[3];

		// step 1: read picture from sd to psram
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", filename);
			return;
		}

		size_64bit = f_size(&file);

		uint32_t total_size = (uint32_t)size_64bit;// total byte
		fr = f_read(&file, ucRdTemp, total_size, &uiTemp);
		if (fr != FR_OK) {
			os_printf("read file fail.\r\n");
			return;
		}

		fr = f_close(&file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", filename);
			return;
		}

		// step 2: start jpeg_dec
		bk_jpeg_dec_sw_register_finish_callback(NULL);
		err = bk_jpeg_dec_sw_fun((uint8_t *)PSRAM_BASEADDR, (uint8_t *)JPEGDEC_DATA_ADDR, total_size);
		if (err != kNoErr) {
			os_printf("jpeg_decoder failed\r\n");
			return;
		}

		// step 3: save jpeg_dec data to sdcard
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, fileout);

		fr = f_open(&file, cFileName, FA_OPEN_APPEND | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", fileout);
			return;
		}

		fr = f_write(&file, (char *)JPEGDEC_DATA_ADDR, width * height * 2, &uiTemp);
		if (fr != FR_OK) {
			os_printf("write %s fail.\r\n", fileout);
			return;
		}

		fr = f_close(&file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", fileout);
			return;
		}
#else
		os_printf("Not support\r\n");
#endif
	}else if (os_strcmp(argv[1], "display_picture") == 0) {
#if (CONFIG_SDCARD_HOST || CONFIG_USB_HOST)
		char *filename = NULL;
		char cFileName[FF_MAX_LFN];
		FIL file;
		FRESULT fr;
		FSIZE_t size_64bit = 0;
		char *ucRdTemp = (char *)PSRAM_BASEADDR;
		unsigned int uiTemp = 0;

		if (argc != 3) {
			os_printf("display_picture: picture_name\r\n");
			return;
		}

		filename = argv[2];

		// step 1: stop dma from jpeg_dec data to rgb fifo, if rgb display is working
		bk_dma_stop(dma_channel_id);
		dma_int_cnt = 0;

		// step 2: read picture from sd to psram
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", filename);
			return;
		}

		size_64bit = f_size(&file);

		uint32_t total_size = (uint32_t)size_64bit;// total byte
		fr = f_read(&file, ucRdTemp, total_size, &uiTemp);
		if (fr != FR_OK) {
			os_printf("read file fail.\r\n");
			return;
		}

		fr = f_close(&file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", filename);
			return;
		}

		// step 3: disable uvc, for uvc_jpegdec_frame_end_callback
		g_uvc_enable = false;

		// step 4: in jpeg_dec finish callback start dma
		bk_jpeg_dec_sw_register_finish_callback(uvc_jpegdec_frame_end_callback);

		// step 5: start jpeg_dec
		err = bk_jpeg_dec_sw_fun((uint8_t *)PSRAM_BASEADDR, (uint8_t *)JPEGDEC_DATA_ADDR, total_size);
		if (err != kNoErr) {
			os_printf("jpeg_decoder failed\r\n");
			return;
		}
#else
		os_printf("Not support\r\n");
#endif
	} else if (os_strcmp(argv[1], "stop_display") == 0) {
		// step 1: stop dma
		bk_dma_stop(dma_channel_id);
		dma_int_cnt = 0;

		// step 2: stop display
		bk_lcd_rgb_display_en(0);
	} else if (os_strcmp(argv[1], "uvc_display_init") == 0) {
#if CONFIG_USB_UVC
		uint8_t fps = 0;
		uint16_t ppi = 0;

		if (argc != 4) {
			os_printf("uvc_dispaly_init: pps fps\r\n");
			return;
		}

		ppi = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
		fps = os_strtoul(argv[3], NULL, 10) & 0xFF;

		// step 1: stop dma from jpeg_dec data to rgb fifo, if rgb display is working
		bk_dma_stop(dma_channel_id);
		dma_int_cnt = 0;

		// step 2: init uvc
		err = bk_uvc_init();
		if (err != kNoErr) {
			os_printf("uvc init failed!\r\n");
			return;
		}

		// step 3: set uvc_pps_fps if need
		err = bk_uvc_set_ppi_fps(ppi, fps);
		if (err != kNoErr) {
			os_printf("uvc set ppi and fps failed!\r\n");
			return;
		}

		// step 4: register frame_end_callback, and excute jpeg_dec
		bk_uvc_register_frame_end_callback(uvc_jpeg_frame_end_callback);

		// step 5: reset lcd_dma if need

		// step 6 enable lcd_dma in jpeg_dec_finish_callback
		bk_jpeg_dec_sw_register_finish_callback(uvc_jpegdec_frame_end_callback);
#else
		os_printf("Not support\r\n");
#endif
	}else if (os_strcmp(argv[1], "start_uvc") == 0) {
#if CONFIG_USB_UVC
		if (argc != 2) {
			os_printf("start_uvc: error\r\n");
			return;
		}
		// step 1: stop dma
		bk_dma_stop(dma_channel_id);
		dma_int_cnt = 0;

		// step 2: enable uvc_enable flag
		g_uvc_enable = true;

		// step 3: start uvc
		err = bk_uvc_set_start();
		if (err != kNoErr) {
			os_printf("uvc set start failed!\r\n");
			return;
		}
#else
		os_printf("Not support\r\n");
#endif
	} else if (os_strcmp(argv[1], "close_uvc") == 0) {
#if CONFIG_USB_UVC
		if (argc != 2) {
			os_printf("close_uvc: error\r\n");
			return;
		}

		// step 1: stop uvc
		err = bk_uvc_set_stop();
		if (err != kNoErr) {
			os_printf("uvc set stop failed!\r\n");
			return;
		}

		// step 2: diasble uvc_enable flag
		g_uvc_enable = false;
#else
		os_printf("Not support\r\n");
#endif
	}else if (os_strcmp(argv[1], "deinit") == 0) {
		if (argc != 2) {
			os_printf("deinit: error\r\n");
			return;
		}

		// step 1: deinit uvc if need
#if CONFIG_USB_UVC
		err = bk_uvc_deinit();
		if (err != kNoErr) {
			os_printf("uvc deinit failed!\r\n");
			return;
		}
#else

		// step 2: deinit jpeg_dec
		bk_jpeg_dec_sw_deinit();

		// step 3: stop dma and deinit
		err = bk_dma_deinit(dma_channel_id);
		if (err != kNoErr) {
			os_printf("dma deinit failed!\r\n");
			return;
		}

		err = bk_dma_free(DMA_DEV_DTCM, dma_channel_id);
		if (err != BK_OK) {
			os_printf("dma free failed!\r\n");
			return;
		}

		dma_channel_id = 0;
		dma_int_cnt = 0;

		// step 3: deinit lcd
		bk_lcd_rgb_display_en(0);

		// step 4: video power off
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_OFF);

		// step 5: deinit psram
		delay(2000);
		err = bk_psram_deinit();
		if (err != kNoErr) {
			os_printf("uvc deinit failed!\r\n");
			return;
		}
#endif
		g_uvc_enable = false;
	} else {
		// jpeg_dec cmd help
		cli_jpegdec_help();
		return;
	}
}



