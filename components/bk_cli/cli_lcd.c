#include "rtos_pub.h"
#include "cli.h"
#include "bk_api_int.h"
#include "bk_err.h"
#include "lcd_disp_types.h"
#include "BK7256_RegList.h"
#include "sys_driver.h"
#include "lcd_disp_hal.h"
#include "dma_hal.h"
#include "bk_api_lcd.h"
#include "bk_api_dma.h"
#include "bk_api_gpio.h"
#include "bk_general_dma.h"
#include "cli_lcd.h"
#include "bk_api_psram.h"

volatile uint32_t     dma_int_flag = 0;
volatile uint32_t     dma_int_cnt = 0;
volatile uint32_t     lcd_isr_cnt = 0;
volatile uint8_t   RGB_FRAME_DONE = 0;

static void cpu_delay( volatile unsigned int times)
{
	while(times--);
}

static void lcd_isr(void)
{
	uint32 int_status = 0;

	int_status = bk_lcd_int_status_get();

	if(int_status & I8080_OUTPUT_SOF)
	{
		CLI_LOGI("lcd sof int triggered \r\n");
		cpu_delay(16);
		bk_lcd_8080_sof_int_clear();
	}

	if(int_status & I8080_OUTPUT_EOF)
	{
		lcd_isr_cnt++;
		CLI_LOGI("lcd eof int triggered %x\r\n", lcd_isr_cnt);
		cpu_delay(16);
		bk_lcd_8080_eof_int_clear();
	}

	if(int_status & RGB_OUTPUT_SOF)
	{
		CLI_LOGI("lcd rgb sof int triggered\r\n");
		bk_lcd_rgb_sof_int_clear(); 
	}

	if(int_status & RGB_OUTPUT_EOF)
	{
		lcd_isr_cnt++;
		CLI_LOGI("lcd rgb eof int triggered lcd_isr_cnt=%x\r\n", lcd_isr_cnt);

		if(lcd_isr_cnt == 400) {
			RGB_FRAME_DONE = 1;
			bk_lcd_rgb_display_en(0);
		}
		bk_lcd_rgb_eof_int_clear();
	}
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



static void dma2_finish_isr(dma_id_t id)
{
	CLI_LOGI("enter dma2_finish_isr \r\n");
	dma_int_flag++;
	bk_dma_stop(2);
}

static void dma0_finish_isr(dma_id_t id)
{
	CLI_LOGI("enter dma0_finish_isr \r\n");
	dma_int_cnt++;
	if(dma_int_cnt == 1)
	{
		dma_int_cnt = 0;
		bk_dma_stop(0);
		CLI_LOGI("bk_dma_stop \r\n");
	}
}



/*flash to ram */
static void dma2_config(void)
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

	BK_LOG_ON_ERR(bk_dma_init(2, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(2, 768)); //DMA_DEV_LCD_DATA
	BK_LOG_ON_ERR(bk_dma_register_isr(2, NULL, dma2_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(2));
	
	//CLI_LOGI("dma2 rgb565_data src addr is %x\r\n", (&rgb_565_red[0] + 767));
	//CLI_LOGI("data size= %x\r\n", (sizeof(rgb_565_red) / sizeof(rgb_565_red[0])));
	//CLI_LOGI("dma_get_src_read_addr = %x\r\n", dma_get_src_read_addr(2));
	//CLI_LOGI("dma_get_src_read_addr = %x\r\n", dma_get_dest_write_addr(2));
	//CLI_LOGI("dma2 src reg addr is %x\r\n", addGENER_DMA_Reg0x12);
	//CLI_LOGI("dma2 dest reg addr is %x\r\n", addGENER_DMA_Reg0x11);
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

	BK_LOG_ON_ERR(bk_dma_init(2, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(2, 768));
	BK_LOG_ON_ERR(bk_dma_register_isr(2, NULL, dma2_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(2));
}


/*
 * DMA0 SRC SM1 DEST I8080 DAT FIFO, FOREVER MODE.
 */
static void dma0_config(void)
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

	BK_LOG_ON_ERR(bk_dma_init(0, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(0, 768));
	BK_LOG_ON_ERR(bk_dma_register_isr(0, NULL, dma0_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
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



static void cli_lcd_help(void)
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
		bk_lcd_isr_register(lcd_isr);

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

static void lcd_dma_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		bk_lcd_deinit();

		memset((void *)0x30020000, 0, 768);
		CLI_LOGI("config dma2 from flash to sram,not start transfer \r\n \r\n");
		dma2_config(); //flash to sram

		CLI_LOGI("config dma0 from sram to fifo,not start transfer \r\n");
		dma0_config();
		bk_lcd_fifo_clr();


		CLI_LOGI("dma2 start transfer \r\n", dma_int_flag);
		BK_LOG_ON_ERR(bk_dma_start(2));

		while(dma_int_flag == 0);
		CLI_LOGI("dma2 transfer complete,data_from flash to sram ok\r\n");

		CLI_LOGI("lcd sys clk init \r\n");
		bk_lcd_init(1, 0, 2);
		gpio_lcd_8080_sel();

		CLI_LOGI("lcd reg ini \r\n");
		bk_lcd_disconti_mode(1);
		bk_lcd_8080_int_enable(1, 1);
		bk_lcd_8080_set_dat_fifo_thrd(96,96);
		bk_lcd_isr_register(lcd_isr);
		bk_lcd_pixel_config(X_PIXEL_8080, Y_PIXEL_8080);
		bk_lcd_isr_register(lcd_isr);

		bk_lcd_8080_display_enable(1);
		bk_lcd_8080_display_enable(0);
		bk_lcd_8080_reset();
		//cpu_delay(7017857); // 131ms
		lcd_st7796s_init();
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);
		cpu_delay(1000);

		CLI_LOGI("lcd 8080 display_enable \r\n");
		bk_lcd_8080_display_enable(1);
		lcd_st7796s_init();
		transfer_set(X_PIXEL_8080, Y_PIXEL_8080);

		bk_lcd_8080_start_transfer(1); //dat_on

		CLI_LOGI("dma0 start transfer \r\n");
		BK_LOG_ON_ERR(bk_dma_start(0));
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

static void lcd_rgb_init()
{
	bk_lcd_rgb_int_enable(1,1);
	bk_lcd_rgb_clk_div(3);
	bk_lcd_rgb_dclk_rev_edge(1);
	bk_lcd_pixel_config(x_pixel_V272P, y_pixel_V272P);
	bk_lcd_rgb_disp_sel();
	bk_lcd_disconti_mode(1);
	bk_lcd_rgb_fifo_thrd_set(0x60, 0x60);
	bk_lcd_sync_config(hsync_back_porch, hsync_front_porch, vsync_back_porch, vsync_front_porch);
	CLI_LOGI("lcd rgb yuv mode.\r\n");
	bk_lcd_rgb_data_format(1);
}


void dma2_config_yuv(void) //can use cpu memcpy
{
	dma_config_t dma_config = {0};

	dma_config.mode = 0;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) &yuv_dat[0];
	dma_config.src.end_addr =  (uint32) &yuv_dat[0] + 80; //153600

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr =   (uint32) sm1_addr;
	dma_config.dst.end_addr = (uint32) sm1_addr + 80;

	BK_LOG_ON_ERR(bk_dma_init(2, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(2, 80));
	//BK_LOG_ON_ERR(bk_dma_register_isr(2, NULL, dma2_finish_isr));
	//BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(2));
}

static void dma1_finish_isr(dma_id_t id)
{
	//dma_int_cnt++;
	//CLI_LOGI("enter dma1_finish_isr dma_int_cnt=%x\r\n", RGB_FRAME_DONE);
	if(RGB_FRAME_DONE == 1)
	{
		//dma_int_cnt = 0;
		bk_dma_stop(1);
		//CLI_LOGI("dma1_close \r\n");
	}
}

void dma1_config_yuv(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) sm1_addr;
	dma_config.src.end_addr =  (uint32) sm1_addr + 80;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) &REG_DISP_RGB_FIFO;

	BK_LOG_ON_ERR(bk_dma_init(1, &dma_config));
	//addGENER_DMA_Reg0xf = (0) + (11 << 4);
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(1, 80));
	BK_LOG_ON_ERR(bk_dma_register_isr(1, NULL, dma1_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(1));
}


static void lcd_rgb_dma_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) {
		cli_lcd_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		memset((void *)sm1_addr, 0, 80);
		//bk_psram_init(0x00054043);
		memcpy((void *)sm1_addr, &yuv_dat[0], 80);
		dma1_config_yuv();

		CLI_LOGI("display fifo mem clear\r\n");
		bk_lcd_fifo_clr();

		CLI_LOGI("lcd rgb io init.\r\n");
		gpio_lcd_rgb_sel();
		
		CLI_LOGI("lcd system core.\r\n");
		bk_lcd_init(1, 0, 2);
		
		CLI_LOGI("lcd rgb reg init.\r\n");
		lcd_rgb_init();
		bk_lcd_isr_register(lcd_isr);
		
		CLI_LOGI("lcd rgb disply enable.\r\n");
		bk_lcd_rgb_display_en(1);
		CLI_LOGI("dma start transfer.\r\n");
		BK_LOG_ON_ERR(bk_dma_start(1));
	}
	else
	{
		CLI_LOGI("unsupport cmd \r\n");
	}
}

static void lcd_green(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	CLI_LOGI("lcd_debug = start \r\n");

	if (os_strcmp(argv[1], "start") == 0) {
		dma2_config_green();
		CLI_LOGI("dma2 start transfer \r\n");
		BK_LOG_ON_ERR(bk_dma_start(2));
		while(dma_int_flag == 0);
		CLI_LOGI("dma2 transfer complete,data_from flash to sram ok\r\n");

		dma0_config();
		bk_lcd_fifo_clr();
		BK_LOG_ON_ERR(bk_dma_start(0));
		bk_lcd_8080_write_cmd(continue_write);
		}
	else {
		CLI_LOGI("unsupport cmd \r\n");
	}
}

#define LCD_CNT (sizeof(s_lcd_commands) / sizeof(struct cli_command))
static const struct cli_command s_lcd_commands[] = {
	{"lcd_no_dma_test", "lcd_no_dma_test {start|stop} {red|blue|green|yellow\r\n}", lcd_no_dma_test},
	{"lcd_dma_test", "lcd_dma_test {start|stop}\r\n", lcd_dma_test},
	{"lcd_debug", "lcd_debug {start|stop}", lcd_debug},
	{"lcd_green", "lcd_green {start|stop}", lcd_green},
	{"lcd_rgb_dma_test", "lcd_debug {start|stop}", lcd_rgb_dma_test},
	//{"lcd_rgb_jpeg_dma_test", "lcd_green {start|stop}", lcd_rgb_jpeg_dma_test},
};

int cli_lcd_init(void)
{
	return cli_register_commands(s_lcd_commands, LCD_CNT);
}








