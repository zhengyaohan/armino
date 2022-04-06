/*
 * File      : main.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <stdio.h>
#include <board.h>
#include "BK_System.h"
#include "platform.h"

#if ((CFG_SOC_NAME == SOC_BK7231N) || (CFG_SOC_NAME == SOC_BK7236))
unsigned int bk72 __attribute__((at(0x100))) = (0x32374B42);
unsigned int b31  __attribute__((at(0x104))) = (0x00003133);
#endif

#if (CFG_SOC_NAME == SOC_BK7271)
const unsigned int bk7271[2] __attribute__((at(0x100)))={0x32374B42,0x00003137};
const unsigned int bk7271_sign_ptr __attribute__((at(0x110)))={0x00100000};
#endif
extern u32 startup;
extern u32 uart_buff_write;
extern u8 bim_uart_rx_buf[512];
static u16 bim_uart_temp, uart_buff_read;
static int  check_cnt = 0;
extern void reset_timer(void);
extern void system_startup(void);
extern void bk_printf(const char *fmt);
extern void bk_print_hex(unsigned int num);
extern void wdt_reboot();
extern void boot_uart_data_callback( u8 *buff, u16 len);

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
    *((volatile unsigned long *)    (0x44010000+0x20*4)) |= 0x1<<4;
    *((volatile unsigned long *)    (0x44010000+0xc*4)) |= 0x4;


#if (CFG_SOC_NAME != SOC_BK7256)
    __disable_irq();
    __disable_fiq();
    intc_forbidden_all();
    int_init();
#endif
    rt_hw_board_init();
	reset_timer();   

#if (CFG_SOC_NAME != SOC_BK7256)
    intc_start();
    /* enable exceptions */
    __enable_irq();
#else
	clear_csr(NDS_MIE, MIP_MEIP | MIP_MTIP | MIP_MSIP); 
	arch_interrupt_enable();
	set_csr(NDS_MIE, MIP_MEIP);			/* Enable the Machine External/Timer/Sofware interrupt bit in MIE. */
	set_csr(NDS_MSTATUS, MSTATUS_MIE);	/* Enable interrupts in general. */
#endif
#if 0
u32 reg;
reg = REG_READ(0x44000000+0x2*4);
if(reg&0x10000)
{
	while(1);
}
*((volatile unsigned long *)    (0x44000000+0x2*4)) |= 0x1 << 16;  //reset flag
*((volatile unsigned long *)    (0x44000000+0x2*4)) |= 0x1 << 7;   //wdt reset not clr PMU
#endif

    //printf_flash_ID();
    #if ((CFG_SOC_NAME != SOC_BK7231N) && (CFG_SOC_NAME != SOC_BK7236))
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
                if(check_cnt++ > 100000 && 0 == uart_buff_write)  //13000-->100000
                {
                    //break;
                }
            }
            else
            {
                if(check_cnt++ > 400000)
                {
                    #if (CFG_SOC_NAME != SOC_BK7256)
                    sys_forbidden_interrupts();
                    #endif
                    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_IN_BOOT_RESET1_VALUE;
                    wdt_reboot();
                }
            }
        }
    }
    #endif

	
    extern u32 boot_downloading;
    boot_downloading = FALSE;
    startup = TRUE;
    system_startup();
		
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


#define  ARM_REG_SAVED_SVC_START    0x400020
#define  ARM_REG_SAVED_SVC_LEN      0x40
#define  ARM_REG_SAVED_SVC_END      (ARM_REG_SAVED_SVC_START + ARM_REG_SAVED_SVC_LEN)
#define  ARM_REG_SAVED_IRQ_START    0x400064
#define  ARM_REG_SAVED_IRQ_LEN      0x24
#define  ARM_REG_SAVED_IRQ_END      (ARM_REG_SAVED_IRQ_START + ARM_REG_SAVED_IRQ_LEN)
#define  ARM_REG_SAVED_FIQ_START    ARM_REG_SAVED_IRQ_END
#define  ARM_REG_SAVED_FIQ_LEN      0x24
#define  ARM_REG_SAVED_FIQ_END      (ARM_REG_SAVED_FIQ_START + ARM_REG_SAVED_FIQ_LEN)
#define  ARM_REG_SAVED_ABT_START    ARM_REG_SAVED_FIQ_END
#define  ARM_REG_SAVED_ABT_LEN      0x24
#define  ARM_REG_SAVED_ABT_END      (ARM_REG_SAVED_ABT_START + ARM_REG_SAVED_ABT_LEN)
#define  ARM_REG_SAVED_UND_START    ARM_REG_SAVED_ABT_END
#define  ARM_REG_SAVED_UND_LEN      0x24
#define  ARM_REG_SAVED_UND_END      (ARM_REG_SAVED_UND_START + ARM_REG_SAVED_UND_LEN)
#define  ARM_REG_SAVED_SYS_START    ARM_REG_SAVED_UND_END
#define  ARM_REG_SAVED_SYS_LEN      0x20
#define  ARM_REG_SAVED_SYS_END      (ARM_REG_SAVED_SYS_START + ARM_REG_SAVED_SYS_LEN)

void bk_show_register (void)
{
    int i;
    unsigned int *reg1;

    bk_printf("\r\nIRQ:");
    reg1 = ( unsigned int *)ARM_REG_SAVED_IRQ_START;
    for(i = 0; i <(ARM_REG_SAVED_IRQ_LEN >> 2); i++)
    {
        if((i == 0) || (i == 1) || (i == 7) || (i == 8))
        {
			bk_print_hex(*(reg1 + i));
			bk_printf("  ");
        }
    }

    bk_printf("\r\nFIR:");
    reg1 = ( unsigned int *)ARM_REG_SAVED_FIQ_START;
    for(i = 0; i <(ARM_REG_SAVED_FIQ_LEN >> 2); i++)
    {
        if((i == 0) || (i == 1) || (i == 7) || (i == 8))
			{
				bk_print_hex(*(reg1 + i));
				bk_printf("  ");
			}
    }
#if 0
    bk_printf("\r\nABT:");
    reg1 = ( unsigned int *)ARM_REG_SAVED_ABT_START;
    for(i = 0; i <(ARM_REG_SAVED_ABT_LEN >> 2); i++)
    {
        if((i == 0) || (i == 1) || (i == 7) || (i == 8))
			{
				bk_print_hex(*(reg1 + i));
				bk_printf("  ");
			}
    }

    bk_printf("\r\nUND:");
    reg1 = ( unsigned int *)ARM_REG_SAVED_UND_START;
    for(i = 0; i <(ARM_REG_SAVED_UND_LEN >> 2); i++)
    {
        if((i == 0) || (i == 1) || (i == 7) || (i == 8))
			{
				bk_print_hex(*(reg1 + i));
				bk_printf("  ");
			}
    }
#endif
    reg1 = ( unsigned int *)ARM_REG_SAVED_SYS_START;
    bk_printf("\r\nSYS:");
    for(i = 0; i <(ARM_REG_SAVED_SYS_LEN >> 2); i++)
    {
        if((i == 0) || (i == 6) || (i == 7))
			{
				bk_print_hex(*(reg1 + i));
				bk_printf("  ");
			}
        else if(i == 1)
            bk_printf("            ");
    }
}

void reset_register_dump(void)
{	
	#if (CFG_SOC_NAME == SOC_BK7256)
	bk_printf("BK7256");
	bk_printf("_1.0.11");
	#else
    const struct reset_register *reg = (const struct reset_register *)ARM_REG_SAVED_SVC_START;
    #if (CFG_SOC_NAME == SOC_BK7221U)
    bk_printf("BK7251");
    #elif(CFG_SOC_NAME == SOC_BK7231U)
    bk_printf("BK7231u");
    #elif(CFG_SOC_NAME == SOC_BK7231N)
    bk_printf("BK7231n");
    #elif(CFG_SOC_NAME == SOC_BK7271)
    bk_printf("BK7271");
    #elif(CFG_SOC_NAME == SOC_BK7236)
    bk_printf("BK7236");
    #else
    bk_printf("BK7231");
    #endif
    bk_printf("_1.0.11");
    bk_printf("\r\nREG:cpsr        spsr        r13         r14\r\n");
    bk_printf("SVC:");
    bk_print_hex(reg->cpsr);
    bk_printf("            ");
    bk_printf("  ");
    bk_print_hex(reg->r13);
    bk_printf("  ");
    bk_print_hex(reg->r14);
    bk_show_register();
    bk_printf("\r\nST:");
    bk_print_hex(*((uint32_t *)START_TYPE_ADDR));
	#endif
    bk_printf("\r\n");
}
// eof
