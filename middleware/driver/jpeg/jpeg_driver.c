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

#include "bk_api_dma.h"
#include "bk_api_int.h"
#include "bk_api_mem.h"
#include "bk_api_jpeg.h"
#include "clock_driver.h"
#include "gpio_driver.h"
#include "jpeg_driver.h"
#include "jpeg_hal.h"
#include "power_driver.h"
#include "sys_driver.h"

extern void delay(int num);//TODO fix me

typedef struct {
	jpeg_isr_t isr_handler;
	void *param;
} jpeg_isr_handler_t;

typedef struct {
	jpeg_hal_t hal;
	jpeg_isr_handler_t frame_start_handler;
	jpeg_isr_handler_t frame_end_handler;
	jpeg_isr_handler_t end_yuv_handler;
} jpeg_driver_t;

#define JPEG_RETURN_ON_NOT_INIT() do {\
	if (!s_jpeg_driver_is_init) {\
		return BK_ERR_JPEG_NOT_INIT;\
	}\
} while(0)

#define JPEG_RX_DMA_CHANNEL    DMA_ID_4

#if (CONFIG_SYSTEM_CTRL)
#define addSYSTEM_Reg0xe                                        *((volatile unsigned long *) (0x44010000+0xe*4))
#define setf_SYSTEM_Reg0xe_jpeg_disckg                         addSYSTEM_Reg0xe |= 0x40000000
#define clrf_SYSTEM_Reg0xe_jpeg_disckg                         addSYSTEM_Reg0xe &= ~0x40000000
#define setf_SYSTEM_Reg0xe_jpeg_dec_disckg                     addSYSTEM_Reg0xe |= 0x80000000
#define clrf_SYSTEM_Reg0xe_jpeg_dec_disckg                     addSYSTEM_Reg0xe &= ~0x80000000
static bool s_jpeg_deinit = false;
#endif

static jpeg_driver_t s_jpeg = {0};
static bool s_jpeg_driver_is_init = false;
static bool s_jpeg_cli_deinit = false;

static void jpeg_isr(void);

#if (CONFIG_SYSTEM_CTRL)
static void jpeg_power_config_set(void)
{
	sys_drv_set_jpeg_clk_sel(1);
	sys_drv_set_clk_div_mode1_clkdiv_jpeg(0x4);
	setf_SYSTEM_Reg0xe_jpeg_disckg;
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_JPEG, CLK_PWR_CTRL_PWR_UP);
}
#endif

static void jpeg_init_gpio(void)
{
	jpeg_gpio_map_t jpeg_gpio_map_table[] = JPEG_GPIO_MAP;
#if (CONFIG_SYSTEM_CTRL)

	for (uint32_t i = 0; i < 2; i++) {
		gpio_dev_unmap(jpeg_gpio_map_table[i].gpio_id);
		gpio_dev_map(jpeg_gpio_map_table[i].gpio_id, jpeg_gpio_map_table[i].dev);
	}
#else
	for(uint32_t i = 0; i < JPEG_GPIO_PIN_NUMBER; i++) {
		gpio_dev_unmap(jpeg_gpio_map_table[i].gpio_id);
		gpio_dev_map(jpeg_gpio_map_table[i].gpio_id, jpeg_gpio_map_table[i].dev);
	}
#endif
}

static void jpeg_deinit_gpio(void)
{
	jpeg_gpio_map_t jpeg_gpio_map_table[] = JPEG_GPIO_MAP;

	for(uint32_t i = 0; i < JPEG_GPIO_PIN_NUMBER; i++) {
		gpio_dev_unmap(jpeg_gpio_map_table[i].gpio_id);
	}
}

static void jpeg_dma_rx_init(const jpeg_config_t *config)
{
	dma_config_t dma_config = {0};

	if (config->yuv_mode == 0) {
		dma_config.mode = DMA_WORK_MODE_REPEAT;
		dma_config.chan_prio = 0;
		dma_config.src.dev = DMA_DEV_JPEG;
		dma_config.src.width = DMA_DATA_WIDTH_32BITS;
		dma_config.src.start_addr = JPEG_R_RX_FIFO;
		dma_config.dst.dev = DMA_DEV_DTCM;
		dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		dma_config.dst.start_addr = (uint32_t)config->rx_buf;
		dma_config.dst.end_addr = (uint32_t)(config->rx_buf + config->rx_buf_len);

		BK_LOG_ON_ERR(bk_dma_init(JPEG_RX_DMA_CHANNEL, &dma_config));
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(JPEG_RX_DMA_CHANNEL, config->node_len));
		BK_LOG_ON_ERR(bk_dma_register_isr(JPEG_RX_DMA_CHANNEL, NULL, config->dma_rx_finish_handler));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(JPEG_RX_DMA_CHANNEL));
		BK_LOG_ON_ERR(bk_dma_enable_half_finish_interrupt(JPEG_RX_DMA_CHANNEL));
		BK_LOG_ON_ERR(bk_dma_start(JPEG_RX_DMA_CHANNEL));
	} else {
		/*dma_config.mode = DMA_WORK_MODE_REPEAT;
		dma_config.chan_prio = 0;
		dma_config.src.dev = DMA_DEV_JPEG;
		dma_config.src.width = DMA_DATA_WIDTH_32BITS;
		dma_config.src.start_addr = PSRAM_BASEADDR;//PSRAM_BASEADDR
		dma_config.dst.dev = DMA_DEV_DTCM;
		dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
		dma_config.dst.start_addr = (uint32_t)config->rx_buf;
		dma_config.dst.end_addr = (uint32_t)(config->rx_buf + config->rx_buf_len);

		BK_LOG_ON_ERR(bk_dma_init(JPEG_RX_DMA_CHANNEL, &dma_config));
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(JPEG_RX_DMA_CHANNEL, config->node_len));
		BK_LOG_ON_ERR(bk_dma_register_isr(JPEG_RX_DMA_CHANNEL, NULL, config->dma_rx_finish_handler));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(JPEG_RX_DMA_CHANNEL));
		BK_LOG_ON_ERR(bk_dma_enable_half_finish_interrupt(JPEG_RX_DMA_CHANNEL));
		BK_LOG_ON_ERR(bk_dma_start(JPEG_RX_DMA_CHANNEL));*/
	}
}

static void jpeg_dma_rx_deinit(void)
{
	BK_LOG_ON_ERR(bk_dma_stop(JPEG_RX_DMA_CHANNEL));
}

static void jpeg_init_common(void)
{
#if (CONFIG_SYSTEM_CTRL)
	jpeg_power_config_set();
	sys_drv_int_enable(JPEGENC_INTERRUPT_CTRL_BIT);
#else
	power_jpeg_pwr_up();
	icu_enable_jpeg_interrupt();
#endif

	jpeg_init_gpio();
}

static void jpeg_deinit_common(void)
{
	jpeg_hal_stop_common(&s_jpeg.hal);
	jpeg_hal_reset_config_to_default(&s_jpeg.hal);
#if (CONFIG_SYSTEM_CTRL)
	clrf_SYSTEM_Reg0xe_jpeg_disckg;
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_JPEG, CLK_PWR_CTRL_PWR_DOWN);
	sys_drv_int_disable(JPEGENC_INTERRUPT_CTRL_BIT);
#else
	icu_disable_jpeg_interrupt();
	power_jpeg_pwr_down();
#endif

	jpeg_dma_rx_deinit();
	jpeg_deinit_gpio();
}

static void jpeg_cli_deinit_common(void)
{
	jpeg_hal_stop_common(&s_jpeg.hal);
	jpeg_hal_reset_config_to_default(&s_jpeg.hal);
#if (CONFIG_SYSTEM_CTRL)
	clrf_SYSTEM_Reg0xe_jpeg_disckg;
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_JPEG, CLK_PWR_CTRL_PWR_DOWN);
	sys_drv_int_disable(JPEGENC_INTERRUPT_CTRL_BIT);
#endif
	jpeg_deinit_gpio();
}


bk_err_t bk_jpeg_driver_init(void)
{
	if (s_jpeg_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_jpeg, 0, sizeof(s_jpeg));
	jpeg_hal_init(&s_jpeg.hal);
	bk_int_isr_register(INT_SRC_JPEG, jpeg_isr, NULL);
	s_jpeg_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_jpeg_driver_deinit(void)
{
	if (!s_jpeg_driver_is_init) {
		return BK_OK;
	}
	bk_int_isr_unregister(INT_SRC_JPEG);
	s_jpeg_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_jpeg_init(const jpeg_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	JPEG_RETURN_ON_NOT_INIT();

	jpeg_init_common();
	jpeg_hal_configure(&s_jpeg.hal, config);
	jpeg_dma_rx_init(config);

	return BK_OK;
}

bk_err_t bk_jpeg_deinit(void)
{
#if (CONFIG_SYSTEM_CTRL)
	s_jpeg_deinit = true;
	delay(2500);
#else
	jpeg_deinit_common();
#endif

	return BK_OK;
}

bk_err_t bk_jpeg_cli_init(const jpeg_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	JPEG_RETURN_ON_NOT_INIT();

	jpeg_init_common();
	jpeg_hal_configure(&s_jpeg.hal, config);

	return BK_OK;
}

bk_err_t bk_jpeg_cli_deinit(void)
{
	s_jpeg_cli_deinit = true;
	while(s_jpeg_cli_deinit) {
		delay(200);
	}

	return BK_OK;
}

bk_err_t bk_jpeg_set_x_pixel(uint32_t x_pixel)
{
	JPEG_RETURN_ON_NOT_INIT();

	jpeg_hal_set_x_pixel(&s_jpeg.hal, x_pixel);
	return BK_OK;
}

bk_err_t bk_jpeg_set_y_pixel(uint32_t y_pixel)
{
	JPEG_RETURN_ON_NOT_INIT();

	jpeg_hal_set_y_pixel(&s_jpeg.hal, y_pixel);
	return BK_OK;
}

bk_err_t bk_jpeg_set_yuv_mode(uint32_t mode)
{
	JPEG_RETURN_ON_NOT_INIT();
#if (CONFIG_SYSTEM_CTRL)
	jpeg_hal_set_yuv_mode(&s_jpeg.hal, mode);
#endif
	return BK_OK;

}

uint32_t bk_jpeg_get_frame_byte_number(void)
{
	JPEG_RETURN_ON_NOT_INIT();

	return jpeg_hal_get_frame_byte_number(&s_jpeg.hal);
}

bk_err_t bk_jpeg_register_frame_start_isr(jpeg_isr_t isr, void *param)
{
	uint32_t int_level = rtos_disable_int();
	s_jpeg.frame_start_handler.isr_handler = isr;
	s_jpeg.frame_start_handler.param = param;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_jpeg_register_frame_end_isr(jpeg_isr_t isr, void *param)
{
	uint32_t int_level = rtos_disable_int();
	s_jpeg.frame_end_handler.isr_handler = isr;
	s_jpeg.frame_end_handler.param = param;

	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_jpeg_register_end_yuv_isr(jpeg_isr_t isr, void *param)
{
	uint32_t int_level = rtos_disable_int();
	s_jpeg.end_yuv_handler.isr_handler = isr;
	s_jpeg.end_yuv_handler.param = param;

	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_jpeg_gpio_enable_func2(void)
{
	jpeg_gpio_map_t jpeg_gpio_map_table[] = JPEG_GPIO_MAP;
	for (uint32_t i = 2; i < JPEG_GPIO_PIN_NUMBER; i++) {
		gpio_dev_unmap(jpeg_gpio_map_table[i].gpio_id);
		gpio_dev_map(jpeg_gpio_map_table[i].gpio_id, jpeg_gpio_map_table[i].dev);
	}

	return BK_OK;
}

static void jpeg_isr(void)
{
	jpeg_hal_t *hal = &s_jpeg.hal;
	uint32_t int_status = jpeg_hal_get_interrupt_status(hal);
	jpeg_hal_clear_interrupt_status(hal, int_status);

	if (jpeg_hal_is_frame_start_int_triggered(hal, int_status)) {
		if (s_jpeg.frame_start_handler.isr_handler) {
			s_jpeg.frame_start_handler.isr_handler(0, s_jpeg.frame_start_handler.param);
		}
	}

	if (jpeg_hal_is_frame_end_int_triggered(hal, int_status)) {
		if (s_jpeg.frame_end_handler.isr_handler) {
			s_jpeg.frame_end_handler.isr_handler(0, s_jpeg.frame_end_handler.param);
		}
	}

#if (CONFIG_SYSTEM_CTRL)
	if (jpeg_hal_is_yuv_end_int_triggered(hal, int_status)) {
		if (s_jpeg.end_yuv_handler.isr_handler) {
			s_jpeg.end_yuv_handler.isr_handler(0, s_jpeg.end_yuv_handler.param);
		}
	}

	if (s_jpeg_deinit) {
		jpeg_deinit_common();
		s_jpeg_deinit = false;
	}
#endif

	if (s_jpeg_cli_deinit) {
		jpeg_cli_deinit_common();
		s_jpeg_cli_deinit = false;
	}
}

