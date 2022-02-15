#include "sys_rtos.h"
#include "bk_fake_clock.h"
#include "bk_pwm.h"
#include "bk_timer.h"
#include "bk_icu.h"
#include "bk_drv_model.h"
#include "bk_uart.h"
#include "bk_api_rtos.h"
#include "bk_api_clock.h"

#include "bk_log.h"
#include "bk_ps_time.h"

#include "bk_wdt.h"
#include "bk_api_timer.h"

#define TAG "clk"

#define FAKE_CLOCK_TICK_RATE    500

static volatile UINT64 current_clock = 0;
static volatile UINT32 current_seconds = 0;
static UINT32 second_countdown = FAKE_CLOCK_TICK_RATE;

void bk_tick_handle(uint8_t arg)
{
	current_clock ++;
	if (--second_countdown == 0) {
		current_seconds ++;
		second_countdown = FAKE_CLOCK_TICK_RATE;
	}
}

uint32_t bk_update_tick(uint32_t tick)
{
	current_clock += tick;

	while (tick >= FAKE_CLOCK_TICK_RATE) {
		current_seconds ++;
		tick -= FAKE_CLOCK_TICK_RATE;
	}

	if (second_countdown <= tick) {
		current_seconds ++;
		second_countdown = FAKE_CLOCK_TICK_RATE - (tick - second_countdown);
	} else
		second_countdown -= tick;

	return 0;
}

uint32_t bk_get_tick(void)
{
	return (UINT32)current_clock;
}

UINT64 fclk_get_local_tick64(void)
{
	return current_clock;
}

uint32_t bk_get_tick_per_second(void)
{
	return FAKE_CLOCK_TICK_RATE;
}

uint32_t bk_get_second(void)
{
	return bk_get_tick() / bk_get_tick_per_second();
}
