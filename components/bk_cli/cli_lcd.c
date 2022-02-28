#include "rtos_pub.h"
#include "cli.h"
#include "bk_api_int.h"
#include "bk_err.h"
#include "sdkconfig.h"
#include "lcd_disp_types.h"
#include "BK7256_RegList.h"
#include "sys_driver.h"
#include "lcd_disp_hal.h"
#include "dma_hal.h"
#include "jpeg_hal.h"
#include "bk_api_jpeg.h"
#include "bk_api_lcd.h"
#include "bk_api_dma.h"
#include "bk_api_gpio.h"
#include "bk_general_dma.h"
#include "cli_lcd.h"
#include "bk_api_psram.h"
#include "video_transfer.h"
#include "camera_intf_pub.h"
#include "bk_api_i2c.h"

volatile uint32_t     dma_int_flag = 0;
volatile uint32_t     dma_int_cnt = 0;
volatile uint32_t     lcd_isr_cnt = 0;
volatile uint32_t     jpeg_int_flag = 0;

static void cpu_delay( volatile unsigned int times)
{
	while(times--);
}

static void i8080_lcd_isr(void)
{
	//os_printf("%s, %d\n", __func__, __LINE__);
	uint32 int_status = 0;
	int_status = bk_lcd_int_status_get();

	if(int_status & I8080_OUTPUT_SOF)
	{
		///CLI_LOGI("lcd i8080 sof int triggered \r\n");
		bk_lcd_8080_sof_int_clear();
	}

	if(int_status & I8080_OUTPUT_EOF)
	{
		lcd_isr_cnt++;
		//CLI_LOGI("lcd i8080 eof int triggered %x\r\n", lcd_isr_cnt);
		bk_lcd_8080_eof_int_clear();
		if(lcd_isr_cnt == 400)
		{
			lcd_isr_cnt = 0;
			bk_dma_stop(I8080_TEST_CH);
			CLI_LOGI("bk_dma_stop \r\n");
		}
	}
}

static void lcd_rgb_isr(void)
{
	//os_printf("%s, %d\n", __func__, __LINE__);

	uint32 int_status = 0;
	int_status = bk_lcd_int_status_get();

	if(int_status & RGB_OUTPUT_SOF)
	{
		//CLI_LOGI("lcd rgb sof int triggered\r\n");
		bk_lcd_rgb_sof_int_clear(); 
	}

	if(int_status & RGB_OUTPUT_EOF)
	{
		lcd_isr_cnt++;
		//CLI_LOGI("lcd_rgb_isr. eof int triggered lcd_isr_cnt=%x\r\n", lcd_isr_cnt);
		if(lcd_isr_cnt == 400) {
			bk_lcd_rgb_display_en(0);
			bk_dma_stop(RGB_DATA_TEST_CH);
		}
		bk_lcd_rgb_eof_int_clear();
	}
}

/*
static void lcd_jpeg_yuv_isr(void)
{
	bk_lcd_rgb_eof_int_clear();
	//bk_lcd_rgb_display_en(0);
	//bk_jpeg_set_yuv_mode(1);
}
*/
static void dma_jpeg_yuv_finish_isr(dma_id_t id)
{
	//bk_gpio_set_output_high(2);
	//CLI_LOGI("enter dma_jpeg_yuv_finish_isr \r\n");
	dma_int_cnt++;
	if(dma_int_cnt == 4)//32)
	{
		bk_dma_stop(0);
		dma_int_cnt = 0;
		addGENER_DMA_Reg0x2 =(uint32)PSRAM_BASEADDR;
		dma_int_flag = 1;
	}
	else {
		addGENER_DMA_Reg0x2 = (uint32)PSRAM_BASEADDR + (uint32_t)(65280 * dma_int_cnt);
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
		bk_dma_start(0);
	}
	//bk_gpio_set_output_low(2);
}

static void end_of_yuv_frame(jpeg_unit_t id, void *param)
{
	//CLI_LOGI("enter end_of_yuv_frame \r\n");
	//bk_jpeg_set_yuv_mode(0);
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
	//bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(0));
	//bk_jpeg_set_yuv_mode(1);
}

static void flash_to_sram_finish_isr(dma_id_t id)
{
	CLI_LOGI("enter dma2_finish_isr \r\n");
	dma_int_flag = 1;
}

static void i8080_dma_finish_isr(dma_id_t id)
{
	CLI_LOGI("enter dma0_finish_isr \r\n");
}


void transfer_set(uint16 column, uint16 row)
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
/*flash to ram */
static void flash_to_sram_dma_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) &rgb_565_red[0];
	dma_config.src.end_addr = (uint32_t) &rgb_565_red[0] + 768;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) sm1_addr;
	dma_config.dst.end_addr = (uint32_t) sm1_addr + 768;

	BK_LOG_ON_ERR(bk_dma_init(FLASH_TO_SRAM_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(FLASH_TO_SRAM_CH, 768)); //DMA_DEV_LCD_DATA
	BK_LOG_ON_ERR(bk_dma_register_isr(FLASH_TO_SRAM_CH, NULL, flash_to_sram_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(FLASH_TO_SRAM_CH));
}

static void dma2_config_green(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) &rgb_565_green[0];
	dma_config.src.end_addr = (uint32_t) &rgb_565_green[0] + 768;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32_t) sm1_addr;
	dma_config.dst.end_addr = (uint32_t) sm1_addr + 768;

	BK_LOG_ON_ERR(bk_dma_init(FLASH_TO_SRAM_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(FLASH_TO_SRAM_CH, 768));
	BK_LOG_ON_ERR(bk_dma_register_isr(FLASH_TO_SRAM_CH, NULL, flash_to_sram_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(FLASH_TO_SRAM_CH));
}


/*
 * DMA0 SRC SM1 DEST I8080 DAT FIFO, FOREVER MODE.
 */
static void i8080_dma_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_16BITS;
	dma_config.src.start_addr = (uint32_t) sm1_addr;
	dma_config.src.end_addr = (uint32_t)(uint32_t) sm1_addr + 768;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;//0x11;//
	dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	dma_config.dst.start_addr =  (uint32_t) &REG_DISP_DAT_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(I8080_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(I8080_TEST_CH, 768));
	BK_LOG_ON_ERR(bk_dma_register_isr(I8080_TEST_CH, NULL, i8080_dma_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(I8080_TEST_CH));
}


static void rgb_data_dma_config(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32_t) sm0_addr;
	dma_config.src.end_addr = (uint32_t)(uint32_t) sm0_addr + 80;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr =  (uint32_t) &REG_DISP_RGB_FIFO;

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
	dma_config.dst.start_addr = (uint32) &REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(YUV_DATA_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(YUV_DATA_TEST_CH, 80));
	//BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(YUV_DATA_TEST_CH));
}




/**
 * @brief  lcd_st7796s_init
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
static bk_err_t lcd_st7796s_init(void)
{
	//cpu_delay(2339285);
	cpu_delay(7017857); // 131ms

	//*************LCD Driver Initial *****//
	bk_lcd_8080_write_cmd(sleep_out);
	//cpu_delay(1285714); // need Delay 120ms
	cpu_delay(6428571);//120ms
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
	cpu_delay(2);
	bk_lcd_8080_write_data(0x1b);

	bk_lcd_8080_write_cmd(command_1);
	bk_lcd_8080_write_data(0x3c);
	bk_lcd_8080_write_cmd(command_1);
	bk_lcd_8080_write_data(0x69);
	cpu_delay(1285714);
	bk_lcd_8080_write_cmd(display_on); //Display ON
	return BK_OK;
}

void cli_lcd_help(void)
{
	CLI_LOGI("asic test cmd is : lcd_debug=start \r\n");
	CLI_LOGI("asic test cmd is : lcd_no_dma_test=start \r\n");
	CLI_LOGI("asic test cmd is : lcd_dma_test=start \r\n");
	CLI_LOGI("asic test cmd is : lcd_green=start \r\n");
}

static void lcd_no_dma_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t i = 0;
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		CLI_LOGI("lcd test start\n");

		/* 0:clk_320M  1:clk_480M */
		/*  Frequency division : F/(1+clkdiv_disp_l+clkdiv_disp_h*2)*/
		gpio_lcd_8080_sel();
		bk_lcd_init(1, 0, 2);
		bk_lcd_deinit();
		bk_lcd_8080_int_enable(1, 1);
		bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_disconti_mode(1);
		bk_lcd_isr_register(i8080_lcd_isr);

		bk_lcd_8080_display_enable(1);
		bk_lcd_8080_display_enable(0);
		lcd_st7796s_init();
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);

		bk_lcd_8080_display_enable(1);
		lcd_st7796s_init();
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_8080_start_transfer(1);

		bk_lcd_8080_write_cmd(ram_write);
		for(i = 0; i<(320*480*3); i++) {
			bk_lcd_8080_write_data(0xffff);
			for (int j = 0; j<8; j++) {
				__asm volatile("nop");
			};
		}
	}
else
	{
		CLI_LOGI("not support cmd\r\n");
	}
}

static void lcd_8080_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		bk_lcd_deinit();
		dma_int_cnt = 0;

		memset((void *)0x30020000, 0, 768);
		CLI_LOGI("config dma2 from flash to sram,not start transfer.\r\n");
		flash_to_sram_dma_config();
		CLI_LOGI("config dma0 from sram to fifo,not start transfer \r\n");
		i8080_dma_config();

		bk_lcd_fifo_clr();
		CLI_LOGI("dma2 start transfer \r\n", dma_int_flag);
		BK_LOG_ON_ERR(bk_dma_start(FLASH_TO_SRAM_CH));

		while(dma_int_flag == 0);
		dma_int_flag = 0;
		CLI_LOGI("dma2 transfer complete,data_from flash to sram ok\r\n");

		CLI_LOGI("lcd sys clk init \r\n");
		bk_lcd_init(1, 0, 2);
		gpio_lcd_8080_sel();

		CLI_LOGI("lcd reg ini \r\n");
		bk_lcd_disconti_mode(1);
		bk_lcd_8080_int_enable(0, 1);
		bk_lcd_8080_set_dat_fifo_thrd(96,96);
		bk_lcd_isr_register(i8080_lcd_isr);
		bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);

		bk_lcd_8080_display_enable(1);
		bk_lcd_8080_display_enable(0);
		bk_lcd_8080_reset();
		lcd_st7796s_init();
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);
		cpu_delay(1000);

		CLI_LOGI("lcd 8080 display_enable \r\n");
		bk_lcd_8080_display_enable(1);
		lcd_st7796s_init();
		bk_lcd_8080_set_tik(2);
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);

		bk_lcd_8080_start_transfer(1); //dat_on

		CLI_LOGI("dma0 start transfer \r\n");
		BK_LOG_ON_ERR(bk_dma_start(I8080_TEST_CH));
		CLI_LOGI("lcd write.\r\n");
		bk_lcd_8080_write_cmd(ram_write);
	}
	else
	{
		CLI_LOGI("asic test cmd is : asic_test = start \r\n");
	}
}

static void lcd_debug(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	CLI_LOGI("lcd_debug = start \r\n");

	if (os_strcmp(argv[1], "start") == 0) {
			bk_lcd_debug();
		}
	else {
		CLI_LOGI("unsupport cmd \r\n");
	}
}

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

static void lcd_rgb_init(uint8_t rgb_div, uint8_t yuv_mode)
{
	bk_lcd_rgb_clk_div(rgb_div);
	bk_lcd_rgb_dclk_rev_edge(0);
	bk_lcd_pixel_config(x_pixel_V272P, y_pixel_V272P);
	bk_lcd_rgb_disp_sel();
	bk_lcd_disconti_mode(1);
	bk_lcd_rgb_fifo_thrd_set(0x60, 0x60);
	bk_lcd_sync_config(hsync_back_porch, hsync_front_porch, vsync_back_porch, vsync_front_porch);
	bk_lcd_rgb_data_format(yuv_mode);
}

static void lcd_yuv_data_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		memset((void *)sm1_addr, 0, 80);
		memcpy((void *)sm1_addr, &yuv_dat[0], 80);
		yuv_data_dma_config();

		CLI_LOGI("display fifo mem clear\r\n");
		bk_lcd_fifo_clr();

		CLI_LOGI("lcd rgb io init.\r\n");
		gpio_lcd_rgb_sel();

		CLI_LOGI("lcd system core.\r\n");
		bk_lcd_init(1, 0, 2);

		CLI_LOGI("lcd rgb reg init.\r\n");
		lcd_rgb_init(5,ORGINAL_YUYV_DATA);
		bk_lcd_rgb_int_enable(0,1);
		bk_lcd_isr_register(lcd_rgb_isr);

		CLI_LOGI("lcd rgb disply enable.\r\n");
		bk_lcd_rgb_display_en(1);
		CLI_LOGI("dma start transfer.\r\n");
		BK_LOG_ON_ERR(bk_dma_start(YUV_DATA_TEST_CH));
	}
	else
	{
		CLI_LOGI("unsupport cmd \r\n");
	}
}

static void lcd_rgb_data_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		memset((void *)sm0_addr, 0, 80);
		memcpy((void *)sm0_addr, &red_dat[0], 80);
		rgb_data_dma_config();

		CLI_LOGI("display fifo mem clear\r\n");
		bk_lcd_fifo_clr();

		CLI_LOGI("lcd rgb io init.\r\n");
		gpio_lcd_rgb_sel();
		
		CLI_LOGI("lcd system core.\r\n");
		bk_lcd_init(1, 0, 2);
		
		CLI_LOGI("lcd rgb reg init.\r\n");

		lcd_rgb_init(5,RGB565_DATA);
		bk_lcd_isr_register(lcd_rgb_isr);

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

static void lcd_rgb_clolor(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t color_datas[20] = {0};
	uint32_t color  = 0;
	lcd_isr_cnt = 0;
	color = os_strtoul(argv[1], NULL, 16) & 0xFFFFFFFF;
	memset((uint32_t *)sm0_addr, 0, 80);
	for(int i = 0; i< 20; i++) {
		color_datas[i] = color;
	}
	CLI_LOGI("display color is %x\r\n", color_datas[19]);
	memcpy((void *)sm0_addr, &color_datas[0], 80);

	rgb_data_dma_config();
	bk_lcd_rgb_display_en(1);
	BK_LOG_ON_ERR(bk_dma_start(RGB_DATA_TEST_CH));
}

static void lcd_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	bk_lcd_deinit();
	bk_lcd_deinit();
	err = bk_jpeg_deinit();
	if (err != kNoErr) {
		os_printf("jpeg deinit error\n");
		return;
	}
	os_printf("jpeg deinit ok!\n");

	err = bk_i2c_deinit(CONFIG_CAMERA_I2C_ID);
	if (err != kNoErr) {
		os_printf("i2c deinit error\n");
		return;
	}
	os_printf("I2c deinit ok!\n");
}


void dma0_config_yuv_jpeg(void)
{
	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) PSRAM_BASEADDR;
	dma_config.src.end_addr =  (uint32) PSRAM_BASEADDR + 65280;//65280/4
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) &REG_DISP_RGB_FIFO;

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
	dma_config.src.start_addr = (uint32) PSRAM_BASEADDR;
	dma_config.src.end_addr =  (uint32) PSRAM_BASEADDR + 4;//65280/4
	dma_config.dst.dev = DMA_DEV_DTCM ;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) PSRAM_BASEADDR2;

	BK_LOG_ON_ERR(bk_dma_init(YUV_JPEG_TEST_CH, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(YUV_JPEG_TEST_CH, 4));
	BK_LOG_ON_ERR(bk_dma_register_isr(YUV_JPEG_TEST_CH, NULL, dma_jpeg_yuv_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(YUV_JPEG_TEST_CH));
}

static void lcd_green(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "start") == 0) {
		dma_int_flag = 0;
		dma2_config_green();
		CLI_LOGI("dma2 start transfer \r\n");
		BK_LOG_ON_ERR(bk_dma_start(FLASH_TO_SRAM_CH));
		while(dma_int_flag == 0);
		dma_int_flag = 0;
		CLI_LOGI("dma2 transfer complete,data_from flash to sram ok\r\n");
		i8080_dma_config();
		bk_lcd_fifo_clr();
		BK_LOG_ON_ERR(bk_dma_start(I8080_TEST_CH));
		bk_lcd_8080_write_cmd(continue_write);
	}
	else {
		CLI_LOGI("unsupport cmd \r\n");
	}
}
static void lcd_video(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int err = kNoErr;
	uint8_t rgb_clk = 0;
	uint8_t rgb_clk_div = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t fps;
	uint32_t psram_mode = 0x00054043;
	dma_int_flag = 0;
	dma_int_cnt = 0;

	bk_gpio_enable_output(2);
	bk_gpio_set_output_low(2);

	rgb_clk = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk = %dMHz \r\n", rgb_clk);

	rgb_clk_div = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	os_printf("rgb_clk_div  = %d \r\n", rgb_clk_div);
	
	fps = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
	os_printf("fps  = %dHz \r\n", fps);

	if (argc < 2) {
		os_printf("input param error\n");
		return;
	}

	err = bk_psram_init(psram_mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		return;
	}

	dma0_cache_config_yuv_jpeg();
	os_printf("pre dma init.\r\n");

	bk_lcd_fifo_clr();
	CLI_LOGI("lcd rgb io init.\r\n");
	gpio_lcd_rgb_sel();
	CLI_LOGI("lcd system core.\r\n");
	if(rgb_clk == 96) {
		bk_lcd_init(1, 0, 2);
	} else if(rgb_clk == 160) {
		bk_lcd_init(1, 0, 1);
	} else if(rgb_clk == 240) {
		bk_lcd_init(1, 1, 0);
	} else if(rgb_clk == 480) {
		bk_lcd_init(1, 0, 0);
	}

	CLI_LOGI("lcd rgb reg init.\r\n");
	lcd_rgb_init(rgb_clk_div, 1);
//	bk_lcd_rgb_display_en(0);
//	bk_lcd_rgb_int_enable(0,1);
//	bk_lcd_isr_register(lcd_jpeg_yuv_isr);
	os_printf("pre disp init.\r\n");

	jpeg_config.yuv_mode = 1;
	jpeg_config.x_pixel = X_PIXEL_480;
	jpeg_config.y_pixel = Y_PIXEL_272;
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

	bk_jpeg_register_end_yuv_isr(end_of_yuv_frame, NULL);
	err = bk_jpeg_init(&jpeg_config);
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

	err = camera_set_ppi_fps(VGA_480_272, fps);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		return;
	}

	camera_intf_config_senser();
	os_printf("camera init ok\n");

	while(dma_int_flag == 0) {};
	dma_int_flag = 0;
	os_printf("dma pre cache done,next config real dma to lcd. \r\n");

	dma0_config_yuv_jpeg();
	os_printf("open jpeg yuv\r\n");
	bk_lcd_rgb_display_en(1);
	bk_jpeg_set_yuv_mode(1);
}

#define LCD_CNT (sizeof(s_lcd_commands) / sizeof(struct cli_command))
static const struct cli_command s_lcd_commands[] = {
	{"lcd_no_dma_test", "lcd_no_dma_test {start|stop} {red|blue|green|yellow\r\n}", lcd_no_dma_test},
	{"lcd_8080_test", "lcd_dma_test {start|stop}\r\n", lcd_8080_test},
	{"lcd_green", "lcd_green {start|stop}", lcd_green},
	{"lcd_yuv_data_test", "lcd_debug {start|stop}", lcd_yuv_data_test},
	{"lcd_rgb_data_test", "lcd_rgb_data_test {start|stop}", lcd_rgb_data_test},
	{"lcd_debug", "lcd_debug {start|stop}", lcd_debug},
	{"printf_log", "printf_log {close|open}", printf_log},
	{"lcd_rgb_clolor", "lcd_rgb_clolor {close|open}", lcd_rgb_clolor},
	{"lcd_video", "lcd_video = 272,25", lcd_video},
	{"lcd_close", "lcd_close", lcd_close},
};

int cli_lcd_init(void)
{
	return cli_register_commands(s_lcd_commands, LCD_CNT);
}








