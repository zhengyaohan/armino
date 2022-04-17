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
#include <os/mem.h>
#include "icu_driver.h"
#include "timer_driver.h"
#include "timer_hal.h"
#include <driver/timer.h>
#include "clock_driver.h"
#include "power_driver.h"
#include <driver/int.h>
#include "sys_driver.h"

#if (SOC_TIMER_INTERRUPT_NUM > 1)
static void timer1_isr(void) __SECTION(".itcm");
#endif
static void timer_isr(void) __SECTION(".itcm");

typedef struct {
    timer_hal_t hal;
} timer_driver_t;

static timer_driver_t s_timer = {0};
static timer_isr_t s_timer_isr[SOC_TIMER_CHAN_NUM_PER_UNIT] = {NULL};
static bool s_timer_driver_is_init = false;

#define TIMER_RETURN_ON_NOT_INIT() do {\
        if (!s_timer_driver_is_init) {\
            return BK_ERR_TIMER_NOT_INIT;\
        }\
    } while(0)

#define TIMER_RETURN_ON_INVALID_ID(id) do {\
        if ((id) >= SOC_TIMER_CHAN_NUM_PER_UNIT) {\
            return BK_ERR_TIMER_ID;\
        }\
    } while(0)

#define TIMER_RETURN_ON_IS_RUNNING(id, status) do {\
        if ((status) & BIT((id))) {\
            return BK_ERR_TIMER_IS_RUNNING;\
        }\
    } while(0)

#define TIMER_RETURN_TIMER_ID_IS_ERR(id) do {\
        if ((~CONFIG_TIMER_SUPPORT_ID_BITS) & BIT((id))) {\
            return BK_ERR_TIMER_ID_ON_DEFCONFIG;\
        }\
    } while(0)

#if (CONFIG_SYSTEM_CTRL)
static void timer_clock_select(timer_id_t id, timer_src_clk_t mode)
{
	uint32_t group_index = 0;

	group_index = id / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			sys_drv_timer_select_clock(SYS_SEL_TIMER0, mode);
			break;
		case 1:
			sys_drv_timer_select_clock(SYS_SEL_TIMER1, mode);
			break;
		default:
			break;
	}
}

static void timer_clock_enable(timer_id_t id)
{
	uint32_t group_index = 0;

	group_index = id / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_TIMER_1, CLK_PWR_CTRL_PWR_UP);
			break;
		case 1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_TIMER_2, CLK_PWR_CTRL_PWR_UP);
			break;
		default:
			break;
	}
}

static void timer_clock_disable(timer_id_t id)
{
	uint32_t group_index = 0;

	group_index = id / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_TIMER_1, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case 1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_TIMER_2, CLK_PWR_CTRL_PWR_DOWN);
			break;
		default:
			break;
	}
}

static void timer_interrupt_enable(timer_id_t id)
{
	uint32_t group_index = 0;

	group_index = id / SOC_TIMER_CHAN_NUM_PER_GROUP;
	switch(group_index)
	{
		case 0:
			sys_drv_int_enable(TIMER_INTERRUPT_CTRL_BIT);
			break;

		case 1:
			sys_drv_int_enable(TIMER1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
	}
}
#endif

static void timer_chan_init_common(timer_id_t timer_id)
{
#if (CONFIG_SYSTEM_CTRL)
	timer_clock_select(timer_id, TIMER_SCLK_XTAL);
	timer_clock_enable(timer_id);
#else
	power_pwr_up_timer(timer_id);
#endif
}

static void timer_chan_deinit_common(timer_id_t timer_id)
{
    timer_hal_stop_common(&s_timer.hal, timer_id);
    timer_hal_reset_config_to_default(&s_timer.hal, timer_id);
#if (CONFIG_SYSTEM_CTRL)
	timer_clock_disable(timer_id);
#else
	power_pwr_down_timer(timer_id);
#endif
}

static void timer_chan_enable_interrupt_common(timer_id_t timer_id)
{
#if (CONFIG_SYSTEM_CTRL)
	timer_interrupt_enable(timer_id);
#else
    icu_enable_timer_interrupt();
#endif
    timer_hal_enable_interrupt(&s_timer.hal, timer_id);
}

static void timer_isr(void) __SECTION(".itcm");

bk_err_t bk_timer_driver_init(void)
{
    if (s_timer_driver_is_init) {
        return BK_OK;
    }

    os_memset(&s_timer, 0, sizeof(s_timer));
    os_memset(&s_timer_isr, 0, sizeof(s_timer_isr));
    bk_int_isr_register(INT_SRC_TIMER, timer_isr, NULL);
#if (SOC_TIMER_INTERRUPT_NUM > 1)
    bk_int_isr_register(INT_SRC_TIMER1, timer1_isr, NULL);
#endif
#if CONFIG_TIMER_INIT_EN
    timer_hal_init(&s_timer.hal);
#endif

    s_timer_driver_is_init = true;

    return BK_OK;
}

bk_err_t bk_timer_driver_deinit(void)
{
    if (!s_timer_driver_is_init) {
        return BK_OK;
    }

    for (int chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_UNIT; chan++) {
        timer_chan_deinit_common(chan);
    }

    s_timer_driver_is_init = false;

    return BK_OK;
}

extern void delay(int num);//TODO fix me

bk_err_t bk_timer_start_without_callback(timer_id_t timer_id, uint32_t time_ms)
{
    uint32_t en_status = 0;

    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);

    timer_chan_init_common(timer_id);
    timer_chan_enable_interrupt_common(timer_id);

    en_status = timer_hal_get_enable_status(&s_timer.hal);
    if (en_status & BIT(timer_id)) {
        TIMER_LOGD("timer(%d) is running, stop it\r\n", timer_id);
        timer_hal_disable(&s_timer.hal, timer_id);
        /* Delay to fix the bug that timer counter becomes bigger than
         * timer period. Once timer counter becomes bigger than timer period,
         * the timer will never timeout, or takes very very long time to
         * timeout.
         *
         * This issue is firstly observed in HOS tick timer. HOS restarts
         * the tick timer with different time_ms(timer period) again and again,
         * without the delay, the tick timer counter becomes bigger than timer
         * period very soon, then the tick interrupt will never be triggered.
         * */
        delay(4);
    }

    timer_hal_init_timer(&s_timer.hal, timer_id, time_ms);
    timer_hal_start_common(&s_timer.hal, timer_id);

    return BK_OK;
}

bk_err_t bk_timer_start(timer_id_t timer_id, uint32_t time_ms, timer_isr_t callback)
{
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    BK_LOG_ON_ERR(bk_timer_start_without_callback(timer_id, time_ms));
    s_timer_isr[timer_id] = callback;

    return BK_OK;
}

bk_err_t bk_timer_stop(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    TIMER_RETURN_ON_INVALID_ID(timer_id);

    timer_hal_stop_common(&s_timer.hal, timer_id);

    return BK_OK;
}

uint32_t bk_timer_get_cnt(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    TIMER_RETURN_ON_INVALID_ID(timer_id);

    return timer_hal_get_count(&s_timer.hal, timer_id);
}

bk_err_t bk_timer_enable(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    timer_hal_enable(&s_timer.hal, timer_id);
    return BK_OK;
}

bk_err_t bk_timer_disable(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    timer_hal_disable(&s_timer.hal, timer_id);
    return BK_OK;
}

uint32_t bk_timer_get_period(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_TIMER_ID_IS_ERR(timer_id);
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    return timer_hal_get_end_count(&s_timer.hal, timer_id);
}

uint32_t bk_timer_get_enable_status(void)
{
    TIMER_RETURN_ON_NOT_INIT();
    return timer_hal_get_enable_status(&s_timer.hal);
}

bool bk_timer_is_interrupt_triggered(timer_id_t timer_id)
{
    TIMER_RETURN_ON_NOT_INIT();
    TIMER_RETURN_ON_INVALID_ID(timer_id);
    uint32_t int_status = timer_hal_get_interrupt_status(&s_timer.hal);
    return timer_hal_is_interrupt_triggered(&s_timer.hal, timer_id, int_status);
}

static void timer_isr(void)
{
    timer_hal_t *hal = &s_timer.hal;
    uint32_t int_status = 0;

    int_status = timer_hal_get_interrupt_status(hal);
    timer_hal_clear_interrupt_status(hal, int_status);
#if (SOC_TIMER_INTERRUPT_NUM > 1)
     for(int chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_GROUP; chan++) {
#else
    for(int chan = 0; chan < SOC_TIMER_CHAN_NUM_PER_UNIT; chan++) {
#endif
        if(timer_hal_is_interrupt_triggered(hal, chan, int_status)) {
            if(s_timer_isr[chan]) {
                s_timer_isr[chan](chan);
            }
        }
    }
}

#if (SOC_TIMER_INTERRUPT_NUM > 1)
static void timer1_isr(void)
{
    timer_hal_t *hal = &s_timer.hal;
    uint32_t int_status = 0;

    int_status = timer_hal_get_interrupt_status(hal);
    timer_hal_clear_interrupt_status(hal, int_status);
    for(int chan = SOC_TIMER_CHAN_NUM_PER_GROUP; chan < SOC_TIMER_CHAN_NUM_PER_UNIT; chan++) {
        if(timer_hal_is_interrupt_triggered(hal, chan, int_status)) {
            if(s_timer_isr[chan]) {
                s_timer_isr[chan](chan);
            }
        }
    }
}
#endif
