#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_icu.h"
#include "driver_gpio.h"
#include "drv_timer.h"
#include "drv_pwm.h"
#include "driver_flash.h"
#include "crc.h"

#define SYSTEM_TIMEOUT_STARTUP   (400)

static u32 reset;
u32 boot_downloading = TRUE;
u32 startup = FALSE;

void system_startup(void)
{
    reset_register_dump();
    
    reset = UP_BOOT_ADDR;
    //bk_printf("\r\nboot");
    //bk_print_hex(reset);
    //bk_printf("...\r\n");

    #if (DEBUG_PORT_UART0 == PRINT_PORT)
    uart0_wait_tx_finish();
	#elif (DEBUG_PORT_UART2 == PRINT_PORT) && (SOC_BK7271 == CFG_SOC_NAME)
	uart2_wait_tx_finish();
    #else
    uart1_wait_tx_finish();
    #endif

    sys_forbidden_interrupts();

    jump_pc_address(reset);
    while(1);
}

void system_timeout_startup(void)
{

    if((1 == uart_download_status) )
    {
        return;
    }

    if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
    {
        boot_downloading = FALSE;
        system_startup();
    }
}
// eof
