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
#include "bk_api_gpio.h"
#include "bk_api_int.h"
#include "bk_api_spi.h"
#include "clock_driver.h"
#include "dma_hal.h"
#include "gpio_driver.h"
#include "icu_driver.h"
#include "include.h"
#include "mem_pub.h"
#include "power_driver.h"
#include "rtos_pub.h"
#include "spi_driver.h"
#include "spi_hal.h"
#include "spi_statis.h"
#include "spi_config.h"
#include "sys_driver.h"

typedef struct {
	spi_hal_t hal;
	uint8_t id_init_bits;
	uint8_t * tx_buf;
	uint32_t tx_size;
	uint8_t * rx_buf;
	uint32_t rx_size;
	volatile uint32_t rx_offset;
	bool is_tx_blocked;
	bool is_sw_tx_finished;
	bool is_rx_blocked;
	beken_semaphore_t tx_sema;
	beken_semaphore_t rx_sema;
} spi_driver_t;

typedef struct {
	spi_isr_t callback;
	void *param;
} spi_callback_t;

#define SPI_RX_DMA_CHANNEL    DMA_ID_1
#define SPI_TX_DMA_CHANNEL    DMA_ID_3

#define SPI_RETURN_ON_NOT_INIT() do {\
	if (!s_spi_driver_is_init) {\
		SPI_LOGE("SPI driver not init\r\n");\
		return BK_ERR_SPI_NOT_INIT;\
	}\
} while(0)

#define SPI_RETURN_ON_ID_NOT_INIT(id) do {\
	if (!s_spi[id].id_init_bits) {\
		SPI_LOGE("SPI(%d) not init\r\n", id);\
		return BK_ERR_SPI_ID_NOT_INIT;\
	}\
} while(0)

#define SPI_RETURN_ON_INVALID_ID(id) do {\
	if ((id) >= SOC_SPI_UNIT_NUM) {\
		SPI_LOGE("SPI id number(%d) is invalid\r\n", (id));\
		return BK_ERR_SPI_INVALID_ID;\
	}\
} while(0)

#define SPI_SET_PIN(id) do {\
	gpio_dev_unmap(SPI##id##_LL_CSN_PIN);\
	gpio_dev_unmap(SPI##id##_LL_SCK_PIN);\
	gpio_dev_unmap(SPI##id##_LL_MOSI_PIN);\
	gpio_dev_unmap(SPI##id##_LL_MISO_PIN);\
	gpio_dev_map(SPI##id##_LL_CSN_PIN, GPIO_DEV_SPI##id##_CSN);\
	gpio_dev_map(SPI##id##_LL_SCK_PIN, GPIO_DEV_SPI##id##_SCK);\
	gpio_dev_map(SPI##id##_LL_MOSI_PIN, GPIO_DEV_SPI##id##_MOSI);\
	gpio_dev_map(SPI##id##_LL_MISO_PIN, GPIO_DEV_SPI##id##_MISO);\
	bk_gpio_pull_up(SPI##id##_LL_CSN_PIN);\
	bk_gpio_pull_up(SPI##id##_LL_SCK_PIN);\
} while(0)

static spi_driver_t s_spi[SOC_SPI_UNIT_NUM] = {0};
static bool s_spi_driver_is_init = false;
static volatile spi_id_t s_current_spi_dma_wr_id;
static volatile spi_id_t s_current_spi_dma_rd_id;
static spi_callback_t s_spi_rx_isr[SOC_SPI_UNIT_NUM] = {NULL};
static spi_callback_t s_spi_tx_finish_isr[SOC_SPI_UNIT_NUM] = {NULL};

static void spi_isr(void);
#if (SOC_SPI_UNIT_NUM > 1)
static void spi2_isr(void);
#endif
#if (SOC_SPI_UNIT_NUM > 2)
static void spi3_isr(void);
#endif

static void spi_init_gpio(spi_id_t id)
{
	switch (id) {
	case SPI_ID_0:
		SPI_SET_PIN(0);
		break;
#if (SOC_SPI_UNIT_NUM > 1)
	case SPI_ID_1:
		SPI_SET_PIN(1);
		break;
#endif
#if (SOC_SPI_UNIT_NUM > 2)
	case SPI_ID_2:
		SPI_SET_PIN(2);
		break;
#endif
	default:
		break;
	}
#if (!CONFIG_SYSTEM_CTRL)
	gpio_spi_sel(GPIO_SPI_MAP_MODE1);
#endif
}

#if (CONFIG_SYSTEM_CTRL)
static void spi_clock_enable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_1, CLK_PWR_CTRL_PWR_UP);
			break;
		case SPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_2, CLK_PWR_CTRL_PWR_UP);
			break;
		default:
			break;
	}
}

static void spi_clock_disable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_1, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case SPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_SPI_2, CLK_PWR_CTRL_PWR_DOWN);
			break;
		default:
			break;
	}
}

static void spi_interrupt_enable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_int_enable(SPI_INTERRUPT_CTRL_BIT);
			break;
		case SPI_ID_1:
			sys_drv_int_enable(SPI1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
	}
}

static void spi_interrupt_disable(spi_id_t id)
{
	switch(id)
	{
		case SPI_ID_0:
			sys_drv_int_disable(SPI_INTERRUPT_CTRL_BIT);
			break;
		case SPI_ID_1:
			sys_drv_int_disable(SPI1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
	}
}
#endif

/* 1. power up spi
 * 2. set clk
 * 3. set gpio as spi
 * 4. icu enable interrupt
 */
static bk_err_t spi_id_init_common(spi_id_t id)
{
	int ret = 0;

#if (CONFIG_SYSTEM_CTRL)
	spi_clock_enable(id);
	spi_interrupt_enable(id);
#else
	power_up_spi(id);
	clk_set_spi_clk_26m(id);
	icu_enable_spi_interrupt(id);
#endif
	spi_init_gpio(id);

	if (s_spi[id].tx_sema == NULL) {
		ret = rtos_init_semaphore(&(s_spi[id].tx_sema), 1);
		BK_ASSERT(kNoErr == ret);
	}
	if (s_spi[id].rx_sema == NULL) {
		ret = rtos_init_semaphore(&(s_spi[id].rx_sema), 1);
		BK_ASSERT(kNoErr == ret);
	}
	s_spi[id].is_tx_blocked = false;
	s_spi[id].is_sw_tx_finished = true;
	s_spi[id].is_rx_blocked = false;
	s_spi[id].id_init_bits |= BIT(id);

	return ret;
}

static void spi_id_deinit_common(spi_id_t id)
{
	spi_hal_stop_common(&s_spi[id].hal);

#if (CONFIG_SYSTEM_CTRL)
	spi_clock_disable(id);
	spi_interrupt_disable(id);
#else
	icu_disable_spi_interrupt(id);
	power_down_spi(id);
#endif
	rtos_deinit_semaphore(&(s_spi[id].tx_sema));
	rtos_deinit_semaphore(&(s_spi[id].rx_sema));
	s_spi[id].id_init_bits &= ~BIT(id);
}

static void spi_id_write_bytes_common(spi_id_t id)
{
	for (int i = 0; i < s_spi[id].tx_size; i++) {
		BK_WHILE (!spi_hal_is_tx_fifo_wr_ready(&s_spi[id].hal));
		spi_hal_write_byte(&s_spi[id].hal, s_spi[id].tx_buf[i]);
	}
}

static uint32_t spi_id_read_bytes_common(spi_id_t id)
{
	uint8_t data = 0;
	uint32_t offset = s_spi[id].rx_offset;

	while (spi_hal_read_byte(&s_spi[id].hal, &data) == BK_OK) {
		if ((s_spi[id].rx_buf) && (offset < s_spi[id].rx_size)) {
			s_spi[id].rx_buf[offset++] = data;
		}
	}
	s_spi[id].rx_offset = offset;
	SPI_LOGD("spi offset:%d\r\n", s_spi[id].rx_offset);
	return offset;
}

#if CONFIG_SPI_DMA

static void spi_dma_tx_finish_handler(dma_id_t id)
{
	SPI_LOGI("[%s] spi_id:%d\r\n", __func__, s_current_spi_dma_wr_id);
}

static void spi_dma_rx_finish_handler(dma_id_t id)
{
	SPI_LOGI("[%s] spi_id:%d\r\n", __func__, s_current_spi_dma_rd_id);
	if (s_spi[s_current_spi_dma_rd_id].is_rx_blocked) {
		rtos_set_semaphore(&s_spi[s_current_spi_dma_rd_id].rx_sema);
		s_spi[s_current_spi_dma_rd_id].is_rx_blocked = false;
	}
}

static void spi_dma_tx_init(spi_id_t id)
{
	dma_config_t dma_config;
	spi_int_config_t int_cfg_table[] = SPI_INT_CONFIG_TABLE;

	os_memset(&dma_config, 0, sizeof(dma_config));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = DMA_DATA_WIDTH_8BITS;
	dma_config.dst.start_addr = SPI_R_DATA(id);
	dma_config.dst.dev = int_cfg_table[id].dma_dev;

	BK_LOG_ON_ERR(bk_dma_init(SPI_TX_DMA_CHANNEL, &dma_config));
	BK_LOG_ON_ERR(bk_dma_register_isr(SPI_TX_DMA_CHANNEL, NULL, spi_dma_tx_finish_handler));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(SPI_TX_DMA_CHANNEL));
	BK_LOG_ON_ERR(bk_dma_enable_half_finish_interrupt(SPI_TX_DMA_CHANNEL));
}

static void spi_dma_rx_init(spi_id_t id)
{
	dma_config_t dma_config;
	spi_int_config_t int_cfg_table[] = SPI_INT_CONFIG_TABLE;

	os_memset(&dma_config, 0, sizeof(dma_config));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = int_cfg_table[id].dma_dev;
	dma_config.src.width = DMA_DATA_WIDTH_8BITS;
	dma_config.src.start_addr = SPI_R_DATA(id);
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	BK_LOG_ON_ERR(bk_dma_init(SPI_RX_DMA_CHANNEL, &dma_config));
	BK_LOG_ON_ERR(bk_dma_register_isr(SPI_RX_DMA_CHANNEL, NULL, spi_dma_rx_finish_handler));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(SPI_RX_DMA_CHANNEL));
	BK_LOG_ON_ERR(bk_dma_enable_half_finish_interrupt(SPI_RX_DMA_CHANNEL));
}

#endif /* CONFIG_SPI_DMA */

bk_err_t bk_spi_driver_init(void)
{
	if (s_spi_driver_is_init) {
		return BK_OK;
	}

	spi_int_config_t int_config_table[] = SPI_INT_CONFIG_TABLE;

	os_memset(&s_spi, 0, sizeof(s_spi));
	os_memset(&s_spi_rx_isr, 0, sizeof(s_spi_rx_isr));
	os_memset(&s_spi_tx_finish_isr, 0, sizeof(s_spi_tx_finish_isr));

	for (int id = SPI_ID_0; id < SPI_ID_MAX; id++) {
		spi_int_config_t *cur_int_cfg = &int_config_table[id];
		bk_int_isr_register(cur_int_cfg->int_src, cur_int_cfg->isr, NULL);
		s_spi[id].hal.id = id;
		spi_hal_init(&s_spi[id].hal);
	}
	spi_statis_init();
	s_spi_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_spi_driver_deinit(void)
{
	if (!s_spi_driver_is_init) {
		return BK_OK;
	}
	spi_int_config_t int_cfg_table[] = SPI_INT_CONFIG_TABLE;
	for (int id = SPI_ID_0; id < SPI_ID_MAX; id++) {
		spi_id_deinit_common(id);
		bk_int_isr_unregister(int_cfg_table[id].int_src);
	}
	s_spi_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_spi_init(spi_id_t id, const spi_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);

	spi_id_init_common(id);
	spi_hal_configure(&s_spi[id].hal, config);
	spi_hal_start_common(&s_spi[id].hal);
#if (CONFIG_SPI_DMA)
	if (config->dma_mode) {
#if (!CONFIG_SYSTEM_CTRL)
		gpio_spi_sel(GPIO_SPI_MAP_MODE0);
#endif
		spi_dma_tx_init(id);
		spi_dma_rx_init(id);
	}
#endif

	return BK_OK;
}

bk_err_t bk_spi_deinit(spi_id_t id)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	spi_id_deinit_common(id);
	return BK_OK;
}

bk_err_t bk_spi_set_mode(spi_id_t id, spi_mode_t mode)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);

	spi_hal_t *hal = &s_spi[id].hal;
	switch (mode) {
	case SPI_POL_MODE_0:
		spi_hal_set_cpol(hal, SPI_POLARITY_LOW);
		spi_hal_set_cpha(hal, SPI_PHASE_1ST_EDGE);
		break;
	case SPI_POL_MODE_1:
		spi_hal_set_cpol(hal, SPI_POLARITY_LOW);
		spi_hal_set_cpha(hal, SPI_PHASE_2ND_EDGE);
		break;
	case SPI_POL_MODE_2:
		spi_hal_set_cpol(hal, SPI_POLARITY_HIGH);
		spi_hal_set_cpha(hal, SPI_PHASE_1ST_EDGE);
		break;
	case SPI_POL_MODE_3:
	default:
		spi_hal_set_cpol(hal, SPI_POLARITY_HIGH);
		spi_hal_set_cpha(hal, SPI_PHASE_2ND_EDGE);
		break;
	}
	return BK_OK;
}

bk_err_t bk_spi_set_bit_width(spi_id_t id, spi_bit_width_t bit_width)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	spi_hal_set_bit_width(&s_spi[id].hal, bit_width);
	return BK_OK;
}

bk_err_t bk_spi_set_wire_mode(spi_id_t id, spi_wire_mode_t wire_mode)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	spi_hal_set_wire_mode(&s_spi[id].hal, wire_mode);
	return BK_OK;
}

bk_err_t bk_spi_set_baud_rate(spi_id_t id, uint32_t baud_rate)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	spi_hal_set_baud_rate(&s_spi[id].hal, baud_rate);
	return BK_OK;
}

bk_err_t bk_spi_set_bit_order(spi_id_t id, spi_bit_order_t bit_order)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	spi_hal_set_first_bit(&s_spi[id].hal, bit_order);
	return BK_OK;
}

bk_err_t bk_spi_register_rx_isr(spi_id_t id, spi_isr_t isr, void *param)
{
	SPI_RETURN_ON_INVALID_ID(id);
	uint32_t int_level = rtos_disable_int();
	s_spi_rx_isr[id].callback = isr;
	s_spi_rx_isr[id].param = param;
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_spi_register_tx_finish_isr(spi_id_t id, spi_isr_t isr, void *param)
{
	SPI_RETURN_ON_INVALID_ID(id);
	uint32_t int_level = rtos_disable_int();
	s_spi_tx_finish_isr[id].callback = isr;
	s_spi_tx_finish_isr[id].param = param;
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_spi_write_bytes(spi_id_t id, const void *data, uint32_t size)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	uint32_t int_level = rtos_disable_int();
	s_spi[id].tx_buf = (uint8_t *)data;
	s_spi[id].tx_size = size;
	s_spi[id].is_sw_tx_finished = false;
	s_spi[id].is_tx_blocked = true;
	s_spi[id].rx_size = size;
	s_spi[id].rx_offset = 0;
	spi_hal_clear_tx_fifo(&s_spi[id].hal);
	spi_hal_set_tx_trans_len(&s_spi[id].hal, size);
#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	spi_hal_disable_tx_fifo_int(&s_spi[id].hal);
#else
	spi_hal_enable_tx_fifo_int(&s_spi[id].hal);
#endif
	spi_hal_enable_tx(&s_spi[id].hal);
	rtos_enable_int(int_level);

#if CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	s_spi[id].is_sw_tx_finished = true;
	spi_id_write_bytes_common(id); /* to solve slave write */
#endif

	rtos_get_semaphore(&s_spi[id].tx_sema, BEKEN_NEVER_TIMEOUT);

	int_level = rtos_disable_int();
	spi_hal_disable_tx_fifo_int(&s_spi[id].hal);
	spi_hal_disable_tx(&s_spi[id].hal);
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_spi_read_bytes(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_ON_NOT_INIT();
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	uint32_t int_level = rtos_disable_int();
	s_spi[id].rx_size = size;
	s_spi[id].rx_buf = (uint8_t *)data;
	s_spi[id].rx_offset = 0;
	s_spi[id].is_rx_blocked = true;
	spi_hal_set_rx_trans_len(&s_spi[id].hal, size);
	spi_hal_clear_rx_fifo(&s_spi[id].hal);
	spi_hal_enable_rx_fifo_int(&s_spi[id].hal);
	spi_hal_enable_rx_finish_int(&s_spi[id].hal);
	spi_hal_enable_rx(&s_spi[id].hal);
#if !CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
	/* special for bk7251, bk7251 need enable tx fifo int, otherwize spi_isr will not triggered */
	spi_hal_enable_tx_fifo_int(&s_spi[id].hal);
#endif
	rtos_enable_int(int_level);

	rtos_get_semaphore(&(s_spi[id].rx_sema), BEKEN_NEVER_TIMEOUT);

	int_level = rtos_disable_int();
	spi_hal_disable_rx(&s_spi[id].hal);
	spi_hal_disable_tx_fifo_int(&s_spi[id].hal);
	spi_hal_disable_rx_fifo_int(&s_spi[id].hal);
	s_spi[id].rx_size = 0;
	s_spi[id].rx_offset = 0;
	s_spi[id].rx_buf = NULL;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_spi_transmit(spi_id_t id, const void *tx_data, uint32_t tx_size, void *rx_data, uint32_t rx_size)
{
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);

	if (tx_size && tx_data) {
		BK_LOG_ON_ERR(bk_spi_write_bytes(id, tx_data, tx_size));
	}

	if (rx_size && rx_data) {
		BK_LOG_ON_ERR(bk_spi_read_bytes(id, rx_data, rx_size));
	}

	return BK_OK;
}

#if CONFIG_SPI_DMA

bk_err_t bk_spi_dma_write_bytes(spi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);

	uint32_t int_level = rtos_disable_int();
	s_spi[id].is_tx_blocked = true;
	s_current_spi_dma_wr_id = id;
	spi_hal_set_tx_trans_len(&s_spi[id].hal, size);
	spi_hal_enable_tx(&s_spi[id].hal);
	rtos_enable_int(int_level);
	bk_dma_write(SPI_TX_DMA_CHANNEL, (uint8_t *)data, size);

	rtos_get_semaphore(&s_spi[id].tx_sema, BEKEN_NEVER_TIMEOUT);

	int_level = rtos_disable_int();
	spi_hal_disable_tx(&s_spi[id].hal);
	bk_dma_stop(SPI_TX_DMA_CHANNEL);
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_spi_dma_read_bytes(spi_id_t id, void *data, uint32_t size)
{
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);
	BK_RETURN_ON_NULL(data);

	uint32_t int_level = rtos_disable_int();
	s_current_spi_dma_rd_id = id;
	s_spi[id].is_rx_blocked = true;
	spi_hal_set_rx_trans_len(&s_spi[id].hal, size);
	spi_hal_clear_rx_fifo(&s_spi[id].hal);
	spi_hal_disable_rx_fifo_int(&s_spi[id].hal);
	spi_hal_enable_rx(&s_spi[id].hal);
	rtos_enable_int(int_level);
	bk_dma_read(SPI_RX_DMA_CHANNEL, (uint8_t *)data, size);

	rtos_get_semaphore(&s_spi[id].rx_sema, BEKEN_NEVER_TIMEOUT);

	int_level = rtos_disable_int();
	spi_hal_disable_rx(&s_spi[id].hal);
	bk_dma_stop(SPI_RX_DMA_CHANNEL);
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_spi_dma_transmit(spi_id_t id, const void *tx_data, uint32_t tx_size, void *rx_data, uint32_t rx_size)
{
	SPI_RETURN_ON_INVALID_ID(id);
	SPI_RETURN_ON_ID_NOT_INIT(id);

	if (tx_size && tx_data) {
		BK_LOG_ON_ERR(bk_spi_dma_write_bytes(id, tx_data, tx_size));
	}

	if (rx_size && rx_data) {
		BK_LOG_ON_ERR(bk_spi_dma_read_bytes(id, rx_data, rx_size));
	}

	return BK_OK;
}

#endif

static void spi_isr_common(spi_id_t id)
{
	spi_hal_t *hal = &s_spi[id].hal;
	uint8_t rd_data = 0;
	uint32_t int_status = 0;
	uint32_t rd_offset = s_spi[id].rx_offset;
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, id);

	int_status = spi_hal_get_interrupt_status(hal);
	spi_hal_clear_interrupt_status(hal, int_status);

	SPI_LOGD("int_status:%x\r\n", int_status);

	if (spi_hal_is_rx_fifo_int_triggered_with_status(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_fifo_isr_cnt);
		SPI_LOGD("rx fifo int triggered\r\n");
		spi_id_read_bytes_common(id);
		if (s_spi_rx_isr[id].callback) {
			s_spi_rx_isr[id].callback(id, s_spi_rx_isr[id].param);
		}
	}

	if (spi_hal_is_rx_finish_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_finish_isr_cnt);
		SPI_LOGD("rx fifo finish int triggered\r\n");
		if (s_spi[id].rx_size && s_spi[id].rx_buf) {
			while (spi_hal_read_byte(hal, &rd_data) == BK_OK) {
				if (rd_offset < s_spi[id].rx_size) {
					s_spi[id].rx_buf[rd_offset++] = rd_data;
				}
			}
			s_spi[id].rx_offset = rd_offset;
		}
		if (s_spi[id].is_rx_blocked) {
			rtos_set_semaphore(&s_spi[id].rx_sema);
			s_spi[id].is_rx_blocked = false;
		}
	}

	if (spi_hal_is_rx_overflow_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->rx_overflow_isr_cnt);
		SPI_LOGW("rx overflow int triggered\r\n");
	}

	if (spi_hal_is_tx_fifo_int_triggered_with_status(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->tx_fifo_isr_cnt);
		SPI_LOGD("tx fifo int triggered\r\n");
#if !CONFIG_SPI_SUPPORT_TX_FIFO_WR_READY
		if ((!s_spi[id].is_sw_tx_finished) &&
			s_spi[id].tx_size &&
			s_spi[id].tx_buf) {
			spi_id_write_bytes_common(id);
			s_spi[id].is_sw_tx_finished = true;
		}
		for (int i = 0; (i < s_spi[id].rx_size) && s_spi[id].rx_buf; i++) {
			/* bk7251 need spi_hal_write_byte when read byte,
			 * bk7251 master read data will not work without this operation */
			if (spi_hal_is_master(hal)) {
				spi_hal_write_byte(hal, 0xff);
			}
			if (spi_hal_read_byte(hal, &rd_data) == BK_OK) {
				SPI_LOGD("tx fifo int read byte\r\n");
				if (rd_offset < s_spi[id].rx_size) {
					s_spi[id].rx_buf[rd_offset++] = rd_data;
				}
			} else {
				break;
			}
		}
		s_spi[id].rx_offset = rd_offset;
		if (s_spi[id].is_sw_tx_finished) {
			spi_hal_disable_tx_fifo_int(hal);
		}
#endif
	}

	if (spi_hal_is_tx_finish_int_triggered(hal, int_status) &&
		s_spi[id].is_sw_tx_finished) {
		SPI_STATIS_INC(spi_statis->tx_finish_isr_cnt);
		SPI_LOGD("tx finish int triggered\r\n");
		if (spi_hal_is_master(hal)) {
			if (s_spi[id].is_tx_blocked) {
				rtos_set_semaphore(&s_spi[id].tx_sema);
				s_spi[id].is_tx_blocked = false;
			}
			if (s_spi_tx_finish_isr[id].callback) {
				s_spi_tx_finish_isr[id].callback(id, s_spi_tx_finish_isr[id].param);
			}
		} else {
			SPI_LOGW("tx finish int triggered, but current mode is spi_slave\r\n");
		}
	}

	if (spi_hal_is_tx_underflow_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->tx_underflow_isr_cnt);
		SPI_LOGW("tx underflow int triggered\r\n");
	}

	if (spi_hal_is_slave_release_int_triggered(hal, int_status)) {
		SPI_STATIS_INC(spi_statis->slave_release_isr_cnt);
		SPI_LOGD("slave cs up int triggered\r\n");
		if (spi_hal_is_slave(hal)) {
			if (s_spi[id].is_tx_blocked) {
				rtos_set_semaphore(&s_spi[id].tx_sema);
				s_spi[id].is_tx_blocked = false;
			}
			if (s_spi_tx_finish_isr[id].callback) {
				s_spi_tx_finish_isr[id].callback(id, s_spi_tx_finish_isr[id].param);
			}
		}
		if (s_spi[id].is_rx_blocked) {
			rtos_set_semaphore(&s_spi[id].rx_sema);
			s_spi[id].is_rx_blocked = false;
		}
	}
}

static void spi_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_0);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	spi_isr_common(SPI_ID_0);
}

#if (SOC_SPI_UNIT_NUM > 1)

static void spi2_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_1);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	spi_isr_common(SPI_ID_1);
}

#endif

#if (SOC_SPI_UNIT_NUM > 2)

static void spi3_isr(void)
{
	SPI_STATIS_DEC();
	SPI_STATIS_GET(spi_statis, SPI_ID_2);
	SPI_STATIS_INC(spi_statis->spi_isr_cnt);
	spi_isr_common(SPI_ID_2);
}

#endif
