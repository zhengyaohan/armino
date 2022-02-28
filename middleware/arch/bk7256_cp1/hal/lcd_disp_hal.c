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

#include "lcd_disp_hal.h"
#include "lcd_disp_ll.h"

bk_err_t lcd_hal_init(lcd_disp_hal_t *hal)
{
	hal->hw = (lcd_disp_hw_t *)LCD_LL_REG_BASE(hal->id);

	return BK_OK;
}

bk_err_t lcd_hal_display_enable(lcd_disp_hal_t *hal, bool en)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_display_enable(hal->hw, en);
#elif defined LCD_RGB_IF
	lcd_display_ll_rgb_enable(hal->hw, en);
#endif
//printf lcd interface neither 8080 and rgb
	return BK_OK;
}


bk_err_t lcd_hal_data_transfer_start(lcd_disp_hal_t *hal, bool start_transfer)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_data_start_transfer(hal->hw, start_transfer);
#endif

	return BK_OK;
}


bk_err_t lcd_hal_set_thrd(lcd_disp_hal_t *hal, uint32_t thrd_type, uint32_t threshold_val)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_thrd_set(hal->hw, thrd_type, threshold_val);
#elif defined LCD_RGB_IF
	lcd_display_ll_rgb_thrd_set(hal->hw, thrd_type, threshold_val);
#else
	return BK_FAIL;
#endif

	return BK_OK;
}


bk_err_t lcd_hal_write_cmd(lcd_disp_hal_t *hal, uint16_t cmd)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_cmd_send(hal->hw, cmd);
#endif

	return BK_OK;
}

bk_err_t lcd_hal_write_data(lcd_disp_hal_t *hal, uint16_t data)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_data_send(hal->hw, data);
#endif

	return BK_OK;
}


bk_err_t lcd_hal_reset_befor_cmd_init(lcd_disp_hal_t *hal)
{
#ifdef LCD_8080_IF
	lcd_display_ll_8080_reset_in_sleep(hal->hw, 0);
#endif
	return BK_OK;
}








