#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <stdlib.h>
//#include <components/system.h>
#include "driver/lcd_disp_types.h"
#include <driver/lcd.h>
#include <driver/dma.h>
#include "bk_cli.h"
//#include "sys_driver.h"
#include <lcd_dma2d_config.h>
#include <driver/psram.h>
#include "lcd_spi_config.h"
#include <modules/pm.h>

#define LCD_FRAMEADDR     0x60000000

static uint32_t dma_transfer_len = 0;
uint8_t lcd_dma_channel = 0;
uint8_t lcd_dma_cnt = 0;
uint8_t dma_all_cnt = 0;

static void dma_gc9503v_to_lcdfifo_isr(dma_id_t id)
{
	uint32_t source_start_addr;
	lcd_dma_cnt++;
	if(lcd_dma_cnt == dma_all_cnt)
	{
		lcd_dma_cnt = 0;
		source_start_addr = (uint32_t)LCD_FRAMEADDR;
		bk_dma_set_src_addr(lcd_dma_channel, source_start_addr, 0);
		bk_dma_start(lcd_dma_channel);
	}
	else {
		source_start_addr = (uint32_t)LCD_FRAMEADDR + (uint32_t)(dma_transfer_len * lcd_dma_cnt);
		bk_dma_set_src_addr(lcd_dma_channel, source_start_addr, 0);
		bk_dma_start(lcd_dma_channel);
	}
}

static void dma_frame_data_to_lcdfifo(void)
{
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) LCD_FRAMEADDR;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	dma_config.dst.start_addr =  (uint32) REG_DISP_RGB_FIFO;


	lcd_dma_channel = bk_dma_alloc(DMA_DEV_DTCM);
	if ((lcd_dma_channel < DMA_ID_0) || (lcd_dma_channel >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}

	BK_LOG_ON_ERR(bk_dma_init(lcd_dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(lcd_dma_channel, dma_transfer_len));
	BK_LOG_ON_ERR(bk_dma_register_isr(lcd_dma_channel, NULL, dma_gc9503v_to_lcdfifo_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(lcd_dma_channel));
}

void lcd_display_frame_isr(void)
{}

void create_a_red_pic(uint16_t w, uint16_t h, uint16_t color, uint32_t div)
{
	dm2d_lcd_create_picture(LCD_FRAMEADDR, w, h, color);

	bk_lcd_driver_init(LCD_96M);

	bk_lcd_rgb_init(div, w, h, RGB565_DATA);

	bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_display_frame_isr);

	dma_frame_data_to_lcdfifo();

	bk_lcd_rgb_display_en(1);
	bk_dma_start(lcd_dma_channel);
}

void lcd_device_init_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "init") == 0) {
		// step 1 : init psram
		uint32_t psram_mode = 0x00054043;
		bk_psram_init(psram_mode);

		// step 2: init gpio
		lcd_init_gpio();
		spi_init_gpio();
		//sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_ON);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_DMA2D, PM_POWER_MODULE_STATE_ON);
	} else if (os_strcmp(argv[1], "gc9503v") == 0) {
		uint32_t time = 0;

		time = os_strtoul(argv[2], NULL, 10);
		lcd_set_spi_delay_time(time);
		
		lcd_gc9503v_set_config();
		os_printf("GC9503V init finish!\r\n");
	} else if (os_strcmp(argv[1], "st7701s") == 0) {
		uint32_t time = 0;
		time = os_strtoul(argv[2], NULL, 10);
		lcd_set_spi_delay_time(time);
		lcd_st7701s_set_config();
		os_printf("st7710s init finish!\r\n");
	} else if (os_strcmp(argv[1], "display") == 0) {
		uint16_t width = 0;
		uint16_t height = 0;
		uint16_t color = 0xf800;
		uint32_t dis_clk_div = 0;

		width = os_strtoul(argv[2], NULL, 10);
		height = os_strtoul(argv[3], NULL, 10);

		if (width * height == 0x1FE00) {
			dma_transfer_len = 0x1FE00 / 2;
			dma_all_cnt = 4;
		} else {
			dma_transfer_len = 0xC800;
			dma_all_cnt = 15;
		}

		dis_clk_div = os_strtoul(argv[4], NULL, 10);
		color = os_strtoul(argv[5], NULL, 16);
		os_printf("color:%04x\r\n", color);

		create_a_red_pic(width, height, color, dis_clk_div);
	}else if (os_strcmp(argv[1], "close") == 0)  {
		// setp 1: stop display
		bk_lcd_rgb_display_en(0);

		// step 2: stop and deinit dma
		bk_dma_stop(lcd_dma_channel);
		bk_dma_deinit(lcd_dma_channel);

		bk_dma_free(DMA_DEV_DTCM, lcd_dma_channel);
		lcd_dma_cnt = 0;
		os_printf("close ok\r\n");
	} else {
		os_printf("CMD not found\r\n");
	}
}


