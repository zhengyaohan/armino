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


static dma_transfer_t s_dma_transfer_param = {0};
#define           LCD_FRAMEADDR    0x60000000   /**<define frame base addr */
#define           LCD_FG_IMG_ADDR    0x60200000   /**<define frame base addr */

static uint32_t*  jpeg_dec_buff =  (uint32_t *)0x60000000;
static uint32_t*  jpeg_buff     = (uint32_t *)0x30000000;
static dma_id_t   lcd_dma_id    = DMA_ID_MAX;
static dma_id_t   jpeg_dma_id   = DMA_ID_MAX;


//static void lcd_rgb_isr(void)
//{
//	s_dma_transfer_param.lcd_isr_cnt++;
//	if(s_dma_transfer_param.lcd_isr_cnt == 400) {
//		s_dma_transfer_param.lcd_isr_cnt = 0;
//		bk_dma_stop(lcd_dma_id);
//	}
//}


static void jpeg_enc_end_of_yuv_cb(jpeg_unit_t id, void *param)
{
	//bk_jpeg_enc_set_yuv_mode(0);
	BK_LOG_ON_ERR(bk_dma_start(lcd_dma_id));
}

static void jpeg_enc_end_of_yuv_blend_cb(jpeg_unit_t id, void *param)
{
	yuv_blend(LCD_FRAMEADDR, LCD_FG_IMG_ADDR);
	BK_LOG_ON_ERR(bk_dma_start(lcd_dma_id));
}

static void jpeg_enc_end_of_frame_cb(jpeg_unit_t id, void *param)
{
	//bk_gpio_set_output_high(GPIO_2);
	//jpeg enc off
	//bk_jpeg_enc_set_enable(0);
	bk_dma_stop(jpeg_dma_id);

	//jpeg dec on
	bk_jpeg_dec_init(jpeg_buff, jpeg_dec_buff);
	//bk_gpio_set_output_low(GPIO_2);
}


static void jpeg_dec_end_of_frame_cb()
{
//	bk_gpio_set_output_high(GPIO_4);
	//lcd display open
	bk_dma_start(lcd_dma_id);

	//jpeg enc open
	//bk_jpeg_enc_set_enable(1);
	bk_dma_start(jpeg_dma_id);
	
//	bk_gpio_set_output_low(GPIO_4);
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
	}
	else {
		bk_dma_set_src_start_addr(lcd_dma_id, ((uint32_t)LCD_FRAMEADDR + (uint32_t)(s_dma_transfer_param.dma_transfer_len * s_dma_transfer_param.dma_int_cnt)));
		bk_dma_start(lcd_dma_id);
	}
	//bk_gpio_set_output_low(GPIO_3);
}


void dma_jpeg_config(uint32_t dma_ch)
{
	dma_config_t dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_JPEG;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = JPEG_RX_FIFO;
	dma_config.dst.start_addr = (uint32_t)jpeg_buff;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, 65536));
	BK_LOG_ON_ERR(bk_dma_start(dma_ch));
}

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
	BK_LOG_ON_ERR(bk_dma_register_isr(dma_ch, NULL, dma_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
}

static void lcd_rgb_display_init(uint32_t rgb_clk_div, uint32_t yuv_mode, uint32_t dma_width)
{
	s_dma_transfer_param.dma_int_cnt = 0;
	s_dma_transfer_param.dma_frame_end_flag = 0;
	s_dma_transfer_param.lcd_isr_cnt = 0;
	s_dma_transfer_param.dma_transfer_len = 65280;
	s_dma_transfer_param.dma_transfer_cnt = 4;

	os_printf("lcd power on. \r\n");
	//bk_lcd_power_on_ctrl(1);
	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());
	os_printf("psram init. \r\n");
	bk_psram_init(0x00054043);

//	bk_gpio_enable_output(GPIO_2);	//output
//	bk_gpio_enable_output(GPIO_3);	//output
//	bk_gpio_enable_output(GPIO_4); //output

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

//	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_rgb_isr);
	dma_lcd_config(0, lcd_dma_id, (uint32_t)LCD_FRAMEADDR, dma_width);
	BK_LOG_ON_ERR(bk_dma_register_isr(lcd_dma_id, NULL, dma_finish_isr));
}


void lcd_rgb_display_yuv(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t dev = 3; // gc0328c
	uint32_t camera_cfg = 0;
	if (argc < 2) {
		os_printf("input param error\n");
		return;
	}
	rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());
	lcd_rgb_display_init(rgb_clk_div, ORGINAL_YUYV_DATA, 2);
	os_printf("rgb_clk_div	= %d \r\n", rgb_clk_div);
	os_printf("yuv mode default yuyv \r\n");

	fps = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("fps	= %dHz \r\n", fps);
	camera_cfg = (272 << 16) | fps;

	jpeg_config.yuv_mode = 1;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
	jpeg_config.sys_clk_div = 4;
	jpeg_config.mclk_div = 0;
	bk_jpeg_enc_register_isr(END_OF_YUV, jpeg_enc_end_of_yuv_cb, NULL);
	err = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (err != kNoErr) {
		os_printf("jpeg init error\n");
		return;
	}

	i2c_config.baud_rate = 100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != kNoErr) {
		os_printf("i2c init error\n");
		return;
	}

	err = bk_camera_set_param(dev, camera_cfg);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		return;
	}

	bk_camera_sensor_config();
	bk_lcd_rgb_display_en(1);
}


void lcd_rgb_display_yuv_blending(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t dev = 3; // gc0328c
	uint32_t camera_cfg = 0;

	char filename[] ="yuyv422_120_56_rgb"; 
	char *file_name = filename;

	if (argc < 2) {
		os_printf("input param error\n");
		return;
	}
	rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());
	lcd_rgb_display_init(rgb_clk_div, ORGINAL_YUYV_DATA, 2);
	os_printf("rgb_clk_div	= %d \r\n", rgb_clk_div);
	os_printf("yuv mode default yuyv \r\n");

	lcd_sdcard_read_to_mem(file_name, LCD_FG_IMG_ADDR);

	fps = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("fps	= %dHz \r\n", fps);
	camera_cfg = (272 << 16) | fps;

	jpeg_config.yuv_mode = 1;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
	jpeg_config.sys_clk_div = 4;
	jpeg_config.mclk_div = 0;
	bk_jpeg_enc_register_isr(END_OF_YUV, jpeg_enc_end_of_yuv_blend_cb, NULL);
	err = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (err != kNoErr) {
		os_printf("jpeg init error\n");
		return;
	}

	i2c_config.baud_rate = 100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != kNoErr) {
		os_printf("i2c init error\n");
		return;
	}

	err = bk_camera_set_param(dev, camera_cfg);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		return;
	}

	bk_camera_sensor_config();
	bk_lcd_rgb_display_en(1);
}

void lcd_rgb_display_jpeg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t dev = 3;// gc0328c
	uint32_t camera_cfg = 0;


	err=bk_jpeg_dec_driver_init();
	if (err != BK_OK)
		return;
	os_printf("jpegdec driver init successful.\r\n");

	rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk_div  = %d \r\n", rgb_clk_div);
	
	fps = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("fps  = %dHz \r\n", fps);

	jpeg_dma_id = bk_dma_alloc(DMA_DEV_JPEG);
	if ((jpeg_dma_id < DMA_ID_0) || (jpeg_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc jpeg dma fail \r\n");
		return;
	}
	os_printf("malloc jpeg dma ch is DMA_ch%x \r\n", jpeg_dma_id);

	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());

	lcd_rgb_display_init(rgb_clk_div, VUYY_DATA, 2);
	os_printf("rgb_clk_div	= %d \r\n", rgb_clk_div);
	os_printf("yuv mode default vyuy \r\n");

	bk_jpeg_enc_register_isr(END_OF_FRAME, jpeg_enc_end_of_frame_cb, NULL);
	bk_jpeg_dec_complete_cb(jpeg_dec_end_of_frame_cb, NULL);
	
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
		return;
	}

	os_printf("jpeg dma config\r\n");
	dma_jpeg_config(jpeg_dma_id);

	i2c_config.baud_rate = 100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != BK_OK) {
		os_printf("i2c init error\r\n");
		return;
	}

	err = bk_camera_set_param(dev, camera_cfg);
	if (err != BK_OK) {
		os_printf("set camera ppi and fps error\n");
		return;
	}
	bk_camera_sensor_config();
	bk_lcd_rgb_display_en(1);
}

void lcd_rgb_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_memset(&s_dma_transfer_param, 0, sizeof(s_dma_transfer_param));

	if (os_strcmp(argv[1], "yuv_display") == 0) {
		if (bk_dma_deinit(lcd_dma_id) != BK_OK) {
			os_printf("deinit lcd dma error\r\n");
		}
		if (bk_dma_free(DMA_DEV_LCD_DATA, lcd_dma_id) != BK_OK) {
			os_printf("free lcd dma: %d error\r\n", lcd_dma_id);
		}
		if( bk_i2c_deinit(CONFIG_CAMERA_I2C_ID) !=  BK_OK) {
			os_printf("i2c deinit error\n");
		}
		if (bk_jpeg_enc_dvp_deinit() != BK_OK) {
			os_printf("deinit jpeg enc error \r\n");
		}
		if (bk_jpeg_enc_driver_deinit() != BK_OK) {
			os_printf("deinit jpeg enc error \r\n");
		}
		if (bk_lcd_rgb_deinit() != BK_OK) {
			os_printf("deinit rgb lcd error \r\n");
		}
		if (bk_psram_deinit() != BK_OK) {
			os_printf("psram deinit error \r\n");
		}
		os_printf("rgb lcd closed. \r\n");
	} else if (os_strcmp(argv[1], "jpeg_display") == 0) {
		if (bk_dma_deinit(lcd_dma_id) != BK_OK) {
			os_printf("deinit lcd dma error\r\n");
		}
		if (bk_dma_free(DMA_DEV_LCD_DATA, lcd_dma_id) != BK_OK) {
			os_printf("free lcd dma: %d error\r\n", lcd_dma_id);
		}
		if (bk_dma_deinit(jpeg_dma_id) != BK_OK) {
			os_printf("deinit jpeg dma error \r\n");
		}
		if (bk_dma_free(DMA_DEV_JPEG, jpeg_dma_id) != BK_OK) {
			os_printf("free jpeg dma: %d error\r\n", jpeg_dma_id);
		}

		if( bk_i2c_deinit(CONFIG_CAMERA_I2C_ID) !=  BK_OK) {
			os_printf("i2c deinit error\n");
		}

		if( bk_jpeg_dec_driver_deinit() !=  BK_OK) {
			os_printf("jpeg dec deinit error\n");
		}

		if (bk_jpeg_enc_dvp_deinit() != BK_OK) {
			os_printf("deinit jpeg enc error\r\n");
		}
		if (bk_jpeg_enc_driver_deinit() != BK_OK) {
			os_printf("deinit jpeg enc error \r\n");
		}

		if (bk_lcd_rgb_deinit() !=BK_OK) {
			os_printf("deinit rgb \r\n");
		}

		if (bk_psram_deinit() !=BK_OK) {
			os_printf("psram deinit error. \r\n");
		}
		
		os_printf("rgb lcd closed. \r\n");
	} else {
		os_printf("cmd: lcd_rgb_close=yuv_display|jpeg_display\r\n");
	}
}

