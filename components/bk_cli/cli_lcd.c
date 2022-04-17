
#include <os/os.h>
#include "cli.h"
#include <driver/int.h>
#include <common/bk_err.h>
#include "sdkconfig.h"
#include "driver/lcd_disp_types.h"
#include "BK7256_RegList.h"
#include "sys_driver.h"
#include <driver/jpeg_enc.h>
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include "bk_general_dma.h"
#include "cli_lcd.h"
#include <driver/psram.h>
#include <components/video_transfer.h>
#include <components/dvp_camera.h>
#include <driver/i2c.h>
#include <driver/jpeg_dec.h>
#include "ff.h"
#include "diskio.h"
#include "stdio.h"



volatile uint32_t dma_int_cnt = 0;
volatile uint32_t     lcd_isr_cnt = 0;
volatile uint32_t     dma_int_flag = 0;

static uint32_t* rd_buff = (uint32_t *)0x60000000;//0x30004f00;//; //lea add
static uint32_t* JpegRxBuff = (uint32_t *)0x30000000;

void st7796s_set_display_mem_area(uint16 column, uint16 row);
bk_err_t st7796s_init(void);
extern void delay(INT32 num);
//static void cpu_delay( volatile unsigned int times)
//{
//	os_printf("11122. \r\n");
//	while(times--);
//	os_printf("11123. \r\n");
//}

void lcd_video_power(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "on") == 0) {
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_ON);
		
		bk_psram_init(0x00054043);
	} else if(os_strcmp(argv[1], "off") == 0) {
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_OFF);
	}else {
		CLI_LOGI("cmd error: lcd_video_power=on/off \r\n");
	}
}

static void lcd_i8080_isr(void)
{
	addAON_GPIO_Reg0x2 = 0x2;
	//os_printf("lcd 8080 end frame\r\n");
#if(1)
	bk_lcd_rgb_display_en(0);
	bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);
	bk_lcd_rgb_display_en(1);
	bk_lcd_8080_write_cmd(ram_write);
#else
	//use this appear asic bug, nest resion modify
	bk_lcd_8080_write_cmd(continue_write);
#endif
	addAON_GPIO_Reg0x2 = 0x0;
}

static void lcd_rgb_isr(void)
{
	//os_printf("%s, %d\n", __func__, __LINE__);
	lcd_isr_cnt++;
	if(lcd_isr_cnt == 400) {
		bk_lcd_rgb_display_en(0);
		bk_dma_stop(RGB_DATA_TEST_CH);
		CLI_LOGI("lcd_rgb_close.\r\n");
	}
}

static void dma_jpeg_yuv_finish_isr(dma_id_t id)
{
	addAON_GPIO_Reg0x4 = 0x2;
	//CLI_LOGI("enter dma_jpeg_yuv_finish_isr \r\n");
	dma_int_cnt++;
	if(dma_int_cnt == 4)//32)
	{
		bk_dma_stop(DMA_ID_0);
		dma_int_cnt = 0;
		bk_dma_set_src_addr(DMA_ID_0, (uint32_t)LCD_FRAMEADDR, 0);
		//addGENER_DMA_Reg0x2 =(uint32_t)LCD_FRAMEADDR;
		dma_int_flag = 1;
	}
	else {
		
		bk_dma_set_src_addr(DMA_ID_0, ((uint32_t)LCD_FRAMEADDR + (uint32_t)(65280 * dma_int_cnt)), 0);
		//addGENER_DMA_Reg0x2 = (uint32_t)LCD_FRAMEADDR + (uint32_t)(65280 * dma_int_cnt);
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(DMA_ID_0));
		bk_dma_start(DMA_ID_0);
	}
	addAON_GPIO_Reg0x4 = 0x0;
}

static void end_of_yuv_frame(jpeg_unit_t id, void *param)
{
	addAON_GPIO_Reg0x3 = 0x2;
	//CLI_LOGI("enter end_of_yuv_frame \r\n");
	//bk_jpeg_set_yuv_mode(0);
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
	//bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(0));
	//bk_jpeg_set_yuv_mode(1);
	addAON_GPIO_Reg0x3 = 0x0;
}



/**
 * @brief  lcd_st7796s_init
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t st7796s_init(void)
{
	//cpu_delay(7017857); // 131ms
	delay(16470);
	delay(16470);

	//*************LCD Driver Initial *****//
	bk_lcd_8080_write_cmd(sleep_out);
	delay(16470);//120ms
	delay(16470);//120ms
	//os_printf("delay 120ms. \r\n");
	bk_lcd_8080_write_cmd(command_1);

	bk_lcd_8080_write_data(0xc3);
	bk_lcd_8080_write_cmd(command_2);
	bk_lcd_8080_write_data(0x96);
	bk_lcd_8080_write_cmd(mem_access_ctrl);
	bk_lcd_8080_write_data(0x48);
	bk_lcd_8080_write_cmd(color_mode);
	bk_lcd_8080_write_data(0x5);
	bk_lcd_8080_write_cmd(ploar_convert);
	bk_lcd_8080_write_data(0x1);
	bk_lcd_8080_write_cmd(disp_out_ctrl);
	bk_lcd_8080_write_data(0x40);
	bk_lcd_8080_write_data(0x8a);
	bk_lcd_8080_write_data(0x00);
	bk_lcd_8080_write_data(0x00);
	bk_lcd_8080_write_data(0x29);
	bk_lcd_8080_write_data(0x19);
	bk_lcd_8080_write_data(0xa5);
	bk_lcd_8080_write_data(0x33);
	bk_lcd_8080_write_cmd(power_ctrl1);//no
	bk_lcd_8080_write_data(0x06);
	bk_lcd_8080_write_cmd(power_ctrl2);
	bk_lcd_8080_write_data(0xa7);
	bk_lcd_8080_write_cmd(vcom_ctrl);
	bk_lcd_8080_write_data(0x18);
	bk_lcd_8080_write_cmd(cathode_ctrl); //Positive Voltage Gamma Control
	bk_lcd_8080_write_data(0xf0);
	bk_lcd_8080_write_data(0x09);
	bk_lcd_8080_write_data(0x0b);
	bk_lcd_8080_write_data(0x06);
	bk_lcd_8080_write_data(0x04);
	bk_lcd_8080_write_data(0x15);
	bk_lcd_8080_write_data(0x2f);
	bk_lcd_8080_write_data(0x54);
	bk_lcd_8080_write_data(0x42);
	bk_lcd_8080_write_data(0x3c);
	bk_lcd_8080_write_data(0x17);
	bk_lcd_8080_write_data(0x14);
	bk_lcd_8080_write_data(0x18);
	bk_lcd_8080_write_data(0x1b);
	bk_lcd_8080_write_cmd(anode_ctrl); //Negative Voltage Gamma Control
	bk_lcd_8080_write_data(0xf0);
	bk_lcd_8080_write_data(0x09);
	bk_lcd_8080_write_data(0x0b);
	bk_lcd_8080_write_data(0x06);
	bk_lcd_8080_write_data(0x04);
	bk_lcd_8080_write_data(0x03);
	bk_lcd_8080_write_data(0x2d);
	bk_lcd_8080_write_data(0x43);
	bk_lcd_8080_write_data(0x42);
	bk_lcd_8080_write_data(0x3b);
	bk_lcd_8080_write_data(0x16);
	bk_lcd_8080_write_data(0x14);
	bk_lcd_8080_write_data(0x17);
	bk_lcd_8080_write_data(0x1b);

	bk_lcd_8080_write_cmd(command_1);
	bk_lcd_8080_write_data(0x3c);
	bk_lcd_8080_write_cmd(command_1);
	bk_lcd_8080_write_data(0x69);
	
	delay(16470);
	delay(16470);//120ms
	bk_lcd_8080_write_cmd(display_on); //Display ON
	return BK_OK;
}



void st7796s_set_display_mem_area(uint16 column, uint16 row)
{
	/*
	This function need set in initial.
	Can't set again in int handle.
	Will cause imag dislocaled.
	*/

	uint16 column_num1,column_num2,row_num1,row_num2;

	column_num1 = column >> 8;
	column_num2 = column & 0xff;

	row_num1 = row >> 8;
	row_num2 = row & 0xff;

	bk_lcd_8080_write_cmd(column_set);
	bk_lcd_8080_write_data(0x0);
	bk_lcd_8080_write_data(0x0);
	bk_lcd_8080_write_data(column_num1);
	bk_lcd_8080_write_data(column_num2);
	
	bk_lcd_8080_write_cmd(row_set);
	bk_lcd_8080_write_data(0x0);
	bk_lcd_8080_write_data(0x0);
	bk_lcd_8080_write_data(row_num1);
	bk_lcd_8080_write_data(row_num2);
}

static void rgb_data_dma_config(uint32_t* src_addr)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) src_addr;
	dma_config.src.end_addr = (uint32_t)src_addr + 80;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr =  (uint32_t) REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(RGB_DATA_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(RGB_DATA_TEST_CH, 80));
	//BK_LOG_ON_ERR(bk_dma_register_isr(RGB_DATA_TEST_CH, NULL, rgb_data_dma_finish_isr));
	//BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(RGB_DATA_TEST_CH));
}

void yuv_data_dma_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_16BITS;
	dma_config.src.start_addr = (uint32) sm1_addr;
	dma_config.src.end_addr =  (uint32) sm1_addr + 80;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(YUV_DATA_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(YUV_DATA_TEST_CH, 80));
	//BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(YUV_DATA_TEST_CH));
}

void cli_lcd_help(void)
{
	CLI_LOGI("lcd cmd include : \r\n.");
	CLI_LOGI("lcd_8080_init = 96,0,2 \r\n.");
	CLI_LOGI("lcd_video=96M,8,25 \r\n.");
	CLI_LOGI("lcd_video_jpeg_dec=96M,13,25 \r\n.");
	CLI_LOGI("lcd_close=lcd8080 | lcdrgb \r\n.");
}



static void flash_to_sram_finish_isr(dma_id_t id)
{
		dma_int_cnt++;
		if(dma_int_cnt == 5)
		{
			dma_int_cnt = 0;
			dma_int_flag = 1;
			bk_dma_set_src_addr(I8080_TEST_CH, (uint32_t)LCD_FRAMEADDR, 0);
		}
		else {
			//		addGENER_DMA_Reg0x2 = (uint32)LCD_FRAMEADDR + (uint32_t)(61440 * dma_int_cnt);
			bk_dma_set_src_addr(I8080_TEST_CH, (uint32)(LCD_FRAMEADDR + (61440 * dma_int_cnt)), 0);
			BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(I8080_TEST_CH));
			bk_dma_start(I8080_TEST_CH);
		}
}

/*flash to ram */
static void flash_to_sram_dma_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) &rgb_565[0];
	dma_config.src.end_addr = (uint32_t) &rgb_565[0] + 640;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) sm1_addr;
	dma_config.dst.end_addr = (uint32_t) sm1_addr + 640;

	BK_LOG_ON_ERR(bk_dma_init(I8080_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(I8080_TEST_CH, 640)); 
	BK_LOG_ON_ERR(bk_dma_register_isr(I8080_TEST_CH, NULL, flash_to_sram_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(I8080_TEST_CH));
}
#if(0)
static void lcd_8080_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "start") == 0) {
		
		bk_lcd_driver_init(LCD_96M);

		CLI_LOGI("psram init. \r\n");
		bk_psram_init(0x00054043);

		CLI_LOGI("dma pre config. \r\n");
		flash_to_sram_dma_config();
		BK_LOG_ON_ERR(bk_dma_start(FLASH_TO_SRAM_CH));
		while(dma_int_flag == 0);
		dma_int_flag = 0;

		CLI_LOGI("i8080 lcd init. \r\n");
		bk_lcd_8080_init(X_PIXEL_8080, Y_PIXEL_8080);
		st7796s_init();
		bk_lcd_8080_display_enable(1);
		st7796s_init();
		//st7796s_set_display_mem_area(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_8080_int_enable(0, 1);
		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr);
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(ram_write);
		CLI_LOGI("init ok. \r\n");
	}
	else
	{
		CLI_LOGI("asic test cmd is : asic_test = start \r\n");
	}
}
#else
static void lcd_8080_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {

		CLI_LOGI("lcd driver init. \r\n");
		bk_lcd_driver_init(LCD_96M);

		CLI_LOGI("psram init. \r\n");
		bk_psram_init(0x00054043);

		CLI_LOGI("i8080 lcd init. \r\n");
		bk_lcd_8080_init(X_PIXEL_8080, Y_PIXEL_8080);

		CLI_LOGI("st7796 init. \r\n");
		st7796s_init();

		//st7796s_set_display_mem_area(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr);

		flash_to_sram_dma_config();
		BK_LOG_ON_ERR(bk_dma_start(I8080_TEST_CH));
		while(dma_int_flag == 0);
		dma_int_flag = 0;


		CLI_LOGI("8080 lcd start transfer. \r\n");
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(ram_write);
	}
	else
	{
		CLI_LOGI("asic test cmd is : asic_test = start \r\n");
	}
}
#endif

static void printf_log(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "close") == 0) {
			bk_set_printf_enable(0);
		} else if(os_strcmp(argv[1], "open") == 0) {
		bk_set_printf_enable(1);
	} else {
		CLI_LOGI("printf_log=close | open \r\n");
	}
}

static void lcd_rgb_data_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("lcd driver init. \r\n");
		bk_lcd_driver_init(LCD_96M);
		
		CLI_LOGI("psram init. \r\n");
		bk_psram_init(0x00054043);

		memcpy(rd_buff, &red_dat[0], 80);
		rgb_data_dma_config(rd_buff);
		bk_lcd_rgb_init(5,X_PIXEL_RGB, Y_PIXEL_RGB, RGB565_DATA);
		
		bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_rgb_isr);
		CLI_LOGI("lcd rgb disply enable.\r\n");
		bk_lcd_rgb_display_en(1);
		CLI_LOGI("dma start transfer.\r\n");
		BK_LOG_ON_ERR(bk_dma_start(RGB_DATA_TEST_CH));
	}
	else
	{
		CLI_LOGI("unsupport cmd \r\n");
	}
}

static void lcd_rgb_color(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t color_datas[20] = {0};
	uint32_t color  = 0;
	lcd_isr_cnt = 0;
	color = os_strtoul(argv[1], NULL, 16) & 0xFFFFFFFF;
	for(int i = 0; i< 20; i++) {
		color_datas[i] = color;
	}
	CLI_LOGI("display color is %x\r\n", color_datas[19]);
	memcpy((void *)rd_buff, &color_datas[0], 80);

	rgb_data_dma_config(rd_buff);
	bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(RGB_DATA_TEST_CH));
}


void dma0_config_yuv_jpeg(void)
{
	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.end_addr =  (uint32) LCD_FRAMEADDR + 65280;//65280/4
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(YUV_JPEG_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(YUV_JPEG_TEST_CH, 65280));
	BK_LOG_ON_ERR(bk_dma_register_isr(YUV_JPEG_TEST_CH, NULL, dma_jpeg_yuv_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(YUV_JPEG_TEST_CH));
}

void  dma0_cache_config_yuv_jpeg(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.end_addr =  (uint32) LCD_FRAMEADDR + 4;//65280/4
	dma_config.dst.dev = DMA_DEV_DTCM ;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) LCD_FRAMEADDR + 0x100;

	BK_LOG_ON_ERR(bk_dma_init(YUV_JPEG_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(YUV_JPEG_TEST_CH, 4));
	BK_LOG_ON_ERR(bk_dma_register_isr(YUV_JPEG_TEST_CH, NULL, dma_jpeg_yuv_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(YUV_JPEG_TEST_CH));
}
static void lcd_video(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t psram_mode = 0x00054043;
	dma_int_flag = 0;
	dma_int_cnt = 0;

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

	dma0_cache_config_yuv_jpeg();
	os_printf("pre dma init.\r\n");

	CLI_LOGI("lcd system core.\r\n");
	bk_lcd_driver_init(LCD_96M);

	CLI_LOGI("lcd rgb reg init.\r\n");
	bk_lcd_rgb_init(rgb_clk_div, X_PIXEL_RGB, Y_PIXEL_RGB, ORGINAL_YUYV_DATA);
	os_printf("pre disp init.\r\n");

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

	bk_jpeg_enc_register_isr(END_OF_YUV, end_of_yuv_frame, NULL);
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
	os_printf("camera init ok\n");

	while(dma_int_flag == 0) {};
	dma_int_flag = 0;
	os_printf("dma pre cache done,next config real dma to lcd. \r\n");

	dma0_config_yuv_jpeg();
	os_printf("open jpeg yuv\r\n");
	bk_lcd_rgb_display_en(1);
	bk_jpeg_enc_set_yuv_mode(1);
}

static void dma1_jpeg_dec_isr(dma_id_t id)
{
	//CLI_LOGI("enter dma1_jpeg_dec_isr \r\n");
	addAON_GPIO_Reg0x4 = 0x2;
	dma_int_cnt++;
	if(dma_int_cnt == 4)
	{
		bk_dma_stop(1);
		dma_int_cnt = 0;
		bk_dma_set_src_start_addr(1, (uint32)LCD_FRAMEADDR);
		dma_int_flag = 1;
	}
	else {
		bk_dma_set_src_start_addr(1, (uint32)(LCD_FRAMEADDR + (65280 * dma_int_cnt)));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(1));
		bk_dma_start(1);
	}
	addAON_GPIO_Reg0x4 = 0x0;
}


static void jpeg_end_of_frame(jpeg_unit_t id, void *param)
{
	//CLI_LOGI("enter jpeg end_of_frame isr \r\n");
	addAON_GPIO_Reg0x3 = 0x2;
#if(1)
	//jpegenc_off();
	addJPEG_Reg0x1      &= 0xffffffef;
	addGENER_DMA_Reg0x0 &= 0xfffffffe;
#else
	bk_jpeg_enc_set_enable(0);
	bk_dma_stop(DMA_ID_0);
#endif
	bk_jpeg_dec_start_dec(JpegRxBuff, rd_buff);
	addAON_GPIO_Reg0x3 = 0x0;
}


static void jpeg_dec_end_of_frame()
{
	//jpegenc_en();
#if(1)
	addJPEG_Reg0x1 |= 0x10;
	addGENER_DMA_Reg0x0 |= 1;
#else
	bk_jpeg_enc_set_enable(1);
	bk_dma_start(DMA_ID_0);
#endif
	bk_lcd_rgb_display_en(1);
	bk_dma_start(1);

}

static void lcd_jpeg_dec_frame_isr(void)
{
	addAON_GPIO_Reg0x5 = 0x2;

	//CLI_LOGI("enter lcd_jpeg_dec_frame_isr \r\n");
	addAON_GPIO_Reg0x5 = 0x0;
}




static void dma_pre_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.end_addr =  (uint32) LCD_FRAMEADDR + 4;//65280/4
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) LCD_FRAMEADDR + 0x1000;

	BK_LOG_ON_ERR(bk_dma_init(1, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(1, 65280));
	BK_LOG_ON_ERR(bk_dma_register_isr(1, NULL, dma1_jpeg_dec_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(1));
}



static  void dma_lcd_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.end_addr = (uint32) LCD_FRAMEADDR + 65280;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr =  (uint32) REG_DISP_RGB_FIFO;
	BK_LOG_ON_ERR(bk_dma_init(1, &dma_config));

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(1, 65280));
	BK_LOG_ON_ERR(bk_dma_register_isr(1, NULL, dma1_jpeg_dec_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(1));
}



void dma0_jpeg_config(void)
{
#if(0)
	dma_config_t dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_JPEG;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = JPEG_RX_FIFO;
	dma_config.dst.start_addr = (uint32_t)JpegRxBuff;
	//dma_config.dst.end_addr = (uint32_t)JpegRxBuff + 4;
	BK_LOG_ON_ERR(bk_dma_init(0, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_src_addr(0, JPEG_RX_FIFO, 0));
	BK_LOG_ON_ERR(bk_dma_set_dest_addr(0,  (uint32_t)JpegRxBuff, 0));
	bk_dma_enable_dest_addr_inc(0);

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(0, 65536));
	BK_LOG_ON_ERR(bk_dma_start(0));
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


static void lcd_video_jpeg_dec(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t psram_mode = 0x00054043;
	dma_int_cnt = 0;

	bk_jpeg_dec_driver_init();

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
	
	dma_pre_config();
	os_printf(" dma_pre config \r\n");
	BK_LOG_ON_ERR(bk_dma_start(1));

	while(dma_int_flag == 0);
	dma_int_flag = 0;
	CLI_LOGI("dma1 pre transfer completek\r\n");

	bk_lcd_driver_init(LCD_96M);

	CLI_LOGI("lcd rgb reg init.\r\n");
	bk_lcd_rgb_init(rgb_clk_div, X_PIXEL_RGB, Y_PIXEL_RGB, VUYY_DATA);

	bk_jpeg_enc_register_isr(END_OF_FRAME, jpeg_end_of_frame, NULL);
	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_jpeg_dec_frame_isr);
	//bk_jpeg_dec_isr_register(jpeg_dec_end_of_frame);
	bk_jpeg_dec_complete_cb(jpeg_dec_end_of_frame, NULL);
	os_printf("lcd disp init.\r\n");

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
	if (err != kNoErr) {
		os_printf("jpeg init error\r\n");
		return;
	}

	os_printf("jpeg dma config\r\n");
	dma0_jpeg_config();

	i2c_config.baud_rate = 100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != kNoErr) {
		os_printf("i2c init error\r\n");
		return;
	}

	os_printf("dma_lcd_config \r\n");
	dma_lcd_config();
	err = bk_camera_set_ppi_fps(VGA_480_272, fps);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		return;
	}
	bk_camera_sensor_config();
}

static void lcd_cp0_psram_to_sdcard(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t psram = 0x60000000;

	char file_name[] = "1:/lcd_rgb_data.txt";
	FRESULT fr;
	FIL file;
	//int number = DISK_NUMBER_SDIO_SD;
	uint32 uiTemp = 0;

	if (argc != 2) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("cp0: write lcd data file start\n");

		/*open pcm file*/
		fr = f_open(&file, file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", file_name);
			return;
		}

		os_printf("data:\n");
		fr = f_write(&file, (void *)psram, 0x3fc00, &uiTemp);
		if (fr != FR_OK) {
			os_printf("write %s fail.\r\n", file_name);
			return;
		}

		os_printf("\n");

		fr = f_close(&file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", file_name);
			return;
		}

		CLI_LOGI("cp0: write LCD data to file successful\r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		CLI_LOGI("cp0: LCD data to file test stop\n");

		CLI_LOGI("cp0: LCD data to file test stop successful\n");
	} else {
		cli_lcd_help();
		return;
	}
}


static void lcd_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "8080lcd") == 0) {
		CLI_LOGI("close 8080 lcd \r\n");
			bk_lcd_8080_deinit();
		}
	if (os_strcmp(argv[1], "rgblcd") == 0) {
		CLI_LOGI("close rgb lcd \r\n");
			bk_lcd_rgb_deinit();
		}
}


#define LCD_CNT (sizeof(s_lcd_commands) / sizeof(struct cli_command))
static const struct cli_command s_lcd_commands[] = {
	//{"lcd_8080_init", "lcd_8080_init {start|stop}\r\n", lcd_8080_init},
	{"lcd_8080_init", "lcd_8080_init {start|stop}\r\n", lcd_8080_init},
	{"lcd_rgb_data_test", "lcd_rgb_data_test {start|stop}", lcd_rgb_data_test},
	{"printf_log", "printf_log {close|open}", printf_log},
	{"lcd_rgb_clolor", "lcd_rgb_clolor {close|open}", lcd_rgb_color},
	{"lcd_video", "lcd_video=96M,8,25", lcd_video},
	{"lcd_video_jpeg_dec", "lcd_video_jpeg_dec = 96M,8,25,4", lcd_video_jpeg_dec},
	{"lcd_video_power", "lcd_video_power = on/off", lcd_video_power},
	{"lcd_cp0_psram_to_sdcard", "lcd_cp0_psram_to_sdcard {start|stop}", lcd_cp0_psram_to_sdcard},
	{"lcd_close", "lcd_close", lcd_close},
};

int cli_lcd_init(void)
{
	return cli_register_commands(s_lcd_commands, LCD_CNT);
}








