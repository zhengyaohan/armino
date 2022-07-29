#include "cli.h"

extern void lcd_8080_display_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
//extern void lcd_rgb_display_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_display_jpeg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_display_yuv(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
extern void lcd_rgb_close(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#define LCD_CNT (sizeof(s_lcd_commands) / sizeof(struct cli_command))
static const struct cli_command s_lcd_commands[] = {
	{"lcd_8080", "init|fram_disp", lcd_8080_display_test},
	//{"lcd_rgb", "picture_jpeg_dec", lcd_rgb_display_test},
	{"lcd_display_yuv", "lcd_display_yuv=480p | 720p", lcd_rgb_display_yuv},
	{"lcd_display_jpeg", "lcd_display_jpeg =480p | 720p", lcd_rgb_display_jpeg},
	{"lcd_close", " 480p | 720p", lcd_rgb_close},
};

int cli_lcd_init(void)
{
	return cli_register_commands(s_lcd_commands, LCD_CNT);
}








