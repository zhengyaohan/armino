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

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/trng.h>
#include "trng_driver.h"
#include "trng_hal.h"
#include "bk_wifi_rw.h"

typedef struct {
	trng_hal_t hal;
	bool is_enabled;
} trng_driver_t;

#define TRNG_RETURN_ON_NOT_INIT() do {\
		if (!s_trng_driver_is_init) {\
			return BK_ERR_TRNG_DRIVER_NOT_INIT;\
		}\
	} while(0)

#define TRNG_READ_COUNT  (16)

static trng_driver_t s_trng = {0};
static bool s_trng_driver_is_init = false;

static void trng_init_common(void)
{
	trng_hal_start_common(&s_trng.hal);
	s_trng.is_enabled = true;
}

static void trng_deinit_common(void)
{
	trng_hal_stop_common(&s_trng.hal);
	s_trng.is_enabled = false;
}

static uint32_t trng_get_random_number(void)
{
	return trng_hal_get_random_number(&s_trng.hal);
}

bk_err_t bk_trng_driver_init(void)
{
	if (s_trng_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_trng, 0, sizeof(s_trng));
	trng_hal_init(&s_trng.hal);
	s_trng_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_trng_driver_deinit(void)
{
	if (!s_trng_driver_is_init) {
		return BK_OK;
	}
	trng_deinit_common();
	s_trng_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_trng_start(void)
{
	TRNG_RETURN_ON_NOT_INIT();
	trng_init_common();
	return BK_OK;
}

bk_err_t bk_trng_stop(void)
{
	TRNG_RETURN_ON_NOT_INIT();
	trng_deinit_common();
	return BK_OK;
}

uint32_t prandom_get(void)
{
	return bk_wifi_get_monotonic_counter_2_lo();
}

#if (CONFIG_TRNG_SUPPORT)
int bk_rand(void)
{
	int i = 0, number = 0;

	/*Different board , same time point, the trng generate random number maybe same*/
	for(i = 0; i < TRNG_READ_COUNT; i++) {
		trng_get_random_number();
	}
	
	number = (int)trng_get_random_number();
	return (number & RAND_MAX);
}
#else
int bk_rand(void)
{
	int number = (int)prandom_get();
	return (number & RAND_MAX);
}
#endif

