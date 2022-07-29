#include <common/bk_include.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>
#include <components/video_types.h>
#include <components/dvp_camera_types.h>
#include <os/mem.h>
#include "dvp_camera_config.h"
#include "video_transfer_common.h"
#include "video_mailbox.h"

static camera_config_t ejpeg_cfg;

static void video_transfer_jpeg_pixel_set(uint32_t ppi_type)
{
	switch (ppi_type) {
	case QVGA_320_240:
		ejpeg_cfg.x_pixel = X_PIXEL_320;
		ejpeg_cfg.y_pixel = Y_PIXEL_240;
		break;
	case VGA_480_272:
		ejpeg_cfg.x_pixel = X_PIXEL_480;
		ejpeg_cfg.y_pixel = Y_PIXEL_272;
		break;
	case VGA_640_480:
		ejpeg_cfg.x_pixel = X_PIXEL_640;
		ejpeg_cfg.y_pixel = Y_PIXEL_480;
		break;
	case VGA_800_600:
		ejpeg_cfg.x_pixel = X_PIXEL_800;
		ejpeg_cfg.y_pixel = Y_PIXEL_600;
		break;
	case VGA_1280_720:
		ejpeg_cfg.x_pixel = X_PIXEL_1280;
		ejpeg_cfg.y_pixel = Y_PIXEL_720;
		break;
	default:
		os_printf("cm PPI unknown, use QVGA\r\n");
		ejpeg_cfg.x_pixel = X_PIXEL_640;
		ejpeg_cfg.y_pixel = Y_PIXEL_480;
		break;
	}
}

void video_transfer_camera_register_set(void)
{
#if 0
	uint32_t size;
	switch (ejpeg_cfg.dev_id) {
	case PAS6329_DEV:
		s_camera_dev_id = PAS6329_DEV_ID;
		size = sizeof(pas6329_page0) / 2;
		PAS6329_SET_PAGE0;
		camera_inf_write_cfg_byte(pas6329_page0, size);
		size = sizeof(pas6329_page1) / 2;
		PAS6329_SET_PAGE1;
		camera_inf_write_cfg_byte(pas6329_page1, size);
		size = sizeof(pas6329_page2) / 2;
		PAS6329_SET_PAGE2;
		camera_inf_write_cfg_byte(pas6329_page2, size);
		PAS6329_SET_PAGE0;
		os_printf("PAS6329 init finish\r\n");
		break;
	case OV_7670_DEV:
		s_camera_dev_id = OV_7670_DEV_ID;
		size = sizeof(ov_7670_init_talbe) / 2;
		camera_inf_write_cfg_byte(ov_7670_init_talbe, size);
		os_printf("OV_7670 init finish\r\n");
		break;
	case PAS6375_DEV:
		s_camera_dev_id = PAS6375_DEV_ID;
		size = sizeof(pas6375_init_talbe) / 2;
		camera_inf_write_cfg_byte(pas6375_init_talbe, size);
		os_printf("PAS6375 init finish\r\n");
		break;
	case GC0328C_DEV:
		s_camera_dev_id = GC0328C_DEV_ID;
		size = sizeof(gc0328c_init_talbe) / 2;

		camera_inf_write_cfg_byte(gc0328c_init_talbe, size);

		camera_inf_cfg_gc0328c_ppi(CMPARAM_GET_PPI(ejpeg_cfg.sener_cfg));
		camera_inf_cfg_gc0328c_fps(CMPARAM_GET_FPS(ejpeg_cfg.sener_cfg));
		os_printf("GC0328C init finish\r\n");
		break;
	case BF_2013_DEV:
		s_camera_dev_id = BF_2013_DEV_ID;
		size = sizeof(bf_2013_init_talbe) / 2;
		camera_inf_write_cfg_byte(bf_2013_init_talbe, size);
		os_printf("BF_2013 init finish\r\n");
		break;
	case GC0308C_DEV:
		s_camera_dev_id = GC0308C_DEV_ID;
		size = sizeof(gc0308c_init_talbe) / 2;
		camera_inf_write_cfg_byte(gc0308c_init_talbe, size);
		os_printf("GC0308C init finish\r\n");
		break;
	case HM_1055_DEV:
		s_camera_dev_id = HM_1055_DEV_ID;
		size = sizeof(hm_1055_init_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_init_talbe, size);
		camera_inf_cfg_hm1055_fps(CMPARAM_GET_FPS(ejpeg_cfg.sener_cfg));
		os_printf("HM_1055 init finish\r\n");
		break;
	default:
		os_printf("NOT Find this sensor\r\n");
	}

	bk_jpeg_enc_dvp_gpio_enable();
#endif
}

static void video_transfer_camera_ppi_fps(uint32_t resolution, uint8_t fps)
{
	uint32_t width = (resolution >> 16) & 0xFFFF;
	uint32_t height = resolution & 0xFFFF;
	ejpeg_cfg.sener_cfg = 0;

	os_printf("%s, height:%d\r\n", __func__, height);
	if (width == 320 && height == 240) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, QVGA_320_240);
	} else if (width == 480 && height == 272) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_480_272);
	} else if (width == 640 && height == 480) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_640_480);
	} else if (width == 800 && height == 600) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_800_600);
	}else if (width == 1280 && height == 720) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_1280_720);
	} else {
		os_printf("not support this ppi, use default!\r\n");
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_640_480);
	}

	os_printf("%s, fps:%d\r\n", __func__, fps);
	switch (fps) {
		case 5:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_5FPS);
			break;
		case 10:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_10FPS);
			break;
		case 15:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_15FPS);
			break;
		case 20:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_20FPS);
			break;
		case 25:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_25FPS);
			break;
		case 30:
			CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, TYPE_30FPS);
			break;
		default:
			os_printf("not support this fps, use_default!\r\n");
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, TYPE_20FPS);
	}
}


void video_transfer_set_camera_cfg(video_transfer_setup_t *param)
{
	os_memset(&ejpeg_cfg, 0, sizeof(camera_config_t));
	video_transfer_camera_ppi_fps(param->resolution, param->frame_rate);
	if (param->dev_id > 7) {
		ejpeg_cfg.dev_id = 0xABC03;
	} else {
		ejpeg_cfg.dev_id = 0xABC00 + param->dev_id;
	}

	video_transfer_jpeg_pixel_set(CMPARAM_GET_PPI(ejpeg_cfg.sener_cfg));

	camera_intf_set_sener_cfg_value(ejpeg_cfg.sener_cfg);
}

bk_err_t video_transfer_get_camera_cfg(camera_config_t *config)
{
	if (config == NULL)
		return BK_FAIL;
	os_memcpy(config, &ejpeg_cfg, sizeof(camera_config_t));
	return BK_OK;
}

