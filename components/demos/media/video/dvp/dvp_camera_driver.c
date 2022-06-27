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
#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/psram.h>
#include <os/mem.h>
#include <driver/timer.h>
#include "bk_arm_arch.h"
#include <components/video_transfer.h>
#include <components/dvp_camera.h>
#include <components/dvp_camera_types.h>
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
#include <driver/dma2d.h>
#include <lcd_dma2d_config.h>
#include <driver/jpeg_dec_types.h>
#endif

#define EJPEG_DELAY_HTIMER_CHANNEL     5
#define EJPEG_I2C_DEFAULT_BAUD_RATE    I2C_BAUD_RATE_100KHZ
#define EJPEG_DELAY_HTIMER_VAL         (2)  // 2ms

static uint32_t s_camera_sensor = 0x01E00014;//(480 << 16) | 20;

static jpegenc_desc_t ejpeg_cfg;

#if CONFIG_VIDEO_LCD
extern u64 riscv_get_mtimer(void);

static uint8_t tvideo_lcd_enable = 0;
static uint8_t tvideo_lcd_blend_enable = 0;
static uint8_t  lcd_status = READY;
uint8_t * video_jpeg_buff = NULL;

static void jpeg_dec_eof_cb(void *param)
{
	//bk_gpio_set_output_high(GPIO_2);

	ejpeg_cfg.lcd_blend_frame_cnt ++;
	lcd_status = DISPLAYING;
	if (ejpeg_cfg.jpeg_dec_pixel_x == JPEGDEC_X_PIXEL_640) {
		dma2d_crop_params_t  crop_params;
		crop_params.src_addr = (uint32_t)ejpeg_cfg.jpeg_dec_dst_addr;
		crop_params.dst_addr = (uint32_t)ejpeg_cfg.lcd_display_addr;
		crop_params.x = (640 - 480)/2;
		crop_params.y = (480 - 272)/2;
 		crop_params.src_width = 640;
		crop_params.src_height = 480;
		crop_params.dst_width = 480;
		crop_params.dst_height = 272;
		dma2d_crop_image(&crop_params);
	} else if(ejpeg_cfg.jpeg_dec_pixel_x == JPEGDEC_X_PIXEL_720) {
		dma2d_crop_params_t  crop_params;
		crop_params.src_addr = (uint32_t)ejpeg_cfg.jpeg_dec_dst_addr;
		crop_params.dst_addr = (uint32_t)ejpeg_cfg.lcd_display_addr;
		crop_params.x = (1280 - 480)/2;
		crop_params.y = (720 - 272)/2;
 		crop_params.src_width = 1280;
		crop_params.src_height = 720;
		crop_params.dst_width = 480;
		crop_params.dst_height = 272;
		dma2d_crop_image(&crop_params);
	}

	if (tvideo_lcd_blend_enable) {
		u64 cur_time = riscv_get_mtimer();
		if ((u64)(cur_time/26000000) % 2 == 0) {
			yuv_blend((uint32_t)ejpeg_cfg.lcd_display_addr, (uint32_t)(LCD_BLEND_IMAGE1));
		} else {
			yuv_blend((uint32_t)ejpeg_cfg.lcd_display_addr, (uint32_t)(LCD_BLEND_IMAGE2));
		}
	}

	bk_lcd_rgb_display_en(1);
	bk_dma_start(ejpeg_cfg.dma_lcd_channel);

	//bk_gpio_set_output_low(GPIO_2);
}

static void dma_rgb_finish_isr(dma_id_t id)
{
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
			video_jpeg_buff = (uint8_t *)ejpeg_cfg.jpeg_dec_src_addr;
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

	camera_intf_set_sener_cfg_value(ejpeg_cfg.sener_cfg);
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
		bk_dma2d_driver_deinit();
		bk_video_lcd_init(ejpeg_cfg.x_pixel);
	}
#endif

	bk_camera_sensor_config();
	return err;
}

bk_err_t bk_camera_deinit(void)
{

	os_printf("bk_jpeg_enc_deinit\r\n");
	bk_jpeg_enc_deinit();
	os_printf("bk_dma_deinit\r\n");
	bk_dma_deinit(ejpeg_cfg.dma_channel);
	bk_dma_free(DMA_DEV_JPEG, ejpeg_cfg.dma_channel);
#if CONFIG_VIDEO_LCD
	if(tvideo_lcd_enable) {	

		os_printf("dma_lcd_channel free\r\n");
		bk_dma_deinit(ejpeg_cfg.dma_lcd_channel);
		if (bk_dma_free(DMA_DEV_LCD_DATA, ejpeg_cfg.dma_lcd_channel ) != BK_OK) {
			os_printf("free lcd dma: %d error\r\n", ejpeg_cfg.dma_lcd_channel );
		}
		bk_jpeg_dec_driver_deinit();
		bk_lcd_rgb_deinit();
		bk_dma2d_driver_deinit();
	}
#endif

	bk_i2c_deinit(CONFIG_CAMERA_I2C_ID);

#if CONFIG_SOC_BK7256XX
	bk_jpeg_enc_driver_deinit();
#endif


	os_memset(&ejpeg_cfg, 0, sizeof(jpegenc_desc_t));

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
	lcd_status = READY;
	if(x_pixel == 0x50) {
		ejpeg_cfg.jpeg_dec_pixel_x = JPEGDEC_X_PIXEL_640;
	} else if (x_pixel == 0x3c) {
		ejpeg_cfg.jpeg_dec_pixel_x = JPEGDEC_X_PIXEL_480;
	} else if (x_pixel == 0xA0) {
		ejpeg_cfg.jpeg_dec_pixel_x = JPEGDEC_X_PIXEL_720;
	}

	os_printf("ejpeg_cfg.jpeg_dec_pixel_x = %x \r\n", ejpeg_cfg.jpeg_dec_pixel_x);
	bk_gpio_enable_output(GPIO_2);	//output
	bk_gpio_enable_output(GPIO_3);	//output
	bk_gpio_enable_output(GPIO_4);	//output
	os_printf("psram init. \r\n");
	bk_psram_init(0x00054043);


	ejpeg_cfg.jpeg_dec_src_addr = (uint8_t *)PARAM_JPEG_SRC_ADDR_BASE;
	os_printf("jpeg_dec_src_addr = %x \r\n", ejpeg_cfg.jpeg_dec_src_addr);
	video_jpeg_buff = ejpeg_cfg.jpeg_dec_src_addr;

	ejpeg_cfg.jpeg_dec_dst_addr = (uint8_t *)PARAM_JPEG_DEC_DST_ADDR_BASE;
	os_printf("jpeg_dec_dst_addr= %x \r\n", ejpeg_cfg.jpeg_dec_dst_addr);

	ejpeg_cfg.lcd_display_addr = (uint8_t *)PARAM_LCD_DISPLAY_ADDR_BASE;
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

void  bk_lcd_video_blending(uint8_t blend_enable)
{
#if (CONFIG_SOC_BK7256 || CONFIG_SOC_BK7237)
	tvideo_lcd_blend_enable = blend_enable;
#endif
}

#endif

bk_err_t bk_camera_set_param(uint32_t dev, uint32_t cfg)
{
	camera_intf_set_device(dev);
	
	s_camera_sensor = cfg;
	camera_intf_set_sener_cfg(cfg);
	return kNoErr;
}

