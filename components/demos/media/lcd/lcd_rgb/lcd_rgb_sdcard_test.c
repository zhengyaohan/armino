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
#include <lcd_rgb_demo.h>
#include "bk_cli.h"
#include "stdio.h"
#include <driver/dma2d.h>
#include "modules/image_scale.h"
#include <lcd_dma2d_config.h>
#include <components/jpeg_decode.h>

#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"
#include "test_fatfs.h"
#endif

#if (CONFIG_BK7256XX_MP)
#else

static dma_transfer_t s_dma_transfer_param = {0};
static dma_id_t lcd_dma_id = DMA_ID_MAX;
#define LCD_FRAMEADDR        0x60000000 /**<define frame base addr */


static void lcd_sd_rgb_isr(void)
{
//	bk_gpio_set_output_high(GPIO_2);

	s_dma_transfer_param.lcd_isr_cnt++;
	if(s_dma_transfer_param.lcd_isr_cnt == 400) {
		s_dma_transfer_param.lcd_isr_cnt = 0;
		bk_dma_stop(lcd_dma_id);
	} else {
		BK_LOG_ON_ERR(bk_dma_start(lcd_dma_id));
	}
//	bk_gpio_set_output_high(GPIO_2);
}

static void dma_sd_finish_isr(dma_id_t id)
{
//	bk_gpio_set_output_high(GPIO_3);
	s_dma_transfer_param.dma_int_cnt ++;

	if (s_dma_transfer_param.dma_int_cnt == s_dma_transfer_param.dma_transfer_cnt) {
		s_dma_transfer_param.dma_int_cnt = 0;
		bk_dma_set_src_start_addr(lcd_dma_id, (uint32_t)LCD_FRAMEADDR);
	}
	else {
		bk_dma_set_src_start_addr(lcd_dma_id, ((uint32_t)LCD_FRAMEADDR + (uint32_t)(s_dma_transfer_param.dma_transfer_len * s_dma_transfer_param.dma_int_cnt)));
		bk_dma_start(lcd_dma_id);
	}
//	bk_gpio_set_output_low(GPIO_3);
}

static void dma_lcd_config(uint32_t dma_ch, uint32_t dma_src_mem_addr,uint32_t dma_dst_width)
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
	dma_config.dst.width = dma_dst_width;
	dma_config.dst.start_addr = rgb_fifo;

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, s_dma_transfer_param.dma_transfer_len));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
}


void lcd_rgb_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	s_dma_transfer_param.dma_int_cnt = 0;
	s_dma_transfer_param.dma_frame_end_flag = 0;
	s_dma_transfer_param.lcd_isr_cnt = 0;
	s_dma_transfer_param.dma_transfer_len = 65280;
	s_dma_transfer_param.dma_transfer_cnt = 4;

	uint32_t rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk_div  = %d \r\n", rgb_clk_div);

	uint32_t yuv_mode = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("yuv_mode  = %d \r\n", yuv_mode);
	
	os_printf("psram init. \r\n");
	bk_psram_init();
//	bk_gpio_enable_output(GPIO_2); //output
//	bk_gpio_enable_output(GPIO_3); //output

	lcd_dma_id = bk_dma_alloc(DMA_DEV_LCD_DATA);
	if ((lcd_dma_id < DMA_ID_0) || (lcd_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}
	os_printf("malloc lcd dma ch is DMA_ch%x \r\n", lcd_dma_id);

	os_printf("lcd driver init. \r\n");
	bk_lcd_driver_init(LCD_96M);

	os_printf("lcd rgb reg init.\r\n");
	bk_lcd_rgb_init(rgb_clk_div,  X_PIXEL_RGB, Y_PIXEL_RGB, yuv_mode);
	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_sd_rgb_isr);
	if (yuv_mode == 0) {
		dma_lcd_config(lcd_dma_id, (uint32_t)LCD_FRAMEADDR, 1);
	} else {
		dma_lcd_config(lcd_dma_id, (uint32_t)LCD_FRAMEADDR, 2);
	}

	BK_LOG_ON_ERR(bk_dma_register_isr(lcd_dma_id, NULL, dma_sd_finish_isr));
}

void lcd_sdcard_read_to_mem_jpeg_dec(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = 0;
	char *filename;
	uint32_t srcaddr;
	uint32_t dstaddr;
	uint32_t total_size = 0;

	filename = argv[1]; //saved file name
	os_printf("filename  = %s \r\n", filename);
	srcaddr = os_strtoul(argv[2], NULL, 16) & 0xFFFFFFFF;
	os_printf("image p_srcaddr  = %X \r\n", srcaddr);
	dstaddr = os_strtoul(argv[3], NULL, 16) & 0xFFFFFFFF;
	os_printf("image p_dstaddr  = %X \r\n", dstaddr);
	uint8_t * p_srcaddr = (uint8_t *)srcaddr;
	uint8_t * p_dstaddr = (uint8_t *)dstaddr;

	err = bk_jpeg_dec_sw_init(p_srcaddr, p_dstaddr);
	if (err != kNoErr) {
		os_printf("init jpeg_decoder failed\r\n");
		return;
	}

#if (CONFIG_SDCARD_HOST)
		char cFileName[FF_MAX_LFN];
		FIL file;
		FRESULT fr;
		FSIZE_t size_64bit = 0;
		unsigned int uiTemp = 0;

		// step 1: read picture from sd to psram
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);
		char *ucRdTemp = (char *)p_srcaddr;
		os_printf("write to psram addr:  %x \r\n", ucRdTemp);

		/*open pcm file*/
		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", filename);
			return;
		}
		size_64bit = f_size(&file);
		total_size = (uint32_t)size_64bit;// total byte
		os_printf("read file total_size = %d.\r\n", total_size);

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
		os_printf("file read ok\r\n");
#else
		os_printf("Not support\r\n");
#endif
	// step 2: start jpeg_dec
	
	os_printf("start jpeg_dec.\r\n");
	bk_jpeg_dec_sw_register_finish_callback(NULL);
	err = bk_jpeg_dec_sw_start(total_size);
	if (err != kNoErr) {
		os_printf("jpeg_decoder failed\r\n");
		return;
	}
	os_printf("jpeg_dec ok.\r\n");
}

void lcd_rgb_sdcard_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;

	uint32_t src_w = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("image src_w  = %d \r\n", src_w);
	uint32_t src_h = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
	os_printf("image src_h  = %d \r\n", src_h);
	uint32_t dst_w =480;
	uint32_t dst_h =272;
	os_printf("image dst_w  = 480 \r\n");
	os_printf("image dst_h  =272 \r\n");

//		uint32_t srcaddr = os_strtoul(argv[7], NULL, 16) & 0xFFFFFFFF;
//		uint32_t dstaddr = os_strtoul(argv[8], NULL, 16) & 0xFFFFFFFF;
	unsigned char *pSrcImg = (unsigned char *) 0x60200000;
//		unsigned char *pDstImg = (unsigned char *) dstaddr;
	unsigned char *pDstImg = (unsigned char *) 0x60000000;

	bk_gpio_set_output_high(GPIO_2);
	if (os_strcmp(argv[1], "compress_only") == 0){
		err = image_16bit_scaling(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "compress_rotate") == 0) { 
		err = image_16bit_scaling_rotate(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "anticlockwise_rotate") == 0) {
		image_16bit_rotate90_anticlockwise(pDstImg, pSrcImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "clockwise_rotate") == 0) {
		image_16bit_rotate90_clockwise(pDstImg, pSrcImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "yuyv_rotate") == 0) {
		yuyv_rotate_degree90(pSrcImg, pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "crop_compress") == 0) {
		err = image_scale_crop_compress(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "only_crop") == 0) {
		image_center_crop(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	}  else if (os_strcmp(argv[1], "dma2d_crop") == 0) {
		dma2d_crop_params_t  crop_params;
		crop_params.dst_addr = (uint32_t)pDstImg;
		crop_params.src_addr = (uint32_t)pSrcImg;
		crop_params.x = (src_w - dst_w)/2;
		crop_params.y = (src_h - dst_h)/2;
		crop_params.src_width = src_w;
		crop_params.src_height = src_h;
		crop_params.dst_width = dst_w;
		crop_params.dst_height = dst_h;
		dma2d_crop_image(&crop_params);
	} else if (os_strcmp(argv[1], "crop_compress_rotate") == 0) {
		err = image_scale_crop_compress_rotate(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "uyvy_to_rgb565") == 0) {
		uyvy_to_rgb565_convert(pSrcImg, pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "yuyv_to_rgb565") == 0) {
		yuyv_to_rgb565_convert(pSrcImg, pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "vuyy_to_rgb888") == 0) {
		vuyy_to_rgb888(pDstImg, pSrcImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "rgb888_to_rgb565") == 0) {
		bk_example_dma2d_rgb888_to_arg565pixel((uint32_t)pSrcImg, (uint32_t)pDstImg, src_w, src_h);
	}else if (os_strcmp(argv[1], "rgb565_to_uyvy") == 0) {
		rgb565_to_uyvy_convert((uint16_t *)pSrcImg, (uint16_t *)pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "rgb565_to_yuyv") == 0) {
		rgb565_to_yuyv_convert((uint16_t *)pSrcImg, (uint16_t *)pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "rgb_blend") == 0) {
		yuv_blend(0x60000000, 0x60200000);
	} else if (os_strcmp(argv[1], "display") == 0) {
		bk_lcd_rgb_display_en(1);
		bk_dma_start(lcd_dma_id);
	}else {
		os_printf("argv[1] error. \r\n");
		return ;
	}
	bk_gpio_set_output_low(GPIO_2);
	if (err != BK_OK) {
		os_printf("img_down_scale error\n");
		return;
	}
}
#endif

