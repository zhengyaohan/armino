#ifndef _LOW_POWER_MANAGE_API_H_
#define _LOW_POWER_MANAGE_API_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "sys_types.h"
#include <driver/int.h>

typedef enum
{
   GPIO_TRIGGER_INTERRUPE_LEVEL_LOW_ACTIVE = 0,
   GPIO_TRIGGER_INTERRUPE_LEVEL_HIGH_ACTIVE,
   GPIO_TRIGGER_INTERRUPE_EDGE_RISING,
   GPIO_TRIGGER_INTERRUPE_EDGE_FALLING,
}gpio_trigger_interrupt_type_e;

typedef struct
{
  uint32_t gpio_id;
  gpio_trigger_interrupt_type_e gpio_trigger_interrupt_type;
  int_group_isr_t isr_callback;
}gpio_wakeup_param_t;
typedef struct
{
    uint32_t  period;
    int_group_isr_t isr_callback;
}rtc_wakeup_param_t;
typedef enum
{
    WIFI_WAKEUP = 1,
    BT_WAKEUP,
}wifi_bt_wakeup_type_e;
typedef struct
{
    wifi_bt_wakeup_type_e  wifi_bt_wakeup;
    int_group_isr_t isr_callback;
}system_wakeup_param_t;
typedef struct
{
    int_group_isr_t isr_callback;
}usbplug_wakeup_param_t;
typedef struct
{
	uint32_t  touch_channel;
    int_group_isr_t isr_callback;
}touch_wakeup_param_t;
/*low_power_init*/
void low_power_init();

/*low power hardware init*/
void low_power_hardware_init();

/*set_sleep_mode */
void low_power_set_sleep_mode(low_power_sleep_mode_e sleep_mode);

/*wakeup_source_set*/
void low_power_wakeup_source_set(wakeup_source_t wakeup_source, void* source_param);

void low_power_module_enter_sleep_ctrl(power_module_name_t module,uint32_t sleep_state);
/*power control*/
/*1. power module ctrl:power on or power off*/
void low_power_power_ctrl(power_module_name_t module,power_module_state_t power_state);

/*2. power lowvol_ctrl low voltage on or lowvol_off*/
void low_power_lowvol_ctrl(power_module_name_t module,lowvol_module_state_t lowvol_state);


/*clock control*/
/*1. module clock enable*/
void low_power_clock_ctrl(dev_clk_pwr_id_t module,dev_clk_pwr_ctrl_t clock_state);
/*2. module clock switch*/
void low_power_clock_switch(dev_clk_pwr_id_t name,int clksel,int clkdiv);
/*3. core_bus_clock_enable*/
void low_power_core_bus_clock_ctrl(high_clock_module_name_t core, int clksel,int clkdiv, high_clock_module_name_t bus,int bus_clksel,int bus_clkdiv);

/*rf control*/
/*1. RF power on or power off */
void low_power_rf_power_ctrl (module_name_t module,power_module_state_t power_state);
/*2. RF power switch */
void low_power_rf_switch(module_name_t name);

/*enter low power manage*/
int low_power_suppress_ticks_and_sleep(UINT32 sleep_ticks);
#ifdef __cplusplus
}
#endif

#endif
