#include "at_common.h"
#include <driver/jpeg_enc.h>
#include <driver/i2c.h>
#include <driver/dma.h>
#include <components/video_transfer.h>
#include <components/dvp_camera.h>
#include <driver/psram.h>

extern void delay(int num);//TODO fix me


#if CONFIG_PSRAM
int video_read_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int video_disable_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
#endif

#if (CONFIG_CAMERA && CONFIG_PSRAM)
int video_set_yuv_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

int video_close_yuv_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

#endif //CONFIG_CAMERA && CONFIG_PSRAM
const at_command_t video_at_cmd_table[] = {
#if CONFIG_PSRAM
	{2, "READPSRAM", 0, "psram", video_read_psram_handler},
	{3, "DEINITPSRAM", 0, "deinit psram", video_disable_psram_handler},
#endif
#if (CONFIG_CAMERA && CONFIG_PSRAM)
	{4, "SETYUV", 0, "set jpeg/yuv mode and to psram", video_set_yuv_psram_handler},
	{5, "CLOSEYUV", 0, "close jpeg", video_close_yuv_psram_handler},
#endif
};

int video_at_cmd_cnt(void)
{
	return sizeof(video_at_cmd_table) / sizeof(video_at_cmd_table[0]);
}


#if CONFIG_PSRAM
int video_read_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;
	uint8_t i = 0;

	uint32_t psram = 0x60000000;
	uint32_t mode = 0x00054043;
	if (argc != 0) {
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

	err = bk_psram_init(mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		err = kParamErr;
		goto error;
	}

	os_memset((uint8_t *)psram, 0, 30);

	for (i = 0; i < 30; i++) {
		*((uint8_t *)psram + i) = i;
	}

	os_printf("data:\n");
	for (i = 0; i <30; i++) {
		os_printf("%d ", *((uint8_t *)psram + i));
	}
	os_printf("\n");

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}

int video_disable_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;

	if (argc != 0) {
			os_printf("input param error\n");
			err = kParamErr;
			goto error;
	}

	err = bk_psram_deinit();
	if (err != kNoErr) {
		os_printf("psram deinit error\n");
		err = kParamErr;
		goto error;
	}

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}
#endif // CONFIG_PSRAM

#if (CONFIG_CAMERA && CONFIG_PSRAM)
static void end_of_jpeg_frame(jpeg_unit_t id, void *param)
{
	//os_printf("%s, %d\n", __func__, __LINE__);
}

static void end_of_yuv_frame(jpeg_unit_t id, void *param)
{
	//os_printf("%s, %d\n", __func__, __LINE__);
}

int video_set_yuv_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;
	uint8_t yuv_mode = 0;
	uint16_t display_pixel = 0;
	jpeg_config_t jpeg_config = {0};
	i2c_config_t i2c_config = {0};
	uint32_t ppi, fps;
	uint32_t psram_mode = 0x00054043;

	if (argc != 4) {
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}

	yuv_mode = os_strtoul(argv[0], NULL, 16) & 0xFF;
	if (yuv_mode > 1) {
		os_printf("input param error\n");
		err = kParamErr;
		goto error;
	}
	err = bk_psram_init(psram_mode);
	if (err != kNoErr) {
		os_printf("psram init error\n");
		err = kParamErr;
		goto error;
	}

#if CONFIG_SYSTEM_CTRL
	err = bk_jpeg_enc_driver_init();
	if (err != kNoErr) {
		os_printf("video init error\n");
		err = kParamErr;
		goto error;
	}
#endif

	display_pixel = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	switch (display_pixel) {
		case 240:
			jpeg_config.x_pixel = X_PIXEL_320;
			jpeg_config.y_pixel = Y_PIXEL_240;
			ppi = QVGA_320_240;
			break;
		case 272:
			jpeg_config.x_pixel = X_PIXEL_480;
			jpeg_config.y_pixel = Y_PIXEL_272;
			ppi = VGA_480_272;
			break;
		case 480:
			jpeg_config.x_pixel = X_PIXEL_640;
			jpeg_config.y_pixel = Y_PIXEL_480;
			ppi = VGA_640_480;
			break;
		case 600:
			jpeg_config.x_pixel = X_PIXEL_800;
			jpeg_config.y_pixel = Y_PIXEL_600;
			ppi = VGA_800_600;
			break;
		case 720:
			jpeg_config.x_pixel = X_PIXEL_1280;
			jpeg_config.y_pixel = Y_PIXEL_720;
			ppi = VGA_1280_720;
			break;
		default:
			os_printf("input pixel param error\n");
			err = kParamErr;
			goto error;
	}

	fps = os_strtoul(argv[2], NULL, 10) & 0xFF;
	switch (fps) {
		case 5:
			fps = TYPE_5FPS;
			break;
		case 10:
			fps = TYPE_10FPS;
			break;
		case 15:
			fps = TYPE_15FPS;
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
			err = kParamErr;
			goto error;
	}

	jpeg_config.yuv_mode = yuv_mode;

	if (ppi == VGA_1280_720) {
		jpeg_config.sys_clk_div = 3;
		jpeg_config.mclk_div = 2;
	}else {
		jpeg_config.sys_clk_div = 4;
		jpeg_config.mclk_div = 0;
	}

	err = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (err != kNoErr) {
		os_printf("jpeg init error\n");
		err = kParamErr;
		goto error;
	}

	bk_jpeg_enc_register_isr(END_OF_FRAME, end_of_jpeg_frame, NULL);
	bk_jpeg_enc_register_isr(END_OF_YUV, end_of_yuv_frame, NULL);

	i2c_config.baud_rate = os_strtoul(argv[3], NULL, 10) & 0xFFFFFFFF;//100000;// 400k
	i2c_config.addr_mode = 0;
	err = bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
	if (err != kNoErr) {
		os_printf("i2c init error\n");
		err = kParamErr;
		goto error;
	}

	err = bk_camera_set_ppi_fps(ppi, fps);
	if (err != kNoErr) {
		os_printf("set camera ppi and fps error\n");
		err = kParamErr;
		goto error;
	}

	bk_camera_sensor_config();
	os_printf("camera init ok\n");

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));

	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}

int video_close_yuv_psram_handler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int err = kNoErr;

	err = bk_jpeg_enc_dvp_deinit();
	if (err != kNoErr) {
		os_printf("jpeg deinit error\n");
		err = kParamErr;
		goto error;
	}
	os_printf("jpeg deinit ok!\n");

	err = bk_i2c_deinit(CONFIG_CAMERA_I2C_ID);
	if (err != kNoErr) {
		os_printf("i2c deinit error\n");
		err = kParamErr;
		goto error;
	}
	os_printf("I2c deinit ok!\n");

#if CONFIG_SYSTEM_CTRL
	err = bk_jpeg_enc_driver_deinit();
	if (err != kNoErr) {
		os_printf("video deinit error\n");
		err = kParamErr;
		goto error;
	}
	os_printf("video deinit ok!\n");
#endif

	delay(2000);
	err = bk_psram_deinit();
	if (err != kNoErr) {
		os_printf("psram deinit error\n");
		err = kParamErr;
		goto error;
	}
	os_printf("psram deinit ok!\n");

	msg = AT_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return kNoErr;

error:
	msg = AT_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return err;
}
#endif // CONFIG_CAMERA && CONFIG_PSRAM

