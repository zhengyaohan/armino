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

void lcd_display_frame_isr(void)
{

}

#define write_data(addr,val)                 *((volatile uint32_t *)(addr)) = val

void lcd_device_init_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

	os_printf(" lcd_device_init_handler \r\n");

	if (os_strcmp(argv[1], "init") == 0) {
		// step 1 : init psram
		
		bk_psram_init();
		// step 2: init gpio
		lcd_init_gpio();

		os_printf("init finish!\r\n");
	} else if (os_strcmp(argv[1], "gc9503v") == 0) {
		spi_init_gpio(); 
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
		//uint32_t color;

		width = os_strtoul(argv[2], NULL, 10);
		height = os_strtoul(argv[3], NULL, 10);
		//color = os_strtoul(argv[4], NULL, 16)  & 0xFFFFFFFF;
		//os_printf("color:%08x\r\n", color);
		for (uint32_t i = 0; i < 480 * 800 / 2; i++)
			write_data((0x60000000 + i * 0x4), 0xF800F800);
		os_printf("W  H %x, %x\r\n",width,height );

		//dm2d_lcd_create_picture(LCD_FRAMEADDR, width, height, color);
		
		os_printf("bk_lcd_driver_init\r\n");
		bk_lcd_driver_init(LCD_26M);
		bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_display_frame_isr);
		
		os_printf("bk_lcd_rgb_init\r\n");
		bk_lcd_rgb_init(0, width, height, RGB565_DATA);
		bk_lcd_set_display_base_addr(LCD_FRAMEADDR);
		
		os_printf("bk_lcd_rgb_display_en\r\n");
		bk_lcd_rgb_display_en(1);
	} else {
		os_printf("CMD not found\r\n");
	}
}








