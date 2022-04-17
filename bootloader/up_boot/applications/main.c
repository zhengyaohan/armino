/*
 * File      : main.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <board.h>

#include "BK_System.h"
#include "board.h"
//#include "rt_heap.h"
#include "driver_flash.h"
#include "drv_uart.h"
#include "interrupt.h"

#if (CFG_SOC_NAME != SOC_BK7256)

extern u32 startup;
extern u32 uart_buff_write;
extern u32 uart_download_status;
extern u8 bim_uart_rx_buf[512];
extern uint32_t boot_version_a;

const char *boot_version = (char *)&boot_version_a;
static u16 bim_uart_temp, uart_buff_read;
static int  check_cnt = 0;

extern void reset_register_dump(void);
extern void wdt_reboot(void);
extern int jump_to_app(void);
extern int ota_main(void);

#define MALLOC_HEAP_SIZE (128*1024)
int malloc_heap[MALLOC_HEAP_SIZE/4] = {0}; 
#pragma import (__use_realtime_heap) 
unsigned __rt_heap_extend(unsigned size, void **block) 
{   
    return 0;  
}

void gpio_13_high()
{
    REG_WRITE((0x0802800 +(13*4)),(0x2));
}

int boot_main(void)
{
    #if (CFG_SOC_NAME == SOC_BK7221U)
    // for fix bk7221u reset
    gpio_13_high();
    #endif
    
    __disable_irq();
    __disable_fiq();
    intc_forbidden_all();

    rt_hw_board_init();
    reset_timer();
    intc_start();

    __enable_irq();

    reset_register_dump();

    while(1)
    {
        bim_uart_temp = uart_buff_write;
        if (uart_buff_read < bim_uart_temp)
        {
            boot_uart_data_callback(bim_uart_rx_buf + uart_buff_read, bim_uart_temp - uart_buff_read);
            uart_buff_read = bim_uart_temp;
            check_cnt = 0;

        }
        else if (uart_buff_read > bim_uart_temp)
        {
            boot_uart_data_callback(bim_uart_rx_buf + uart_buff_read, sizeof(bim_uart_rx_buf) - uart_buff_read);
            boot_uart_data_callback(bim_uart_rx_buf, bim_uart_temp);
            uart_buff_read = bim_uart_temp;
            check_cnt = 0;
        }
        else
        {
            if(uart_download_status == 0 )
            {
                if(check_cnt++ > 13000 && 0 == uart_buff_write)
                {
                    break;
                }
            }
            else
            {
                if(check_cnt++ > 400000)
                {
                    sys_forbidden_interrupts();
                    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_IN_BOOT_RESET1_VALUE;
                    wdt_reboot();
                }
            }
        }
    }
	
    #if (CFG_SUPPORT_OS == OS_RTTOS)
    extern u32 boot_downloading;
    boot_downloading = FALSE;
    startup = TRUE;

    // Import the runtime and initialize the C heap 
    _init_alloc((unsigned)&malloc_heap[0], (unsigned)(&malloc_heap[0]) + sizeof(malloc_heap) - 1); 
    
    system_startup();
    #endif
		
		return 0;
}


struct reset_register
{
    uint32_t cpsr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10; // sl stack limit
    uint32_t r11; // fp frame pointer
    uint32_t r12; // ip Intra-Procedure-call scratch register
    uint32_t r13; // sp Stack Pointer.
    uint32_t r14; // lr Link Register.
};

void reset_register_dump(void)
{
    const struct reset_register *reg = (const struct reset_register *)0x400020;

    printf("\r\nV:");
    printf(boot_version);
    printf("\r\nCPSR:");
    bk_print_hex(reg->cpsr);
    #if 1
    printf("\r\nR0:");
    bk_print_hex(reg->r0);
    printf("\r\nR1:");
    bk_print_hex(reg->r1);
    printf("\r\nR2:");
    bk_print_hex(reg->r2);
    printf("\r\nR3:");
    bk_print_hex(reg->r3);
    printf("\r\nR4:");
    bk_print_hex(reg->r4);
    printf("\r\nR13:");
    bk_print_hex(reg->r13);
    printf("\r\nR14(LR):");
    bk_print_hex(reg->r14);
    printf("\r\nST:");
    bk_print_hex(*((uint32_t *)START_TYPE_ADDR));
    printf("\r\n");
    #endif
}
// eof
#else
//extern void rt_hw_board_init(void);
//int boot_main(void)
 extern void __attribute__((section(".itcm_write_flash"))) rt_hw_board_init(void);
int __attribute__((section(".itcm_write_flash")))  boot_main(void)
{
	rt_hw_board_init();	
    //heap_init();
    bk_printf("rt_hw_board_init finish  \r\n");
	system_timeout_startup();

    return 0;
}

#endif


