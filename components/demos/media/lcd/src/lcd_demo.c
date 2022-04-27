#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdlib.h>
#include <components/system.h>
#include "driver/lcd_disp_types.h"
#include "BK7256_RegList.h"
#include <driver/jpeg_enc.h>
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/psram.h>
#include <components/video_transfer.h>
#include <components/dvp_camera.h>
#include <driver/i2c.h>
#include <driver/jpeg_dec.h>
#include <lcd_demo.h>
#include <st7796s.h>
#include "bk_cli.h"


static dma_transfer_t s_dma_transfer_param = {0};
static uint32_t* jpeg_dec_buff = (uint32_t *)0x60000000;
static uint32_t* jpeg_buff = (uint32_t *)0x30000000;

static void lcd_i8080_isr(void)
{
	addAON_GPIO_Reg0x2 = 0x2;
#if(1)
	bk_lcd_rgb_display_en(0);
	bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);
	bk_lcd_rgb_display_en(1);
	bk_lcd_8080_write_cmd(RAM_WRITE);
#else
	//use this appear asic bug, nest resion modify
	bk_lcd_8080_write_cmd(continue_write);
#endif
	addAON_GPIO_Reg0x2 = 0x0;
}

static void lcd_rgb_isr(void)
{
	s_dma_transfer_param.lcd_isr_cnt++;
	if(s_dma_transfer_param.lcd_isr_cnt == 400) {
		s_dma_transfer_param.lcd_isr_cnt = 0;
		bk_lcd_rgb_display_en(0);
		bk_dma_stop(LCD_DMA_CH);
		os_printf("lcd_rgb_close.\r\n");
	}
}

static void jpeg_enc_end_of_yuv_cb(jpeg_unit_t id, void *param)
{
	addAON_GPIO_Reg0x3 = 0x2;
	//os_printf("enter end_of_yuv_frame \r\n");
	//bk_jpeg_set_yuv_mode(0);
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(LCD_DMA_CH));
	//bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(LCD_DMA_CH));
	//bk_jpeg_set_yuv_mode(1);
	addAON_GPIO_Reg0x3 = 0x0;
}


static void jpeg_enc_end_of_frame_cb(jpeg_unit_t id, void *param)
{
	//os_printf("%s, %d\n", __func__, __LINE__);
	addAON_GPIO_Reg0x3 = 0x2;
#if(1)
	//jpegenc_off();
	addJPEG_Reg0x1      &= 0xffffffef;
	addGENER_DMA_Reg0x0 &= 0xfffffffe;
#else
	bk_jpeg_enc_set_enable(0);
	bk_dma_stop(DMA_ID_0);
#endif
	//set_JPEG_DEC_Reg0x8_DEC_CMD(4);
	//set_JPEG_DEC_Reg0x8_DEC_CMD(1);
	bk_jpeg_dec_start();

	addAON_GPIO_Reg0x3 = 0x0;
}


static void jpeg_dec_end_of_frame_cb()
{
	addAON_GPIO_Reg0x4 = 0x2;
	//jpegenc_en();
#if(1)
	addJPEG_Reg0x1 |= 0x10;
	addGENER_DMA_Reg0x0 |= 1;
#else
	bk_jpeg_enc_set_enable(1);
	bk_dma_start(DMA_ID_0);
#endif
	bk_lcd_rgb_display_en(1);
	bk_dma_start(LCD_DMA_CH);
	addAON_GPIO_Reg0x4 = 0x0; 
}


static void dma_finish_isr(dma_id_t id)
{
	addAON_GPIO_Reg0x4 = 0x2;

	s_dma_transfer_param.dma_int_cnt ++;
	if (s_dma_transfer_param.dma_int_cnt == s_dma_transfer_param.dma_transfer_cnt)
	{
		s_dma_transfer_param.dma_int_cnt = 0;
		bk_dma_set_src_start_addr(LCD_DMA_CH, (uint32_t)LCD_FRAMEADDR);
		s_dma_transfer_param.dma_frame_end_flag = 1;
		
	}
	else {
		bk_dma_set_src_start_addr(LCD_DMA_CH, ((uint32_t)LCD_FRAMEADDR + (uint32_t)(s_dma_transfer_param.dma_transfer_len * s_dma_transfer_param.dma_int_cnt)));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(LCD_DMA_CH));
		bk_dma_start(LCD_DMA_CH);
	}
	addAON_GPIO_Reg0x4 = 0x0;
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

static void dma_rgb_color_config(uint32_t dma_ch)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) LCD_FRAMEADDR;
	dma_config.src.end_addr = (uint32_t) LCD_FRAMEADDR + 80; //if unopen "//"code,data transfer a little, then dma halt
	dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE; //
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;

	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;   //
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE; //
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) REG_DISP_RGB_FIFO;
	dma_config.dst.end_addr = (uint32_t) REG_DISP_RGB_FIFO + 4; //

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, 80));
}

void dma_rgb_config(uint32_t dma_ch)
{
	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(dma_ch, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, s_dma_transfer_param.dma_transfer_len));
	BK_LOG_ON_ERR(bk_dma_register_isr(dma_ch, NULL, dma_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_ch));
}


void dma_jpeg_config(uint32_t dma_ch)
{
#if(1)
	dma_config_t dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_REPEAT;
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
#else
	addGENER_DMA_Reg0x1 = (uint32_t)JpegRxBuff;
	addGENER_DMA_Reg0x2 = 0x48000014;
	addGENER_DMA_Reg0x3 = 0x0;
	addGENER_DMA_Reg0x4 = 0x0;
	addGENER_DMA_Reg0x5 = 0x0;
	addGENER_DMA_Reg0x6 = 0x0;
	addGENER_DMA_Reg0x7 = 0xa;
    addGENER_DMA_Reg0x0 = ( 65535 << 16 ) | 0x2a9;
#endif
}

void lcd_help(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("lcd cmd include \r\n.");
	os_printf("lcd_8080_init \r\n.");
	os_printf("lcd_video=8,25 \r\n.");
	os_printf("lcd_video_jpeg_dec=13,25 \r\n.");
	os_printf("lcd_close=lcd8080\r\n.");
	os_printf("lcd_close=lcdrgb\r\n.");
}

void lcd_8080_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (os_strcmp(argv[1], "start") == 0) {
		s_dma_transfer_param.dma_transfer_len = 61440;
		s_dma_transfer_param.dma_transfer_cnt = 5;

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

		//st7796s_set_display_mem_area(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr);

		os_printf("i8080 pre dma1 config. \r\n");
		dma_pre_config(LCD_DMA_CH);

		os_printf("wait dma pre transfer done. \r\n");
		while(s_dma_transfer_param.dma_frame_end_flag == 0);
		s_dma_transfer_param.dma_frame_end_flag = 0;

		os_printf("8080 lcd start transfer. \r\n");
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(RAM_WRITE);
	}
	else {
		os_printf("cmd is : lcd_8080_init = start \r\n");
	}

}

void lcd_rgb_color(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t color_datas[20] = {0};
	uint32_t color  = 0;

	os_printf("lcd driver init. \r\n");
	bk_lcd_driver_init(LCD_96M);

	os_printf("psram init. \r\n");
	bk_psram_init(0x00054043);

	color = os_strtoul(argv[1], NULL, 16) & 0xFFFFFFFF;
	for(int i = 0; i< 20; i++) {
		color_datas[i] = color;
	}
	os_printf("display color is %x\r\n", color_datas[19]);
	os_memcpy((void *)jpeg_dec_buff, &color_datas[0], 80);
	
	dma_rgb_color_config(LCD_DMA_CH);
	bk_lcd_rgb_init(5,X_PIXEL_RGB, Y_PIXEL_RGB, RGB565_DATA);
	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_rgb_isr);

	os_printf("lcd rgb disply enable.\r\n");
	bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(LCD_DMA_CH));
}

void lcd_video(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t psram_mode = 0x00054043;
	
	s_dma_transfer_param.dma_transfer_len = 65280;
	s_dma_transfer_param.dma_transfer_cnt = 4;

	rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk_div  = %d \r\n", rgb_clk_div);
	
	fps = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("fps  = %dHz \r\n", fps);

	if (argc < 2) {
		os_printf("input param error\n");
		return;
	}
	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());

	err = bk_psram_init(psram_mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		return;
	}

	os_printf("pre dma init.\r\n");
	dma_pre_config(LCD_DMA_CH);

	while(s_dma_transfer_param.dma_frame_end_flag == 0) {};
	s_dma_transfer_param.dma_frame_end_flag = 0;
	os_printf("dma pre cache done. \r\n");

	os_printf("lcd system core.\r\n");
	bk_lcd_driver_init(LCD_96M);

	os_printf("lcd rgb reg init.\r\n");
	bk_lcd_rgb_init(rgb_clk_div, X_PIXEL_RGB, Y_PIXEL_RGB, ORGINAL_YUYV_DATA);

	dma_rgb_config(LCD_DMA_CH);

	jpeg_config.yuv_mode = 1;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
	jpeg_config.sys_clk_div = 3;
	jpeg_config.mclk_div = 0;
	switch (fps) {
		case 5:
			fps = TYPE_5FPS;
			break;
		case 10:
			fps = TYPE_10FPS;
			break;
		case 20:
			fps = TYPE_20FPS;
			break;
		case 25:
			fps = TYPE_25FPS;
			break;
		case 30:
			fps = TYPE_30FPS;
			break;
		default:
			os_printf("input fps param error\n");
			return;
	}

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

	err = bk_camera_set_ppi_fps(VGA_480_272, fps);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		return;
	}

	bk_camera_sensor_config();

	os_printf("open jpeg yuv\r\n");
	bk_lcd_rgb_display_en(1);
	bk_jpeg_enc_set_yuv_mode(1);
}

void lcd_video_jpeg_dec(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t psram_mode = 0x00054043;
	s_dma_transfer_param.dma_transfer_len = 65280;
	s_dma_transfer_param.dma_transfer_cnt = 4;

	err=bk_jpeg_dec_driver_init();
	if (err != BK_OK)
		return;
	os_printf("jpegdec driver init successful.\r\n");

	rgb_clk_div = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk_div  = %d \r\n", rgb_clk_div);
	
	fps = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("fps  = %dHz \r\n", fps);

	BK_LOG_ON_ERR(bk_jpeg_enc_driver_init());

	err = bk_psram_init(psram_mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		return;
	}

	os_printf(" dma_pre config \r\n");
	dma_pre_config(LCD_DMA_CH);

	while(s_dma_transfer_param.dma_frame_end_flag == 0) {};
	s_dma_transfer_param.dma_frame_end_flag = 0;
	os_printf("dma1 pre transfer completek\r\n");

	os_printf("lcd rgb reg init.\r\n");
	bk_lcd_driver_init(LCD_96M);
	bk_lcd_rgb_init(rgb_clk_div, X_PIXEL_RGB, Y_PIXEL_RGB, VUYY_DATA);

	os_printf("dma_lcd_config \r\n");
	dma_rgb_config(LCD_DMA_CH);

	bk_jpeg_enc_register_isr(END_OF_FRAME, jpeg_enc_end_of_frame_cb, NULL);
	bk_jpeg_dec_complete_cb(jpeg_dec_end_of_frame_cb, NULL);
	
	os_printf("jpeg enc init.\r\n");
	jpeg_config.yuv_mode = 0;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
	jpeg_config.sys_clk_div = 3;
	jpeg_config.mclk_div = 0;
	switch (fps) {
		case 5:
			fps = TYPE_5FPS;
			break;
		case 10:
			fps = TYPE_10FPS;
			break;
		case 20:
			fps = TYPE_20FPS;
			break;
		case 25:
			fps = TYPE_25FPS;
			break;
		case 30:
			fps = TYPE_30FPS;
			break;
		default:
			os_printf("input fps param error\n");
			return;
	}

	err = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (err != BK_OK) {
		os_printf("jpeg init error\r\n");
		return;
	}

	os_printf("jpeg dma config\r\n");
	dma_jpeg_config(JPEG_DMA_CH);

	i2c_config.baud_rate = 100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != BK_OK) {
		os_printf("i2c init error\r\n");
		return;
	}

	err = bk_camera_set_ppi_fps(VGA_480_272, fps);
	if (err != BK_OK) {
		os_printf("set camera ppi and fps error\n");
		return;
	}
	bk_camera_sensor_config();

	err = bk_jpeg_dec_init(jpeg_buff, jpeg_dec_buff);
	if (err != BK_OK) {
		os_printf("jpeg dec error\r\n");
		return;
	}
}


void lcd_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_memset(&s_dma_transfer_param, 0, sizeof(s_dma_transfer_param));

	if (os_strcmp(argv[1], "8080lcd") == 0) {
		os_printf("close 8080 lcd \r\n");
		bk_lcd_8080_deinit();
	}
	if (os_strcmp(argv[1], "rgblcd") == 0) {
		os_printf("close rgb lcd \r\n");
		bk_lcd_rgb_deinit();
	}
}

