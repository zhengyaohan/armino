// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <common/bk_include.h>
#if CONFIG_CAMERA
#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/psram.h>
#include <os/mem.h>
#include <driver/timer.h>
#include <components/dvp_camera.h>
#include "bk_arm_arch.h"
#include <components/video_transfer.h>
#include "dvp_camera_config.h"
#include "bk_drv_model.h"

#if CONFIG_GENERAL_DMA
#include "bk_general_dma.h"
#endif

#if CONFIG_VIDEO_LCD
#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/jpeg_dec.h>
#include <lcd_dma2d_config.h>
#include <driver/jpeg_dec_types.h>
#endif

#define EJPEG_DELAY_HTIMER_CHANNEL     5
#define EJPEG_DELAY_HTIMER_VAL         (2)  // 2ms
#define EJPEG_I2C_DEFAULT_BAUD_RATE    I2C_BAUD_RATE_100KHZ
#define I2C_WIRTE_TIMEOUT_MS           2000


static jpegenc_desc_t ejpeg_cfg;
static uint32_t s_camera_dev_id = 0;
static uint32_t s_camera_dev = GC0328C_DEV;
static uint32_t s_camera_sensor = 0x01E00014;//(480 << 16) | 20;

#if CONFIG_VIDEO_LCD
static uint8_t tvideo_lcd_enable = 0;
uint8_t * video_jpeg_buff = NULL;
static uint8_t  lcd_status = READY;

static void jpeg_dec_eof_cb(void *param)
{
	lcd_status = DISPLAYING;
	if (ejpeg_cfg.jpeg_dec_pixel_x == JPEGDEC_X_PIXEL_640) {
		dma2d_crop_params_t  crop_params;
		crop_params.dst_addr = (uint32_t)ejpeg_cfg.lcd_display_addr;
		crop_params.src_addr = (uint32_t)ejpeg_cfg.jpeg_dec_dst_addr;
		crop_params.x = (640 - 480)/2;
		crop_params.y = (480 - 272)/2;
//		crop_params.x = 0;
//		crop_params.y = 0;
		crop_params.src_width = 640;
		crop_params.src_height = 480;
		crop_params.dst_width = 480;
		crop_params.dst_height = 272;
		dma2d_crop_image(&crop_params);
	}
	bk_lcd_rgb_display_en(1);
	//bk_dma_set_src_start_addr(ejpeg_cfg.dma_lcd_channel, (uint32_t)ejpeg_cfg.jpeg_dec_lcd_buf);
	bk_dma_start(ejpeg_cfg.dma_lcd_channel);
}

static void dma_rgb_finish_isr(dma_id_t id)
{
	bk_gpio_set_output_high(GPIO_2);

	ejpeg_cfg.dma_lcd_int_cnt ++;
	if (ejpeg_cfg.dma_lcd_int_cnt == 4)
	{
		ejpeg_cfg.dma_lcd_int_cnt = 0;
		bk_dma_set_src_start_addr(ejpeg_cfg.dma_lcd_channel, (uint32_t)ejpeg_cfg.lcd_display_addr);
		lcd_status = READY;
	}
	else {
		bk_dma_set_src_start_addr(ejpeg_cfg.dma_lcd_channel, ((uint32_t)ejpeg_cfg.lcd_display_addr + (uint32_t)(65280 * ejpeg_cfg.dma_lcd_int_cnt)));
		bk_dma_start(ejpeg_cfg.dma_lcd_channel);
	}
	bk_gpio_set_output_low(GPIO_2);
}

static void jpeg_dec_callback(void *src, void *dst)
{
	lcd_status = JPEGDECING;

	bk_jpeg_dec_init((uint32_t *)src, (uint32_t *)dst);
}
#endif

static void camera_intf_delay_timer_hdl(timer_id_t timer_id)
{
#if CONFIG_GENERAL_DMA
	uint16_t already_len = ejpeg_cfg.rx_read_len;
	GLOBAL_INT_DECLARATION();
	uint32_t left_len = bk_dma_get_remain_len(ejpeg_cfg.dma_channel);
	uint32_t rec_len = ejpeg_cfg.node_len - left_len;
	uint32_t frame_len = bk_jpeg_enc_get_frame_size();

#if CONFIG_VIDEO_LCD
	if (tvideo_lcd_enable) {
		if (lcd_status == MEMCPYING) {
			dma_memcpy(video_jpeg_buff, ejpeg_cfg.rxbuf + already_len, rec_len);
			video_jpeg_buff = (uint8_t *)0x60000000; //ejpeg_cfg.jpeg_lcd_data_buf;
			lcd_status = JPEGDE_START;
		}
		if ((ejpeg_cfg.jpeg_dec_callback != NULL) && (rec_len > 0) && (lcd_status == JPEGDE_START)) {
				ejpeg_cfg.jpeg_dec_callback((uint32_t *)ejpeg_cfg.jpeg_dec_src_addr, (uint32_t *)ejpeg_cfg.jpeg_dec_dst_addr);
		}
	}
#endif

	if ((ejpeg_cfg.node_full_handler != NULL) && (rec_len > 0))
		ejpeg_cfg.node_full_handler(ejpeg_cfg.rxbuf + already_len, rec_len, 1, frame_len);
	already_len += rec_len;
	if (already_len >= ejpeg_cfg.rxbuf_len)
		already_len -= ejpeg_cfg.rxbuf_len;
	GLOBAL_INT_DISABLE();
	ejpeg_cfg.rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
	// turn off dma, so dma can start from first configure. for easy handler
	bk_dma_stop(ejpeg_cfg.dma_channel);
	ejpeg_cfg.rx_read_len = 0;
	bk_dma_start(ejpeg_cfg.dma_channel);
#endif
	if ((ejpeg_cfg.data_end_handler))
		ejpeg_cfg.data_end_handler();
#if (!CONFIG_SYSTEM_CTRL)
	bk_timer_disable(EJPEG_DELAY_HTIMER_CHANNEL);
#endif
}

static void camera_intf_start_delay_timer(void)
{
#if (!CONFIG_SYSTEM_CTRL)
	bk_timer_start(EJPEG_DELAY_HTIMER_CHANNEL, EJPEG_DELAY_HTIMER_VAL, camera_intf_delay_timer_hdl);
#else
	camera_intf_delay_timer_hdl(EJPEG_DELAY_HTIMER_CHANNEL);
#endif
}

static void camera_intf_ejpeg_rx_handler(dma_id_t dma_id)
{
	uint16_t already_len = ejpeg_cfg.rx_read_len;
	uint16_t copy_len = ejpeg_cfg.node_len;
	GLOBAL_INT_DECLARATION();

#if CONFIG_VIDEO_LCD
	if(tvideo_lcd_enable) {
		if ((*(ejpeg_cfg.rxbuf + already_len) == 0xff) && (*(ejpeg_cfg.rxbuf + already_len + 1) == 0xd8) && (lcd_status == READY)) {
			lcd_status = MEMCPYING;
		}
		if (lcd_status == MEMCPYING) {
			dma_memcpy(video_jpeg_buff, ejpeg_cfg.rxbuf + already_len, copy_len);
			video_jpeg_buff += ejpeg_cfg.node_len;
		}
	}
#endif

	if (ejpeg_cfg.node_full_handler != NULL)
		ejpeg_cfg.node_full_handler(ejpeg_cfg.rxbuf + already_len, copy_len, 0, 0);
	already_len += copy_len;
	if (already_len >= ejpeg_cfg.rxbuf_len)
		already_len = 0;
	GLOBAL_INT_DISABLE();
	ejpeg_cfg.rx_read_len = already_len;
	GLOBAL_INT_RESTORE();
}

static void camera_intf_ejpeg_end_handler(jpeg_unit_t id, void *param)
{
	camera_intf_start_delay_timer();
}

static void camera_intf_init_ejpeg_pixel(uint32_t ppi_type)
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
		CAMERA_LOGW("cm PPI unknown, use QVGA\r\n");
		ejpeg_cfg.x_pixel = X_PIXEL_640;
		ejpeg_cfg.y_pixel = Y_PIXEL_480;
		break;
	}
}

static void camera_intf_set_sener_cfg(uint32_t cfg)
{
	uint32_t ppi = (cfg >> 16) & 0xFFFF;
	uint32_t fps = cfg & 0xFFFF;
	ejpeg_cfg.sener_cfg = 0;

	CAMERA_LOGI("%s, ppi:%d\r\n", __func__, ppi);
	switch (ppi) {
		case 240:
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, QVGA_320_240);
			break;
		case 272:
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_480_272);
			break;
		case 480:
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_640_480);
			break;
		case 600:
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_800_600);
			break;
		case 720:
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_1280_720);
			break;
		default:
			CAMERA_LOGE("not support this ppi\r\n");
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, VGA_640_480);
	}

	CAMERA_LOGI("%s, fps:%d\r\n", __func__, fps);
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
			CAMERA_LOGE("not support this fps\r\n");
			CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, TYPE_20FPS);
	}
}

static void camera_intf_config_ejpeg(void *data)
{
	os_memset(&ejpeg_cfg, 0, sizeof(jpegenc_desc_t));
	os_memcpy(&ejpeg_cfg, data, sizeof(video_config_t));
	camera_intf_set_sener_cfg(s_camera_sensor);
	camera_intf_init_ejpeg_pixel(CMPARAM_GET_PPI(ejpeg_cfg.sener_cfg));

#if CONFIG_GENERAL_DMA
	ejpeg_cfg.dma_rx_handler = camera_intf_ejpeg_rx_handler;
	ejpeg_cfg.dma_channel = bk_dma_alloc(DMA_DEV_JPEG);
	if ((ejpeg_cfg.dma_channel < DMA_ID_0) || (ejpeg_cfg.dma_channel >= DMA_ID_MAX)) {
		CAMERA_LOGE("malloc dma fail \r\n");
		return;
	}
#endif
}

static void camera_intf_sccb_write_byte(uint8_t addr, uint8_t data)
{
	i2c_mem_param_t mem_param = {0};
	mem_param.dev_addr = s_camera_dev_id;
	mem_param.mem_addr = addr;
	mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
	mem_param.data = &data;
	mem_param.data_size = 1;
	mem_param.timeout_ms = I2C_WIRTE_TIMEOUT_MS;
	BK_LOG_ON_ERR(bk_i2c_memory_write(CONFIG_CAMERA_I2C_ID, &mem_param));
}

static void camera_intf_sccb_write_word(uint16_t addr, uint8_t data)
{
	i2c_mem_param_t mem_param = {0};
	mem_param.dev_addr = s_camera_dev_id;
	mem_param.mem_addr = addr;
	mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
	mem_param.data = &data;
	mem_param.data_size = 1;
	mem_param.timeout_ms = I2C_WIRTE_TIMEOUT_MS;
	BK_LOG_ON_ERR(bk_i2c_memory_write(CONFIG_CAMERA_I2C_ID, &mem_param));
}

__attribute__((unused)) static void camera_intf_sccb_read_byte(uint8_t addr, uint8_t *data)
{
	i2c_mem_param_t mem_param = {0};
	mem_param.dev_addr = s_camera_dev_id;
	mem_param.mem_addr = addr;
	mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_8BIT;
	mem_param.data = data;
	mem_param.data_size = 1;
	mem_param.timeout_ms = I2C_WIRTE_TIMEOUT_MS;
	BK_LOG_ON_ERR(bk_i2c_memory_read(CONFIG_CAMERA_I2C_ID, &mem_param));
}

__attribute__((unused)) static void camera_intf_sccb_read_word(uint16_t addr, uint8_t *data)
{
	i2c_mem_param_t mem_param = {0};
	mem_param.dev_addr = s_camera_dev_id;
	mem_param.mem_addr = addr;
	mem_param.mem_addr_size = I2C_MEM_ADDR_SIZE_16BIT;
	mem_param.data = data;
	mem_param.data_size = 1;
	mem_param.timeout_ms = I2C_WIRTE_TIMEOUT_MS;
	BK_LOG_ON_ERR(bk_i2c_memory_read(CONFIG_CAMERA_I2C_ID, &mem_param));
}

static void camera_inf_write_cfg_byte(const uint8_t (*cfg_table)[2], uint32_t size)
{
	uint8_t addr = 0;
	uint8_t data = 0;
	for (uint32_t i = 0; i < size; i++) {
		addr = cfg_table[i][0];
		data = cfg_table[i][1];
		camera_intf_sccb_write_byte(addr, data);
	}
}

static void camera_inf_write_cfg_word(const uint16_t (*cfg_table)[2], uint32_t size)
{
	uint16_t addr = 0;
	uint8_t data = 0;
	for (uint32_t i = 0; i < size; i++) {
		addr = cfg_table[i][0];
		data = cfg_table[i][1];
		camera_intf_sccb_write_word(addr, data);
	}
}

#if 0//(CONFIG_SYSTEM_CTRL)
static void camera_inf_write_init_table(const uint8_t (*cfg_table)[2], uint32_t size)
{
	uint8_t addr = 0;
	uint8_t data = 0;
	for (uint32_t i = 0; i < size; i++) {
		/*if (i == (size - 1)) {
			bk_jpeg_enc_dvp_gpio_enable();
		}*/
		addr = cfg_table[i][0];
		data = cfg_table[i][1];
		camera_intf_sccb_write(addr, data);
	}
}
#endif

static void camera_inf_cfg_gc0328c_ppi(uint32_t ppi_type)
{
	uint32_t size;
	switch (ppi_type) {
	case VGA_480_272:
		size = sizeof(gc0328c_WQVGA_480_272_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_WQVGA_480_272_talbe, size);
		break;
	case QVGA_320_240:
		size = sizeof(gc0328c_QVGA_320_240_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_QVGA_320_240_talbe, size);
		break;
	case VGA_640_480:
		size = sizeof(gc0328c_VGA_640_480_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_VGA_640_480_talbe, size);
		break;
	default:
		CAMERA_LOGW("set PPI unknown\r\n");
		size = sizeof(gc0328c_VGA_640_480_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_VGA_640_480_talbe, size);
	}
}

static void camera_inf_cfg_gc0328c_fps(uint32_t fps_type)
{
	uint32_t size;
	switch (fps_type) {
	case TYPE_5FPS:
		size = sizeof(gc0328c_5pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_5pfs_talbe, size);
		break;
	case TYPE_10FPS:
		size = sizeof(gc0328c_10pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_10pfs_talbe, size);
		break;
	case TYPE_20FPS:
		size = sizeof(gc0328c_20pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_20pfs_talbe, size);
		break;
	case TYPE_25FPS:
		size = sizeof(gc0328c_25pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_25pfs_talbe, size);
		break;
	case TYPE_30FPS:
		size = sizeof(gc0328c_30pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_30pfs_talbe, size);
		break;
	default:
		CAMERA_LOGW("set FPS unknown\r\n");
		size = sizeof(gc0328c_20pfs_talbe) / 2;
		camera_inf_write_cfg_byte(gc0328c_20pfs_talbe, size);
	}
}

static void camera_inf_cfg_hm1055_fps(uint32_t fps_type)
{
	uint32_t size;
	switch (fps_type) {
	case TYPE_5FPS:
		size = sizeof(hm_1055_720P_5fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_5fps_talbe, size);
		break;
	case TYPE_10FPS:
		size = sizeof(hm_1055_720P_10fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_10fps_talbe, size);
		break;
	case TYPE_15FPS:
		size = sizeof(hm_1055_720P_15fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_15fps_talbe, size);
		break;
	case TYPE_20FPS:
		/*size = sizeof(hm_1055_720P_20fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_20fps_talbe, size);*/
		break;
	case TYPE_25FPS:
		size = sizeof(hm_1055_720P_25fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_25fps_talbe, size);
		break;
	case TYPE_30FPS:
		size = sizeof(hm_1055_720P_30fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_30fps_talbe, size);
		break;
	default:
		CAMERA_LOGW("set FPS unknown, default 20 fps\r\n");
		size = sizeof(hm_1055_720P_20fps_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_720P_20fps_talbe, size);
	}
}

void bk_camera_sensor_config(void)
{
	uint32_t size;
	switch (s_camera_dev) {
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
		CAMERA_LOGI("PAS6329 init finish\r\n");
		break;
	case OV_7670_DEV:
		s_camera_dev_id = OV_7670_DEV_ID;
		size = sizeof(ov_7670_init_talbe) / 2;
		camera_inf_write_cfg_byte(ov_7670_init_talbe, size);
		CAMERA_LOGI("OV_7670 init finish\r\n");
		break;
	case PAS6375_DEV:
		s_camera_dev_id = PAS6375_DEV_ID;
		size = sizeof(pas6375_init_talbe) / 2;
		camera_inf_write_cfg_byte(pas6375_init_talbe, size);
		CAMERA_LOGI("PAS6375 init finish\r\n");
		break;
	case GC0328C_DEV:
		s_camera_dev_id = GC0328C_DEV_ID;
		size = sizeof(gc0328c_init_talbe) / 2;

#if 1//(!CONFIG_SYSTEM_CTRL)
		camera_inf_write_cfg_byte(gc0328c_init_talbe, size);
#else
		camera_inf_write_init_table(gc0328c_init_talbe, size);
#endif
		camera_inf_cfg_gc0328c_ppi(CMPARAM_GET_PPI(ejpeg_cfg.sener_cfg));
		camera_inf_cfg_gc0328c_fps(CMPARAM_GET_FPS(ejpeg_cfg.sener_cfg));
		bk_jpeg_enc_dvp_gpio_enable();
		CAMERA_LOGI("GC0328C init finish\r\n");
		break;
	case BF_2013_DEV:
		s_camera_dev_id = BF_2013_DEV_ID;
		size = sizeof(bf_2013_init_talbe) / 2;
		camera_inf_write_cfg_byte(bf_2013_init_talbe, size);
		CAMERA_LOGI("BF_2013 init finish\r\n");
		break;
	case GC0308C_DEV:
		s_camera_dev_id = GC0308C_DEV_ID;
		size = sizeof(gc0308c_init_talbe) / 2;
		camera_inf_write_cfg_byte(gc0308c_init_talbe, size);
		CAMERA_LOGI("GC0308C init finish\r\n");
		break;
	case HM_1055_DEV:
		s_camera_dev_id = HM_1055_DEV_ID;
		size = sizeof(hm_1055_init_talbe) / 4;
		camera_inf_write_cfg_word(hm_1055_init_talbe, size);
		bk_jpeg_enc_dvp_gpio_enable();
		camera_inf_cfg_hm1055_fps(CMPARAM_GET_FPS(ejpeg_cfg.sener_cfg));
		CAMERA_LOGI("HM_1055 init finish\r\n");
		break;
	default:
		CAMERA_LOGE("NOT Find this sensor\r\n");
	}
}

#if 0
void camera_flip(uint8_t n)
{
	uint8_t addr, data, dt0, dt1;
	if (s_camera_dev_id == GC0328C_DEV_ID) {
		addr = 0x17;
		dt0 = 0x14;
		dt1 = 0x17;
	} else {
		addr = 0x17;
		dt0 = 0x14;
		dt1 = 0x17;
	}
	if (n) {
		data = dt1;     //flip 180
	} else {
		data = dt0;     //normal
	}
	camera_intf_sccb_write(addr, data);
}
#endif

bk_err_t bk_camera_init(void *data)
{
	int err = kNoErr;
	i2c_config_t i2c_config = {0};
	jpeg_config_t jpeg_config = {0};
	uint8_t ppi = 0;

#if CONFIG_SOC_BK7256XX
	bk_jpeg_enc_driver_init();
#endif
	camera_intf_config_ejpeg(data);

	ppi = CMPARAM_GET_PPI(ejpeg_cfg.sener_cfg);
	jpeg_config.rx_buf = ejpeg_cfg.rxbuf;
	jpeg_config.rx_buf_len = ejpeg_cfg.rxbuf_len;
	jpeg_config.node_len = ejpeg_cfg.node_len;
	jpeg_config.x_pixel = ejpeg_cfg.x_pixel;
	jpeg_config.y_pixel = ejpeg_cfg.y_pixel;
	if (ppi >= VGA_1280_720) {
		jpeg_config.sys_clk_div = 3; // sys jpeg div (3+1)
		jpeg_config.mclk_div = 2;// jpeg 2 div
	} else {
		jpeg_config.sys_clk_div = 4; // sys jpeg div (4+1)
		jpeg_config.mclk_div = 0; // jpeg 4 div
	}

	jpeg_config.dma_rx_finish_handler = ejpeg_cfg.dma_rx_handler;
	jpeg_config.dma_channel = ejpeg_cfg.dma_channel;

	bk_jpeg_enc_init(&jpeg_config);
	bk_jpeg_enc_register_isr(END_OF_FRAME, camera_intf_ejpeg_end_handler, NULL);
#if USE_JTAG_FOR_DEBUG
	//set i2c2 mode master/slave
	uint32_t i2c2_trans_mode = (0 & (~I2C2_MSG_WORK_MODE_MS_BIT)// master
							  & (~I2C2_MSG_WORK_MODE_AL_BIT))// 7bit address
							 | (I2C2_MSG_WORK_MODE_IA_BIT); // with inner address
	i2c_hdl = ddev_open(DD_DEV_TYPE_I2C2, &status, i2c2_trans_mode);
	bk_printf("open I2C2\r\n");
	{
		extern void uart_hw_uninit(uint8_t uport);
		// disable uart temporarily
		uart_hw_uninit(1);
	}
#else
	i2c_config.baud_rate = EJPEG_I2C_DEFAULT_BAUD_RATE;
	i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
	bk_i2c_init(CONFIG_CAMERA_I2C_ID, &i2c_config);
#endif


#if CONFIG_VIDEO_LCD
	if(tvideo_lcd_enable) {
		bk_printf("open LCD Display\r\n");
		bk_video_lcd_init(ejpeg_cfg.x_pixel);
	}
#endif

	bk_camera_sensor_config();
	return err;
}

bk_err_t bk_camera_deinit(void)
{
	bk_jpeg_enc_deinit();

	bk_dma_deinit(ejpeg_cfg.dma_channel);
	bk_dma_free(DMA_DEV_JPEG, ejpeg_cfg.dma_channel);
	
#if CONFIG_VIDEO_LCD
	if(tvideo_lcd_enable) {
		while(lcd_status != READY);
		bk_dma_deinit(ejpeg_cfg.dma_lcd_channel);
		if (bk_dma_free(DMA_DEV_LCD_DATA, ejpeg_cfg.dma_lcd_channel ) != BK_OK) {
			os_printf("free lcd dma: %d error\r\n", ejpeg_cfg.dma_lcd_channel );
		}
		bk_jpeg_dec_driver_deinit();
		bk_lcd_rgb_deinit();
	}
#endif
	bk_i2c_deinit(CONFIG_CAMERA_I2C_ID);
	os_memset(&ejpeg_cfg, 0, sizeof(jpegenc_desc_t));
	if (bk_jpeg_enc_dvp_deinit() != BK_OK) {
		os_printf("deinit jpeg enc error\r\n");
		}
#if CONFIG_SOC_BK7256XX
	bk_jpeg_enc_driver_deinit();
#endif
	CAMERA_LOGI("camera deinit finish\r\n");
	return kNoErr;
}

#if CONFIG_VIDEO_LCD
static void dma_video_lcd_config(uint32_t dma_ch, uint32_t dma_src_mem_addr)
{
	dma_config_t dma_config = {0};
	
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) dma_src_mem_addr;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.start_addr = (uint32) REG_DISP_RGB_FIFO;

	bk_dma_init(dma_ch, &dma_config);
	bk_dma_register_isr(dma_ch, NULL, dma_rgb_finish_isr);
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_ch, 65280));
	bk_dma_enable_finish_interrupt(dma_ch);
}


bk_err_t bk_video_lcd_init(uint16_t x_pixel)
{
	int err = kNoErr;
	
	if(x_pixel == 0x50) {
		ejpeg_cfg.jpeg_dec_pixel_x = JPEGDEC_X_PIXEL_640;
	} else if (x_pixel == 0x3c) {
		ejpeg_cfg.jpeg_dec_pixel_x = JPEGDEC_X_PIXEL_480;
	}

	os_printf("ejpeg_cfg.jpeg_dec_pixel_x = %x \r\n", ejpeg_cfg.jpeg_dec_pixel_x);
	bk_gpio_enable_output(GPIO_2);	//output
	bk_gpio_enable_output(GPIO_3);	//output
	bk_gpio_enable_output(GPIO_4);	//output
	os_printf("psram init. \r\n");
	bk_psram_init(0x00054043);


	ejpeg_cfg.jpeg_dec_src_addr = (uint8_t *)0x60000000;
	os_printf("jpeg_dec_src_addr = %x \r\n", ejpeg_cfg.jpeg_dec_src_addr);
	video_jpeg_buff = ejpeg_cfg.jpeg_dec_src_addr;

	ejpeg_cfg.jpeg_dec_dst_addr = (uint8_t *)0x60200000;
	os_printf("jpeg_dec_dst_addr= %x \r\n", ejpeg_cfg.jpeg_dec_dst_addr);

	ejpeg_cfg.lcd_display_addr = (uint8_t *)0x60500000;
	os_printf("lcd_display_addr = %x \r\n", ejpeg_cfg.lcd_display_addr);

	bk_lcd_driver_init(LCD_96M);
	bk_lcd_rgb_init(13,	X_PIXEL_RGB, Y_PIXEL_RGB, VUYY_DATA);
	bk_lcd_rgb_display_en(1);

	ejpeg_cfg.dma_lcd_channel = bk_dma_alloc(DMA_DEV_LCD_DATA);
	if ((ejpeg_cfg.dma_lcd_channel < DMA_ID_0) || (ejpeg_cfg.dma_lcd_channel >= DMA_ID_MAX)) {
		os_printf("malloc lcd dma fail \r\n");
		return -1;
	}
	os_printf("malloc lcd display dma = ch%d \r\n", ejpeg_cfg.dma_lcd_channel);

	dma_video_lcd_config(ejpeg_cfg.dma_lcd_channel, (uint32_t)ejpeg_cfg.lcd_display_addr);
	err=bk_jpeg_dec_driver_init();
	if (err != BK_OK)
		return 0;
	os_printf("jpegdec driver init successful.\r\n");
	ejpeg_cfg.jpeg_dec_callback = jpeg_dec_callback;

	bk_jpeg_dec_complete_cb(jpeg_dec_eof_cb, ejpeg_cfg.jpeg_dec_pixel_x);

	return err;
}

void bk_lcd_video_enable(uint8_t enable)
{
#if (CONFIG_SOC_BK7256 || CONFIG_SOC_BK7237)
	tvideo_lcd_enable = enable;
#endif
}
#endif

#if 0
bk_err_t camera_intfer_set_video_param(uint32_t ppi_type, uint32_t pfs_type)
{
	if (s_camera_dev == GC0328C_DEV) {
		if (ppi_type < PPI_MAX) {
			camera_intf_init_ejpeg_pixel(ppi_type);
			bk_jpeg_set_x_pixel(ejpeg_cfg.x_pixel);
			bk_jpeg_set_y_pixel(ejpeg_cfg.y_pixel);
			camera_inf_cfg_gc0328c_ppi(ppi_type);
		} else {
			return kParamErr;
		}
		if (pfs_type < FPS_MAX) {
			camera_inf_cfg_gc0328c_fps(pfs_type);
		} else {
			return kParamErr;
		}
	}
	return kNoErr;
}

bk_err_t bk_camera_set_ppi_fps(uint32_t ppi_type, uint32_t fps_type)
{
	if (ppi_type < PPI_MAX) {
		CMPARAM_SET_PPI(ejpeg_cfg.sener_cfg, ppi_type);
	} else {
		return kParamErr;
	}

	if (fps_type < FPS_MAX) {
		CMPARAM_SET_FPS(ejpeg_cfg.sener_cfg, fps_type);
	} else {
		return kParamErr;
	}
	return kNoErr;
}
#endif

bk_err_t bk_camera_set_param(uint32_t dev, uint32_t cfg)
{
	if (dev > 7) {
		s_camera_dev = 0xABC03;
	} else {
		s_camera_dev = 0xABC00 + dev;
	}
	s_camera_sensor = cfg;
	camera_intf_set_sener_cfg(cfg);
	return kNoErr;
}

#endif // CONFIG_CAMERA
