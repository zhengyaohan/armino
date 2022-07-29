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

#include "sys_hal.h"
#include "sys_driver.h"

//sys_ctrl CMD: CMD_QSPI_VDDRAM_VOLTAGE
void sys_drv_set_qspi_vddram_voltage(uint32_t param)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_set_qspi_vddram_voltage(param);

	rtos_enable_int(int_level);
}

//sys_ctrl CMD: CMD_QSPI_IO_VOLTAGE
void sys_drv_set_qspi_io_voltage(uint32_t param)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_set_qspi_io_voltage(param);

	rtos_enable_int(int_level);
}

/**  psram Start **/
uint32_t sys_drv_psram_volstage_sel(uint32_t value)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_volstage_sel(value);
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_xtall_osc_enable(uint32_t value)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_xtall_osc_enable(value);
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_dco_enable(uint32_t value)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_doc_enable(value);
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_dpll_enable(uint32_t value)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_dpll_enable(value);
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_ldo_enable(uint32_t value)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_ldo_enable(value);
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_psram_power_enable(void)
{
	uint32_t int_level = rtos_disable_int();

	sys_hal_psram_power_enable();
	rtos_enable_int(int_level);
	return SYS_DRV_SUCCESS;
}


/**  psram End **/

