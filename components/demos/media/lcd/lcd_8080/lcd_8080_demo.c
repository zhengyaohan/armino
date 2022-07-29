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
#include "BK7256_RegList.h"
#include <driver/dma2d.h>
#include "lcd_disp_ll_macro_def_mp2.h"

#if (CONFIG_SDCARD_HOST)
#include "ff.h"
#include "diskio.h"
#include "test_fatfs.h"
#endif


extern const uint8_t fg_blend_image1[];

#define LCD_FRAMEADDR   0x60000000 /**<define frame base addr */
#define LCD_FRAMEADDR2   0x60200000 /**<define frame base addr */

#define LCD_SIZE_X 320
#define LCD_SIZE_Y 480

#define LCD_POINT_DISPLAY_X 3
#define LCD_POINT_DISPLAY_Y 3

//CMD2A dafult value
#define ST7796_XS 0
#define ST7796_XE (LCD_SIZE_X - 1)
//CMD2B dafult value
#define ST7796_YS 0
#define ST7796_YE (LCD_SIZE_Y - 1)

//normal partical
#define PARTICAL_XS   0x65//101
#define PARTICAL_XE   0xdc//220
#define PARTICAL_YS   0x65//101
#define PARTICAL_YE   0x17c //380

//edge partical
#define EDGE_PARTICAL_XS   1
#define EDGE_PARTICAL_XE   220
#define EDGE_PARTICAL_YS   1
#define EDGE_PARTICAL_YE   380


uint8_t g_disp_frame_done_flag = 0;

void lcd_i8080_isr(void)
{
	g_disp_frame_done_flag = 1;
	os_printf("lcd_i8080_isr. \r\n");
}

static void dma2d_lcd_fill_test(uint32_t frameaddr, uint16_t width, uint16_t high, uint16_t color)
{
//	void *pDiSt=&(((uint16_t *)frameaddr)[y*320+x]);

	dma2d_config_t dma2d_config = {0};

	dma2d_config.init.mode	 = DMA2D_R2M;					   /**< Mode Register to Memory */
	dma2d_config.init.color_mode	   = DMA2D_OUTPUT_RGB565;  /**< DMA2D Output color mode is ARGB4444 (16 bpp) */
	dma2d_config.init.output_offset  = 0; 		   /**< offset in output */
	dma2d_config.init.red_blue_swap   = DMA2D_RB_REGULAR;		/**< No R&B swap for the output image */
	dma2d_config.init.alpha_inverted = DMA2D_REGULAR_ALPHA; 	/**< No alpha inversion for the output image */
	bk_dma2d_driver_init(&dma2d_config);
	bk_dma2d_start_transfer(&dma2d_config, color, frameaddr, width, high);
	while (bk_dma2d_is_transfer_busy()) {
	}
}

static void cpu_lcd_fill_test(uint32_t *addr, uint32_t color)
{
	uint32_t *p_addr = addr;
	os_printf("p_addr = %x", p_addr);
	for(int i=0; i<320*240; i++)
	{
		*(p_addr + i) = color;
	}
	
	os_printf("p_addr0 = %x\r\n", *p_addr);
	os_printf("p_addr1 = %x \r\n", *(p_addr + 1));
}

void lcd_8080_display_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t ARR_DISP_partial_3x3[9] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9};

	if (os_strcmp(argv[1], "dma2d_fill") == 0)
	{
		os_printf("psram init \r\n"),
		bk_psram_init();
		uint32_t frameaddr = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
		os_printf("frameaddr= %x \r\n", frameaddr);
		uint32_t width = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
		uint32_t high = os_strtoul(argv[4], NULL, 10) & 0xFFFF;
		uint32_t color = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
		os_printf("(width, high) = (%x %x )\r\n", width, high);
		os_printf("color = %x \r\n", color);
		dma2d_lcd_fill_test(frameaddr,width, high, color);
	}  else if (os_strcmp(argv[1], "cpu_fill") == 0) {
		bk_psram_init();
		uint32_t addr = os_strtoul(argv[2], NULL, 16) & 0xffffffff;
		os_printf("addr = (%x\r\n", addr);
		uint32_t color = os_strtoul(argv[3], NULL, 16) & 0xffffffff;
		os_printf("color = (%x\r\n", color);
		cpu_lcd_fill_test((uint32_t*) addr, color);
	} else if (os_strcmp(argv[1], "init") == 0) {
		//step1: fill FRAMEADDR data all oxffff init lcd
		os_printf("lcd driver init. \r\n");
		ret = bk_lcd_driver_init(LCD_80M);
		if (ret != BK_OK) {
			os_printf("bk_lcd_driver_init failed\r\n");
			return;
		}

		os_printf("i8080 lcd init. \r\n");
		ret = bk_lcd_8080_init(LCD_SIZE_X, LCD_SIZE_Y);
		if (ret != BK_OK) {
			os_printf("bk_lcd_8080_initinit failed\r\n");
			return;
		}
		ret = bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr);
		if (ret != BK_OK) {
			os_printf("bk_lcd_isr_register failed\r\n");
			return;
		}	
		bk_lcd_set_display_base_addr(LCD_FRAMEADDR);
		os_printf("base addr = %x\r\n", lcd_disp_ll_get_mater_rd_base_addr());
		ret = st7796s_init();
		if (ret != BK_OK) {
			os_printf("st7796s init failed\r\n");
			return;
		}
		os_printf("st7796 init ok. \r\n");
	} else if (os_strcmp(argv[1], "frame_disp") == 0) {
		//st7796s_set_display_mem_area(ST7796_XS, ST7796_XE, ST7796_YS, ST7796_YE);
		os_printf("bk_lcd_8080_start_transfer \r\n");
		bk_lcd_8080_start_transfer(1);
		os_printf("bk_lcd_8080_ram_write \r\n");
		bk_lcd_8080_ram_write(RAM_WRITE);
	} else if (os_strcmp(argv[1], "disp_continue") == 0) {
		//st7796s_set_display_mem_area(ST7796_XS, ST7796_XE, ST7796_YS, ST7796_YE);
		os_printf("bk_lcd_8080_ram_write \r\n");
		bk_lcd_8080_ram_write(CONTINUE_WRITE);
	}
	else if (os_strcmp(argv[1], "point_display")) {
//		st7796s_set_display_mem_area(LCD_POINT_DISPLAY_X, LCD_POINT_DISPLAY_X);
		bk_lcd_8080_send_cmd((sizeof(ARR_DISP_partial_3x3)/sizeof(ARR_DISP_partial_3x3[0])), 0x2c, ARR_DISP_partial_3x3);
	} else if (os_strcmp(argv[1], "point_and_frame_display")) {
		//step 3 sd card read to psram a 320*480 picture
		bk_lcd_set_display_base_addr(LCD_FRAMEADDR);
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(RAM_WRITE);
		while(g_disp_frame_done_flag == 0);
		g_disp_frame_done_flag=0;
		bk_lcd_8080_start_transfer(0);
		bk_lcd_8080_send_cmd((sizeof(ARR_DISP_partial_3x3)/sizeof(ARR_DISP_partial_3x3[0])), 0x2c, ARR_DISP_partial_3x3);
	} else if (os_strcmp(argv[1], "partical_display")) {
		bk_lcd_set_partical_display(PARTICAL_XS, PARTICAL_XE, PARTICAL_YS, PARTICAL_YE);
		//st7796s_set_display_mem_area(PARTICAL_XS, PARTICAL_XE, PARTICAL_YS, PARTICAL_YE);
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_ram_write(RAM_WRITE);
	}  else if (os_strcmp(argv[1], "edge_partical")) {
		bk_lcd_set_partical_display(EDGE_PARTICAL_XS, EDGE_PARTICAL_XE, EDGE_PARTICAL_YS, EDGE_PARTICAL_YE);
		//st7796s_set_display_mem_area(PARTICAL_XS, PARTICAL_XE, PARTICAL_YS, PARTICAL_YE);
		bk_lcd_8080_start_transfer(1);
		bk_lcd_8080_write_cmd(RAM_WRITE);
	} else if (os_strcmp(argv[1], "close") == 0) {
		bk_lcd_8080_deinit();
	}
}


