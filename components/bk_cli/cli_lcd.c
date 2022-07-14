#include "cli.h"

extern void lcd_help(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void sdcard_write_from_mem(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void sdcard_read_to_mem(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);


extern void lcd_8080_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_fill(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_8080_sdcard_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_8080_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

extern void lcd_rgb_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

extern void lcd_rgb_display_jpeg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_display_yuv(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_display_yuv_blending(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

extern void lcd_rgb_sdcard_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_sdcard_read_to_mem_jpeg_dec(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

extern void lcd_device_init_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void cli_cp0_lcd_rgb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#define LCD_CNT (sizeof(s_lcd_commands) / sizeof(struct cli_command))
static const struct cli_command s_lcd_commands[] = {
	{"lcd", "lcd help", lcd_help },
	{"lcd_8080_init", "lcd_8080_init {start|stop}\r\n", lcd_8080_init},
	{"lcd_fill", "lcd_dma2d_fill", lcd_fill},
	{"lcd_8080_sdcard_test", "sd w/r and image scale and pfc", lcd_8080_sdcard_test},
	{"lcd_8080_close", "lcd_8080_close", lcd_8080_close},

	{"lcd_video", "lcd_video=9,25", lcd_rgb_display_yuv},
	{"lcd_video_blend", "lcd_video_blend=15,25", lcd_rgb_display_yuv_blending},
	{"lcd_video_jpeg_dec", "lcd_video_jpeg_dec = 14,20", lcd_rgb_display_jpeg},
	{"lcd_rgb_close", "lcd_close=yuv_display|jpeg_display", lcd_rgb_close},

	{"lcd_rgb_init", "rgb_clk_div,yuv_mode,dma_dst_w", lcd_rgb_init},
	{"picture_jpeg_dec", "picture_jpeg_dec", lcd_sdcard_read_to_mem_jpeg_dec},
	{"lcd_rgb_sdcard_test", "sd w/r and image scale and pfc", lcd_rgb_sdcard_test},
	{"sdcard_write_from_mem", "filename,wxpixel,ypixel,fileaddr", sdcard_write_from_mem},
	{"sdcard_read_to_mem", "filename,|fileaddr", sdcard_read_to_mem},

	{"lcd_dev", "init gc9503v|st7710s demo", lcd_device_init_handler},
	{"lcd_rgb", " {display} |{close}|{capture name}", cli_cp0_lcd_rgb_cmd},

	
};

int cli_lcd_init(void)
{
	return cli_register_commands(s_lcd_commands, LCD_CNT);
}








