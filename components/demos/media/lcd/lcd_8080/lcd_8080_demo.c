#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdlib.h>
#include <components/system.h>
#include "driver/lcd_disp_types.h"
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/psram.h>
#include "bk_cli.h"
#include "stdio.h"
#include <lcd_dma2d_config.h>
#include <st7796s.h>
#include "modules/image_scale.h"

#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"
#include "test_fatfs.h"
#endif

typedef struct {
	volatile uint32_t dma_int_cnt;
	uint32_t          dma_transfer_cnt;
	volatile uint32_t lcd_isr_cnt;
	uint32_t          dma_transfer_len;
	volatile uint32_t dma_frame_end_flag;
} dma_transfer_t;


static dma_transfer_t s_dma_transfer_param = {0};
#define LCD_FRAMEADDR   0x60000000 /**<define frame base addr */
static dma_id_t lcd_dma_id = DMA_ID_MAX;

static void lcd_i8080_isr(void)
{
//	bk_gpio_set_output_high(GPIO_2);
#if(1)
	bk_lcd_rgb_display_en(0);
	bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);
	bk_lcd_rgb_display_en(1);
	bk_lcd_8080_write_cmd(RAM_WRITE);
#else
	//use this appear asic bug, nest resion modify
	bk_lcd_8080_write_cmd(continue_write);
#endif
//	bk_gpio_set_output_low(GPIO_2);
}


static void dma_finish_isr(dma_id_t id)
{
	//bk_gpio_set_output_high(GPIO_3);
	s_dma_transfer_param.dma_int_cnt ++;
	if (s_dma_transfer_param.dma_int_cnt == s_dma_transfer_param.dma_transfer_cnt)
	{
		s_dma_transfer_param.dma_int_cnt = 0;
		bk_dma_set_src_start_addr(lcd_dma_id, (uint32_t)LCD_FRAMEADDR);
		s_dma_transfer_param.dma_frame_end_flag = 1;
		//BK_LOG_ON_ERR(bk_dma_start(lcd_dma_id));
	}
	else {
		bk_dma_set_src_start_addr(lcd_dma_id, ((uint32_t)LCD_FRAMEADDR + (uint32_t)(s_dma_transfer_param.dma_transfer_len * s_dma_transfer_param.dma_int_cnt)));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(lcd_dma_id));
		bk_dma_start(lcd_dma_id);
	}
	//bk_gpio_set_output_low(GPIO_3);
}

static void dma_pre_config(uint32_t dma_ch)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) LCD_FRAMEADDR;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) LCD_FRAMEADDR + 0x50000;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, 640)); 
	BK_LOG_ON_ERR(bk_dma_register_isr(dma_ch, NULL, dma_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
	BK_LOG_ON_ERR(bk_dma_start(dma_ch));
}

/**
  * @brief dma transfer lcd config
  * @param1  is_8080_if 1：8080 interface, 0 rgb interface
  * @param2  dma chanal
  * @param3  dma_src_mem_addr dma src addr
  * @param4   dma_dst_width 0:8bit, 1:16bit, 2:32bit
  * @return none
  */
static void dma_lcd_config(uint8_t is_8080_if, uint32_t dma_ch, uint32_t dma_src_mem_addr,uint32_t dma_dst_width)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) dma_src_mem_addr;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = dma_dst_width;
	if(is_8080_if) //8080 fifo
		dma_config.dst.start_addr = (uint32) REG_DISP_DAT_FIFO;
	else //rgb fifo
		dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, s_dma_transfer_param.dma_transfer_len));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
}


void lcd_help(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("lcd_8080_init=start \r\n.");
	os_printf("lcd_fill=x,y w,h,color \r\n.");
	os_printf("lcd_8080_close\r\n.");
	os_printf("lcd_8080_sdcard_test=blend|rotate\r\n.");
}


void lcd_8080_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	lcd_dma_id = bk_dma_alloc(DMA_DEV_LCD_DATA);
	if ((lcd_dma_id < DMA_ID_0) || (lcd_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}
	os_printf("malloc lcd dma ch is DMA_ch%x \r\n", lcd_dma_id);

//	bk_gpio_enable_output(GPIO_2);	//output
//	bk_gpio_enable_output(GPIO_3);	//output

	if (os_strcmp(argv[1], "start") == 0) {
		s_dma_transfer_param.dma_transfer_len = 61440;
		s_dma_transfer_param.dma_transfer_cnt = 5;
		s_dma_transfer_param.dma_frame_end_flag = 0; 
		s_dma_transfer_param.dma_int_cnt = 0;

		os_printf("lcd driver init. \r\n");
		bk_lcd_driver_init(LCD_96M);

		os_printf("psram init. \r\n");
		bk_psram_init(0x00054043);

		os_printf("i8080 lcd init. \r\n");
		bk_lcd_8080_init(X_PIXEL_8080, Y_PIXEL_8080);

		os_printf("st7796 init. \r\n");
		ret = st7796s_init();
		if (ret != BK_OK) {
			os_printf("st7796s init failed\r\n");
			return;
		}

		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr);
		os_printf("i8080 pre dma1 config. \r\n");
		dma_pre_config(lcd_dma_id);
		os_printf("wait dma pre transfer done. \r\n");

		while(s_dma_transfer_param.dma_frame_end_flag == 0);
		s_dma_transfer_param.dma_frame_end_flag = 0;

		dma_lcd_config(1, lcd_dma_id, (uint32) LCD_FRAMEADDR, 1);
		os_printf("8080 lcd start transfer. \r\n");
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(RAM_WRITE);
	}
	else {
		os_printf("cmd is : lcd_8080_init = start \r\n");
	}
}



void lcd_fill(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint16_t x, y, w, h, color;

	uint32_t frameaddr = 0x60000000;
	x = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("x = %d \r\n", x);

	y = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("y = %d \r\n", y);

	w = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
	os_printf("w = %d \r\n", w);

	h= os_strtoul(argv[4], NULL, 10) & 0xFFFF;
	os_printf("h = %d \r\n", h);
	
	color = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
	os_printf("fill_color = %x \r\n", color);

	dma2d_lcd_fill(frameaddr, x, y, w, h, color);
	BK_LOG_ON_ERR(bk_dma_start(lcd_dma_id));
}


void lcd_8080_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	os_memset(&s_dma_transfer_param, 0, sizeof(s_dma_transfer_param));
	os_printf("deinit lcd 8080 \r\n");
	bk_lcd_8080_deinit();
	bk_dma_deinit(lcd_dma_id);
	ret = bk_dma_free(DMA_DEV_LCD_DATA, lcd_dma_id);
	if (ret == BK_OK) {
		os_printf("free dma: %d success\r\n", lcd_dma_id);
		return;
	}
	os_printf("deinit lcd dma and free lcd dma ch\r\n");
	bk_psram_deinit();
	os_printf("psram deinit. \r\n");
	os_printf("lcd power down. \r\n");
	os_printf("lcd 8080 closed. \r\n");
}



void lcd_8080_sdcard_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint32_t src_w = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("image src_w	= %d \r\n", src_w);
	uint32_t src_h = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
	os_printf("image src_h	= %d \r\n", src_h);

	uint32_t dst_w =320;
	uint32_t dst_h =480;

//		uint32_t srcaddr = os_strtoul(argv[7], NULL, 16) & 0xFFFFFFFF;
//		uint32_t dstaddr = os_strtoul(argv[8], NULL, 16) & 0xFFFFFFFF;
	unsigned char *pSrcImg = (unsigned char *) 0x60200000;
//		unsigned char *pDstImg = (unsigned char *) dstaddr;
	unsigned char *pDstImg = (unsigned char *) 0x60000000;

	//bk_gpio_set_output_high(GPIO_2);
	if (os_strcmp(argv[1], "compress_only") == 0){
		err = image_16bit_scaling(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "compress_rotate") == 0) { 
		err = image_16bit_scaling_rotate(pSrcImg, pDstImg, src_w, src_h, dst_w, dst_h);
	} else if (os_strcmp(argv[1], "anticlockwise_rotate") == 0) {
		image_16bit_rotate90_anticlockwise(pDstImg, pSrcImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "clockwise_rotate") == 0) {
		image_16bit_rotate90_clockwise(pDstImg, pSrcImg, src_w, src_h);
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
	} else if (os_strcmp(argv[1], "rgb565_to_uyvy") == 0) {
		rgb565_to_uyvy_convert((uint16_t *)pSrcImg, (uint16_t *)pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "rgb565_to_yuyv") == 0) {
		rgb565_to_yuyv_convert((uint16_t *)pSrcImg, (uint16_t *)pDstImg, src_w, src_h);
	} else if (os_strcmp(argv[1], "rgb565_blend") == 0) {
		uint32_t bgimg_w = os_strtoul(argv[2], NULL, 10) & 0xFFFF; //100
		uint32_t bgimg_h = os_strtoul(argv[3], NULL, 10) & 0xFFFF; //60
		uint32_t bg_offset = dst_w - bgimg_w;
		dma2d_blend_rgb565_data(pSrcImg, pDstImg, pDstImg, 0, bg_offset, bg_offset, bgimg_w, bgimg_h, 80);
	} else {
		os_printf("argv[1] error. \r\n");
		return ;
	}
	bk_dma_start(lcd_dma_id);
	//bk_gpio_set_output_low(GPIO_2);
	if (err != BK_OK) {
		os_printf("img_down_scale error\n");
		return;
	}
}


