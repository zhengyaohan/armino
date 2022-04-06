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
#include <components/log.h>
#include <common/bk_err.h>
#include <components/system.h>
#include <driver/wdt.h>
#include "bk_misc.h"
#include "bk_private/reset_reason.h"
#include "drv_model_pub.h"
#include "param_config.h"
#include "aon_pmu_driver.h"

#define TAG "sys"

void bk_reboot(void)
{
	UINT32 wdt_val = 5;

	BK_LOGI(TAG, "bk_reboot\r\n");

#if (CONFIG_SOC_BK7256XX) || (CONFIG_SOC_BK7256_CP1)
	set_reboot_tag(REBOOT_TAG_REQ);
#endif
#if (CONFIG_SYSTEM_CTRL)
	aon_pmu_drv_wdt_rst_dev_enable();
#endif
	bk_misc_update_set_type(RESET_SOURCE_REBOOT);

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	bk_wdt_stop();
	BK_LOGI(TAG, "wdt reboot\r\n");
#if (CONFIG_SOC_BK7231N) || (CONFIG_SOC_BK7236) || (CONFIG_SOC_BK7256XX)
	delay_ms(100); //add delay for bk_writer BEKEN_DO_REBOOT cmd
#endif
	bk_wdt_start(wdt_val);
	while (1);
	GLOBAL_INT_RESTORE();
}
